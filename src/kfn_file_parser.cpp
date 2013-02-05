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

#include "kfn_file_parser.h"

#include <QMap>
#include <QRegExp>
#include <QStringList>

#define KFN_SUPPORT_ENCRYPTION

#if defined (KFN_SUPPORT_ENCRYPTION)
	#include <openssl/evp.h>
#endif

class KFNFileException
{
};

KFNFileParser::KFNFileParser()
{
}

const char * KFNFileParser::errorMsg() const
{
	return qPrintable( m_errorMsg );
}

const QList<KFNFileParser::Entry> KFNFileParser::entries() const
{
	return m_entries;
}

bool KFNFileParser::open( const QString& filename )
{
	m_entryMusic = -1;
	m_entrySongIni = -1;

	// Open the KFN file
	m_file.setFileName( filename );

	if ( !m_file.open( QIODevice::ReadOnly ) )
	{
		m_errorMsg = "Cannot open file for reading";
		return false;
	}

	try
	{
		// See http://www.ulduzsoft.com/2012/10/reverse-engineering-the-karafun-file-format-part-1-the-header/
		char name[4];

		// Check the signature
		readBytes( name, 4 );

		if ( name[0] != 'K' || name[1] != 'F' || name[2] != 'N' || name[3] != 'B' )
		{
			m_errorMsg = "Not a valid KFN file";
			return false;
		}

		// Parse the file header
		while ( 1 )
		{
			readBytes( name, 4 );
			quint8 type = readByte();
			quint32 len_or_value = readDword();

			// Type 2 is variable data length
			if ( type == 2 )
			{
				QByteArray data = readBytes( len_or_value );

				// Store the AES encoding key
				if ( name[0] == 'F' && name[1] == 'L' && name[2] == 'I' && name[3] == 'D' )
					m_aesKey = data;
			}

			// End of header?
			if ( name[0] == 'E' && name[1] == 'N' && name[2] == 'D' && name[3] == 'H' )
				break;
		}

		// Parse the directory
		quint32 totalFiles = readDword();

		for ( quint32 i = 0; i < totalFiles; i++ )
		{
			Entry entry;

			entry.filename = readString( readDword() );
			entry.type = readDword();
			entry.length_out = readDword();
			entry.offset = readDword();
			entry.length_in = readDword();
			entry.flags = readDword();

			m_entries.push_back( entry );
		}

		// Since all the offsets are based on the end of directory, readjust them and find the music/lyrics
		for ( int i = 0; i < m_entries.size(); i++ )
		{
			m_entries[i].offset += m_file.pos();

			if ( m_entries[i].type == TYPE_SONGTEXT )
				m_entrySongIni = i;

			if ( m_entries[i].type == TYPE_MUSIC && m_entryMusic == -1 )
				m_entryMusic = i;
		}

		if ( m_entryMusic == -1 || m_entrySongIni == -1 )
		{
			m_errorMsg = "File doesn't have any music or lyrics";
			return false;
		}

		return true;
	}
	catch ( KFNFileException ex )
	{
		m_errorMsg = "Cannot read data: incomplete file";
		return false;
	}
}

void KFNFileParser::close()
{
	m_file.close();
}

QString	KFNFileParser::musicFileExtention() const
{
	if ( m_entryMusic == -1 )
		return QString::null;

	const Entry& entry = m_entries[m_entryMusic];

	QString ext = entry.filename;
	int lastdot = ext.lastIndexOf( '.' );

	if ( lastdot == -1 )
		return "";
	else
		return ext.mid( lastdot + 1 );
}

bool KFNFileParser::writeMusicFile( QFile& outfile )
{
	if ( m_entryMusic == -1 )
		return false;

	const Entry& entry = m_entries[m_entryMusic];

	QByteArray data = extract( entry );

	if ( data.isEmpty() )
		return false;

	return outfile.write( data );
}

QString	KFNFileParser::lyricsAsLRC()
{
	if ( m_entrySongIni == -1 )
		return QString::null;

	const Entry& entry = m_entries[m_entrySongIni];

	QByteArray data = extract( entry );

	if ( data.isEmpty() )
		return QString::null;

	QString songini = QString::fromUtf8( data.data(), data.size() );
	songini.replace( QRegExp("[\r\n]+"), "\n" );
	QStringList lines = songini.split( "\n" );

	// Parse the song.ini and fill up the sync and text arrays
	QStringList texts;
	QList< int > syncs;

	QRegExp patternSync( "^Sync[0-9]+=(.+)" );
	QRegExp patternText( "^Text[0-9]+=(.*)" );

	// Analyze each line
	for ( int i = 0; i < lines.size(); i++ )
	{
		QString line = lines[i];

		// Try to match the sync first
		if ( line.indexOf( patternSync ) != -1 )
		{
			// Syncs are split by comma
			QStringList values = patternSync.cap( 1 ).split(",");

			for ( int v = 0; v < values.size(); v++ )
				syncs.push_back( values[v].toInt() );
		}

		// Now the text
		if ( line.indexOf( patternText ) != -1 )
		{
			if ( !patternText.cap( 1 ).isEmpty() )
			{
				// Text is split by word and optionally by the slash
				QStringList values = patternText.cap( 1 ).split(" ");

				for ( int v = 0; v < values.size(); v++ )
				{
					QStringList morevalues = values[v].split( "/" );

					for ( int vv = 0; vv < morevalues.size(); vv++ )
						texts.push_back( morevalues[vv] );

					// We split by space, so add it at the end of each word
					texts.last() = texts.last() + " ";
				}
			}

			// Line matched, so make it a line
			if ( texts.size() > 2 && texts[ texts.size() - 2 ] != "\n" )
				texts.push_back( "\n" );
		}
	}

	int curr_sync = 0;
	bool has_linefeed = false;
	int lines_no_block = 0;
	int lastsync = -1;

	// The original timing marks are not necessarily sorted, so we add them into a map
	// and then output them from that map
	QMap< int, QString > sortedLyrics;

	for ( int i = 0; i < texts.size(); i++ )
	{
		if ( texts[i] == "\n" )
		{
			if ( lastsync == -1 )
				continue;

			if ( has_linefeed )
				lines_no_block = 0;
			else if ( ++lines_no_block > 6 )
			{
				lines_no_block = 0;
				sortedLyrics[ lastsync ] += "\n";
			}

			has_linefeed = true;
			sortedLyrics[ lastsync ] += "\n";
			continue;
		}
		else
			has_linefeed = false;

		// Get the time if we have it
		if ( curr_sync >= syncs.size() )
			continue;

		lastsync = syncs[ curr_sync++ ];
		sortedLyrics.insert( lastsync, texts[i] );
	}

	QString lrcoutput = "";
	for ( QMap< int, QString >::const_iterator it = sortedLyrics.begin(); it != sortedLyrics.end(); ++it )
	{
		int syncval = it.key();
		int min = syncval / 6000;
		int sec = (syncval - (min * 6000)) / 100;
		int msec = syncval - (min * 6000 + sec * 100);
		char timebuf[256];
		sprintf( timebuf, "[%d:%02d.%02d]", min, sec, msec );

		lrcoutput += timebuf + it.value();
	}

	return lrcoutput.trimmed();
}

QByteArray KFNFileParser::extract( const Entry& entry )
{
	// The file starts here
	m_file.seek( entry.offset );

	// For a non-encrypted files we just return the whole array
	if ( (entry.flags & 0x01) == 0 )
	{
		QByteArray array = m_file.read( entry.length_in );

		if ( array.size() != entry.length_in )
			return QByteArray();

		return array;
	}

#if defined (KFN_SUPPORT_ENCRYPTION)
	// A file is encrypted, decrypt it
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init( &ctx );

	EVP_DecryptInit_ex( &ctx, EVP_aes_128_ecb(), 0, (const unsigned char*) m_aesKey.data(), 0 );

	QByteArray array( entry.length_out, 0 );
	int total_in = 0, total_out = 0;

	// Size of the buffer must be a multiple of 16
	char buffer[8192], outbuf[8192];

	while ( total_in < entry.length_in )
	{
		int toRead = qMin( (unsigned int)  sizeof(buffer), (unsigned int) entry.length_in - total_in );
		int bytesRead = m_file.read( buffer, toRead );

		// We might need to write less than we read since the file is rounded to 16 bytes
		int toWrite = sizeof(outbuf);

		if ( bytesRead != toRead )
		{
			EVP_CIPHER_CTX_cleanup( &ctx );
			m_errorMsg = "File truncated";
			return QByteArray();
		}

		// Decrypt the content
		if ( !EVP_DecryptUpdate( &ctx, (unsigned char*) outbuf, &toWrite, (unsigned char*) buffer, bytesRead ) )
		{
			EVP_CIPHER_CTX_cleanup( &ctx );
			m_errorMsg = "Decryption failed";
			return QByteArray();
		}

		memcpy( array.data() + total_out, outbuf, toWrite );
		total_out += toWrite;
		total_in += bytesRead;
	}

	EVP_CIPHER_CTX_cleanup( &ctx );
	return array;
#else
	m_errorMsg = "File is encrypted, but decryption support is not compiled in";
	return QByteArray();
#endif
}

quint8 KFNFileParser::readByte()
{
	quint8 byte;

	if ( m_file.read( (char*) &byte, 1) != 1 )
		throw KFNFileException();

	return byte;
}

quint16	KFNFileParser::readWord()
{
	quint8 b1 = readByte();
	quint8 b2 = readByte();

	return b2 << 8 | b1;
}

quint32	KFNFileParser::readDword()
{
	quint8 b1 = readByte();
	quint8 b2 = readByte();
	quint8 b3 = readByte();
	quint8 b4 = readByte();

	return b4 << 24 | b3 << 16 | b2 << 8 | b1;
}

QByteArray KFNFileParser::readBytes( unsigned int len )
{
	QByteArray arr = m_file.read( len );

	if ( arr.size() != (int) len )
		throw KFNFileException();

	return arr;
}

QString KFNFileParser::readString( unsigned int len )
{
	QByteArray arr = readBytes( len );

	return QString::fromUtf8( arr.data(), arr.size() );
}

void KFNFileParser::readBytes( char * buf, unsigned int len )
{
	if ( m_file.read( buf, len ) != len )
		throw KFNFileException();
}
