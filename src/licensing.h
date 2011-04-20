/**************************************************************************
 *  Karlyriceditor - a lyrics editor and CD+G / video export for Karaoke  *
 *  songs.                                                                *
 *  Copyright (C) 2009-2011 George Yunaev, support@karlyriceditor.com     *
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

#ifndef LICENSING_H
#define LICENSING_H

class LicensingPrivate;

//
// A simple licensing class based on X509 certificates.
//
// The application author must create the CA and store the CA public key in the cpp file as well
// as the CA issuer common name (CN). Then the author may generate certs for users (private keys
// could be discarded), sign them using the CA private key, and provide the certificate content
// as license key.
//
class Licensing
{
	public:
		Licensing();
		virtual ~Licensing();

		bool	init();
		bool	validate( const QString& cert );
		QString errMsg() const;
		bool	isValid() const;

		// Those return the licensing information; should only be called
		// if validate() returned true.
		QString		subject() const;
		QDate		expires() const;

	private:
		LicensingPrivate * d;
};

#endif // LICENSING_H
