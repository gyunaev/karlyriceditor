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

#include <QColorDialog>

#include "colorbutton.h"

ColorButton::ColorButton( QWidget * parent )
	: QPushButton( parent )
{
	connect( this, SIGNAL(clicked()), this, SLOT(btnClicked()) );
}

void ColorButton::setColor( const QColor& color )
{
	// After the constructor is called, UIC-generated code calls setLabel, so we overwrite it here
	setText( tr("Select") );

	// Background color
	QPalette pal = palette();
	pal.setColor( QPalette::Button, color );
	setPalette( pal );
	update();
}

QColor ColorButton::color() const
{
	return palette().color( QPalette::Button );
}

void ColorButton::btnClicked()
{
	QColor newcolor = QColorDialog::getColor( color() );

	if ( newcolor.isValid() )
		setColor( newcolor );
}
