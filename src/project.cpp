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

#include <QFile>
#include <QDataStream>
#include <QMessageBox>

#include "mainwindow.h"
#include "project.h"
#include "settings.h"
#include "version.h"
#include "lyrics.h"
#include "karaokelyricstextkar.h"
#include "cdggenerator.h"
#include "kfn_file_parser.h"
#include "util.h"

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
	PD_LYRICS_OLD,
	PD_LYRICTYPE,
	PD_TAG_TITLE,
	PD_TAG_ARTIST,
	PD_TAG_ALBUM,
	PD_LYRICS_NEW,

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
	PD_TAG_VIDEOGAP,		// UltraStar tag
	PD_TAG_EDITION,			// UltraStar tag

	PD_TAG_CDG_BCOLOR,		// CD+G tag - background color
	PD_TAG_CDG_ICOLOR,		// CD+G tag - info (title/credits) color
	PD_TAG_CDG_ACOLOR,		// CD+G tag - active (not sung yet) color
	PD_TAG_CDG_NCOLOR,		// CD+G tag - inactive (sung) color
	PD_TAG_CDG_FONT,		// CD+G tag - font family
	PD_TAG_CDG_FONTSIZE,	// CD+G tag - font size
	PD_TAG_CDG_MINTITLE,	// CD+G tag - minimum title time
	PD_TAG_CDG_PREAMBLE,	// CD+G tag - whether to show squares

	PD_TAG_VIDEO_BCOLOR,	// Video tag - background color
	PD_TAG_VIDEO_ICOLOR,	// Video tag - info (title/credits) color
	PD_TAG_VIDEO_ACOLOR,	// Video tag - active (not sung yet) color
	PD_TAG_VIDEO_NCOLOR,	// Video tag - inactive (sung) color
	PD_TAG_VIDEO_FONT,		// Video tag - font family
	PD_TAG_VIDEO_FONTSIZE,	// Video tag - font size
	PD_TAG_VIDEO_MINTITLE,	// Video tag - minimum title time
	PD_TAG_VIDEO_PREAMBLE,	// Video tag - whether to show squares
	PD_TAG_VIDEO_BGFILE,	// unused
	PD_TAG_VIDEO_IMAGESIZE,	// index of image size
	PD_TAG_VIDEO_FPSSIZE,	// index of FPS
	PD_TAG_VIDEO_ENCODING,	// index of video encoding
	PD_TAG_VIDEO_CONTAINER,	// index of container format
	PD_TAG_VIDEO_ALLKEYFRAMES, // if nonzero, every frame is a keyframe
	PD_TAG_VIDEO_NOAUDIO	// export no audio
};


void Project::splitTimeMark( qint64 mark, int * min, int * sec, int * msec )
{
	*min = mark / 60000;
	*sec = (mark - *min * 60000) / 1000;
	*msec = mark - (*min * 60000 + *sec * 1000 );
}

Project::Project( Editor* editor )
{
	m_editor = editor;
	m_editor->setProject( this );

	m_modified = false;
	clear();
}

void Project::clear()
{
	m_projectData.clear();
	m_projectData[ PD_SIGNATURE ] = "BONIFACI";
	m_projectData[ PD_VERSION ] = 1;
	m_projectData[ PD_TAG_OFFSET ] = QString::number( pSettings->m_phononSoundDelay );
	m_projectData[ PD_TAG_APPLICATION ] = APP_NAME;
	m_projectData[ PD_TAG_APPVERSION ] = QString("%1.%2").arg( APP_VERSION_MAJOR ).arg( APP_VERSION_MINOR );

	// Init CD+G data
	m_projectData[ PD_TAG_CDG_BCOLOR ] = "black";
	m_projectData[ PD_TAG_CDG_ICOLOR ] = "white";
	m_projectData[ PD_TAG_CDG_ACOLOR ] = QColor( 0xC0, 0x00, 0xC0 ).name();
	m_projectData[ PD_TAG_CDG_NCOLOR ] = QColor( 0x00, 0xC0, 0xC0 ).name();
	m_projectData[ PD_TAG_CDG_FONT ] = "Droid Sans";
	m_projectData[ PD_TAG_CDG_FONTSIZE ] = "12";
	m_projectData[ PD_TAG_CDG_MINTITLE ] = "5";
	m_projectData[ PD_TAG_CDG_PREAMBLE ] = "1";

	m_totalSongLength = 0;
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

	if ( !m_projectData[ PD_LYRICS_NEW ].isEmpty() )
		m_editor->importFromString( m_projectData[ PD_LYRICS_NEW ] );
	else
		m_editor->importFromOldString( m_projectData[ PD_LYRICS_OLD ] );

	// Check the lyrics type, and reset it to LRC2 if not specified
	switch ( type() )
	{
		case LyricType_LRC1:
		case LyricType_LRC2:
		case LyricType_UStar:
			m_modified = false;
			break;

		default:
			setType( LyricType_LRC2 );
			m_modified = true;
			break;
	}

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

	m_projectData[ PD_LYRICS_NEW ] = m_editor->exportToString();

	QDataStream stream( &file );
	stream.setVersion( QDataStream::Qt_4_5 );
	stream << m_projectData;

	m_modified = false;
	return true;
}

int	Project::tagToId( Tag tag  ) const
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

		case Tag_CreatedBy:
			tagid = PD_TAG_CREATEDBY;
			break;

		case Tag_Offset:
			tagid = PD_TAG_OFFSET;
			break;

		case Tag_Application:
			tagid = PD_TAG_APPLICATION;
			break;

		case Tag_Appversion:
			tagid = PD_TAG_APPVERSION;
			break;

		case Tag_VideoGap:
			tagid = PD_TAG_VIDEOGAP;
			break;

		case Tag_Edition:
			tagid = PD_TAG_EDITION;
			break;

		case Tag_CDG_bgcolor:
			tagid = PD_TAG_CDG_BCOLOR;
			break;

		case Tag_CDG_infocolor:
			tagid = PD_TAG_CDG_ICOLOR;
			break;

		case Tag_CDG_activecolor:
			tagid = PD_TAG_CDG_ACOLOR;
			break;

		case Tag_CDG_inactivecolor:
			tagid = PD_TAG_CDG_NCOLOR;
			break;

		case Tag_CDG_font:
			tagid = PD_TAG_CDG_FONT;
			break;

		case Tag_CDG_fontsize:
			tagid = PD_TAG_CDG_FONTSIZE;
			break;

		case Tag_CDG_titletime:
			tagid = PD_TAG_CDG_MINTITLE;
			break;

		case Tag_CDG_preamble:
			tagid = PD_TAG_CDG_PREAMBLE;
			break;

		case Tag_Video_bgcolor:
			tagid = PD_TAG_VIDEO_BCOLOR;
			break;

		case Tag_Video_infocolor:
			tagid = PD_TAG_VIDEO_ICOLOR;
			break;

		case Tag_Video_activecolor:
			tagid = PD_TAG_VIDEO_ACOLOR;
			break;

		case Tag_Video_inactivecolor:
			tagid = PD_TAG_VIDEO_NCOLOR;
			break;

		case Tag_Video_font:
			tagid = PD_TAG_VIDEO_FONT;
			break;

		case Tag_Video_fontsize:
			tagid = PD_TAG_VIDEO_FONTSIZE;
			break;

		case Tag_Video_titletime:
			tagid = PD_TAG_VIDEO_MINTITLE;
			break;

		case Tag_Video_preamble:
			tagid = PD_TAG_VIDEO_PREAMBLE;
			break;

		case Tag_Video_ImgSizeIndex:
			tagid = PD_TAG_VIDEO_IMAGESIZE;
			break;

		case Tag_Video_FpsIndex:
			tagid = PD_TAG_VIDEO_FPSSIZE;
			break;

		case Tag_Video_EncodingIndex:
			tagid = PD_TAG_VIDEO_ENCODING;
			break;

		case Tag_Video_ContainerIndex:
			tagid = PD_TAG_VIDEO_CONTAINER;
			break;

		case Tag_Video_AllKeyframes:
			tagid = PD_TAG_VIDEO_ALLKEYFRAMES;
			break;

		case Tag_Video_ExportNoAudio:
			tagid = PD_TAG_VIDEO_NOAUDIO;
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

QString Project::tag( Tag itag, const QString& defvalue ) const
{
	int tagid = tagToId( itag );

	if ( tagid != -1 && m_projectData.contains( tagid ) )
		return m_projectData[ tagid ];

	return defvalue;
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
	appendIfPresent( PD_TAG_CREATEDBY, "by", hdr, LyricType_LRC1 );
	appendIfPresent( PD_TAG_OFFSET, "offset", hdr, LyricType_LRC1 );
	appendIfPresent( PD_TAG_APPLICATION, "re", hdr, LyricType_LRC1 );
	appendIfPresent( PD_TAG_APPVERSION, "ve", hdr, LyricType_LRC1 );

	return hdr;
}

QString	Project::generateUStarheader()
{
	QString hdr;

	appendIfPresent( PD_TAG_TITLE, "TITLE", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_ARTIST, "ARTIST", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_LANGUAGE, "LANGUAGE", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_GENRE, "GENRE", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_COVER, "COVER", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_BACKGROUND, "BACKGROUND", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_VIDEO, "VIDEO", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_VIDEOGAP, "VIDEOGAP", hdr, LyricType_UStar );
	appendIfPresent( PD_TAG_EDITION, "EDITION", hdr, LyricType_UStar );

	return hdr;
}

QByteArray Project::exportLyrics()
{
	switch ( type() )
	{
		case LyricType_LRC1:
			return exportLyricsAsLRC1();

		case LyricType_LRC2:
			return exportLyricsAsLRC2();

		case LyricType_UStar:
			return exportLyricsAsUStar();
	}

	return "UNSUPORTED";
}

QByteArray Project::exportLyricsAsLRC1()
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
			return QByteArray();
	}

	QString	lrc = generateLRCheader();
	Lyrics lyrics;

	if ( !m_editor->exportLyrics( &lyrics) )
		return QByteArray();

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
				int minute, second, msecond;

				splitTimeMark( lentry.timing, &minute, &second, &msecond );
				timetag.sprintf( "%02d:%02d.%02d", minute, second, msecond / 10 );

				if ( pos == 0 )
					lrc += "[" + timetag + "]";

				lrc += lentry.text;
			}

			lrc += "\n";
		}
	}

	return lrc.toUtf8();
}

QByteArray Project::exportLyricsAsLRC2()
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
			return QByteArray();
	}

	Lyrics lyrics;

	if ( !m_editor->exportLyrics( &lyrics ) )
		return QByteArray();

	QString	lrc = generateLRCheader();

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
				int minute, second, msecond;

				splitTimeMark( lentry.timing, &minute, &second, &msecond );
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

	return lrc.toUtf8();
}

QByteArray Project::exportLyricsAsUStar()
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
			return QByteArray();
	}

	Lyrics lyrics;

	if ( !m_editor->exportLyrics( &lyrics ) )
		return QByteArray();

	if ( lyrics.totalBlocks() > 1 )
	{
		QMessageBox::information( 0,
								QObject::tr("Block separator found"),
								QObject::tr("UltraStart lyrics cannot contain block separators. Blocks will be merged.") );
	}

	// Calculate gap
	int beat_time_ms = 1000 / (bpm / 60);
	int gap = lyrics.block(0).front().front().timing;

	QString	lyricstext = generateUStarheader();
	lyricstext += QString("#MP3:%1\n") .arg( musicFile() );
	lyricstext += QString("#BPM:%1\n") .arg(bpm);
	lyricstext += QString("#GAP:%1\n") .arg(gap);

	for ( int i = 0; i < lyrics.totalBlocks(); i++ )
	{
		const Lyrics::Block& block = lyrics.block( i );

		for ( int ln = 0; ln < block.size(); ln++ )
		{
			const Lyrics::Line& line = block[ln];

			for ( int pos = 0; pos < line.size(); pos++ )
			{
				Lyrics::Syllable lentry = line[pos];
				bool last_entry = (pos == line.size() - 1);

				// Ultrastar lyrics must have tags at the line end, so the last tag should be empty
				if ( last_entry && !lentry.text.isEmpty() )
				{
					QMessageBox::critical( 0,
											QObject::tr("No timing mark at the end of line"),
											QObject::tr("UltraStart lyrics require timing marks at the end of line. Export aborted.") );
					return QByteArray();
				}

				// Calculate timing and duration
				int duration, timing = (lentry.timing - gap) / beat_time_ms;
				char prefix;

				if ( last_entry )
				{
					// We're at the end of line, which is empty. Get the time to calculate duration from there
					prefix = '-';

					if ( ln == block.size() - 1 )
						duration = 5000 / beat_time_ms; // assume 5sec duration
					else
						duration = (block[ln+1].front().timing - lentry.timing) / beat_time_ms;

					lyricstext += QString("- %1 %2\n").arg( timing ) .arg( duration );
				}
				else
				{
					prefix = ':';
					duration = (line[pos+1].timing - lentry.timing) / beat_time_ms;
					int pitch = lentry.pitch;

					// If lyrics is not set, ignore it
					if ( pitch == -1 )
						pitch = 0;

					if ( pitch & Lyrics::PITCH_NOTE_FREESTYLE )
					{
						prefix = 'F';
						pitch &= ~Lyrics::PITCH_NOTE_FREESTYLE;
					}
					else if ( pitch & Lyrics::PITCH_NOTE_GOLDEN )
					{
						prefix = '*';
						pitch &= ~Lyrics::PITCH_NOTE_GOLDEN;
					}

					lyricstext += QString("%1 %2 %3 %4 %5\n").arg( prefix )
															 .arg( timing )
															 .arg( duration )
															 .arg( pitch )
															 .arg( lentry.text );
				}
			}
		}
	}

	lyricstext += "E\n";
	return lyricstext.toUtf8();
}


bool Project::importLyrics( const QString& filename )
{
	bool fake_lrc_header = false;
	QString lyrictext;

	// If this is KaraFun file, parse it
	if ( filename.endsWith( "kfn", Qt::CaseInsensitive ) )
	{
		KFNFileParser parser;

		if ( !parser.open( filename ) )
		{
			QMessageBox::information( 0,
							   QObject::tr("Invalid KFN file"),
							   QObject::tr("The file %1 cannot be opened: %2") .arg( filename ) .arg( parser.errorMsg() ) );

			return false;
		}

		lyrictext = parser.lyricsAsLRC();

		// Fake the LRC header
		fake_lrc_header = true;
	}
	else
	{
		QFile file( filename );

		// Open the file and read its content
		if ( !file.open( QIODevice::ReadOnly ) )
		{
			QMessageBox::critical( 0,
								   QObject::tr( "Cannot open file" ),
								   QObject::tr( "Cannot open file %1: %2" ) .arg( filename ) .arg( file.errorString() ) );
			return false;
		}

		QByteArray data = file.readAll();

		// Close the file - the user might want to edit it now when we ask for encoding
		file.close();

		// If this is the MIDI file, parse it first
		if ( filename.endsWith( "mid", Qt::CaseInsensitive ) || filename.endsWith( "midi", Qt::CaseInsensitive ) || filename.endsWith( "kar", Qt::CaseInsensitive ) )
		{
			data = CKaraokeLyricsTextKAR::getLyrics( data );

			if ( data.isEmpty() )
			{
				QMessageBox::critical( 0,
									   QObject::tr( "Cannot read MIDI " ),
									   QObject::tr( "Cannot read lyrics from MIDI file %1" ) .arg( filename ) );
				return false;
			}

			// Fake the LRC header
			fake_lrc_header = true;
		}

		lyrictext = Util::convertWithUserEncoding( data );
	}

	if ( lyrictext.isEmpty() )
		return false;

	// Prepend a fake LRC header for MIDI import
	if ( fake_lrc_header )
		lyrictext.prepend( "[ar: unknown]\n[ti: unknown]\n" );

	// Convert CRLF to CRs and replace LFs
	lyrictext.replace( "\r\n", "\n" );
	lyrictext.remove( '\r' );

	QStringList linedtext = lyrictext.split( '\n' );

	// Import lyrics
	Lyrics lyr;
	bool success = false;

	if ( filename.endsWith( "txt" ) )
	{
		// LyricType_UStar:
		lyr.beginLyrics();
		success = importLyricsTxt( linedtext, lyr );
		lyr.endLyrics();
	}
	else if ( filename.endsWith( "kok" ) )
	{
		lyr.beginLyrics();
		success = importLyricsKOK( linedtext, lyr );
		lyr.endLyrics();
	}
	else
	{
		lyr.beginLyrics();
		success = importLyricsLRC( linedtext, lyr );
		lyr.endLyrics();
	}

	// importLyrics* already showed error message
	if ( !success )
		return false;

	m_editor->importLyrics( lyr );
	return true;
}

bool Project::convertLyrics( const QString& content )
{
	QStringList lines = content.split( '\n' );
	Lyrics lyr;

	lyr.beginLyrics();
	bool success = importLyricsLRC( lines, lyr, true );
	lyr.endLyrics();

	if ( !success )
		return false;

	m_editor->importLyrics( lyr );
	return true;
}

bool Project::importLyricsLRC( const QStringList & readlyrics, Lyrics& lyrics, bool relaxed )
{
	bool header = true;
	bool type_lrc2 = false;
	qint64 last_time = -1;

	lyrics.clear();

	for ( int i = 0; i < readlyrics.size(); i++ )
	{
		QString line = readlyrics[i];

		// To simplify the matching
		line.replace( '[', '<' );
		line.replace( ']', '>' );

		if ( header )
		{
			QRegExp regex( "^<([a-zA-Z]+):\\s*(.*)\\s*>$" );
			regex.setMinimal( true );

			if ( regex.indexIn( line ) != -1 )
			{
				QString tag = regex.cap( 1 );
				QString value = regex.cap( 2 );
				int tagid = -1;

				if ( tag == "ti" )
					tagid = PD_TAG_TITLE;
				else if ( tag == "ar" )
					tagid = PD_TAG_ARTIST;
				else if ( tag == "al" )
					tagid = PD_TAG_ALBUM;
				else if ( tag == "by" )
					tagid = PD_TAG_CREATEDBY;
				else if ( tag == "offset" )
					tagid = PD_TAG_OFFSET;
				else if ( tag == "re" )
					tagid = PD_TAG_APPLICATION;
				else if ( tag == "ve" )
					tagid = PD_TAG_APPVERSION;
				else
					qDebug("Unsupported LRC tag found: '%s', ignored", qPrintable( tag ) );

				if ( tagid != -1 )
					update( tagid, value );
			}
			else
			{
				// Tag not found; either header ended, or invalid file
				if ( i == 0 && !relaxed )
				{
					QMessageBox::critical( 0,
										   QObject::tr("Invalid LRC file"),
										   QObject::tr("Missing LRC header") );
					return false;
				}

				header = false;
			}
		}

		// We may fall-through, so no else
		if ( !header )
		{
			QRegExp regex( "<(\\d+):(\\d+)(.\\d+)?>([^<]*)" );
			int pos = 0;

			while ( (pos = regex.indexIn( line, pos )) != -1 )
			{
				if ( pos != 0 )
					type_lrc2 = true;

				QStringList match = regex.capturedTexts();
				int minutes = match[1].toInt();
				int seconds = match[2].toInt();
				QString text = match[3];
				int ms = 0;

				// msecs require more precise handling
				if ( match.size() > 4 )
				{
					QString strms = match[3];

					// We have msecs. Remove the first dot
					strms.remove( 0, 1 );

					// Convert to full msecs. Kinda ugly, but simple
					while ( strms.length() < 3 )
						strms+= "0";

					ms = strms.toInt();
					text = match[4];
				}

				qint64 timing = minutes * 60000 + seconds * 1000 + ms;

				if ( timing != last_time && timing != -1 )
				{
					lyrics.curLyricAdd();
					lyrics.curLyricSetTime( timing );
					last_time = timing;
				}

				lyrics.curLyricAppendText( text );
				pos++;
			}

			lyrics.curLyricAddEndOfLine();
		}
	}

	// Check the lyric type
	if ( type() == LyricType_LRC1 && type_lrc2 && !relaxed )
	{
		QMessageBox::warning( 0,
							  QObject::tr("LRC2 file read for LCR1 project"),
							  QObject::tr("The lyric type for current project is set for LRC1, but the lyrics read were LRC2.\n\n"
										  "Either change the project type, or fix the lyrics.") );
	}

	return true;
}

bool Project::importLyricsTxt( const QStringList & readlyrics, Lyrics& lyrics )
{
	lyrics.clear();

	if ( readlyrics.isEmpty() )
		return false;

	// TXT could be UltraStar or PowerKaraoke
	if ( readlyrics.first().indexOf( QRegExp( "^#[a-zA-Z]+:\\s*.*\\s*$" ) ) != -1 )
		return importLyricsUStar( readlyrics, lyrics );
	else if ( readlyrics.first().indexOf( QRegExp( "^([0-9.]+) ([0-9.]+) (.+)" ) ) != -1 )
		return importLyricsPowerKaraoke( readlyrics, lyrics );

	QMessageBox::critical( 0,
						   QObject::tr("Invalid text file"),
						   QObject::tr("This file is not a valid UltraStar nor PowerKaraoke lyric file") );
	return false;
}

static int powerKaraokeTime( QString time )
{
	int timing = 0;

	if ( time.contains(":") )
	{
		QStringList parts = time.split( ":" );
		timing = parts[0].toInt() * 60000;
		time = parts[1];
	}

	timing += (int) (time.toFloat() * 1000 );
	return timing;
}


bool Project::importLyricsKOK( const QStringList & readlyrics, Lyrics& lyrics )
{
	// J'ai ;7,9050634;tra;8,0651993;vail;8,2144914;lé;8,3789922;
	foreach ( QString line, readlyrics )
	{
		if ( line.isEmpty() )
			continue;

		QStringList entries = line.split( ";" );

		// Must be even
		if ( (entries.size() % 2) != 0 )
			return false;

		for ( int i = 0; i < entries.size() / 2; i++ )
		{
			QString text = entries[2 * i];
			QString timing = entries[2 * i + 1].replace( ",", "." );
			int timevalue = (int) ( timing.toFloat() * 1000 );

			lyrics.curLyricSetTime( timevalue );
			lyrics.curLyricAppendText( text );

			if ( i == entries.size() / 2 - 1 )
				lyrics.curLyricAddEndOfLine();
			else
				lyrics.curLyricAdd();
		}
	}

	return true;
}

bool Project::importLyricsPowerKaraoke( const QStringList & readlyrics, Lyrics& lyrics )
{
	// For the PowerKaraoke format there is no header, just times.
	QRegExp regex("^([0-9.:]+) ([0-9.:]+) (.*)");

	// Analyze each line
	for ( int i = 0; i < readlyrics.size(); i++ )
	{
		QString line = readlyrics[i];

		if ( line.isEmpty() )
			continue;

		// Try to match the sync first
		if ( line.indexOf( regex ) == -1 )
		{
			QMessageBox::critical( 0,
								   QObject::tr("Invalid PowerKaraoke file"),
								   QObject::tr("This file is not a valid PowerKaraoke lyric file, error at line %1") .arg( i + 1 ) );
			return false;
		}

		int start = powerKaraokeTime( regex.cap( 1 ) );
		//int end = powerKaraokeTime( regex.cap( 2 ) );
		QString text = regex.cap( 3 ).trimmed();

		lyrics.curLyricSetTime( start );

		if ( text.endsWith( "\\n" ) )
		{
			text.chop( 2 );
			lyrics.curLyricAppendText( text );
			lyrics.curLyricAddEndOfLine();
		}
		else if ( !text.endsWith( "-" ) )
		{
			text += " ";
			lyrics.curLyricAppendText( text );
			lyrics.curLyricAdd();
		}
		else
		{
			text.chop( 1 );
			lyrics.curLyricAppendText( text );
			lyrics.curLyricAdd();
		}
	}

	return true;
}

bool Project::importLyricsUStar( const QStringList & readlyrics, Lyrics& lyrics )
{
	bool header = true;
	bool relative = false;
	int bpm = -1, gap = -1;
	double msecs_per_beat = 0;
	int last_time_ms = 0;
	int next_time_ms = 0;
	int last_pitch = 0;

	for ( int i = 0; i < readlyrics.size(); i++ )
	{
		QString line = readlyrics[i];

		if ( header )
		{
			QRegExp regex( "^#([a-zA-Z]+):\\s*(.*)\\s*$" );

			if ( regex.indexIn( line ) != -1 )
			{
				QString tag = regex.cap( 1 );
				QString value = regex.cap( 2 );
				int tagid = -1;

				if ( tag == "TITLE" )
					tagid = PD_TAG_TITLE;
				else if ( tag == "ARTIST" )
					tagid = PD_TAG_ARTIST;
				else if ( tag == "LANGUAGE" )
					tagid = PD_TAG_LANGUAGE;
				else if ( tag == "GENRE" )
					tagid = PD_TAG_GENRE;
				else if ( tag == "MP3FILE" )
					tagid = PD_TAG_MP3FILE;
				else if ( tag == "COVER" )
					tagid = PD_TAG_COVER;
				else if ( tag == "BACKGROUND" )
					tagid = PD_TAG_BACKGROUND;
				else if ( tag == "VIDEO" )
					tagid = PD_TAG_VIDEO;
				else if ( tag == "VIDEOGAP" )
					tagid = PD_TAG_VIDEOGAP;
				else if ( tag == "EDITION" )
					tagid = PD_TAG_EDITION;
				else if ( tag == "BPM" )
					bpm = value.toInt();
				else if ( tag == "GAP" )
					gap = value.toInt();
				else if ( tag == "RELATIVE" )
					relative = value.compare( "yes" );
				else
					qDebug("Unsupported UltraStar tag found: '%s', ignored", qPrintable( tag ) );

				if ( tagid != -1 )
					update( tagid, value );
			}
			else
			{
				// Tag not found; either header ended, or invalid file
				if ( bpm == -1 || gap == -1 )
				{
					QMessageBox::critical( 0,
										   QObject::tr("Invalid UltraStar file"),
										   QObject::tr("This file is not a valid UltraStar lyric file; BPM and/or GAP is missing.") );
					return false;
				}

				msecs_per_beat = (int) ((60.0 / (double) bpm / 4.0) * 1000.0);
				header = false;
			}
		}

		// We may fall-through, so no else
		if ( !header )
		{
			if ( line[0] != 'E' && line[0] != ':' && line[0] != '*' && line[0] != 'F' && line[0] != '-' )
			{
				QMessageBox::critical( 0,
						   QObject::tr("Invalid UltraStar file"),
						   QObject::tr("This file is not a valid UltraStar lyric file; error at line %1.") .arg(i+1) );

				return false;
			}

			// End?
			if ( line[0] == 'E' )
				break;

			QStringList parsed = line.split( QRegExp("\\s+") );

			if ( parsed.size() < 3 )
			{
				QMessageBox::critical( 0,
									   QObject::tr("Invalid UltraStar file"),
									   QObject::tr("This file is not a valid UltraStar lyric file; error at line %1.").arg(i) );
				return false;
			}

			int timing = relative ? last_time_ms : 0;
			timing += parsed[1].toInt() * msecs_per_beat;

			// Should we add an empty field?
			if ( next_time_ms != 0 && timing > next_time_ms )
			{
				lyrics.curLyricSetTime( next_time_ms );
				lyrics.curLyricSetPitch( last_pitch );
				lyrics.curLyricAdd();
			}

			next_time_ms = timing + parsed[2].toInt() * msecs_per_beat;
			lyrics.curLyricSetTime( timing );

			if ( parsed[0] == "F" || parsed[0] == "*"  || parsed[0] == ":" )
			{
				if ( parsed.size() < 5 )
				{
					QMessageBox::critical( 0,
										   QObject::tr("Invalid UltraStar file"),
										   QObject::tr("This file is not a valid UltraStar lyric file; error at line %1.").arg(i) );
					return false;
				}

				int pitch = parsed[3].toInt();

				if ( parsed[0] == "F" )
					pitch |= Lyrics::PITCH_NOTE_FREESTYLE;
				else if ( parsed[0] == "*" )
					pitch |= Lyrics::PITCH_NOTE_GOLDEN;

				lyrics.curLyricSetPitch( pitch );
				lyrics.curLyricAppendText( parsed[4] );
			}
			else if ( parsed[0] == "-" )
				lyrics.curLyricAddEndOfLine();
			else
				abort(); // should never happen
		}
	}

	return true;
}

void Project::setSongLength( qint64 length )
{
	m_totalSongLength = length;
}

qint64 Project::getSongLength() const
{
	return m_totalSongLength;
}
