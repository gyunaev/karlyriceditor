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

class FFMpegVideoEncoderPriv
{
	public:
		// gop: maximal interval in frames between keyframes
		bool createFile( const QString& filename, unsigned int width, unsigned int height,
						 unsigned int bitrate, unsigned int gop );
		bool close();

		int encodeImage( const QImage & img);
		bool isOk();

		unsigned Width,Height;
		unsigned Bitrate;
		unsigned Gop;
		bool ok;

		// FFmpeg stuff
		AVFormatContext *pFormatCtx;
		AVOutputFormat *pOutputFormat;
		AVCodecContext *pCodecCtx;
		AVStream *pVideoStream;
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
		AVPacket pkt;

		QString fileName;

		FFMpegVideoEncoderPriv();
		~FFMpegVideoEncoderPriv();

		unsigned getWidth();
		unsigned getHeight();
		bool isSizeValid();

		void initVars();
		bool initCodec();

		// Alloc/free the output buffer
		bool initOutputBuf();
		void freeOutputBuf();

		// Alloc/free a frame
		bool initFrame();
		void freeFrame();

		// Frame conversion
		bool convertImage(const QImage &img);
		bool convertImage_sws(const QImage &img);
};


FFMpegVideoEncoderPriv::FFMpegVideoEncoderPriv()
{
   initVars();
}

FFMpegVideoEncoderPriv::~FFMpegVideoEncoderPriv()
{
   close();
}

bool FFMpegVideoEncoderPriv::createFile( const QString& fileName, unsigned int width, unsigned int height, unsigned int bitrate, unsigned int gop )
{
   // If we had an open video, close it.
   close();

   Width=width;
   Height=height;
   Gop=gop;
   Bitrate=bitrate;

   if(!isSizeValid())
   {
	  printf("Invalid size\n");
	  return false;
   }

   pOutputFormat = guess_format(NULL, fileName.toStdString().c_str(), NULL);
   if (!pOutputFormat) {
	  printf("Could not deduce output format from file extension: using MPEG.\n");
	  pOutputFormat = guess_format("mpeg", NULL, NULL);
   }

   pFormatCtx=avformat_alloc_context();
   if(!pFormatCtx)
   {
	  printf("Error allocating format context\n");
	  return false;
   }
   pFormatCtx->oformat = pOutputFormat;
   strncpy( pFormatCtx->filename, fileName.toUtf8(), sizeof(pFormatCtx->filename) );


   // Add the video stream

   pVideoStream = av_new_stream(pFormatCtx,0);
   if(!pVideoStream )
   {
	  printf("Could not allocate stream\n");
	  return false;
   }


   pCodecCtx=pVideoStream->codec;
   pCodecCtx->codec_id = pOutputFormat->video_codec;
   pCodecCtx->codec_type = CODEC_TYPE_VIDEO;

   pCodecCtx->bit_rate = Bitrate;
   pCodecCtx->width = getWidth();
   pCodecCtx->height = getHeight();
   pCodecCtx->time_base.den = 25;
   pCodecCtx->time_base.num = 1;
   pCodecCtx->gop_size = Gop;
   pCodecCtx->pix_fmt = PIX_FMT_YUV420P;


   avcodec_thread_init(pCodecCtx, 10);

   //if (c->codec_id == CODEC_ID_MPEG2VIDEO)
   //{
	  //c->max_b_frames = 2;  // just for testing, we also add B frames
   //}

   // some formats want stream headers to be separate
   if(pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
	  pCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;


   if (av_set_parameters(pFormatCtx, NULL) < 0)
   {
	  printf("Invalid output format parameters\n");
	  return false;
   }

   dump_format(pFormatCtx, 0, fileName.toStdString().c_str(), 1);

   // open_video
   // find the video encoder
   pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
   if (!pCodec)
   {
	  printf("codec not found\n");
	  return false;
   }
   // open the codec
   if (avcodec_open(pCodecCtx, pCodec) < 0)
   {
	  printf("could not open codec\n");
	  return false;
   }

   // Allocate memory for output
   if(!initOutputBuf())
   {
	  printf("Can't allocate memory for output bitstream\n");
	  return false;
   }

   // Allocate the YUV frame
   if(!initFrame())
   {
	  printf("Can't init frame\n");
	  return false;
   }

   if (url_fopen(&pFormatCtx->pb, fileName.toStdString().c_str(), URL_WRONLY) < 0)
   {
	  printf( "Could not open '%s'\n", fileName.toStdString().c_str());
	  return false;
   }

   av_write_header(pFormatCtx);



   ok=true;

   return true;
}

/**
   \brief Completes writing the stream, closes it, release resources.
**/
bool FFMpegVideoEncoderPriv::close()
{
   if(!isOk())
	  return false;

   av_write_trailer(pFormatCtx);

   // close_video

   avcodec_close(pVideoStream->codec);
   freeFrame();
   freeOutputBuf();


   /* free the streams */

   for(int i = 0; i < pFormatCtx->nb_streams; i++)
   {
	  av_freep(&pFormatCtx->streams[i]->codec);
	  av_freep(&pFormatCtx->streams[i]);
   }

   // Close file
   url_fclose(pFormatCtx->pb);

   // Free the stream
   av_free(pFormatCtx);

   initVars();
   return true;
}


/**
   \brief Encode one frame

   The frame must be of the same size as specifie
**/
int FFMpegVideoEncoderPriv::encodeImage(const QImage &img)
{
   if(!isOk())
	  return -1;

   //convertImage(img);       // Custom conversion routine
   convertImage_sws(img);     // SWS conversion

   int out_size = avcodec_encode_video(pCodecCtx,outbuf,outbuf_size,ppicture);
   //printf("Frame size: %d\n",out_size);
   if (out_size > 0)
   {

	  av_init_packet(&pkt);

	  //if (pCodecCtx->coded_frame->pts != AV_NOPTS_VALUE)
	  if (pCodecCtx->coded_frame->pts != (0x8000000000000000LL))
		 pkt.pts= av_rescale_q(pCodecCtx->coded_frame->pts, pCodecCtx->time_base, pVideoStream->time_base);
	  if(pCodecCtx->coded_frame->key_frame)
		 pkt.flags |= PKT_FLAG_KEY;

	  pkt.stream_index= pVideoStream->index;
	  pkt.data= outbuf;
	  pkt.size= out_size;
	  int ret = av_interleaved_write_frame(pFormatCtx, &pkt);
	  //printf("Wrote %d\n",ret);
	  if(ret<0)
		 return -1;
   }
   return out_size;
}

void FFMpegVideoEncoderPriv::initVars()
{
   ok=false;
   pFormatCtx=0;
   pOutputFormat=0;
   pCodecCtx=0;
   pVideoStream=0;
   pCodec=0;
   ppicture=0;
   outbuf=0;
   picture_buf=0;
   img_convert_ctx=0;
}



/**
  Ensures sizes are some reasonable multiples
**/
bool FFMpegVideoEncoderPriv::isSizeValid()
{
   if(getWidth()%8)
	  return false;
   if(getHeight()%8)
	  return false;
   return true;
}

unsigned FFMpegVideoEncoderPriv::getWidth()
{
   return Width;
}

unsigned FFMpegVideoEncoderPriv::getHeight()
{
   return Height;
}

bool FFMpegVideoEncoderPriv::isOk()
{
   return ok;
}

/**
  Allocate memory for the compressed bitstream
**/
bool FFMpegVideoEncoderPriv::initOutputBuf()
{
   outbuf_size = getWidth()*getHeight()*3;        // Some extremely generous memory allocation for the encoded frame.
   outbuf = new uint8_t[outbuf_size];
   if(outbuf==0)
	  return false;
   return true;
}
/**
  Free memory for the compressed bitstream
**/
void FFMpegVideoEncoderPriv::freeOutputBuf()
{
   if(outbuf)
   {
	  delete[] outbuf;
	  outbuf=0;
   }
}

bool FFMpegVideoEncoderPriv::initFrame()
{
   ppicture = avcodec_alloc_frame();
   if(ppicture==0)
	  return false;

   int size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
   picture_buf = new uint8_t[size];
   if(picture_buf==0)
   {
	  av_free(ppicture);
	  ppicture=0;
	  return false;
   }

   // Setup the planes
   avpicture_fill((AVPicture *)ppicture, picture_buf,pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

   return true;
}

void FFMpegVideoEncoderPriv::freeFrame()
{
   if(picture_buf)
   {
	  delete[] picture_buf;
	  picture_buf=0;
   }
   if(ppicture)
   {
	  av_free(ppicture);
	  ppicture=0;
   }
}

/**
  \brief Convert the QImage to the internal YUV format

  Custom conversion - not very optimized.

**/

bool FFMpegVideoEncoderPriv::convertImage(const QImage &img)
{
   // Check if the image matches the size
   if(img.width()!=getWidth() || img.height()!=getHeight())
   {
	  printf("Wrong image size!\n");
	  return false;
   }
   if(img.format()!=QImage::Format_RGB32	&& img.format() != QImage::Format_ARGB32)
   {
	  printf("Wrong image format\n");
	  return false;
   }

   // RGB32 to YUV420

   int size = getWidth()*getHeight();
   // Y
   for(unsigned y=0;y<getHeight();y++)
   {

	  unsigned char *s = (unsigned char*)img.scanLine(y);
	  unsigned char *d = (unsigned char*)&picture_buf[y*getWidth()];
	  //printf("Line %d. d: %p. picture_buf: %p\n",y,d,picture_buf);

	  for(unsigned x=0;x<getWidth();x++)
	  {
		 unsigned int r=s[2];
		 unsigned int g=s[1];
		 unsigned int b=s[0];

		 unsigned Y = (r*2104 + g*4130 + b*802 + 4096 + 131072) >> 13;
		 if(Y>235) Y=235;

		 *d = Y;

		 d+=1;
		 s+=4;
	  }
   }

   // U,V
   for(unsigned y=0;y<getHeight();y+=2)
   {
	  unsigned char *s = (unsigned char*)img.scanLine(y);
	  unsigned int ss = img.bytesPerLine();
	  unsigned char *d = (unsigned char*)&picture_buf[size+y/2*getWidth()/2];

	  //printf("Line %d. d: %p. picture_buf: %p\n",y,d,picture_buf);

	  for(unsigned x=0;x<getWidth();x+=2)
	  {
		 // Cr = 128 + 1/256 * ( 112.439 * R'd -  94.154 * G'd -  18.285 * B'd)
		 // Cb = 128 + 1/256 * (- 37.945 * R'd -  74.494 * G'd + 112.439 * B'd)

		 // Get the average RGB in a 2x2 block
		 int r=(s[2] + s[6] + s[ss+2] + s[ss+6] + 2) >> 2;
		 int g=(s[1] + s[5] + s[ss+1] + s[ss+5] + 2) >> 2;
		 int b=(s[0] + s[4] + s[ss+0] + s[ss+4] + 2) >> 2;

		 int Cb = (-1214*r - 2384*g + 3598*b + 4096 + 1048576)>>13;
		 if(Cb<16)
			Cb=16;
		 if(Cb>240)
			Cb=240;

		 int Cr = (3598*r - 3013*g - 585*b + 4096 + 1048576)>>13;
		 if(Cr<16)
			Cr=16;
		 if(Cr>240)
			Cr=240;

		 *d = Cb;
		 *(d+size/4) = Cr;

		 d+=1;
		 s+=8;
	  }
   }
   return true;
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
   if(img.width()!=getWidth() || img.height()!=getHeight())
   {
	  printf("Wrong image size!\n");
	  return false;
   }
   if(img.format()!=QImage::Format_RGB32	&& img.format() != QImage::Format_ARGB32)
   {
	  printf("Wrong image format\n");
	  return false;
   }

   img_convert_ctx = sws_getCachedContext(img_convert_ctx,getWidth(),getHeight(),PIX_FMT_BGRA,getWidth(),getHeight(),PIX_FMT_YUV420P,SWS_BICUBIC, NULL, NULL, NULL);
   //img_convert_ctx = sws_getCachedContext(img_convert_ctx,getWidth(),getHeight(),PIX_FMT_BGRA,getWidth(),getHeight(),PIX_FMT_YUV420P,SWS_FAST_BILINEAR, NULL, NULL, NULL);
   if (img_convert_ctx == NULL)
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


   sws_scale(img_convert_ctx, srcplanes, srcstride,0, getHeight(), ppicture->data, ppicture->linesize);

   return true;
}



FFMpegVideoEncoder::FFMpegVideoEncoder()
{
	ffmpeg_init_once();
	d = new FFMpegVideoEncoderPriv();
}

FFMpegVideoEncoder::~FFMpegVideoEncoder()
{
	delete d;
}

bool FFMpegVideoEncoder::createFile( const QString& filename, unsigned int width, unsigned int height,
				 unsigned int bitrate, unsigned int gop )
{
	return d->createFile( filename, width, height, bitrate, gop );
}

bool FFMpegVideoEncoder::close()
{
	return d->close();

}

int FFMpegVideoEncoder::encodeImage( const QImage & img)
{
	return d->encodeImage( img );

}

bool FFMpegVideoEncoder::isOk()
{
	return d->isOk();
}
