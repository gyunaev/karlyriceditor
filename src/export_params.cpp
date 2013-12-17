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

#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <QWhatsThis>

#include "textrenderer.h"
#include "export_params.h"
#include "playerwidget.h"
#include "licensing.h"
#include "version.h"
#include "util.h"
#include "cdg.h"


DialogExportOptions::DialogExportOptions( Project * project, const Lyrics& lyrics,  bool video, QWidget *parent )
	: QDialog(parent), Ui::DialogExportParams(), m_renderer(0, 0)
{
	m_videomode = video;
	m_project = project;
	m_lyrics = lyrics;
	m_time = 0;

	// UIC stuff
	setupUi( this );

	// Buttons
	connect( btnDetectSize, SIGNAL(clicked()), this, SLOT(autodetectFontSize()) );
	connect( btnTestSize, SIGNAL(clicked()), this, SLOT(testFontSize()) );
	connect( btnBrowse, SIGNAL(clicked()), this, SLOT(browseOutputFile()) );

	// Tabs
	connect( tabWidget, SIGNAL(currentChanged(int)), this, SLOT(activateTab(int)) );
	connect( seekSlider, SIGNAL(valueChanged(int)), this, SLOT(previewSliderMoved(int)) );

	// Video comboboxes
	connect( boxVideoTarget, SIGNAL(currentIndexChanged(int)), this, SLOT(videoTargetChanged(int)) );
	connect( boxVideoMedium, SIGNAL(currentIndexChanged(int)), this, SLOT(videoMediumChanged(int)) );

	// Details label
	connect( lblVideoDetailsLink, SIGNAL(linkActivated(QString)), this, SLOT(videoShowDetails()) );

	if ( video )
	{
		// Set the video output params
		boxVideoMedium->addItems( pVideoEncodingProfiles->videoMediumTypes() );
		btnVideoColorActive->setColor( m_project->tag( Project::Tag_Video_activecolor, "blue" ) );
		btnVideoColorBg->setColor( m_project->tag( Project::Tag_Video_bgcolor, "black" ) );
		btnVideoColorInactive->setColor( m_project->tag( Project::Tag_Video_inactivecolor, "green" ) );
		btnVideoColorInfo->setColor( m_project->tag( Project::Tag_Video_infocolor, "white" ) );
		fontVideo->setCurrentFont( QFont( m_project->tag( Project::Tag_Video_font, "arial" ) ) );
		fontVideoSize->setValue( m_project->tag( Project::Tag_Video_fontsize, "8" ).toInt() );
		titleVideoMin->setValue( m_project->tag( Project::Tag_Video_titletime, "5" ).toInt() );
		cbVideoPreamble->setChecked( m_project->tag( Project::Tag_Video_preamble, "1" ).toInt() );

		setWindowTitle( tr("Specify video parameters") );
	}
	else
	{
		// Hide video part
		groupVideo->hide();

		btnVideoColorActive->setColor( m_project->tag( Project::Tag_CDG_activecolor, "blue" ) );
		btnVideoColorBg->setColor( m_project->tag( Project::Tag_CDG_bgcolor, "black" ) );
		btnVideoColorInactive->setColor( m_project->tag( Project::Tag_CDG_inactivecolor, "green" ) );
		btnVideoColorInfo->setColor( m_project->tag( Project::Tag_CDG_infocolor, "white" ) );
		fontVideo->setCurrentFont( QFont( m_project->tag( Project::Tag_CDG_font, "arial" ) ) );
		fontVideoSize->setValue( m_project->tag( Project::Tag_CDG_fontsize, "8").toInt() );
		titleVideoMin->setValue( m_project->tag( Project::Tag_CDG_titletime, "5").toInt() );
		cbVideoPreamble->setChecked( m_project->tag( Project::Tag_CDG_preamble, "1").toInt() );

		setWindowTitle( tr("Specify CDG parameters") );
		lblOutput->setText( tr("Write CD+G data to file:") );
		leOutputFile->setText( Util::removeFileExtention( m_project->musicFile() ) + "cdg" );

		// resize as we hid the video part
		resize( width(), 1 );
	}

	fontVideo->setFontFilters( QFontComboBox::ScalableFonts | QFontComboBox::MonospacedFonts | QFontComboBox::ProportionalFonts );

	// Title window
	leTitle->setText( m_project->tag( Project::Tag_Title, "") );
	leArtist->setText( m_project->tag( Project::Tag_Artist, "") );

	if ( pLicensing->isValid() )
	{
		leTitleCreatedBy->setText( QString("Created by %1<br>http://www.ulduzsoft.com") .arg(APP_NAME) );
	}
	else
	{
		leTitleCreatedBy->setEnabled( false );
		leTitleCreatedBy->setText( "Application not registered, this field cannot be modified" );
	}
}


void DialogExportOptions::setBoxIndex( Project::Tag tag, QComboBox * box )
{
	QString val = m_project->tag( tag );

	if ( val.isEmpty() )
	{
		box->setCurrentIndex( 0 );
	}
	else
	{
		int idx = box->findText( val );

		if ( idx != -1 )
			box->setCurrentIndex( idx );
		else
			box->setCurrentIndex( 0 );
	}
}

void DialogExportOptions::autodetectFontSize()
{
	QFont font = fontVideo->currentFont();

	if ( !m_videomode )
		font.setStyleStrategy( QFont::NoAntialias );

	// Ask the renderer
	TextRenderer renderer( 100, 100 );
	renderer.setLyrics( m_lyrics );
	int fsize = renderer.autodetectFontSize( getVideoSize(), font );

	fontVideoSize->setValue( fsize );
}

bool DialogExportOptions::testFontSize()
{
	QFont font = fontVideo->currentFont();
	font.setPointSize( fontVideoSize->value() );

	if ( !m_videomode )
		font.setStyleStrategy( QFont::NoAntialias );

	TextRenderer renderer( 100, 100 );
	renderer.setLyrics( m_lyrics );

	if ( !renderer.verifyFontSize( getVideoSize(), font ) )
	{
		QMessageBox::critical( 0,
							  tr("Font size too large"),
							  tr("The output text cannot fit into screen using the specified font size") );
		return false;
	}

	return true;
}


void DialogExportOptions::browseOutputFile()
{
	QString outfile;

	if ( m_videomode )
	{
		QString exportdir = QSettings().value( "general/exportdirvideo", "" ).toString();
		outfile = QFileDialog::getSaveFileName( 0, tr("Export video to a file"), exportdir );
	}
	else
	{
		QString exportdir = QSettings().value( "general/exportdircdg", "" ).toString();
		outfile = QFileDialog::getSaveFileName( 0, tr("Export CD+G graphics to a file"), exportdir, "CD+G (*.cdg)" );
	}

	if ( outfile.isEmpty() )
		return;

	QFileInfo finfo( outfile );

	if ( m_videomode )
		QSettings().setValue( "general/exportdirvideo", finfo.dir().absolutePath() );
	else
		QSettings().setValue( "general/exportdircdg", finfo.dir().absolutePath() );

	leOutputFile->setText( outfile );
}


void DialogExportOptions::accept()
{
	m_outputVideo = leOutputFile->text();

	if ( m_outputVideo.isEmpty() )
	{
		QMessageBox::critical( 0, tr("Output file not specified"), tr("You must specify the output video file") );
		return;
	}

	if ( !testFontSize() )
		return;

	// Store title params
	m_artist = leArtist->text();
	m_title = leTitle->text();
	m_createdBy = leTitleCreatedBy->text();

	// Store encoding params
	if ( m_videomode )
	{
		m_currentVideoFormat = pVideoEncodingProfiles->videoFormat( boxVideoProfile->currentText() );
		m_currentProfile = pVideoEncodingProfiles->videoProfile( boxVideoTarget->currentText() );

		if ( !m_currentProfile || !m_currentVideoFormat )
			return;

		m_audioEncodingMode = boxVideoAudio->currentIndex();
		m_quality = boxVideoQuality->itemData( boxVideoQuality->currentIndex() ).toInt();

		// Store rendering params
		m_project->setTag( Project::Tag_Video_activecolor, btnVideoColorActive->color().name() );
		m_project->setTag( Project::Tag_Video_bgcolor, btnVideoColorBg->color().name() );
		m_project->setTag( Project::Tag_Video_inactivecolor, btnVideoColorInactive->color().name() );
		m_project->setTag( Project::Tag_Video_infocolor, btnVideoColorInfo->color().name() );
		m_project->setTag( Project::Tag_Video_font, fontVideo->currentFont().family() );
		m_project->setTag( Project::Tag_Video_fontsize, QString::number( fontVideoSize->value() ) );
		m_project->setTag( Project::Tag_Video_titletime, QString::number( titleVideoMin->value() ) );
		m_project->setTag( Project::Tag_Video_preamble, cbVideoPreamble->isChecked() ? "1" : "0" );
	}
	else
	{
		// Store rendering params
		m_project->setTag( Project::Tag_CDG_activecolor, btnVideoColorActive->color().name() );
		m_project->setTag( Project::Tag_CDG_bgcolor, btnVideoColorBg->color().name() );
		m_project->setTag( Project::Tag_CDG_inactivecolor, btnVideoColorInactive->color().name() );
		m_project->setTag( Project::Tag_CDG_infocolor, btnVideoColorInfo->color().name() );
		m_project->setTag( Project::Tag_CDG_font, fontVideo->currentFont().family() );
		m_project->setTag( Project::Tag_CDG_fontsize, QString::number( fontVideoSize->value() ) );
		m_project->setTag( Project::Tag_CDG_titletime, QString::number( titleVideoMin->value() ) );
		m_project->setTag( Project::Tag_CDG_preamble, cbVideoPreamble->isChecked() ? "1" : "0" );
	}

	QDialog::accept();
}

void DialogExportOptions::videoMediumChanged(int newvalue)
{
	// This filters off target-specific profiles such as DVD/PAL and DVD/NTSC for DVD
	QStringList	profilenames = pVideoEncodingProfiles->videoProfiles();
	boxVideoTarget->clear();

	for ( int i = 0; i < profilenames.size(); i++ )
	{
		const VideoEncodingProfile * p = pVideoEncodingProfiles->videoProfile( profilenames[i] );

		if ( !p )
			continue;

		if ( p->type != newvalue )
			continue;

		boxVideoTarget->addItem( profilenames[i] );
	}

	// Update the qualities
	videoTargetChanged( 0 );
}

void DialogExportOptions::videoTargetChanged( int )
{
	// This filters off video profile-specific formats (i.e. no PAL video encodings for NTSC formats),
	// as well as quality settings.
	boxVideoProfile->clear();
	boxVideoQuality->clear();

	m_currentProfile = pVideoEncodingProfiles->videoProfile( boxVideoTarget->currentText() );

	if ( !m_currentProfile )
		return;

	// Filter out the video formats
	QStringList videoFormats = pVideoEncodingProfiles->videoFormats();

	Q_FOREACH ( QString format, videoFormats )
	{
		if ( !m_currentProfile->limitFormats.empty() && !m_currentProfile->limitFormats.contains( format ) )
			continue;

		boxVideoProfile->addItem( format );
	}

	if ( m_currentProfile->bitratesEnabled[VideoEncodingProfile::BITRATE_HIGH] )
		boxVideoQuality->addItem( tr("High"), VideoEncodingProfile::BITRATE_HIGH );

	if ( m_currentProfile->bitratesEnabled[VideoEncodingProfile::BITRATE_MEDIUM] )
		boxVideoQuality->addItem( tr("Medium"), VideoEncodingProfile::BITRATE_MEDIUM );

	if ( m_currentProfile->bitratesEnabled[VideoEncodingProfile::BITRATE_LOW] )
		boxVideoQuality->addItem( tr("Low"), VideoEncodingProfile::BITRATE_LOW );

	boxVideoProfile->setEnabled( boxVideoProfile->count() > 1 );
	boxVideoQuality->setEnabled( boxVideoQuality->count() > 1 );
}

void DialogExportOptions::videoShowDetails()
{
	m_currentVideoFormat = pVideoEncodingProfiles->videoFormat( boxVideoProfile->currentText() );
	m_currentProfile = pVideoEncodingProfiles->videoProfile( boxVideoTarget->currentText() );

	if ( !m_currentProfile || !m_currentVideoFormat )
		return;

	m_audioEncodingMode = boxVideoAudio->currentIndex();
	m_quality = boxVideoQuality->itemData( boxVideoQuality->currentIndex() ).toInt();

	QString data = tr("<table border=0>"
					  "<tr colspan=2><td><b>Video parameters</b></td></tr>"
					  "<tr><td>Codec:</td><td>$videoCodec</td></tr>"
					  "<tr><td>Container:</td><td>$videoContainer</td></tr>"
					  "<tr><td>Resolution:</td><td>$videoResolution</td></tr>"
					  "<tr><td>Frame rate:</td><td>$frameRate FPS</td></tr>"
					  "<tr><td>Bitrate:</td><td>$videoBitrate</td></tr>"
					  "<tr><td>Display&nbsp;aspect&nbsp;ratio:</td><td>$displayAspectRatio</td></tr>"
					  "<tr><td>Sample&nbsp;aspect&nbsp;ratio:</td><td>$sampleAspectRatio</td></tr>"
					  "<tr><td>Progressive:</td><td>$progressive</td></tr>"
					  "<tr colspan=2><td>&nbsp;</td></tr>"
					  "<tr colspan=2><td><b>Audio parameters</b></td></tr>"
					  "<tr><td>Codec:</td><td>$audioCodec</td></tr>"
					  "<tr><td>Sample rate:</td><td>$audioSampleRate</td></tr>"
					  "<tr><td>Channels:</td><td>$audioChannels</td></tr>"
					  "<tr><td>Bitrate:</td><td>$audioBitrate</td></tr>"
					  "</table>");

	QMap< QString, QString > variables;
	variables["$videoCodec"] = m_currentProfile->videoCodec;
	variables["$videoContainer"] = m_currentProfile->videoContainer;
	variables["$videoResolution"] = QString("%1x%2") .arg(m_currentVideoFormat->width) .arg(m_currentVideoFormat->height);
	variables["$frameRate"] = QString::number( (double) m_currentVideoFormat->frame_rate_den / m_currentVideoFormat->frame_rate_num, 'g', 2 );
	variables["$videoBitrate"] = QString("%1Kbps") .arg( m_currentProfile->bitratesVideo[m_quality] );
	variables["$displayAspectRatio"] = QString("%1:%2") .arg( m_currentVideoFormat->display_aspect_num ) .arg( m_currentVideoFormat->display_aspect_den );
	variables["$sampleAspectRatio"] = QString("%1:%2") .arg( m_currentVideoFormat->sample_aspect_num ) .arg( m_currentVideoFormat->sample_aspect_den );
	variables["$progressive"] = m_currentVideoFormat->progressive ? "true" : "false";
	variables["$audioCodec"] = m_currentProfile->audioCodec;
	variables["$audioSampleRate"] = QString::number( m_currentProfile->sampleRate );
	variables["$audioChannels"] = QString::number( m_currentProfile->channels );
	variables["$audioBitrate"] = QString("%1Kbps") .arg( m_currentProfile->bitratesAudio[m_quality] );

	Q_FOREACH( QString key, variables.keys() )
	{
		data.replace( key, variables[key]);
	}

	QWhatsThis::showText( mapToGlobal(lblVideoDetailsLink->pos()), data );
}


QSize DialogExportOptions::getVideoSize()
{
	if ( !m_videomode )
	{
		// CD+G size is predefined
		return QSize( CDG_DRAW_WIDTH, CDG_DRAW_HEIGHT );
	}

	// Get the current video profile value
	const VideoFormat * vf = pVideoEncodingProfiles->videoFormat( boxVideoProfile->currentText() );

	if ( !vf )
		return QSize( 100, 100 );

	return QSize( vf->width, vf->height );
}

bool DialogExportOptions::videoParams(const VideoEncodingProfile **profile, const VideoFormat **format, unsigned int *audioMode, unsigned int *qualty)
{
	*profile = m_currentProfile;
	*format = m_currentVideoFormat;
	*audioMode = m_audioEncodingMode;
	*qualty = m_quality;

	return true;
}


void DialogExportOptions::activateTab( int index )
{
	// We're only interested in Preview tab
	if ( index != 1 )
	{
		adjustSize();
		return;
	}

	// Prepare the text renderer using current params
	QFont font = fontVideo->currentFont();
	font.setPointSize( fontVideoSize->value() );

	m_renderer = TextRenderer( getVideoSize().width(), getVideoSize().height() );

	// Initialize colors from m_project
	m_renderer.setLyrics( m_lyrics );
	m_renderer.setRenderFont( font );
	m_renderer.setColorBackground( btnVideoColorBg->color() );
	m_renderer.setColorTitle( btnVideoColorInfo->color() );
	m_renderer.setColorSang( btnVideoColorInactive->color() );
	m_renderer.setColorToSing( btnVideoColorActive->color() );

	// CD+G?
	if ( !m_videomode )
		m_renderer.forceCDGmode();

	// Title
	m_renderer.setTitlePageData( leArtist->text(),
								 leTitle->text(),
								 pLicensing->isValid() ? leTitleCreatedBy->text() : "",
								 titleVideoMin->value() * 1000 );

	// Preamble
	if ( cbVideoPreamble->isChecked() )
		m_renderer.setPreambleData( 4, 5000, 8 );

	// Update the image
	previewUpdateImage();
	adjustSize();
}

void DialogExportOptions::previewUpdateImage()
{
	// Update the image
	m_renderer.update( m_time );
	QImage img = m_renderer.image();

	// For CD+G mode we enlarge the image as it is too small
	if ( !m_videomode )
	{
		QSize scaledsize( 2 * CDG_DRAW_WIDTH, 2 * CDG_DRAW_HEIGHT );
		img = img.scaled( scaledsize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
	}

	lblImage->setPixmap( QPixmap::fromImage( img ) );

	// Update the timings
	qint64 reminder = m_project->getSongLength() - m_time;

	lblCurrent->setText( PlayerWidget::tickToString( m_time ) );
	lblTotal->setText( PlayerWidget::tickToString( reminder ) );
}

void DialogExportOptions::previewSliderMoved( int newvalue )
{
	m_time = newvalue * m_project->getSongLength() / seekSlider->maximum();
	previewUpdateImage();
}
