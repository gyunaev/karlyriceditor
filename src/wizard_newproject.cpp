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

#include <QDesktopServices>
#include <QWhatsThis>
#include <QMessageBox>
#include <QFileDialog>

#include "mediaplayer.h"
#include "project.h"
#include "version.h"
#include "wizard_newproject.h"
#include "kfn_file_parser.h"
#include "settings.h"
#include "util.h"

namespace WizardNewProject
{

//
// "Select lyrics format" page
//
PageLyricType::PageLyricType( Project * project, QWidget *parent )
	: QWizardPage( parent ), Ui::WizNewProject_LyricType()
{
	setupUi( this );

	m_project = project;
	connect( lblHelp, SIGNAL( linkActivated ( const QString &) ), this, SLOT( showhelp() ) );
}

bool PageLyricType::validatePage()
{
	// At least one should be selected
	if ( rbLRC1->isChecked() )
		m_project->setType( Project::LyricType_LRC1 );
	else if ( rbLRC2->isChecked() )
		m_project->setType( Project::LyricType_LRC2 );
	else if ( rbLRC3->isChecked() )
		m_project->setType( Project::LyricType_UStar );
	else
		return false;

	return true;
}

void PageLyricType::showhelp()
{
	QString help = tr( "LRC1 is the first version of LRC format, containing a single "
					   "line with timing mark at the beginning. This format is supported by most players\n\n"
					   "LRC2 is the second version, which can contain timing marks inside "
					   "the line. Supported by less players.\n\n"
					   "UStar/UltraStar format is lyrics format used in SingStar, Sinatra, Performous and "
					   "similar games.\n\nXBMC supports all those formats" );

	QWhatsThis::showText ( mapToGlobal( lblHelp->pos() ), help );
}


//
// "Choose music file" page
//
PageMusicFile::PageMusicFile( Project * project, QWidget *parent )
	: QWizardPage( parent ), Ui::WizNewProject_MusicFile()
{
	setupUi( this );

	m_project = project;

    m_mediaPlayer = new MediaPlayer();
    connect( m_mediaPlayer, SIGNAL(mediaLoadingFinished(MediaPlayer::State,QString)), this,SLOT(mediaLoadingFinished(MediaPlayer::State,QString)) );
    connect( m_mediaPlayer, SIGNAL(tagsChanged(QString,QString)), this,SLOT(mediaTagsChanged(QString,QString)) );

	connect( btnBrowse, SIGNAL( clicked() ), this, SLOT( browse() ) );
}

PageMusicFile::~PageMusicFile()
{
}

void PageMusicFile::browse()
{
	QString filename = QFileDialog::getOpenFileName( 0,
            tr("Choose a music file to load"),
            pSettings->m_LastUsedDirectory );

	if ( filename.isEmpty() )
		return;

	// We cannot handle MIDI/KAR files
	if ( filename.endsWith( ".mid", Qt::CaseInsensitive ) || filename.endsWith( ".kar", Qt::CaseInsensitive ) )
	{
		if ( QMessageBox::question( 0,
						   tr("Trying to open a MIDI file?"),
						   tr("It looks like you are trying to open the MIDI file.\n"
							  "MIDI file contains the lyrics embedded into the file in a special format. "
							  "%1 cannot edit regular MIDI files, it is recommended to use RoseGarden instead.\n\n"
							  "Are you sure you still want to try to open this file?") .arg( APP_NAME ),
								   QMessageBox::Yes | QMessageBox::No, QMessageBox::No )
				== QMessageBox::No )
		{
			return;
		}
	}

	// Handle the KFN file
	if ( filename.endsWith( ".kfn",Qt::CaseInsensitive  ) )
	{
		if ( QMessageBox::question( 0,
						   tr("Trying to open a KaraFun file?"),
						   tr("It looks like you are trying to open the KaraFun file.\n"
							  "Karaoke Lyrics Editor cannot edit those files directly. However it can import music and lyrics from this file, and export them as any supported format "
							  "such as CDG, LRC or UltraStar (but not KFN)\n\n"
							  "Do you want to import the music and lyrics from this file?"),
								   QMessageBox::Yes | QMessageBox::No, QMessageBox::No )
				== QMessageBox::No )
		{
			return;
		}

		KFNFileParser parser;

		if ( !parser.open( filename ) )
		{
			QMessageBox::information( 0,
							   tr("Invalid KFN file"),
							   tr("The file %1 cannot be opened: %2") .arg( filename ) .arg( parser.errorMsg() ) );
			return;
		}

		// Music file without .KFN extention
		QString musicFileName = filename.left( filename.size() - 3 ) + parser.musicFileExtention();
		QFile musicFile( musicFileName );

		if ( !musicFile.open( QIODevice::WriteOnly ) )
		{
			QMessageBox::information( 0,
							   tr("Cannot import KFN file"),
							   tr("The file %1 cannot be imported: the music file %2 cannot be created") .arg( filename ) .arg( musicFileName ) );
			return;
		}

		if ( !parser.writeMusicFile(musicFile) )
		{
			QMessageBox::information( 0,
							   tr("Cannot import KFN file"),
							   tr("The file %1 cannot be imported: the music file %2 cannot be stored: %s") .arg( filename ) .arg( musicFileName ) .arg( parser.errorMsg() ) );
			return;
		}

		musicFile.close();
		filename = musicFileName;

		// Lyrics
		m_hasLrcLyrics = parser.lyricsAsLRC();

		// Still fall through
		if ( m_hasLrcLyrics.isEmpty() )
			QMessageBox::information( 0,
							   tr("Cannot import lyrics"),
							   tr("The file %1 cannot be imported: the lyrics cannot be parsed: %s") .arg( filename ) .arg( parser.errorMsg() ) );
	}

    // Try to open it; we will receive signals
    m_lastMusicFile = filename;
    m_mediaPlayer->loadMedia( filename, MediaPlayer::LoadAudioStream );
}

void PageMusicFile::mediaLoadingFinished(MediaPlayer::State state, QString text)
{
    if ( state == MediaPlayer::StateFailed )
    {
        QMessageBox::critical( 0,
                               tr("Cannot open the music file"),
                               tr("Cannot open the music file.\n\n%1") .arg(text) );

        m_lastMusicFile.clear();
    }
    else
    {
        // Store the music file
        leSongFile->setText( m_lastMusicFile );

        // If there's an LRC file nearby, ask whether the user wants to load it (and get the values from it)
        if ( !m_hasLrcLyrics.isEmpty() )
            return;

        QString lrcfile = Util::removeFileExtention( m_lastMusicFile ) + "lrc";
        QFile file( lrcfile );

        if ( file.open( QIODevice::ReadOnly ) )
        {
            if ( QMessageBox::question( 0,
                               tr("Lyrics file found"),
                               tr("It looks like there is a lyrics file %1 matching this music file.\n\n"
                                  "Do you want to import it as well?") .arg(lrcfile),
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No )
                    == QMessageBox::Yes )
            {
                m_hasLrcLyrics = Util::convertWithUserEncoding( file.readAll() );

            }
        }

        // Store the last used import directory
        pSettings->updateLastUsedDirectory( QDir(m_lastMusicFile).dirName() );
    }
}


void PageMusicFile::mediaTagsChanged(QString artist, QString title)
{
    leArtist->setText( artist );
    leTitle->setText( title );
}


bool PageMusicFile::validatePage()
{
	if ( leSongFile->text().isEmpty() )
	{
		QMessageBox::critical( 0,
							   tr("Music file not selected"),
							   tr("You must select a music file to continue.") );
		return false;
	}

	if ( leTitle->text().isEmpty() )
	{
		QMessageBox::critical( 0,
							   tr("Title field is empty"),
							   tr("You must type song title to continue.") );
		return false;
	}

	if ( leArtist->text().isEmpty() )
	{
		QMessageBox::critical( 0,
							   tr("Artist field is empty"),
							   tr("You must type song artist to continue.") );
		return false;
	}

	m_project->setMusicFile( leSongFile->text() );
	m_project->setTag( Project::Tag_Title, leTitle->text() );
	m_project->setTag( Project::Tag_Artist, leArtist->text() );

	if ( !leAlbum->text().isEmpty() )
		m_project->setTag( Project::Tag_Album, leAlbum->text() );

	if ( !m_hasLrcLyrics.isEmpty() )
		m_project->convertLyrics( m_hasLrcLyrics );

	return true;
}



Wizard::Wizard( Project * project, QWidget *parent )
	: QWizard( parent )
{
	addPage( new PageIntro( project, this ) );
	addPage( new PageLyricType( project, this ) );
	addPage( new PageMusicFile( project, this ) );
	addPage( new PageFinish( project, this ) );

#ifndef Q_WS_MAC
	setWizardStyle(ModernStyle);
#endif

	setOption( HaveHelpButton, false );

	setWindowTitle( tr("New karaoke lyrics project") );
	setPixmap( QWizard::WatermarkPixmap, QPixmap(":/images/casio.jpg") );
}


} // namespace
