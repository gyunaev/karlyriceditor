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

#include "mediaplayer.h"
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

MediaPlayer * pAudioPlayer;


PlayerWidget::PlayerWidget(QWidget *parent)
	: QDockWidget(parent), Ui::PlayerWidget()
{
	setupUi(this);

	m_ready = false;
	m_sliderDown = false;
    mDuration = 0;

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

    // Initialize the update timer
    connect( &mUpdateTimer, &QTimer::timeout, this, &PlayerWidget::updateTimer );
    mUpdateTimer.setInterval( 250 );

	// Create the audio player, and initialize it
    mAudioPlayer = new MediaPlayer();

    // Connect the player signals
    connect( mAudioPlayer, &MediaPlayer::loaded, this, &PlayerWidget::mediaLoaded );
    connect( mAudioPlayer, &MediaPlayer::error, this, &PlayerWidget::mediaError );
    connect( mAudioPlayer, &MediaPlayer::finished, this, &PlayerWidget::mediaFinished );
    connect( mAudioPlayer, &MediaPlayer::durationChanged, this, &PlayerWidget::mediaDurationAvailable );

	// Connect the seek slider
    connect( seekSlider, SIGNAL(valueChanged(int)), this, SLOT(seekSliderMoved(int)) );
	connect( seekSlider, SIGNAL(sliderPressed()), this, SLOT(seekSliderDown()) );
	connect( seekSlider, SIGNAL(sliderReleased()), this, SLOT(seekSliderUp()) );
}

PlayerWidget::~PlayerWidget()
{
    delete mAudioPlayer;
}

void PlayerWidget::updateTimer()
{
    qint64 tickvalue = currentTime();
    emit tick(  tickvalue, totalTime() );

    lblTimeCur->setText( tr("<b>%1</b>") .arg( tickToString( tickvalue ) ) );

    if ( mDuration == 0 )
    {
        seekSlider->setEnabled( false );
        lblTimeRemaining->setText( tr("---") );
    }
    else
    {
        seekSlider->setEnabled( true );

        qint64 remaining = qMax( (qint64) 0, totalTime() - tickvalue );
        lblTimeRemaining->setText( tr("<b>-%1</b>") .arg( tickToString( remaining ) ) );

        if ( !m_sliderDown )
        {
            int value = currentTime() * seekSlider->maximum() / mDuration;

            if ( seekSlider->value() != value )
            {
                mCurrentSliderValue = value;
                seekSlider->setValue( value );
            }
        }
    }
}

QString PlayerWidget::tickToString( qint64 tickvalue )
{
	int minute = tickvalue / 60000;
	int second = (tickvalue-minute*60000)/1000;
	int msecond = (tickvalue-minute*60000-second*1000) / 100; // round milliseconds to 0-9 range}

    QString outtime;
    outtime.sprintf( "%02d:%02d.%d", minute, second, msecond );
    return outtime;
}

void PlayerWidget::openMusicFile( Project * project )
{
    mDuration = 0;
    mAudioPlayer->loadMedia( project->musicFile(), MediaPlayer::LoadAudioStream );
}

void PlayerWidget::btn_playerStop()
{
    if ( isPlaying() )
    {
        mAudioPlayer->seekTo( 0 );
        mAudioPlayer->stop();
    }

    updatePlayerState( Audio_StoppedState );
    mUpdateTimer.stop();
}

void PlayerWidget::btn_playerPlayPause()
{
    if ( isPlaying() )
	{
        mAudioPlayer->stop();
        mUpdateTimer.stop();
		updatePlayerState( Audio_PausedState );
	}
	else
	{
        mAudioPlayer->play();
        mUpdateTimer.start();
		updatePlayerState( Audio_PlayingState );
	}
}

void PlayerWidget::btn_playerSeekForward()
{
    seekToTime( currentTime() + 5000 );
}

void PlayerWidget::btn_playerSeekBackward()
{
    seekToTime( currentTime() - 5000 );
}

void PlayerWidget::seekToTime(qint64 time)
{
    if ( mAudioPlayer->state() == MediaPlayer::StatePlaying || mAudioPlayer->state() == MediaPlayer::StatePaused )
    {
        if ( time < 0 )
            time = 0;

        mAudioPlayer->seekTo( time );
    }
}

void PlayerWidget::startPlaying()
{
    if ( mAudioPlayer->state() == MediaPlayer::StatePlaying || mAudioPlayer->state() == MediaPlayer::StatePaused )
	{
        mAudioPlayer->stop();
        mAudioPlayer->reset();
	}

    mAudioPlayer->play();
    mUpdateTimer.start();
	updatePlayerState( Audio_PlayingState );
}

qint64 PlayerWidget::currentTime() const
{
    return mAudioPlayer->position();
}

qint64 PlayerWidget::totalTime() const
{
    return mDuration;
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
        lblTimeCur->setText( tr("<b>%1</b>") .arg( tickToString( currentTime() ) ) );

        if ( mDuration > 0 )
            lblTimeRemaining->setText( tr("<b>-%1</b>") .arg( tickToString( qMax( (qint64) 0, mDuration - currentTime() ) ) ) );
        else
            lblTimeRemaining->setText( tr("---") );
	}

	pMainWindow->updateState();
}


void PlayerWidget::seekSliderUp()
{
    mAudioPlayer->seekTo( seekSlider->value() * totalTime() / seekSlider->maximum() );
	m_sliderDown = false;
}

void PlayerWidget::seekSliderDown()
{
    m_sliderDown = true;
}

void PlayerWidget::mediaLoaded()
{
    // Media is successfully loaded
    mDuration = mAudioPlayer->duration();

    if ( mDuration > 0 )
        pMainWindow->project()->setSongLength( mDuration);

    setWindowTitle( tr("Player controls - %1 by %2")
                    .arg( pMainWindow->project()->tag( Project::Tag_Title ) )
                    .arg( pMainWindow->project()->tag( Project::Tag_Artist ) ) );


    updatePlayerState( Audio_StoppedState );
}

void PlayerWidget::mediaError(QString text)
{
    if ( pMainWindow->project() )
    {
        QMessageBox::critical( 0,
                           tr("Cannot open media file"),
                           tr("Cannot play media file %1: %2")
                                .arg( pMainWindow->project()->musicFile() )
                                .arg( text ) );

        updatePlayerState( Audio_ErrorState );
    }
}

void PlayerWidget::mediaFinished()
{
    btn_playerStop();
}

void PlayerWidget::mediaDurationAvailable()
{
    mDuration = mAudioPlayer->duration();
}

void PlayerWidget::seekSliderMoved( int newvalue )
{
    // Slider change is called because we changed its position programmatically
    if ( newvalue == mCurrentSliderValue )
        return;

    mAudioPlayer->seekTo( newvalue * totalTime() / seekSlider->maximum() );
}

bool PlayerWidget::isPlaying() const
{
    return mAudioPlayer->state() == MediaPlayer::StatePlaying;
}
