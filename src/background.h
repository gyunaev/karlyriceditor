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

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <QImage>

class MediaPlayer;

class Background
{
	public:
		Background();
		virtual ~Background();

		// Actual draw function to implement. Should return time when the next
		// doDraw() should be called - for example, for videos it should only
		// be called when it is time to show the next frame; for static images
		// it should not be called at all. If 0 is returned, doDraw() will be called
		// again the next update. If -1 is returned, doDraw() will never be called
		// again, and the cached image will be used.
        virtual qint64 doDraw( QImage& image, qint64 timing ) = 0;

		// This function is called if timing went backward (user seek back), in which
		// case it will be called before doDraw() with a new time. Video players, for
		// example, may use it to seek back to zero. Default implementation does nothing.
		virtual void reset();

		// Should return true if the event was created successfully
		virtual bool isValid() const = 0;
};

class BackgroundImage : public Background
{
	public:
		BackgroundImage( const QString& filename );
		bool isValid() const;

        qint64 doDraw( QImage& image, qint64 timing );

	private:
		QImage	m_image;
};

class BackgroundVideo : public Background
{
	public:
		BackgroundVideo( const QString& arg );
        ~BackgroundVideo();

		bool isValid() const;
        qint64 doDraw( QImage& image, qint64 timing );

	private:
        MediaPlayer    *    m_videoDecoder;
		bool				m_valid;
};

class BackgroundColor : public Background
{
    public:
        BackgroundColor( const QString& arg );

        bool isValid() const;
        qint64 doDraw( QImage& image, qint64 timing );

    private:
        QColor              m_color;
};

#endif // BACKGROUND_H
