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

#include <QWidget>
#include <QPainter>

#include "editortimemark.h"
#include "settings.h"
#include "lyrics.h"

// constants
static const int X_SPACE = 3;
static const int Y_BOTTOM = 3;


EditorTimeMark::EditorTimeMark()
	: QObject(), QTextObjectInterface()
{
	m_timing = -1;
	m_pitch = -1;
}

QSizeF EditorTimeMark::intrinsicSize( QTextDocument *, int, const QTextFormat& format )
{
	updatePixmapIfNecessary( format );
	return m_pixmap.size();
}

void EditorTimeMark::drawObject( QPainter *painter, const QRectF &rect, QTextDocument *,
								 int, const QTextFormat& format )
{
	updatePixmapIfNecessary( format );
	painter->drawPixmap( rect, m_pixmap, m_pixmap.rect() );
}

void EditorTimeMark::updatePixmapIfNecessary( const QTextFormat &format )
{
	qint64 timing = format.property( TimeProperty ).toLongLong();
	int pitch = format.property( PitchProperty ).toInt();

	if ( timing == m_timing && pitch == m_pitch )
		return;

	m_timing = timing;
	m_pitch = pitch;

	QString mark;
	QColor bgcolor, fgcolor;

	if ( timing > 0 )
	{
		int minute = timing / 60000;
		int second = (timing - minute * 60000) / 1000;
		int msecond = timing - (minute * 60000 + second * 1000 );

		mark.sprintf( "%02d:%02d.%d", minute, second, msecond / 10 );
		bgcolor = pSettings->m_timeMarkTimeBackground;
		fgcolor = pSettings->m_timeMarkTimeText;

		if ( pitch != -1 && pSettings->m_timeMarkShowPitch )
		{
			mark += " " + Lyrics::pitchToNote( pitch, false );
			bgcolor = pSettings->m_timeMarkPitchBackground;
		}
	}
	else
	{
		mark = "<->"; // a placeholder
		bgcolor = pSettings->m_timeMarkPlaceholderBackground;
		fgcolor = pSettings->m_timeMarkPlaceholderText;
	}

	// Calculate what is the size of image we need to fit the text
	QFont font( pSettings->m_timeMarkFontFamily );
	font.setPointSize( pSettings->m_timeMarkFontSize );
	QFontMetrics fm( font );

	m_pixmap = QPixmap( QSize( fm.width( mark ) + X_SPACE * 2, fm.height() ) );
	m_pixmap.fill( bgcolor );

	QRect drawarea( 1, 1, m_pixmap.width() - 2, m_pixmap.height() - 2 );
	QRect textarea( drawarea.x() + X_SPACE, drawarea.y(),
					drawarea.width() - X_SPACE, drawarea.height() );

	QPainter painter;
	painter.begin( &m_pixmap );
	painter.setFont( font );
	painter.setPen( fgcolor );

	painter.drawRect( drawarea );
	textarea = painter.boundingRect( textarea, Qt::AlignLeft | Qt::AlignTop, mark );
	painter.drawText( textarea, Qt::AlignLeft | Qt::AlignTop, mark );
	painter.end();
}
