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

#include "cdgrenderer.h"

CDGRenderer::CDGRenderer()
	: LyricsRenderer()
{
	m_image = QImage( CDG_FULL_WIDTH, CDG_FULL_HEIGHT, QImage::Format_Indexed8 );
}

void CDGRenderer::setImageSize( int, int )
{
	// do nothing
}

void CDGRenderer::setCDGdata( const QByteArray& cdgdata )
{
	m_packet = 0;

	m_image.fill( 0 );
	m_stream.clear();
	m_stream.reserve( cdgdata.size() / sizeof( SubCode ) );

	for ( int offset = 0; offset < cdgdata.size(); offset += sizeof( SubCode ) )
	{
		SubCode * sc = (SubCode *) (cdgdata.data() + offset);

		if ( ( sc->command & CDG_MASK) == CDG_COMMAND )
		{
			// Validate the command and instruction
			switch ( sc->instruction & CDG_MASK )
			{
				case CDG_INST_MEMORY_PRESET:
				case CDG_INST_BORDER_PRESET:
				case CDG_INST_LOAD_COL_TBL_0_7:
				case CDG_INST_LOAD_COL_TBL_8_15:
				case CDG_INST_TILE_BLOCK_XOR:
					break;

				// We do not support those commands, as we do not generate them
				//case CDG_INST_SCROLL_PRESET:
				//case CDG_INST_SCROLL_COPY:
				//case CDG_INST_DEF_TRANSP_COL:
				//case CDG_INST_TILE_BLOCK:
				default:
					qFatal("Unsupported CD+G instruction: %d", sc->instruction & CDG_MASK );
			}
		}

		m_stream.push_back( *sc );
	}
}

int CDGRenderer::update( qint64 tickmark )
{
	int status = UPDATE_NOCHANGE;

	unsigned int packets_due = tickmark * 300 / 1000;

	// Was the stream position reversed? In this case we have to "replay" the whole stream
	// as the screen is a state machine, and "clear" may not be there.
	if ( m_packet > packets_due - 1 )
	{
		qDebug( "CDGRenderer: packet number changed backward (%d played, %d asked", m_packet, packets_due );
		m_image.fill( Qt::black );
		m_packet = 0;
	}

	// Process all packets already due
	for ( ; m_packet < packets_due; m_packet++ )
	{
		SubCode& sc = m_stream[m_packet];
		if ( (sc.command & CDG_MASK) != CDG_COMMAND )
			continue;

		// Execute the instruction
		switch ( sc.instruction & CDG_MASK )
		{
			case CDG_INST_MEMORY_PRESET:
				cmdMemoryPreset( sc.data );
				status = UPDATE_FULL;
				break;

			case CDG_INST_BORDER_PRESET:
				cmdBorderPreset( sc.data );
				status = UPDATE_FULL;
				break;

			case CDG_INST_LOAD_COL_TBL_0_7:
				cmdLoadColorTable( sc.data, 0 );
				break;

			case CDG_INST_LOAD_COL_TBL_8_15:
				cmdLoadColorTable( sc.data, 8 );
				break;

			case CDG_INST_TILE_BLOCK_XOR:
				cmdTileBlockXor( sc.data );
				status = UPDATE_COLORCHANGE;
				break;

			default:
				qFatal("Unsupported CD+G instruction: %d", m_stream[m_packet].instruction & CDG_MASK );
		}
	}

	return status;
}

void CDGRenderer::cmdMemoryPreset( const char * data )
{
	CDG_MemPreset* preset = (CDG_MemPreset*) data;

	if ( preset->repeat & 0x0F )
		return;  // No need for multiple clearings

	quint32 bgColor = preset->color & 0x0F;

	for ( unsigned int i = CDG_BORDER_WIDTH; i < CDG_FULL_WIDTH - CDG_BORDER_WIDTH; i++ )
		for ( unsigned int  j = CDG_BORDER_HEIGHT; j < CDG_FULL_HEIGHT - CDG_BORDER_HEIGHT; j++ )
			m_image.setPixel( i, j, bgColor );

//	qDebug( "MemPreset: filling memory with color %d (%08X)", preset->color & 0x0F, bgColor );
}

void CDGRenderer::cmdBorderPreset( const char * data )
{
	CDG_BorderPreset* preset = (CDG_BorderPreset*) data;

	quint32 borderColor = preset->color & 0x0F;

	for ( unsigned int i = 0; i < CDG_BORDER_WIDTH; i++ )
	{
		for ( unsigned int j = 0; j < CDG_FULL_HEIGHT; j++ )
		{
			m_image.setPixel( i, j, borderColor );
			m_image.setPixel( CDG_FULL_WIDTH - i - 1, j, borderColor );
		}
	}

	for ( unsigned int i = 0; i < CDG_FULL_WIDTH; i++ )
	{
		for ( unsigned int j = 0; j < CDG_BORDER_HEIGHT; j++ )
		{
			m_image.setPixel( i, j, borderColor );
			m_image.setPixel( i, CDG_FULL_HEIGHT - j - 1, borderColor );
		}
	}

//	qDebug( "BorderPreset: filling border with color %d (%08X)", preset->color & 0x0F, borderColor );
}

void CDGRenderer::cmdLoadColorTable( const char * data, int index )
{
	CDG_LoadColorTable* table = (CDG_LoadColorTable*) data;

//	qDebug( "LoadColors: filling color table %d", index / 8 );

	for ( int i = 0; i < 8; i++ )
	{
		quint32 colourEntry = ((table->colorSpec[2 * i] & CDG_MASK) << 8);
		colourEntry = colourEntry + (table->colorSpec[(2 * i) + 1] & CDG_MASK);
		colourEntry = ((colourEntry & 0x3F00) >> 2) | (colourEntry & 0x003F);

		quint8 red = ((colourEntry & 0x0F00) >> 8) * 17;
		quint8 green = ((colourEntry & 0x00F0) >> 4) * 17;
		quint8 blue = ((colourEntry & 0x000F)) * 17;

		quint32 color = qRgba( red, green, blue, 0xFF );
		m_image.setColor( index+ i, color );

//		qDebug( "LoadColors: color %d -> %02X %02X %02X (%08X)", index + i, red, green, blue, color );
	}
}

void CDGRenderer::cmdTileBlockXor( const char * data )
{
	CDG_Tile* tile = (CDG_Tile*) data;
	quint32 offset_y = (tile->row & 0x1F) * 12;
	quint32 offset_x = (tile->column & 0x3F) * 6;

//	qDebug( "TileBlockXor: %d, %d", offset_x, offset_y );

	if ( offset_x + 6 >= CDG_FULL_WIDTH || offset_y + 12 >= CDG_FULL_HEIGHT )
		return;

	// In the XOR variant, the color values are combined with the color values that are
	// already onscreen using the XOR operator.  Since CD+G only allows a maximum of 16
	// colors, we are XORing the pixel values (0-15) themselves, which correspond to
	// indexes into a color lookup table.  We are not XORing the actual R,G,B values.
	quint8 color_0 = tile->color0 & 0x0F;
	quint8 color_1 = tile->color1 & 0x0F;

	quint8 mask[6] = { 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

	for ( int i = 0; i < 12; i++ )
	{
		quint8 bTemp = tile->tilePixels[i] & 0x3F;

		for ( int j = 0; j < 6; j++ )
		{
			QPoint p( offset_x + j, offset_y + i );

			// Find the original color index
			quint8 origindex = m_image.pixelIndex( p );

			if ( bTemp & mask[j] )  //pixel xored with color1
				m_image.setPixel( p, origindex ^ color_1 );
			else
				m_image.setPixel( p, origindex ^ color_0 );
		}
	}
}
