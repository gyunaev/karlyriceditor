/**************************************************************************
 *  Karlyriceditor - a lyrics editor for Karaoke songs                    *
 *  Copyright (C) 2009 George Yunaev, support@karlyriceditor.com          *
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

#include <phonon/seekslider.h>
#include <phonon/volumeslider.h>

#include "playerwidget.h"
#include "mainwindow.h"
#include "project.h"


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

	// Create the Phonon player objects
	m_mediaObject = new Phonon::MediaObject(this);
	m_mediaAudioOutput = new Phonon::AudioOutput( Phonon::MusicCategory, this );
	Phonon::createPath( m_mediaObject, m_mediaAudioOutput );

	// Tick each 100ms should be fine enough
	m_mediaObject->setTickInterval( 100 );

	// Connect Phonon slots
	connect( m_mediaObject,
			 SIGNAL( stateChanged( Phonon::State, Phonon::State ) ),
			 this,
			 SLOT( phonon_StateChanged(Phonon::State,Phonon::State)) );

	connect( m_mediaObject,
			 SIGNAL( tick(qint64) ),
			 this,
			 SLOT( phonon_Tick(qint64)) );

	// Connect seek slider to Phonon
	seekSlider->setMediaObject( m_mediaObject );
	seekSlider->setOrientation(Qt::Horizontal);

	// Volume control
	volumeSlider->setAudioOutput( m_mediaAudioOutput );
	volumeSlider->setOrientation( Qt::Horizontal );
}

PlayerWidget::~PlayerWidget()
{
	delete m_mediaObject;
//	delete m_mediaAudioOutput;
}

void PlayerWidget::phonon_StateChanged ( Phonon::State newstate, Phonon::State )
{
	bool enable_playpause = false, enable_seek = false, enable_stop = false;

	switch ( newstate )
	{
		case Phonon::LoadingState:
			m_ready = false;

			// all buttons are disabled
			break;

		case Phonon::ErrorState:
			m_ready = false;

			// Show error message
			QMessageBox::critical( 0,
							   tr("Cannot play media file"),
							   tr("Cannot play media file %1: %2")
									.arg( m_mediaObject->currentSource().fileName() )
									.arg( m_mediaObject->errorType() ) );

			// all buttons are disabled
			break;

		case Phonon::PausedState:
			btnPausePlay->setPixmap( QPixmap( ":images/dryicons_play.png" ) );
			enable_playpause = true;
			enable_stop = true;
			break;

		case Phonon::StoppedState:
			btnPausePlay->setPixmap( QPixmap( ":images/dryicons_play.png" ) );
			m_ready = true;
			enable_playpause = true;
			break;

		case Phonon::PlayingState:
		case Phonon::BufferingState:
			btnPausePlay->setPixmap( QPixmap( ":images/dryicons_pause.png" ) );
			enable_seek = true;
			enable_playpause = true;
			enable_stop = true;
			break;
	}

	btnRew->setEnabled( enable_seek );
	btnFwd->setEnabled( enable_seek );
	btnPausePlay->setEnabled( enable_playpause );
	btnStop->setEnabled( enable_stop );

	if ( !m_ready )
	{
		lblTimeCur->setText( "---" );
		lblTimeRemaining->setText( "---" );
	}
	else
		phonon_Tick( 0 );

	pMainWindow->updateState();
}

void PlayerWidget::phonon_Tick( qint64 )
{
	emit tick( currentTime() );

	lblTimeCur->setText( tr("<b>%1</b>") .arg( tickToString( currentTime() ) ) );
	lblTimeRemaining->setText( tr("<b>-%1</b>") .arg( tickToString( m_mediaObject->remainingTime() ) ) );
}

QString PlayerWidget::tickToString( qint64 tickvalue )
{
	int minute = tickvalue / 60000;
	int second = (tickvalue-minute*60000)/1000;
	int msecond = (tickvalue-minute*60000-second*1000) / 100; // round milliseconds to 0-9 range}

	QTime ct( 0, minute, second, msecond );
	return ct.toString( "mm:ss.z" );
}

void PlayerWidget::setMusicFile( Project * project )
{
	setWindowTitle( tr("Player controls - %1 by %2")
					.arg( project->tag( Project::Tag_Title ) )
					.arg( project->tag( Project::Tag_Artist ) ) );

	m_ready = false;
	m_mediaObject->setCurrentSource( project->musicFile() );
}

void PlayerWidget::btn_playerStop()
{
	m_mediaObject->stop();
}

void PlayerWidget::btn_playerPlayPause()
{
	if ( m_mediaObject->state() == Phonon::PlayingState )
		m_mediaObject->pause();
	else
		m_mediaObject->play();
}

void PlayerWidget::btn_playerSeekForward()
{
	m_mediaObject->seek( m_mediaObject->currentTime() + 5000 );
}

void PlayerWidget::btn_playerSeekBackward()
{
	m_mediaObject->seek( m_mediaObject->currentTime() - 5000 );
}

qint64 PlayerWidget::currentTime() const
{
	return m_mediaObject->currentTime();
}
