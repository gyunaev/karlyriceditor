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

#include <QPainter>
#include <QMessageBox>

#include "textrenderer.h"
#include "settings.h"
#include "project.h"


// active - not sang, inactive - sang

// Formatting characters
static const QChar actionChar = QChar( 0x2016 );
static const QChar actionColorTitle = 'T';
static const QChar actionColorToSing = 'A';
static const QChar actionColorSang = 'I';
static const QChar actionSmallFont = 'S';

// Convenience strings
static const QString actionSetColorTitle = QString(actionChar) + actionColorTitle;
static const QString actionSetColorToSing  = QString(actionChar) + actionColorToSing;
static const QString actionSetColorSang  = QString(actionChar) + actionColorSang;
static const QString actionSetSmallFont = QString(actionChar) + actionSmallFont;

// Some preamble constants
static const int PREAMBLE_SQUARE = 500; // 500ms for each square
static const int PREAMBLE_MIN_PAUSE = 5000; // 5000ms minimum pause between verses for the preamble to appear
static const int LYRICS_SHOW_ADVANCE = 5000; // Show the lyrics at least 5 seconds in advance


TextRenderer::TextRenderer( int width, int height )
	: LyricsRenderer()
{
	m_videoDecoder = 0;
	m_image = QImage( width, height, QImage::Format_ARGB32 );
	init();
}

void TextRenderer::setData( const Lyrics& lyrics )
{
	init();

	m_lyrics = lyrics;
}

void TextRenderer::setRenderFont( const QFont& font )
{
	m_renderFont = font;
	m_smallFont = font;
	m_smallFont.setPixelSize( font.pointSize() - 2 );

	m_forceRedraw = true;
}

void TextRenderer::setRenderSmallFont( const QFont& font )
{
	m_smallFont = font;
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

void TextRenderer::setTitlePageData( const QString& artist, const QString& title, unsigned int msec )
{
	m_titleArtist = artist;
	m_titleSong = title;
	m_requestedTitleDuration = msec;

	m_forceRedraw = true;
}

bool TextRenderer::setVideoFile( const QString& filename )
{
	delete m_videoDecoder;
	m_videoDecoder = new FFMpegVideoDecoder();

	if ( !m_videoDecoder->openFile( filename ) )
	{
		QMessageBox::critical( 0, "Invalid video file",
							   QString("Cannot open video file %1: %2") .arg( filename) .arg( m_videoDecoder->errorMsg() ) );
		delete m_videoDecoder;
		return false;
	}

	return true;
}

void TextRenderer::init()
{
	m_forceRedraw = true;

	m_colorBackground = pSettings->m_previewBackground;
	m_colorTitle = QColor( Qt::white );
	m_colorToSing = pSettings->m_previewTextActive;
	m_colorSang = pSettings->m_previewTextInactive;
	setRenderFont( QFont( pSettings->m_previewFontFamily, pSettings->m_previewFontSize ) );

	m_requestedTitleDuration = 0;
	m_preambleHeight = 0;
	m_preambleLengthMs = 0;
	m_preambleCount = 0;

	m_preambleTimeLeft = 0;
	m_lastDrawnPreamble = 0;
	m_lastSungTime = 0;
	m_drawPreamble = false;

	m_lastLyricsText.clear();
}

void TextRenderer::setPreambleData( unsigned int height, unsigned int timems, unsigned int count )
{
	m_preambleHeight = height;
	m_preambleLengthMs = timems;
	m_preambleCount = count;

	m_forceRedraw = true;
}

void TextRenderer::setCDGfonts( const Project * prj )
{
	// Disable anti-aliasing for fonts
	QFont renderFont = QFont( prj->tag( Project::Tag_CDG_font ), prj->tag( Project::Tag_CDG_fontsize ).toInt() );
	renderFont.setStyleStrategy( QFont::NoAntialias );
	//renderFont.setWeight( QFont::Bold );
	setRenderFont( renderFont );

	QFont smallFont = QFont( prj->tag( Project::Tag_CDG_font ), prj->tag( Project::Tag_CDG_fontsize ).toInt() - 2 );
	smallFont.setStyleStrategy( QFont::NoAntialias );
	setRenderSmallFont( smallFont );

	m_forceRedraw = true;
}

static inline QString stripActionSequences( const QString& line )
{
	QString stripped;
	stripped.reserve( line.length() );

	for ( int ch = 0; ch < line.length(); ch++ )
	{
		if ( line[ch] == actionChar )
		{
			ch++;

			// If next char is actionChar, allow it alone - it is escape
			if ( ch >= line.length() || line[ch] != actionChar )
				continue;
		}

		stripped.append( line[ch] );
	}

	return stripped;
}

static inline void fixActionSequences( QString& block )
{
	if ( block.startsWith( "@@" ) )
	{
		block.remove( "@@" );
		block = QString(actionChar) + "T" + block;
	}
}

QString TextRenderer::lyricForTime( qint64 tickmark )
{
	QString block;
	int pos;
	qint64 nexttime;

	// If there is a block within next one second, show it.
	if ( m_lyrics.nextBlock( tickmark, nexttime, block ) && nexttime - tickmark <= 1000 )
	{
		// We update the timing, but the preamble show/noshow does not change here
		m_preambleTimeLeft = qMax( 0, (int) (nexttime - tickmark) );

		fixActionSequences( block );
		return block;
	}

	if ( !m_lyrics.blockForTime( tickmark, block, pos, nexttime ) )
	{
		// Nothing active to show, so if there is a block within next five seconds, show it.
		if ( m_lyrics.nextBlock( tickmark, nexttime, block ) && nexttime - tickmark <= LYRICS_SHOW_ADVANCE )
		{
			m_preambleTimeLeft = qMax( 0, (int) (nexttime - tickmark) );

			// We show preamble if there was silence over PREAMBLE_MIN_PAUSE and the block
			// does not contain any title stuff
			if ( tickmark - m_lastSungTime > PREAMBLE_MIN_PAUSE && !block.contains( "@@" ) )
			{
				// m_preambleHeight == 0 disables the preamble
				if ( m_preambleHeight > 0 )
					m_drawPreamble = true;
			}

			fixActionSequences( block );
			return block;
		}

		// No next block or too far away yet
		m_drawPreamble = false;
		return QString();
	}

	m_drawPreamble = false;
	int endidx;

	if ( block.startsWith("@@") && (endidx = block.indexOf( "@@", 2 )) != -1 )
	{
		// The whole part between @@...@@ is considered title, and not affected by pos.
		QString title = block.mid( 2, endidx - 2 );

		if ( pos < endidx + 2 )
			pos = endidx + 2;

		// At least part of the block is outside the title tag
		QString inactive = block.mid( endidx + 2, pos - (endidx + 2) );
		QString active = block.mid( pos );

		block = QString(actionChar) + "T" + title + QString(actionChar) + "I" + inactive + actionChar + "A" + active;
	}
	else
	{
		QString inactive = block.left( pos );
		QString active = block.mid( pos );

		block = QString(actionChar) + "I" + inactive + actionChar + "A" + active;

		// Should be unaffected by titles
		m_lastSungTime = tickmark;
	}

	return block;
}

QRect TextRenderer::boundingRect( const QString& text )
{
	return boundingRect( text, m_renderFont, m_smallFont );
}

QRect TextRenderer::boundingRect( const QString& text, const QFont& font, const QFont& smallfont )
{
	QStringList lines = text.split( "\n" );

	// Calculate the height
	QFontMetrics m = QFontMetrics( font );
	int maxheight = m.height() * lines.size();

	// Calculate the width for every line
	int maxwidth = 0;

	for ( int i = 0; i < lines.size(); i++ )
	{
		const QString& line = lines[i];

		// Calculate the line width first
		int width = 0;

		// We cannot use stripActionSequences as we need to account for smaller font
		for ( int ch = 0; ch < line.length(); ch++ )
		{
			if ( line[ch] == actionChar )
			{
				ch++; // skip formatting char

				if ( line[ch] == actionSmallFont )
					m = QFontMetrics( smallfont );

				if ( line[ch] != actionChar )
					continue; // allow unescape
			}

			width += m.width( line[ch] );
		}

		maxwidth = qMax( maxwidth, width );
	}

	return QRect( 0, 0, maxwidth, maxheight );
}


void TextRenderer::drawLyrics( const QString& paragraph, const QRect& boundingRect )
{
	QStringList lines = paragraph.split( "\n" );

	// Prepare the painter
	QPainter painter( &m_image );
	painter.setPen( m_colorToSing );
	painter.setFont( m_renderFont );

	QFontMetrics m = QFontMetrics( m_renderFont );

	// Get the height offset from the rect
	int start_y = (m_image.height() - boundingRect.height()) / 2 + m.height();

	// Now draw it, line by line
	for ( int i = 0; i < lines.size(); i++ )
	{
		QString& line = lines[i];

		// Calculate the line width first, char by char
		// We cannot use stripActionSequences as we need to account for font size changes inside the text
		int width = 0;
		for ( int ch = 0; ch < line.length(); ch++ )
		{
			if ( line[ch] == actionChar )
			{
				ch++; // skip color

				if ( ch >= line.length() )
				{
					qWarning("Unterminated escape sequence at the end of line found");
					continue;
				}

				// Account for smaller font
				if ( line[ch] == actionSmallFont )
					m = QFontMetrics( m_smallFont );

				if ( line[ch] != actionChar )
					continue; // allow @@ as unescape
			}

			width += m.width( line[ch] );
		}

		// Now we know the width, calculate the start offset
		int start_x = (m_image.width() - width) / 2;

		// And draw the line
		for ( int ch = 0; ch < line.length(); ch++ )
		{
			if ( line[ch] == actionChar )
			{
				ch++;

				if ( line[ch] == actionColorToSing )
					painter.setPen( m_colorToSing );
				else if ( line[ch] == actionColorSang )
					painter.setPen( m_colorSang );
				else if ( line[ch] == actionColorTitle )
					painter.setPen( m_colorTitle );
				else if ( line[ch] == actionSmallFont )
					painter.setFont( m_smallFont );
				else
					abort();

				continue;
			}

			// Outline
			const int OL = 1;
			painter.save();
			painter.setPen( Qt::black );
			painter.drawText( start_x - OL, start_y - OL, (QString) line[ch] );
			painter.drawText( start_x + OL, start_y - OL, (QString) line[ch] );
			painter.drawText( start_x - OL, start_y + OL, (QString) line[ch] );
			painter.drawText( start_x + OL, start_y + OL, (QString) line[ch] );
			painter.restore();

			painter.drawText( start_x, start_y, (QString) line[ch] );
			start_x += painter.fontMetrics().width( line[ch] );
		}

		start_y += painter.fontMetrics().height();
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
	painter.setPen( m_colorTitle );
	painter.setBrush( m_colorTitle );

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

	if ( m_videoDecoder )
	{
		if ( timing > m_videoDecoder->getVideoLengthMs() )
			timing %= m_videoDecoder->getVideoLengthMs();

		m_videoDecoder->seekMs( (int) timing );

		QImage videoframe;
		m_videoDecoder->getFrame( videoframe );

		QPainter p( &m_image );
		p.drawImage( m_image.rect(), videoframe, videoframe.rect() );
	}
}

int TextRenderer::update( qint64 timing )
{
	int result = UPDATE_COLORCHANGE;
	bool redrawPreamble = false;
	QString lyricstext;

	// Should we show the title?
	if ( m_requestedTitleDuration > 0
	&& timing < m_requestedTitleDuration
	&& timing < (m_lyrics.block(0).first().first().timing - 1000) )
	{
		lyricstext = QString("%1%2\n\n%3\n\n%4Created by Karaoke Lyric Editor\n%5http://www.karlyriceditor.com/\n")
						.arg( actionSetColorTitle )
						.arg( m_titleArtist )
						.arg( m_titleSong )
						.arg( actionSetSmallFont )
						.arg( actionSetColorToSing );
	}
	else
	{
		lyricstext = lyricForTime( timing );
	}

	// Force the full redraw if time went backward
	if ( timing < m_lastSungTime )
		m_forceRedraw = true;

	// Is it time to redraw the preamble?
	if ( m_drawPreamble && abs( m_lastDrawnPreamble - m_preambleTimeLeft ) > 450 )
		redrawPreamble = true;

	// Check whether we can skip the redraws
	if ( !m_forceRedraw && !redrawPreamble && !m_videoDecoder )
	{
		// Did lyrics change at all?
		if ( lyricstext == m_lastLyricsText )
			return UPDATE_NOCHANGE;

		// If the new lyrics is empty, but we just finished playing something, keep it for 5 more seconds
		// (i.e. post-delay)
		if ( lyricstext.isEmpty() && timing - m_lastSungTime < 5000 )
			return UPDATE_NOCHANGE;
	}

	// Do the new lyrics fit into the image without resizing?
	QRect imgrect = boundingRect( lyricstext, m_renderFont, m_smallFont );

	if ( imgrect.width() > m_image.width() || imgrect.height() > m_image.height() )
	{
		QSize newsize = QSize( qMax( imgrect.width() + 10, m_image.width() ),
							   qMax( imgrect.height() + 10, m_image.height() ) );
		m_image = QImage( newsize, QImage::Format_ARGB32 );
		result = UPDATE_RESIZED;
	}

	// Draw the background first
	drawBackground( timing );

	// Draw the lyrics
	drawLyrics( lyricstext, imgrect );

	// Draw the preamble if needed
	if ( m_drawPreamble )
		drawPreamble();

	// Is the text change significant enough to warrant full screen redraw?
	if ( stripActionSequences( lyricstext ) != stripActionSequences( m_lastLyricsText ) || m_forceRedraw )
	{
		if ( result != UPDATE_RESIZED )
			result = UPDATE_FULL;
	}

	//saveImage();

	m_lastLyricsText = lyricstext;
	m_forceRedraw = false;
	return result;
}
