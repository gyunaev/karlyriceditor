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

#include <QProgressDialog>
#include <QApplication>
#include <QMessageBox>

#include "audioplayer.h"
#include "videogenerator.h"
#include "textrenderer.h"
#include "export_params.h"
#include "ffmpegvideoencoder.h"
#include "editor.h"

#include "ui_dialog_videoencprogress.h"


VideoGenerator::VideoGenerator( Project * prj )
{
	m_project = prj;
}


void VideoGenerator::generate( const Lyrics& lyrics, qint64 total_length )
{
	// Show the dialog with video options
	DialogExportOptions dlg( m_project, lyrics, true );

	if ( dlg.exec() != QDialog::Accepted )
		return;

	// Validate params
	unsigned int num, den;
	QSize videosize = dlg.getVideoSize( m_project );
	dlg.getFPS( m_project, &num, &den );

	if ( videosize.isEmpty() || num == 0 || den == 0 )
	{
		QMessageBox::critical( 0, "Invalid parameters", "Video size or FPS is not valid" );
		return;
	}

	// Prepare the renderer
	TextRenderer lyricrenderer( videosize.width(), videosize.height() );
	lyricrenderer.setData( lyrics );

	// Initialize colors from m_project
	lyricrenderer.setRenderFont( QFont( m_project->tag(Project::Tag_Video_font), m_project->tag(Project::Tag_Video_fontsize).toInt()) );
	lyricrenderer.setColorBackground( m_project->tag( Project::Tag_Video_bgcolor ) );
	lyricrenderer.setColorTitle( m_project->tag( Project::Tag_Video_infocolor ) );
	lyricrenderer.setColorSang( m_project->tag( Project::Tag_Video_inactivecolor ) );
	lyricrenderer.setColorToSing( m_project->tag( Project::Tag_Video_activecolor ) );

	// Title
	lyricrenderer.setTitlePageData( m_project->tag( Project::Tag_Artist ),
								 m_project->tag( Project::Tag_Title ),
								 m_project->tag( Project::Tag_Video_titletime ).toInt() * 1000 );

	// Preamble
	if ( m_project->tag( Project::Tag_Video_preamble).toInt() != 0 )
		lyricrenderer.setPreambleData( 4, 5000, 8 );

	// Video encoder
	FFMpegVideoEncoder encoder;

	QString errmsg = encoder.createFile( dlg.m_outputVideo,
										dlg.getContainer( m_project ),
										videosize,
										2000000,
										num, den,
										25,
										m_project->tag( Project::Tag_Video_ExportNoAudio).toInt() > 0 ? 0 : pAudioPlayer );

	if ( !errmsg.isEmpty() )
	{
		QMessageBox::critical( 0,
							  "Cannot write video",
							  QString("Cannot create video file: %1") .arg(errmsg) );
		return;
	}

	// Pop up progress dialog
	QDialog progressdlg;
	Ui::DialogVideoEncodingProgress ui;
	ui.setupUi( &progressdlg );

	ui.progressBar->setMaximum( 99 );
	ui.progressBar->setMinimum( -1 );
	ui.progressBar->setValue( -1 );

	ui.lblFrames->setText( "0" );
	ui.lblOutput->setText( "0 Mb" );
	ui.lblTime->setText( "0:00.00" );

	progressdlg.show();

	qint64 dialog_step = total_length / 100;
	qint64 time_step = (1000 * num) / den;

	int frames = 0, size = 0;

	// Rendering
	for ( qint64 time = 0; time < total_length; time += time_step )
	{
		frames++;
		lyricrenderer.update( time );
		QImage image = lyricrenderer.image();
		size += encoder.encodeImage( image );

		// Should we update the progress dialog?
		if ( time / dialog_step > ui.progressBar->value() )
		{
			ui.progressBar->setValue( time / dialog_step );

			ui.lblFrames->setText( QString::number( frames ) );
			ui.lblOutput->setText( QString( "%1 Mb" ) .arg( size / (1024*1024) ) );
			ui.lblTime->setText( markToTime( time ) );
			ui.image->setPixmap( QPixmap::fromImage( image ).scaled( ui.image->size() ) );

			qApp->processEvents( QEventLoop::ExcludeUserInputEvents );
		}
	}

	encoder.close();
}
