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

#ifndef KARAOKELYRICSTEXTKAR_H
#define KARAOKELYRICSTEXTKAR_H

// This file is taken from my XBMC Karaoke MIDI parser
// It is hacked to keep the code base the same.
// It looks kinda ugly, but this makes updating easier.
// HACK START
#include <QtCore>

typedef QByteArray	CStdString;
class CKaraokeLyricsText
{
	public:
		
	enum
	{
		LYRICS_NEW_PARAGRAPH,
		LYRICS_NEW_LINE
	};
};
// HACK END


//! This class loads MIDI/KAR format lyrics
class CKaraokeLyricsTextKAR : public CKaraokeLyricsText
{
	public:
		static QByteArray getLyrics( const QByteArray& arr )
		{
			CKaraokeLyricsTextKAR karfile( "qq" );

			if ( karfile.LoadData( arr ) )
				return karfile.m_lyrics;

			return QByteArray();
		}

	protected:
		CKaraokeLyricsTextKAR( const CStdString & midiFile );
		~CKaraokeLyricsTextKAR();

// HACK START		
		bool LoadData( const QByteArray& arr )
		{
			m_midiSize = arr.size();
			m_midiData = (unsigned char*) arr.data();

			// Parse MIDI
			try
			{
				parseMIDI();
				m_midiData = 0;
				return true;
			}
			catch ( const char * p )
			{
				m_midiData = 0;
				return false;
			}
		}

		void	addLyrics( CStdString text, unsigned int mstime, unsigned int flags )
		{
			// Convert time
			int min = mstime / 60000;
			int sec = (mstime - min * 60000) / 1000;
			int msec = mstime - (min * 60000 + sec * 1000 );

			QString timing = QString().sprintf( "[%02d:%02d.%02d]", min, sec, msec / 10 );

			if ( flags & LYRICS_NEW_PARAGRAPH )
				m_lyrics += "\n\n";
			else if ( flags & LYRICS_NEW_LINE )
				m_lyrics += "\n";

			m_lyrics += timing.toLocal8Bit() + text;
		}
		
		QByteArray 		m_lyrics;
// HACK END		

	private:
		void			parseMIDI();
		
		unsigned char 	readByte();
		unsigned short	readWord();
		unsigned int	readDword();
		int 			readVarLen();
		void			readData( void * buf, unsigned int length );

		unsigned int 	currentPos() const;
		void			setPos( unsigned int offset );

		// MIDI file name
		CStdString 		m_midiFile;

		// MIDI in-memory information
		unsigned char *	m_midiData;
		unsigned int	m_midiOffset;
		unsigned int	m_midiSize;
};

#endif
