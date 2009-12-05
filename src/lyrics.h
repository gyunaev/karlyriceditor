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

#ifndef LYRICS_H
#define LYRICS_H

#include <QDataStream>
#include <QString>
#include <QVector>
#include <QList>

class Lyrics
{
	public:
		Lyrics();

		static const int PITCH_NOTE_FREESTYLE = (1 << 17);
		static const int PITCH_NOTE_GOLDEN = (1 << 18);

		//
		// Those functions are used to access lyrics
		//
		typedef struct
		{
			qint64	timing;
			QString	text;		// May be empty
			int		pitch;		// -1 if not set, used for Ultrastar
		} Syllable;

		typedef QVector<Syllable>	Line;
		typedef QList<Line>			Block;

		// Lyrics are composed from one or more blocks, depending on mode
		// A block can contain one or more lines (up to the whole text)
		// A line can contain one or more syllables
		bool	isEmpty() const;
		int		totalBlocks() const;
		const Block&	block( int index ) const;

		//
		// Those functions are used during lyric scanning
		//

		// Indicates the lyrics are being built
		void	beginLyrics();

		// Set the time for current lyric
		void	curLyricSetTime( qint64 timems );

		// Set the pitch for current lyric
		void	curLyricSetPitch( int pitch );

		// Append text to current lyric
		void	curLyricAppendText( const QString& text );

		// Adds the currently set lyric
		void	curLyricAdd();

		// Add "end of line". Multiple end of lines mean end of paragraph. This implies curLyricAdd()
		void	curLyricAddEndOfLine();

		// Indicates the lyrics are being built. No curlyric* functions may be called.
		void	endLyrics();

		// Clear the lyrics
		void	clear();

		// Pitch text representation
		static QString pitchToNore( int pitch );

	private:
		QList<Block>	m_lyrics;

		// Used during scanning lyrics
		bool			m_scanning;
		int				m_added_eofs;
		Syllable		m_currentLyric;
};

//QDataStream &operator<<(QDataStream &, const Lyrics &);
//QDataStream &operator>>(QDataStream &, Lyrics &);


#endif // LYRICS_H
