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

#ifndef CDGRENDERER_H
#define CDGRENDERER_H

#include <QByteArray>
#include <QImage>
#include "cdg.h"

class CDGRenderer
{
	public:
		CDGRenderer();

		void	setCDGdata( const QByteArray& cdgdata );
		QImage	update( qint64 timing, bool * screen_changed = 0 );

	private:
		void	cmdMemoryPreset( const char * data );
		void	cmdBorderPreset( const char * data );
		void	cmdLoadColorTable( const char * data, int index );
		void	cmdTileBlockXor( const char * data );

		unsigned int		m_packet;		// packet offset which hasn't been processed yet
		QVector< SubCode >	m_stream;		// CD+G stream
		QImage				m_cdgimage;		// rendered image
};

#endif // CDGRENDERER_H
