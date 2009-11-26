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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QSettings>

#include "ui_mainwindow.h"

class Project;
class ViewWidget;
class PlayerWidget;
class TestWindow;


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

		void	act_projectOpenMusicFile();
		void	act_projectOpenLyricFile();
		void	act_projectEditHeader();
		void	act_projectValidateLyrics();
		void	act_projectViewLyricFile();
		void	act_projectTest();
		void	act_projectExportLyricFile();
		void	act_projectSettings();

		void	act_settingsGeneral();

		void	act_helpAbout();

		/*		void	file_openRecentFile();


		void updateRecentFileActions();
*/

	protected:
		void	closeEvent(QCloseEvent *event);

	private:
		void	connectActions();
		void	createToolbars();
		void	setCurrentProject( Project * proj );
		bool	saveProject( const QString& filename );

		// If the project is changed, and user selected "Cancel", returns false
		bool	tryCloseCurrentProject();

		PlayerWidget		*	m_player;

		Project				*	m_project;
		ViewWidget			*	m_viewer;
		TestWindow			*	m_testWindow;
		QString					m_projectFile;

		// Validator icons
		QIcon					m_validatorIconRegular;
		QIcon					m_validatorIconAccepted;
		QIcon					m_validatorIconFailed;
};

extern MainWindow * pMainWindow;

#endif // MAINWINDOW_H
