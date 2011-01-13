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

#include <QMap>
#include <QList>
#include <QApplication>
#include <QPainter>
#include <QProgressDialog>
#include <QTextDocument>

#include "cdggenerator.h"
#include "version.h"

// Formatting characters
static const QChar actionChar = QChar( 0x2016 );
static const QChar actionColorTitle = 'T';
static const QChar actionColorActive = 'A';
static const QChar actionColorInactive = 'I';
static const QChar actionSmallFont = 'S';

// Convenience strings
static const QString actionSetColorTitle = QString(actionChar) + actionColorTitle;
static const QString actionSetColorActive  = QString(actionChar) + actionColorActive;
static const QString actionSetColorInactive  = QString(actionChar) + actionColorInactive;
static const QString actionSetSmallFont = QString(actionChar) + actionSmallFont;


// Color code indexes
static int COLOR_IDX_BACKGROUND = 0;	// background

CDGGenerator::CDGGenerator( const Project * proj )
	: m_project( proj )
{
}

void CDGGenerator::init()
{
	// Disable anti-aliasing for fonts
	m_renderFont = QFont( m_project->tag( Project::Tag_CDG_font ), m_project->tag( Project::Tag_CDG_fontsize ).toInt() );
	m_renderFont.setStyleStrategy( QFont::NoAntialias );
	m_renderFont.setWeight( QFont::Bold );

	m_smallFont = QFont( m_project->tag( Project::Tag_CDG_font ), m_project->tag( Project::Tag_CDG_fontsize ).toInt() - 2 );
	m_smallFont.setStyleStrategy( QFont::NoAntialias );

	// Initialize colors m_project
	m_colorBackground = m_project->tag( Project::Tag_CDG_bgcolor );
	m_colorInfo = m_project->tag( Project::Tag_CDG_infocolor );
	m_colorInactive = m_project->tag( Project::Tag_CDG_inactivecolor );
	m_colorActive = m_project->tag( Project::Tag_CDG_activecolor );

	initColors();

	// Initialize the stream
	m_stream.clear();
	addEmpty();

	clearScreen();
}

void CDGGenerator::initColors()
{
	m_colors.clear();
	m_colors.push_back( m_colorBackground );
	m_streamColorIndex = -1;
}

void CDGGenerator::addSubcode( const SubCode& sc )
{
	m_stream.push_back( sc );
}

void CDGGenerator::addEmpty()
{
	SubCode sc;
	memset( &sc, 0, sizeof( sc ) );

	addSubcode( sc );
}

void CDGGenerator::fillColor( char * buffer, const QColor& color )
{
	// Green
	char red = (color.red() / 17) & 0x0F;
	char green = (color.green() / 17) & 0x0F;
	char blue = (color.blue() / 17) & 0x0F;

	// Red and green
	buffer[0] = (red << 2) | (green >> 2);
	buffer[1] = ((green & 0x03) << 5 ) | blue;
}

void CDGGenerator::clearScreen()
{
	SubCode sc;

	// First copy the colors if we got them (there is always one background color)
	if ( m_streamColorIndex != -1 && m_colors.size() > 1 )
	{
		// Load first lower 8 colors
		SubCode sc;

		sc.command = CDG_COMMAND;
		sc.instruction = CDG_INST_LOAD_COL_TBL_0_7;
		memset( sc.data, 0, 16 );

		for ( int i = 0; i < 7; i++ )
		{
			if ( i >= m_colors.size() )
				break;

			fillColor( sc.data + i * 2, m_colors[i] );
		}

		m_stream[ m_streamColorIndex ] = sc;

		// Do we have more colors?
		if ( m_colors.size() > 8 )
		{
			sc.instruction = CDG_INST_LOAD_COL_TBL_8_15;
			memset( sc.data, 0, 16 );

			for ( int i = 8; i < 16; i++ )
			{
				if ( i >= m_colors.size() )
					break;

				fillColor( sc.data + i * 2, m_colors[i] );
			}

			m_stream[ m_streamColorIndex + 1 ] = sc;
		}

		initColors();
	}

	// Now clear the screen
	for ( int i = 0; i < 16; i++ )
	{
		sc.command = CDG_COMMAND;
		sc.instruction = CDG_INST_MEMORY_PRESET;
		CDG_MemPreset* preset = (CDG_MemPreset*) sc.data;

		preset->repeat = i & 0x0F;
		preset->color = COLOR_IDX_BACKGROUND;

		addSubcode( sc );
	}

	sc.command = CDG_COMMAND;
	sc.instruction = CDG_INST_BORDER_PRESET;
	CDG_BorderPreset* preset = (CDG_BorderPreset*) sc.data;

	preset->color = COLOR_IDX_BACKGROUND;

	addSubcode( sc );

	// Reserve space for two LoadColor commands
	m_streamColorIndex = m_stream.size();
	addEmpty();
	addEmpty();
}

static inline QString stripColors( const QString& str )
{
	QRegExp rx( QString("%1.").arg( actionChar) );
	rx.setMinimal( true );

	QString stripped = str;
	stripped.remove( rx );
	return stripped;
}

QByteArray CDGGenerator::stream()
{
	return QByteArray( (const char*) m_stream.data(), m_stream.size() * sizeof( SubCode ) );
}

int CDGGenerator::getColor( QRgb color )
{
	// Mask alpha channel
	color |= 0xFF000000;

	// Find in the array of colors
	for ( int i = 0; i < m_colors.size(); i++ )
		if ( m_colors[i] == QColor(color) )
			return i;

	if ( m_colors.size() == 16 )
		qFatal("Color table is out of color entries");

	qDebug("Adding color %08X", color );
	m_colors.push_back( color );
	return m_colors.size() - 1;
}

void CDGGenerator::checkTile( int offset_x, int offset_y, const QImage& orig,const QImage& newimg )
{
	// The first loop checks if there are any colors to change, and enumerates them
	QMap< int, QList<int> > color_changes;

	// Tiles are 6x12
	for ( int y = 0; y < 12; y++ )
	{
		const QRgb * orig_line = (const QRgb *) orig.scanLine( y + offset_y );
		const QRgb * new_line = (const QRgb *) newimg.scanLine( y + offset_y );

		for ( int x = 0; x < 6; x++ )
		{
			int origcolor = getColor( orig_line[ offset_x + x ] );
			int newcolor = getColor( new_line[ offset_x + x ] );

			if ( origcolor == newcolor )
				continue;

			// Calculate the mask for the color change
			int mask = origcolor ^ newcolor;

			if ( (mask & 0xFFFFFFF0) != 0 )
				qFatal("error in mask calculation");

			// Store the coordinates in lo/hi bytes
			int coord = x << 8 | y;
			color_changes[ mask ].push_back( coord );
		}
	}

	// Anything to change?
	if ( color_changes.isEmpty() )
		return;

	// Enumerate the map entries
	const QList<int>& colors = color_changes.keys();

	// Bitmasks
	quint8 bitmask[6] = { 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

	for ( int i = 0; i < colors.size(); i++ )
	{
		SubCode sc;

		sc.command = CDG_COMMAND;
		sc.instruction = CDG_INST_TILE_BLOCK_XOR;
		memset( sc.data, 0, 16 );
		CDG_Tile* tile = (CDG_Tile*) sc.data;

		tile->column = offset_x / 6;
		tile->row = offset_y / 12;
		tile->color0 = 0;
		tile->color1 = colors[ i ];

		const QList<int>& coords = color_changes[ colors[ i ] ];

		for ( int p = 0; p < coords.size(); p++ )
		{
			int x = coords[p] >> 8;
			int y = coords[p] & 0x0F;

			tile->tilePixels[y] |= bitmask[x];
		}

		addSubcode( sc );
	}
}

void CDGGenerator::applyTileChanges( const QImage& orig,const QImage& newimg )
{
/*
	static unsigned int i = 0;
	QString fname = QString("image-%1.bmp") .arg(i);

	qDebug("generating image %d", i );

	if ( ++i > 9 )
		i = 0;

	orig.save( "orig-" + fname, "bmp" );
	newimg.save( "new-" + fname, "bmp" );
*/
	// Tiles are 6x12
	for ( unsigned int offset_y = 0; offset_y < CDG_FULL_HEIGHT; offset_y += 12 )
		for ( unsigned int offset_x = 0; offset_x < CDG_FULL_WIDTH; offset_x += 6 )
			checkTile( offset_x, offset_y, orig, newimg );
}

bool CDGGenerator::validateParagraph( const QString& paragraph, QList<ValidatorError>& errors )
{
	QImage image( CDG_FULL_WIDTH, CDG_FULL_HEIGHT, QImage::Format_RGB32 );

	return drawText( image, paragraph, &errors );
}


bool CDGGenerator::drawText( QImage& image, const QString& paragraph, QList<ValidatorError> * errors )
{
	// Some params
	const int min_x = CDG_BORDER_WIDTH;
	const int max_x = CDG_FULL_WIDTH - CDG_BORDER_WIDTH;
	const int min_y = CDG_BORDER_HEIGHT;
	const int max_y = CDG_FULL_HEIGHT - CDG_BORDER_HEIGHT;
	const int min_spacing = 0;

	// Set up painter with disabled anti-aliasing to handle color detection
	image.fill( m_colorBackground.rgb() );

	QPainter painter( &image );
	painter.setPen( m_colorActive );
	painter.setFont( m_renderFont );

	QStringList lines = paragraph.split( "\n" );

	// Can we fit into the boundaries?
	QFontMetrics m = painter.fontMetrics();
	int height = m.height() * lines.size() + min_spacing * (lines.size() - 1);

	if ( height > max_y - min_y )
	{
		if ( errors )
		{
			errors->push_back(
					ValidatorError(
							0,
							0,
							QObject::tr( "This paragraph contains too many lines, "
								"and the result height of %1 cannot fit into a CD+G screen using the font selected.").arg( height) ) );
		}
		else
		{
			qWarning("Paragraph height %d (%d lines) exceeds the allowed width %d, font height %d",
							 height + min_y, lines.size(), max_y, m.height() );
			return false;
		}
	}

	// Calculate start and increment y
	int start_y = min_y + ( max_y - min_y - height) / 2 + m.height();
	int step_y = m.height() + min_spacing;

	// Draw it, line by line
	for ( int i = 0; i < lines.size(); i++ )
	{
		QString& line = lines[i];
		int width = 0;

		// Calculate the line width first
		for ( int ch = 0; ch < line.length(); ch++ )
		{
			if ( line[ch] == actionChar )
			{
				ch++; // skip color

				if ( line[ch] == actionSmallFont )
				{
					painter.setFont( m_smallFont );
					m = painter.fontMetrics();
				}

				continue;
			}

			width += m.width( line[ch] );
		}

		if ( width > max_x - min_x )
		{
			if ( errors )
			{
				errors->push_back(
						ValidatorError(
								0,
								0,
								QObject::tr( "Line too long. The line width %1 cannot fit into a "
									"CD+G screen using the font selected.").arg( width) ) );
			}
			else
			{
				qWarning("Line '%s' width %d exceeds the allowed width %d", qPrintable(stripColors(line)), width + min_x, max_x );
				return false;
			}
		}

		// If we're in a checking mode, continue
		if ( errors )
			continue;

		// Now we know the width, calculate start
		int start_x = min_x + ( max_x - min_x - width) / 2;

		// Draw the line
		for ( int ch = 0; ch < line.length(); ch++ )
		{
			if ( line[ch] == actionChar )
			{
				ch++;

				if ( line[ch] == actionColorActive )
					painter.setPen( m_colorActive );
				else if ( line[ch] == actionColorInactive )
					painter.setPen( m_colorInactive );
				else if ( line[ch] == actionColorTitle )
					painter.setPen( m_colorInfo );
				else if ( line[ch] == actionSmallFont )
					painter.setFont( m_smallFont );
				else
					abort();

				continue;
			}

			painter.drawText( start_x, start_y, (QString) line[ch] );
			start_x += m.width( line[ch] );
		}

		start_y += step_y;
	}

	return true;
}

QString CDGGenerator::lyricForTime( const Lyrics& lyrics, qint64 tickmark )
{
	QString block;
	int pos;
	qint64 nexttime;

	// If there is a block within next one second, show it.
	if ( lyrics.nextBlock( tickmark, nexttime, block ) && nexttime - tickmark <= 1000 )
		return block;

	if ( !lyrics.blockForTime( tickmark, block, pos, nexttime ) )
	{
		// Nothing active to show, so if there is a block within next five seconds, show it.
		if ( lyrics.nextBlock( tickmark, nexttime, block ) && nexttime - tickmark <= 5000 )
		{
			return block;
		}

		return QString();
	}

	QString inactive = block.left( pos );
	QString active = block.mid( pos );

	block = QString(actionChar) + "I" + inactive + actionChar + "A" + active;

	return block;
}

void CDGGenerator::generate( const Lyrics& lyrics, qint64 total_length )
{
	QString	lastLyrics;

	// Prepare images
	QImage image( CDG_FULL_WIDTH, CDG_FULL_HEIGHT, QImage::Format_RGB32 );
	QImage lastImage( CDG_FULL_WIDTH, CDG_FULL_HEIGHT, QImage::Format_RGB32 );

	image.fill( m_colorBackground.rgb() );
	lastImage.fill( m_colorBackground.rgb() );

	// Pop up progress dialog
	QProgressDialog dlg ("Rendering CD+G lyrics",
						 QString::null,
						 0,
						 99 );

	dlg.setMinimumDuration( 2000 );
	dlg.setValue( 0 );

	qint64 dialog_step = total_length / 100;

	// Preallocate the array
	m_stream.clear();
	m_stream.reserve( total_length * 300 / 1000 );

	// Render
	while ( 1 )
	{
		// There should be 300 packets in 1000ms of music
		// So each packet adds 1000 / 300 ms
		// Speed up time a little to compensate CD+G reader delay
		qint64 timing = m_stream.size() * 1000 / 300 + 250;

		// Should we show the next step?
		if ( timing / dialog_step > dlg.value() )
		{
			dlg.setValue( timing / dialog_step );
			qApp->processEvents( QEventLoop::ExcludeUserInputEvents );
		}

		if ( timing > total_length )
			return;

//		qDebug("timing: %d packets, %dms (%d sec)", m_stream.size(), (int) timing, (int) (timing / 1000) );

		QString lyricpaga;

		// Show title?
		if ( timing < m_project->tag( Project::Tag_CDG_titletime ).toInt() * 1000 )
		{
			lyricpaga = QString("%1%2\n\n%3\n\n%4Created by Karaoke Lyric Editor %5.%6\n%7http://www.karlyriceditor.com/\n")
							.arg( actionSetColorTitle )
							.arg( m_project->tag( Project::Tag_Artist ) )
							.arg( m_project->tag( Project::Tag_Title ) )
							.arg( actionSetSmallFont )
							.arg( APP_VERSION_MAJOR )
							.arg( APP_VERSION_MINOR )
							.arg( actionSetColorActive );
		}
		else
		{
			lyricpaga = lyricForTime( lyrics, timing );
		}

		// Did lyrics change at all?
		if ( lyricpaga == lastLyrics )
		{
			addEmpty();
			continue;
		}

		// If lyrics cleared up but we just finished showing something, keep it for 5 more seconds
		if ( lyricpaga.isEmpty() && time(0) - m_lastPlayed < 5 )
		{
			addEmpty();
			continue;
		}

		// Render the lyrics
		drawText( image, lyricpaga );

		// Is change significant enough to warrant full redraw?
		if ( stripColors( lyricpaga ) != stripColors( lastLyrics ) )
		{
			clearScreen();

			// Clear the old image too
			lastImage.fill( m_colorBackground.rgb() );
		}

		applyTileChanges( lastImage, image );
		lastImage = image;
		lastLyrics = lyricpaga;
	}
}
