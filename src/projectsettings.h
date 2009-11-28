#ifndef PROJECTSETTINGS_H
#define PROJECTSETTINGS_H

#include <QDialog>
#include "ui_dialog_projectsettings.h"

class Project;

class ProjectSettings : public QDialog, public Ui::DialogProjectSettings
{
	public:
		ProjectSettings( Project* proj, bool showtype = true, QWidget * parent = 0 );

	public slots:
		void	browseMusicFile();
		void	accept();

	private:
		Project*	m_project;
};

#endif // PROJECTSETTINGS_H
