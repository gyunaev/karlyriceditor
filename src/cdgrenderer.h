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

#ifndef CDGRENDERER_H
#define CDGRENDERER_H

#include <QByteArray>
#include <QImage>

#include "lyricsrenderer.h"
#include "cdg.h"

class CDGRenderer : public LyricsRenderer
{
	public:
		CDGRenderer();
		~CDGRenderer();

		void	setCDGdata( const QByteArray& cdgdata );
		virtual int	update( qint64 timing );

	private:
		typedef struct
		{
			unsigned int	packetnum;
			SubCode			subcode;
		} CDGPacket;

		void	dumpPacket( CDGPacket * packet );
		int		UpdateBuffer( unsigned int packets_due );
		void	RenderImage( QImage& imagepixels, unsigned int width, unsigned int height, unsigned int pitch ) const;
		quint8	getPixel( int x, int y );
		void	setPixel( int x, int y, quint8 color );

		void	cmdMemoryPreset( const char * data );
		void	cmdBorderPreset( const char * data );
		void	cmdLoadColorTable( const char * data, int index );
		void	cmdTileBlock( const char * data );
		void	cmdTileBlockXor( const char * data );
		void	cmdTransparentColor( const char * data );
		void	cmdScroll( const char * data, bool loop );
		void	scrollLeft( int color );
		void	scrollRight( int color );
		void	scrollUp( int color );
		void	scrollDown( int color );

		QVector<CDGPacket>  m_cdgStream;	// Parsed CD+G stream storage
		int					m_streamIdx;	// packet offset which hasn't been processed yet

		// Rendering stuff
		quint32			   m_colorTable[16];// CD+G color table; color format is A8R8G8B8
		quint8			   m_bgColor;       // Background color index
		quint8             m_borderColor;   // Border color index
		quint8			   m_cdgScreen[CDG_FULL_WIDTH*CDG_FULL_HEIGHT];	// Image state for CD+G stream

		// These values are used to implement screen shifting.  The CDG specification allows the entire
		// screen to be shifted, up to 5 pixels right and 11 pixels down.  This shift is persistent
		// until it is reset to a different value.  In practice, this is used in conjunction with
		// scrolling (which always jumps in integer blocks of 6x12 pixels) to perform
		// one-pixel-at-a-time scrolls.
		quint8				m_hOffset;
		quint8				m_vOffset;
};

#endif // CDGRENDERER_H
