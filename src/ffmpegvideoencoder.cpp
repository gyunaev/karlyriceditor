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

bool FFMpegVideoEncoderPriv::createFile( const QString& fileName )
{
	// If we had an open video, close it.
	close();

	// Check sizes
	if ( m_width % 8 )
		return false;

	if ( m_height % 8 )
		return false;

	// Allocate output format
	pOutputFormat = guess_format(NULL, fileName.toStdString().c_str(), NULL);

	if ( !pOutputFormat )
	{
		printf("Could not deduce output format from file extension: using MPEG.\n");
		pOutputFormat = guess_format("mpeg", NULL, NULL);
	}

	pOutputCtx = avformat_alloc_context();

	if ( !pOutputCtx )
	{
		printf("Error allocating format context\n");
		return false;
	}

	pOutputCtx->oformat = pOutputFormat;
	strncpy( pOutputCtx->filename, fileName.toUtf8(), sizeof(pOutputCtx->filename) );

	// Add the video stream, index 0
	pVideoStream = av_new_stream( pOutputCtx, 0 );

	if ( !pVideoStream )
	{
		printf("Could not allocate video stream\n");
		return false;
	}

	pVideoCodecCtx = pVideoStream->codec;
	pVideoCodecCtx->codec_id = pOutputFormat->video_codec;
	pVideoCodecCtx->codec_type = CODEC_TYPE_VIDEO;

	pVideoCodecCtx->bit_rate = m_videobitrate;
	pVideoCodecCtx->width = m_width;
	pVideoCodecCtx->height = m_height;
	pVideoCodecCtx->time_base.den = 25;
	pVideoCodecCtx->time_base.num = 1;
	pVideoCodecCtx->gop_size = m_videogop;
	pVideoCodecCtx->pix_fmt = PIX_FMT_YUV420P;

	// Do we also have audio stream?
	if ( m_aplayer )
	{
		// Add the audio stream, index 1
		pAudioStream = av_new_stream( pOutputCtx, 1 );

		if ( !pAudioStream )
		{
			printf("Could not allocate audio stream\n");
			return false;
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

	avcodec_thread_init( pVideoCodecCtx, 10 );

	// some formats want stream headers to be separate
	if ( pOutputCtx->oformat->flags & AVFMT_GLOBALHEADER )
		pVideoCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

	// find the video encoder
	pCodec = avcodec_find_encoder( pVideoCodecCtx->codec_id );

	if ( !pCodec )
	{
		printf("codec not found\n");
		return false;
	}

	// open the codec
	if ( avcodec_open( pVideoCodecCtx, pCodec ) < 0 )
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

	int size = avpicture_get_size( pVideoCodecCtx->pix_fmt, pVideoCodecCtx->width, pVideoCodecCtx->height );
	picture_buf = new uint8_t[size];

	if ( !picture_buf )
		return false;

	// Setup the planes
	avpicture_fill( (AVPicture *)ppicture, picture_buf,pVideoCodecCtx->pix_fmt, pVideoCodecCtx->width, pVideoCodecCtx->height );

	if ( url_fopen( &pOutputCtx->pb, fileName.toStdString().c_str(), URL_WRONLY) < 0 )
	{
		printf( "Could not open '%s'\n", fileName.toStdString().c_str());
		return false;
	}

	av_write_header(pOutputCtx);
	return true;
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
		url_fclose(pOutputCtx->pb);

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

int FFMpegVideoEncoderPriv::encodeImage( qint64 timing, const QImage &img )
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
			pkt.flags |= PKT_FLAG_KEY;

			int ret = av_interleaved_write_frame( pOutputCtx, &pkt );

			// and free it
			av_free_packet( &audiopkt );

			if ( ret < 0 )
				return -1;

			outsize += ret;

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
			pkt.flags |= PKT_FLAG_KEY;

		pkt.stream_index = pVideoStream->index;
		pkt.data = outbuf;
		pkt.size = out_size;

		int ret = av_interleaved_write_frame(pOutputCtx, &pkt);

		if ( ret < 0 )
			return -1;

		outsize += ret;
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
