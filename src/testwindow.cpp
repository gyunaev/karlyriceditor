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

#include <QSettings>

#include "testwindow.h"
#include "lyricswidget.h"
#include "mediaplayer.h"
#include "playerwidget.h"

TestWindow::TestWindow( QWidget *parent )
	: QDialog(parent), Ui::DialogTestWindow()
{
	setupUi( this );

	m_widget = 0;
    m_lastTick = 0;
	m_layout = new QVBoxLayout( frame );

    progressBar->setValue( 0 );

    connect( btnLocateTimemark, SIGNAL(clicked()), this, SLOT(locateButtonClicked()) );
}

void TestWindow::setLyricWidget( LyricsWidget * lw )
{
	// Clear the layout
	QWidget *child = (QWidget*) m_layout->takeAt(0);
	delete child;

	m_widget = lw;
	m_layout->addWidget( lw );
	update();
}

void TestWindow::closeEvent( QCloseEvent * event )
{
	emit closed();

	QDialog::closeEvent( event );
}

void TestWindow::tick( qint64 current, qint64 total )
{
    m_widget->updateLyrics( current );

    qint64 reminder = total - current;

    lblCurrent->setText( PlayerWidget::tickToString( current ) );
	lblTotal->setText( PlayerWidget::tickToString( reminder ) );

    int pval = current * progressBar->maximum() / total;

	if ( progressBar->value() != pval )
        progressBar->setValue( pval );

    m_lastTick = current;
}

void TestWindow::locateButtonClicked()
{
    emit editorTick( m_lastTick );
}
