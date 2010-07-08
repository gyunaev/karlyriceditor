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

#include <QTime>
#include <QToolBar>
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QWhatsThis>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>

#include "wizard_newproject.h"
#include "mainwindow.h"
#include "playerwidget.h"
#include "project.h"
#include "settings.h"
#include "viewwidget.h"
#include "testwindow.h"
#include "testcdgwindow.h"
#include "version.h"
#include "projectsettings.h"
#include "recentfiles.h"
#include "gentlemessagebox.h"
#include "pianorollwidget.h"
#include "ui_dialog_about.h"


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

	// Initialize stuff
	m_project = 0;

	// Create dock widgets
	m_player = new PlayerWidget( this );
	addDockWidget( Qt::BottomDockWidgetArea, m_player );
	actionShow_Player_dock_wingow->setChecked( true );

	m_pianoRoll = new PianoRollDock( this );
	addDockWidget( Qt::TopDockWidgetArea, m_pianoRoll );
	m_pianoRoll->hide();
	actionShow_Piano_Roll_dock_window->setChecked( false );

	// Create a lyric viewer window, hidden so far
	m_viewer = new ViewWidget( this );

	// Test window
	m_testWindow = new TestWindow( this );
	connect( m_player, SIGNAL(tick(qint64)), m_testWindow, SLOT(tick(qint64)) );

	// CD+G test window
	m_testCDGWindow = new TestCDGWindow( this );
	connect( m_player, SIGNAL(tick(qint64)), m_testCDGWindow, SLOT(tick(qint64)) );

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

	// Show some message boxes about Phonon
	GentleMessageBox::warning( this,
							   "phonongstreamer",
							   tr("Advice: set Phonon backend to Xine?"),
							   tr("GStreamer Phonon backend is less reliable than Xine for reporting time, "
								  "so it is suggested to use Xine backend." ) );

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

	connect( pNewVer, SIGNAL(error(int)), this, SLOT(newVerAvailError(int)) );
	connect( pNewVer, SIGNAL(newVersionAvailable( NewVersionMetaMap )), this, SLOT(newVerAvailable(NewVersionMetaMap)) );

	pNewVer->setUrl( "http://www.karlyriceditor.com/latestversion.txt" );
	pNewVer->setCurrentVersion( QString("%1.%2").arg( APP_VERSION_MAJOR ) . arg( APP_VERSION_MINOR ) );
	pNewVer->start();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if ( m_project && !tryCloseCurrentProject() )
	{
		event->ignore();
		return;
	}

	editor->cleanupAutoSave();

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
	connect( actionGeneral, SIGNAL( triggered()), this, SLOT(act_settingsGeneral()) );
	connect( actionAbout, SIGNAL( triggered()), this, SLOT(act_helpAbout()) );
	connect( actionProject_settings, SIGNAL( triggered()), this, SLOT(act_projectSettings()) );
	connect( actionExport_lyric_file, SIGNAL( triggered()), this, SLOT(act_projectExportLyricFile()) );
	connect( actionEdit_header_data, SIGNAL( triggered()), this, SLOT( act_projectEditHeader()) );
	connect( actionValidate_lyrics, SIGNAL( triggered()), this, SLOT( act_projectValidateLyrics()) );
	connect( actionView_lyric_file, SIGNAL( triggered()), this, SLOT( act_projectViewLyricFile()) );
	connect( actionTest_lyric_file, SIGNAL( triggered()), this, SLOT( act_projectTest()) );
	connect( actionTest_CDG_lyrics, SIGNAL( triggered()), this, SLOT( act_projectTestCDG()) );
	connect( actionShow_Piano_Roll_dock_window, SIGNAL(triggered(bool)), this, SLOT(act_settingsShowPianoRoll(bool)) );
	connect( actionShow_Player_dock_wingow, SIGNAL(triggered(bool)), this, SLOT(act_settingsShowPlayer(bool)) );

	// docks
	connect( m_player,SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityPlayer(bool)) );
	connect( m_pianoRoll, SIGNAL(visibilityChanged(bool)), this, SLOT(visibilityPianoRoll(bool)) );

	// Text editor
	connect( actionUndo, SIGNAL( triggered()), editor, SLOT( undo()) );
	connect( actionRedo, SIGNAL( triggered()), editor, SLOT( redo()) );
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
			".",
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
	QString fileName = QFileDialog::getSaveFileName(this,
			tr("Save as a project file"),
			".",
			tr("Project files (*.kleproj)") );

	if ( fileName.isEmpty() )
		return false;

	if ( !saveProject( fileName ) )
		return false;

	m_projectFile = fileName;
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
	editor->cleanupAutoSave();

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
	m_pianoRoll->hide();
	updateState();

	return true;
}

void MainWindow::setCurrentProject( Project * proj )
{
	m_project = proj;

	setWindowTitle( tr("%1[*] - Lyric Editor") .arg( m_projectFile ) );

	statusBar()->showMessage( tr("Loading the music file %1") .arg(m_project->musicFile()), 2000);

	// Set the music file into player; it will call updateState()
	m_player->setMusicFile( m_project );

	// Open a piano roll if this is UltraStar project
	if ( proj->type() == Project::LyricType_UStar )
		m_pianoRoll->show();
}

void MainWindow::act_editInsertTag()
{
	editor->insertTimeTag( m_player->currentTime() );
}

void MainWindow::act_editRemoveTag()
{
	editor->removeLastTimeTag();
}

void MainWindow::act_editClearText()
{
	if ( QMessageBox::question( 0,
			tr("Clearing all the text"),
			tr("Are you sure you want to clear the text?\nThis operation cannot be undone!"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) == QMessageBox::Yes  )
		editor->clear();
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
			".",
			tr("LRC files (*.lrc);;UltraStar files (*.txt)") );

	if ( fileName.isEmpty() )
		return;

	m_project->importLyrics( fileName, fileName.endsWith( "txt" ) ? Project::LyricType_UStar : Project::LyricType_LRC2 );
}

void MainWindow::act_projectEditHeader()
{
	ProjectSettings ps( m_project, false, this );
	ps.setWindowTitle( tr("Edit lyric header data" ) );
	ps.exec();
}

void MainWindow::act_projectTest()
{
	if ( !editor->validate() )
		return;

	m_testWindow->setLyrics( editor->exportLyrics() );
	m_testWindow->show();
}

void MainWindow::act_projectExportLyricFile()
{
	if ( !editor->validate() )
		return;

	QString filter_LRC1 = tr("LRC version 1 (*.lrc1)");
	QString filter_LRC2 = tr("LRC version 2 (*.lrc)");
	QString filter_UStar = tr("UltraStar (*.txt)");
	QString filter_CDG = tr("CD+G (*.cdg)");

	QString filter, selected, outfilter;

	switch ( m_project->type() )
	{
		case Project::LyricType_LRC1:
			filter = filter_LRC1 + ";;" + filter_LRC2 + ";;" + filter_UStar + ";;" + filter_CDG;
			selected = filter_LRC1;
			break;

		case Project::LyricType_LRC2:
			filter = filter_LRC2 + ";;" + filter_LRC1 + ";;" + filter_UStar + ";;" + filter_CDG;
			selected = filter_LRC2;
			break;

		case Project::LyricType_UStar:
			filter = filter_UStar + ";;" + filter_LRC2 + ";;" + filter_LRC1 + ";;" + filter_CDG;
			selected = filter_UStar;
			break;

		case Project::LyricType_CDG:
			filter =  filter_CDG + ";;" + filter_UStar + ";;" + filter_LRC2 + ";;" + filter_LRC1;
			selected = filter_CDG;
			break;
	}

	QString outfile = QFileDialog::getSaveFileName( 0, tr("Export lyrics to a file"), ".", filter, &outfilter );
	QString lyrics;

	if ( outfile.isEmpty() )
		return;

	if ( outfilter == filter_LRC1 )
		lyrics = m_project->exportLyricsAsLRC1();
	else if ( outfilter == filter_LRC2 )
		lyrics = m_project->exportLyricsAsLRC2();
	else if ( outfilter == filter_UStar )
		lyrics = m_project->exportLyricsAsUStar();
	else if ( outfilter == filter_CDG )
		lyrics = m_project->exportLyricsAsCDG();
	else
	{
		QMessageBox::critical( 0, "Unknown filter", "Unknown filter" );
		return;
	}

	QFile file( outfile );
	if ( !file.open( QIODevice::WriteOnly ) )
	{
		QMessageBox::critical( 0,
							   tr("Cannot write lyric file"),
							   tr("Cannot write lyric file %1: %2") .arg(outfile) .arg(file.errorString()) );
		return;
	}

	file.write( lyrics.toUtf8() );
}

void MainWindow::act_projectSettings()
{
	ProjectSettings ps( m_project, true, this );
	ps.setWindowTitle( tr("Edit project settings" ) );

	if ( ps.exec() == QDialog::Accepted )
	{
		if ( ps.musicFileChanged() )
			m_player->setMusicFile( m_project );
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
			"Copyright (C) George Yunaev 2009, <a href=\"mailto:support@karlyriceditor.com\">support@karlyriceditor.com</a><br><br>"
			"Web site: <a href=\"http://www.karlyriceditor.com\">www.karlyriceditor.com</a><br><br>"
			"This program is licensed under terms of GNU General Public License<br>"
			"version 3; see LICENSE file for details.") .arg(APP_VERSION_MAJOR) .arg(APP_VERSION_MINOR) );

	dlg.exec();
}

void MainWindow::updateState()
{
	bool project_available = m_project ? true : false;
	bool project_ready = m_project ? m_player->isReady() : false;

	actionSave->setEnabled( project_available );
	actionSave_as->setEnabled( project_available );
	actionUndo->setEnabled( project_ready );
	actionRedo->setEnabled( project_ready );
	actionInsert_tag->setEnabled( project_ready );
	actionRemove_tag->setEnabled( project_ready );
	actionEdit_header_data->setEnabled( project_ready );
	actionValidate_lyrics->setEnabled( project_ready );
	actionView_lyric_file->setEnabled( project_ready );
	actionTest_lyric_file->setEnabled( project_ready );
	actionTest_CDG_lyrics->setEnabled( project_ready );
	actionRemove_all_tags->setEnabled( project_ready );
	actionOpen_lyric_file->setEnabled( project_ready );
	actionClear_text->setEnabled( project_ready );
	actionExport_lyric_file->setEnabled( project_ready );
	actionEdit_header_data->setEnabled( project_available );
	actionProject_settings->setEnabled( project_available );

	editor->setEnabled( project_ready );
	m_pianoRoll->setEnabled( project_ready );

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

void MainWindow::noteMouseOver( unsigned int pitch )
{
	int octave;
	QString note = PianoRollWidget::pitchToNote( pitch, &octave );

	statusBar()->showMessage( tr("Selected note: %1 at octave %2") .arg(note) .arg(octave), 2000 );
}

void MainWindow::noteClicked( unsigned int pitch )
{
	editor->pianoRollClicked( pitch );
}

void MainWindow::act_settingsShowPlayer( bool checked )
{
	if ( checked )
		m_player->show();
	else
		m_player->hide();
}

void MainWindow::act_settingsShowPianoRoll( bool checked )
{
	if ( checked )
		m_pianoRoll->show();
	else
		m_pianoRoll->hide();
}

void MainWindow::visibilityPianoRoll( bool visible )
{
	actionShow_Piano_Roll_dock_window->setChecked( visible );
}

void MainWindow::visibilityPlayer( bool visible )
{
	actionShow_Player_dock_wingow->setChecked( visible );
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

	m_testCDGWindow->setCDGdata( cdgdata );
	m_testCDGWindow->show();
}
