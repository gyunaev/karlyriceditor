/**************************************************************************
 *  Karlyriceditor - a lyrics editor for Karaoke songs                    *
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
#include <QMessageBox>

#include "textrenderer.h"
#include "export_params.h"
#include "cdg.h"

#define CDG_VIDEO_SIZE	QSize( CDG_FULL_WIDTH - 2*CDG_BORDER_WIDTH, CDG_FULL_HEIGHT - CDG_BORDER_HEIGHT )

DialogExportOptions::DialogExportOptions( Project * project, const Lyrics& lyrics,  bool video, QWidget *parent )
	: QDialog(parent), Ui::DialogExportParams()
{
	m_videomode = video;
	m_project = project;
	m_lyrics = lyrics;

	// UIC stuff
	setupUi( this );

	// Buttons
	connect( btnDetectSize, SIGNAL(clicked()), this, SLOT(autodetectFontSize()) );
	connect( btnPreview, SIGNAL(clicked()), this, SLOT(showPreview()) );
	connect( btnBrowse, SIGNAL(clicked()), this, SLOT(browseOutputFile()) );

	if ( video )
	{
		// Set the video output params
		setBoxIndex( Project::Tag_Video_ImgSizeIndex, boxImageSize );
		setBoxIndex( Project::Tag_Video_FpsIndex, boxFPS );
		setBoxIndex( Project::Tag_Video_EncodingIndex, boxVideoEncoding );
		setBoxIndex( Project::Tag_Video_ContainerIndex, boxContainer );
		cbAllKeyframes->setChecked( m_project->tag( Project::Tag_Video_AllKeyframes).toInt() );
		cbExportNoAudio->setChecked( m_project->tag( Project::Tag_Video_ExportNoAudio).toInt() );

		btnVideoColorActive->setColor( m_project->tag( Project::Tag_Video_activecolor ) );
		btnVideoColorBg->setColor( m_project->tag( Project::Tag_Video_bgcolor ) );
		btnVideoColorInactive->setColor( m_project->tag( Project::Tag_Video_inactivecolor ) );
		btnVideoColorInfo->setColor( m_project->tag( Project::Tag_Video_infocolor ) );
		fontVideo->setCurrentFont( QFont( m_project->tag( Project::Tag_Video_font ) ) );
		fontVideoSize->setValue( m_project->tag( Project::Tag_Video_fontsize).toInt() );
		titleVideoMin->setValue( m_project->tag( Project::Tag_Video_titletime).toInt() );
		cbVideoPreamble->setChecked( m_project->tag( Project::Tag_Video_preamble).toInt() );

		setWindowTitle( tr("Specify video parameters") );
	}
	else
	{
		// Hide video part
		groupVideo->hide();

		btnVideoColorActive->setColor( m_project->tag( Project::Tag_CDG_activecolor ) );
		btnVideoColorBg->setColor( m_project->tag( Project::Tag_CDG_bgcolor ) );
		btnVideoColorInactive->setColor( m_project->tag( Project::Tag_CDG_inactivecolor ) );
		btnVideoColorInfo->setColor( m_project->tag( Project::Tag_CDG_infocolor ) );
		fontVideo->setCurrentFont( QFont( m_project->tag( Project::Tag_CDG_font ) ) );
		fontVideoSize->setValue( m_project->tag( Project::Tag_CDG_fontsize).toInt() );
		titleVideoMin->setValue( m_project->tag( Project::Tag_CDG_titletime).toInt() );
		cbVideoPreamble->setChecked( m_project->tag( Project::Tag_CDG_preamble).toInt() );

		setWindowTitle( tr("Specify CDG parameters") );

		// resize as we hid the video part
		resize( width(), 1 );
	}

	fontVideo->setFontFilters( QFontComboBox::ScalableFonts | QFontComboBox::MonospacedFonts | QFontComboBox::ProportionalFonts );
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
	QSize videosize = m_videomode ? getVideoSize( m_project ) : CDG_VIDEO_SIZE;

	// Ask the renderer
	int size = TextRenderer::autodetectFontSize( videosize, m_lyrics, fontVideo->currentFont() );

	fontVideoSize->setValue( size );
}

void DialogExportOptions::btnTestFontSize()
{
	testFontSize();
}

bool DialogExportOptions::testFontSize()
{
	QSize videosize = m_videomode ? getVideoSize( m_project ) : CDG_VIDEO_SIZE;

	QFont font = fontVideo->currentFont();
	font.setPixelSize( fontVideoSize->value() );

	if ( !TextRenderer::verifyFontSize( videosize, m_lyrics, font ) )
	{
		QMessageBox::critical( 0,
							  tr("Font size too large"),
							  tr("The output text cannot fit into screen using the specified font size") );
		return false;
	}

	return true;
}


void DialogExportOptions::showPreview()
{

}

void DialogExportOptions::browseOutputFile()
{
	QString outfile;

	if ( m_videomode )
		outfile = QFileDialog::getSaveFileName( 0, tr("Export video to a file") );
	else
		outfile = QFileDialog::getSaveFileName( 0, tr("Export CD+G graphics to a file"), QString::null, "CD+G (*.cdg)" );

	if ( outfile.isEmpty() )
		return;

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

QSize DialogExportOptions::getVideoSize( Project * project )
{
	QRegExp re( "^(\\d+)x(\\d+)");
	QString val = project->tag( Project::Tag_Video_ImgSizeIndex );

	if ( val.indexOf( re ) == -1 )
		return QSize();

	return QSize( re.cap(1).toInt(), re.cap(2).toInt() );
}

void DialogExportOptions::getFPS( Project * project, unsigned int * num, unsigned int * den )
{
	QRegExp re( "^([0-9.]+) FPS");
	QString val = project->tag( Project::Tag_Video_FpsIndex);

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

QString	DialogExportOptions::getEncoding( Project * project )
{
	return project->tag( Project::Tag_Video_EncodingIndex ).toLower();
}

QString	DialogExportOptions::getContainer( Project * project )
{
	return project->tag( Project::Tag_Video_ContainerIndex ).toLower();
}
