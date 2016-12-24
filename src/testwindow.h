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

#ifndef TESTWINDOW_H
#define TESTWINDOW_H

#include <QDialog>
#include "ui_dialog_testwindow.h"

class LyricsWidget;


class TestWindow : public QDialog, public Ui::DialogTestWindow
{
    Q_OBJECT

	public:
		TestWindow( QWidget *parent = 0 );

        void    setLyricWidget( LyricsWidget * lw );

	signals:
        void    closed();
        void    editorTick( qint64 tick );

	public slots:
        void    tick( qint64 value );
        void    locateButtonClicked();

	protected:
		void	closeEvent( QCloseEvent * event );

	private:
        QVBoxLayout * m_layout;
		LyricsWidget* m_widget;
        qint64        m_lastTick;
};


#endif // TESTWINDOW_H
