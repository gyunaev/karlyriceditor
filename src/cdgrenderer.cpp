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
#include <QFile>
#include "cdgrenderer.h"


CDGRenderer::CDGRenderer()
	: LyricsRenderer()
{
	m_streamIdx = -1;
	m_hOffset = 0;
	m_vOffset = 0;
	m_borderColor = 0;
	m_bgColor = 0;

	memset( m_cdgScreen, 0, sizeof(m_cdgScreen) );

	for ( int i = 0; i < 16; i++ )
		m_colorTable[i] = 0;
}

CDGRenderer::~CDGRenderer()
{
}

quint8 CDGRenderer::getPixel( int x, int y )
{
	unsigned int offset = x + y * CDG_FULL_WIDTH;

	if ( x >= (int) CDG_FULL_WIDTH || y >= (int) CDG_FULL_HEIGHT )
		return m_borderColor;

	if ( x < 0 || y < 0 || offset > CDG_FULL_HEIGHT * CDG_FULL_WIDTH )
	{
		qWarning( "CDG renderer: requested pixel (%d,%d) is out of boundary", x, y );
		return 0;
	}

	return m_cdgScreen[offset];
}

void CDGRenderer::setPixel( int x, int y, quint8 color )
{
	unsigned int offset = x + y * CDG_FULL_WIDTH;

	if ( x < 0 || y < 0 || offset > CDG_FULL_HEIGHT * CDG_FULL_WIDTH )
	{
		qWarning( "CDG renderer: set pixel (%d,%d) is out of boundary", x, y );
		return;
	}

	m_cdgScreen[offset] = color;
}

void CDGRenderer::cmdMemoryPreset( const char * data )
{
	CDG_MemPreset* preset = (CDG_MemPreset*) data;

	if ( preset->repeat & 0x0F )
		return;  // No need for multiple clearings

	m_bgColor = preset->color & 0x0F;

	for ( unsigned int i = CDG_BORDER_WIDTH; i < CDG_FULL_WIDTH - CDG_BORDER_WIDTH; i++ )
		for ( unsigned int  j = CDG_BORDER_HEIGHT; j < CDG_FULL_HEIGHT - CDG_BORDER_HEIGHT; j++ )
			setPixel( i, j, m_bgColor );
}

void CDGRenderer::cmdBorderPreset( const char * data )
{
	CDG_BorderPreset* preset = (CDG_BorderPreset*) data;

	m_borderColor = preset->color & 0x0F;

	for ( unsigned int i = 0; i < CDG_BORDER_WIDTH; i++ )
	{
		for ( unsigned int j = 0; j < CDG_FULL_HEIGHT; j++ )
		{
			setPixel( i, j, m_borderColor );
			setPixel( CDG_FULL_WIDTH - i - 1, j, m_borderColor );
		}
	}

	for ( unsigned int i = 0; i < CDG_FULL_WIDTH; i++ )
	{
		for ( unsigned int j = 0; j < CDG_BORDER_HEIGHT; j++ )
		{
			setPixel( i, j, m_borderColor );
			setPixel( i, CDG_FULL_HEIGHT - j - 1, m_borderColor );
		}
	}

	//CLog::Log( LOGDEBUG, "CDG: border color set to %d", borderColor );
}

void CDGRenderer::cmdTransparentColor( const char * data )
{
	int index = data[0] & 0x0F;
	m_colorTable[index] = 0xFFFFFFFF;
}

void CDGRenderer::cmdLoadColorTable( const char * data, int index )
{
	CDG_LoadColorTable* table = (CDG_LoadColorTable*) data;

	for ( int i = 0; i < 8; i++ )
	{
		unsigned int colourEntry = ((table->colorSpec[2 * i] & CDG_MASK) << 8);
		colourEntry = colourEntry + (table->colorSpec[(2 * i) + 1] & CDG_MASK);
		colourEntry = ((colourEntry & 0x3F00) >> 2) | (colourEntry & 0x003F);

		quint8 red = ((colourEntry & 0x0F00) >> 8) * 17;
		quint8 green = ((colourEntry & 0x00F0) >> 4) * 17;
		quint8 blue = ((colourEntry & 0x000F)) * 17;

		m_colorTable[index+i] = (red << 16) | (green << 8) | blue;

		//CLog::Log( LOGDEBUG, "CDG: loadColors: color %d -> %02X %02X %02X (%08X)", index + i, red, green, blue, m_colorTable[index+i] );
	}
}

void CDGRenderer::cmdTileBlock( const char * data )
{
	CDG_Tile* tile = (CDG_Tile*) data;
	unsigned int offset_y = (tile->row & 0x1F) * 12;
	unsigned int offset_x = (tile->column & 0x3F) * 6;

	//CLog::Log( LOGERROR, "TileBlockXor: %d, %d", offset_x, offset_y );

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
			if ( bTemp & mask[j] )
				setPixel( offset_x + j, offset_y + i, color_1 );
			else
				setPixel( offset_x + j, offset_y + i, color_0 );
		}
	}
}

void CDGRenderer::cmdTileBlockXor( const char * data )
{
	CDG_Tile* tile = (CDG_Tile*) data;
	unsigned int offset_y = (tile->row & 0x1F) * 12;
	unsigned int offset_x = (tile->column & 0x3F) * 6;

	//CLog::Log( LOGERROR, "TileBlockXor: %d, %d", offset_x, offset_y );

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
			// Find the original color index
			quint8 origindex = getPixel( offset_x + j, offset_y + i );

			if ( bTemp & mask[j] )  //pixel xored with color1
				setPixel( offset_x + j, offset_y + i, origindex ^ color_1 );
			else
				setPixel( offset_x + j, offset_y + i, origindex ^ color_0 );
		}
	}
}

// Based on http://cdg2video.googlecode.com/svn/trunk/cdgfile.cpp
void CDGRenderer::cmdScroll( const char * data, bool copy )
{
	int colour, hScroll, vScroll;
	int hSCmd, hOffset, vSCmd, vOffset;
	int vScrollPixels, hScrollPixels;

	// Decode the scroll command parameters
	colour  = data[0] & 0x0F;
	hScroll = data[1] & 0x3F;
	vScroll = data[2] & 0x3F;

	hSCmd = (hScroll & 0x30) >> 4;
	hOffset = (hScroll & 0x07);
	vSCmd = (vScroll & 0x30) >> 4;
	vOffset = (vScroll & 0x0F);

	m_hOffset = hOffset < 5 ? hOffset : 5;
	m_vOffset = vOffset < 11 ? vOffset : 11;

	// Scroll Vertical - Calculate number of pixels
	vScrollPixels = 0;

	if (vSCmd == 2)
	{
		vScrollPixels = - 12;
	}
	else  if (vSCmd == 1)
	{
		vScrollPixels = 12;
	}

	// Scroll Horizontal- Calculate number of pixels
	hScrollPixels = 0;

	if (hSCmd == 2)
	{
		hScrollPixels = - 6;
	}
	else if (hSCmd == 1)
	{
		hScrollPixels = 6;
	}

	if (hScrollPixels == 0 && vScrollPixels == 0)
	{
		return;
	}

	// Perform the actual scroll.
	unsigned char temp[CDG_FULL_HEIGHT][CDG_FULL_WIDTH];
	int vInc = vScrollPixels + CDG_FULL_HEIGHT;
	int hInc = hScrollPixels + CDG_FULL_WIDTH;
	unsigned int ri; // row index
	unsigned int ci; // column index

	for (ri = 0; ri < CDG_FULL_HEIGHT; ++ri)
	{
		for (ci = 0; ci < CDG_FULL_WIDTH; ++ci)
		{
			temp[(ri + vInc) % CDG_FULL_HEIGHT][(ci + hInc) % CDG_FULL_WIDTH] = getPixel( ci, ri );
		}
	}

	// if copy is false, we were supposed to fill in the new pixels
	// with a new colour. Go back and do that now.

	if (!copy)
	{
		if (vScrollPixels > 0)
		{
			for (ci = 0; ci < CDG_FULL_WIDTH; ++ci)
			{
				for (ri = 0; ri < (unsigned int)vScrollPixels; ++ri) {
					temp[ri][ci] = colour;
				}
			}
		}
		else if (vScrollPixels < 0)
		{
			for (ci = 0; ci < CDG_FULL_WIDTH; ++ci)
			{
				for (ri = CDG_FULL_HEIGHT + vScrollPixels; ri < CDG_FULL_HEIGHT; ++ri) {
					temp[ri][ci] = colour;
				}
			}
		}

		if (hScrollPixels > 0)
		{
			for (ci = 0; ci < (unsigned int)hScrollPixels; ++ci)
			{
				for (ri = 0; ri < CDG_FULL_HEIGHT; ++ri) {
					temp[ri][ci] = colour;
				}
			}
		}
		else if (hScrollPixels < 0)
		{
			for (ci = CDG_FULL_WIDTH + hScrollPixels; ci < CDG_FULL_WIDTH; ++ci)
			{
				for (ri = 0; ri < CDG_FULL_HEIGHT; ++ri) {
					temp[ri][ci] = colour;
				}
			}
		}
	}

	// Now copy the temporary buffer back to our array
	for (ri = 0; ri < CDG_FULL_HEIGHT; ++ri)
	{
		for (ci = 0; ci < CDG_FULL_WIDTH; ++ci)
		{
			setPixel( ci, ri, temp[ri][ci] );
		}
	}
}

void CDGRenderer::setCDGdata( const QByteArray& cdgdata )
{
	// Parse the CD+G stream
	int buggy_commands = 0;

	m_cdgStream.clear();
	m_cdgStream.reserve( cdgdata.size() / sizeof( SubCode ) );

	for ( int offset = 0; offset < cdgdata.size(); offset += sizeof( SubCode ) )
	{
		SubCode * sc = (SubCode *) (cdgdata.data() + offset);

		if ( ( sc->command & CDG_MASK) == CDG_COMMAND )
		{
			CDGPacket packet;

			// Validate the command and instruction
			switch ( sc->instruction & CDG_MASK )
			{
				case CDG_INST_MEMORY_PRESET:
				case CDG_INST_BORDER_PRESET:
				case CDG_INST_LOAD_COL_TBL_0_7:
				case CDG_INST_LOAD_COL_TBL_8_15:
				case CDG_INST_TILE_BLOCK_XOR:
				case CDG_INST_TILE_BLOCK:
				case CDG_INST_DEF_TRANSP_COL:
				case CDG_INST_SCROLL_PRESET:
				case CDG_INST_SCROLL_COPY:
					memcpy( &packet.subcode, sc, sizeof(SubCode) );
					packet.packetnum = offset / sizeof( SubCode );
					m_cdgStream.push_back( packet );
					break;

				default:
					buggy_commands++;
					break;
			}
		}
	}

	// Init the screen
	memset( m_cdgScreen, 0, sizeof(m_cdgScreen) );

	// Init color table
	for ( int i = 0; i < 16; i++ )
		m_colorTable[i] = 0;

	m_streamIdx = 0;
	m_borderColor = 0;
	m_bgColor = 0;
	m_hOffset = 0;
	m_vOffset = 0;

	if ( buggy_commands > 0 )
		qDebug( "CDG loader: CDG file was damaged, %d errors ignored", buggy_commands );
}

void CDGRenderer::dumpPacket( CDGPacket * packet )
{
	SubCode& sc = packet->subcode;
	const char * data = sc.data;

	QString dumpstr = "Packet " + QString::number( packet->packetnum ) + ": ";

	switch ( sc.instruction & CDG_MASK )
	{
		case CDG_INST_MEMORY_PRESET:
		{
			CDG_MemPreset* preset = (CDG_MemPreset*) data;
			dumpstr += "CDG_INST_MEMORY_PRESET, color " + QString::number( preset->color & 0x0F ) + ", repeat " + QString::number( preset->repeat & 0x0F );
			break;
		}

		case CDG_INST_BORDER_PRESET:
		{
			CDG_BorderPreset* preset = (CDG_BorderPreset*) data;
			dumpstr += "CDG_INST_BORDER_PRESET, color " + QString::number( preset->color & 0x0F );
			break;
		}

		case CDG_INST_LOAD_COL_TBL_0_7:
		{
			dumpstr += "CDG_INST_LOAD_COL_TBL_0_7\n";

			CDG_LoadColorTable* table = (CDG_LoadColorTable*) data;

			for ( int i = 0; i < 8; i++ )
			{
				unsigned int colourEntry = ((table->colorSpec[2 * i] & CDG_MASK) << 8);
				colourEntry = colourEntry + (table->colorSpec[(2 * i) + 1] & CDG_MASK);
				colourEntry = ((colourEntry & 0x3F00) >> 2) | (colourEntry & 0x003F);

				quint8 red = ((colourEntry & 0x0F00) >> 8) * 17;
				quint8 green = ((colourEntry & 0x00F0) >> 4) * 17;
				quint8 blue = ((colourEntry & 0x000F)) * 17;

				dumpstr += QString("    %1: %2 %3 %4\n") .arg( i ) .arg( red ) .arg( green ) .arg( blue );
			}
			break;
		}

		case CDG_INST_LOAD_COL_TBL_8_15:
		{
			dumpstr += "CDG_INST_LOAD_COL_TBL_8_15\n";

			CDG_LoadColorTable* table = (CDG_LoadColorTable*) data;

			for ( int i = 0; i < 8; i++ )
			{
				unsigned int colourEntry = ((table->colorSpec[2 * i] & CDG_MASK) << 8);
				colourEntry = colourEntry + (table->colorSpec[(2 * i) + 1] & CDG_MASK);
				colourEntry = ((colourEntry & 0x3F00) >> 2) | (colourEntry & 0x003F);

				quint8 red = ((colourEntry & 0x0F00) >> 8) * 17;
				quint8 green = ((colourEntry & 0x00F0) >> 4) * 17;
				quint8 blue = ((colourEntry & 0x000F)) * 17;

				dumpstr += QString("    %1: %2 %3 %4\n") .arg( i + 8 ) .arg( red ) .arg( green ) .arg( blue );
			}
			break;
		}

		case CDG_INST_DEF_TRANSP_COL:
			dumpstr += "CDG_INST_DEF_TRANSP_COL, color " + QString::number( data[0] & 0x0F );
			break;

		case CDG_INST_TILE_BLOCK:
		{
			CDG_Tile* tile = (CDG_Tile*) data;
			unsigned int offset_y = (tile->row & 0x1F) * 12;
			unsigned int offset_x = (tile->column & 0x3F) * 6;

			dumpstr += "CDG_INST_TILE_BLOCK\n";

			quint8 color_0 = tile->color0 & 0x0F;
			quint8 color_1 = tile->color1 & 0x0F;

			quint8 mask[6] = { 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

			for ( int i = 0; i < 12; i++ )
			{
				quint8 bTemp = tile->tilePixels[i] & 0x3F;

				for ( int j = 0; j < 6; j++ )
				{
					if ( bTemp & mask[j] )
						dumpstr += "    pixel " + QString::number( offset_x + j ) + "," + QString::number(offset_y + i) + " -> " + QString::number(color_1) + "\n";
					else
						dumpstr += "    pixel " + QString::number( offset_x + j ) + ", " + QString::number(offset_y + i) + " -> " + QString::number(color_0) + "\n";
				}
			}
			break;
		}

		case CDG_INST_TILE_BLOCK_XOR:
		{
			CDG_Tile* tile = (CDG_Tile*) data;
			unsigned int offset_y = (tile->row & 0x1F) * 12;
			unsigned int offset_x = (tile->column & 0x3F) * 6;

			dumpstr += "CDG_INST_TILE_BLOCK_XOR\n";

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
					// Find the original color index
					quint8 origindex = getPixel( offset_x + j, offset_y + i );

					if ( bTemp & mask[j] )  //pixel xored with color1
						dumpstr += "    " + QString::number( origindex ) + " pixel at " + QString::number( offset_x + j ) + "," + QString::number(offset_y + i) + " ^ " + QString::number(color_1) + "\n";
					else
						dumpstr += "    " + QString::number( origindex ) + " pixel at " + QString::number( offset_x + j ) + "," + QString::number(offset_y + i) + " ^ " + QString::number(color_0) + "\n";
				}
			}
			break;
		}

		case CDG_INST_SCROLL_PRESET:
		case CDG_INST_SCROLL_COPY:
		{
			int colour  = data[0] & 0x0F;
			int hScroll = data[1] & 0x3F;
			int vScroll = data[2] & 0x3F;

			int hSCmd = (hScroll & 0x30) >> 4;
			int hOffset = (hScroll & 0x07);
			int vSCmd = (vScroll & 0x30) >> 4;
			int vOffset = (vScroll & 0x0F);


			if ( (sc.instruction & CDG_MASK) == CDG_INST_SCROLL_COPY )
				dumpstr += "CDG_INST_SCROLL_COPY";
			else
				dumpstr += "CDG_INST_SCROLL_PRESET";

			dumpstr += ": hSCmd " + QString::number( hSCmd ) + ", hoffset " + QString::number(hOffset)
					+ "vSCmd " + QString::number( vSCmd ) + ", voffset " + QString::number(vOffset) + ", color " + QString::number(colour);
			break;
		}

		default:
			abort();
	}

	QFile file( "cdg.dump" );
	if ( !file.open( QIODevice::Append | QIODevice::WriteOnly ) )
		abort();

	dumpstr += "\n";
	file.write( dumpstr.toLocal8Bit() );
	file.close();
}

int CDGRenderer::UpdateBuffer( unsigned int packets_due )
{
	int status = UPDATE_NOCHANGE;

	// Are we done?
	if ( m_streamIdx == -1 )
		return status;

	// Was the stream position reversed? In this case we have to "replay" the whole stream
	// as the screen is a state machine, and "clear" may not be there.
	if ( m_streamIdx > 0 && m_cdgStream[ m_streamIdx-1 ].packetnum > packets_due )
	{
		qDebug( "CDG renderer: packet number changed backward (%d played, %d asked", m_cdgStream[ m_streamIdx-1 ].packetnum, packets_due );
		m_streamIdx = 0;
	}

	// Process all packets already due
	while ( m_cdgStream[ m_streamIdx ].packetnum <= packets_due )
	{
		SubCode& sc = m_cdgStream[ m_streamIdx ].subcode;

		//dumpPacket( &m_cdgStream[ m_streamIdx ] );

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

			case CDG_INST_DEF_TRANSP_COL:
				cmdTransparentColor( sc.data );
				break;

			case CDG_INST_TILE_BLOCK:
				cmdTileBlock( sc.data );
				status = UPDATE_COLORCHANGE;
				break;

			case CDG_INST_TILE_BLOCK_XOR:
				cmdTileBlockXor( sc.data );
				status = UPDATE_COLORCHANGE;
				break;

			case CDG_INST_SCROLL_PRESET:
				cmdScroll( sc.data, false );
				status = UPDATE_COLORCHANGE;
				break;

			case CDG_INST_SCROLL_COPY:
				cmdScroll( sc.data, true );
				status = UPDATE_COLORCHANGE;
				break;

			default: // this shouldn't happen as we validated the stream in Load()
				break;
		}

		m_streamIdx++;

		if ( m_streamIdx >= (int) m_cdgStream.size() )
		{
			m_streamIdx = -1;
			break;
		}
	}

	return status;;
}


int CDGRenderer::update( qint64 songTime )
{
	int status;

	// Make the generic image twice larger
	if ( m_image.width() < (int) (2 * CDG_FULL_WIDTH) )
	{
		m_image = QImage( 2 * CDG_FULL_WIDTH, 2 * CDG_FULL_HEIGHT, QImage::Format_ARGB32 );
		status = UPDATE_RESIZED;
	}

	// Time to update?
	unsigned int packets_due = songTime * 300 / 1000;

	if ( status == UPDATE_RESIZED || (status = UpdateBuffer( packets_due )) != UPDATE_NOCHANGE )
	{
		QImage img( QSize( CDG_FULL_WIDTH, CDG_FULL_HEIGHT ), QImage::Format_ARGB32 );

		// Update the image
		for ( unsigned int y = 0; y < CDG_FULL_HEIGHT; y++ )
		{
			for ( unsigned int x = 0; x < CDG_FULL_WIDTH; x++ )
			{
				quint8 colorindex = getPixel( x + m_hOffset, y + m_vOffset );
				quint32 TexColor = m_colorTable[ colorindex ];

				// Is it transparent color?
				if ( TexColor != 0xFFFFFFFF )
				{
					// Uncomment this to make background colors transparent
					//if ( colorindex != m_bgColor )
					TexColor |= 0xFF000000; // color table has 0x00 alpha
				}
				else
					TexColor = 0x00000000;

				img.setPixel( x, y, TexColor );
			}
		}

		m_image = img.scaled( m_image.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
	}

	return status;
}
