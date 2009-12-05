/**************************************************************************
 *  Karlyriceditor - a lyrics editor for Karaoke songs                    *
 *  Copyright (C) 2009 George Yunaev, support@karlyriceditor.com          *
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

#include <QStringList>

#include "phononhelper.h"

PhononHelper::PhononHelper()
{
}


QString PhononHelper::getTag( Phonon::MediaObject * phonon, Phonon::MetaData tag )
{
	QStringList data = phonon->metaData( tag );
	QString text;

	if ( data.isEmpty() )
		return QString();

	for ( int i = 0; i < data.size(); i++ )
	{
		// Phonon seems to return non-English tags in UTF8 as QString,
		// i.e. done by QString( tag ), not QString::fromUtf8( tag ).
		// Therefore heuristics tries to detect whether it is UTF8, and converts it.
		QString linedata = data[i];
		QByteArray encodedstring;
		bool found_128_255 = false;

		for ( int k = 0; k < linedata.size(); k++ )
		{
			unsigned short unichar = linedata[k].unicode();

			if ( unichar > 256 )
			{
				encodedstring.clear();
				break;
			}
			else
			{
				encodedstring.push_back( (char) unichar );

				if ( unichar > 127 )
					found_128_255 = true;
			}
		}

		if ( !encodedstring.isEmpty() && found_128_255 && !QString::fromUtf8( encodedstring ).isEmpty() )
		{
			qDebug("Phonon hack: converting double-converted tag back to UTF8");
			linedata = QString::fromUtf8( encodedstring );
		}

		if ( !text.isEmpty() )
			text += ", ";

		text += linedata;
	}

	return text;
}
