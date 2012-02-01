/**************************************************************************
 *  Karlyriceditor - a lyrics editor and CD+G / video export for Karaoke  *
 *  songs.                                                                *
 *  Copyright (C) 2009-2011 George Yunaev, support@karlyriceditor.com     *
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

#include "textrenderer.h"
#include "export_params.h"
#include "playerwidget.h"
#include "cdg.h"


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
	connect( btnDetectSize, SIGNAL(clicked()), this, SLOT(autodetectFontSize()) );
	connect( btnTestSize, SIGNAL(clicked()), this, SLOT(testFontSize()) );
	connect( btnBrowse, SIGNAL(clicked()), this, SLOT(browseOutputFile()) );

	// Tabs
	connect( tabWidget, SIGNAL(currentChanged(int)), this, SLOT(activateTab(int)) );
	connect( seekSlider, SIGNAL(valueChanged(int)), this, SLOT(previewSliderMoved(int)) );

	if ( video )
	{
		// Set the video output params
		setBoxIndex( Project::Tag_Video_ImgSizeIndex, boxImageSize );
		setBoxIndex( Project::Tag_Video_FpsIndex, boxFPS );
		setBoxIndex( Project::Tag_Video_EncodingIndex, boxVideoEncoding );
		setBoxIndex( Project::Tag_Video_ContainerIndex, boxContainer );
		cbAllKeyframes->setChecked( m_project->tag( Project::Tag_Video_AllKeyframes).toInt() );
		cbExportNoAudio->setChecked( m_project->tag( Project::Tag_Video_ExportNoAudio).toInt() );

		btnVideoColorActive->setColor( m_project->tag( Project::Tag_Video_activecolor, "blue" ) );
		btnVideoColorBg->setColor( m_project->tag( Project::Tag_Video_bgcolor, "black" ) );
		btnVideoColorInactive->setColor( m_project->tag( Project::Tag_Video_inactivecolor, "green" ) );
		btnVideoColorInfo->setColor( m_project->tag( Project::Tag_Video_infocolor, "white" ) );
		fontVideo->setCurrentFont( QFont( m_project->tag( Project::Tag_Video_font, "arial" ) ) );
		fontVideoSize->setValue( m_project->tag( Project::Tag_Video_fontsize, "8" ).toInt() );
		titleVideoMin->setValue( m_project->tag( Project::Tag_Video_titletime, "5" ).toInt() );
		cbVideoPreamble->setChecked( m_project->tag( Project::Tag_Video_preamble, "1" ).toInt() );

		setWindowTitle( tr("Specify video parameters") );
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
		fontVideoSize->setValue( m_project->tag( Project::Tag_CDG_fontsize, "8").toInt() );
		titleVideoMin->setValue( m_project->tag( Project::Tag_CDG_titletime, "5").toInt() );
		cbVideoPreamble->setChecked( m_project->tag( Project::Tag_CDG_preamble, "1").toInt() );

		setWindowTitle( tr("Specify CDG parameters") );
		lblOutput->setText( tr("Write CD+G data to file:") );

		// resize as we hid the video part
		resize( width(), 1 );
	}

	fontVideo->setFontFilters( QFontComboBox::ScalableFonts | QFontComboBox::MonospacedFonts | QFontComboBox::ProportionalFonts );
}

DialogExportOptions::~DialogExportOptions()
{
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

void DialogExportOptions::autodetectFontSize()
{
	QFont font = fontVideo->currentFont();

	if ( !m_videomode )
		font.setStyleStrategy( QFont::NoAntialias );

	// Ask the renderer
	TextRenderer renderer( 100, 100 );
	renderer.setLyrics( m_lyrics );
	int fsize = renderer.autodetectFontSize( getVideoSize(), font );

	fontVideoSize->setValue( fsize );
}

bool DialogExportOptions::testFontSize()
{
	QFont font = fontVideo->currentFont();
	font.setPointSize( fontVideoSize->value() );

	if ( !m_videomode )
		font.setStyleStrategy( QFont::NoAntialias );

	TextRenderer renderer( 100, 100 );
	renderer.setLyrics( m_lyrics );

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

	// Store encoding params
	if ( m_videomode )
	{
		m_project->setTag( Project::Tag_Video_ImgSizeIndex, boxImageSize->currentText() );
		m_project->setTag( Project::Tag_Video_FpsIndex, boxFPS->currentText() );
		m_project->setTag( Project::Tag_Video_EncodingIndex, boxVideoEncoding->currentText() );
		m_project->setTag( Project::Tag_Video_ContainerIndex, boxContainer->currentText() );
		m_project->setTag( Project::Tag_Video_AllKeyframes, cbAllKeyframes->isChecked() ? "1" : "0" );
		m_project->setTag( Project::Tag_Video_ExportNoAudio, cbExportNoAudio->isChecked() ? "1" : "0" );

		// Store rendering params
		m_project->setTag( Project::Tag_Video_activecolor, btnVideoColorActive->color().name() );
		m_project->setTag( Project::Tag_Video_bgcolor, btnVideoColorBg->color().name() );
		m_project->setTag( Project::Tag_Video_inactivecolor, btnVideoColorInactive->color().name() );
		m_project->setTag( Project::Tag_Video_infocolor, btnVideoColorInfo->color().name() );
		m_project->setTag( Project::Tag_Video_font, fontVideo->currentFont().family() );
		m_project->setTag( Project::Tag_Video_fontsize, QString::number( fontVideoSize->value() ) );
		m_project->setTag( Project::Tag_Video_titletime, QString::number( titleVideoMin->value() ) );
		m_project->setTag( Project::Tag_Video_preamble, cbVideoPreamble->isChecked() ? "1" : "0" );
	}
	else
	{
		// Store rendering params
		m_project->setTag( Project::Tag_CDG_activecolor, btnVideoColorActive->color().name() );
		m_project->setTag( Project::Tag_CDG_bgcolor, btnVideoColorBg->color().name() );
		m_project->setTag( Project::Tag_CDG_inactivecolor, btnVideoColorInactive->color().name() );
		m_project->setTag( Project::Tag_CDG_infocolor, btnVideoColorInfo->color().name() );
		m_project->setTag( Project::Tag_CDG_font, fontVideo->currentFont().family() );
		m_project->setTag( Project::Tag_CDG_fontsize, QString::number( fontVideoSize->value() ) );
		m_project->setTag( Project::Tag_CDG_titletime, QString::number( titleVideoMin->value() ) );
		m_project->setTag( Project::Tag_CDG_preamble, cbVideoPreamble->isChecked() ? "1" : "0" );
	}

	QDialog::accept();
}

QSize DialogExportOptions::getVideoSize()
{
	if ( !m_videomode )
	{
		// CD+G size is predefined
		return QSize( CDG_DRAW_WIDTH, CDG_DRAW_HEIGHT );
	}

	// Get the current value as it is used in this module as well
	QRegExp re( "^(\\d+)x(\\d+)");
	QString val = boxImageSize->currentText();

	if ( val.indexOf( re ) == -1 )
		return QSize();

	return QSize( re.cap(1).toInt(), re.cap(2).toInt() );
}

void DialogExportOptions::getFPS(  unsigned int * num, unsigned int * den )
{
	QRegExp re( "^([0-9.]+) FPS");
	QString val = m_project->tag( Project::Tag_Video_FpsIndex);

	*num = 0;
	*den = 0;

	if ( val.indexOf( re ) != -1 )
	{
		double fps = re.cap( 1 ).toDouble();

		if ( fps == 25.0 )
		{
			*num = 1;
			*den = 25;
		}
		else if ( fps == 29.97 )
		{
			*num = 1001;
			*den = 30000;
		}
		else if ( fps == 23.976 )
		{
			*num = 1001;
			*den = 24000;
		}
	}
}

QString	DialogExportOptions::getEncoding()
{
	return m_project->tag( Project::Tag_Video_EncodingIndex ).toLower();
}

QString	DialogExportOptions::getContainer()
{
	return m_project->tag( Project::Tag_Video_ContainerIndex ).toLower();
}

void DialogExportOptions::activateTab( int index )
{
	// We're only interested in Preview tab
	if ( index != 1 )
		return;

	// Prepare the text renderer using current params
	QFont font = fontVideo->currentFont();
	font.setPointSize( fontVideoSize->value() );

	m_renderer = TextRenderer( getVideoSize().width(), getVideoSize().height() );

	// Initialize colors from m_project
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
	m_renderer.setTitlePageData( m_project->tag( Project::Tag_Artist ),
								 m_project->tag( Project::Tag_Title ),
								 titleVideoMin->value() * 1000 );

	// Preamble
	if ( cbVideoPreamble->isChecked() )
		m_renderer.setPreambleData( 4, 5000, 8 );

	// Update the image
	previewUpdateImage();
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
