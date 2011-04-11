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

#ifndef DIALOG_RENDERPREVIEW_H
#define DIALOG_RENDERPREVIEW_H

#include <QDialog>
#include "ui_dialog_renderpreview.h"

class Project;
class TextRenderer;

class DialogRenderPreview : public QDialog, public Ui::DialogRenderPreview
{
    Q_OBJECT

	public:
		DialogRenderPreview( QWidget *parent = 0 );

	public slots:
		void	updateParams( TextRenderer * renderer, Project * project );
		void	lyricsUpdated();

	private slots:
		void	updateImage();
		void	seekSliderMoved( int newvalue );

	private:
		TextRenderer * m_renderer;
		Project		 * m_project;
		qint64		   m_time;
};

#endif // DIALOG_RENDERPREVIEW_H
