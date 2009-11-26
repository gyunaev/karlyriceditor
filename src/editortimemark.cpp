/***************************************************************************
 *   Copyright (C) 2009 George Yunaev, gyunaev@ulduzsoft.com               *
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

#include <QWidget>
#include <QPainter>

#include "editortimemark.h"
#include "settings.h"

// constants
static const int X_SPACE = 3;
static const int Y_BOTTOM = 3;


EditorTimeMark::EditorTimeMark()
	: QObject(), QTextObjectInterface()
{
	m_timing = -1;
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
	qint64 timing = qVariantValue<qint64>( format.property( TimeProperty ) );

	if ( timing == m_timing )
		return;

	m_timing = timing;

	QString mark;
	QColor bgcolor;

	if ( timing > 0 )
	{
		int minute = timing / 60000;
		int second = (timing - minute * 60000) / 1000;
		int msecond = timing - (minute * 60000 + second * 1000 );

		mark.sprintf( "%02d:%02d.%d", minute, second, msecond / 10 );
		m_tooltip = tr("Time: %1") .arg(mark);
		bgcolor = pSettings->m_timeMarkBackground;
	}
	else
	{
		mark = "<->"; // a placeholder
		m_tooltip = tr("Placeholder for timing mark");
		bgcolor = pSettings->m_timeMarkHolderBackground;
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
	painter.setPen( pSettings->m_timeMarkText );

	painter.drawRect( drawarea );
	textarea = painter.boundingRect( textarea, Qt::AlignLeft | Qt::AlignTop, mark );
	painter.drawText( textarea, Qt::AlignLeft | Qt::AlignTop, mark );
	painter.end();
}
