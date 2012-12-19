/**************************************************************************
 *  Karlyriceditor - a lyrics editor and CD+G / video export for Karaoke  *
 *  songs.                                                                *
 *  Copyright (C) 2009-2011 George Yunaev, support@karlyriceditor.com     *
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
 **************************************************************************
 * This class uses ideas from QTFFmpegWrapper - QT FFmpeg Wrapper Class   *
 * Copyright (C) 2009,2010: Daniel Roggen, droggen@gmail.com              *
 * All rights reserved.                                                   *
 **************************************************************************/

#include "ffmpeg_headers.h"
#include "ffmpegvideoencoder.h"
#include "audioplayer.h"
#include "audioplayerprivate.h"


class FFMpegVideoEncoderPriv
{
	public:
		FFMpegVideoEncoderPriv();
		~FFMpegVideoEncoderPriv();

		bool createFile( const QString& filename, const QString& outformat );
		bool close();
		int encodeImage( const QImage & img);

	public:
		// Video output parameters
		unsigned int	m_width;
		unsigned int	m_height;
		unsigned int	m_videobitrate;
		unsigned int	m_videogop; // maximal interval in frames between keyframes
		unsigned int	m_time_base_num;
		unsigned int	m_time_base_den;

		// Do we also have an audio source?
		AudioPlayerPrivate * m_aplayer;

		// Error message
		QString		m_errorMsg;

	private:
		bool convertImage_sws(const QImage &img);

		// FFmpeg stuff
		AVFormatContext *pOutputCtx;
		AVOutputFormat *pOutputFormat;
		AVCodecContext *pVideoCodecCtx;
		AVStream *pVideoStream;
		AVStream *pAudioStream;
		AVCodec *pCodec;

		// Frame data
		AVFrame *ppicture;
		uint8_t *picture_buf;

		// Compressed data
		int outbuf_size;
		uint8_t* outbuf;

		// Conversion
		SwsContext *img_convert_ctx;
};


FFMpegVideoEncoderPriv::FFMpegVideoEncoderPriv()
{
	ffmpeg_init_once();

	pOutputCtx = 0;
	pOutputFormat = 0;
	pVideoCodecCtx = 0;
	pVideoStream = 0;
	pAudioStream = 0;
	pCodec = 0;
	ppicture = 0;
	outbuf = 0;
	picture_buf = 0;
	img_convert_ctx = 0;
}

FFMpegVideoEncoderPriv::~FFMpegVideoEncoderPriv()
{
	close();
}

bool FFMpegVideoEncoderPriv::createFile( const QString& fileName, const QString& outformat )
{
	int err, size;

	// If we had an open video, close it.
	close();

	// Check sizes
	if ( m_width % 8 )
		return false;

	if ( m_height % 8 )
		return false;

	// Allocate output format
	//pOutputFormat = av_guess_format( FFMPEG_FILENAME( outformat ), FFMPEG_FILENAME( fileName ), 0 );
	pOutputFormat = av_guess_format( 0, FFMPEG_FILENAME( fileName ), 0 );

	if ( !pOutputFormat )
	{
		m_errorMsg = QString("Could not guess the output format %1") .arg(outformat);
		goto cleanup;
	}

	pOutputCtx = avformat_alloc_context();

	if ( !pOutputCtx )
	{
		m_errorMsg = "Error allocating format context";
		goto cleanup;
	}

	pOutputCtx->oformat = pOutputFormat;
	strncpy( pOutputCtx->filename, FFMPEG_FILENAME( fileName ), sizeof(pOutputCtx->filename) );

	// Add the video stream, index 0
	pVideoStream = avformat_new_stream( pOutputCtx, 0 );

	if ( !pVideoStream )
	{
		m_errorMsg = "Could not allocate video stream";
		goto cleanup;
	}

	pVideoCodecCtx = pVideoStream->codec;
	pVideoCodecCtx->codec_id = pOutputFormat->video_codec;
	pVideoCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;

	pVideoCodecCtx->bit_rate = m_videobitrate;
	pVideoCodecCtx->width = m_width;
	pVideoCodecCtx->height = m_height;
	pVideoCodecCtx->time_base.num = m_time_base_num;
	pVideoCodecCtx->time_base.den = m_time_base_den;
	pVideoCodecCtx->gop_size = m_videogop;
	pVideoCodecCtx->pix_fmt = PIX_FMT_YUV420P;

	//pVideoCodecCtx->bit_rate_tolerance = pVideoCodecCtx->bit_rate * av_q2d(pVideoCodecCtx->time_base);

	// Do we also have audio stream?
	if ( m_aplayer )
	{
		// Add the audio stream, index 1
		pAudioStream = avformat_new_stream( pOutputCtx, 0 );

		if ( !pAudioStream )
		{
			m_errorMsg = "Could not allocate audio stream";
			goto cleanup;
		}

		// Copy the stream data
		AVStream * origAudioStream = m_aplayer->pFormatCtx->streams[m_aplayer->audioStream];

		pAudioStream->time_base = origAudioStream->time_base;
		pAudioStream->disposition = origAudioStream->disposition;
		pAudioStream->pts.num = origAudioStream->pts.num;
		pAudioStream->pts.den = origAudioStream->pts.den;

		AVCodecContext * oldCtx = m_aplayer->aCodecCtx;
		AVCodecContext * newCtx = pAudioStream->codec;

		// We're copying the stream
		newCtx->codec_id = oldCtx->codec_id;
		newCtx->codec_type = oldCtx->codec_type;
		newCtx->codec_tag = oldCtx->codec_tag;
		newCtx->bit_rate = oldCtx->bit_rate;
		newCtx->extradata= oldCtx->extradata;
		newCtx->extradata_size= oldCtx->extradata_size;
		newCtx->time_base = oldCtx->time_base;
		newCtx->channel_layout = oldCtx->channel_layout;
		newCtx->sample_rate = oldCtx->sample_rate;
		newCtx->sample_fmt = oldCtx->sample_fmt;
		newCtx->channels = oldCtx->channels;
		newCtx->frame_size = oldCtx->frame_size;
		newCtx->block_align= oldCtx->block_align;

		if ( newCtx->block_align == 1 && newCtx->codec_id == CODEC_ID_MP3 )
			newCtx->block_align= 0;

		if ( newCtx->codec_id == CODEC_ID_AC3 )
			newCtx->block_align= 0;

		// Rewind the audio player
		m_aplayer->reset();
	}

	pVideoCodecCtx->thread_count = 10;

	// some formats want stream headers to be separate
	if ( pOutputCtx->oformat->flags & AVFMT_GLOBALHEADER )
		pVideoCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

	// find the video encoder
	pCodec = avcodec_find_encoder( pVideoCodecCtx->codec_id );

	if ( !pCodec )
	{
		m_errorMsg = "Video codec not found";
		goto cleanup;
	}

	// open the codec
	if ( ( err = avcodec_open( pVideoCodecCtx, pCodec )) < 0 )
	{
		m_errorMsg = QString("Could not open video codec: error %1") .arg( err );
		goto cleanup;
	}

	// Allocate memory for output
	outbuf_size = m_width * m_height * 3;
	outbuf = new uint8_t[ outbuf_size ];

	if ( !outbuf )
	{
		m_errorMsg = "Could not open allocate output buffer";
		goto cleanup;
	}

	// Allocate the YUV frame
	ppicture = avcodec_alloc_frame();

	if ( !ppicture )
	{
		m_errorMsg = "Could not open allocate picture frame buffer";
		goto cleanup;
	}

	size = avpicture_get_size( pVideoCodecCtx->pix_fmt, pVideoCodecCtx->width, pVideoCodecCtx->height );
	picture_buf = new uint8_t[size];

	if ( !picture_buf )
	{
		m_errorMsg = "Could not open allocate picture buffer";
		goto cleanup;
	}

	// Setup the planes
	avpicture_fill( (AVPicture *)ppicture, picture_buf,pVideoCodecCtx->pix_fmt, pVideoCodecCtx->width, pVideoCodecCtx->height );

	if ( avio_open( &pOutputCtx->pb, FFMPEG_FILENAME( fileName ), AVIO_FLAG_WRITE) < 0 )
	{
		m_errorMsg = "Could not create the video file";
		goto cleanup;
	}

	avformat_write_header(pOutputCtx, 0);
	return true;

cleanup:
	if ( pOutputCtx )
	{
		// free the streams
		for ( int i = 0; i < pOutputCtx->nb_streams; i++ )
		{
			av_freep(&pOutputCtx->streams[i]->codec);
			av_freep(&pOutputCtx->streams[i]);
		}

		// Free the format
		av_free( pOutputCtx );
		pOutputCtx = 0;
	}

	if ( outbuf )
	{
		delete[] outbuf;
		outbuf = 0;
	}

	if ( picture_buf )
	{
		delete[] picture_buf;
		picture_buf = 0;
	}

	if ( ppicture )
	{
		av_free(ppicture);
		ppicture = 0;
	}

	return false;
}

bool FFMpegVideoEncoderPriv::close()
{
	if ( pOutputCtx )
	{
		av_write_trailer( pOutputCtx );

		// close video
		avcodec_close( pVideoStream->codec );

		// free the streams
		for ( int i = 0; i < pOutputCtx->nb_streams; i++ )
		{
			av_freep(&pOutputCtx->streams[i]->codec);
			av_freep(&pOutputCtx->streams[i]);
		}

		// Close file
		avio_close(pOutputCtx->pb);

		// Free the format
		av_free( pOutputCtx );
	}

	if ( outbuf )
		delete[] outbuf;

	if ( picture_buf )
		delete[] picture_buf;

	if ( ppicture )
		av_free(ppicture);

	pOutputCtx = 0;
	pOutputFormat = 0;
	pVideoCodecCtx = 0;
	pVideoStream = 0;
	pAudioStream = 0;
	pCodec = 0;
	ppicture = 0;
	outbuf = 0;
	picture_buf = 0;
	img_convert_ctx = 0;

	return true;
}

int FFMpegVideoEncoderPriv::encodeImage( const QImage &img )
{
	int outsize = 0;
	AVPacket pkt;
	double audio_pts, video_pts = (double) pVideoStream->pts.val * pVideoStream->time_base.num / pVideoStream->time_base.den;

	// If we have audio, first add all audio packets
	if ( m_aplayer )
	{
		audio_pts = (double)pAudioStream->pts.val * pAudioStream->time_base.num / pAudioStream->time_base.den;

		while ( audio_pts < video_pts )
		{
			AVPacket audiopkt;

			// Read a frame from audio stream
			if ( av_read_frame( m_aplayer->pFormatCtx, &audiopkt ) < 0 )
				break;

			if ( audiopkt.stream_index != m_aplayer->audioStream )
			{
				av_free_packet( &audiopkt );
				continue;
			}

			// Store the packet
			av_init_packet( &pkt );

			pkt.data = audiopkt.data;
			pkt.size = audiopkt.size;
			pkt.stream_index = pAudioStream->index;
			pkt.flags |= AV_PKT_FLAG_KEY;

			int ret = av_interleaved_write_frame( pOutputCtx, &pkt );

			// and free it
			av_free_packet( &audiopkt );

			if ( ret < 0 )
				return -1;

			outsize += pkt.size;

			// Recalculate audio_pts
			audio_pts = (double)pAudioStream->pts.val * pAudioStream->time_base.num / pAudioStream->time_base.den;
		}
	}

	// SWS conversion
	convertImage_sws(img);

	int out_size = avcodec_encode_video(pVideoCodecCtx,outbuf,outbuf_size,ppicture);

	if (out_size > 0)
	{
		av_init_packet(&pkt);

		if ( pVideoCodecCtx->coded_frame->pts != (0x8000000000000000LL) )
			pkt.pts= av_rescale_q(pVideoCodecCtx->coded_frame->pts, pVideoCodecCtx->time_base, pVideoStream->time_base);

		if ( pVideoCodecCtx->coded_frame->key_frame )
			pkt.flags |= AV_PKT_FLAG_KEY;

		pkt.stream_index = pVideoStream->index;
		pkt.data = outbuf;
		pkt.size = out_size;

		int ret = av_interleaved_write_frame(pOutputCtx, &pkt);

		if ( ret < 0 )
			return -1;

		outsize += pkt.size;
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
	if ( img.width() != m_width || img.height() != m_height )
	{
		printf("Wrong image size!\n");
		return false;
	}

	if ( img.format()!=QImage::Format_RGB32	&& img.format() != QImage::Format_ARGB32 )
	{
		printf("Wrong image format\n");
		return false;
	}

	img_convert_ctx = sws_getCachedContext( img_convert_ctx,
										   m_width,
										   m_height,
										   PIX_FMT_BGRA,
										   m_width,
										   m_height,
										   PIX_FMT_YUV420P,
										   SWS_BICUBIC,
										   NULL,
										   NULL,
										   NULL );
	if ( img_convert_ctx == NULL )
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

	sws_scale( img_convert_ctx, srcplanes, srcstride,0, m_height, ppicture->data, ppicture->linesize);
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

int FFMpegVideoEncoder::encodeImage( const QImage & img)
{
	return d->encodeImage( img );
}

QString FFMpegVideoEncoder::createFile( const QString& filename, const QString& outformat, QSize size,
									unsigned int videobitrate, unsigned int time_base_num,
									unsigned int time_base_den, unsigned int gop, AudioPlayer * audio )
{
	d->m_aplayer = audio ? audio->impl() : 0;
	d->m_width = size.width();
	d->m_height = size.height();
	d->m_videobitrate = videobitrate;
	d->m_time_base_den = time_base_den;
	d->m_time_base_num = time_base_num;
	d->m_videogop = gop;

	if ( d->createFile( filename, outformat ) )
		return QString();

	return d->m_errorMsg;
}
