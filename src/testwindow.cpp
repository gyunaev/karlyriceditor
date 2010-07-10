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

#include <QTextDocument>

#include "testwindow.h"
#include "settings.h"

TestWindow::TestWindow( QWidget *parent )
	: QDialog(parent),
		Ui::TestWindow(),
		m_pixsize( 2*CDG_FULL_WIDTH, 2*CDG_FULL_HEIGHT )
{
	setupUi( this );

	// Background color
	QPalette pal = palette();
	pal.setColor( QPalette::Window, pSettings->m_previewBackground );
	setPalette( pal );

	label->setText("");
}

void TestWindow::setLyrics( const Lyrics& lyrics )
{
	// Font
	QFont font( pSettings->m_previewFontFamily, pSettings->m_previewFontSize );
	setFont( font );

	// Same for the text label
	QPalette pal = label->palette();
	pal.setColor( QPalette::Window, pSettings->m_previewBackground );
	label->setPalette( pal );
	label->setFont( font );

	m_lyricrenderer.setLyrics( lyrics );
	m_lyricrenderer.setColors( pSettings->m_previewTextActive.name(), pSettings->m_previewTextInactive.name() );
	m_renderingLyrics = true;
}

void TestWindow::setTitleData( const QString& titledata )
{
	m_lyricrenderer.setTitlePage( titledata );
}

void TestWindow::setCDGdata( const QByteArray& cdgdata )
{
	m_cdgrenderer.setCDGdata( cdgdata );
	m_renderingLyrics = false;
	resize( m_pixsize );
}

void TestWindow::tick( qint64 tickmark )
{
	if ( isHidden() )
		return;

	if ( m_renderingLyrics )
	{
		QString text = m_lyricrenderer.update( tickmark );

		if ( text == label->text() )
			return;

		label->setText( text );
	}
	else
	{
		bool updated = false;

		QImage image = m_cdgrenderer.update( tickmark, &updated );

		if ( updated )
			label->setPixmap( QPixmap::fromImage( image.scaled( m_pixsize ) ) );
	}
}
