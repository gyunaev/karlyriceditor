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

#include "videogenerator.h"
#include "textrenderer.h"
#include "dialog_export_params.h"
#include "editor.h"
#include "playerwidget.h"
#include "settings.h"

VideoGenerator::VideoGenerator( Project * prj, const Lyrics &lyrics, qint64 totaltime )
    : QDialog(), m_lyrics(lyrics)
{
    mProgress.setupUi( this );
	m_project = prj;
    m_encoder = 0;
    m_totalTime = totaltime;
}

void VideoGenerator::generate( PlayerWidget * widget )
{
	// Show the dialog with video options
    DialogExportOptions dlg( m_project, m_lyrics, true );

	if ( dlg.exec() != QDialog::Accepted )
    {
        deleteLater();
		return;
    }

    // Video profile and size
    QSize resolution = dlg.getVideoSize();
    QString profile = dlg.videoProfile();

	// Prepare the renderer
    TextRenderer * lyricrenderer = new TextRenderer( resolution.width(), resolution.height() );

    // Must be set before lyrics
    lyricrenderer->setDefaultVerticalAlign( (TextRenderer::VerticalAlignment) m_project->tag( Project::Tag_Video_TextAlignVertical, QString::number( TextRenderer::VerticalBottom ) ).toInt() );

    // The order matters because setLyrics resets font and colors
    lyricrenderer->setLyrics( m_lyrics );

    // Title
    lyricrenderer->setTitlePageData( dlg.m_artist,
                                    dlg.m_title,
                                    dlg.m_createdBy,
                                    m_project->tag( Project::Tag_Video_titletime, "5" ).toInt() * 1000 );


    // Rendering font
    QFont renderFont( m_project->tag(Project::Tag_Video_font ) );
    int fontsize = m_project->tag(Project::Tag_Video_fontsize).toInt();

    if ( fontsize == 0 )
        fontsize = lyricrenderer->autodetectFontSize( resolution, renderFont );

    renderFont.setPointSize( fontsize );

	// Initialize colors from m_project
    lyricrenderer->setRenderFont( renderFont );
    lyricrenderer->setColorBackground( m_project->tag( Project::Tag_Video_bgcolor, "black" ) );
    lyricrenderer->setColorTitle( m_project->tag( Project::Tag_Video_infocolor, "white" ) );
    lyricrenderer->setColorSang( m_project->tag( Project::Tag_Video_inactivecolor, "blue" ) );
    lyricrenderer->setColorToSing( m_project->tag( Project::Tag_Video_activecolor, "green" ) );    

	// Preamble
	if ( m_project->tag( Project::Tag_Video_preamble).toInt() != 0 )
        lyricrenderer->setPreambleData( 4, 5000, 8 );

    m_processedFrames = 0;
    m_lastImageUpdate.start();
    m_totalRenderTime.start();

    connect( &m_progressUpdateTimer, SIGNAL(timeout()), this, SLOT(updateProgress()));

    // Pop up our progress dialog
    mProgress.progressBar->setMaximum( 99 );
    mProgress.progressBar->setMinimum( -1 );
    mProgress.progressBar->setValue( -1 );

    mProgress.lblFrames->setText( "0" );
    mProgress.lblTime->setText( "0:00.00" );

    m_encoder = new MediaPlayer();
    connect( m_encoder, SIGNAL(mediaLoadingFinished(MediaPlayer::State,QString)), this, SLOT(mediaLoadingFinished(MediaPlayer::State,QString)) );
    connect( m_encoder, SIGNAL(durationChanged()), this, SLOT(mediaDurationChanged()) );
    connect( m_encoder, SIGNAL(finished()), this, SLOT(mediaFinished()) );

    // Launch the encoder process
    m_paramsSet = false;
    m_encoder->prepareVideoEncoder( m_project->musicFile(),
                                    dlg.m_outputVideo,
                                    profile,
                                    resolution,
                                    [this, widget, lyricrenderer] ( qint64 timing ) -> const QImage {

                                        // Detect if the stream should end; we only do this once we have the length,
                                        // as we do not know the duration until we start playing
                                        if ( m_encoder == 0 || (m_totalTime != -1 && timing >= m_totalTime) )
                                            return QImage();

                                        m_currentTime = timing;
                                        m_processedFrames++;

                                        lyricrenderer->update(timing);

                                        if ( m_lastImageUpdate.elapsed() > 450 )
                                        {
                                            m_lastImageUpdate.restart();

                                            QMutexLocker m( &mProgressMutex );
                                            m_lastRenderedImage = lyricrenderer->image();
                                        }

                                        return lyricrenderer->image();
                                    });

    if ( pSettings->isRegistered() )
        widget->applySettings( m_encoder );

    exec();
}

void VideoGenerator::mediaLoadingFinished(MediaPlayer::State newstate, QString error)
{
    if ( newstate == MediaPlayer::StateFailed )
    {
        QMessageBox::critical( 0,
                               tr("Cannot set up encoder"),
                               error );

        mediaFinished();
    }
    else
    {
        mProgress.lblInfo->setText("Encoding...");
        m_progressUpdateTimer.start( 250 );
        m_encoder->play();
    }
}

void VideoGenerator::mediaFinished()
{
    qDebug("media finished");
    m_encoder->stop();
    m_encoder->deleteLater();

    // Close the dialog window
    reject();
    deleteLater();
}

void VideoGenerator::buttonAbort()
{
    m_encoder->stop();
    m_encoder->deleteLater();
    m_encoder = 0;

    reject();
    deleteLater();
}

void VideoGenerator::updateProgress()
{
    mProgress.progressBar->setValue( (m_currentTime * 100) / m_totalTime );
    mProgress.lblFrames->setText( QString::number(m_processedFrames) );

    // elapsed encoding time
    unsigned int elapsed = m_totalRenderTime.elapsed() / 1000;
    mProgress.lblTime->setText( QString::asprintf("%02d:%02d", elapsed / 60, elapsed % 60 ) );

    QMutexLocker m( &mProgressMutex );

    if ( !m_lastRenderedImage.isNull() )
        mProgress.image->setPixmap( QPixmap::fromImage( m_lastRenderedImage ).scaled( mProgress.image->size() ) );
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
