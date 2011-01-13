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

	clear();
}

void TestWindow::showEvent( QShowEvent * )
{
	clear();
}

void TestWindow::clear()
{
	label->setText("");
	m_lastUpdate = -1;
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

	m_lyrics = lyrics;
	m_renderingLyrics = true;

	clear();
	update();
}

void TestWindow::setCDGdata( const QByteArray& cdgdata )
{
	m_cdgrenderer.setCDGdata( cdgdata );
	m_renderingLyrics = false;
	resize( m_pixsize );

	label->setText("");
	update();
}

QString TestWindow::getLyrics( qint64 tickmark )
{
	QString block;
	int pos;
	qint64 nexttime;

	if ( !m_lyrics.blockForTime( tickmark, block, pos, nexttime ) )
	{
		// Nothing active to show, so we show the following:
		// - If there is a block within next five seconds, show it.
		// - Otherwise show a blank screen
		if ( m_lyrics.nextBlock( tickmark, nexttime, block ) && nexttime - tickmark <= 5000 )
		{
			block = Qt::escape( block );
			block.replace( "\n", "<br>" );

			return QString("<qt><font color=\"%1\">%2</font></qt>")
							.arg( pSettings->m_previewTextActive.name() ) .arg(block);
		}

		return QString();
	}

	QString inactive = Qt::escape( block.left( pos ) );
	QString active = Qt::escape( block.mid( pos ) );

	inactive.replace( "\n", "<br>" );
	active.replace( "\n", "<br>" );

	block = QString("<qt><font color=\"%1\">%2</font><font color=\"%3\">%4</font></qt>")
				.arg( pSettings->m_previewTextInactive.name() )
				.arg( inactive )
				.arg( pSettings->m_previewTextActive.name() )
				.arg( active );

	return block;
}

void TestWindow::tick( qint64 tickmark )
{
	if ( isHidden() )
		return;

	if ( m_renderingLyrics )
	{
		QString block = getLyrics( tickmark );

		// If there is nothing to show but the last block was updated less than five seconds ago, keep it.
		if ( block.isEmpty() && time(0) - m_lastUpdate < 5 )
			return;

		if ( block == label->text() )
			return;

		//qDebug("block at %d: %s", (int) tickmark, qPrintable(block) );

		m_lastUpdate = time(0);
		label->setText( block );
	}
	else
	{
		bool updated = false;

		QImage image = m_cdgrenderer.update( tickmark, &updated );

		if ( updated )
			label->setPixmap( QPixmap::fromImage( image.scaled( m_pixsize ) ) );
	}
}
