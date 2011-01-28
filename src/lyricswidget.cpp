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

#include <QPainter>

#include "lyricswidget.h"
#include "testwindow.h"
#include "settings.h"
#include "textrenderer.h"
#include "cdgrenderer.h"

static const unsigned int PADDING_X = 10;
static const unsigned int PADDING_Y = 8;

LyricsWidget::LyricsWidget( QWidget *parent )
	: QWidget(parent)
{
	m_renderer = 0;
	m_lastImage = QImage( 100, 100, QImage::Format_ARGB32 );
}

LyricsWidget::~LyricsWidget()
{
	delete m_renderer;
}

QSize LyricsWidget::sizeHint () const
{
	return minimumSizeHint();
}

QSize LyricsWidget::minimumSizeHint() const
{
	return QSize( m_lastImage.width() + 2 * PADDING_X, m_lastImage.height() + 2 * PADDING_Y );
}

void LyricsWidget::paintEvent( QPaintEvent * )
{
	QPainter p( this );

	p.fillRect( QRect( 0, 0, width() - 1, height() - 1 ), Qt::black );

	int x = (width() - m_lastImage.width()) / 2;
	int y = (height() - m_lastImage.height() ) / 2;

	p.drawImage( x, y, m_lastImage );
}

void LyricsWidget::setLyrics( const Lyrics& lyrics, const QString& artist, const QString& title )
{
	TextRenderer * re = new TextRenderer( 100, 100 );
	re->setData( lyrics );
	re->setPreambleData( 5, 5000, 10 );

	if ( !artist.isEmpty() && !title.isEmpty() )
		re->setTitlePageData( artist, title, 5000 );

	//re->setVideoFile( "/home/tim/work/my/karlyriceditor/test/wffc_2004_canopy_accident.MPG" );
	m_renderer = re;

	updateGeometry();
	update();
}

void LyricsWidget::setCDGdata( const QByteArray& cdgdata )
{
	CDGRenderer * re = new CDGRenderer();
	re->setCDGdata( cdgdata );
	m_renderer = re;

	updateGeometry();
	update();
}

void LyricsWidget::updateLyrics( qint64 tickmark )
{
	if ( isHidden() || !m_renderer )
		return;

	int status = m_renderer->update( tickmark );

	if ( status == LyricsRenderer::UPDATE_NOCHANGE )
		return;

	m_lastImage = m_renderer->image();

	if ( status == LyricsRenderer::UPDATE_RESIZED )
		updateGeometry();

	update();
}
