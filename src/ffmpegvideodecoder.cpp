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

#include "ffmpeg_headers.h"
#include "ffmpegvideodecoder.h"


class FFMpegVideoDecoderPriv
{
	public:
		void	init();
		bool	readFrame( int frame );

	public:
		unsigned int	skipFrames;
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

bool FFMpegVideoDecoder::openFile( const QString& filename, unsigned int seekto )
{
	// See http://dranger.com/ffmpeg/tutorial01.html
	close();

	// Open video file
	if ( avformat_open_input( &d->pFormatCtx, FFMPEG_FILENAME( filename ), NULL, 0 ) != 0 )
	{
		d->m_errorMsg = "Could not open video file";
		return false;
	}

	// Retrieve stream information
	if ( avformat_find_stream_info( d->pFormatCtx, 0 ) < 0 )
	{
		d->m_errorMsg = "Could not find stream information in the video file";
		return false;
	}

	// Find the first video stream
	d->videoStream = -1;

	for ( unsigned i = 0; i < d->pFormatCtx->nb_streams; i++ )
	{
        AVStream *stream = d->pFormatCtx->streams[i];
        AVCodec *dec = avcodec_find_decoder( stream->codecpar->codec_id );

        if ( !dec )
            continue;

        AVCodecContext * codec_ctx = avcodec_alloc_context3( dec );

        if ( !codec_ctx )
            continue;

        if ( avcodec_parameters_to_context(codec_ctx, stream->codecpar ) < 0 )
        {
            avcodec_free_context( &codec_ctx );
            continue;
        }

        // Must be video stream
        if ( codec_ctx->codec_type != AVMEDIA_TYPE_VIDEO )
        {
            avcodec_free_context( &codec_ctx );
            continue;
        }

        // Open a decoder
        if ( avcodec_open2(codec_ctx, dec, NULL) < 0 )
        {
            avcodec_free_context( &codec_ctx );
            continue;
        }

        // We got our stream
        d->videoStream = i;
        d->pCodecCtx = codec_ctx;
        d->pCodec = dec;
        break;
    }

    if ( d->videoStream == -1 )
		return false; // Didn't find a video stream

	d->m_fps_den = d->pFormatCtx->streams[d->videoStream]->r_frame_rate.den;
	d->m_fps_num = d->pFormatCtx->streams[d->videoStream]->r_frame_rate.num;

	if ( d->m_fps_den == 60000 )
		d->m_fps_den = 30000;
	
    // Allocate video frame and AVFrame structure
    // Credits: http://stackoverflow.com/questions/24057248/ffmpeg-undefined-references-to-av-frame-alloc
    d->pFrame = av_frame_alloc();
    d->pFrameRGB = av_frame_alloc();

    if ( !d->pFrame || !d->pFrameRGB )
	{
		d->m_errorMsg = "Could not allocate memory for video frames";
		return false;
	}

	// Determine required buffer size and allocate buffer
    int numBytes = av_image_get_buffer_size( AV_PIX_FMT_RGB24, d->pCodecCtx->width, d->pCodecCtx->height, 32 );
	d->m_buffer.resize( numBytes );

	// Assign appropriate parts of buffer to image planes in pFrameRGB
    av_image_fill_arrays( d->pFrameRGB->data, d->pFrameRGB->linesize, (uint8_t*) d->m_buffer.data(),
                    AV_PIX_FMT_RGB24, d->pCodecCtx->width, d->pCodecCtx->height, 1 );

	d->skipFrames = seekto;
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
		avformat_close_input( &d->pFormatCtx );

	// Reset the pointers
	d->init();
}

bool FFMpegVideoDecoderPriv::readFrame( int frame )
{
	while ( m_currentFrameNumber < frame )
	{
        AVPacket packet;

		// Read a frame
        if ( av_read_frame( pFormatCtx, &packet ) < 0 )
            return false;  // Frame read failed (e.g. end of stream)

        // Is this a packet from the video stream -> decode video frame
		if ( packet.stream_index == videoStream )
		{
            if ( avcodec_send_packet( pCodecCtx, &packet) < 0 )
                return false;  // Frame read failed (e.g. end of stream)

            // Using new API: https://blogs.gentoo.org/lu_zero/2016/03/29/new-avcodec-api/
            while ( avcodec_receive_frame( pCodecCtx, pFrame ) == 0 )
            {
				m_currentFrameNumber++;

                if ( m_currentFrameNumber >= frame )
				{
					int w = pCodecCtx->width;
					int h = pCodecCtx->height;

                    img_convert_ctx = sws_getCachedContext(img_convert_ctx,w, h, pCodecCtx->pix_fmt, w, h, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

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

        av_packet_unref( &packet );
	}

	return true;
}

QImage FFMpegVideoDecoder::frame( qint64 timems )
{
	// Use current frame?
	int frame_for_time = ((timems * d->m_fps_num) / d->m_fps_den) / 1000;

	if ( frame_for_time == 0 )
		frame_for_time = 1;

	frame_for_time += d->skipFrames;

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

		d->skipFrames = 0;
		avcodec_flush_buffers( d->pCodecCtx );
	}

	return d->m_currentFrameImage;
}
