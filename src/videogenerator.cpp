#include <QProgressDialog>
#include <QApplication>
#include <QMessageBox>

#include "audioplayer.h"
#include "videogenerator.h"
#include "textrenderer.h"
#include "videoexportoptions.h"
#include "ffmpegvideoencoder.h"


VideoGenerator::VideoGenerator( Project * prj )
{
	m_project = prj;
}


void VideoGenerator::generate( const Lyrics& lyrics, qint64 total_length )
{
	// Show the dialog with video options
	VideoExportOptionsDialog dlg( m_project );

	if ( dlg.exec() != QDialog::Accepted )
		return;

	// Validate params
	unsigned int num, den;
	QSize videosize = dlg.getVideoSize( m_project );
	dlg.getFPS( m_project, &num, &den );

	if ( videosize.isEmpty() || num == 0 || den == 0 )
	{
		QMessageBox::critical( 0, "Invalid parameters", "Video size or FPS is not valid" );
		return;
	}

	// Prepare the renderer
	TextRenderer lyricrenderer( videosize.width(), videosize.height() );
	lyricrenderer.setData( lyrics );

	// Initialize colors from m_project
	lyricrenderer.setRenderFont( QFont( m_project->tag(Project::Tag_Video_font), m_project->tag(Project::Tag_Video_fontsize).toInt()) );
	lyricrenderer.setColorBackground( m_project->tag( Project::Tag_Video_bgcolor ) );
	lyricrenderer.setColorTitle( m_project->tag( Project::Tag_Video_infocolor ) );
	lyricrenderer.setColorSang( m_project->tag( Project::Tag_Video_inactivecolor ) );
	lyricrenderer.setColorToSing( m_project->tag( Project::Tag_Video_activecolor ) );

	// Title
	lyricrenderer.setTitlePageData( m_project->tag( Project::Tag_Artist ),
								 m_project->tag( Project::Tag_Title ),
								 m_project->tag( Project::Tag_Video_titletime ).toInt() * 1000 );

	// Preamble
	if ( m_project->tag( Project::Tag_Video_preamble).toInt() != 0 )
		lyricrenderer.setPreambleData( 4, 5000, 8 );

	// Video encoder
	FFMpegVideoEncoder encoder;

	QString errmsg = encoder.createFile( dlg.m_outputVideo,
										dlg.getContainer( m_project ),
										videosize,
										2000000,
										num, den,
										//m_project->tag( Project::Tag_Video_AllKeyframes ).toInt() > 0 ? 1 : (int) (num * 5 / den), // every 5 secs
										25,
										m_project->tag( Project::Tag_Video_ExportNoAudio).toInt() > 0 ? 0 : pAudioPlayer );

	if ( !errmsg.isEmpty() )
	{
		QMessageBox::critical( 0,
							  "Cannot write video",
							  QString("Cannot create video file: %1") .arg(errmsg) );
		return;
	}

	// Pop up progress dialog
	QProgressDialog progressdlg ("Rendering video lyrics",
						 QString::null,
						 0,
						 99 );

	progressdlg.setValue( 0 );
	progressdlg.show();

	qint64 dialog_step = total_length / 100;
	qint64 time_step = (1000 * num) / den;

	// Rendering
	for ( qint64 time = 0; time < total_length; time += time_step )
	{
		// Should we show the next step?
		if ( time / dialog_step > progressdlg.value() )
		{
			progressdlg.setValue( time / dialog_step );
			qApp->processEvents( QEventLoop::ExcludeUserInputEvents );
		}

		lyricrenderer.update( time );
		encoder.encodeImage( lyricrenderer.image() );
	}

	encoder.close();
}
