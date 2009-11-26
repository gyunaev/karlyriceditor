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

#include <QPainter>
#include <QBitmap>

#include "playerbutton.h"


PlayerButton::PlayerButton( QWidget *parent )
	: QPushButton( parent )
{
	m_inButton = false;
	setFocusPolicy(Qt::NoFocus);
}

QPoint PlayerButton::getAlignedCoords( const QPixmap& pix )
{
	QSize diff = (m_iconRaised.size() - pix.size()) / 2;
	return QPoint( diff.width(), diff.height() );
}

void PlayerButton::setPixmap( const QPixmap& icon )
{
	m_iconNormal = icon;

	// Create a larger icon, and a smaller icon
	m_iconRaised = m_iconNormal.scaled( m_iconNormal.size() * 1.1, Qt::KeepAspectRatio, Qt::SmoothTransformation );
	m_iconDown = m_iconNormal.scaled( m_iconNormal.size() * 0.9, Qt::KeepAspectRatio, Qt::SmoothTransformation );

	m_pointNormal = getAlignedCoords( m_iconNormal );
	m_pointDown = getAlignedCoords( m_iconDown );

	updateGeometry();
	update();
}

QSize PlayerButton::minimumSizeHint() const
{
	return sizeHint();
}

QSize PlayerButton::sizeHint() const
{
	return m_iconRaised.size();
}

QSizePolicy PlayerButton::sizePolicy () const
{
	return QSizePolicy ( QSizePolicy::Fixed, QSizePolicy::Fixed );
}

void PlayerButton::enterEvent(QEvent * e)
{
	m_inButton = true;
	update();

	QPushButton::enterEvent(e);
}

void PlayerButton::leaveEvent(QEvent * e)
{
	m_inButton = false;
	update();

	QPushButton::leaveEvent(e);
}

void PlayerButton::paintEvent( QPaintEvent *)
{
	QPainter painter (this);

	if ( m_inButton && isEnabled() )
	{
		if ( !isDown() )
			painter.drawPixmap( 0,0, m_iconRaised );
		else
			painter.drawPixmap( m_pointDown, m_iconDown );
	}
	else
		painter.drawPixmap( m_pointNormal, m_iconNormal );

	painter.end();
}
