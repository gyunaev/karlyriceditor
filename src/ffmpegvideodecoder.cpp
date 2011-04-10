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
 **************************************************************************/

#include "ffmpeg_headers.h"
#include "ffmpegvideodecoder.h"


class FFMpegVideoDecoderPriv
{
	public:
		void	init();
		bool	readFrame( int frame );

	public:
		AVFormatContext *pFormatCtx;
		int				 videoStream;
		AVCodecContext  *pCodecCtx;
		AVCodec         *pCodec;
		AVFrame         *pFrame;
		AVFrame         *pFrameRGB;
		SwsContext      *img_convert_ctx;

		QByteArray		m_buffer;
		QString			m_errorMsg;
		int				m_maxFrame;
		int				m_fps_den;
		int				m_fps_num;
		int				m_currentFrameNumber;
		QImage			m_currentFrameImage;
};

void FFMpegVideoDecoderPriv::init()
{
	pFormatCtx = 0;
	pCodecCtx = 0;
	pCodec = 0;
	pFrame = 0;
	pFrameRGB = 0;
	img_convert_ctx = 0;

	m_maxFrame = 0;
	m_currentFrameNumber = 0;
}


//
// A wrapper interface class
//
FFMpegVideoDecoder::FFMpegVideoDecoder()
{
	ffmpeg_init_once();

	d = new FFMpegVideoDecoderPriv();

	// Reset the pointers
	d->init();
}

FFMpegVideoDecoder::~FFMpegVideoDecoder()
{
	close();
	delete d;
}

bool FFMpegVideoDecoder::openFile( const QString& filename )
{
	// See http://dranger.com/ffmpeg/tutorial01.html
	close();

	// Open video file
	if ( av_open_input_file( &d->pFormatCtx, filename.toUtf8(), NULL, 0, NULL ) != 0 )
	{
		d->m_errorMsg = "Could not open video file";
		return false;
	}

	// Retrieve stream information
	if ( av_find_stream_info( d->pFormatCtx ) < 0 )
	{
		d->m_errorMsg = "Could not find stream information in the video file";
		return false;
	}

	// Find the first video stream
	d->videoStream = -1;

	for ( unsigned i = 0; i < d->pFormatCtx->nb_streams; i++ )
	{
		if ( d->pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO )
		{
			d->videoStream = i;
			break;
		}
	}

	if ( d->videoStream == -1 )
		return false; // Didn't find a video stream

	d->m_fps_den = d->pFormatCtx->streams[d->videoStream]->r_frame_rate.den;
	d->m_fps_num = d->pFormatCtx->streams[d->videoStream]->r_frame_rate.num;

	if ( d->m_fps_den == 60000 )
		d->m_fps_den = 30000;
	
	// Get a pointer to the codec context for the video stream
	d->pCodecCtx = d->pFormatCtx->streams[d->videoStream]->codec;

	// Find the decoder for the video stream
	d->pCodec = avcodec_find_decoder( d->pCodecCtx->codec_id );

	if ( d->pCodec == NULL )
	{
	   d->m_errorMsg = "Could not find the decoder for the video file";
	   return false;
	}

	// Open codec
	if ( avcodec_open( d->pCodecCtx, d->pCodec ) < 0 )
	{
	   d->m_errorMsg = "Could not open the codec for the video file";
	   return false;
	}

	// Allocate video frame
	d->pFrame = avcodec_alloc_frame();

	// Allocate an AVFrame structure
	d->pFrameRGB = avcodec_alloc_frame();

	if ( !d->pFrame || !d->pFrameRGB )
	{
		d->m_errorMsg = "Could not allocate memory for video frames";
		return false;
	}

	// Determine required buffer size and allocate buffer
	int numBytes = avpicture_get_size( PIX_FMT_RGB24, d->pCodecCtx->width, d->pCodecCtx->height );
	d->m_buffer.resize( numBytes );

	// Assign appropriate parts of buffer to image planes in pFrameRGB
	avpicture_fill( (AVPicture *) d->pFrameRGB, (uint8_t*) d->m_buffer.data(),
					PIX_FMT_RGB24, d->pCodecCtx->width, d->pCodecCtx->height );

	return true;
}

QString FFMpegVideoDecoder::errorMsg() const
{
	return d->m_errorMsg;
}

void FFMpegVideoDecoder::close()
{
	// Free the YUV frame
	if ( d->pFrame )
		av_free( d->pFrame );

	// Free the RGB frame
	if ( d->pFrameRGB )
		av_free( d->pFrameRGB );

	// Close the codec
	if ( d->pCodecCtx )
		avcodec_close( d->pCodecCtx );

	// Close the video file
	if ( d->pFormatCtx )
		av_close_input_file( d->pFormatCtx );

	// Reset the pointers
	d->init();
}

bool FFMpegVideoDecoderPriv::readFrame( int frame )
{
	AVPacket packet;
	int frameFinished;

	while ( m_currentFrameNumber < frame )
	{
		// Read a frame
		if ( av_read_frame( pFormatCtx, &packet ) < 0 )
			return false;  // Frame read failed (e.g. end of stream)

		if ( packet.stream_index == videoStream )
		{
			// Is this a packet from the video stream -> decode video frame
			avcodec_decode_video2( pCodecCtx, pFrame, &frameFinished, &packet );

			// Did we get a video frame?
			if ( frameFinished )
			{
				m_currentFrameNumber++;

				if ( m_currentFrameNumber >= frame )
				{
					int w = pCodecCtx->width;
					int h = pCodecCtx->height;

					img_convert_ctx = sws_getCachedContext(img_convert_ctx,w, h, pCodecCtx->pix_fmt, w, h, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

					if ( img_convert_ctx == NULL )
					{
						printf("Cannot initialize the conversion context!\n");
						return false;
					}

					sws_scale( img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize );

					// Convert the frame to QImage
					m_currentFrameImage = QImage( w, h, QImage::Format_RGB888 );

					for ( int y = 0; y < h; y++ )
						memcpy( m_currentFrameImage.scanLine(y), pFrameRGB->data[0] + y * pFrameRGB->linesize[0], w*3 );
				}
			}
		}

		av_free_packet( &packet );
	}

	return true;
}

QImage FFMpegVideoDecoder::frame( qint64 timems )
{
	// Use current frame?
	int frame_for_time = ((timems * d->m_fps_num) / d->m_fps_den) / 1000;

	if ( frame_for_time == 0 )
		frame_for_time = 1;

	// Loop if we know how many frames we have total
	if ( d->m_maxFrame > 0 )
		frame_for_time %= d->m_maxFrame;

	while ( 1 )
	{
		if ( d->readFrame( frame_for_time ) )
			break;

		// End of video; restart
		d->m_maxFrame = d->m_currentFrameNumber - 1;
		d->m_currentFrameNumber = 0;
		frame_for_time = 0;

		if ( av_seek_frame( d->pFormatCtx, d->videoStream, 0, 0 ) < 0 )
		{
			qWarning("Cannot seek video back to zero");
			return QImage();
		}

		avcodec_flush_buffers( d->pCodecCtx );
	}

	return d->m_currentFrameImage;
}
