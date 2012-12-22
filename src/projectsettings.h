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

#ifndef PROJECTSETTINGS_H
#define PROJECTSETTINGS_H

#include <QDialog>
#include "ui_dialog_projectsettings.h"

class Project;

class ProjectSettings : public QDialog, public Ui::DialogProjectSettings
{
	Q_OBJECT

	public:
		ProjectSettings( Project* proj, bool showtype = true, QWidget * parent = 0 );
		bool	musicFileChanged() const { return m_musicFileChanged; }

	public slots:
		void	browseMusicFile();
		void	changeProjectType();
		void	accept();

	private:
		Project*	m_project;
		bool		m_musicFileChanged; // to reinitialize the player if changed
		bool		m_generalTabShown;
};

#endif // PROJECTSETTINGS_H
