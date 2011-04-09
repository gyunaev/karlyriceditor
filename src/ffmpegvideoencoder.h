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

#ifndef FFMPEGVIDEOENCODER_H
#define FFMPEGVIDEOENCODER_H

#include <QString>
#include <QImage>

class AudioPlayer;
class FFMpegVideoEncoderPriv;

class FFMpegVideoEncoder
{
	public:
		FFMpegVideoEncoder();
		virtual ~FFMpegVideoEncoder();

		// Returns non-empty error message if failed
		QString createFile( const QString& filename,
						const QString& outformat,
						QSize size,
						unsigned int videobitrate,
						unsigned int time_base_num,
						unsigned int time_base_den,
						unsigned int gop,	// maximal interval in frames between keyframes
						AudioPlayer * audio = 0 );

		bool close();
		int encodeImage( const QImage & img);

	private:
		FFMpegVideoEncoderPriv * d;
};

#endif // FFMPEGVIDEOENCODER_H
