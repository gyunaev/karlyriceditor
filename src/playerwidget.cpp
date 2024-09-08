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

#include <QTime>
#include <QMessageBox>

#include "audioplayer.h"
#include "playerwidget.h"
#include "mainwindow.h"
#include "project.h"

enum
{
	Audio_ErrorState,
	Audio_PausedState,
	Audio_StoppedState,
	Audio_PlayingState
};


PlayerWidget::PlayerWidget(QWidget *parent)
	: QDockWidget(parent), Ui::PlayerWidget()
{
	setupUi(this);

	m_ready = false;

	// Set up icons
	btnFwd->setPixmap( QPixmap(":images/dryicons_forward.png") );
	btnPausePlay->setPixmap( QPixmap(":images/dryicons_play.png") );
	btnRew->setPixmap( QPixmap(":images/dryicons_rewind.png") );
	btnStop->setPixmap( QPixmap(":images/dryicons_stop") );

	// Connect button slots
	connect( btnFwd, SIGNAL( clicked() ), this, SLOT(btn_playerSeekForward()) );
	connect( btnRew, SIGNAL( clicked() ), this, SLOT(btn_playerSeekBackward()) );
	connect( btnPausePlay, SIGNAL( clicked() ), this, SLOT(btn_playerPlayPause()) );
	connect( btnStop, SIGNAL( clicked() ), this, SLOT(btn_playerStop()) );

	// Create the audio player, and initialize it
	pAudioPlayer = new AudioPlayer();

	if ( !pAudioPlayer->init() )
	{
		QMessageBox::critical( 0,
							  tr("Cannot initialize audio"),
							  tr("Audio output device cannot be initialized.\n\nPlease make sure the sound card "
								 "in your computer works fine, that all necessary drivers are installed, and "
								 "that no other application is currently locking the sound device.\n\n"
								 "Program is now aborting.") );
		exit( 1 );
	}

	// Connect the player
	connect( pAudioPlayer,
			 SIGNAL( tick(qint64) ),
			 this,
			 SLOT( slotAudioTick(qint64)) );

    connect( pAudioPlayer, SIGNAL(finished()), this, SLOT(btn_playerStop()) );

	// Connect the seek slider
	connect( seekSlider, SIGNAL(actionTriggered(int)), this, SLOT(seekSliderActionTriggered(int)) );
	connect( seekSlider, SIGNAL(sliderReleased()), this, SLOT(seekSliderUp()) );
}

PlayerWidget::~PlayerWidget()
{
	delete pAudioPlayer;
}

void PlayerWidget::slotAudioTick( qint64 tickvalue )
{
	emit tick( tickvalue );
	updateSlider( tickvalue );
	updateTimestamp( tickvalue );
}

QString PlayerWidget::tickToString( qint64 tickvalue )
{
	int minute = tickvalue / 60000;
	int second = (tickvalue-minute*60000)/1000;
	int msecond = (tickvalue-minute*60000-second*1000) / 100; // round milliseconds to 0-9 range}

	QTime ct( 0, minute, second, msecond );
    return ct.toString( "mm:ss" );
}

bool PlayerWidget::openMusicFile( Project * project )
{
	if ( !pAudioPlayer->open( project->musicFile() ) )
	{
		QMessageBox::critical( 0,
						   tr("Cannot open media file"),
						   tr("Cannot play media file %1: %2")
								.arg( project->musicFile() )
								.arg( pAudioPlayer->errorMsg() ) );

		updatePlayerState( Audio_ErrorState );
		return false;
	}

  seekSlider->setMaximum(totalTime() / 100);
	setWindowTitle( tr("Player controls - %1 by %2")
					.arg( project->tag( Project::Tag_Title ) )
					.arg( project->tag( Project::Tag_Artist ) ) );


	updatePlayerState( Audio_StoppedState );
	return true;
}

void PlayerWidget::btn_playerStop()
{
	pAudioPlayer->stop();
	pAudioPlayer->reset();
	updatePlayerState( Audio_StoppedState );
}

void PlayerWidget::btn_playerPlayPause()
{
	if ( pAudioPlayer->isPlaying() )
	{
		pAudioPlayer->stop();
		updatePlayerState( Audio_PausedState );
	}
	else
	{
		pAudioPlayer->play();
		updatePlayerState( Audio_PlayingState );
	}
}

void PlayerWidget::btn_playerSeekForward()
{
    seekToTime( pAudioPlayer->currentTime() + 5000 );
}

void PlayerWidget::btn_playerSeekBackward()
{
    seekToTime( pAudioPlayer->currentTime() - 5000 );
}

void PlayerWidget::seekToTime(qint64 time)
{
    pAudioPlayer->seekTo( qBound( 0, time, totalTime() ) );
    updateSlider( currentTime() );
    updateTimestamp( currentTime() );
}

void PlayerWidget::startPlaying()
{
	if ( pAudioPlayer->isPlaying() )
	{
		pAudioPlayer->stop();
		pAudioPlayer->reset();
	}

	pAudioPlayer->play();
	updatePlayerState( Audio_PlayingState );
}

qint64 PlayerWidget::currentTime() const
{
	return pAudioPlayer->currentTime();
}

qint64 PlayerWidget::totalTime() const
{
	return pAudioPlayer->totalTime();
}

void PlayerWidget::updatePlayerState( int newstate )
{
	bool ready = true, enable_playpause = false, enable_seek = false, enable_stop = false;

	switch ( newstate )
	{
		case Audio_ErrorState:
			ready = false;
			break;

		case Audio_PausedState:
			btnPausePlay->setPixmap( QPixmap( ":images/dryicons_play.png" ) );
			enable_playpause = true;
			enable_stop = true;
			break;

		case Audio_StoppedState:
			btnPausePlay->setPixmap( QPixmap( ":images/dryicons_play.png" ) );
			m_ready = true;
			enable_playpause = true;
			break;

		case Audio_PlayingState:
			btnPausePlay->setPixmap( QPixmap( ":images/dryicons_pause.png" ) );
			enable_seek = true;
			enable_playpause = true;
			enable_stop = true;
			break;
	}

	seekSlider->setEnabled( ready );
	btnRew->setEnabled( enable_seek );
	btnFwd->setEnabled( enable_seek );
	btnPausePlay->setEnabled( enable_playpause );
	btnStop->setEnabled( enable_stop );

	if ( !ready )
	{
		lblTimeCur->setText( "---" );
		lblTimeRemaining->setText( "---" );
	}
	else
	{
		updateSlider( currentTime() );
		updateTimestamp( currentTime() );
	}

	pMainWindow->updateState();
}

void PlayerWidget::updateSlider( qint64 time )
{
	if ( !seekSlider->isSliderDown() )
	{
		seekSlider->setValue( time * seekSlider->maximum() / totalTime() );
	}
}

void PlayerWidget::updateTimestamp( qint64 time )
{
  qint64 remaining = qMax( (qint64) 0, totalTime() - time );
  lblTimeCur->setText( tr("<b>%1</b>") .arg( tickToString( time ) ) );
  lblTimeRemaining->setText( tr("<b>-%1</b>") .arg( tickToString( remaining ) ) );
}

void PlayerWidget::seekSliderActionTriggered( int action )
{
  qint64 time = seekSlider->sliderPosition() * totalTime() / seekSlider->maximum();
  if ( action == QAbstractSlider::SliderMove )
  {
    seekSlider->setToolTip( tickToString( time ) );
    QPoint gpos = QCursor::pos();
    QPoint relpos = gpos;
    gpos -= seekSlider->pos();
    QHelpEvent ev( QEvent::ToolTip, relpos, gpos );
    QCoreApplication::sendEvent( seekSlider, &ev );
  }
  else
  {
    pAudioPlayer->seekTo( time );
    updateTimestamp( time );
  }
}

void PlayerWidget::seekSliderUp()
{
  qint64 time = seekSlider->sliderPosition() * totalTime() / seekSlider->maximum();
  seekSlider->setToolTip( QString() );
  pAudioPlayer->seekTo( time );
  updateTimestamp( time );
}

bool PlayerWidget::isPlaying() const
{
	return pAudioPlayer->isPlaying();
}
