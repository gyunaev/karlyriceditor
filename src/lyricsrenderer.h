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

#ifndef LYRICSRENDERER_H
#define LYRICSRENDERER_H

#include <QVector>
#include "lyrics.h"

class LyricsRenderer
{
	public:
		LyricsRenderer();

		void setLyrics( const Lyrics& lyrics, bool internaloutput = false );
		void setColors( const QString& active, const QString& inactive  );
		void setTitlePage( const QString& titlepage, unsigned int duration = 50 );

		// Sets time necessary for the block to be drawn, even if the previous block would
		// be hidden earlier
		void setPrefetch( int prefetch );

		QString update( qint64 tickmark );


	private:
		// Redraw the label using block or line mode
		void	redrawBlocks( qint64 tickmark );
		void	redrawLines( qint64 tickmark );
		void	setText( const QString& text );
		int		findBlockToShow( qint64 tickmark );
		void	splitSyllable( int index );

		typedef struct
		{
			qint64	timestart;
			QString	text;		// converted to HTML; includes <br> on line ends
			int		blockindex;

		} LyricIndex;

		typedef struct
		{
			qint64	timestart;	// for block
			qint64	timeend;	// for block
			int		index;		// in LyricIndex

		} Time;

		QVector<LyricIndex>	m_lyricIndex;

		// This one is used in block mode (LRC2 and UltraStar)
		QVector<Time>		m_blockIndex;

		// Rendered text
		QString				m_text;

		// Title page (if set, is shown at the beginning of the song). HTML allowed.
		QString				m_titlePage;
		qint64				m_titleTimingCut;

		// Colors
		QString				m_colorActive;
		QString				m_colorInactive;

		// Prefetch
		int					m_prefetch;

		// Output control
		QString				m_tagLyricsStart;
		QString				m_tagLyricsEnd;
		QString				m_tagColorPattern;
		QString				m_tagLineFeed;
		bool				m_escapeHTML;
};

#endif // LYRICSRENDERER_H
