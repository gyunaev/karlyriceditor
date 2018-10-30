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

#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <QDockWidget>
#include <QTimer>

#include "playerbutton.h"
#include "ui_playerwidget.h"

class MediaPlayer;
class Project;

class PlayerWidget : public QDockWidget, public Ui::PlayerWidget
{
    Q_OBJECT

	public:
		PlayerWidget(QWidget *parent = 0);
		~PlayerWidget();

		// Sets the current music file from a project.
        void openMusicFile( Project * project );

		// Is music file ready to play?
		bool	isReady() const { return m_ready; }

		// Is playing?
		bool	isPlaying() const;

		qint64	currentTime() const;
		qint64	totalTime() const;

		static QString tickToString( qint64 tick );

	signals:
        void	tick( qint64 position, qint64 duration );

	public slots:
		void	startPlaying();
		void	btn_playerStop();
		void	btn_playerPlayPause();
		void	btn_playerSeekForward();
		void	btn_playerSeekBackward();
        void	seekToTime( qint64 time );

	private slots:
        void	updateTimer();
		void	seekSliderMoved( int newvalue );
		void	seekSliderUp();
		void	seekSliderDown();

        void    mediaLoaded();
        void    mediaError( QString text );
        void    mediaFinished();
        void    mediaDurationAvailable();

	private:
		void	updatePlayerState( int state );

	private:
		bool			m_ready;
		bool			m_sliderDown; // do not update position

        // Timer to update the play status
        QTimer          mUpdateTimer;

        // Our audio player
        MediaPlayer  *  mAudioPlayer;

        // Song duration, when known
        qint64          mDuration;

        // Current slider value (to detect changes by user)
        int             mCurrentSliderValue;
};

#endif // PLAYERWIDGET_H
