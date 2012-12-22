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

#ifndef LYRICSWIDGET_H
#define LYRICSWIDGET_H

#include <QWidget>

#include "lyrics.h"
#include "cdgrenderer.h"
#include "lyricsrenderer.h"

class LyricsWidget : public QWidget
{
	Q_OBJECT

	public:
		LyricsWidget( QWidget * parent );
		~LyricsWidget();

		// For lyrics
		void	setLyrics( const Lyrics& lyrics, const QString& artist = "", const QString& title = "" );

		// For CD+G
		void	setCDGdata( const QByteArray& cdgdata );

	public slots:
		void	updateLyrics( qint64 tickmark );

	protected:
		void	paintEvent( QPaintEvent * );
		QSize	sizeHint () const;
		QSizePolicy	sizePolicy () const;
		QSize	minimumSizeHint() const;

	private:
		LyricsRenderer	* m_renderer;
		QImage			  m_lastImage;
};


#endif // LYRICSWIDGET_H
