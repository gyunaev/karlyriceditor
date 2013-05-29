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

#ifndef UTIL_H
#define UTIL_H

namespace Util
{

// Parse the QByteArray; if the text is ASCII, return the string as-is. Otherwise pop up a dialog asking the user
// for encoding, and apply the specified encoding, modifying the string.
// Returned an empty string if encoding wasn't guessed and the user didn't choose any
QString convertWithUserEncoding( const QByteArray& data );

// Removes the file extention (but keeps the dot)
QString removeFileExtention( const QString& filename );


}

#endif // UTIL_H
