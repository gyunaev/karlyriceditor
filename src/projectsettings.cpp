#include "projectsettings.h"
#include "project.h"

ProjectSettings::ProjectSettings( Project* proj, bool showtype, QWidget * parent )
	: QDialog( parent ), Ui::ProjectSettings()
{
	setupUi( this );

	m_project = proj;

	if ( m_project->type() != Project::LyricType_UStar )
	{
		// Hide Ultrastar-specific fields
		labelUSbpmgap->hide();
		labelMusicFile->hide();
		leUSmp3->hide();

		groupUStar->hide();
	}
	else
		groupLRC->hide();

	if ( showtype )
	{
	}

}

void ProjectSettings::accept()
{
}

void ProjectSettings::browseMusicFile()
{
}
