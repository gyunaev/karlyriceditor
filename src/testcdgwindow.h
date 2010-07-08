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

#ifndef TESTCDGWINDOW_H
#define TESTCDGWINDOW_H

#include <QImage>
#include <QDialog>

#include "ui_testcdgwindow.h"
#include "cdg.h"

class TestCDGWindow : public QDialog, public Ui::TestCDGWindow
{
	Q_OBJECT

	public:
		TestCDGWindow( QWidget * parent = 0 );

		bool	setCDGdata( const QByteArray& cdgdata );

	public slots:
		void	tick( qint64 tickmark );

	private:
		void	cmdMemoryPreset( const char * data );
		void	cmdBorderPreset( const char * data );
		void	cmdLoadColorTable( const char * data, int index );
		void	cmdTileBlockXor( const char * data );

		unsigned int		m_packet;		// packet offset which hasn't been processed yet
		QVector< SubCode >	m_stream;		// CD+G stream
		QImage				m_cdgimage;		// rendered image
		qint64				m_lastupdate;	// last screen update time
		QSize				m_pixsize;		// output pixmap size
};

#endif // TESTCDGWINDOW_H
