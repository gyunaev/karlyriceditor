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

#include "settings.h"

Settings * pSettings;

Settings::Settings()
{
	m_phononSoundDelay = 250;

	m_editorStopAtLineEnd = true;
	m_editorStopNextWord = false;
	m_editorWordChars = 2;
	m_editorSkipEmptyLines = true;
	m_editorSupportBlocks = true;
	m_editorMaxBlock = 8;
	m_editorFontFamily = "arial";
	m_editorFontSize = 14;
	m_editorDoubleTimeMark = true;

	m_timeMarkFontFamily = "arial";
	m_timeMarkFontSize = 10;
	m_timeMarkHolderBackground = QColor( "grey" );
	m_timeMarkBackground = QColor( "yellow" );
	m_timeMarkText = QColor( "black" );

	m_previewFontFamily = "arial";
	m_previewFontSize = 24;

	m_previewBackground = QColor( "black" );
	m_previewTextInactive = QColor( "white" );
	m_previewTextActive = QColor( "green" );
}
