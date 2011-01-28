/*
 QTFFmpegWrapper - QT FFmpeg Wrapper Class
 Copyright (C) 2009,2010: Daniel Roggen, droggen@gmail.com
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are
 permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE FREEBSD PROJECT OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef FFMPEGVIDEODECODER_H
#define FFMPEGVIDEODECODER_H

#include <QImage>

class FFMpegVideoDecoderPriv;

class FFMpegVideoDecoder
{
	public:
		FFMpegVideoDecoder();
		~FFMpegVideoDecoder();

		bool openFile( const QString& file );
		void close();

		bool getFrame( QImage& img, int * effectiveframenumber = 0, int * effectiveframetime = 0,
					   int * desiredframenumber = 0, int * desiredframetime = 0 );
		bool seekNextFrame();
		bool seekMs(int ts);
		bool seekFrame(int64_t frame);
		int  getVideoLengthMs();
		bool isOk();

		QString errorMsg() const;

	protected:
		FFMpegVideoDecoderPriv * d;
};

#endif // FFMPEGVIDEODECODER_H
