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

#include <QMessageBox>
#include "dialog_timeadjustment.h"
#include "editor.h"

DialogTimeAdjustment::DialogTimeAdjustment(QWidget *parent) :
	QDialog(parent)
{
	setupUi( this );
	m_valueAdd = 0;
	m_valueMultiply = 1.0;

	connect( leAdd, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)) );
	connect( leMultiply, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)) );
	connect( leTestIn, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)) );

	leTestIn->setText( "0:20.11" );
	leAdd->setText( QString::number( m_valueAdd ) );
	leMultiply->setText( QString::number( m_valueMultiply ) );
}

void DialogTimeAdjustment::textChanged ( const QString & )
{
	if ( !getAndValidate( false ) )
		return;

	qint64 timing = timeToMark( leTestIn->text() );
	timing *= m_valueMultiply;
	timing += m_valueAdd;

	leTestOut->setText( markToTime( timing ));
}

void DialogTimeAdjustment::accept()
{
	if ( !getAndValidate( true ) )
		return;

	QDialog::accept();
}


bool DialogTimeAdjustment::getAndValidate( bool msgboxIfError )
{
	bool ok;
	QString errtxt;

	m_valueAdd = leAdd->text().toLongLong( &ok );

	if ( ok )
	{
		m_valueMultiply = leMultiply->text().toDouble( &ok );

		if ( !ok )
			errtxt = "Specified Multiply value is not valid";
	}
	else
		errtxt = "Specified Add value is not valid";

	if ( ok )
	{
		txtError->setText("");
		return true;
	}

	if ( msgboxIfError )
		QMessageBox::critical( 0, "Invalid value", errtxt );

	txtError->setText( "<font color='red'>" + errtxt + "</font>" );
	return false;
}
