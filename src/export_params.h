/**************************************************************************
 *  Karlyriceditor - a lyrics editor for Karaoke songs                    *
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

#ifndef VIDEOEXPORTOPTIONS_H
#define VIDEOEXPORTOPTIONS_H

#include <QDialog>

#include "ui_dialog_export_params.h"
#include "lyrics.h"
#include "project.h"


class DialogExportOptions : public QDialog, public Ui::DialogExportParams
{
    Q_OBJECT

	public:
		DialogExportOptions( Project * project, const Lyrics& lyrics, bool video = true, QWidget *parent = 0 );

		// For video mode
		static QSize	getVideoSize( Project * project );
		static void		getFPS( Project * project, unsigned int * num, unsigned int * den );
		static QString	getEncoding( Project * project );
		static QString	getContainer( Project * project );

	public slots:
		void	autodetectFontSize();
		void	showPreview();
		void	browseOutputFile();
		void	btnTestFontSize();
		void	accept();

	public:
		QString	m_outputVideo;

	private:
		bool	testFontSize();
		void	setBoxIndex( Project::Tag tag, QComboBox * box );

	private:
		bool		m_videomode;
		Project*	m_project;
		Lyrics		m_lyrics;
};

#endif // VIDEOEXPORTOPTIONS_H
