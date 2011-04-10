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

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "projectsettings.h"
#include "project.h"

ProjectSettings::ProjectSettings( Project* proj, bool showtype, QWidget * parent )
	: QDialog( parent ), Ui::DialogProjectSettings()
{
	setupUi( this );

	connect( btnBrowse, SIGNAL(clicked()), this, SLOT(browseMusicFile()) );
	connect( rbLRC1, SIGNAL( toggled(bool)), this, SLOT(changeProjectType()) );
	connect( rbLRC2, SIGNAL( toggled(bool)), this, SLOT(changeProjectType()) );
	connect( rbLRC3, SIGNAL( toggled(bool)), this, SLOT(changeProjectType()) );

	m_project = proj;
	m_musicFileChanged = false;
	m_generalTabShown = showtype;

	// Set the first tab accoring to options
	if ( showtype )
	{
		// Add values
		switch ( m_project->type() )
		{
			case Project::LyricType_LRC1:
				rbLRC1->setChecked( true );
				break;

			case Project::LyricType_LRC2:
				rbLRC2->setChecked( true );
				break;

			case Project::LyricType_UStar:
				rbLRC3->setChecked( true );
				break;
		}

		leSongFile->setText( m_project->musicFile() );
	}
	else
		tabSettings->removeTab( 0 );

	// Init general params
	leTitle->setText( m_project->tag( Project::Tag_Title ) );
	leArtist->setText( m_project->tag( Project::Tag_Artist ) );

	if ( m_project->type() == Project::LyricType_UStar )
	{
		if ( m_project->tag( Project::Tag_MP3File ).isEmpty() )
			leUSmp3->setText( QFileInfo( m_project->musicFile() ).fileName() );
		else
			leUSmp3->setText( m_project->tag( Project::Tag_MP3File ) );
	}

	// Init advanced params
	switch ( m_project->type() )
	{
		case Project::LyricType_LRC1:
		case Project::LyricType_LRC2:
			// Hide Ultrastar-specific fields
			labelUSbpmgap->hide();
			labelMusicFile->hide();
			leUSmp3->hide();

			// Hide ultrastar and CD+G group
			groupUStar->hide();

			// Add LRC params
			leLRCalbum->setText( m_project->tag( Project::Tag_Album ) );
			leLRCapp->setText( m_project->tag( Project::Tag_Application ) );
			leLRCappver->setText( m_project->tag( Project::Tag_Appversion ) );
			leLRCcreated->setText( m_project->tag( Project::Tag_CreatedBy ) );
			leLRCoffset->setText( m_project->tag( Project::Tag_Offset) );
			break;

		case Project::LyricType_UStar:
			// Hide LRC and CD+G groups
			groupLRC->hide();

			// Add UltraStar params
			leUSbackground->setText( m_project->tag( Project::Tag_Background ) );
			leUScover->setText( m_project->tag( Project::Tag_Cover ) );
			leUSedition->setText( m_project->tag( Project::Tag_Edition) );
			leUSgenre->setText( m_project->tag( Project::Tag_Genre ) );
			leUSlang->setText( m_project->tag( Project::Tag_Language ) );
			leUSvideo->setText( m_project->tag( Project::Tag_Video ) );
			leUSvideogap->setText( m_project->tag( Project::Tag_VideoGap ) );
			break;
	}

	// Scale down the dialog as part of it is hidden
	resize( QSize( width(), 1 ) );
}

void ProjectSettings::browseMusicFile()
{
	QString filename = QFileDialog::getOpenFileName( 0,
			tr("Choose a music file to load"), "." );

	if ( filename.isEmpty() )
		return;

	leSongFile->setText( filename );
}

void ProjectSettings::changeProjectType()
{
	// Init advanced params
	if ( rbLRC1->isChecked() || rbLRC2->isChecked() )
	{
		groupLRC->show();

		// Hide Ultrastar-specific fields
		labelUSbpmgap->hide();
		labelMusicFile->hide();
		leUSmp3->hide();

		// Hide ultrastar group
		groupUStar->hide();
	}
	else if ( rbLRC3->isChecked() )
	{
		// Hide LRC group
		groupLRC->hide();

		// Hide Ultrastar-specific fields
		labelUSbpmgap->show();
		labelMusicFile->show();
		leUSmp3->show();

		// Hide ultrastar group
		groupUStar->show();
	}
}

void ProjectSettings::accept()
{
	if ( m_generalTabShown )
	{
		// Music file
		if ( leSongFile->text().isEmpty() )
		{
			tabSettings->setCurrentIndex( 0 );
			QMessageBox::critical( 0, tr("Settings error"), tr( "Music file cannot be empty" ) );
			return;
		}

		if ( leSongFile->text() != m_project->musicFile() )
		{
			m_project->setMusicFile( leSongFile->text() );
			m_musicFileChanged = true;
		}

		// Lyric type
		if ( rbLRC1->isChecked() )
			m_project->setType( Project::LyricType_LRC1 );
		else if ( rbLRC2->isChecked() )
			m_project->setType( Project::LyricType_LRC2 );
		else if ( rbLRC3->isChecked() )
			m_project->setType( Project::LyricType_UStar );
	}

	// 2nd tab
	tabSettings->setCurrentIndex( 1 );

	if ( leTitle->text().isEmpty() )
	{
		QMessageBox::critical( 0,
							   tr("Settings error"),
							   tr("You must type song title to continue.") );
		return;
	}

	if ( leArtist->text().isEmpty() )
	{
		QMessageBox::critical( 0,
							   tr("Settings error"),
							   tr("You must type song artist to continue.") );
		return;
	}

	m_project->setTag( Project::Tag_Title, leTitle->text() );
	m_project->setTag( Project::Tag_Artist, leArtist->text() );

	if ( m_project->type() == Project::LyricType_UStar )
	{
		if ( leUSmp3->text().isEmpty() )
		{
			QMessageBox::critical( 0,
								   tr("Settings error"),
								   tr("You must enter MP3 file name to continue.") );
			return;
		}

		m_project->setTag( Project::Tag_MP3File, leUSmp3->text() );
	}

	// 3rd tab
	tabSettings->setCurrentIndex( 2 );

	if ( m_project->type() != Project::LyricType_UStar )
	{
		m_project->setTag( Project::Tag_Album, leLRCalbum->text() );
		m_project->setTag( Project::Tag_Application, leLRCapp->text() );
		m_project->setTag( Project::Tag_Appversion, leLRCappver->text() );
		m_project->setTag( Project::Tag_CreatedBy, leLRCcreated->text() );
		m_project->setTag( Project::Tag_Offset, leLRCoffset->text() );
	}
	else
	{
		m_project->setTag( Project::Tag_Background, leUSbackground->text() );
		m_project->setTag( Project::Tag_Cover, leUScover->text() );
		m_project->setTag( Project::Tag_Edition, leUSedition->text() );
		m_project->setTag( Project::Tag_Genre, leUSgenre->text() );
		m_project->setTag( Project::Tag_Language, leUSlang->text() );
		m_project->setTag( Project::Tag_Video, leUSvideo->text() );
		m_project->setTag( Project::Tag_VideoGap, leUSvideogap->text() );
	}

	QDialog::accept();
}
