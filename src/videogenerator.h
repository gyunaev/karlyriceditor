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

#ifndef VIDEOGENERATOR_H
#define VIDEOGENERATOR_H

#include <QDialog>
#include <QElapsedTimer>
#include <QTimer>

#include "lyrics.h"
#include "project.h"
#include "mediaplayer.h"

#include "ui_dialog_encodingprogress.h"

class MediaPlayer;
class PlayerWidget;

class VideoGenerator : public QDialog
{
    Q_OBJECT

	public:
        VideoGenerator( Project * prj, const Lyrics &lyrics, qint64 totaltime );
        void generate(PlayerWidget *widget);

    public slots:
        void    mediaLoadingFinished( MediaPlayer::State newstate, QString error );
        void    mediaFinished();

        void    buttonAbort();
        void    updateProgress();

    protected:
        void    closeEvent(QCloseEvent *e);

	private:
		Project *	m_project;
        const Lyrics&  m_lyrics;
        MediaPlayer * m_encoder;

        Ui::DialogEncodingProgress  mProgress;

        // Progress info
        QTimer          m_progressUpdateTimer;
        QElapsedTimer   m_totalRenderTime;
        QElapsedTimer   m_lastImageUpdate;

        mutable QMutex  mProgressMutex;
        QImage          m_lastRenderedImage;
        unsigned int    m_processedFrames;
        qint64          m_totalTime;
        qint64          m_currentTime;
};

#endif // VIDEOGENERATOR_H
