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

#ifndef DIALOG_EDITTIMEMARK_H
#define DIALOG_EDITTIMEMARK_H

#include "ui_dialog_edittimemark.h"

class DialogEditTimeMark : public QDialog, public Ui::TimeMarkEdit
{
	Q_OBJECT

	public:
		DialogEditTimeMark( bool has_pitch, QWidget * parent = 0 );

		void	setPitch( int pitch );
		void	setTimemark( qint64 timemark );

		int		pitch() const;
		qint64 	timemark() const;

	private slots:
		void	pitchValueChanged( int newpitch );
};


#endif // DIALOG_EDITTIMEMARK_H
