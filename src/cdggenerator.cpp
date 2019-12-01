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

#include <QFile>
#include <QDebug>
#include <QPainter>
#include <QMessageBox>
#include <QApplication>

#include <math.h>

#include "editor.h"
#include "cdggenerator.h"
#include "dialog_export_params.h"
#include "ui_dialog_encodingprogress.h"


// Color code indexes
static int COLOR_IDX_BACKGROUND = 0;	// background

CDGGenerator::CDGGenerator( Project * proj )
{
    m_project = proj;
    m_enableAntiAlias = true;
}

void CDGGenerator::init()
{
	// Initialize colors from m_project
	m_colorBackground = m_project->tag( Project::Tag_CDG_bgcolor, "black" );
	m_colorInfo = m_project->tag( Project::Tag_CDG_infocolor, "white" );
	m_colorInactive = m_project->tag( Project::Tag_CDG_inactivecolor, "blue" );
	m_colorActive = m_project->tag( Project::Tag_CDG_activecolor, "green" );

	initColors();

	// Initialize the stream
	m_stream.clear();
	addEmpty();

	clearScreen();
}

void CDGGenerator::addColorGradations( const QColor& color, unsigned int number )
{
	int r = color.red();
	int g = color.green();
	int b = color.blue();

	int r_step = r / number;
	int g_step = g / number;
	int b_step = b / number;

	for ( unsigned int i = 0; i < number; i++ )
	{
		QColor newcolor( r - r_step * i, g - g_step * i, b - b_step * i );
		m_colors.push_back( newcolor );
	}
}

int CDGGenerator::getColor( QRgb rgbcolor )
{
	// See http://stackoverflow.com/questions/4057475/rounding-colour-values-to-the-nearest-of-a-small-set-of-colours
	QColor targetcolor( rgbcolor );

	int smallest_dist_idx = -1;
	double smallest_dist = 0.0;

	// Calculate the smallest color distance between this color and other colors in the array
	for ( int i = 0; i < m_colors.size(); i++ )
	{
		const QColor& origcolor = m_colors.at( i );

		double dist = sqrt( pow( origcolor.redF() - targetcolor.redF(), 2.0)
							+ pow( origcolor.greenF() - targetcolor.greenF(), 2.0 )
							+ pow( origcolor.blueF() - targetcolor.blueF(), 2.0 ) );

		if ( smallest_dist_idx == -1 || dist < smallest_dist )
		{
			smallest_dist_idx = i;
			smallest_dist = dist;
		}
	}

    // If we have room in the color table and the distance is too big, add it
    if ( m_colors.size() < 16 && smallest_dist > 1.0  )
    {
        m_colors.push_back( targetcolor );
        smallest_dist_idx = m_colors.size() - 1;
    }

	return smallest_dist_idx;
}

void CDGGenerator::initColors()
{
	// Initialize the color map with the following:
	// - 1 entry for the background color
	// - 5 entries for the init color
	// - 5 entries for the sung color
	// - 5 entries for the unsung color
	m_colors.clear();
	m_colors.push_back( m_colorBackground );

    // We can't have more than two gradations here, CD+G format has too limited throughput
    if ( m_enableAntiAlias )
    {
        addColorGradations( m_colorInfo, 2 );
        addColorGradations( m_colorInactive, 2 );
        addColorGradations( m_colorActive, 2 );
    }
    else
    {
        // Only add three colors here
        m_colors.push_back( m_colorInfo );
        m_colors.push_back( m_colorInactive );
        m_colors.push_back( m_colorActive );
    }

	m_streamColorIndex = -1;
}

void CDGGenerator::addSubcode( const SubCode& sc )
{
	m_stream.append( sc );
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

		//initColors();
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

QByteArray CDGGenerator::stream()
{
	return QByteArray( (const char*) m_stream.data(), m_stream.size() * sizeof( SubCode ) );
}


void CDGGenerator::checkTile( int offset_x, int offset_y, const QImage& orig,const QImage& newimg )
{
	// The first loop checks if there are any colors to change, and enumerates them
	QMap< int, QList<int> > color_changes;

	// Tiles are 6x12
	for ( int y = 0; y < 12; y++ )
	{
		// Since the offsets assume borders, but our image does not contain them, we
		// adjust in the calculations
		int image_offset_y = y + offset_y - CDG_BORDER_HEIGHT;

		const QRgb * orig_line = (const QRgb *) orig.scanLine( image_offset_y );
		const QRgb * new_line = (const QRgb *) newimg.scanLine( image_offset_y );

		for ( int x = 0; x < 6; x++ )
		{
			int image_offset_x = offset_x + x - CDG_BORDER_WIDTH;

			if ( orig_line[ image_offset_x ] == new_line[ image_offset_x ] )
				continue;

			int origcolor = getColor( orig_line[ image_offset_x ] );
			int newcolor = getColor( new_line[ image_offset_x ] );

			// Calculate the mask for the color change
			int mask = origcolor ^ newcolor;

			if ( (mask & 0xFFFFFF00) != 0 )
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
	QString ofname = QString("/home/tim/1/%1-orig.bmp") .arg(i);
	QString nfname = QString("/home/tim/1/%1-new.bmp") .arg(i);

	qDebug("generating image %d", i );

	if ( ++i > 90 )
		i = 0;

	orig.save( ofname, "bmp" );
	newimg.save( nfname, "bmp" );
*/
	// Tiles are 6x12, but we skip the border area 
	for ( unsigned int offset_y = CDG_BORDER_HEIGHT; offset_y < CDG_FULL_HEIGHT - CDG_BORDER_HEIGHT; offset_y += 12 )
		for ( unsigned int offset_x = CDG_BORDER_WIDTH; offset_x < CDG_FULL_WIDTH - CDG_BORDER_WIDTH; offset_x += 6 )
			checkTile( offset_x, offset_y, orig, newimg );
}

void CDGGenerator::generate( const Lyrics& lyrics, qint64 total_length )
{
	// Show the dialog with video options
	DialogExportOptions dlg( m_project, lyrics, false );

	if ( dlg.exec() != QDialog::Accepted )
		return;

    // Get our parameters
    m_enableAntiAlias = dlg.boxEnableAntialiasing->isChecked();

	// Initialize the buffer and colors
	init();

	// Prepare the renderer
	TextRenderer lyricrenderer( CDG_DRAW_WIDTH, CDG_DRAW_HEIGHT );

    // This must be set before lyrics
    lyricrenderer.setDefaultVerticalAlign( (TextRenderer::VerticalAlignment) m_project->tag( Project::Tag_CDG_TextAlignVertical, QString::number( TextRenderer::VerticalBottom ) ).toInt() );

    // Lyrics must be set before anything else as it overrides the data
    lyricrenderer.setLyrics( lyrics );

    // Title
    lyricrenderer.setTitlePageData( dlg.m_artist,
                                    dlg.m_title,
                                    dlg.m_createdBy,
                                    m_project->tag( Project::Tag_CDG_titletime, "5" ).toInt() * 1000 );

    // Rendering font
    QFont renderFont( m_project->tag(Project::Tag_CDG_font) );
    int fontsize = m_project->tag(Project::Tag_CDG_fontsize).toInt();

    // Is anti-aliasing enabled?
    if ( m_enableAntiAlias )
        renderFont.setStyleStrategy( QFont::PreferAntialias );
    else
        renderFont.setStyleStrategy( QFont::NoAntialias );

    // Apply boldness and aliasing first as it affects the maximum size
    renderFont.setWeight( dlg.fontVideoStyle->currentData().toInt( ) );

    if ( fontsize == 0 )
        fontsize = lyricrenderer.autodetectFontSize( QSize(CDG_DRAW_WIDTH, CDG_DRAW_HEIGHT), renderFont );

    renderFont.setPointSize( fontsize );

    lyricrenderer.setRenderFont( renderFont );

    // Colors
	lyricrenderer.setColorBackground( m_colorBackground );
	lyricrenderer.setColorTitle( m_colorInfo );
	lyricrenderer.setColorSang( m_colorInactive );
	lyricrenderer.setColorToSing( m_colorActive );

	// Preamble
	lyricrenderer.setPreambleData( 4, 5000, 8 );

	// CD+G prefetching
	lyricrenderer.setPrefetch( 1000 );

	// CD+G fonts
	lyricrenderer.forceCDGmode();

	// Prepare images
	QImage lastImage( CDG_DRAW_WIDTH, CDG_DRAW_HEIGHT, QImage::Format_ARGB32 );
	lastImage.fill( m_colorBackground.rgb() );

	// Pop up progress dialog
	QDialog progressDialog;
	Ui::DialogEncodingProgress progressUi;
	progressUi.setupUi( &progressDialog );

	progressUi.groupBox->setTitle( "CD+G output statistics");
	progressUi.txtFrames->setText("CD+G packets:");

	progressUi.progressBar->setMaximum( 99 );
	progressUi.progressBar->setMinimum( -1 );
	progressUi.progressBar->setValue( -1 );

	progressUi.lblFrames->setText( "0" );
	progressUi.lblOutput->setText( "0 Mb" );
	progressUi.lblTime->setText( "0:00.00" );

	progressDialog.show();

	qint64 dialog_step = total_length / 100;

    // Preallocate the arrays
	init();
	m_stream.reserve( total_length * 300 / 1000 );

	// Render
	try
	{
		while ( 1 )
		{
			// There should be 300 packets in 1000ms of music
			// So each packet adds 1000 / 300 ms
			// Speed up time a little to compensate CD+G reader delay
			qint64 timing = m_stream.size() * 1000 / 300 + 250;

			// Should we show the next step?
			if ( timing / dialog_step > progressUi.progressBar->value() )
			{
				progressUi.progressBar->setValue( timing / dialog_step );

				progressUi.lblFrames->setText( QString::number( m_stream.size() ) );
				progressUi.lblOutput->setText( QString( "%1 Kb" ) .arg( m_stream.size() * 24 / 1024 ) );
				progressUi.lblTime->setText( markToTime( timing ) );

				progressUi.image->setPixmap( QPixmap::fromImage( lastImage ) );

				qApp->processEvents( QEventLoop::ExcludeUserInputEvents );
			}

			if ( timing > total_length )
				break;

	//		qDebug("timing: %d packets, %dms (%d sec)", m_stream.size(), (int) timing, (int) (timing / 1000) );
			int status = lyricrenderer.update( timing );

			if ( status == LyricsRenderer::UPDATE_RESIZED )
			{
				QImage errimg = lyricrenderer.image();
				errimg.save( "error", "bmp" );

				QMessageBox::critical( 0,
									  "Invalid lyrics",
									  QString("Lyrics out of boundary at %1, screen requested: %2x%3")
										.arg( markToTime( timing ) )
										.arg( errimg.width() )
										.arg( errimg.height() )
									  );

				m_stream.clear();
				return ;
			}

			if ( status == LyricsRenderer::UPDATE_NOCHANGE )
			{
				addEmpty();
				continue;
			}

			// Is change significant enough to warrant full redraw?
			if ( status == LyricsRenderer::UPDATE_FULL )
			{
				clearScreen();

				// Clear the old image too
				lastImage.fill( m_colorBackground.rgb() );
			}

			int packets = m_stream.size();
			const QImage& currImage = lyricrenderer.image();
			applyTileChanges( lastImage, currImage );
			lastImage = currImage;

			// Make sure we added at least some tiles
			if ( packets == m_stream.size() )
				addEmpty();
		}

		// Clean up the parity bits in the CD+G stream
		char *p = (char*) &m_stream[0];

		for ( unsigned int i = 0; i < m_stream.size() * sizeof(SubCode); i++, p++ )
		{
			if ( *p & 0xC0 )
				*p &= 0x3F;
		}

		QFile file( dlg.m_outputVideo );
		if ( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
		{
			QMessageBox::critical( 0,
								   QObject::tr("Cannot write CD+G file"),
								   QObject::tr("Cannot write CD+G file %1: %2")
										.arg( dlg.m_outputVideo)
										.arg(file.errorString()) );
			return;
		}

		file.write( stream() );
	}
	catch ( QString& txt )
	{
		QMessageBox::critical( 0,
							   QObject::tr("Cannot write CD+G file"),
							   QObject::tr("Cannot write CD+G file: %1")
									.arg( txt ) );
		return;
	}
}
