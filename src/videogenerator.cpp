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

#include <QProgressDialog>
#include <QApplication>
#include <QMessageBox>
#include <QTime>

#include "audioplayer.h"
#include "videogenerator.h"
#include "textrenderer.h"
#include "export_params.h"
#include "ffmpegvideoencoder.h"
#include "editor.h"
#include "videogeneratorthread.h"


VideoGenerator::VideoGenerator( Project * prj )
{
    mProgress.setupUi( this );
	m_project = prj;
    mVideoGeneratorThread = 0;
}


void VideoGenerator::generate( const Lyrics& lyrics, qint64 total_length )
{
	// Show the dialog with video options
	DialogExportOptions dlg( m_project, lyrics, true );

	if ( dlg.exec() != QDialog::Accepted )
		return;

	// Get the video info
	const VideoEncodingProfile * profile;
	const VideoFormat * format;
    unsigned int		audioEncodingType;
	unsigned int		quality;

    if ( !dlg.videoParams( &profile, &format, &audioEncodingType, &quality ) )
		return;

	// Prepare the renderer
    TextRenderer * lyricrenderer = new TextRenderer( format->width, format->height );
    lyricrenderer->setLyrics( lyrics );

	// Initialize colors from m_project
    lyricrenderer->setRenderFont( QFont( m_project->tag(Project::Tag_Video_font, "arial"), m_project->tag(Project::Tag_Video_fontsize, "8").toInt()) );
    lyricrenderer->setColorBackground( m_project->tag( Project::Tag_Video_bgcolor, "black" ) );
    lyricrenderer->setColorTitle( m_project->tag( Project::Tag_Video_infocolor, "white" ) );
    lyricrenderer->setColorSang( m_project->tag( Project::Tag_Video_inactivecolor, "blue" ) );
    lyricrenderer->setColorToSing( m_project->tag( Project::Tag_Video_activecolor, "green" ) );

	// Title
    lyricrenderer->setTitlePageData( dlg.m_artist,
									dlg.m_title,
									dlg.m_createdBy,
									m_project->tag( Project::Tag_Video_titletime, "5" ).toInt() * 1000 );

	// Preamble
	if ( m_project->tag( Project::Tag_Video_preamble).toInt() != 0 )
        lyricrenderer->setPreambleData( 4, 5000, 8 );

	// Video encoder
    FFMpegVideoEncoder * encoder = new FFMpegVideoEncoder();

	// audioEncodingMode: 0 - encode, 1 - copy, 2 - no audio
    QString errmsg = encoder->createFile( dlg.m_outputVideo,
										 profile,
										 format,
										 quality,
                                         audioEncodingType == 1 ? 0 : pAudioPlayer );

	if ( !errmsg.isEmpty() )
	{
		QMessageBox::critical( 0,
							  "Cannot write video",
							  QString("Cannot create video file: %1") .arg(errmsg) );
		return;
	}

    // Calculate the time step for rendering
    qint64 time_step = (1000 * format->frame_rate_num) / format->frame_rate_den;

    // Start the video encoding
    mVideoGeneratorThread = new VideoGeneratorThread( encoder, lyricrenderer, total_length, time_step );
    mVideoGeneratorThread->start();

    // Connect the signals
    connect( mVideoGeneratorThread, SIGNAL( finished(QString)), this, SLOT(finished(QString)), Qt::QueuedConnection );
    connect( mVideoGeneratorThread, SIGNAL( progress(int, QString, QString, QString)), this, SLOT(progress(int, QString, QString, QString)), Qt::QueuedConnection );

    // Pop up our progress dialog
    mProgress.progressBar->setMaximum( 99 );
    mProgress.progressBar->setMinimum( -1 );
    mProgress.progressBar->setValue( -1 );

    mProgress.lblFrames->setText( "0" );
    mProgress.lblOutput->setText( "0 Mb" );
    mProgress.lblTime->setText( "0:00.00" );

    exec();
}

void VideoGenerator::progress( int progress, QString frames, QString size, QString timing )
{
    mProgress.progressBar->setValue( progress );

    mProgress.lblFrames->setText( frames );
    mProgress.lblOutput->setText( size );
    mProgress.lblTime->setText( timing );
    mProgress.image->setPixmap( QPixmap::fromImage( mVideoGeneratorThread->currentImage() ).scaled( mProgress.image->size() ) );
}

void VideoGenerator::finished( QString errormsg )
{
    // This slot is called when the encoding thread is finished, which may mean aborted, or error
    if ( !errormsg.isEmpty() )
        QMessageBox::critical( 0,
                               "Video encoding failed",
                               QString( "Failed to encode the video:\n%1").arg( errormsg ) );

    mVideoGeneratorThread->deleteLater();

    // Close the dialog window
    reject();
}

void VideoGenerator::buttonAbort()
{
    mVideoGeneratorThread->abort();
}

void VideoGenerator::closeEvent(QCloseEvent *e)
{
    // We handle it internally
    e->ignore();

    if ( QMessageBox::question( 0,
                                "Abort video encoding?",
                                "Do you want to abort video encoding process?",
                                QMessageBox::Yes,
                                QMessageBox::No ) != QMessageBox::Yes )
        return;

    buttonAbort();
}
