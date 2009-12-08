/**************************************************************************
 *  Karlyriceditor - a lyrics editor for Karaoke songs                    *
 *  Copyright (C) 2009 George Yunaev, support@karlyriceditor.com          *
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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QColor>

class Settings
{
	public:
		Settings();

		void	edit();

	public:
		// There is a delay between a note is being singed, and Phonon sends its tick()
		// signal. To compensate for this delay, output lyrics should include it.
		int			m_phononSoundDelay;

		// Check for updates
		bool		m_checkForUpdates;

		// When moving the cursor after inserting the tag,
		// also stop at the line ends.
		bool		m_editorStopAtLineEnd;

		// When moving the cursor after inserting the tag,
		// also stop at next words (a word should have more
		// than m_editorWordChars chars to be considered).
		bool		m_editorStopNextWord;
		int			m_editorWordChars;

		// When moving the cursor after inserting the tag,
		// skip the empty lines
		bool		m_editorSkipEmptyLines;

		// Should editor support blocks
		bool		m_editorSupportBlocks;
		int			m_editorMaxBlock;

		// Editor font family and size
		QString		m_editorFontFamily;
		int			m_editorFontSize;

		// If set, and the time placeholder is present, it keeps the cursor
		// after the time is replaced, letting to enter two timing marks
		// at the same position.
		bool		m_editorDoubleTimeMark;

		// Time mark font family and size
		QString		m_timeMarkFontFamily;
		int			m_timeMarkFontSize;

		QColor		m_timeMarkPlaceholderBackground;
		QColor		m_timeMarkTimeBackground;
		QColor		m_timeMarkPitchBackground; // background for timing marks which have both time and pitch
		QColor		m_timeMarkPlaceholderText;
		QColor		m_timeMarkTimeText;

		// Whether to show pitch in timing mark
		bool		m_timeMarkShowPitch;

		// Preview window font family and size
		QString		m_previewFontFamily;
		int			m_previewFontSize;

		// Preview window colors - background, text, active text
		QColor		m_previewBackground;
		QColor		m_previewTextInactive;
		QColor		m_previewTextActive;
};

extern Settings * pSettings;

#endif // SETTINGS_H
