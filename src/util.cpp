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

#include <QString>
#include "util.h"
#include "dialog_selectencoding.h"

namespace Util
{

QString convertWithUserEncoding( const QByteArray& data )
{
	// Before we ask the user for the text encoding, run a loop - if all characters there are < 127, this is ASCII,
	// and we do not need to ask.
	for ( int i = 0; i < data.size(); i++ )
	{
		if ( data.at(i) < 0 )
		{
			// This is not ASCII. Ask the text encoding
			DialogSelectEncoding dlg( data );

			if ( dlg.exec() == QDialog::Rejected )
				return QString();

			return dlg.codec()->toUnicode( data );
			break;
		}
	}

	return QString::fromLatin1( data );
}


QString removeFileExtention( const QString& filename )
{
	int extdot = filename.lastIndexOf( '.' );

	if ( extdot == -1 )
		return filename;

	return filename.mid( 0, extdot + 1 );
}


}; // namespace
