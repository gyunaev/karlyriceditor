/***************************************************************************
 *   Copyright (C) 2009 Georgy Yunaev, gyunaev@ulduzsoft.com               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include <QDockWidget>

#include <phonon/mediaobject.h>
#include <phonon/audiooutput.h>

#include "playerbutton.h"
#include "ui_playerwidget.h"

class Project;

class PlayerWidget : public QDockWidget, public Ui::PlayerWidget
{
    Q_OBJECT

	public:
		PlayerWidget(QWidget *parent = 0);
		~PlayerWidget();

		// Sets the current music file from a project. This operation is asynchronous, everything is
		// handled in phonon_StateChanged() and propagated via MainWindow::updateState().
		void	setMusicFile( Project * project );

		// Is music file ready to play?
		bool	isReady() const { return m_ready; }

		qint64	currentTime() const;

	signals:
		void	tick( qint64 tickvalue );

	private slots:
		void	phonon_StateChanged ( Phonon::State newstate, Phonon::State oldstate );
		void	phonon_Tick( qint64 tickvalue );

		void	btn_playerStop();
		void	btn_playerPlayPause();
		void	btn_playerSeekForward();
		void	btn_playerSeekBackward();

	private:
		QString tickToString( qint64 tick );

	private:
		Phonon::MediaObject *	m_mediaObject;
		Phonon::AudioOutput *	m_mediaAudioOutput;

		bool					m_ready;
};

#endif // PLAYERWIDGET_H
