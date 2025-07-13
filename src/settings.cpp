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

#include <QSettings>
#include <QDateTime>
#include <QDialog>
#include <QSslCertificate>
#include <QSslError>

#if defined (WIN32)
    #include <windows.h>
    #include <wincrypt.h>
#endif

#include "ui_dialog_settings.h"
#include "settings.h"

Settings * pSettings;

Settings::Settings()
{
	QSettings settings;

    m_LastUsedDirectory = settings.value( "main/lastseddir", "." ).toString();

	m_phononSoundDelay = settings.value( "advanced/phononsounddelay", 250 ).toInt();
	m_checkForUpdates = settings.value( "advanced/checkforupdates", true ).toBool();

	m_editorStopAtLineEnd = settings.value( "editor/stopatlineend", true ).toBool();
	m_editorStopNextWord = settings.value( "editor/stopatnextword", false ).toBool();
	m_editorWordChars = settings.value( "editor/charsinword", 2 ).toInt();
	m_editorSkipEmptyLines = settings.value( "editor/skipemptylines", true ).toBool();
	m_editorSupportBlocks = settings.value( "editor/supportblocks", true ).toBool();
	m_editorMaxBlock = settings.value( "editor/maxlinesinblock", 8 ).toInt();
	m_editorFontFamily = settings.value( "editor/fontfamily", "arial" ).toString();
	m_editorFontSize = settings.value( "editor/fontsize", 14 ).toInt();
	m_editorDoubleTimeMark = settings.value( "editor/doubletimemark", true ).toBool();
    m_editorAutoUpdateTestWindows = settings.value( "editor/autoupdatetestwindow", false ).toBool();
    m_editorAutoUpdatePlayerBackseek = settings.value( "editor/autoupdateplayerbackseek", 0 ).toInt();

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

    QString key = settings.value( "advanced/registrationkey", "").toString();

    if ( key.isEmpty() )
        key = settings.value( "general/registrationkey", "").toString();

    if ( !key.isEmpty() )
        validateCert( key );
}

void Settings::edit()
{
	QSettings settings;
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

    ui.boxEditorRealtimeTesting->setChecked( m_editorAutoUpdateTestWindows );
    ui.leEditorSecondsBackward->setValue( m_editorAutoUpdatePlayerBackseek );

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

	ui.cbCheckForUpdates->setChecked( m_checkForUpdates );

	if ( settings.contains( "advanced/lastupdate" ) )
		ui.lblLastUpdate->setText( QObject::tr("Last checked: %1")
            .arg( settings.value( "advanced/lastupdate" ).toDateTime().toString() ) );

	if ( dlg.exec() == QDialog::Rejected )
		return;

	// Get the values
	m_phononSoundDelay = ui.leTickDelay->text().toInt();
	m_checkForUpdates = ui.cbCheckForUpdates->isChecked();

	m_editorWordChars = ui.leEditorWordCount->text().toInt();
    m_editorMaxBlock = ui.leEditorBlockLines->value();

	m_editorStopAtLineEnd = ui.cbEditorStopAtEnd->isChecked();
	m_editorStopNextWord = ui.cbEditorStopAtWords->isChecked();
	m_editorDoubleTimeMark = ui.cbEditorDoubleTags->isChecked();
	m_editorSupportBlocks = ui.cbEditorSupportBlocks->isChecked();
	m_editorFontFamily = ui.fontEditor->currentFont().family();
	m_editorFontSize = ui.fontEditorSize->value();

    m_editorAutoUpdateTestWindows = ui.boxEditorRealtimeTesting->isChecked();
    m_editorAutoUpdatePlayerBackseek = ui.leEditorSecondsBackward->value();

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
	settings.setValue( "advanced/phononsounddelay", m_phononSoundDelay );
	settings.setValue( "advanced/checkforupdates", m_checkForUpdates );

	settings.setValue( "editor/stopatlineend", m_editorStopAtLineEnd );
	settings.setValue( "editor/stopatnextword", m_editorStopNextWord );
	settings.setValue( "editor/charsinword", m_editorWordChars );
	settings.setValue( "editor/skipemptylines", m_editorSkipEmptyLines );
	settings.setValue( "editor/supportblocks", m_editorSupportBlocks );
	settings.setValue( "editor/maxlinesinblock", m_editorMaxBlock );
	settings.setValue( "editor/fontfamily", m_editorFontFamily );
	settings.setValue( "editor/fontsize", m_editorFontSize );
	settings.setValue( "editor/doubletimemark", m_editorDoubleTimeMark );
    settings.setValue( "editor/autoupdatetestwindow", m_editorAutoUpdateTestWindows );
    settings.setValue( "editor/autoupdateplayerbackseek", m_editorAutoUpdatePlayerBackseek );

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

void Settings::updateLastUsedDirectory(const QString &lastdir)
{
    m_LastUsedDirectory = lastdir;
    QSettings settings;
    settings.setValue( "main/lastseddir", lastdir );
}


QString Settings::validateCert(const QString &pemdata)
{
    unsigned char cacert_der[] = {
      0x30, 0x82, 0x02, 0x90, 0x30, 0x82, 0x01, 0xf9, 0xa0, 0x03, 0x02, 0x01,
      0x02, 0x02, 0x09, 0x00, 0xb6, 0x59, 0xc9, 0xd9, 0x2b, 0xa3, 0x03, 0xbb,
      0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
      0x05, 0x05, 0x00, 0x30, 0x81, 0x80, 0x31, 0x31, 0x30, 0x2f, 0x06, 0x03,
      0x55, 0x04, 0x03, 0x0c, 0x28, 0x6b, 0x61, 0x72, 0x6c, 0x79, 0x72, 0x69,
      0x63, 0x65, 0x64, 0x69, 0x74, 0x6f, 0x72, 0x2e, 0x63, 0x6f, 0x6d, 0x2e,
      0x63, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x65, 0x2e,
      0x61, 0x75, 0x74, 0x68, 0x6f, 0x72, 0x69, 0x74, 0x79, 0x31, 0x13, 0x30,
      0x11, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x0a, 0x43, 0x61, 0x6c, 0x69,
      0x66, 0x6f, 0x72, 0x6e, 0x69, 0x61, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03,
      0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x29, 0x30, 0x27, 0x06,
      0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01, 0x16, 0x1a,
      0x73, 0x75, 0x70, 0x70, 0x6f, 0x72, 0x74, 0x40, 0x6b, 0x61, 0x72, 0x6c,
      0x79, 0x72, 0x69, 0x63, 0x65, 0x64, 0x69, 0x74, 0x6f, 0x72, 0x2e, 0x63,
      0x6f, 0x6d, 0x30, 0x1e, 0x17, 0x0d, 0x31, 0x39, 0x30, 0x38, 0x30, 0x35,
      0x30, 0x34, 0x31, 0x39, 0x33, 0x34, 0x5a, 0x17, 0x0d, 0x32, 0x39, 0x30,
      0x38, 0x30, 0x32, 0x30, 0x34, 0x31, 0x39, 0x33, 0x34, 0x5a, 0x30, 0x81,
      0x80, 0x31, 0x31, 0x30, 0x2f, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0c, 0x28,
      0x6b, 0x61, 0x72, 0x6c, 0x79, 0x72, 0x69, 0x63, 0x65, 0x64, 0x69, 0x74,
      0x6f, 0x72, 0x2e, 0x63, 0x6f, 0x6d, 0x2e, 0x63, 0x65, 0x72, 0x74, 0x69,
      0x66, 0x69, 0x63, 0x61, 0x74, 0x65, 0x2e, 0x61, 0x75, 0x74, 0x68, 0x6f,
      0x72, 0x69, 0x74, 0x79, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04,
      0x08, 0x0c, 0x0a, 0x43, 0x61, 0x6c, 0x69, 0x66, 0x6f, 0x72, 0x6e, 0x69,
      0x61, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02,
      0x55, 0x53, 0x31, 0x29, 0x30, 0x27, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86,
      0xf7, 0x0d, 0x01, 0x09, 0x01, 0x16, 0x1a, 0x73, 0x75, 0x70, 0x70, 0x6f,
      0x72, 0x74, 0x40, 0x6b, 0x61, 0x72, 0x6c, 0x79, 0x72, 0x69, 0x63, 0x65,
      0x64, 0x69, 0x74, 0x6f, 0x72, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x81, 0x9f,
      0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
      0x01, 0x05, 0x00, 0x03, 0x81, 0x8d, 0x00, 0x30, 0x81, 0x89, 0x02, 0x81,
      0x81, 0x00, 0xc7, 0xc3, 0x12, 0x24, 0x0d, 0x68, 0x3f, 0x9c, 0x81, 0x41,
      0x2c, 0x8d, 0x40, 0xa1, 0x03, 0x67, 0x5c, 0x83, 0x42, 0x91, 0xe3, 0xb6,
      0x72, 0x42, 0xf6, 0x63, 0x97, 0xa1, 0x2f, 0x08, 0xd5, 0x05, 0x07, 0x66,
      0xa9, 0xc9, 0xd8, 0xc3, 0xb2, 0x61, 0x93, 0x48, 0x6c, 0x31, 0x9d, 0xdf,
      0x32, 0x7a, 0x6b, 0x79, 0x00, 0xef, 0x11, 0x3f, 0xe4, 0x0a, 0xe3, 0xda,
      0xaf, 0xd2, 0xf1, 0x36, 0x5f, 0x9b, 0x69, 0xd2, 0x83, 0x8f, 0x80, 0x2b,
      0xa0, 0x03, 0xfd, 0xc5, 0x41, 0x7e, 0x5f, 0x60, 0xdd, 0x99, 0x7f, 0x35,
      0x52, 0x9b, 0x8a, 0x16, 0x63, 0x41, 0x19, 0xbd, 0x68, 0x31, 0x78, 0x61,
      0xaa, 0xfd, 0x1a, 0xda, 0x45, 0x7a, 0xff, 0xfc, 0x39, 0x67, 0x7c, 0xff,
      0xe8, 0xd4, 0xf2, 0xf5, 0x9f, 0xd9, 0x7b, 0xef, 0x03, 0x4f, 0x82, 0x0c,
      0x04, 0xf6, 0x5a, 0xe1, 0x5f, 0x1c, 0x3e, 0x40, 0xd7, 0x8f, 0x02, 0x03,
      0x01, 0x00, 0x01, 0xa3, 0x10, 0x30, 0x0e, 0x30, 0x0c, 0x06, 0x03, 0x55,
      0x1d, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xff, 0x30, 0x0d, 0x06,
      0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00,
      0x03, 0x81, 0x81, 0x00, 0x55, 0x46, 0xb0, 0x9c, 0x70, 0xc4, 0xce, 0xce,
      0x5d, 0x13, 0x2c, 0xd6, 0x56, 0x29, 0xc3, 0x54, 0xb4, 0x68, 0x37, 0x7c,
      0xfb, 0x5e, 0x3f, 0x95, 0xb7, 0x5f, 0x31, 0xd3, 0x8c, 0xab, 0x3a, 0x28,
      0x79, 0x60, 0xea, 0x69, 0x58, 0x6d, 0xc0, 0xf3, 0x71, 0xb2, 0xaf, 0x32,
      0xac, 0x64, 0x1c, 0x47, 0x20, 0xee, 0x30, 0xa9, 0x13, 0xd3, 0x39, 0xfb,
      0x99, 0xd1, 0x40, 0x54, 0x0a, 0x53, 0x94, 0xe6, 0x41, 0xd7, 0x52, 0x30,
      0xfa, 0x51, 0x17, 0x12, 0x8e, 0xc7, 0xa7, 0x30, 0xcd, 0xa2, 0x33, 0xd2,
      0x57, 0x39, 0x37, 0x1c, 0x62, 0x4a, 0x5b, 0x3d, 0x2b, 0x35, 0xf4, 0x7f,
      0xc7, 0x8c, 0xcf, 0xe1, 0x2a, 0x33, 0x5b, 0x78, 0xaa, 0x48, 0xcd, 0xb5,
      0x99, 0x42, 0x75, 0x88, 0x09, 0x64, 0x09, 0x51, 0x2b, 0xf0, 0xa2, 0x1f,
      0x7a, 0x38, 0x11, 0x08, 0x5b, 0x39, 0x06, 0x5c, 0x9b, 0xb0, 0x4a, 0xb3
    };

    registeredName.clear();
    registeredUntil = QDateTime();
    registeredDigest.clear();

    if ( pemdata.length() == 0 )
        return "The license is not valid";

    QByteArray derdata = QByteArray::fromBase64( pemdata.toLatin1() );
    QList<QSslCertificate> uc = QSslCertificate::fromData( derdata, QSsl::Der );

    if ( uc.length() != 1 || !uc[0].expiryDate().isValid() )
        return "The license is not valid";

#if defined WIN32
    PCCERT_CONTEXT pCertCA = nullptr, pCertUser = nullptr;
    bool validated = false;

    // Create a certificate context from the first buffer (certificate to be verified)
    pCertCA = CertCreateCertificateContext(X509_ASN_ENCODING, cacert_der, sizeof(cacert_der));
    pCertUser = CertCreateCertificateContext(X509_ASN_ENCODING, (BYTE*)derdata.data(), derdata.size() );

    if ( pCertCA
    && pCertUser
    && CryptVerifyCertificateSignature(NULL, X509_ASN_ENCODING, pCertUser->pbCertEncoded, pCertUser->cbCertEncoded, &pCertCA->pCertInfo->SubjectPublicKeyInfo) )
    {
    validated = true;
    }

    if (pCertCA)
        CertFreeCertificateContext(pCertCA);

    if (pCertUser)
        CertFreeCertificateContext(pCertUser);

    if ( !validated )
        return "The license is not valid for this program";
#else
    QList<QSslCertificate> ca = QSslCertificate::fromData( QByteArray( (char*) cacert_der, 1423), QSsl::Der );

    if ( ca.length() != 1 )
        return "The license is not valid";

    QList<QSslError> err = QSslCertificate::verify( QList<QSslCertificate> { uc[0], ca[0] } );

    if ( err.length() != 1 || err[0].error() != 10 || err[0].certificate().digest().toHex() != "84f0d72d7979e95e3e58ed44f7f5b988" )
        return "The license is not valid for this program";
#endif

    registeredName = uc[0].subjectDisplayName();
    registeredUntil = uc[0].expiryDate();
    registeredDigest = uc[0].digest().toHex();

    QSettings settings;
    settings.setValue( "advanced/registrationkey", pemdata );
    return "";
}

bool Settings::isRegistered() const
{
    return registeredUntil.isValid() && registeredUntil > QDateTime::currentDateTime();
}
