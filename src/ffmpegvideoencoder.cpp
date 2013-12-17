/**************************************************************************
 *  Karlyriceditor - a lyrics editor and CD+G / video export for Karaoke  *
 *  songs.                                                                *
 *  Copyright (C) 2009-2013 George Yunaev, support@ulduzsoft.com          *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *																	      *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

/**************************************************************************
 * This class uses ideas from QTFFmpegWrapper - QT FFmpeg Wrapper Class   *
 * Copyright (C) 2009,2010: Daniel Roggen, droggen@gmail.com              *
 * All rights reserved.                                                   *
 **************************************************************************/

#include "ffmpeg_headers.h"
#include "ffmpegvideoencoder.h"
#include "videoencodingprofiles.h"
#include "audioplayer.h"
#include "audioplayerprivate.h"


#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio


class FFMpegVideoEncoderPriv
{
	public:
		FFMpegVideoEncoderPriv();
		~FFMpegVideoEncoderPriv();

		bool	createFile( const QString& filename );
		bool	close();
		int		encodeImage( const QImage & img, qint64 time );

	public:
		// Video output parameters
		const VideoEncodingProfile * m_profile;
		const VideoFormat		   * m_videoformat;
		bool						 m_convertaudio;
		unsigned int				 m_videobitrate;
		unsigned int				 m_audiobitrate;

		// Do we also have an audio source?
		AudioPlayerPrivate * m_aplayer;

		// Error message
		QString		m_errorMsg;

	private:
		bool convertImage_sws(const QImage &img);

		// FFmpeg stuff
		AVFormatContext		*	outputFormatCtx;
		AVOutputFormat		*	outputFormat;
		AVCodecContext		*	videoCodecCtx;
		AVCodecContext		*	audioCodecCtx;
		AVStream			*	videoStream;
		AVStream			*	audioStream;
		AVCodec				*	videoCodec;
		AVCodec				*	audioCodec;

		// Video frame data
		AVFrame				*	videoFrame;
		uint8_t				*	videoImageBuffer;

		// Audio frame data
		AVFrame				*	audioFrame;
		uint8_t				*	audioSampleBuffer;
		unsigned int			audioSampleBuffer_size;

		// Conversion
		SwsContext			*	videoConvertCtx;

		// FIXME: Audio resample
		AVAudioResampleContext* audioResampleCtx;

		// File has been outputFileOpened successfully
		bool					outputFileOpened;

		// Video frame PTS
		unsigned int			videoFrameNumber;
		unsigned int			audioFrameNumber;
};


static bool isAudioSampleFormatSupported( const enum AVSampleFormat * sample_formats, AVSampleFormat format )
{
	while ( *sample_formats != AV_SAMPLE_FMT_NONE )
	{
		if ( *sample_formats == format )
			return true;

		sample_formats++;
	}

	return false;
}


FFMpegVideoEncoderPriv::FFMpegVideoEncoderPriv()
{
	ffmpeg_init_once();

	outputFormatCtx = 0;
	outputFormat = 0;
	videoCodecCtx = 0;
	videoStream = 0;
	audioStream = 0;
	audioCodec = 0;
	videoCodec = 0;
	videoFrame = 0;
	audioResampleCtx = 0;

	audioFrame = 0;
	audioCodecCtx = 0;
	videoImageBuffer = 0;
	audioSampleBuffer = 0;
	videoConvertCtx = 0;
	outputFileOpened = false;
}

FFMpegVideoEncoderPriv::~FFMpegVideoEncoderPriv()
{
	close();
}

bool FFMpegVideoEncoderPriv::createFile( const QString& fileName )
{
	int err, size;

	// If we had an open video, close it.
	close();

av_log_set_level(AV_LOG_VERBOSE);

	// Find the output container format
	outputFormat = av_guess_format( qPrintable( m_profile->videoContainer ), qPrintable( m_profile->videoContainer ), 0 );

	if ( !outputFormat )
	{
		m_errorMsg = QString("Could not guess the output format %1") .arg( m_profile->videoContainer );
		goto cleanup;
	}

	// Allocate the output context
	outputFormatCtx = avformat_alloc_context();

	if ( !outputFormatCtx )
	{
		m_errorMsg = "Error allocating format context";
		goto cleanup;
	}

	outputFormatCtx->oformat = outputFormat;

	// Find the video encoder
	videoCodec = avcodec_find_encoder_by_name( qPrintable(m_profile->videoCodec) );

	if ( !videoCodec )
	{
		m_errorMsg = QString( "Video codec %1 is not found") .arg( m_profile->videoCodec );
		goto cleanup;
	}

	// Allocate the video codec context
	videoCodecCtx = avcodec_alloc_context3( videoCodec );

	if ( !videoCodecCtx )
	{
		m_errorMsg = QString( "Context for video codec %1 cannot be allocated") .arg( m_profile->videoCodec );
		goto cleanup;
	}

	// Set the video encoding parameters
	videoCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	videoCodecCtx->width = m_videoformat->width;
	videoCodecCtx->height = m_videoformat->height;
	videoCodecCtx->sample_aspect_ratio.den = m_videoformat->sample_aspect_den;
	videoCodecCtx->sample_aspect_ratio.num = m_videoformat->sample_aspect_num;
	videoCodecCtx->time_base.num = m_videoformat->frame_rate_num;
	videoCodecCtx->time_base.den = m_videoformat->frame_rate_den;
	videoCodecCtx->gop_size = (m_videoformat->frame_rate_den / m_videoformat->frame_rate_num) / 2;	// GOP size is framerate / 2
	videoCodecCtx->pix_fmt = PIX_FMT_YUV420P;
	videoCodecCtx->bit_rate = m_videobitrate;
	videoCodecCtx->bit_rate_tolerance = m_videobitrate * av_q2d(videoCodecCtx->time_base);

	// Set up the colorspace
	if ( m_videoformat->colorspace == 601 )
		videoCodecCtx->colorspace = ( 576 % videoCodecCtx->height ) ? AVCOL_SPC_SMPTE170M : AVCOL_SPC_BT470BG;
	else if ( m_videoformat->colorspace == 709 )
		videoCodecCtx->colorspace = AVCOL_SPC_BT709;

	// Enable interlacing if needed
	if ( m_videoformat->flags & VIFO_INTERLACED )
		videoCodecCtx->flags |= CODEC_FLAG_INTERLACED_DCT;

	// Enable multithreaded encoding: breaks FLV!
	//videoCodecCtx->thread_count = 4;

	// Video format-specific hacks
	switch ( videoCodec->id )
	{
		case AV_CODEC_ID_H264:
			av_opt_set( videoCodecCtx->priv_data, "preset", "slow", 0 );
			break;

		case AV_CODEC_ID_MPEG2VIDEO:
			videoCodecCtx->max_b_frames = 2;
			videoCodecCtx->bit_rate_tolerance = m_videobitrate * av_q2d(videoCodecCtx->time_base) * 2;
			break;

		case AV_CODEC_ID_MPEG1VIDEO:
			// Needed to avoid using macroblocks in which some coeffs overflow.
			videoCodecCtx->mb_decision = 2;
			break;

		default:
			break;
	}

	// If we have a global header for the format, no need to duplicate the codec info in each keyframe
	if ( outputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER )
		videoCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

	// Open the codec
	if ( ( err = avcodec_open2( videoCodecCtx, videoCodec, 0 )) < 0 )
	{
		m_errorMsg = QString("Could not open video codec: error %1") .arg( err );
		goto cleanup;
	}

	// Create the video stream, index
	videoStream = avformat_new_stream( outputFormatCtx, videoCodecCtx->codec );

	if ( !videoStream )
	{
		m_errorMsg = "Could not allocate video stream";
		goto cleanup;
	}

	// Specify the coder for the stream
	videoStream->codec = videoCodecCtx;

	// Set the video stream timebase if not set
	if ( videoStream->time_base.den == 0 )
		videoStream->time_base = videoCodecCtx->time_base;

	// Do we also have audio stream?
	if ( m_aplayer )
	{
		// Are we copying the stream data?
		if ( !m_convertaudio )
		{
			// Add the audio stream, index 1
			audioStream = avformat_new_stream( outputFormatCtx, 0 );

			if ( !audioStream )
			{
				m_errorMsg = "Could not allocate audio stream";
				goto cleanup;
			}

			AVStream * origAudioStream = m_aplayer->pFormatCtx->streams[m_aplayer->audioStream];

			audioStream->time_base = origAudioStream->time_base;
			audioStream->disposition = origAudioStream->disposition;
			audioStream->pts.num = origAudioStream->pts.num;
			audioStream->pts.den = origAudioStream->pts.den;

			AVCodecContext * newCtx = audioStream->codec;

			// We're copying the stream
			memcpy( newCtx, m_aplayer->aCodecCtx, sizeof(AVCodecContext) );

			if ( newCtx->block_align == 1 && newCtx->codec_id == CODEC_ID_MP3 )
				newCtx->block_align= 0;

			if ( newCtx->codec_id == CODEC_ID_AC3 )
				newCtx->block_align= 0;
		}
		else
		{
			// Find the audio codec
			audioCodec = avcodec_find_encoder_by_name( qPrintable( m_profile->audioCodec ) );

			if ( !audioCodec )
			{
				m_errorMsg = QString("Could not use the audio codec %1") .arg( m_profile->audioCodec );
				goto cleanup;
			}

			// Hack to use the fixed AC3 codec if available
			if ( audioCodec->id == CODEC_ID_AC3 && avcodec_find_encoder_by_name( "ac3_fixed" ) )
				audioCodec = avcodec_find_encoder_by_name( "ac3_fixed" );

			// Allocate the audio context
			audioCodecCtx = avcodec_alloc_context3( audioCodec );

			if ( !audioCodecCtx )
			{
				m_errorMsg = QString( "Context for audio codec %1 cannot be allocated") .arg( m_profile->audioCodec );
				goto cleanup;
			}

			audioCodecCtx->codec_id = audioCodec->id;
			audioCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
			audioCodecCtx->bit_rate = m_audiobitrate;
			audioCodecCtx->channels = m_profile->channels;
			audioCodecCtx->sample_rate = m_profile->sampleRate;
			audioCodecCtx->channel_layout = av_get_channel_layout( m_profile->channels == 1 ? "mono" : "stereo" );
			audioCodecCtx->time_base.num = 1;
			audioCodecCtx->time_base.den = m_profile->sampleRate;

			if ( outputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER )
				audioCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

			// Since different audio codecs support different sample formats, look up which one is supported by this specific codec
			if ( isAudioSampleFormatSupported( audioCodec->sample_fmts, AV_SAMPLE_FMT_FLTP ) )
			{
				qDebug("Audio format %s: using AV_SAMPLE_FMT_FLTP", qPrintable( m_profile->audioCodec ) );
				audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
			}
			else if ( isAudioSampleFormatSupported( audioCodec->sample_fmts, AV_SAMPLE_FMT_S16 ) )
			{
				qDebug("Audio format %s: using AV_SAMPLE_FMT_S16", qPrintable( m_profile->audioCodec ) );
				audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
			}
			else if ( isAudioSampleFormatSupported( audioCodec->sample_fmts, AV_SAMPLE_FMT_S16P ) )
			{
				qDebug("Audio format %s: using AV_SAMPLE_FMT_S16P", qPrintable( m_profile->audioCodec ) );
				audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16P;
			}
			else
			{
				QString supported;

				for ( const enum AVSampleFormat * fmt = audioCodec->sample_fmts; *fmt != AV_SAMPLE_FMT_NONE; fmt++ )
					supported+= QString(" %1") .arg( *fmt );

				m_errorMsg = QString("Could not find the sample format supported by the audio codec %1; supported: %2") . arg( m_profile->audioCodec ) .arg(supported);
				goto cleanup;
			}

			// Open the audio codec
			err = avcodec_open2( audioCodecCtx, audioCodec, 0 );
			if ( err < 0 )
			{
				m_errorMsg = QString("Could not open the audio codec: %1") . arg( err );
				goto cleanup;
			}

			// Allocate the audio stream
			audioStream = avformat_new_stream( outputFormatCtx, audioCodec );

			if ( !audioStream )
			{
				m_errorMsg = "Could not allocate audio stream";
				goto cleanup;
			}

			// Remember the codec for the stream
			audioStream->codec = audioCodecCtx;

			// Setup the audio resampler
			audioResampleCtx = avresample_alloc_context();

			if ( !audioResampleCtx )
			{
				m_errorMsg = QString("Could not open the audio resampler");
				goto cleanup;
			}

			av_opt_set_int( audioResampleCtx, "in_channel_layout",	m_aplayer->aCodecCtx->channel_layout, 0 );
			av_opt_set_int( audioResampleCtx, "in_sample_fmt",     m_aplayer->aCodecCtx->sample_fmt, 0);
			av_opt_set_int( audioResampleCtx, "in_sample_rate",    m_aplayer->aCodecCtx->sample_rate, 0);
			av_opt_set_int( audioResampleCtx, "in_channels",       m_aplayer->aCodecCtx->channels,0);
			av_opt_set_int( audioResampleCtx, "out_channel_layout", audioCodecCtx->channel_layout, 0);
			av_opt_set_int( audioResampleCtx, "out_sample_fmt",     audioCodecCtx->sample_fmt, 0);
			av_opt_set_int( audioResampleCtx, "out_sample_rate",    audioCodecCtx->sample_rate, 0);
			av_opt_set_int( audioResampleCtx, "out_channels",       audioCodecCtx->channels, 0);

			err = avresample_open( audioResampleCtx );
			if ( err < 0 )
			{
				m_errorMsg = QString("Could not open the audio resampler: %1") . arg( err );
				goto cleanup;
			}

			audioFrame = avcodec_alloc_frame();

			if ( !audioFrame )
			{
				m_errorMsg = "Could not allocate audio frame";
				goto cleanup;
			}

			audioFrame->nb_samples = audioStream->codec->frame_size;
			audioFrame->format = audioStream->codec->sample_fmt;
			audioFrame->channel_layout = audioStream->codec->channel_layout;

			// Tthe codec gives us the frame size, in samples,we calculate the size of the samples buffer in bytes
			audioSampleBuffer_size = av_samples_get_buffer_size( NULL, audioCodecCtx->channels, audioCodecCtx->frame_size, audioCodecCtx->sample_fmt, 0 );
			audioSampleBuffer = (uint8_t*) av_malloc( audioSampleBuffer_size );

			if ( !audioSampleBuffer )
			{
				m_errorMsg = "Could not allocate audio buffer";
				goto cleanup;
			}

			// Setup the data pointers in the AVFrame
			if ( avcodec_fill_audio_frame( audioFrame, audioStream->codec->channels, audioStream->codec->sample_fmt, (const uint8_t*) audioSampleBuffer, audioSampleBuffer_size, 0 ) < 0 )
			{
				m_errorMsg = "Could not set up audio frame";
				goto cleanup;
			}

			if ( audioStream->codec->block_align == 1 && audioStream->codec->codec_id == CODEC_ID_MP3 )
				audioStream->codec->block_align= 0;

			if ( audioStream->codec->codec_id == CODEC_ID_AC3 )
				audioStream->codec->block_align= 0;
		}

		// Rewind the audio player
		m_aplayer->reset();
	}

	// Allocate the buffer for the picture
	size = avpicture_get_size( videoCodecCtx->pix_fmt, videoCodecCtx->width, videoCodecCtx->height );
	videoImageBuffer = new uint8_t[size];

	if ( !videoImageBuffer )
	{
		m_errorMsg = "Could not open allocate picture buffer";
		goto cleanup;
	}

	// Allocate the YUV frame
	videoFrame = avcodec_alloc_frame();

	if ( !videoFrame )
	{
		m_errorMsg = "Could not open allocate picture frame buffer";
		goto cleanup;
	}

	// Reset the PTS
	videoFrame->pts = 0;

	// Setup the planes
	avpicture_fill( (AVPicture *)videoFrame, videoImageBuffer,videoCodecCtx->pix_fmt, videoCodecCtx->width, videoCodecCtx->height );

	// Create the file and write the header
	strncpy( outputFormatCtx->filename, FFMPEG_FILENAME( fileName ), sizeof(outputFormatCtx->filename) );

	if ( avio_open( &outputFormatCtx->pb, FFMPEG_FILENAME( fileName ), AVIO_FLAG_WRITE) < 0 )
	{
		m_errorMsg = "Could not create the video file";
		goto cleanup;
	}

	avformat_write_header( outputFormatCtx, 0 );

	// Dump output streams
	for ( unsigned int i = 0; i < outputFormatCtx->nb_streams; i++)
	{
		qDebug( "Output stream %d: %s %.02f FPS, ", i, outputFormatCtx->streams[i]->codec->codec->name,
				((double) outputFormatCtx->streams[i]->codec->time_base.den / outputFormatCtx->streams[i]->codec->time_base.num) );

		if ( outputFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			qDebug("width %d height %d bitrate %d\n", outputFormatCtx->streams[i]->codec->width,
				   outputFormatCtx->streams[i]->codec->height, outputFormatCtx->streams[i]->codec->bit_rate );

		if ( outputFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
			qDebug( "channels %d sample_rate %d bitrate %d\n", outputFormatCtx->streams[i]->codec->channels,
					outputFormatCtx->streams[i]->codec->sample_rate,
					outputFormatCtx->streams[i]->codec->bit_rate );
	}

	videoFrame->pts = 0;

	videoFrameNumber = 0;
	audioFrameNumber = 0;
	outputFileOpened = true;
	return true;

cleanup:
	close();
	return false;
}

bool FFMpegVideoEncoderPriv::close()
{
	if ( outputFormatCtx )
	{
		if ( outputFileOpened )
		{
			av_write_trailer( outputFormatCtx );

			// close video and audio
			avcodec_close( videoStream->codec );
			avcodec_close( audioStream->codec );

			// Close the file
			avio_close( outputFormatCtx->pb );
		}

		// free the streams
		for ( unsigned int i = 0; i < outputFormatCtx->nb_streams; i++ )
		{
			av_freep(&outputFormatCtx->streams[i]->codec);
			av_freep(&outputFormatCtx->streams[i]);
		}

		// Free the format
		av_free( outputFormatCtx );
	}

	delete[] videoImageBuffer;
	delete[] audioSampleBuffer;

	if ( videoFrame )
		av_free(videoFrame);

	if ( audioFrame )
		av_free( audioFrame );

	outputFormatCtx = 0;
	outputFormat = 0;
	videoCodecCtx = 0;
	videoStream = 0;
	audioStream = 0;
	videoCodec = 0;
	videoFrame = 0;
	audioFrame = 0;
	videoImageBuffer = 0;
	audioSampleBuffer = 0;
	videoConvertCtx = 0;

	return true;
}

int FFMpegVideoEncoderPriv::encodeImage( const QImage &img, qint64 )
{
	int outsize = 0;
	AVPacket pkt;
	int got_packet;

	// If we have audio, first add all audio packets for this time
	if ( m_aplayer )
	{
		while ( true )
		{
			double audio_pts = (double) audioStream->pts.val * av_q2d( audioStream->time_base );
			double video_pts = (double) videoStream->pts.val * av_q2d( videoStream->time_base );

			qDebug( "PTS check: A: %g V: %g", audio_pts, video_pts );

/*			qint64 audio_timestamp = (audioStream->pts.val * (qint64) audioStream->time_base.num * 1000) / audioStream->time_base.den;

			if ( audio_timestamp >= time )
				break;
*/
			if ( video_pts < audio_pts )
				break;

			AVPacket pkt;

			// Read a frame
			if ( av_read_frame( m_aplayer->pFormatCtx, &pkt ) < 0 )
				return false;  // Frame read failed (e.g. end of stream)

			if ( pkt.stream_index != m_aplayer->audioStream )
			{
				av_free_packet( &pkt );
				continue;
			}

			// Initialize the frame
			AVFrame srcaudio;
			avcodec_get_frame_defaults( &srcaudio );

			// Decode the original audio into the srcaudio frame
			int got_audio;
			int err = avcodec_decode_audio4( m_aplayer->aCodecCtx, &srcaudio, &got_audio, &pkt );

			if ( err < 0 )
			{
				qWarning( "Error decoding audio frame: %d", err );
				return -1;
			}

			// We don't need the AV packet anymore
			av_free_packet( &pkt );

			// Next packet if we didn't get audio
			if ( !got_audio )
				continue;

			// Resample the input into the audioSampleBuffer until we proceed the whole decoded data
			if ( (err = avresample_convert( audioResampleCtx,
											NULL,
											0,
											0,
											srcaudio.data,
											0,
											srcaudio.nb_samples )) < 0 )
			{
				qWarning( "Error resampling decoded audio: %d", err );
				return -1;
			}

			while( avresample_available( audioResampleCtx ) >= audioFrame->nb_samples )
			{
				// Read a frame audio data from the resample fifo
				if ( avresample_read( audioResampleCtx, audioFrame->data, audioFrame->nb_samples ) != audioFrame->nb_samples )
				{
					qWarning( "Error reading resampled audio: %d", err );
					return -1;
				}

				// Prepare the packet
				av_init_packet( &pkt );

				// audioFrame->pts
				//audioFrame->pts = audioFrameNumber;
				//audioFrameNumber += audioFrame->nb_samples;
				//audioFrame->pts = av_rescale_q( time,AV_TIME_BASE_Q, audioCodecCtx->time_base );

				qDebug("Audio time: %g", ((double) audioFrameNumber * audioCodecCtx->time_base.num) / audioCodecCtx->time_base.den );

				//audioFrame->pts = audioFrameNumber;
				//audioFrameNumber += audioFrame->nb_samples;
				//destaudio->pts = first_pts + (int)((double)audio_samples * (90000.0/out_acodec->sample_rate));

				// and encode the audio into the audiopkt
				avcodec_encode_audio2( audioCodecCtx, &pkt, audioFrame, &got_packet );

				if ( got_packet )
				{
					// Set up the packet index
					pkt.stream_index = audioStream->index;

					// Newer ffmpeg versions do it anyway, but just in case
					pkt.flags |= AV_PKT_FLAG_KEY;

					if ( audioCodecCtx->coded_frame && audioCodecCtx->coded_frame->pts != AV_NOPTS_VALUE )
						pkt.pts = av_rescale_q( audioCodecCtx->coded_frame->pts, audioCodecCtx->time_base, audioStream->time_base );

					// And write the file
					av_interleaved_write_frame( outputFormatCtx, &pkt );
					outsize += pkt.size;

					av_free_packet( &pkt );
				}
				else
				{
					qDebug("Frame was not encoded");
				}
			}
		}
	}

	// SWS conversion
	convertImage_sws(img);

	// Setup frame data
	videoFrame->interlaced_frame = (m_videoformat->flags & VIFO_INTERLACED) ? 1 : 0;
	videoFrame->pts = videoFrameNumber++;

	qDebug("Video time: %g", ((double) videoFrameNumber * videoCodecCtx->time_base.num) / videoCodecCtx->time_base.den );

	av_init_packet( &pkt );
	pkt.data = NULL;
	pkt.size = 0;

	int ret = avcodec_encode_video2( videoCodecCtx, &pkt, videoFrame, &got_packet );
	if ( ret < 0 )
	{
		qWarning( "Error encoding video: %d", ret );
		return -1;
	}

	if ( got_packet )
	{
		// Convert the PTS from the packet base to stream base
		if ( pkt.pts != AV_NOPTS_VALUE )
			pkt.pts = av_rescale_q( pkt.pts, videoCodecCtx->time_base, videoStream->time_base );

		// Convert the DTS from the packet base to stream base
		if ( pkt.dts != AV_NOPTS_VALUE )
			pkt.dts = av_rescale_q( pkt.dts, videoCodecCtx->time_base, videoStream->time_base );

		if ( pkt.duration > 0 )
			pkt.duration = av_rescale_q( pkt.duration, videoCodecCtx->time_base, videoStream->time_base );

		if ( videoCodecCtx->coded_frame->key_frame )
			pkt.flags |= AV_PKT_FLAG_KEY;

		pkt.stream_index = videoStream->index;

		int ret = av_interleaved_write_frame( outputFormatCtx, &pkt );

		if ( ret < 0 )
			return -1;

		outsize += pkt.size;
		av_free_packet( &pkt );
	}

	return outsize;
}



/**
  \brief Convert the QImage to the internal YUV format

  SWS conversion

   Caution: the QImage is allocated by QT without guarantee about the alignment and bytes per lines.
   It *should* be okay as we make sure the image is a multiple of many bytes (8 or 16)...
   ... however it is not guaranteed that sws_scale won't at some point require more bytes per line.
   We keep the custom conversion for that case.

**/
bool FFMpegVideoEncoderPriv::convertImage_sws(const QImage &img)
{
	// Check if the image matches the size
	if ( img.width() != (int) m_videoformat->width || img.height() != (int) m_videoformat->height )
	{
		printf("Wrong image size!\n");
		return false;
	}

	if ( img.format()!=QImage::Format_RGB32	&& img.format() != QImage::Format_ARGB32 )
	{
		printf("Wrong image format\n");
		return false;
	}

	videoConvertCtx = sws_getCachedContext( videoConvertCtx,
										   m_videoformat->width,
										   m_videoformat->height,
										   PIX_FMT_BGRA,
										   m_videoformat->width,
										   m_videoformat->height,
										   PIX_FMT_YUV420P,
										   SWS_BICUBIC,
										   NULL,
										   NULL,
										   NULL );
	if ( videoConvertCtx == NULL )
	{
		printf("Cannot initialize the conversion context\n");
		return false;
	}

	uint8_t *srcplanes[3];
	srcplanes[0]=(uint8_t*)img.bits();
	srcplanes[1]=0;
	srcplanes[2]=0;

	int srcstride[3];
	srcstride[0]=img.bytesPerLine();
	srcstride[1]=0;
	srcstride[2]=0;

	sws_scale( videoConvertCtx, srcplanes, srcstride,0, m_videoformat->height, videoFrame->data, videoFrame->linesize);
	return true;
}



FFMpegVideoEncoder::FFMpegVideoEncoder()
{
	d = new FFMpegVideoEncoderPriv();
}

FFMpegVideoEncoder::~FFMpegVideoEncoder()
{
	delete d;
}

bool FFMpegVideoEncoder::close()
{
	return d->close();
}

int FFMpegVideoEncoder::encodeImage( const QImage & img, qint64 time )
{
	return d->encodeImage( img, time );
}


QString FFMpegVideoEncoder::createFile( const QString &filename,
										const VideoEncodingProfile *profile,
										const VideoFormat * videoformat,
										unsigned int quality,
										bool  convert_audio,
										AudioPlayer *audio )
{
	d->m_aplayer = audio ? audio->impl() : 0;
	d->m_profile = profile;
	d->m_videoformat = videoformat;
	d->m_convertaudio = convert_audio;

	d->m_videobitrate = profile->bitratesVideo[quality] * 1000;
	d->m_audiobitrate = profile->bitratesAudio[quality] * 1000;

	if ( d->createFile( filename ) )
		return QString();

	return d->m_errorMsg;
}
