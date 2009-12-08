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

#include <QSettings>
#include <QDialog>
#include "ui_dialog_settings.h"

#include "settings.h"

Settings * pSettings;

Settings::Settings()
{
	QSettings settings;

	m_phononSoundDelay = settings.value( "advanced/phononsounddelay", 250 ).toInt();

	m_editorStopAtLineEnd = settings.value( "editor/stopatlineend", true ).toBool();
	m_editorStopNextWord = settings.value( "editor/stopatnextword", false ).toBool();
	m_editorWordChars = settings.value( "editor/charsinword", 2 ).toInt();
	m_editorSkipEmptyLines = settings.value( "editor/skipemptylines", true ).toBool();
	m_editorSupportBlocks = settings.value( "editor/supportblocks", true ).toBool();
	m_editorMaxBlock = settings.value( "editor/maxlinesinblock", 8 ).toInt();
	m_editorFontFamily = settings.value( "editor/fontfamily", "arial" ).toString();
	m_editorFontSize = settings.value( "editor/fontsize", 14 ).toInt();
	m_editorDoubleTimeMark = settings.value( "editor/doubletimemark", true ).toBool();

	m_timeMarkFontFamily = settings.value( "timemark/fontfamily", "arial" ).toString();
	m_timeMarkFontSize = settings.value( "timemark/fontsize", 10 ).toInt();
	m_timeMarkPlaceholderBackground = QColor( settings.value( "timemark/placeholderbgcolor", "grey" ).toString() );
	m_timeMarkTimeBackground = QColor( settings.value( "timemark/timebgcolor", "yellow" ).toString() );
	m_timeMarkPlaceholderText = QColor( settings.value( "timemark/placeholdertextgcolor", "black" ).toString() );
	m_timeMarkTimeText = QColor( settings.value( "timemark/timetextcolor", "black" ).toString() );
	m_timeMarkPitchBackground = QColor( settings.value( "timemark/pitchbgcolor", "green" ).toString() );
	m_timeMarkShowPitch = settings.value( "timemark/showpitch", true ).toBool();;

	m_previewFontFamily = settings.value( "preview/fontfamily", "arial" ).toString();
	m_previewFontSize = settings.value( "preview/fontsize", 24 ).toInt();
	m_previewBackground = QColor( settings.value( "preview/bgcolor", "black" ).toString() );
	m_previewTextInactive = QColor( settings.value( "preview/inactivecolor", "white" ).toString() );
	m_previewTextActive = QColor( settings.value( "preview/activecolor", "green" ).toString() );
}

void Settings::edit()
{
	QDialog dlg;
	Ui::DialogSettings ui;

	ui.setupUi( &dlg );

	// Set variables
	ui.leTickDelay->setText( QString::number( m_phononSoundDelay ) );
	ui.leEditorWordCount->setValue( m_editorWordChars );
	ui.leEditorBlockLines->setValue( m_editorMaxBlock );

	ui.cbEditorStopAtEnd->setChecked( m_editorStopAtLineEnd );
	ui.cbEditorStopAtWords->setChecked( m_editorStopNextWord );
	ui.cbEditorDoubleTags->setChecked( m_editorDoubleTimeMark );
	ui.cbEditorSupportBlocks->setChecked( m_editorSupportBlocks );
	ui.fontEditor->setCurrentFont( QFont( m_editorFontFamily ) );
	ui.fontEditorSize->setValue( m_editorFontSize );

	ui.fontTimeMark->setCurrentFont( QFont( m_timeMarkFontFamily ) );
	ui.fontTimeMarkSize->setValue( m_timeMarkFontSize );
	ui.btnTimingColorPhBg->setColor( m_timeMarkPlaceholderBackground );
	ui.btnTimingColorPhText->setColor( m_timeMarkPlaceholderText );
	ui.btnTimingColorTiBg->setColor( m_timeMarkTimeBackground );
	ui.btnTimingColorTiText->setColor( m_timeMarkTimeText );
	ui.btnTimingColorTiPitch->setColor( m_timeMarkPitchBackground );
	ui.cbShowPitchTimingMark->setChecked( m_timeMarkShowPitch );

	ui.fontPreview->setCurrentFont( QFont( m_previewFontFamily ) );
	ui.fontPreviewSize->setValue( m_previewFontSize );
	ui.btnPreviewColorActive->setColor( m_previewTextActive );
	ui.btnPreviewColorBg->setColor( m_previewBackground );
	ui.btnPreviewColorInactive->setColor( m_previewTextInactive );

	if ( dlg.exec() == QDialog::Rejected )
		return;

	// Get the values
	m_phononSoundDelay = ui.leTickDelay->text().toInt();
	m_editorWordChars = ui.leEditorWordCount->text().toInt();
	m_editorMaxBlock = ui.leEditorBlockLines->text().toInt();

	m_editorStopAtLineEnd = ui.cbEditorStopAtEnd->isChecked();
	m_editorStopNextWord = ui.cbEditorStopAtWords->isChecked();
	m_editorDoubleTimeMark = ui.cbEditorDoubleTags->isChecked();
	m_editorSupportBlocks = ui.cbEditorSupportBlocks->isChecked();
	m_editorFontFamily = ui.fontEditor->currentFont().family();
	m_editorFontSize = ui.fontEditorSize->value();

	m_timeMarkFontFamily = ui.fontTimeMark->currentFont().family();
	m_timeMarkFontSize = ui.fontTimeMarkSize->value();
	m_timeMarkPlaceholderBackground = ui.btnTimingColorPhBg->color();
	m_timeMarkPlaceholderText = ui.btnTimingColorPhText->color();
	m_timeMarkTimeBackground = ui.btnTimingColorTiBg->color();
	m_timeMarkTimeText = ui.btnTimingColorTiText->color();
	m_timeMarkPitchBackground = ui.btnTimingColorTiPitch->color();
	m_timeMarkShowPitch = ui.cbShowPitchTimingMark->isChecked();

	m_previewFontFamily = ui.fontPreview->currentFont().family();
	m_previewFontSize = ui.fontPreviewSize->value();
	m_previewTextActive = ui.btnPreviewColorActive->color();
	m_previewBackground = ui.btnPreviewColorBg->color();
	m_previewTextInactive = ui.btnPreviewColorInactive->color();

	// And save them
	QSettings settings;

	settings.setValue( "advanced/phononsounddelay", m_phononSoundDelay );

	settings.setValue( "editor/stopatlineend", m_editorStopAtLineEnd );
	settings.setValue( "editor/stopatnextword", m_editorStopNextWord );
	settings.setValue( "editor/charsinword", m_editorWordChars );
	settings.setValue( "editor/skipemptylines", m_editorSkipEmptyLines );
	settings.setValue( "editor/supportblocks", m_editorSupportBlocks );
	settings.setValue( "editor/maxlinesinblock", m_editorMaxBlock );
	settings.setValue( "editor/fontfamily", m_editorFontFamily );
	settings.setValue( "editor/fontsize", m_editorFontSize );
	settings.setValue( "editor/doubletimemark", m_editorDoubleTimeMark );

	settings.setValue( "timemark/fontfamily", m_timeMarkFontFamily );
	settings.setValue( "timemark/fontsize", m_timeMarkFontSize );
	settings.setValue( "timemark/placeholderbgcolor", m_timeMarkPlaceholderBackground.name() );
	settings.setValue( "timemark/timebgcolor", m_timeMarkTimeBackground.name() );
	settings.setValue( "timemark/placeholdertextgcolor", m_timeMarkPlaceholderText.name() );
	settings.setValue( "timemark/timetextcolor", m_timeMarkTimeText.name() );
	settings.setValue( "timemark/pitchbgcolor", m_timeMarkPitchBackground.name() );
	settings.setValue( "timemark/showpitch", m_timeMarkShowPitch );

	settings.setValue( "preview/fontfamily", m_previewFontFamily );
	settings.setValue( "preview/fontsize", m_previewFontSize );
	settings.setValue( "preview/bgcolor", m_previewBackground.name() );
	settings.setValue( "preview/inactivecolor", m_previewTextInactive.name() );
	settings.setValue( "preview/activecolor", m_previewTextActive.name() );
}
