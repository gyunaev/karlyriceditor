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

#include <QTime>
#include <QToolBar>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QWhatsThis>
#include <QDateTime>
#include <QColorDialog>
#include <QDesktopServices>
#include <QUrl>

#include "audioplayer.h"
#include "wizard_newproject.h"
#include "mainwindow.h"
#include "playerwidget.h"
#include "project.h"
#include "settings.h"
#include "viewwidget.h"
#include "testwindow.h"
#include "version.h"
#include "projectsettings.h"
#include "recentfiles.h"
#include "gentlemessagebox.h"
#include "lyricswidget.h"
#include "ui_dialog_about.h"
#include "videogenerator.h"
#include "cdggenerator.h"
#include "videoencodingprofiles.h"
#include "licensing.h"
#include "util.h"
#include "ui_dialog_registration.h"

MainWindow * pMainWindow;


MainWindow::MainWindow()
	: QMainWindow(), Ui::MainWindow()
{
	// A lot of widgets use pMainWindow in constructors
	pMainWindow = this;

	// Settings
	pSettings = new Settings();

	// Call UIC-generated code
	setupUi( this );

	// Video profiles
	pVideoEncodingProfiles = new VideoEncodingProfiles();

	// Initialize stuff
	m_project = 0;
	m_testWindow = 0;

	// Licensing
	pLicensing = new Licensing();

	if ( pLicensing->init() )
	{
		QString key = QSettings().value( "general/registrationkey", "" ).toString();
		pLicensing->validate( key );
	}

	// Create dock widgets
	m_player = new PlayerWidget( this );
	addDockWidget( Qt::BottomDockWidgetArea, m_player );
	actionShow_Player_dock_wingow->setChecked( true );

	// Create a lyric viewer window, hidden so far
	m_viewer = new ViewWidget( this );

	// Recent files
	m_recentFiles = new RecentFiles( menuFile, actionQuit );
	connect( m_recentFiles, SIGNAL( openRecentFile(const QString&) ), this, SLOT( openRecentFile( const QString&)) );

	// Do the rest
	connectActions();
	createToolbars();

	// Add WhatsThis action to the help menu
	QAction * whatsthis = QWhatsThis::createAction( this );
	menuHelp->insertAction( actionAbout, whatsthis );

	// Validator icons
	m_validatorIconRegular.addFile( ":/images/dryicons_application_search.png", QSize(), QIcon::Normal, QIcon::Off );
	m_validatorIconAccepted.addFile( ":/images/dryicons_application_accept.png", QSize(), QIcon::Normal, QIcon::Off );
	m_validatorIconFailed.addFile( ":/images/dryicons_application_deny.png", QSize(), QIcon::Normal, QIcon::Off );

	// Restore current directory
	QDir::setCurrent( QSettings().value( "general/currentdirectory", "." ).toString() );

	// Update window state
	updateState();

	checkNewVersionAvailable();
}

MainWindow::~MainWindow()
{
}

void MainWindow::checkNewVersionAvailable()
{
	QSettings settings;

	if ( !pSettings->m_checkForUpdates )
		return;

	if ( settings.contains( "advanced/lastupdate" ) )
	{
		QDateTime lastupdate = settings.value( "advanced/lastupdate" ).toDateTime();

		if ( lastupdate.secsTo( QDateTime::currentDateTime() ) < 86400 )
			return;
	}

	// Create a New version available object if necessary. This object will auto-delete itself
	CheckNewVersion * pNewVer = new CheckNewVersion();

    connect( pNewVer, SIGNAL(error(int)), this, SLOT(newVerAvailError(int)), Qt::QueuedConnection );
    connect( pNewVer, SIGNAL(newVersionAvailable( NewVersionMetaMap )), this, SLOT(newVerAvailable(NewVersionMetaMap)), Qt::QueuedConnection );

    pNewVer->setUrl( "http://www.ulduzsoft.com/karlyriceditor_latestversion.txt" );
	pNewVer->setCurrentVersion( QString("%1.%2").arg( APP_VERSION_MAJOR ) . arg( APP_VERSION_MINOR ) );
    pNewVer->start();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	pAudioPlayer->stop();

	if ( m_project && !tryCloseCurrentProject() )
	{
		event->ignore();
		return;
	}

	// Save current directory
	QSettings().setValue( "general/currentdirectory", QDir::currentPath() );

	QMainWindow::closeEvent( event );
	event->accept();
}

void MainWindow::connectActions()
{
	// All those actions are defined in .ui file
	connect( actionNew_project, SIGNAL( triggered()), this, SLOT( act_fileNewProject()) );
	connect( action_openProject, SIGNAL( triggered()), this, SLOT( act_fileOpenProject()) );
	connect( actionSave, SIGNAL( triggered()), this, SLOT( act_fileSaveProject()) );
	connect( actionSave_as, SIGNAL( triggered()), this, SLOT( act_fileSaveProjectAs()) );
	connect( actionInsert_tag, SIGNAL( triggered()), this, SLOT( act_editInsertTag()) );
	connect( actionRemove_tag, SIGNAL( triggered()), this, SLOT( act_editRemoveTag()) );
	connect( actionRemove_all_tags, SIGNAL( triggered()), this, SLOT(act_editRemoveAllTags()) );
	connect( actionOpen_lyric_file, SIGNAL( triggered()), this, SLOT(act_projectOpenLyricFile()) );
	connect( actionQuit, SIGNAL( triggered()), qApp, SLOT( quit()) );
	connect( actionClear_text, SIGNAL( triggered()), this, SLOT( act_editClearText()) );
	connect( actionTrimspaces, SIGNAL( triggered()), this, SLOT( act_editTrimspaces()) );
	connect( actionGeneral, SIGNAL( triggered()), this, SLOT(act_settingsGeneral()) );
	connect( actionSplit_current_line, SIGNAL( triggered()), this, SLOT(act_editSplitLine()) );
	connect( actionAbout, SIGNAL( triggered()), this, SLOT(act_helpAbout()) );
	connect( actionProject_settings, SIGNAL( triggered()), this, SLOT(act_projectSettings()) );
	connect( actionExport_lyric_file, SIGNAL( triggered()), this, SLOT(act_projectExportLyricFile()) );
	connect( actionExport_video_file, SIGNAL( triggered()), this, SLOT(act_projectExportVideoFile()) );
	connect( actionExport_CD_G_file, SIGNAL( triggered()), this, SLOT(act_projectExportCDGFile()) );
	connect( actionEdit_header_data, SIGNAL( triggered()), this, SLOT( act_projectEditHeader()) );
	connect( actionValidate_lyrics, SIGNAL( triggered()), this, SLOT( act_projectValidateLyrics()) );
	connect( actionView_lyric_file, SIGNAL( triggered()), this, SLOT( act_projectViewLyricFile()) );
	connect( actionTest_lyric_file, SIGNAL( triggered()), this, SLOT( act_projectTest()) );
	connect( actionTest_CDG_lyrics, SIGNAL( triggered()), this, SLOT( act_projectTestCDG()) );
	connect( actionShow_Player_dock_wingow, SIGNAL(triggered(bool)), this, SLOT(act_settingsShowPlayer(bool)) );
    connect( actionInsert_picture, SIGNAL(triggered()), this, SLOT(act_editInsertPicture() ) );
    connect( actionInsert_video, SIGNAL(triggered()), this, SLOT(act_editInsertVideo() ) );
	connect( actionInsert_color_change, SIGNAL(triggered(bool)), this, SLOT(act_editInsertColorChange() ) );
    connect( actionInsert_background_color_change, SIGNAL(triggered()), this, SLOT(act_editInsertBackgroundColorChange()) );
    connect( actionAdd_eol_timing_marks, SIGNAL(triggered()), this, SLOT(act_addMissingTimingMarks() ) );
    connect( actionTime_adjustment, SIGNAL(triggered()), this, SLOT(act_adjustTiming() ) );

	// docks
	connect( m_player,SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityPlayer(bool)) );

	// Text editor
	connect( actionUndo, SIGNAL( triggered()), editor, SLOT( undo()) );
	connect( actionRedo, SIGNAL( triggered()), editor, SLOT( redo()) );
    connect( editor, SIGNAL(lyricsChanged(qint64)), this, SLOT(lyricsChanged(qint64)) );

	// Registration
	connect( actionRegistration, SIGNAL( triggered()), this, SLOT( act_helpRegistration()) );

	if ( !pLicensing->isEnabled() )
		actionRegistration->setVisible( false );
}

void MainWindow::createToolbars()
{
	// Main toolbar
	QToolBar * main = addToolBar( "Main toolbar" );
	main->addAction( actionNew_project );
	main->addAction( actionSave );

	// Editor toolbar
	QToolBar * editor = addToolBar( "Editor toolbar" );
	editor->addAction( actionUndo );
	editor->addAction( actionRedo );
	editor->addSeparator();
	editor->addAction( actionInsert_tag );
	editor->addAction( actionRemove_tag );
	editor->addSeparator();
	editor->addAction( actionValidate_lyrics );
	editor->addAction( actionView_lyric_file );
	editor->addAction( actionTest_lyric_file );

	// Style for all main window toolbars
	setIconSize ( QSize( 32, 32 ) );
	setToolButtonStyle ( Qt::ToolButtonTextUnderIcon );
}

void MainWindow::editor_undoAvail(bool available)
{
	actionUndo->setEnabled(available);
}

void MainWindow::editor_redoAvail(bool available)
{
    actionRedo->setEnabled( available );
}

void MainWindow::lyricsChanged(qint64 time)
{
    if ( !pSettings->m_editorAutoUpdateTestWindows )
        return;

    if ( m_testWindow )
    {
        Lyrics lyrics;

        if ( !editor->exportLyrics( &lyrics ) )
            return;

        LyricsWidget * lw = new LyricsWidget( m_testWindow );
        lw->setLyrics( lyrics,
                         m_project->tag( Project::Tag_Artist ),
                         m_project->tag( Project::Tag_Title ) );

        m_testWindow->setLyricWidget( lw );

        if ( m_player->isPlaying() && pSettings->m_editorAutoUpdatePlayerBackseek > 0 )
            m_player->seekToTime( time - pSettings->m_editorAutoUpdatePlayerBackseek * 1000 );
    }
}

void MainWindow::act_fileNewProject()
{
	if ( m_project )
	{
		if ( QMessageBox::question( 0,
				tr("Close existing project"),
				tr("Do you really want to close existing project, and create a new project?"),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No ) == QMessageBox::No )
			return;

		if ( !tryCloseCurrentProject() )
			return;
	}

	m_projectFile.clear();
	Project * newproj = new Project( editor );

	// Show the new project wizard
	WizardNewProject::Wizard wi( newproj, this );

	if ( wi.exec() == QDialog::Rejected )
	{
		delete newproj;
		return;
	}

	// Save the project
	m_project = newproj;

	if ( !act_fileSaveProjectAs() )
	{
		delete newproj;
		return;
	}

	setCurrentProject( newproj );
}

void MainWindow::act_fileOpenProject()
{
	if ( m_project )
	{
		if ( QMessageBox::question( 0,
				tr("Close existing project"),
				tr("Do you really want to close existing project, and create a new project?"),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::No ) == QMessageBox::No )
			return;

		if ( !tryCloseCurrentProject() )
			return;
	}

	QString fileName = QFileDialog::getOpenFileName( this,
			tr("Open a project file"),
            QString(),
			tr("Project files (*.kleproj)") );

	if ( fileName.isEmpty() )
		return;

	loadProject( fileName );
}

void MainWindow::openRecentFile( const QString& file )
{
	loadProject( file );
}

bool MainWindow::loadProject( const QString& fileName )
{
	Project * newproj = new Project( editor );

	if ( !newproj->load( fileName ) )
	{
		m_recentFiles->removeRecentFile( fileName );
		delete newproj;
		return false;
	}

	m_recentFiles->setCurrentFile( fileName );
	m_projectFile = fileName;
	setCurrentProject( newproj );

	// Change the current dir
	QFileInfo finfo( fileName );
	QDir::setCurrent( finfo.path() );

	return true;
}

bool MainWindow::act_fileSaveProject()
{
	if ( m_projectFile.isEmpty() )
		return act_fileSaveProjectAs();
	else
		return saveProject( m_projectFile );
}

bool MainWindow::act_fileSaveProjectAs()
{
	QString suggestedFile = m_projectFile;

	if ( suggestedFile.isEmpty() )
        suggestedFile = Util::removeFileExtention( m_project->musicFile() ) + "kleproj";

	QString fileName = QFileDialog::getSaveFileName( this,
			tr("Save as a project file"),
			suggestedFile,
			tr("Project files (*.kleproj)") );

	if ( fileName.isEmpty() )
		return false;

	if ( !saveProject( fileName ) )
		return false;

	m_projectFile = fileName;
	setWindowTitle( tr("%1[*] - Lyric Editor") .arg( m_projectFile ) );

	// Change the current dir
	QFileInfo finfo( fileName );
	QDir::setCurrent( finfo.path() );

	return true;
}

bool MainWindow::saveProject( const QString& fileName )
{
	bool res = m_project->save( fileName );
	updateState();

	if ( res )
		m_recentFiles->setCurrentFile( fileName );

	return res;
}

bool MainWindow::tryCloseCurrentProject()
{
	if ( m_project->isModified() )
	{
		int rc = QMessageBox::question( 0,
			tr("Unsaved changes"),
			tr("The current project has unsaved changes. Do you want to save them?"),
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel );

		if ( rc == QMessageBox::Cancel )
			return false;

		if ( rc == QMessageBox::Yes )
		{
			if ( !act_fileSaveProject() )
				return false;
		}
	}

	delete m_project;
	m_project = 0;

	editor->setProject( 0 );
	editor->clear();
	updateState();

	return true;
}

void MainWindow::setCurrentProject( Project * proj )
{
	m_project = proj;

	setWindowTitle( tr("%1[*] - Lyric Editor") .arg( m_projectFile ) );

	statusBar()->showMessage( tr("Loading the music file %1") .arg(m_project->musicFile()), 2000);

	// Set the music file into player
	if ( m_player->openMusicFile( m_project ) )
	{
		qint64 totaltime = m_player->totalTime();

		if ( totaltime > 0 )
			m_project->setSongLength( totaltime );
	}
}

void MainWindow::act_editInsertTag()
{
  editor->insertTimeTag( m_player->currentTime() );
}

void MainWindow::act_editSplitLine()
{
	editor->splitLine();
}

void MainWindow::act_editRemoveTag()
{
	editor->removeLastTimeTag();
}

void MainWindow::act_editInsertPicture()
{
	QString fileName = QFileDialog::getOpenFileName( this,
			tr("Open an image file"),
            QString(),
            QString() );

	if ( fileName.isEmpty() )
		return;

	editor->insertImageTag( fileName );
}

void MainWindow::act_editInsertVideo()
{
	QString fileName = QFileDialog::getOpenFileName( this,
			tr("Open a video file"),
            QString(),
            QString() );

	if ( fileName.isEmpty() )
		return;

	editor->insertVideoTag( fileName );
}

void MainWindow::act_editInsertColorChange()
{
	QColor newcolor = QColorDialog::getColor();

	if ( newcolor.isValid() )
        editor->insertColorChangeTag( newcolor.name() );
}

void MainWindow::act_editInsertBackgroundColorChange()
{
    QColor newcolor = QColorDialog::getColor();

    if ( newcolor.isValid() )
        editor->insertBackgroundColorChangeTag( newcolor.name() );
}

void MainWindow::act_editClearText()
{
	if ( QMessageBox::question( 0,
			tr("Clearing all the text"),
			tr("Are you sure you want to clear the text?\nThis operation cannot be undone!"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) == QMessageBox::Yes  )
		editor->clear();
}

void MainWindow::act_editTrimspaces()
{
	editor->removeExtraWhitespace();
}

void MainWindow::act_editRemoveAllTags()
{
	editor->removeAllTimeTags();
}

void MainWindow::act_projectViewLyricFile()
{
	if ( !editor->validate() )
		return;

	QString lyrics = m_project->exportLyrics();

	if ( lyrics.isEmpty() )
		return;

	m_viewer->showText( lyrics );
}

void MainWindow::act_projectValidateLyrics()
{
	if ( editor->validate() )
		actionValidate_lyrics->setIcon( m_validatorIconAccepted );
	else
		actionValidate_lyrics->setIcon( m_validatorIconFailed );
}

void MainWindow::act_projectOpenLyricFile()
{
	QString fileName = QFileDialog::getOpenFileName( this,
			tr("Open a lyric file"),
            QString(),
            tr("LRC files (*.lrc);;UltraStar/PowerKaraoke/KaraokeBuilder files (*.txt);;KAR/MIDI files (*.mid *.midi *.kar);;KaraFun files (*.kfn);;DEL Karaoke files (*.kok)") );

	if ( fileName.isEmpty() )
		return;

	m_project->importLyrics( fileName );
}

void MainWindow::act_projectEditHeader()
{
	ProjectSettings ps( m_project, false, this );
	ps.setWindowTitle( tr("Edit lyric header data" ) );
	ps.exec();
}

void MainWindow::act_projectExportLyricFile()
{
	if ( !editor->validate() )
		return;

	QString filter_LRC1 = tr("LRC version 1 (*.lrc1)");
	QString filter_LRC2 = tr("LRC version 2 (*.lrc)");
	QString filter_UStar = tr("UltraStar (*.txt)");

    QString filter, selected, outfilter, extension;

	switch ( m_project->type() )
	{
		case Project::LyricType_LRC1:
			filter = filter_LRC1 + ";;" + filter_LRC2 + ";;" + filter_UStar;
			selected = filter_LRC1;
            extension = "lrc";
			break;

		case Project::LyricType_LRC2:
			filter = filter_LRC2 + ";;" + filter_LRC1 + ";;" + filter_UStar;
			selected = filter_LRC2;
            extension = "lrc";
			break;

		case Project::LyricType_UStar:
			filter = filter_UStar + ";;" + filter_LRC2 + ";;" + filter_LRC1;
			selected = filter_UStar;
            extension = "txt";
			break;
	}

    QString outfile = QFileDialog::getSaveFileName( 0,
                                                    tr("Export lyrics to a file"),
                                                    Util::removeFileExtention( m_project->musicFile() ) + extension,
                                                    filter,
                                                    &outfilter );
	QByteArray lyrics;

	if ( outfile.isEmpty() )
		return;

	if ( outfilter == filter_LRC1 )
		lyrics = m_project->exportLyricsAsLRC1();
	else if ( outfilter == filter_LRC2 )
		lyrics = m_project->exportLyricsAsLRC2();
	else if ( outfilter == filter_UStar )
		lyrics = m_project->exportLyricsAsUStar();
	else
	{
		QMessageBox::critical( 0, "Unknown filter", "Unknown filter" );
		return;
	}

	if ( lyrics.isEmpty() )
		return;

	QFile file( outfile );
	if ( !file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		QMessageBox::critical( 0,
							   tr("Cannot write lyric file"),
							   tr("Cannot write lyric file %1: %2") .arg(outfile) .arg(file.errorString()) );
		return;
	}

	file.write( lyrics );
}

void MainWindow::act_projectSettings()
{
	ProjectSettings ps( m_project, true, this );
	ps.setWindowTitle( tr("Edit project settings" ) );

	if ( ps.exec() == QDialog::Accepted )
	{
		if ( ps.musicFileChanged() )
			m_player->openMusicFile( m_project );
	}
}

void MainWindow::act_settingsGeneral()
{
	pSettings->edit();
}


void MainWindow::act_helpAbout()
{
	QDialog dlg;
	Ui::DialogAbout ui_about;

	ui_about.setupUi( &dlg );

	ui_about.labelAbout->setText( tr("<b>Karaoke Lyrics Editor version %1.%2</b><br><br>"
			"Copyright (C) George Yunaev 2009-2013, <a href=\"mailto:support@ulduzsoft.com\">support@ulduzsoft.com</a><br><br>"
			"Web site: <a href=\"http://www.ulduzsoft.com\">www.ulduzsoft.com/karlyriceditor</a><br><br>"
			"This program is licensed under terms of GNU General Public License "
			"version 3; see LICENSE file for details.") .arg(APP_VERSION_MAJOR) .arg(APP_VERSION_MINOR) );

	dlg.exec();
}

void MainWindow::updateState()
{
	bool project_available = m_project ? true : false;
	bool project_ready = m_project ? m_player->isReady() : false;

	actionSave->setEnabled( project_available );
	actionSave_as->setEnabled( project_available );
	actionUndo->setEnabled( project_available );
	actionRedo->setEnabled( project_available );
	actionInsert_tag->setEnabled( project_ready );
	actionRemove_tag->setEnabled( project_ready );
	actionEdit_header_data->setEnabled( project_available );
	actionValidate_lyrics->setEnabled( project_available );
	actionTest_lyric_file->setEnabled( project_ready );
	actionTest_CDG_lyrics->setEnabled( project_ready );
	actionRemove_all_tags->setEnabled( project_available );
	actionOpen_lyric_file->setEnabled( project_available );
	actionClear_text->setEnabled( project_available );
	actionTrimspaces->setEnabled( project_available );
	actionInsert_picture->setEnabled( project_available );
	actionInsert_video->setEnabled( project_available );
	actionExport_lyric_file->setEnabled( project_available );
	actionExport_video_file->setEnabled( project_available );
	actionExport_CD_G_file->setEnabled( project_available );
	actionEdit_header_data->setEnabled( project_available );
	actionProject_settings->setEnabled( project_available );
	actionView_lyric_file->setEnabled( project_ready );
	actionAdd_eol_timing_marks->setEnabled( project_ready );

	editor->setEnabled( project_available );

	if ( project_ready )
	{
		if ( m_project->isModified() )
		{
			actionValidate_lyrics->setIcon( m_validatorIconRegular );
			setWindowModified( true );
			actionSave->setEnabled( true );
		}
		else
		{
			setWindowModified( false );
			actionSave->setEnabled( false );
		}
	}
}

void MainWindow::newVerAvailError( int  )
{
	statusBar()->showMessage( tr("Unable to check whether a new version is available"), 2000 );
}

void MainWindow::newVerAvailable( NewVersionMetaMap metadata )
{
	QSettings().setValue( "advanced/lastupdate", QDateTime::currentDateTime() );

	if ( QMessageBox::question( 0,
			tr("New version available"),
			tr("<html>A new version <b>%1</b> of Karaoke Lyrics Editor is available!\n\n"
			   "Do you want to visit the application web site %2?")
					.arg( metadata["Version"] )
					.arg( metadata["URL"] ),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::Yes ) == QMessageBox::No )
			return;

	QDesktopServices::openUrl ( QUrl(metadata["URL"]) );
}

void MainWindow::act_settingsShowPlayer( bool checked )
{
	if ( checked )
		m_player->show();
	else
		m_player->hide();
}

void MainWindow::visibilityPlayer( bool visible )
{
	actionShow_Player_dock_wingow->setChecked( visible );
}

void MainWindow::act_projectTest()
{
	if ( !editor->validate() )
		return;

	Lyrics lyrics;

	if ( !editor->exportLyrics( &lyrics ) )
		return;

	if ( !m_testWindow )
	{
		m_testWindow = new TestWindow( this );

		connect( m_player, SIGNAL(tick(qint64)), m_testWindow, SLOT(tick(qint64)) );
		connect( m_testWindow, SIGNAL(closed()), this, SLOT( testWindowClosed() ) );
        connect( m_testWindow, SIGNAL(editorTick(qint64)), editor, SLOT(followingTick(qint64)) );
	}

	LyricsWidget * lw = new LyricsWidget( m_testWindow );

	lw->setLyrics( lyrics,
					 m_project->tag( Project::Tag_Artist ),
					 m_project->tag( Project::Tag_Title ) );

	m_testWindow->setLyricWidget( lw );
	m_testWindow->show();

	// Added the pause to make the test lyrics button simply reload the lyrics and continue
	// playing where it is at, making testing easier
	m_player->btn_playerPlayPause();
	m_player->startPlaying();
}

void MainWindow::testWindowClosed()
{
	m_player->btn_playerStop();

	delete m_testWindow;
	m_testWindow = 0;
}

void MainWindow::act_projectTestCDG()
{
	QString fileName = QFileDialog::getOpenFileName( this,
			tr("Open a CD+G file"),
			".",
			tr("CD+G (*.cdg)") );

	if ( fileName.isEmpty() )
		return;

	QFile f( fileName );

	if ( !f.open( QIODevice::ReadOnly ) )
		return;

	QByteArray cdgdata = f.readAll();

	if ( cdgdata.isEmpty() )
		return;

	if ( !m_testWindow )
	{
		m_testWindow = new TestWindow( this );
		connect( m_player, SIGNAL(tick(qint64)), m_testWindow, SLOT(tick(qint64)) );
		connect( m_testWindow, SIGNAL(closed()), this, SLOT( testWindowClosed() ) );
	}

	LyricsWidget * lw = new LyricsWidget( m_testWindow );
	lw->setCDGdata( cdgdata );

	m_testWindow->setLyricWidget( lw );
	m_testWindow->show();

	m_player->startPlaying();
}


void MainWindow::act_projectExportVideoFile()
{
	if ( !editor->validate() )
		return;

	Lyrics lyrics;

	if ( !editor->exportLyrics( &lyrics ) )
		return;

	// Stop the player if playing
	m_player->btn_playerStop();

	VideoGenerator videogen( m_project );
	videogen.generate( lyrics, m_player->totalTime() );
}

void MainWindow::act_projectExportCDGFile()
{
	if ( !editor->validate() )
		return;

	Lyrics lyrics;

	if ( !editor->exportLyrics( &lyrics ) )
		return;

	// Stop the player if playing
	m_player->btn_playerStop();

	CDGGenerator gen( m_project );
	gen.generate( lyrics, m_player->totalTime() );
}


void MainWindow::act_helpRegistration()
{
	if ( !pLicensing->isEnabled() )
		return;

	if ( !pLicensing->isValid() )
	{
		while ( 1 )
		{
			// Prepare the dialog
			QDialog dlg( this );
			Ui::DialogRegistration ui;
			ui.setupUi( &dlg );

			// Hide the registration info form and shrink the dialog
			ui.groupShowInfo->hide();
			dlg.adjustSize();
			dlg.setWindowTitle( tr("Enter registration key") );

			if ( dlg.exec() != QDialog::Accepted )
				return;

			QString key = ui.leKey->toPlainText();

			if ( pLicensing->validate( key ) )
			{
				QSettings().setValue( "general/registrationkey", key );
				break;
			}

			QMessageBox::critical( 0,
								  tr("Registration failed"),
								  tr("Registration failed: %1") .arg( pLicensing->errMsg() ) );
			continue;
		}
	}

	// Show the registration info dialog
	QDialog dlg( this );
	Ui::DialogRegistration ui;
	ui.setupUi( &dlg );

	// Set the data
	ui.lblExpires->setText( pLicensing->expires().toString() );
	ui.lblSubject->setText( pLicensing->subject() );

	// Remove the "cancel" button from the button box
	ui.buttonBox->removeButton( ui.buttonBox->button(QDialogButtonBox::Cancel ));

	// Hide the "enter key" form and shrink the dialog
	ui.groupEnterKey->hide();
	dlg.adjustSize();
	dlg.setWindowTitle( tr("Registration information") );

	dlg.exec();
}

void MainWindow::act_addMissingTimingMarks()
{
	editor->addMissingTimingMarks();
}

void MainWindow::act_adjustTiming()
{
	editor->adjustTimings();
}
