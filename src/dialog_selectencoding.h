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

#ifndef DIALOG_SELECTENCODING_H
#define DIALOG_SELECTENCODING_H

#include <QDialog>
#include <QString>
#include <QStringDecoder>

#include "ui_dialog_selectencoding.h"


class DialogSelectEncoding : public QDialog, public Ui::DialogSelectEncoding
{
    Q_OBJECT

	public:
		DialogSelectEncoding( const QByteArray& text, QWidget *parent = 0 );
        ~DialogSelectEncoding();
		QString encoding() const { return m_selectedEncoding; }
        QStringDecoder* codec() const { return m_selectedCodec; }

	protected slots:
		void	accept();
		void	encodingChanged( int index );

	private:
        QStringDecoder* m_selectedCodec;
		QString			m_selectedEncoding;
		QByteArray		m_encodedText;

};

#endif // DIALOG_SELECTENCODING_H
