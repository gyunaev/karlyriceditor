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

#include <QPainter>

#include "background.h"

Background::Background()
{
}

Background::~Background()
{
}

void Background::reset()
{
}


//
// BackgroundImage
//
BackgroundImage::BackgroundImage( const QString& filename )
	: Background()
{
    if ( !m_image.load( filename ) )
        qWarning("Cannot load image file %s", qPrintable(filename));
}

bool BackgroundImage::isValid() const
{
	return !m_image.isNull();
}

qint64 BackgroundImage::doDraw( QImage& image, qint64 )
{
	// We don't care about timing
	image = m_image;

	// No updates
	return -1;
}


//
// BackgroundVideo
//
BackgroundVideo::BackgroundVideo( const QString& filename )
	: Background()
{
/*	QRegExp videopathstart("^(.*);STARTFRAME=(\\d+)$");

	if ( filename.indexOf( videopathstart ) != -1 )
	{
		m_valid = m_videoDecoder.openFile( videopathstart.cap(1), videopathstart.cap(2).toUInt() );
	}
	else
        m_valid = m_videoDecoder.openFile( filename, 0 );*/
}

bool BackgroundVideo::isValid() const
{
	return m_valid;
}

qint64 BackgroundVideo::doDraw( QImage& image, qint64 timing )
{
/*	QImage videoframe = m_videoDecoder.frame( timing );

	if ( videoframe.isNull() )
		return 0;

	image = videoframe;

	// We use our own cache
    return 0;*/
}


BackgroundColor::BackgroundColor(const QString &arg)
    : m_color( arg )
{
}

bool BackgroundColor::isValid() const
{
    return m_color.isValid();
}

qint64 BackgroundColor::doDraw(QImage &image, qint64 timing)
{
    image.fill( m_color );

    // No updates
    return -1;
}
