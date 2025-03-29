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


PlayerWidget::PlayerWidget(QWidget *parent)
	: QDockWidget(parent), Ui::PlayerWidget()
{
	setupUi(this);

	m_sliderDown = false;
    m_changingSliderPosition = false;
    m_totalTime = 0;
    m_currentTime = 0;

    setWindowTitle( tr("Player controls") );

	// Set up icons
	btnFwd->setPixmap( QPixmap(":images/dryicons_forward.png") );
	btnPausePlay->setPixmap( QPixmap(":images/dryicons_play.png") );
	btnRew->setPixmap( QPixmap(":images/dryicons_rewind.png") );
	btnStop->setPixmap( QPixmap(":images/dryicons_stop") );

	// Connect the seek slider
	connect( seekSlider, SIGNAL(sliderMoved(int)), this, SLOT(seekSliderMoved(int)) );
	connect( seekSlider, SIGNAL(sliderPressed()), this, SLOT(seekSliderDown()) );
	connect( seekSlider, SIGNAL(sliderReleased()), this, SLOT(seekSliderUp()) );
    connect( seekSlider, SIGNAL(valueChanged(int)), this, SLOT(seekSliderMoved(int)) );
    connect( btnFwd, SIGNAL(clicked()), this, SLOT(btnSeekForward()) );
    connect( btnRew, SIGNAL(clicked()), this, SLOT(btnSeekBackward()) );
}

PlayerWidget::~PlayerWidget()
{
}

QString PlayerWidget::tickToString( qint64 tickvalue )
{
	int minute = tickvalue / 60000;
	int second = (tickvalue-minute*60000)/1000;
	int msecond = (tickvalue-minute*60000-second*1000) / 100; // round milliseconds to 0-9 range}

	QTime ct( 0, minute, second, msecond );
    return ct.toString( "mm:ss" );
}

int PlayerWidget::playerState() const
{
    return m_state;
}


void PlayerWidget::btnSeekForward()
{
    pMainWindow->playerSeekToTime( qMin( m_totalTime, m_currentTime + 5000 ) );
}

void PlayerWidget::btnSeekBackward()
{
    pMainWindow->playerSeekToTime( qMax( 0, m_currentTime - 5000 ) );
}


void PlayerWidget::updatePlayerState( int newstate )
{
    m_state = newstate;
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
        qint64 remaining = qMax( (qint64) 0, m_totalTime - m_currentTime );
        lblTimeCur->setText( tr("<b>%1</b>") .arg( tickToString( m_currentTime ) ) );
		lblTimeRemaining->setText( tr("<b>-%1</b>") .arg( tickToString( remaining ) ) );
	}
}

void PlayerWidget::setCurrentPosition(qint64 time)
{
    m_currentTime = time;

    qint64 remaining = qMax( (qint64) 0, m_totalTime - m_currentTime );
    lblTimeCur->setText( tr("<b>%1</b>") .arg( tickToString( m_currentTime ) ) );
    lblTimeRemaining->setText( tr("<b>-%1</b>") .arg( tickToString( remaining ) ) );

    if ( !m_sliderDown && m_totalTime > 0 )
    {
        int value = m_currentTime * seekSlider->maximum() / m_totalTime;

        if ( seekSlider->value() != value )
        {
            m_changingSliderPosition = true;
            seekSlider->setValue( value );
        }
    }
}

void PlayerWidget::setDuration(qint64 duration)
{
    m_totalTime = duration;
    setCurrentPosition( m_currentTime );
}


void PlayerWidget::seekSliderUp()
{
    pMainWindow->playerSeekToTime( seekSlider->value() * m_totalTime / seekSlider->maximum() );
	m_sliderDown = false;
}

void PlayerWidget::seekSliderDown()
{
	m_sliderDown = true;
}

void PlayerWidget::seekSliderMoved( int newvalue )
{
    if ( !m_changingSliderPosition )
        pMainWindow->playerSeekToTime( newvalue * m_totalTime / seekSlider->maximum() );
    else
        m_changingSliderPosition = false;
}

