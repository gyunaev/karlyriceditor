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
 **************************************************************************/

#ifndef CDGGENERATOR_H
#define CDGGENERATOR_H

#include <QColor>
#include <QImage>
#include <QLabel>
#include <QVector>

#include "cdg.h"
#include "lyrics.h"

class CDGGenerator
{
	public:
		CDGGenerator();

		// Initializes the stream, fills up the color tables and clears screen
		void	init( const QColor& bgcolor,
					  const QColor& titlecolor,
					  const QColor& actcolor,
					  const QColor& inactcolor,
					  const QFont& font );

		// Generate the CD+G lyrics
		void	generate( const Lyrics& lyrics, qint64 total_length );

		// Returns the CD+G stream
		QByteArray	stream();

	private:
		void	addSubcode( const SubCode& sc );
		void	addEmpty();
		void	addLoadColors( const QColor& bgcolor, const QColor& titlecolor,
							   const QColor& actcolor, const QColor& inactcolor );
		void	clearScreen();
		void	applyTileChanges( const QImage& orig,const QImage& newimg );

		void	fillColor( char * buffer, const QColor& color );
		QString	stripHTML( const QString& str );
		int		getColor( QRgb color );
		void	checkTile( int offset_x, int offset_y, const QImage& orig,const QImage& newimg );

	private:
		QVector< SubCode >		m_stream;		// CD+G stream

		QFont					m_renderFont;
};


#endif // CDGGENERATOR_H
