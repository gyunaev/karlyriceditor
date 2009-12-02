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

#ifndef TESTWINDOW_H
#define TESTWINDOW_H

#include <QDialog>
#include "ui_testwindow.h"
#include "lyrics.h"


class TestWindow : public QDialog, public Ui::TestWindow
{
    Q_OBJECT

	public:
		TestWindow(QWidget *parent = 0);
		void setLyrics( const Lyrics& lyrics );

	public slots:
		void	tick( qint64 tickmark );

	private:
		// Redraw the label using block or line mode
		void	redrawBlocks( qint64 tickmark );
		void	redrawLines( qint64 tickmark );
		void	setText( const QString& text );
		int		findBlockToShow( qint64 tickmark );
		void	splitSyllable( int index );

		typedef struct
		{
			qint64	timestart;
			QString	text;		// converted to HTML; includes <br> on line ends
			int		blockindex;

		} LyricIndex;

		typedef struct
		{
			qint64	timestart;	// for block
			qint64	timeend;	// for block
			int		index;		// in LyricIndex

		} Time;

		QVector<LyricIndex>	m_lyricIndex;

		// This one is used in block mode (LRC2 and UltraStar)
		QVector<Time>		m_blockIndex;

		QString	m_labelText; // to prevent unnecessary updates
};


#endif // TESTWINDOW_H
