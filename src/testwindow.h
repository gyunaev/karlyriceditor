/**************************************************************************
 *  Karlyriceditor - a lyrics editor and CD+G / video export for Karaoke  *
 *  songs.                                                                *
 *  Copyright (C) 2009-2011 George Yunaev, support@karlyriceditor.com     *
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
#include <QVBoxLayout>

class LyricsWidget;


class TestWindow : public QDialog
{
    Q_OBJECT

	public:
		TestWindow( QWidget *parent = 0 );

		void setLyricWidget( LyricsWidget * lw );

	signals:
		void closed();

	protected:
		void	closeEvent( QCloseEvent * event );

	private:
		QVBoxLayout * m_layout;
};


#endif // TESTWINDOW_H
