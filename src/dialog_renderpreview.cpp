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

#include "project.h"
#include "textrenderer.h"
#include "dialog_renderpreview.h"
#include "ui_dialog_renderpreview.h"


DialogRenderPreview::DialogRenderPreview( QWidget *parent )
	: QDialog(parent), Ui::DialogRenderPreview()
{
	setupUi( this );

	m_renderer = 0;
	m_project = 0;
	m_time = 0;
}

void DialogRenderPreview::updateParams( TextRenderer * renderer, Project * project )
{
	// We own the renderer object, so we must delete the old one
	delete m_renderer;
	m_renderer = renderer;

	// We do not own the project though
	m_project = project;

	updateImage();
}

void DialogRenderPreview::lyricsUpdated()
{
}

void DialogRenderPreview::seekSliderMoved( int newvalue )
{

}

void DialogRenderPreview::updateImage()
{

}
