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

#include <QTextDocument>

#include "lyricsrenderer.h"
#include "settings.h"

// How long (ms) to show the line/block before the song actually starts
static const int LYRICS_SHOW_BEFORE = 5000;

// How long (ms) to keep the line/block on screen before hiding it
static const int LYRICS_SHOW_AFTER = 5000;


LyricsRenderer::LyricsRenderer()
{
	m_prefetch = 0;
}

void LyricsRenderer::setLyrics( const Lyrics& lyrics )
{
	m_text.clear();
	m_titlePage.clear();
	m_colorActive = "white";
	m_colorInactive = "green";

	if ( lyrics.isEmpty() )
		qFatal("LyricRenderer called with empty lyrics");

	// Get the time the first lyrics is shown
	qint64 timing = lyrics.block( 0 ).at(0).at(0).timing;

	// Depending on how much time we have before having showing the lyrics, we show the title
	// for 5 seconds or less (or not show at all if there is no time)
	if ( timing > LYRICS_SHOW_BEFORE )
		m_titleTimingCut = qMin( (int) (timing - LYRICS_SHOW_BEFORE), 5000 );
	else
		m_titleTimingCut = 0;

	// Index the lyrics
	m_lyricIndex.clear();
	m_blockIndex.clear();

	if ( pSettings->m_editorSupportBlocks )
	{
		// Block mode - fill the m_lyricIndex array
		for ( int bl = 0; bl < lyrics.totalBlocks(); bl++ )
		{
			const Lyrics::Block& block = lyrics.block( bl );

			for ( int ln = 0; ln < block.size(); ln++ )
			{
				const Lyrics::Line& line = block[ln];

				for ( int pos = 0; pos < line.size(); pos++ )
				{
					Lyrics::Syllable lentry = line[pos];
					LyricIndex lidx;

					lidx.timestart = lentry.timing;
					lidx.blockindex = bl;
					lidx.text = lentry.text;

					if ( pos == line.size() - 1 )
					{
						lidx.text += "\n";
						m_lyricIndex.push_back( lidx );
					}
					else
						m_lyricIndex.push_back( lidx );
				}
			}
		}

		// Now rescan the index, and split any which have more than one char
		for ( int i = 0; i < m_lyricIndex.size(); i++ )
		{
			if ( m_lyricIndex[i].text.trimmed().length() > 1 )
				splitSyllable( i );
		}

		// Create the m_blockIndex
		int current_block = -1;

		for ( int i = 0; i < m_lyricIndex.size(); i++ )
		{
			if ( m_lyricIndex[i].blockindex != current_block )
			{
				Time blocktime;
				blocktime.timestart = blocktime.timeend = m_lyricIndex[i].timestart;
				blocktime.index = i;
				m_blockIndex.push_back( blocktime );

				current_block = m_lyricIndex[i].blockindex;
			}
			else
				m_blockIndex.back().timeend = m_lyricIndex[i].timestart;
		}

		// Apply before/after delays
		for ( int i = 0; i < m_blockIndex.size(); i++ )
		{
			if ( i == 0 || m_blockIndex[i].timestart - LYRICS_SHOW_BEFORE > m_blockIndex[i-1].timeend )
				m_blockIndex[i].timestart = qMax( (qint64) 0, m_blockIndex[i].timestart - LYRICS_SHOW_BEFORE );
			else
			{
				if ( m_prefetch > 0 && m_blockIndex[i].timestart - m_blockIndex[i-1].timeend < m_prefetch )
					m_blockIndex[i-1].timeend = m_blockIndex[i].timestart - m_prefetch;

				m_blockIndex[i].timestart = m_blockIndex[i-1].timeend + 1;
			}

			if ( i == m_blockIndex.size() - 1 || m_blockIndex[i].timeend + LYRICS_SHOW_AFTER < m_blockIndex[i+1].timestart )
				m_blockIndex[i].timeend += LYRICS_SHOW_AFTER;
//			else
//				m_blockIndex[i].timeend = m_blockIndex[i+1].timestart - 1;
		}

		// Dump block index
//		for ( int i = 0; i < m_blockIndex.size(); i++ )
//			qDebug("BlockIndex for block %d: idx %d, %d - %d", i, m_blockIndex[i].index, (int) m_blockIndex[i].timestart, (int) m_blockIndex[i].timeend );

		// Dump regular index
//		for ( int i = 0; i < m_lyricIndex.size(); i++ )
//			qDebug("lyric %d: time %d, block %d, text %s", i, (int) m_lyricIndex[i].timestart, m_lyricIndex[i].blockindex, qPrintable( m_lyricIndex[i].text ) );
	}
	else
	{
		// Line mode
		for ( int bl = 0; bl < lyrics.totalBlocks(); bl++ )
		{
			const Lyrics::Block& block = lyrics.block( bl );

			for ( int ln = 0; ln < block.size(); ln++ )
			{
				const Lyrics::Line& line = block[ln];
				LyricIndex lidx;

				lidx.timestart = line[0].timing;
				lidx.blockindex = bl;

				for ( int pos = 0; pos < line.size(); pos++ )
					lidx.text += line[pos].text;

				m_lyricIndex.push_back( lidx );
			}
		}
	}
}

QString LyricsRenderer::update( qint64 tickmark )
{
	if ( !m_titlePage.isEmpty() && tickmark <= m_titleTimingCut )
		return m_titlePage;

	if ( pSettings->m_editorSupportBlocks )
		redrawBlocks( tickmark );
	else
		redrawLines( tickmark );

	return m_text;
}

void LyricsRenderer::redrawBlocks( qint64 tickmark )
{
	// Find the block containing current timemark
	int blockid = -1;

	for ( int i = 0; i < m_blockIndex.size(); i++ )
	{
		if ( tickmark >= m_blockIndex[i].timestart && tickmark <= m_blockIndex[i].timeend )
		{
			blockid = i;
			break;
		}
	}

	if ( blockid == -1 )
	{
		// Nothing to show
		m_text.clear();
		return;
	}

	int index = m_blockIndex[blockid].index;

	// Create a multiline string from current block.
	QString text = "<qt>";

	for ( ; index < m_lyricIndex.size() && m_lyricIndex[index].blockindex == blockid; index++ )
	{
		QString lyrictext = Qt::escape( m_lyricIndex[index].text );
		lyrictext.replace( "\n", "<br>" );

		if ( m_lyricIndex[index].timestart < tickmark )
		{
			// This entry hasn't been played yet
			text += QString("<font color=\"%1\">%2</font>") .arg( m_colorInactive ) .arg( lyrictext );
		}
		else if ( m_lyricIndex[index].timestart >= tickmark )
		{
			// This entry has been played
			text += QString("<font color=\"%1\">%2</font>") .arg( m_colorActive ) .arg( lyrictext );
		}
	}

	text += "</qt>";
	m_text = text;
}

void LyricsRenderer::redrawLines( qint64 tickmark )
{
	static const int LINES_TO_SHOW = 4;

	// Find the first line to show
	int activeline = -1;

	// Find the last active line
	for ( int i = 0; i < m_lyricIndex.size(); i++ )
	{
		if ( tickmark < m_lyricIndex[i].timestart )
			break;
		else
			activeline = i;
	}

	// If no line is active, the song is not started; start at the line 0.
	// If a line N is active, start at line N-1 (if any), or at size() - LINES_TO_SHOW, which is less
	int startline = 0;

	if ( activeline > 0 )
		startline =	qMin( activeline - 1, m_lyricIndex.size() - LINES_TO_SHOW );

	int limit = qMin( startline + LINES_TO_SHOW, m_lyricIndex.size() );

	// Create a multiline string.
	QString text = "<qt>";

	for ( int i = startline; i < limit; i++ )
	{
		if ( i == activeline )
			text += QString("<font color=\"%1\">%2</font>") .arg( m_colorActive ) .arg( m_lyricIndex[i].text );
		else
			text += QString("<font color=\"%1\">%2</font>") .arg( m_colorInactive ) .arg( m_lyricIndex[i].text );

		text += "<br>";
	}

	text += "</qt>";
	m_text = text;
}

void LyricsRenderer::splitSyllable( int index )
{
	LyricIndex orig = m_lyricIndex[ index ];

	qint64 timestart = orig.timestart;
	qint64 timeend = (index == m_lyricIndex.size() - 1) ? timestart : m_lyricIndex[ index + 1 ].timestart;
	QString text = orig.text;
	int ticksperchar = (timeend - timestart) / text.length();

	// Remove the old one - will be replaced by one or more
	m_lyricIndex.remove( index, 1 );

	for ( int i = 0; i < text.length(); i++ )
	{
		LyricIndex lidx;

		lidx.timestart = timestart + i * ticksperchar;
		lidx.blockindex = orig.blockindex;
		lidx.text = QString( text[i] );

		m_lyricIndex.insert( index, 1, lidx );
		index++;
	}
}

void LyricsRenderer::setColors( const QString& active, const QString& inactive )
{
	m_colorActive = active;
	m_colorInactive = inactive;
}

void LyricsRenderer::setTitlePage( const QString& titlepage )
{
	m_titlePage = titlepage;
}

void LyricsRenderer::setPrefetch( int prefetch )
{
	m_prefetch = prefetch;
}
