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

#include <QFile>

#include "ffmpeg_headers.h"
#include "ffmpegvideoencoder.h"
#include "videoencodingprofiles.h"
#include "audioplayer.h"
#include "audioplayerprivate.h"


class FFMpegVideoEncoderPriv
{
	public:
		FFMpegVideoEncoderPriv();
		~FFMpegVideoEncoderPriv();

		bool	createFile( const QString& filename );
		bool	close();
		int		encodeImage( const QImage & img, qint64 time );
		void	flush();

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
        int     encodeMoreAudio();
        void    initAVpacket(AVPacket *packet);
        bool    convertImage_sws(const QImage &img);
        bool    encodeFrame(AVFrame *frame, AVCodecContext *output_codec_context, AVStream *stream);

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

        // Video conversion context
		SwsContext			*	videoConvertCtx;

        // Audio resample context
        SwrContext          *   audioResampleCtx;

        // Output file has been opened successfully
		bool					outputFileOpened;

		// Video frame PTS
		unsigned int			videoFrameNumber;
        unsigned int			audioSamplesOut;

		// Total output size
        unsigned int			outputTotalSize;
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

	audioCodecCtx = 0;
	videoImageBuffer = 0;
	videoConvertCtx = 0;
	outputFileOpened = false;
}

FFMpegVideoEncoderPriv::~FFMpegVideoEncoderPriv()
{
	close();
}

bool FFMpegVideoEncoderPriv::close()
{
	if ( outputFormatCtx )
	{
		if ( outputFileOpened )
		{
			flush();
			av_write_trailer( outputFormatCtx );

			// Close the file
			avio_close( outputFormatCtx->pb );

            // close video and audio codecs
            avcodec_free_context( &videoCodecCtx );
            avcodec_free_context( &audioCodecCtx );
		}

		// free the streams
		for ( unsigned int i = 0; i < outputFormatCtx->nb_streams; i++ )
		{
            av_freep(&outputFormatCtx->streams[i]->codecpar);
			av_freep(&outputFormatCtx->streams[i]);
		}

		// Free the format
        avformat_free_context( outputFormatCtx );
	}

	delete[] videoImageBuffer;

	if ( videoFrame )
		av_free(videoFrame);

    //FIXME
    //if ( audioResampleCtx )
    //audioResampleCtx = 0;

	outputFormatCtx = 0;
	outputFormat = 0;
	videoCodecCtx = 0;
	videoStream = 0;
	audioStream = 0;
	videoCodec = 0;
	videoFrame = 0;
	videoImageBuffer = 0;
	videoConvertCtx = 0;
    audioResampleCtx = 0;

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

void FFMpegVideoEncoderPriv::initAVpacket( AVPacket  * packet )
{
    // Prepare the packet
    av_init_packet( packet );

    // av_init_packet does not do this
    packet->data = NULL;
    packet->size = 0;
}

void FFMpegVideoEncoderPriv::flush()
{
    encodeFrame( nullptr, audioCodecCtx, audioStream );
    encodeFrame( nullptr, videoCodecCtx, videoStream );
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
    videoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	videoCodecCtx->bit_rate = m_videobitrate;
	videoCodecCtx->bit_rate_tolerance = m_videobitrate * av_q2d(videoCodecCtx->time_base);

	// Set up the colorspace
	if ( m_videoformat->colorspace == 601 )
		videoCodecCtx->colorspace = ( 576 % videoCodecCtx->height ) ? AVCOL_SPC_SMPTE170M : AVCOL_SPC_BT470BG;
	else if ( m_videoformat->colorspace == 709 )
		videoCodecCtx->colorspace = AVCOL_SPC_BT709;

	// Enable interlacing if needed
	if ( m_videoformat->flags & VIFO_INTERLACED )
        videoCodecCtx->flags |= AV_CODEC_FLAG_INTERLACED_DCT;

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
        videoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

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
    if ( avcodec_parameters_from_context(  videoStream->codecpar, videoCodecCtx ) < 0 )
    {
        m_errorMsg = "Failed to copy encoder parameters to output stream";
        goto cleanup;
    }

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
            //audioStream->pts.num = origAudioStream->pts.num;
            //audioStream->pts.den = origAudioStream->pts.den;

            //AVCodecContext * newCtx = audioStream->codec;

			// We're copying the stream
            avcodec_parameters_copy( audioStream->codecpar, origAudioStream->codecpar );
            /*
            if ( newCtx->block_align == 1 && newCtx->codec_id == AV_CODEC_ID_MP3 )
				newCtx->block_align= 0;

            if ( newCtx->codec_id == AV_CODEC_ID_AC3 )
                newCtx->block_align= 0;*/
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
            if ( audioCodec->id == AV_CODEC_ID_AC3 && avcodec_find_encoder_by_name( "ac3_fixed" ) )
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
            audioCodecCtx->sample_rate = m_aplayer->aCodecCtx->sample_rate;
			audioCodecCtx->channel_layout = av_get_channel_layout( m_profile->channels == 1 ? "mono" : "stereo" );
			audioCodecCtx->time_base.num = 1;
			audioCodecCtx->time_base.den = m_profile->sampleRate;

			if ( outputFormatCtx->oformat->flags & AVFMT_GLOBALHEADER )
                audioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			// Since different audio codecs support different sample formats, look up which one is supported by this specific codec
			if ( isAudioSampleFormatSupported( audioCodec->sample_fmts, AV_SAMPLE_FMT_FLTP ) )
				audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
			else if ( isAudioSampleFormatSupported( audioCodec->sample_fmts, AV_SAMPLE_FMT_S16 ) )
				audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
			else if ( isAudioSampleFormatSupported( audioCodec->sample_fmts, AV_SAMPLE_FMT_S16P ) )
				audioCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16P;
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

            // Specify the coder for the stream
            if ( avcodec_parameters_from_context(  audioStream->codecpar, audioCodecCtx ) < 0 )
            {
                m_errorMsg = "Failed to copy encoder parameters to output audio stream";
                goto cleanup;
            }

			// Setup the audio resampler
            audioResampleCtx = swr_alloc();

            if ( !audioResampleCtx )
            {
                m_errorMsg = QObject::tr("Cannot allocate audio resampler");
                return false;
            }

			// Some formats (i.e. WAV) do not produce the proper channel layout
			if ( m_aplayer->aCodecCtx->channel_layout == 0 )
                av_opt_set_channel_layout( audioResampleCtx, "in_channel_layout", av_get_channel_layout( m_profile->channels == 1 ? "mono" : "stereo" ), 0 );
			else
                av_opt_set_channel_layout( audioResampleCtx, "in_channel_layout", m_aplayer->aCodecCtx->channel_layout, 0 );

            av_opt_set_channel_layout( audioResampleCtx, "out_channel_layout", audioCodecCtx->channel_layout, 0 );

			av_opt_set_int( audioResampleCtx, "in_sample_fmt",     m_aplayer->aCodecCtx->sample_fmt, 0);
            av_opt_set_int( audioResampleCtx, "out_sample_fmt",     audioCodecCtx->sample_fmt, 0);
			av_opt_set_int( audioResampleCtx, "in_sample_rate",    m_aplayer->aCodecCtx->sample_rate, 0);
            av_opt_set_int( audioResampleCtx, "out_sample_rate",    audioCodecCtx->sample_rate, 0);

            if ( swr_init( audioResampleCtx ) < 0 )
            {
                m_errorMsg = QObject::tr("Cannot initialize audio resampler");
                return false;
            }
		}

		// Rewind the audio player
		m_aplayer->reset();
	}

	// Allocate the buffer for the picture
    size = av_image_get_buffer_size( videoCodecCtx->pix_fmt, videoCodecCtx->width, videoCodecCtx->height, 1 );
	videoImageBuffer = new uint8_t[size];

	if ( !videoImageBuffer )
	{
		m_errorMsg = "Could not open allocate picture buffer";
		goto cleanup;
	}

	// Allocate the YUV frame
    videoFrame = av_frame_alloc();

	if ( !videoFrame )
	{
		m_errorMsg = "Could not open allocate picture frame buffer";
		goto cleanup;
	}

	// Reset the PTS
	videoFrame->pts = 0;
    videoFrame->width = videoCodecCtx->width;
    videoFrame->height = videoCodecCtx->height;
    videoFrame->format = videoCodecCtx->pix_fmt;

    // Setup the planes
    av_image_fill_arrays( videoFrame->data, videoFrame->linesize, videoImageBuffer,videoCodecCtx->pix_fmt, videoCodecCtx->width, videoCodecCtx->height, 1 );

	// Create the file and write the header
    outputFormatCtx->url = av_strdup( FFMPEG_FILENAME( fileName ) );

	if ( avio_open( &outputFormatCtx->pb, FFMPEG_FILENAME( fileName ), AVIO_FLAG_WRITE) < 0 )
	{
		m_errorMsg = "Could not create the video file";
		goto cleanup;
	}

    if ( avformat_write_header( outputFormatCtx, 0 ) < 0 )
    {
        m_errorMsg = "Could not write the video file";
        goto cleanup;
    }

/*
	// Dump output streams
	for ( unsigned int i = 0; i < outputFormatCtx->nb_streams; i++)
	{
        qDebug( "Output stream %d: %s %.02f FPS, ", i, outputFormatCtx->streams[i]->name,
                ((double) outputFormatCtx->streams[i]->time_base.den / outputFormatCtx->streams[i]->time_base.num) );

		if ( outputFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			qDebug("width %d height %d bitrate %d\n", outputFormatCtx->streams[i]->codec->width,
				   outputFormatCtx->streams[i]->codec->height, outputFormatCtx->streams[i]->codec->bit_rate );

		if ( outputFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
			qDebug( "channels %d sample_rate %d bitrate %d\n", outputFormatCtx->streams[i]->codec->channels,
					outputFormatCtx->streams[i]->codec->sample_rate,
					outputFormatCtx->streams[i]->codec->bit_rate );
	}
*/
	videoFrameNumber = 0;
    audioSamplesOut = 0;
	outputTotalSize = 0;
	outputFileOpened = true;

	return true;

cleanup:
    close();
    return false;

}


/**
 * Encode one frame worth of audio or video to the output file.
 * @param      frame                 Samples to be encoded
 * @param      output_format_context Format context of the output file
 * @param      output_codec_context  Codec context of the output file
 * @param[out] data_present          Indicates whether data has been
 *                                   encoded
 * @return The number of bytes sent, or negative value in case of error
 */
bool FFMpegVideoEncoderPriv::encodeFrame( AVFrame *frame, AVCodecContext *output_codec_context, AVStream * stream )
{
    int error = 0;

    // Mark frame writable so it is not refcounted (and thus encoder would make a copy if it needs)
    if ( frame )
        av_frame_make_writable( frame );

    // Send the audio frame stored in the temporary packet to the encoder.
    // The output audio stream encoder is used to do this.
    error = avcodec_send_frame( output_codec_context, frame );

    // The encoder signals that it has nothing more to encode.
    if (error == AVERROR_EOF)
    {
        error = 0;
        goto cleanup;
    }
    else if (error < 0)
    {
        qWarning( "Could not send packet for encoding (error '%d')", error );
        return -1;
    }

    // We repeat this process until we get EOF
    while ( true )
    {
        // Packet used for temporary storage.
        AVPacket output_packet;

        initAVpacket( &output_packet );

        // Receive one encoded frame from the encoder.
        error = avcodec_receive_packet(output_codec_context, &output_packet);

        // If the encoder asks for more data to be able to provide an encoded frame, return indicating that no data is present.
        if ( error == AVERROR(EAGAIN) )
        {
            error = 0;
            break;
        }

        // If the last frame has been encoded, stop encoding.
        if (error == AVERROR_EOF)
        {
            error = 0;
            break;
        }

        if ( error < 0 )
        {
            qWarning( "Could not encode frame (error '%d')", error );
            break;
        }

        // Set up the packet index
        output_packet.stream_index = stream->index;

        // Convert the PTS from the packet base to stream base
        if ( output_packet.pts != AV_NOPTS_VALUE )
            output_packet.pts = av_rescale_q( output_packet.pts, output_codec_context->time_base, stream->time_base );

        // Convert the DTS from the packet base to stream base
        if ( output_packet.dts != AV_NOPTS_VALUE )
            output_packet.dts = av_rescale_q( output_packet.dts, output_codec_context->time_base, stream->time_base );

        if ( output_packet.duration > 0 )
            output_packet.duration = av_rescale_q( output_packet.duration, output_codec_context->time_base, stream->time_base );

        outputTotalSize += output_packet.size;

        // Write one audio frame from the temporary packet to the output file.
        // av_write_frame takes over the packet and unrefs it itself
        if ( (error = av_write_frame( outputFormatCtx, &output_packet)) < 0)
        {
            qWarning( "Could not write frame (error '%d)", error );
            goto cleanup;
        }
    }

cleanup:
    return error == 0;
}


int FFMpegVideoEncoderPriv::encodeMoreAudio()
{
    AVPacket inpkt;
    AVFrame * decodedAudioFrame = 0;
    int ret = -1;

    // Read until we've read the audio frame
    while ( true )
    {
        // Read an audio frame
        initAVpacket( &inpkt );

        if ( av_read_frame( m_aplayer->pFormatCtx, &inpkt ) < 0 )
        {
            av_packet_unref( &inpkt );
            return 0;  // Frame read failed (e.g. end of stream)
        }

        // Skip non-audio frames
        if ( inpkt.stream_index == m_aplayer->audioStream )
            break;
    }

    // Send the packet with the compressed data to the decoder
    if ( avcodec_send_packet( m_aplayer->aCodecCtx, &inpkt ) < 0)
    {
        qWarning( "Error while submitting audio packet to decoder" );
        goto cleanup;
    }

    // Read all the output frames (in general there may be any number of them)
    decodedAudioFrame = av_frame_alloc();

    while ( avcodec_receive_frame( m_aplayer->aCodecCtx, decodedAudioFrame ) >= 0 )
    {
        // Output audio frame
        AVFrame * resampledAudioframe = av_frame_alloc();

        av_frame_copy_props( resampledAudioframe, decodedAudioFrame );
        resampledAudioframe->channel_layout = audioCodecCtx->channel_layout;
        resampledAudioframe->channels = audioCodecCtx->channels;
        resampledAudioframe->sample_rate = audioCodecCtx->sample_rate;
        resampledAudioframe->format = audioCodecCtx->sample_fmt;

        // Run the audio resampling
        if ( swr_convert_frame( audioResampleCtx, resampledAudioframe, decodedAudioFrame ) < 0 )
        {
            qWarning( "Error while converting the audio frame" );
            goto cleanup;
        }

        // Assign the PTS to the audio frame
      /*  AVRational rate;
        rate.num = 1;
        rate.den = audioCodecCtx->sample_rate;

        output_frame->pts = av_rescale_q( audioSamplesOut, rate, audioCodecCtx->time_base);*/
        resampledAudioframe->pts = audioSamplesOut;
        audioSamplesOut += resampledAudioframe->nb_samples;

        bool ret = encodeFrame( resampledAudioframe, audioCodecCtx, audioStream );
        av_frame_unref( resampledAudioframe );

        if ( !ret )
            goto cleanup;


/*
            // Rescale output packet timestamp values from codec to stream timebase
            outpkt.pts = av_rescale_q_rnd( outpkt.pts, audioCodecCtx->time_base, audioStream->time_base, (AVRounding) (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX) );
            outpkt.dts = av_rescale_q_rnd( outpkt.dts, audioCodecCtx->time_base, audioStream->time_base, (AVRounding) (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX) );
            outpkt.duration = av_rescale_q( outpkt.duration, audioCodecCtx->time_base, audioStream->time_base);
*/
    }

    // We sent out the whole packet; return success
    ret = 1;

cleanup:
    if ( decodedAudioFrame )
        av_frame_unref( decodedAudioFrame );

    return ret;
}


int FFMpegVideoEncoderPriv::encodeImage( const QImage &img, qint64 )
{
    int err;

    // Do we need to output audio?
    if ( m_aplayer )
    {
        double video_time = ((double) videoFrameNumber * videoCodecCtx->time_base.num) / videoCodecCtx->time_base.den;
        double audio_time = ((double) audioSamplesOut * audioCodecCtx->time_base.num) / audioCodecCtx->time_base.den;

        while ( audio_time <= video_time )
        {
            //qDebug("Progress: A %g, V %g", audio_time, video_time );

            // Output more audio if we're behind video
            err = encodeMoreAudio();

            if ( err == 0 )
                break; // audio stream ended
            else if ( err < 0 )
                return -1; // error

            // Recalculate
            audio_time = ((double) audioSamplesOut * audioCodecCtx->time_base.num) / audioCodecCtx->time_base.den;
            //qDebug("After encode: A %g, V %g", audio_time, video_time );
        }
    }

    // Convert Qt image into FFMpeg frame (videoFrame)
    convertImage_sws( img );

    // Setup frame data
    videoFrame->interlaced_frame = (m_videoformat->flags & VIFO_INTERLACED) ? 1 : 0;
    videoFrame->pts = videoFrameNumber++;

    //qDebug("Video time: %g", ((double) videoFrameNumber * videoCodecCtx->time_base.num) / videoCodecCtx->time_base.den );

    if ( !encodeFrame( videoFrame, videoCodecCtx, videoStream ) )
        return -1;

    return outputTotalSize;
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

    if ( img.format() != QImage::Format_RGB32 && img.format() != QImage::Format_ARGB32 )
	{
		printf("Wrong image format\n");
		return false;
	}

	videoConvertCtx = sws_getCachedContext( videoConvertCtx,
										   m_videoformat->width,
										   m_videoformat->height,
                                           AV_PIX_FMT_BGRA,
										   m_videoformat->width,
										   m_videoformat->height,
                                           AV_PIX_FMT_YUV420P,
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
