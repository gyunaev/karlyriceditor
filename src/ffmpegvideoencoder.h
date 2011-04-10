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
