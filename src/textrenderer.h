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

#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H

#include <QFont>
#include <QColor>

#include "lyricsrenderer.h"
#include "lyricsevents.h"
#include "lyrics.h"

class Project;

class TextRenderer : public LyricsRenderer
{
	public:
		TextRenderer( int width, int height );

		// Sets the lyrics to render. Resets all previously set params to defaults except setDisplaySize.
		void	setLyrics( const Lyrics& lyrics );

		// Lyrics data to render, overrides defaults from settings
		void	setRenderFont( const QFont& font );
		void	setColorBackground( const QColor& color );
		void	setColorTitle( const QColor& color );
		void	setColorToSing( const QColor& color );
		void	setColorSang( const QColor& color );
		void	setPreambleData( unsigned int height, unsigned int timems, unsigned int count );
		void	setTitlePageData( const QString& artist, const QString& title, const QString& userCreatedBy, unsigned int msec ); // duration = 0 - no title, default
		void	setColorAlpha( int alpha ); // 0 - 255

		// Force CD+G rendering mode (no anti-aliasing)
		void	forceCDGmode();

		// Typically lyrics are shown a little before they are being sung, and kept after they end.
		// This function overrides default before (5000ms) and after (1000ms) lengths
		void	setDurations( unsigned int before, unsigned int after );

		// Some formats (like CD+G) draw lyrics pretty slow, which means if the lyrics screens
		// change immediately, at the time the next page is being drawn it is already sung.
		// This parameter changes the prefetch, meaning if its value is 1000, the new lyrics block
		// is always shown at least 1000ms before it is being sing, even if it is necessary to cut the
		// old block earlier. Default is zero.
		void	setPrefetch( unsigned int prefetch );

		// Draw a new lyrics image
		virtual int	update( qint64 timing );

		// Checks if a line or block fits into the requested image.
		static	bool checkFit( const QSize& imagesize, const QFont& font, const QString& text );

		// Autodetects the largest font size to fit all lyrics into a specific image size.
		int		autodetectFontSize( const QSize& imagesize, const QFont& font );

		// Verifies that all lyrics could be rendered into a specific image size using the provided font
		bool	verifyFontSize( const QSize& imagesize, const QFont& font );

	private:
		// Returns the lyrics bounding box for a line or for paragraph using the font specified,
		// or the default font if not specified
		QRect	boundingRect( int blockid, const QFont& font );

		void	init();
		void	prepareEvents();
		int		lyricForTime( qint64 tickmark, int * sungpos );
		QString	titleScreen() const;
		void	fixActionSequences( QString& block );
		void	drawLyrics( int blockid, int pos, const QRect& boundingRect );
		void	drawPreamble();
		void	drawBackground( qint64 timing );

	private:
		// Lyrics to render
		typedef struct
		{
			qint64	timestart;
			qint64	timeend;

			// Text is for the whole block, with all special characters stripped down
			QString	text;

			// Text offsets in block per specific time
			QMap< qint64, unsigned int > offsets;

			// Per-character color changes for following (non-sung) characters in the block.
			// if none, the default color is used
			QMap< unsigned int, QString > colors;

			// Per-character font size changes for following (non-sung) characters in the block.
			// if none, the default color is used
			QMap< unsigned int, int > fonts;

		} LyricBlockInfo;

		QVector< LyricBlockInfo >	m_lyricBlocks;

		// Compile a single line
		void	compileLine( const QString& line, qint64 starttime, qint64 endtime, LyricBlockInfo * binfo, bool *intitle );

		// True if the image must be redrawn even if lyrics didn't change
		bool					m_forceRedraw;

		// Rendering params
		QColor					m_colorBackground;
		QColor					m_colorTitle;
		QColor					m_colorToSing;
		QColor					m_colorSang;
		QFont					m_renderFont;
		unsigned int			m_preambleHeight;	// how tall the preamble square should be; 0 - no preamble
		unsigned int			m_preambleLengthMs;	// maximum time the preamble is shown
		unsigned int			m_preambleCount;	// how many preamble squares to draw for m_preambleLengthMs
		unsigned int			m_beforeDuration;
		unsigned int			m_afterDuration;
		unsigned int			m_prefetchDuration;

		// Handling the preamble stuff
		int						m_preambleTimeLeft;	// Time left to show the current preamble - 5000 ... 0
		int						m_lastDrawnPreamble; // Timing when the last time the preamble changed
		qint64					m_lastSungTime;
		bool					m_drawPreamble;

		int						m_lastBlockPlayed;
		int						m_lastPosition;

		// Background events
		LyricsEvents			m_lyricEvents;
};

#endif // TEXTRENDERER_H
