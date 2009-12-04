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

#include <QGridLayout>
#include <QSettings>

#include "gentlemessagebox.h"


GentleMessageBox::GentleMessageBox( QWidget * parent, QMessageBox::Icon icon, const QString & title, const QString & text )
	: QDialog( parent ), Ui::GentleMessageBox()
{
	setupUi( this );

	// Copy-paste icon from original messagebox
	QMessageBox msgbox;
	msgbox.setIcon( icon );
	lblIcon->setPixmap( msgbox.iconPixmap() );

	setWindowTitle( title );
	lblText->setText( text );
}


void GentleMessageBox::warning ( QWidget * parent,
		const QString & setting, const QString & title, const QString & text )
{
	QSettings settings;
	QString valuename = "donotshowagain/" + setting;

	if ( settings.contains( valuename ) )
		return;

	GentleMessageBox gmbox( parent, QMessageBox::Warning, title, text );

	if ( gmbox.exec() == QDialog::Accepted )
	{
		if ( gmbox.cbNotAgain->isChecked() )
			settings.setValue( valuename, (int) 1 );
	}
}
