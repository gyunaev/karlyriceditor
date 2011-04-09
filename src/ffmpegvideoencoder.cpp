/**************************************************************************
 *  Karlyriceditor - a lyrics editor for Karaoke songs                    *
 *  Copyright (C) 2009 George Yunaev, support@karlyriceditor.com          *
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
 *                                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  *
 *                                                                        *
 * This class is heavily based on:                                        *
 *  QTFFmpegWrapper - QT FFmpeg Wrapper Class                             *
 * Copyright (C) 2009,2010: Daniel Roggen, droggen@gmail.com              *
 * All rights reserved.                                                   *
 *                                                                        *
 * Redistribution and use in source and binary forms, with or without     *
 * modification, are permitted provided that the following conditions are *
 * met:                                                                   *
 *                                                                        *
 *  1. Redistributions of source code must retain the above copyright     *
 *     notice, this list of conditions and the following disclaimer.      *
 *  2. Redistributions in binary form must reproduce the above copyright  *
 *     notice, this list of conditions and the following disclaimer in    *
 *     the documentation, and/or other materials provided with the        *
 *     distribution.                                                      *
 *                                                                        *
 * THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDERS ``AS IS'' AND ANY       *
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE      *
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR     *
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE FREEBSD PROJECT OR       *
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,  *
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,    *
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR     *
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY    *
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT           *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE      *
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF            *
 * SUCH DAMAGE.                                                           *
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

		bool createFile( const QString& filename );
		bool close();
		int encodeImage( qint64 timing, const QImage & img);

	public:
		// Video output parameters
		unsigned int	m_width;
		unsigned int	m_height;
		unsigned int	m_videobitrate;
		unsigned int	m_videogop; // maximal interval in frames between keyframes
		unsigned int	m_videofps;

		// Do we also have an audio source?
		AudioPlayerPrivate * m_aplayer;

	private:
		void initVars();
		bool convertImage_sws(const QImage &img);

		// FFmpeg stuff
		AVFormatContext *pFormatCtx;
		AVOutputFormat *pOutputFormat;
		AVCodecContext *pCodecCtx;
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

		// Packet


		QString fileName;
};


FFMpegVideoEncoderPriv::FFMpegVideoEncoderPriv()
{
	ffmpeg_init_once();

	initVars();
}

FFMpegVideoEncoderPriv::~FFMpegVideoEncoderPriv()
{
	close();
}

bool FFMpegVideoEncoderPriv::createFile( const QString& fileName )
{
	// If we had an open video, close it.
	close();

	// Check sizes
	if ( m_width % 8 )
		return false;

	if ( m_height % 8 )
		return false;

	pOutputFormat = guess_format(NULL, fileName.toStdString().c_str(), NULL);

	if ( !pOutputFormat )
	{
		printf("Could not deduce output format from file extension: using MPEG.\n");
		pOutputFormat = guess_format("mpeg", NULL, NULL);
	}

	pFormatCtx = avformat_alloc_context();

	if ( !pFormatCtx )
	{
		printf("Error allocating format context\n");
		return false;
	}

	pFormatCtx->oformat = pOutputFormat;
	strncpy( pFormatCtx->filename, fileName.toUtf8(), sizeof(pFormatCtx->filename) );

	// Add the video stream, index 0
	pVideoStream = av_new_stream( pFormatCtx, 0 );

	if ( !pVideoStream )
	{
		printf("Could not allocate video stream\n");
		return false;
	}

	pCodecCtx = pVideoStream->codec;
	pCodecCtx->codec_id = pOutputFormat->video_codec;
	pCodecCtx->codec_type = CODEC_TYPE_VIDEO;

	pCodecCtx->bit_rate = m_videobitrate;
	pCodecCtx->width = m_width;
	pCodecCtx->height = m_height;
	pCodecCtx->time_base.den = 25;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->gop_size = m_videogop;
	pCodecCtx->pix_fmt = PIX_FMT_YUV420P;

	// Do we also have audio stream?
	if ( m_aplayer )
	{
		// Add the audio stream, index 1
		pAudioStream = av_new_stream( pFormatCtx, 1 );

		if ( !pAudioStream )
		{
			printf("Could not allocate audio stream\n");
			return false;
		}

		AVCodecContext * aCodecCtx = pAudioStream->codec;
		aCodecCtx->codec_type = CODEC_TYPE_AUDIO;

		aCodecCtx->codec_id = m_aplayer->aCodecCtx->codec_id;
		aCodecCtx->bit_rate = m_aplayer->aCodecCtx->bit_rate;
		aCodecCtx->sample_rate = m_aplayer->aCodecCtx->sample_rate;
		aCodecCtx->sample_fmt = m_aplayer->aCodecCtx->sample_fmt;
		aCodecCtx->channels = m_aplayer->aCodecCtx->channels;
		aCodecCtx->time_base.den = m_aplayer->aCodecCtx->time_base.den;
		aCodecCtx->time_base.num = m_aplayer->aCodecCtx->time_base.num;

		// Rewind the player
		m_aplayer->reset();
	}

	avcodec_thread_init( pCodecCtx, 10 );

	// some formats want stream headers to be separate
	if ( pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER )
		pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

	if ( av_set_parameters(pFormatCtx, NULL) < 0 )
	{
		printf("Invalid output format parameters\n");
		return false;
	}

	dump_format(pFormatCtx, 0, fileName.toStdString().c_str(), 1);

	// find the video encoder
	pCodec = avcodec_find_encoder( pCodecCtx->codec_id );

	if ( !pCodec )
	{
		printf("codec not found\n");
		return false;
	}

	// open the codec
	if ( avcodec_open( pCodecCtx, pCodec ) < 0 )
	{
		printf("could not open codec\n");
		return false;
	}

	// Allocate memory for output
	outbuf_size = m_width * m_height * 3;
	outbuf = new uint8_t[ outbuf_size ];

	if ( !outbuf )
		return false;

	// Allocate the YUV frame
	ppicture = avcodec_alloc_frame();

	if ( !ppicture )
		return false;

	int size = avpicture_get_size( pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height );
	picture_buf = new uint8_t[size];

	if ( !picture_buf )
		return false;

	// Setup the planes
	avpicture_fill( (AVPicture *)ppicture, picture_buf,pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height );

	if ( url_fopen( &pFormatCtx->pb, fileName.toStdString().c_str(), URL_WRONLY) < 0 )
	{
		printf( "Could not open '%s'\n", fileName.toStdString().c_str());
		return false;
	}

	av_write_header(pFormatCtx);
	return true;
}

/**
   \brief Completes writing the stream, closes it, release resources.
**/
bool FFMpegVideoEncoderPriv::close()
{
	if ( pFormatCtx )
	{
		av_write_trailer( pFormatCtx );

		// close video
		avcodec_close( pVideoStream->codec );

		// free the streams
		for ( int i = 0; i < pFormatCtx->nb_streams; i++ )
		{
			av_freep(&pFormatCtx->streams[i]->codec);
			av_freep(&pFormatCtx->streams[i]);
		}

		// Close file
		url_fclose(pFormatCtx->pb);

		// Free the format
		av_free( pFormatCtx );
	}

	if ( outbuf )
		delete[] outbuf;

	if ( picture_buf )
		delete[] picture_buf;

	if ( ppicture )
		av_free(ppicture);

	initVars();
	return true;
}


/**
   \brief Encode one frame

   The frame must be of the same size as specifie
**/
int FFMpegVideoEncoderPriv::encodeImage( qint64 timing, const QImage &img )
{
	AVPacket pkt;

	// If we have audio, first add all audio packets
	if ( m_aplayer )
	{
		while ( 1 )
		{
			// Read a frame from audio stream
			if ( av_read_frame( m_aplayer->pFormatCtx, &pkt ) < 0 )
				break;

			if ( pkt.stream_index != m_aplayer->audioStream )
			{
				av_free_packet( &pkt );
				continue;
			}

			// Store the packet
			int ret = av_interleaved_write_frame( pFormatCtx, &pkt);

			// and free it
			av_free_packet( &pkt );

			if ( ret < 0 )
				return -1;

			qint64 audio_timing = av_rescale_q( pkt.pts,
									 m_aplayer->pFormatCtx->streams[m_aplayer->audioStream]->time_base,
									 AV_TIME_BASE_Q ) / 1000;

			// No more audio packets for this frame?
			if ( audio_timing >= timing )
				break;

			// Proceed with the next packet
		}
	}

	// SWS conversion
	convertImage_sws(img);

	int out_size = avcodec_encode_video(pCodecCtx,outbuf,outbuf_size,ppicture);

	if (out_size > 0)
	{
		av_init_packet(&pkt);

		if (pCodecCtx->coded_frame->pts != (0x8000000000000000LL))
			pkt.pts= av_rescale_q(pCodecCtx->coded_frame->pts, pCodecCtx->time_base, pVideoStream->time_base);
		if(pCodecCtx->coded_frame->key_frame)
			pkt.flags |= PKT_FLAG_KEY;

		pkt.stream_index= pVideoStream->index;
		pkt.data= outbuf;
		pkt.size= out_size;
		int ret = av_interleaved_write_frame(pFormatCtx, &pkt);

		av_free_packet( &pkt );

		if(ret<0)
			return -1;
	}
	return out_size;
}

void FFMpegVideoEncoderPriv::initVars()
{
	pFormatCtx = 0;
	pOutputFormat = 0;
	pCodecCtx = 0;
	pVideoStream = 0;
	pAudioStream = 0;
	pCodec = 0;
	ppicture = 0;
	outbuf = 0;
	picture_buf = 0;
	img_convert_ctx = 0;
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

int FFMpegVideoEncoder::encodeImage( qint64 timing, const QImage & img)
{
	return d->encodeImage( timing, img );
}

bool FFMpegVideoEncoder::createFile( const QString& filename, unsigned int width, unsigned int height,
									unsigned int videobitrate,  unsigned int fps, unsigned int gop,
									AudioPlayer * audio )
{
	d->m_aplayer = audio ? audio->impl() : 0;
	d->m_width = width;
	d->m_height = height;
	d->m_videobitrate = videobitrate;
	d->m_videofps = fps;
	d->m_videogop = gop;

	return d->createFile( filename );
}
