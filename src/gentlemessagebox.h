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

#ifndef GENTLEMESSAGEBOX_H
#define GENTLEMESSAGEBOX_H

#include <QDialog>
#include <QMessageBox>
#include "ui_gentlemessagebox.h"


// Appends a "Do not show this message again" checkbox to the message box,
// and does not show it if this checkbox has been checked before.
class GentleMessageBox : public QDialog, public Ui::GentleMessageBox
{
	Q_OBJECT

	public:
		static void warning ( QWidget * parent, const QString & setting, const QString & title, const QString & text );

	private:
		GentleMessageBox( QWidget * parent,
						  QMessageBox::Icon icon,
						  const QString & title,
						  const QString & text );
};

#endif // GENTLEMESSAGEBOX_H
