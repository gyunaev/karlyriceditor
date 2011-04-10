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

#include <QMap>
#include <QSettings>
#include <QMessageBox>
#include <QTextCodec>

#include "dialog_selectencoding.h"

static const char * ENCODED_SETTINGS = "general/savedstoredencoding";


DialogSelectEncoding::DialogSelectEncoding( const QByteArray& text, QWidget *parent )
	: QDialog(parent), Ui::DialogSelectEncoding()
{
	setupUi( this );
	m_encodedText = text;

	connect( boxEncoding, SIGNAL(currentIndexChanged(int)), this, SLOT(encodingChanged(int)) );

	// Add the supported encodings.
	QMap< QString, QString > encodings;

	encodings[ "CP1256" ] = "Arabic";
	encodings[ "CP1257" ] = "Baltic";
	encodings[ "CP1250" ] = "Central European";
	encodings[ "GB18030" ] = "Chinese Simplified";
	encodings[ "GBK" ] = "Chinese Simplified";
	encodings[ "GB2313" ] = "Chinese Simplified";
	encodings[ "Big5" ] = "Chinese Traditional";
	encodings[ "Big5-HKSCS" ] = "Chinese Traditional";
	encodings[ "CP1251" ] = "Cyrillic";
	encodings[ "KOI8-R" ] = "Cyrillic";
	encodings[ "CP1253" ] = "Greek";
	encodings[ "CP1255" ] = "Hebrew";
	encodings[ "eucJP" ] = "Japanese";
	encodings[ "JIS7" ] = "Japanese";
	encodings[ "Shift-JIS" ] = "Japanese";
	encodings[ "eucKR" ] = "Korean";
	encodings[ "TSCII" ] = "Tamil";
	encodings[ "TIS-620" ] = "Thai";
	encodings[ "KOI8-U" ] = "Ukrainian";
	encodings[ "CP1254" ] = "Turkish";
	encodings[ "CP1258" ] = "Vietnamese";
	encodings[ "UTF-8" ] = "Unicode";
	encodings[ "UTF-16" ] = "Unicode";
	encodings[ "CP1252" ] = "Western";

	// Fill the combo box
	for ( QMap<QString,QString>::iterator it = encodings.begin(); it != encodings.end(); ++it )
	{
		QString encname = it.key();
		QString lang = it.value();

		boxEncoding->addItem ( QString("%1 - %2") .arg(lang).arg(encname), encname );
	}

	// Restore the last saved encoding if present
	QSettings settings;

	if ( settings.contains( ENCODED_SETTINGS ) )
	{
		int index = boxEncoding->findData( settings.value( ENCODED_SETTINGS ).toString() );

		if ( index != -1 )
			boxEncoding->setCurrentIndex( index );
	}
}

void DialogSelectEncoding::accept()
{
	if ( boxEncoding->currentIndex() == -1 )
	{
		QMessageBox::critical( 0,
							   tr("Encoding not selected"),
							   tr("Please select text encoding") );
		return;
	}

	if ( cbSaveDefault->isChecked() )
	{
		QSettings settings;
		settings.setValue( ENCODED_SETTINGS, m_selectedEncoding );
	}

	QDialog::accept();
}

void DialogSelectEncoding::encodingChanged( int index )
{
	if ( index == -1 )
		return;

	m_selectedEncoding = boxEncoding->itemData ( index ).toString();
	m_selectedCodec = QTextCodec::codecForName( qPrintable(m_selectedEncoding) );

	if ( !m_selectedCodec )
		qFatal("Failed to select Qt text codec for name '%s'", qPrintable(m_selectedEncoding) );

	leSample->setText( m_selectedCodec->toUnicode( m_encodedText ) );
}
