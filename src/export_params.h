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

#ifndef VIDEOEXPORTOPTIONS_H
#define VIDEOEXPORTOPTIONS_H

#include <QDialog>
#include "ui_dialog_export_params.h"

#include "videoencodingprofiles.h"
#include "textrenderer.h"
#include "lyrics.h"
#include "project.h"


class DialogExportOptions : public QDialog, public Ui::DialogExportParams
{
    Q_OBJECT

	public:
		DialogExportOptions( Project * project, const Lyrics& lyrics, bool video = true, QWidget *parent = 0 );

		// For both CD+G and video modes
		QSize	getVideoSize();

		// Returns the selected video profile
		const VideoEncodingProfile * videoProfile() const;

		// Return the video encoding parameters
		bool	videoParams( const VideoEncodingProfile ** profile,
							 const VideoFormat ** format,
							 unsigned int * audioMode,
							 unsigned int * qualty );

	private slots:
		void	activateTab( int index );
		void	autodetectFontSize();
		void	browseOutputFile();
		bool	testFontSize();
		void	previewUpdateImage();
		void	previewSliderMoved( int newvalue );
		void	accept();

		void	videoMediumChanged( int newvalue );
		void	videoTargetChanged(int);
		void	videoShowDetails();

	public:
		QString	m_outputVideo;
		QString	m_artist;
		QString	m_title;
		QString	m_createdBy;

	private:
		void	setBoxIndex( Project::Tag tag, QComboBox * box );

	private:
		bool		m_videomode;
		Project*	m_project;
		Lyrics		m_lyrics;

		// For preview
		TextRenderer	m_renderer;
		qint64			m_time;

		// For current video selection tracking
		const VideoEncodingProfile * m_currentProfile;
		const VideoFormat * m_currentVideoFormat;
		unsigned int		m_audioEncodingMode;
		unsigned int		m_quality;
};

#endif // VIDEOEXPORTOPTIONS_H
