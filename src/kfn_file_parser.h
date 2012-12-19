/**************************************************************************
 *  Karlyriceditor - a lyrics editor and CD+G / video export for Karaoke  *
 *  songs.                                                                *
 *  Copyright (C) 2009-2013 George Yunaev, support@karlyriceditor.com     *
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

#ifndef KFN_FILE_PARSER_H
#define KFN_FILE_PARSER_H

#include <QString>
#include <QFile>

// Parser for the KFN file format
class KFNFileParser
{
	public:
		// File types stored in the KFN file
		enum
		{
			TYPE_SONGTEXT = 1,
			TYPE_MUSIC = 2,
			TYPE_IMAGE = 3,
			TYPE_FONT = 4,
			TYPE_VIDEO = 5
		};

		// Directory entry
		class Entry
		{
			public:
				QString filename;	// the original file name in the original encoding
				int type;			// the file type; see TYPE_
				int length_in;		// the file length in the KFN file
				int length_out;		// the file lenght on disk; if the file is encrypted it is the same or smaller than length_in
				int offset;			// the file offset in the KFN file starting from the directory end
				int flags;			// the file flags; 0 means "not encrypted", 1 means "encrypted"
		};

		KFNFileParser();

		// Open the KFN file
		bool	open( const QString& filename );

		// Close the file
		void	close();

		// If error, returns the message
		const char * errorMsg() const;

		// Extracts and writes the music file in a separate file
		bool	writeMusicFile( QFile& outfile );

		// Returns lyrics as LRC data
		QString	lyricsAsLRC();

		// Original music file extention
		QString	musicFileExtention() const;

		// Directory entries
		const QList<Entry> entries() const;

	private:
		quint8	readByte();
		quint16	readWord();
		quint32	readDword();
		void	readBytes( char * buf, unsigned int len );
		QByteArray	readBytes( unsigned int len );
		QString		readString( unsigned int len );
		QByteArray	extract( const Entry& entry );

	private:
		int				m_entryMusic;
		int				m_entrySongIni;
		QFile			m_file;
		QString			m_errorMsg;
		QByteArray		m_aesKey;
		QList<Entry>	m_entries;
};


#endif // KFN_FILE_PARSER_H
