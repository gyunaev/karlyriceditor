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

#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <QWhatsThis>

#include "textrenderer.h"
#include "dialog_export_params.h"
#include "playerwidget.h"
#include "settings.h"
#include "version.h"
#include "util.h"
#include "cdg.h"

// Video profiles
static const QMap< QString, QString > videoProfileMap = {
    { "OGG video with theora/vorbis" , "application/ogg:video/x-theora:audio/x-vorbis" },
    { "MP4 video with h264/aac" , "application/mp4:video/x-h264:audio/x-aac" },
};

DialogExportOptions::DialogExportOptions( Project * project, const Lyrics& lyrics,  bool video, QWidget *parent )
	: QDialog(parent), Ui::DialogExportParams(), m_renderer(0, 0)
{
	m_videomode = video;
	m_project = project;
	m_lyrics = lyrics;
	m_time = 0;

	// UIC stuff
	setupUi( this );

	// Buttons
	connect( btnBrowse, SIGNAL(clicked()), this, SLOT(browseOutputFile()) );

    // Changes in font type, text size strategy and font size box
    connect( fontVideo, SIGNAL(currentFontChanged(const QFont &)), this, SLOT( recalculateLargestFontSize() ) );
    connect( boxFontVideoSizeType, SIGNAL(currentIndexChanged(int)), this, SLOT( fontSizeStrategyChanged(int)) );

	// Tabs
	connect( tabWidget, SIGNAL(currentChanged(int)), this, SLOT(activateTab(int)) );
	connect( seekSlider, SIGNAL(valueChanged(int)), this, SLOT(previewSliderMoved(int)) );

    // Video resolution setup
    boxVideoResolution->addItem( "NTSC DVD 720x480", "720x480" );
    boxVideoResolution->addItem( "PAL DVD 720x576", "720x576" );
    boxVideoResolution->addItem( "HD 1280x720", "1280x720" );
    boxVideoResolution->addItem( "Full HD 1920x1080", "1920x1080" );

    if ( pSettings->isRegistered() )
        boxVideoResolution->addItem( "Ultra HD 3840x2160", "3840x2160" );

    // Video resolution combobox
    connect( boxVideoResolution, SIGNAL(currentIndexChanged(int)), this, SLOT(recalculateLargestFontSize() ) );

    // Video profiles setup
    for ( const auto v : videoProfileMap.keys() )
        boxVideoProfile->addItem( v, videoProfileMap[v] );

    // Add font video styles and choose "normal" to match previous behavior
    fontVideoStyle->addItem( "Thin", QFont::Thin );
    fontVideoStyle->addItem( "Light", QFont::Light );
    fontVideoStyle->addItem( "Normal", QFont::Normal );
    fontVideoStyle->addItem( "Medium", QFont::Medium );
    fontVideoStyle->addItem( "Bold", QFont::Bold );
    fontVideoStyle->addItem( "X-Bold", QFont::ExtraBold );
    fontVideoStyle->setCurrentIndex( 2 );

	if ( video )
	{
		// Set the video output params
		btnVideoColorActive->setColor( m_project->tag( Project::Tag_Video_activecolor, "blue" ) );
		btnVideoColorBg->setColor( m_project->tag( Project::Tag_Video_bgcolor, "black" ) );
		btnVideoColorInactive->setColor( m_project->tag( Project::Tag_Video_inactivecolor, "green" ) );
		btnVideoColorInfo->setColor( m_project->tag( Project::Tag_Video_infocolor, "white" ) );

		titleVideoMin->setValue( m_project->tag( Project::Tag_Video_titletime, "5" ).toInt() );
		cbVideoPreamble->setChecked( m_project->tag( Project::Tag_Video_preamble, "1" ).toInt() );

		setWindowTitle( tr("Specify video parameters") );
        leOutputFile->setText( m_project->tag( Project::Tag_ExportFilenameVideo, "" ) );

        boxTextVerticalAlign->setCurrentIndex( m_project->tag( Project::Tag_Video_TextAlignVertical, QString::number( TextRenderer::VerticalBottom ) ).toInt() );
	}
	else
	{
		// Hide video part
		groupVideo->hide();

		btnVideoColorActive->setColor( m_project->tag( Project::Tag_CDG_activecolor, "blue" ) );
		btnVideoColorBg->setColor( m_project->tag( Project::Tag_CDG_bgcolor, "black" ) );
		btnVideoColorInactive->setColor( m_project->tag( Project::Tag_CDG_inactivecolor, "green" ) );
		btnVideoColorInfo->setColor( m_project->tag( Project::Tag_CDG_infocolor, "white" ) );
		fontVideo->setCurrentFont( QFont( m_project->tag( Project::Tag_CDG_font, "arial" ) ) );
		titleVideoMin->setValue( m_project->tag( Project::Tag_CDG_titletime, "5").toInt() );
		cbVideoPreamble->setChecked( m_project->tag( Project::Tag_CDG_preamble, "1").toInt() );
        boxTextVerticalAlign->setCurrentIndex( m_project->tag( Project::Tag_CDG_TextAlignVertical, QString::number( TextRenderer::VerticalBottom ) ).toInt() );

		setWindowTitle( tr("Specify CDG parameters") );
		lblOutput->setText( tr("Write CD+G data to file:") );

        // Was filename specified before?
        if ( !m_project->tag( Project::Tag_ExportFilenameCDG, "" ).isEmpty() )
            leOutputFile->setText( m_project->tag( Project::Tag_ExportFilenameCDG, "" ) );
        else
            leOutputFile->setText( Util::removeFileExtention( m_project->musicFile() ) + "cdg" );

		// resize as we hid the video part
		resize( width(), 1 );
	}

    // This order matters!
    // this triggers font change callback and recalculates maximum value for spinFontSize
    fontVideo->setCurrentFont( QFont( m_project->tag( video ? Project::Tag_Video_font : Project::Tag_CDG_font, "arial" ) ) );
    fontVideo->setFontFilters( QFontComboBox::ScalableFonts | QFontComboBox::MonospacedFonts | QFontComboBox::ProportionalFonts );

    int fontsizevalue = m_project->tag( video ? Project::Tag_Video_fontsize : Project::Tag_CDG_fontsize, "0" ).toInt();

    if ( fontsizevalue > 0 )
    {
        boxFontVideoSizeType->setCurrentIndex( 0 );
        spinFontSize->setValue( fontsizevalue );
    }
    else
    {
        // This disables the spin and sets it to maximum
        boxFontVideoSizeType->setCurrentIndex( 1 );
    }

	// Title window
	leTitle->setText( m_project->tag( Project::Tag_Title, "") );
	leArtist->setText( m_project->tag( Project::Tag_Artist, "") );

    if ( pSettings->isRegistered() )
	{
		leTitleCreatedBy->setText( QString("Created by %1<br>http://www.ulduzsoft.com") .arg(APP_NAME) );
	}
	else
	{
		leTitleCreatedBy->setEnabled( false );
		leTitleCreatedBy->setText( "Application not registered, this field cannot be modified" );
    }
}


void DialogExportOptions::setBoxIndex( Project::Tag tag, QComboBox * box )
{
	QString val = m_project->tag( tag );

	if ( val.isEmpty() )
	{
		box->setCurrentIndex( 0 );
	}
	else
	{
		int idx = box->findText( val );

		if ( idx != -1 )
			box->setCurrentIndex( idx );
		else
			box->setCurrentIndex( 0 );
	}
}

int DialogExportOptions::calculateLargestFontSize( const QFont& font )
{
	// Ask the renderer
	TextRenderer renderer( 100, 100 );
	renderer.setLyrics( m_lyrics );
    renderer.setRenderFont( font );

    renderer.setTitlePageData( leArtist->text(),
                               leTitle->text(),
                               leTitleCreatedBy->text(),
                               m_project->tag( Project::Tag_CDG_titletime, "5" ).toInt() * 1000 );

    return renderer.autodetectFontSize( getVideoSize(), font );
}

bool DialogExportOptions::testFontSize()
{
    QFont font = fontVideo->currentFont();
    font.setPointSize( spinFontSize->value() );

    // Ask the renderer
    TextRenderer renderer( 100, 100 );
    renderer.setLyrics( m_lyrics );
    renderer.setRenderFont( font );

    renderer.setTitlePageData( leArtist->text(),
                               leTitle->text(),
                               leTitleCreatedBy->text(),
                               m_project->tag( Project::Tag_CDG_titletime, "5" ).toInt() * 1000 );

	if ( !renderer.verifyFontSize( getVideoSize(), font ) )
	{
		QMessageBox::critical( 0,
							  tr("Font size too large"),
							  tr("The output text cannot fit into screen using the specified font size") );
		return false;
	}

	return true;
}


void DialogExportOptions::browseOutputFile()
{
	QString outfile;

	if ( m_videomode )
	{
		QString exportdir = QSettings().value( "general/exportdirvideo", "" ).toString();
		outfile = QFileDialog::getSaveFileName( 0, tr("Export video to a file"), exportdir );
	}
	else
	{
		QString exportdir = QSettings().value( "general/exportdircdg", "" ).toString();
		outfile = QFileDialog::getSaveFileName( 0, tr("Export CD+G graphics to a file"), exportdir, "CD+G (*.cdg)" );
	}

	if ( outfile.isEmpty() )
		return;

	QFileInfo finfo( outfile );

	if ( m_videomode )
		QSettings().setValue( "general/exportdirvideo", finfo.dir().absolutePath() );
	else
		QSettings().setValue( "general/exportdircdg", finfo.dir().absolutePath() );

	leOutputFile->setText( outfile );
}


void DialogExportOptions::accept()
{
	m_outputVideo = leOutputFile->text();

	if ( m_outputVideo.isEmpty() )
	{
		QMessageBox::critical( 0, tr("Output file not specified"), tr("You must specify the output video file") );
		return;
	}

	if ( !testFontSize() )
		return;

	// Store title params
	m_artist = leArtist->text();
	m_title = leTitle->text();
	m_createdBy = leTitleCreatedBy->text();

	// Store encoding params
	if ( m_videomode )
	{
		// Store rendering params
		m_project->setTag( Project::Tag_Video_activecolor, btnVideoColorActive->color().name() );
		m_project->setTag( Project::Tag_Video_bgcolor, btnVideoColorBg->color().name() );
		m_project->setTag( Project::Tag_Video_inactivecolor, btnVideoColorInactive->color().name() );
		m_project->setTag( Project::Tag_Video_infocolor, btnVideoColorInfo->color().name() );
		m_project->setTag( Project::Tag_Video_font, fontVideo->currentFont().family() );

        if ( boxFontVideoSizeType->currentIndex() == 0 )
            m_project->setTag( Project::Tag_Video_fontsize, QString::number( spinFontSize->value() ) );
        else
            m_project->setTag( Project::Tag_Video_fontsize, "0" );

		m_project->setTag( Project::Tag_Video_titletime, QString::number( titleVideoMin->value() ) );
		m_project->setTag( Project::Tag_Video_preamble, cbVideoPreamble->isChecked() ? "1" : "0" );

        // Store the export file name
        m_project->setTag( Project::Tag_ExportFilenameVideo, leOutputFile->text() );
    }
	else
	{
		// Store rendering params
		m_project->setTag( Project::Tag_CDG_activecolor, btnVideoColorActive->color().name() );
		m_project->setTag( Project::Tag_CDG_bgcolor, btnVideoColorBg->color().name() );
		m_project->setTag( Project::Tag_CDG_inactivecolor, btnVideoColorInactive->color().name() );
		m_project->setTag( Project::Tag_CDG_infocolor, btnVideoColorInfo->color().name() );
		m_project->setTag( Project::Tag_CDG_font, fontVideo->currentFont().family() );

        if ( boxFontVideoSizeType->currentIndex() == 0 )
            m_project->setTag( Project::Tag_CDG_fontsize, QString::number( spinFontSize->value() ) );
        else
            m_project->setTag( Project::Tag_CDG_fontsize, "0" );

		m_project->setTag( Project::Tag_CDG_titletime, QString::number( titleVideoMin->value() ) );
		m_project->setTag( Project::Tag_CDG_preamble, cbVideoPreamble->isChecked() ? "1" : "0" );

        // Store the export file name
        m_project->setTag( Project::Tag_ExportFilenameCDG, leOutputFile->text() );
	}

    m_project->setTag( m_videomode ? Project::Tag_Video_TextAlignVertical : Project::Tag_CDG_TextAlignVertical,
                       QString::number( boxTextVerticalAlign->currentIndex() ) );

	QDialog::accept();
}


QSize DialogExportOptions::getVideoSize() const
{
	if ( !m_videomode )
	{
		// CD+G size is predefined
		return QSize( CDG_DRAW_WIDTH, CDG_DRAW_HEIGHT );
	}

    QStringList resolution = boxVideoResolution->currentData().toString().split("x");
    return QSize( resolution[0].toUInt(), resolution[1].toUInt() );
}

QString DialogExportOptions::videoProfile() const
{
    return boxVideoProfile->currentData().toString();
}

void DialogExportOptions::activateTab( int index )
{
    // We're only interested in Preview tab
    if ( index != 1 )
    {
        adjustSize();
        return;
    }

    // Prepare the text renderer using current params
    QFont font = fontVideo->currentFont();

    // Apply boldness and aliasing first as it affects the maximum size
    if ( boxEnableAntialiasing->isChecked() )
        font.setStyleStrategy( QFont::PreferAntialias );
    else
        font.setStyleStrategy( QFont::NoAntialias );

    font.setWeight( (QFont::Weight) fontVideoStyle->currentData().toInt( ) );

    // Autofit or fixed size?
    if ( boxFontVideoSizeType->currentIndex() == 0 )
        font.setPointSize( spinFontSize->value() );
    else
        font.setPointSize( calculateLargestFontSize(font) );

    m_renderer = TextRenderer( getVideoSize().width(), getVideoSize().height() );

    // The order here matters because setLyrics resets font and colors
    m_renderer.setDefaultVerticalAlign( (TextRenderer::VerticalAlignment) boxTextVerticalAlign->currentIndex() );
    m_renderer.setLyrics( m_lyrics );
    m_renderer.setRenderFont( font );
    m_renderer.setColorBackground( btnVideoColorBg->color() );
    m_renderer.setColorTitle( btnVideoColorInfo->color() );
    m_renderer.setColorSang( btnVideoColorInactive->color() );
    m_renderer.setColorToSing( btnVideoColorActive->color() );

    // CD+G?
    if ( !m_videomode )
        m_renderer.forceCDGmode();

    // Title
    m_renderer.setTitlePageData( leArtist->text(),
                                 leTitle->text(),
                                 pSettings->isRegistered() ? leTitleCreatedBy->text() : "",
                                 titleVideoMin->value() * 1000 );

    // Preamble
    if ( cbVideoPreamble->isChecked() )
        m_renderer.setPreambleData( 4, 5000, 8 );

    // Update the image
    previewUpdateImage();
    adjustSize();
}

void DialogExportOptions::previewUpdateImage()
{
	// Update the image
	m_renderer.update( m_time );
	QImage img = m_renderer.image();

	// For CD+G mode we enlarge the image as it is too small
	if ( !m_videomode )
	{
		QSize scaledsize( 2 * CDG_DRAW_WIDTH, 2 * CDG_DRAW_HEIGHT );
		img = img.scaled( scaledsize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
	}

	lblImage->setPixmap( QPixmap::fromImage( img ) );

	// Update the timings
	qint64 reminder = m_project->getSongLength() - m_time;

	lblCurrent->setText( PlayerWidget::tickToString( m_time ) );
	lblTotal->setText( PlayerWidget::tickToString( reminder ) );
}

void DialogExportOptions::previewSliderMoved( int newvalue )
{
	m_time = newvalue * m_project->getSongLength() / seekSlider->maximum();
    previewUpdateImage();
}

void DialogExportOptions::recalculateLargestFontSize()
{
    int maxsize = calculateLargestFontSize( fontVideo->currentFont() );
    spinFontSize->setMaximum( maxsize );

    if ( boxFontVideoSizeType->currentIndex() == 1 )
        spinFontSize->setValue( maxsize );
}

void DialogExportOptions::fontSizeStrategyChanged(int index)
{
    if ( index == 0 )
    {
        // Fixed size strategy
        spinFontSize->setEnabled( true );
        spinFontSize->setMinimum( 4 );
        spinFontSize->setMaximum( calculateLargestFontSize( fontVideo->currentFont() ) );
        spinFontSize->setValue( spinFontSize->maximum() );
    }
    else
    {
        spinFontSize->setMaximum( calculateLargestFontSize( fontVideo->currentFont() ) );
        spinFontSize->setValue( spinFontSize->maximum() );
        spinFontSize->setEnabled( false );
    }
}
