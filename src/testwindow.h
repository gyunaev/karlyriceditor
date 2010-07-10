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

#ifndef TESTWINDOW_H
#define TESTWINDOW_H

#include <QDialog>
#include <QString>

#include "ui_testwindow.h"
#include "lyrics.h"
#include "lyricsrenderer.h"
#include "cdgrenderer.h"


class TestWindow : public QDialog, public Ui::TestWindow
{
    Q_OBJECT

	public:
		TestWindow(QWidget *parent = 0);

		// For lyrics
		void	setLyrics( const Lyrics& lyrics );
		void	setTitleData( const QString& titledata );

		// For CD+G
		void	setCDGdata( const QByteArray& cdgdata );

	public slots:
		void	tick( qint64 tickmark );

	private:
		// For rendering text-based lyrics
		bool			m_renderingLyrics;		// false if CD+G
		LyricsRenderer	m_lyricrenderer;		// To render lyrics

		// For rendering CD+G lyrics
		CDGRenderer		m_cdgrenderer;			// To render cd+g
		QSize			m_pixsize;				// output pixmap size
};


#endif // TESTWINDOW_H
