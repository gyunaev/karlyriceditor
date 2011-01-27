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

#include <time.h>
#include <QTextDocument>
#include <QResizeEvent>

#include "testwindow.h"
#include "settings.h"
#include "textrenderer.h"
#include "cdgrenderer.h"

TestWindow::TestWindow( QWidget *parent )
	: QDialog(parent), Ui::TestWindow()
{
	setupUi( this );

	// Background color
	QPalette pal = palette();
	pal.setColor( QPalette::Window, pSettings->m_previewBackground );
	setPalette( pal );

	m_renderer = 0;

	clear();
}

void TestWindow::showEvent( QShowEvent * )
{
	clear();
}

void TestWindow::hideEvent( QShowEvent * )
{
	reset();
}

void TestWindow::clear()
{
	label->setText( "" );
}

void TestWindow::reset()
{
	delete m_renderer;
	m_renderer = 0;

	clear();
}

void TestWindow::setLyrics( const Lyrics& lyrics, const QString& artist, const QString& title )
{
	reset();

	TextRenderer * re = new TextRenderer( 100, 100 );
	re->setData( lyrics );
	re->setPreambleData( 5, 5000, 10 );

	if ( !artist.isEmpty() && !title.isEmpty() )
		re->setTitlePageData( artist, title, 5000 );

	m_renderer = re;
}

void TestWindow::setCDGdata( const QByteArray& cdgdata )
{
	reset();

	CDGRenderer * re = new CDGRenderer();
	re->setCDGdata( cdgdata );
	m_renderer = re;

	const QImage& image = m_renderer->image();
	setMinimumSize( image.size() );
	setMaximumSize( image.size() );
	resize( image.size() );
}

void TestWindow::tick( qint64 tickmark )
{
	if ( isHidden() || !m_renderer )
		return;

	int status = m_renderer->update( tickmark );

	if ( status == LyricsRenderer::UPDATE_NOCHANGE )
		return;

	const QImage& image = m_renderer->image();

	if ( status == LyricsRenderer::UPDATE_RESIZED )
	{
		if ( image.width() < width() || image.height() < height() )
		{
			setMinimumSize( image.size() );
			setMaximumSize( image.size() );
			resize( image.size() );
		}
	}

	//label->setPixmap( QPixmap::fromImage( image.scaled( label->size() ) ) );
	label->setPixmap( QPixmap::fromImage( image ) );
}
