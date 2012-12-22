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

#ifndef DIALOG_TIMEADJUSTMENT_H
#define DIALOG_TIMEADJUSTMENT_H

#include <QDialog>
#include "ui_dialog_timeadjustment.h"

class DialogTimeAdjustment : public QDialog, Ui::DialogTimeAdjustment
{
	Q_OBJECT

	public:
		explicit DialogTimeAdjustment( QWidget *parent = 0 );

	private:
		bool	getAndValidate( bool msgboxIfError );

	protected slots:
		void	textChanged ( const QString & text );
		void	accept();

	public:
		qint64	m_valueAdd;
		double	m_valueMultiply;
};

#endif // DIALOG_TIMEADJUSTMENT_H
