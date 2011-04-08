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

#include "lyrics.h"

Lyrics::Lyrics()
{
	m_scanning = false;
	m_added_eofs = 0;
	m_textStarts = 0;

	m_currentLyric.timing = -1;
	m_currentLyric.pitch = -1;
}

Lyrics::~Lyrics()
{
}

void Lyrics::beginLyrics()
{
	m_scanning = true;
	m_added_eofs = 0;
	m_textStarts = 0;

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

	compile();
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

QString Lyrics::pitchToNote( int pitch, bool )
{
	return QString::number( pitch );
}

void Lyrics::compile()
{
	m_playBlocks.clear();

	// Block mode - fill the m_lyricIndex array
	for ( int bl = 0; bl < totalBlocks(); bl++ )
	{
		const Block& block = this->block( bl );

		if ( block.size() == 0 )
			continue;

		BlockInfo binfo;
		binfo.blockstart = block.first().first().timing;
		binfo.blockend = block.last().last().timing;

		for ( int ln = 0; ln < block.size(); ln++ )
		{
			const Line& line = block[ln];

			// Calculate the time the line ends
			qint64 endlinetime = line.last().timing;

			// If the last timing tag for this line is empty, this is the end.
			// We prefer it, but if it is not the case, we'll find something else.
			if ( !line.last().text.isEmpty() )
			{
				qint64 calcenlinetime;

				if ( ln + 1 < block.size() )	// Beginning of next line in this block
					calcenlinetime = block[ln].first().timing;
				else if ( bl + 1 < totalBlocks() ) // Beginning of next block
					calcenlinetime = block[ln+1].first().timing;
				else // last line in last block
					calcenlinetime = endlinetime + 2000; // 2 sec

				endlinetime = qMin( calcenlinetime, endlinetime + 2000 );
			}

			// Last item must be empty, so it is ok
			for ( int pos = 0; pos < line.size(); pos++ )
			{
				Syllable lentry = line[pos];

				if ( lentry.text.trimmed().isEmpty() )
					continue;

				if ( m_textStarts == 0 )
					m_textStarts = lentry.timing;

				qint64 starttime = line[pos].timing;
				qint64 endtime = (pos + 1 == line.size()) ? endlinetime : line[pos+1].timing;
				int blockpos = binfo.text.size();

				// Split the line timing
				int timestep = qMax( 1, (int) ((endtime - starttime) / lentry.text.length() ) );

				for ( int ch = 0; ch < lentry.text.length(); ch++ )
				{
					if ( !lentry.text[ch].isLetterOrNumber() )
						continue;

					binfo.offsets [ starttime + ch * timestep ] = blockpos + ch;
				}

				binfo.text += lentry.text;
			}

			binfo.text += "\n";
			binfo.offsets [ endlinetime ] = binfo.text.size() - 1;
		}

		m_playBlocks.push_back( binfo );
	}

/*	// Dump the result
	qDebug("Block dump, %d blocks\n", m_playBlocks.size() );
	for ( int bl = 0; bl < m_playBlocks.size(); bl++ )
	{
		qDebug("Block %d: (%d-%d)\n", bl, (int) m_playBlocks[bl].blockstart,
			   (int) m_playBlocks[bl].blockend );

		for ( QMap< qint64, unsigned int >::const_iterator it = m_playBlocks[bl].offsets.begin();
				it != m_playBlocks[bl].offsets.end(); ++it )
		{
			int pos = it.value();

			qDebug("\tTiming %d, pos %d\n%s|%s",
				   (int) it.key(), pos,
				   qPrintable( m_playBlocks[bl].text.left( pos + 1 ) ),
				   qPrintable( m_playBlocks[bl].text.mid( pos + 1 ) ) );
		}
	}
	*/
}

bool Lyrics::blockForTime( qint64 timing, QString& block, int& position, qint64& nexttiming ) const
{
	for ( int bl = 0; bl < m_playBlocks.size(); bl++ )
	{
		if ( timing >= m_playBlocks[bl].blockstart && timing <= m_playBlocks[bl].blockend )
		{
			QMap< qint64, unsigned int >::const_iterator it = m_playBlocks[bl].offsets.find( timing );

			if ( it == m_playBlocks[bl].offsets.end() )
				it = m_playBlocks[bl].offsets.lowerBound( timing );

			if ( it == m_playBlocks[bl].offsets.end() )
				return false; // shouldnt happen

			block = m_playBlocks[bl].text;
			position = it.value();

			++it;

			if ( it == m_playBlocks[bl].offsets.end() )
				nexttiming = -1;
			else
				nexttiming = it.key();

			return true;
		}
	}

	return false;
}

bool Lyrics::nextBlock( qint64 current, qint64& time, QString& text ) const
{
	for ( int bl = 0; bl < m_playBlocks.size(); bl++ )
	{
		if ( current < m_playBlocks[bl].blockstart )
		{
			time = m_playBlocks[bl].blockstart;
			text = m_playBlocks[bl].text;
			return true;
		}
	}

	return false;
}

bool Lyrics::addBackgroundEvent( qint64 timing, const QString& text )
{
	return m_events.addEvent( timing, text );
}

LyricsEvents Lyrics::events() const
{
	return m_events;
}

qint64 Lyrics::firstLyric() const
{
	return m_textStarts;
}
