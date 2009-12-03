/***************************************************************************
 *   Copyright (C) 2009 Georgy Yunaev, gyunaev@ulduzsoft.com               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "lyrics.h"

Lyrics::Lyrics()
{
	m_scanning = false;
	m_added_eofs = 0;

	m_currentLyric.timing = -1;
	m_currentLyric.pitch = -1;
}

void Lyrics::beginLyrics()
{
	m_scanning = true;
	m_added_eofs = 0;

	m_currentLyric.timing = -1;
	m_currentLyric.pitch = -1;
	m_currentLyric.text.clear();
}

void Lyrics::curLyricSetTime( qint64 timems )
{
	if ( !m_scanning )
		abort();

	m_currentLyric.timing = timems;
}

void Lyrics::curLyricSetPitch( int pitch )
{
	if ( !m_scanning )
		abort();

	m_currentLyric.pitch = pitch;
}

void Lyrics::curLyricAppendText( const QString& text )
{
	if ( !m_scanning )
		abort();

	m_currentLyric.text += text;
}

void Lyrics::curLyricAdd()
{
	if ( !m_scanning )
		abort();

	// If time is not set, do not add
	if ( m_currentLyric.timing == -1 )
	{
		// Nothing to add
		m_currentLyric.pitch = -1;
		m_currentLyric.text.clear();

		return;
	}

	// Create a new block and new line, and fill the first entry
	// if there are no blocks or m_added_eofs > 1
	if ( m_lyrics.isEmpty() || m_added_eofs > 1 )
	{
		Block block;
		Line line;

		line.push_back( m_currentLyric );
		block.push_back( line );
		m_lyrics.push_back( block );
	}
	else
	{
		Block& lastBlock = m_lyrics.back();

		// Create a new line and fill the first entry if there
		// are no lines, or m_added_eofs > 0
		if ( lastBlock.isEmpty() || m_added_eofs > 0 )
		{
			Line line;

			line.push_back( m_currentLyric );
			lastBlock.push_back( line );
		}
		else
		{
			Line& lastLine = lastBlock.back();
			lastLine.push_back( m_currentLyric );
		}
	}

#if 0
	qDebug("block %d, line %d, entry %d: crlfs %d: %d %s",
		   m_lyrics.size() - 1,
		   m_lyrics.back().size() - 1,
		   m_lyrics.back().back().size() - 1,
		   m_added_eofs,
		   (int) m_currentLyric.timing,
		   qPrintable( m_currentLyric.text ) );
#endif

	// Reset the fields
	m_added_eofs = 0;

	m_currentLyric.timing = -1;
	m_currentLyric.pitch = -1;
	m_currentLyric.text.clear();
}

void Lyrics::curLyricAddEndOfLine()
{
	if ( !m_scanning )
		abort();

	curLyricAdd();
	m_added_eofs++;
}

void Lyrics::endLyrics()
{
	curLyricAdd();
	m_scanning = false;
}

int Lyrics::totalBlocks() const
{
	return m_lyrics.size();
}

const Lyrics::Block& Lyrics::block( int index ) const
{
	return m_lyrics[ index ];
}

bool Lyrics::isEmpty() const
{
	return m_lyrics.isEmpty();
}

void Lyrics::clear()
{
	m_lyrics.clear();
}

QString Lyrics::pitchToNore( int pitch )
{
	static const char * notes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

	return QString("%1, oct. %2") .arg( notes[ pitch % 12 ] ) .arg( pitch / 12 );
}
