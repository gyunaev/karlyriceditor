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

#include "lyrics.h"
#include "project.h"

#include "ui_dialog_encodingprogress.h"

class VideoGeneratorThread;

class VideoGenerator : public QDialog
{
    Q_OBJECT

	public:
		VideoGenerator( Project * prj );
		void generate( const Lyrics& lyrics, qint64 total_length );

    public slots:
        void    progress( int progress, QString frames, QString size, QString timing );
        void    finished( QString errormsg );
        void    buttonAbort();

    protected:
        void    closeEvent(QCloseEvent *e);

	private:
		Project *	m_project;

        VideoGeneratorThread * mVideoGeneratorThread;
        Ui::DialogEncodingProgress  mProgress;
};

#endif // VIDEOGENERATOR_H
