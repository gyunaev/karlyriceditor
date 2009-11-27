/***************************************************************************
 *   Copyright (C) 2009 Georgy Yunaev, gyunaev@ulduzsoft.com               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QFile>
#include <QDataStream>
#include <QMessageBox>

#include "mainwindow.h"
#include "project.h"

// Project data enum
// Formats info:
// UltraStar: http://ultrastardeluxe.xtremeweb-hosting.net/wiki/doku.php?id=editor:txt_file
// LRC: http://en.wikipedia.org/wiki/LRC_%28file_format%29
//
enum
{
	PD_SIGNATURE,		// BONIFACI
	PD_VERSION,			// save file version; currently 1
	PD_MUSICFILE,
	PD_LYRICS,
	PD_LYRICTYPE,
	PD_TAG_TITLE,
	PD_TAG_ARTIST,
	PD_TAG_ALBUM,

	PD_TAG_CREATEDBY = 40,	// LRC tag
	PD_TAG_OFFSET,			// LRC tag
	PD_TAG_APPLICATION,		// LRC tag
	PD_TAG_APPVERSION,		// LRC tag

	PD_TAG_LANGUAGE = 50,	// UltraStar tag
	PD_TAG_GENRE,			// UltraStar tag
	PD_TAG_MP3FILE,			// UltraStar tag
	PD_TAG_COVER,			// UltraStar tag
	PD_TAG_BACKGROUND,		// UltraStar tag
	PD_TAG_VIDEO,			// UltraStar tag
};


Project::Project( Editor* editor )
{
	m_editor = editor;
	m_editor->setProject( this );

	m_modified = false;
	m_projectData[ PD_SIGNATURE ] = "BONIFACI";
	m_projectData[ PD_VERSION ] = 1;
}

void Project::setType( LyricType type )
{
	update( PD_LYRICTYPE, QString::number( type ) );
}

void Project::setMusicFile( const QString& musicfile )
{
	update( PD_MUSICFILE, musicfile );
}


QString Project::musicFile() const
{
	return m_projectData[ PD_MUSICFILE ];
}

bool Project::load( const QString& filename )
{
	QFile file( filename );

	if ( !file.open( QIODevice::ReadOnly ) )
	{
		QMessageBox::critical( 0,
			QObject::tr("Cannot open file"),
			QObject::tr("Cannot open file %1") .arg( filename ) );

		return false;
	}

	QDataStream stream( &file );
	m_projectData.clear();
	stream >> m_projectData;

	if ( m_projectData[ PD_SIGNATURE ] != "BONIFACI" )
	{
		QMessageBox::critical( 0,
			QObject::tr("Invalid project file"),
			QObject::tr("The project file %1 is not a valid Lyric Editor project file") .arg( filename ) );

		return false;
	}

	m_editor->importFromString( m_projectData[ PD_LYRICS ] );
	m_modified = false;
	return true;
}

bool Project::save( const QString& filename )
{
	QFile file( filename );

	if ( !file.open( QIODevice::WriteOnly ) )
	{
		QMessageBox::critical( 0,
			QObject::tr("Cannot write file"),
			QObject::tr("Cannot write file %1") .arg( filename ) );

		return false;
	}

	m_projectData[ PD_LYRICS ] = m_editor->exportToString();

	QDataStream stream( &file );
	stream.setVersion( QDataStream::Qt_4_5 );
	stream << m_projectData;

	m_modified = false;
	return true;
}

int	Project::tagToId( Tag tag  )
{
	int tagid = -1;

	switch ( tag )
	{
		case Tag_Title:
			tagid = PD_TAG_TITLE;
			break;

		case Tag_Artist:
			tagid = PD_TAG_ARTIST;
			break;

		case Tag_Album:
			tagid = PD_TAG_ALBUM;
			break;

		case Tag_Language:
			tagid = PD_TAG_LANGUAGE;
			break;

		case Tag_Genre:
			tagid = PD_TAG_GENRE;
			break;

		case Tag_MP3File:
			tagid = PD_TAG_MP3FILE;
			break;

		case Tag_Cover:
			tagid = PD_TAG_COVER;
			break;

		case Tag_Background:
			tagid = PD_TAG_BACKGROUND;
			break;

		case Tag_Video:
			tagid = PD_TAG_VIDEO;
			break;
	}

	return tagid;
}

void Project::setTag( Tag tag, const QString& value )
{
	int tagid = tagToId( tag );

	if ( tagid != -1  )
		update( tagid, value );
}

QString Project::tag( Tag itag )
{
	int tagid = tagToId( itag );

	if ( tagid != -1 )
		return m_projectData[ tagid ];

	return QString::null;
}

Project::LyricType Project::type() const
{
	return (Project::LyricType) m_projectData[ PD_LYRICTYPE ].toInt();
}

void Project::update( int id, const QString& value )
{
	if ( m_projectData[ id ] == value )
		return;

	m_projectData[ id ] = value;
	setModified();
}

void Project::setModified()
{
	m_modified = true;
	pMainWindow->updateState();
}

void Project::appendIfPresent( int id, const QString& prefix, QString& src, LyricType type )
{
	if ( !m_projectData.contains( id ) || m_projectData[id].isEmpty() )
		return;

	switch ( type )
	{
		case LyricType_LRC1:
		case LyricType_LRC2:
			src += "[" + prefix + ": " + m_projectData[id] + "]\n";
			break;

		case LyricType_UStar:
			src += "#" + prefix + ":" + m_projectData[id] + "\n";
			break;
	}
}

QString	Project::generateLRCheader()
{
	QString hdr;

	appendIfPresent( PD_TAG_TITLE, "ti", hdr, LyricType_LRC1 );
	appendIfPresent( PD_TAG_ARTIST, "ar", hdr, LyricType_LRC1 );
	appendIfPresent( PD_TAG_ALBUM, "al", hdr, LyricType_LRC1 );

	return hdr;
}

QString	Project::generateUStarheader()
{
	QString hdr;

	appendIfPresent( PD_TAG_TITLE, "TITLE", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_ARTIST, "ARTIST", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_LANGUAGE, "LANGUAGE", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_GENRE, "GENRE", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_MP3FILE, "MP3FILE", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_COVER, "COVER", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_BACKGROUND, "BACKGROUND", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_VIDEO, "VIDEO", hdr, LyricType_UStar );

	return hdr;
}

QString	Project::exportLyrics()
{
	switch ( type() )
	{
		case LyricType_LRC1:
			return exportLyricsAsLRC1();

		case LyricType_LRC2:
			return exportLyricsAsLRC2();

		case LyricType_UStar:
			return exportLyricsAsUStar();
			break;
	}

	return "UNSUPORTED";
}

QString	Project::exportLyricsAsLRC1()
{
	// Generate warnings about conversion consequences
	QString warntext;

	switch ( type() )
	{
		case LyricType_LRC2:
			warntext = QObject::tr("Current lyrics format is set to LRC version 2.\n\n"
				"When exporting it as LRC version 1 all time tags except the one in front of the line "
				"will be ignored.\n\n"
				"Do you want to proceed with export?");
			break;

		case LyricType_UStar:
			warntext = QObject::tr("Current lyrics format is set to Ultrastar.\n\n"
				"When exporting it as LRC version 1 all time tags except the one in front of the line "
				"will be ignored as well as the pitch information.\n\n"
				"Do you want to proceed with export?");
			break;

		case LyricType_LRC1:
			break;
	}

	if ( !warntext.isEmpty() )
	{
		if ( QMessageBox::question( 0,
									QObject::tr("Exporting lyrics"),
									warntext,
									QMessageBox::Yes|QMessageBox::No ) == QMessageBox::No )
			return QString::null;
	}

	QString	lrc = generateLRCheader();
	Lyrics lyrics = m_editor->exportLyrics();

	for ( int bl = 0; bl < lyrics.totalBlocks(); bl++ )
	{
		if ( bl > 0 )
			lrc += "\n";

		const Lyrics::Block& block = lyrics.block( bl );

		for ( int ln = 0; ln < block.size(); ln++ )
		{
			const Lyrics::Line& line = block[ln];

			for ( int pos = 0; pos < line.size(); pos++ )
			{
				Lyrics::Syllable lentry = line[pos];

				// Insert timing mark
				QString timetag;
				int minute = lentry.timing / 60000;
				int second = (lentry.timing - minute * 60000) / 1000;
				int msecond = lentry.timing - (minute * 60000 + second * 1000 );
				timetag.sprintf( "%02d:%02d.%02d", minute, second, msecond / 10 );

				if ( pos == 0 )
					lrc += "[" + timetag + "]";

				lrc += lentry.text;
			}

			lrc += "\n";
		}
	}

	return lrc;
}

QString	Project::exportLyricsAsLRC2()
{
	// Generate warnings about conversion consequences
	QString warntext;

	switch ( type() )
	{
		case LyricType_LRC1:
		case LyricType_LRC2:
			break; // seamless conversion

		case LyricType_UStar:
			warntext = QObject::tr("Current lyrics format is set to Ultrastar.\n\n"
				"When exporting it as LRC version 2 the pitch information will be ignored.\n\n"
				"Do you want to proceed with export?");
			break;
	}

	if ( !warntext.isEmpty() )
	{
		if ( QMessageBox::question( 0,
									QObject::tr("Exporting lyrics"),
									warntext,
									QMessageBox::Yes|QMessageBox::No ) == QMessageBox::No )
			return QString::null;
	}

	QString	lrc = generateLRCheader();
	Lyrics lyrics = m_editor->exportLyrics();

	for ( int bl = 0; bl < lyrics.totalBlocks(); bl++ )
	{
		if ( bl > 0 )
			lrc += "\n";

		const Lyrics::Block& block = lyrics.block( bl );

		for ( int ln = 0; ln < block.size(); ln++ )
		{
			const Lyrics::Line& line = block[ln];

			for ( int pos = 0; pos < line.size(); pos++ )
			{
				Lyrics::Syllable lentry = line[pos];

				// Insert timing mark
				QString timetag;
				int minute = lentry.timing / 60000;
				int second = (lentry.timing - minute * 60000) / 1000;
				int msecond = lentry.timing - (minute * 60000 + second * 1000 );
				timetag.sprintf( "%02d:%02d.%02d", minute, second, msecond / 10 );

				if ( pos == 0 )
					lrc += "[" + timetag + "]";
				else
					lrc += "<" + timetag + ">";

				lrc += lentry.text;
			}

			lrc += "\n";
		}
	}

	return lrc;
}

QString	Project::exportLyricsAsUStar()
{
	// 5 beats per second should give us good enough precision
	int bpm = 300;

	// Generate warnings about conversion consequences
	QString warntext;

	switch ( type() )
	{
		case LyricType_UStar:
			break;

		case LyricType_LRC1:
			warntext = QObject::tr("Current lyrics format is set to LRC version 1.\n\n"
				"When exporting it as UltraStar format the output lyrics will not contain pitch information.\n"
				"Also since the lyric syllable duration is a whole line, the output lyrics won't be usable.\n\n"
				"Do you want to proceed with export?");
			break;

		case LyricType_LRC2:
			warntext = QObject::tr("Current lyrics format is set to LRC version 2.\n\n"
				"When exporting it as UltraStar format the output lyrics will not contain pitch information.\n\n"
				"Do you want to proceed with export?");
			break;
	}

	if ( !warntext.isEmpty() )
	{
		if ( QMessageBox::question( 0,
									QObject::tr("Exporting lyrics"),
									warntext,
									QMessageBox::Yes|QMessageBox::No ) == QMessageBox::No )
			return QString::null;
	}

	Lyrics lyrics = m_editor->exportLyrics();

	// Calculate gap
	int beat_time_ms = 1000 / (bpm / 60);
	int gap = lyrics.block(0).front().front().timing / beat_time_ms;

	QString	lrc = generateUStarheader();
	lrc += QString("#BPM: %1\n") .arg(bpm);
	lrc += QString("#GAP: %1\n") .arg(gap);

	for ( int bl = 0; bl < lyrics.totalBlocks(); bl++ )
	{
		//TODO
/*		if ( bl > 0 )
			lrc += "\n";

		const Lyrics::Block& block = lyrics.block( bl );

		for ( int ln = 0; ln < block.size(); ln++ )
		{
			const Lyrics::Line& line = block[ln];

			for ( int pos = 0; pos < line.size(); pos++ )
			{
				Lyrics::Syllable lentry = line[pos];

				// Insert timing mark
				QString timetag;
				int minute = lentry.timing / 60000;
				int second = (lentry.timing - minute * 60000) / 1000;
				int msecond = lentry.timing - (minute * 60000 + second * 1000 );
				timetag.sprintf( "%02d:%02d.%02d", minute, second, msecond / 10 );

				if ( pos == 0 )
					lrc += "[" + timetag + "]";
				else
					lrc += "<" + timetag + ">";

				lrc += lentry.text;
			}

			lrc += "\n";
		}
*/	}

	return lrc;
}
