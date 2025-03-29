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

#include <QVector>
#include <QPainter>
#include <QMessageBox>

#include "textrenderer.h"
#include "settings.h"
#include "project.h"
#include "version.h"
#include "licensing.h"


// Some preamble constants
static const int PREAMBLE_SQUARE = 500; // 500ms for each square
static const int PREAMBLE_MIN_PAUSE = 5000; // 5000ms minimum pause between verses for the preamble to appear

// Font size difference
static const int SMALL_FONT_DIFF = 4; // 4px less


TextRenderer::TextRenderer( int width, int height )
	: LyricsRenderer()
{
	m_currentAlignment = VerticalBottom;
	m_cdgMode = false;
	m_image = QImage( width, height, QImage::Format_ARGB32 );
	init();
}

void TextRenderer::setLyrics( const Lyrics& lyrics )
{
	init();

	// Store the empty block at position 0 which will serve as future title placeholder
	LyricBlockInfo titleblock;
	titleblock.timestart = 0;
	titleblock.timeend = 0;
	m_lyricBlocks.push_back( titleblock );

	// Compile the lyrics
	for ( int bl = 0; bl < lyrics.totalBlocks(); bl++ )
	{
		const Lyrics::Block& block = lyrics.block( bl );

		if ( block.size() == 0 )
			continue;

		bool intitle = false;
		bool hadtext = false;
		LyricBlockInfo binfo;
		binfo.timestart = block.first().first().timing;
		binfo.timeend = block.last().last().timing;

		for ( int ln = 0; ln < block.size(); ln++ )
		{
			const Lyrics::Line& line = block[ln];

			// Calculate the time the line ends
			qint64 endlinetime = line.last().timing;

			// If the last timing tag for this line is empty, this is the end.
			// We prefer it, but if it is not the case, we'll find something else.
			if ( !line.last().text.isEmpty() )
			{
				qint64 calcenlinetime;

				if ( ln + 1 < block.size() )	// Beginning of next line in this block
					calcenlinetime = block[ln].first().timing;
				else if ( bl + 1 < lyrics.totalBlocks() ) // Beginning of next block
                    calcenlinetime = lyrics.block( bl+1 ).first().first().timing;
				else // last line in last block
					calcenlinetime = endlinetime + 2000; // 2 sec

				endlinetime = qMin( calcenlinetime, endlinetime + 2000 );
			}

			// Last item must be empty, so it is ok
			for ( int pos = 0; pos < line.size(); pos++ )
			{
				Lyrics::Syllable lentry = line[pos];

				if ( lentry.text.trimmed().isEmpty() && !hadtext )
					continue;

				if ( lentry.text.isEmpty() )
					continue;

				qint64 starttime = line[pos].timing;
				qint64 endtime = (pos + 1 == line.size()) ? endlinetime : line[pos+1].timing;

				// Reset the block start time in case all previous lines were empty
				if ( !hadtext )
				{
					hadtext = true;
					binfo.timestart = starttime;
				}

				compileLine( lentry.text, starttime, endtime, &binfo, &intitle );
			}

			binfo.text += "\n";
			binfo.offsets [ endlinetime ] = binfo.text.size() - 1;
		}

		binfo.text = binfo.text.trimmed();

		// Do not add the blocks with no text
		if ( !binfo.text.isEmpty() )
			m_lyricBlocks.push_back( binfo );
	}
/*
	// Dump the result
	qDebug("Block dump, %d blocks\n", m_lyricBlocks.size() );
	for ( int bl = 0; bl < m_lyricBlocks.size(); bl++ )
	{
		qDebug("Block %d: (%d-%d)\n", bl, (int) m_lyricBlocks[bl].timestart,
			   (int) m_lyricBlocks[bl].timeend );

		for ( QMap< qint64, unsigned int >::const_iterator it = m_lyricBlocks[bl].offsets.begin();
				it != m_lyricBlocks[bl].offsets.end(); ++it )
		{
			int pos = it.value();

			qDebug("\tTiming %d, pos %d\n%s|%s",
				   (int) it.key(), pos,
				   qPrintable( m_lyricBlocks[bl].text.left( pos + 1 ) ),
				   qPrintable( m_lyricBlocks[bl].text.mid( pos + 1 ) ) );
		}
	}
*/
	m_lyricEvents = lyrics.events();
	prepareEvents();
}

void TextRenderer::compileLine( const QString& line, qint64 starttime, qint64 endtime, LyricBlockInfo * binfo, bool *intitle )
{
	// Drawn text string
	QString drawntext;

	// Stores the character offsets which change their color with time
	QVector<int> timedcharacters;

	int blocktextstart = binfo->text.size();

	for ( int ch = 0; ch < line.length(); ch++ )
	{
		QChar lchar = line[ch];

		// Parse the special sequences
		if ( lchar == '@' )
		{
            // Title start/end: @$title
			if ( ch + 1 < line.length() && line[ch+1] == '$' )
			{
				*intitle = !*intitle;
				ch++;
				continue;
			}

            // Color change: $#00C0C0
			if ( ch + 7 < line.length() && line[ch+1] == '#' )
			{
				// Store the new 'unsung' color for this position
				QString colorname = line.mid( ch + 1, 7 );
				binfo->colors[ blocktextstart + drawntext.length() ] = colorname;
				ch += 7;
				continue;
			}

            // Font size change down: @<
			if ( ch + 1 < line.length() && line[ch+1] == '<' )
			{
				binfo->fonts[ blocktextstart + drawntext.length() ] = -SMALL_FONT_DIFF; // make the font smaller
				ch += 1;
				continue;
			}

            // Font size change up: @>
			if ( ch + 1 < line.length() && line[ch+1] == '>' )
			{
				binfo->fonts[ blocktextstart + drawntext.length() ] = SMALL_FONT_DIFF; // make the font larger
				ch += 1;
				continue;
			}

            // Vertical text alignment: @%T - top, @%M - middle, @%B - bottom
            if ( ch + 2 < line.length() && line[ch+1] == '%' )
			{
                switch ( line[ch+2].unicode() )
                {
                    case 'T':
                        m_currentAlignment = VerticalTop;
                        break;

                    case 'M':
                        m_currentAlignment = VerticalMiddle;
                        break;

                    case 'B':
                        m_currentAlignment = VerticalMiddle;
                        break;
                }

                ch += 2;
                continue;
			}
		}

		// Nothing changes the color in title mode; also skip non-letters
		if ( !*intitle && lchar.isLetterOrNumber() )
			timedcharacters.push_back( drawntext.length() );

		drawntext.push_back( lchar );
	}

	// Now we know the total number of characters, and can calc the time step
	if ( !timedcharacters.isEmpty() )
	{
		int timestep = qMax( 1, (int) ((endtime - starttime) / timedcharacters.size() ) );

		for ( int ch = 0; ch < timedcharacters.size(); ch++ )
			binfo->offsets [ starttime + ch * timestep ] = blocktextstart + timedcharacters[ch];
	}

	binfo->text += drawntext;
	binfo->verticalAlignment = m_currentAlignment;
}

void TextRenderer::setRenderFont( const QFont& font )
{
	m_renderFont = font;
	m_forceRedraw = true;
}

void TextRenderer::setColorBackground( const QColor& color )
{
	m_colorBackground = color;
	m_forceRedraw = true;
}

void TextRenderer::setColorTitle( const QColor& color )
{
	m_colorTitle = color;
	m_forceRedraw = true;
}

void TextRenderer::setColorToSing( const QColor& color )
{
	m_colorToSing = color;
	m_forceRedraw = true;
}

void TextRenderer::setColorSang( const QColor& color )
{
	m_colorSang = color;
	m_forceRedraw = true;
}

void TextRenderer::setColorAlpha( int alpha )
{
	m_colorTitle.setAlpha( alpha );
	m_colorToSing.setAlpha( alpha );
    m_colorSang.setAlpha( alpha );
}

void TextRenderer::setDefaultVerticalAlign(TextRenderer::VerticalAlignment align)
{
    m_currentAlignment = align;
}

void TextRenderer::setTitlePageData( const QString& artist, const QString& title, const QString& userCreatedBy, unsigned int msec )
{
	// setLyrics should be called before setTitlePageData
	if ( m_lyricBlocks.size() < 2 )
		abort();

	QString createdBy = QString("@<@%1Created by %2\nhttp://www.ulduzsoft.com/\n") .arg( m_colorToSing.name()) .arg( APP_NAME);

	if ( pLicensing->isEnabled() && pLicensing->isValid() && !userCreatedBy.isEmpty() )
		createdBy = userCreatedBy;

	createdBy = createdBy.replace( "<br>", "\n" );

	// Block 0 is reserved for us; fill it up
	QString titletext = QString("@%1%2\n\n%3\n\n%4")
							.arg( m_colorTitle.name() )
							.arg( artist )
							.arg( title )
							.arg( createdBy );

	// Do we have at least 500ms to show the title?
	if ( m_lyricBlocks[1].timestart < 500 && !m_lyricBlocks[1].offsets.isEmpty() )
		return;

	m_lyricBlocks[0].timestart = 0;
	m_lyricBlocks[0].timeend = qMin( (qint64) msec, m_lyricBlocks[1].timestart );

	// Compile the line, replacing the spec characters
	bool intitle = true;
	compileLine( titletext, m_lyricBlocks[0].timestart, m_lyricBlocks[0].timeend, &m_lyricBlocks[0], &intitle );

	m_forceRedraw = true;
}

void TextRenderer::init()
{
	m_forceRedraw = true;

	m_colorBackground = pSettings->m_previewBackground;
	m_colorTitle = QColor( Qt::white );
	m_colorToSing = pSettings->m_previewTextActive;
	m_colorSang = pSettings->m_previewTextInactive;
	setRenderFont( QFont( pSettings->m_previewFontFamily, pSettings->m_previewFontSize ) );

	m_preambleHeight = 0;
	m_preambleLengthMs = 0;
	m_preambleCount = 0;

	m_preambleTimeLeft = 0;
	m_lastDrawnPreamble = 0;
	m_lastSungTime = 0;
	m_drawPreamble = false;
	m_lastBlockPlayed = -2;
	m_lastPosition = -2;

	m_beforeDuration = 5000;
	m_afterDuration = 1000;
	m_prefetchDuration = 0;

	m_lyricBlocks.clear();
}

void TextRenderer::setPreambleData( unsigned int height, unsigned int timems, unsigned int count )
{
	m_preambleHeight = height;
	m_preambleLengthMs = timems;
	m_preambleCount = count;

	m_forceRedraw = true;
}

void TextRenderer::forceCDGmode()
{
	m_forceRedraw = true;
    m_cdgMode = true;
}

void TextRenderer::setDurations( unsigned int before, unsigned int after )
{
	m_beforeDuration = before;
	m_afterDuration = after;

	m_forceRedraw = true;
}

void TextRenderer::setPrefetch( unsigned int prefetch )
{
	m_prefetchDuration = prefetch;
	m_forceRedraw = true;
}

int TextRenderer::lyricForTime( qint64 tickmark, int * sungpos )
{
	int nextblk = -1;

	// Find the next playable lyric block
	for ( int bl = 0; bl < m_lyricBlocks.size(); bl++ )
	{
		if ( tickmark < m_lyricBlocks[bl].timestart )
		{
			nextblk = bl;
			break;
		}
	}

	// If there is a block within the prefetch timing, show it even if it overwrites the currently played block
	// (this is why this check is on top)
	if ( m_prefetchDuration > 0 && nextblk != -1 && m_lyricBlocks[nextblk].timestart - tickmark <= m_prefetchDuration )
	{
		// We update the timing, but the preamble show/noshow does not change here
		m_preambleTimeLeft = qMax( 0, (int) (m_lyricBlocks[nextblk].timestart - tickmark) );
		*sungpos = -1;
		return nextblk;
	}

	// Find the block which should be currently played, if any.
	int curblk = -1;
	int pos = -1;

	for ( int bl = 0; bl < m_lyricBlocks.size(); bl++ )
	{
		if ( tickmark < m_lyricBlocks[bl].timestart || tickmark > m_lyricBlocks[bl].timeend )
			continue;

		curblk = bl;

		QMap< qint64, unsigned int >::const_iterator it = m_lyricBlocks[bl].offsets.find( tickmark );

		if ( it == m_lyricBlocks[bl].offsets.end() )
			it = m_lyricBlocks[bl].offsets.lowerBound( tickmark );

		if ( it == m_lyricBlocks[bl].offsets.end() )
		{
			// This may happen if the whole block is title
			break;
		}

		pos = it.value();

		++it;
		break;
	}

	// Anything to play right now?
	if ( curblk == -1 )
	{
		// Nothing active to show, so if there is a block within next five seconds, show it.
		if ( nextblk != -1 && m_lyricBlocks[nextblk].timestart - tickmark <= m_beforeDuration )
		{
			m_preambleTimeLeft = qMax( 0, (int) (m_lyricBlocks[nextblk].timestart - tickmark) );

			// We show preamble if there was silence over PREAMBLE_MIN_PAUSE and the block
			// actually contains any time changes
			if ( tickmark - m_lastSungTime > PREAMBLE_MIN_PAUSE && !m_lyricBlocks[nextblk].offsets.isEmpty() )
			{
				// m_preambleHeight == 0 disables the preamble
				if ( m_preambleHeight > 0 )
					m_drawPreamble = true;
			}

			*sungpos = -1;
			return nextblk;
		}

		// No next block or too far away yet
		m_drawPreamble = false;
		return -1;
	}

	m_drawPreamble = false;

	// If we're singing something (i.e. pos > 0), remember the timing
	if ( pos >= 0 )
		m_lastSungTime = tickmark;

	*sungpos = pos;
	return curblk;
}

bool TextRenderer::verifyFontSize( const QSize& size, const QFont& font )
{
	// Initialize the fonts
	QFont normalfont = font;

	// Test all lyrics whether it fits
	for ( int bl = 0; bl < m_lyricBlocks.size(); bl++ )
	{
		QRect rect = boundingRect( bl, normalfont );

		// Still fit?
		if ( rect.width() >= size.width() || rect.height() >= size.height() )
		{
			// Not fit, use a previous font size
			return false;
		}
	}

	return true;
}

int	TextRenderer::autodetectFontSize( const QSize& size, const QFont& font )
{
	QFont normalfont = font;

	// Start with 8
	int fontsize = 8;

	while ( 1 )
	{
		normalfont.setPointSize( fontsize );

		if ( !verifyFontSize( size, normalfont ) )
			return fontsize - 1;

		fontsize++;
	}
}

bool TextRenderer::checkFit( const QSize& imagesize, const QFont& font, const QString& text )
{
	TextRenderer renderer(10,10);
	renderer.init();

	LyricBlockInfo testblock;
	testblock.timestart = 0;
	testblock.timeend = 0;

	bool intitle = true;
	renderer.compileLine( text, 0, 0, &testblock, &intitle );
	renderer.m_lyricBlocks.push_back( testblock );

	QRect rect = renderer.boundingRect( 0, font );

	return rect.width() <= imagesize.width() && rect.height() <= imagesize.height();
}


QRect TextRenderer::boundingRect( int blockid, const QFont& font )
{
	QString block = m_lyricBlocks[blockid].text;

	// Calculate the height
	QFont curFont(font);
	QFontMetrics metrics = QFontMetrics( curFont );

	// Calculate the width and height for every line
	int linewidth = 0, lineheight = 0, totalheight = 0, totalwidth = 0;
	int cur = 0;

	while ( 1 )
	{
		// Line/text end
		if ( cur >= block.length() || block[cur] == '\n' )
		{
			// Adjust the total height
			totalheight += lineheight;
			totalwidth = qMax( totalwidth, linewidth );

			if ( cur >= block.length() )
				break; // we're done here

			cur++;
			linewidth = 0;

			continue;
		}

		// We're calculating line width here, so check for any font change events
		QMap< unsigned int, int >::const_iterator fontchange = m_lyricBlocks[blockid].fonts.find( cur );

		if ( fontchange != m_lyricBlocks[blockid].fonts.end() )
		{
			curFont.setPointSize( curFont.pointSize() + fontchange.value() );
			metrics = QFontMetrics( curFont );
		}

        linewidth += metrics.horizontalAdvance( block[cur] );
		lineheight = qMax( lineheight, metrics.height() );
		cur++;
	}

	return QRect( 0, 0, totalwidth, totalheight );
}

void TextRenderer::drawLyrics( int blockid, int pos, const QRect& boundingRect )
{
    QString block = m_lyricBlocks[blockid].text;

    // Prepare the painter
    QPainter painter( &m_image );
    painter.setFont( m_renderFont );

    if ( m_cdgMode )
        painter.setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing, false );

    // Used in calculations only
    QFont curFont( m_renderFont );
    QFontMetrics metrics( curFont );
    QColor fallbackColor = m_colorToSing;

    if ( pos == -1 )
        painter.setPen( m_colorToSing );
    else
        painter.setPen( m_colorSang );

    // Get the height offset from the rect.
    int start_y = 0;
    int verticalAlignment = m_lyricBlocks[blockid].verticalAlignment;

    // Draw title in the center, the rest according to the current vertical alignment
    if ( blockid == 0 || verticalAlignment == VerticalMiddle )
        start_y = (m_image.height() - boundingRect.height()) / 2 + painter.fontMetrics().height();
    else if ( verticalAlignment == VerticalTop )
        start_y = painter.fontMetrics().height() + m_image.width() / 50;	// see drawPreamble() for the offset
    else
        start_y = (m_image.height() - boundingRect.height());

    // Draw the whole text
    int linestart = 0;
    int linewidth = 0;
    int cur = 0;

    while ( 1 )
    {
        // Line/text end
        if ( cur >= block.length() || block[cur] == '\n' )
        {
            // Now we know the width, calculate the start offset
            int start_x = (m_image.width() - linewidth) / 2;

            // Draw the line
            for ( int i = linestart; i < cur; i++ )
            {
                // Handle the font change events
                QMap< unsigned int, int >::const_iterator fontchange = m_lyricBlocks[blockid].fonts.find( i );

                if ( fontchange != m_lyricBlocks[blockid].fonts.end() )
                {
                    painter.setFont( QFont(painter.font().family(), painter.font().pointSize() + fontchange.value() ) );
                }

                // Handle the color change events if pos doesn't cover them
                QMap< unsigned int, QString >::const_iterator colchange = m_lyricBlocks[blockid].colors.find( i );

                if ( colchange != m_lyricBlocks[blockid].colors.end() )
                {
                    QColor newcolor( colchange.value() );
                    fallbackColor = newcolor;

                    if ( i > pos )
                        painter.setPen( newcolor );
                }

                if ( pos != -1 && i >= pos )
                {
                    painter.setPen( fallbackColor );
                }

                // Outline
                const int OL = 1;
                painter.save();
                painter.setPen( Qt::black );
                painter.drawText( start_x - OL, start_y - OL, (QString) block[i] );
                painter.drawText( start_x + OL, start_y - OL, (QString) block[i] );
                painter.drawText( start_x - OL, start_y + OL, (QString) block[i] );
                painter.drawText( start_x + OL, start_y + OL, (QString) block[i] );
                painter.restore();

                painter.drawText( start_x, start_y, (QString) block[i] );
                start_x += painter.fontMetrics().horizontalAdvance( block[i] );
            }

            if ( cur >= block.length() )
                break; // we're done here

            // Start the next line
            start_y += painter.fontMetrics().height();
            cur++;
            linewidth = 0;
            linestart = cur;
            continue;
        }

        // We're calculating line width here, so check for any font change events
        QMap< unsigned int, int >::const_iterator fontchange = m_lyricBlocks[blockid].fonts.find( cur );

        if ( fontchange != m_lyricBlocks[blockid].fonts.end() )
        {
            curFont.setPointSize( curFont.pointSize() + fontchange.value() );
            metrics = QFontMetrics( curFont );
        }

        linewidth += metrics.horizontalAdvance( block[cur] );
        cur++;
    }
}

void TextRenderer::drawPreamble()
{
    // Is there anything to draw?
    if ( m_preambleTimeLeft <= PREAMBLE_SQUARE + 50 )
        return;

    int cutoff_time = m_preambleTimeLeft - PREAMBLE_SQUARE - 50;

    int preamble_spacing = m_image.width() / 100;
    int preamble_width = (m_image.width() - preamble_spacing * m_preambleCount ) / m_preambleCount;

    QPainter painter( &m_image );
    painter.setPen( Qt::black );
    painter.setBrush( m_colorTitle );

    if ( m_cdgMode )
        painter.setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing, false );

    // Draw a square for each PREAMBLE_SQUARE; we do not draw anything for the last one, and speed up it 0.15sec
    for ( int i = 0; i < (int) m_preambleCount; i++ )
    {
        if ( i * PREAMBLE_SQUARE > cutoff_time )
            continue;

        painter.drawRect( preamble_spacing + i * (preamble_spacing + preamble_width),
                          preamble_spacing,
                          preamble_width,
                          m_preambleHeight );
    }

    m_lastDrawnPreamble = m_preambleTimeLeft;
}


void TextRenderer::drawBackground( qint64 timing )
{
	// Fill the image background
	m_image.fill( m_colorBackground.rgb() );

	if ( !m_lyricEvents.isEmpty() )
		m_lyricEvents.draw( timing, m_image );
}

int TextRenderer::update( qint64 timing )
{
	int result = UPDATE_COLORCHANGE;
	bool redrawPreamble = false;
	int sungpos = 0;

	// Should we show the title?
	int blockid = lyricForTime( timing, &sungpos );
/*
	if ( blockid != -1 )
	{
		qDebug("Time %d: block %d: (%d-%d), pos %d\n", (int) timing, blockid, (int) m_lyricBlocks[blockid].timestart,
			   (int) m_lyricBlocks[blockid].timeend, sungpos );

		QString outbuf;

		for ( int i = 0; i < m_lyricBlocks[blockid].text.length(); i++ )
		{
			if ( sungpos != -1 && i == sungpos )
				outbuf += '|';

			QMap< unsigned int, int >::const_iterator fontchange = m_lyricBlocks[blockid].fonts.find( i );

			if ( fontchange != m_lyricBlocks[blockid].fonts.end() )
				outbuf += QString("[FONT:%1]").arg( fontchange.value() );

			QMap< unsigned int, QString >::const_iterator colchange = m_lyricBlocks[blockid].colors.find( i );

			if ( colchange != m_lyricBlocks[blockid].colors.end() )
				outbuf += QString("[COLOR:%1]").arg( colchange.value() );

			outbuf.push_back( m_lyricBlocks[blockid].text[i] );
		}

		qDebug("%s", qPrintable(outbuf));
	}
	else
		qDebug("Time %d: no block!", (int) timing );
*/
	// Force the full redraw if time went backward
	if ( timing < m_lastSungTime )
		m_forceRedraw = true;

	// Is it time to redraw the preamble?
	if ( m_drawPreamble && abs( m_lastDrawnPreamble - m_preambleTimeLeft ) > 450 )
		redrawPreamble = true;

	// Check whether we can skip the redraws
	bool background_updated = (m_lyricEvents.isEmpty() || !m_lyricEvents.updated( timing )) ? false : true;

	if ( !m_forceRedraw && !redrawPreamble && !background_updated )
	{
		// Did lyrics change at all?
		if ( blockid == m_lastBlockPlayed && sungpos == m_lastPosition )
			return UPDATE_NOCHANGE;

		// If the new lyrics is empty, but we just finished playing something, keep it for 5 more seconds
		// (i.e. post-delay)
		if ( blockid == -1 && timing - m_lastSungTime < 5000 )
			return UPDATE_NOCHANGE;
	}

	// Draw the background first
	drawBackground( timing );

	// Did we get lyrics?
	if ( blockid != -1 )
	{
		// Do the new lyrics fit into the image without resizing?
		QRect imgrect = boundingRect( blockid, m_renderFont );

		if ( imgrect.width() > m_image.width() || imgrect.height() > m_image.height() )
		{
			QSize newsize = QSize( qMax( imgrect.width() + 10, m_image.width() ),
								   qMax( imgrect.height() + 10, m_image.height() ) );
			m_image = QImage( newsize, QImage::Format_ARGB32 );
			result = UPDATE_RESIZED;

			// Draw the background again on the resized image
			drawBackground( timing );
		}

		// Draw the lyrics
		drawLyrics( blockid, sungpos, imgrect );

		// Draw the preamble if needed
		if ( m_drawPreamble )
			drawPreamble();
	}

	// Is the text change significant enough to warrant full screen redraw?
	if ( blockid != m_lastBlockPlayed || m_forceRedraw )
	{
		if ( result != UPDATE_RESIZED )
			result = UPDATE_FULL;
	}

	//saveImage();

	m_lastBlockPlayed = blockid;
	m_lastPosition = sungpos;

	m_forceRedraw = false;
	return result;
}


void TextRenderer::prepareEvents()
{
	m_lyricEvents.prepare();

	// Events need to be adjusted for the following:
	// - Those at the start of the block may move;
	qint64 lastend = 0;

	for ( int i = 0; i < m_lyricBlocks.size(); i++ )
	{
		qint64 start = m_lyricBlocks[i].timestart;
		qint64 end = m_lyricBlocks[i].timeend;

		// Should the next block be shown earlier than expected via prefetch?
		if ( start - lastend > m_beforeDuration )
		{
			// Adjust by m_beforeDuration
			m_lyricEvents.adjustTime( start, start - m_beforeDuration );
		}
		else if ( m_prefetchDuration > 0 )
		{
			// Adjust by m_prefetchDuration
			m_lyricEvents.adjustTime( start, start - m_prefetchDuration );
		}
		else if ( start > lastend )
		{
			// Adjust by start - lastend
			m_lyricEvents.adjustTime( start, start - (start - lastend) );
		}

		lastend = end;
	}
}
