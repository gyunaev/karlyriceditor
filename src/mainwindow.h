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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QSettings>

#include "checknewversion.h"
#include "ui_mainwindow.h"

class Project;
class ViewWidget;
class PlayerWidget;
class TestWindow;
class RecentFiles;


class MainWindow : public QMainWindow, public Ui::MainWindow
{
	Q_OBJECT

	public:
		MainWindow();
		~MainWindow();

	public slots:
		void	updateState();
		void	editor_undoAvail(bool);
		void	editor_redoAvail(bool);

	private slots:
		void	act_fileNewProject();
		void	act_fileOpenProject();
		bool	act_fileSaveProject();
		bool	act_fileSaveProjectAs();

		void	act_editInsertTag();
		void	act_editRemoveTag();
		void	act_editRemoveAllTags();
		void	act_editClearText();
		void	act_editTrimspaces();
		void	act_editSplitLine();

		void	act_projectOpenLyricFile();
		void	act_projectEditHeader();
		void	act_projectValidateLyrics();
		void	act_projectViewLyricFile();
		void	act_projectTest();
		void	act_projectTestCDG();
		void	act_projectExportLyricFile();
		void	act_projectExportVideoFile();
		void	act_projectExportCDGFile();
		void	act_projectSettings();

		void	act_settingsGeneral();
		void	act_settingsShowPlayer( bool checked );

		void	act_helpAbout();
		void	act_helpRegistration();

		void	openRecentFile( const QString& file );

		// New version available
		void	newVerAvailError( int errorcode );
		void	newVerAvailable( NewVersionMetaMap metadata );

		// Dock widgets
		void	visibilityPlayer( bool visible );

		// Test window
		void	testWindowClosed();

	protected:
		void	closeEvent(QCloseEvent *event);

	private:
		void	checkNewVersionAvailable();
		void	connectActions();
		void	createToolbars();
		void	setCurrentProject( Project * proj );
		bool	saveProject( const QString& filename );
		bool	loadProject( const QString& file );

		// If the project is changed, and user selected "Cancel", returns false
		bool	tryCloseCurrentProject();

		PlayerWidget		*	m_player;

		Project				*	m_project;
		ViewWidget			*	m_viewer;
		TestWindow			*	m_testWindow;
		RecentFiles			*	m_recentFiles;
		QString					m_projectFile;

		// Validator icons
		QIcon					m_validatorIconRegular;
		QIcon					m_validatorIconAccepted;
		QIcon					m_validatorIconFailed;
};

extern MainWindow * pMainWindow;

#endif // MAINWINDOW_H
