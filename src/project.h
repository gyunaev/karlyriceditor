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

#ifndef PROJECT_H
#define PROJECT_H

#include <QMap>
#include <QString>

class Editor;
class Lyrics;


class Project
{
	public:
		enum LyricType
		{
			LyricType_LRC1 = 1,
			LyricType_LRC2,
			LyricType_UStar
		};

		enum Tag
		{
			Tag_Title = 1,
			Tag_Artist,
			Tag_Album,
			Tag_Language,
			Tag_Genre,
			Tag_MP3File,
			Tag_Cover,
			Tag_Background,
			Tag_Video,
			Tag_VideoGap,
			Tag_Edition,
			Tag_CreatedBy,
			Tag_Offset,
			Tag_Application,
			Tag_Appversion,
		};

		Project( Editor * editor );

		// load/save project from/to file
		bool	save( const QString& filename );
		bool	load( const QString& filename );

		// lyric type
		void	setType( LyricType type );
		LyricType	type() const;

		// Music file
		void	setMusicFile( const QString& musicfile );
		QString musicFile() const;

		// Set the music tag fields
		void	setTag( Tag tag, const QString& value );
		QString tag( Tag tagid );

		// Is project modified?
		bool	isModified() const { return m_modified; }
		void	setModified();

		// Export lyrics as current format, and as any specific
		QString	exportLyrics();
		QString	exportLyricsAsLRC1();
		QString	exportLyricsAsLRC2();
		QString	exportLyricsAsUStar();

		// Import lyrics. So far the formats are fairly recognizable, so other imports are private
		bool	importLyrics( const QString& filename, LyricType type );

		// Clear the project
		void	clear();

		// Split the time mark
		static void splitTimeMark( qint64 mark, int * min, int * sec, int * msec );

	private:
		bool	importLyricsLRC( const QStringList & readlyrics, Lyrics& lyrics );
		bool	importLyricsUStar( const QStringList & readlyrics, Lyrics& lyrics );

		void	appendIfPresent( int id, const QString& prefix, QString& src, LyricType type );
		QString	generateLRCheader();
		QString	generateUStarheader();
		void	update( int id, const QString& value );

		int		tagToId( Tag tagid  );
		bool	m_modified;
		Editor* m_editor;

		// This container stores all project-related data
		QMap< int, QString>	m_projectData;

};

#endif // PROJECT_H
