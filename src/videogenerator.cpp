#include <QProgressDialog>
#include <QApplication>
#include <QMessageBox>

#include "audioplayer.h"
#include "videogenerator.h"
#include "textrenderer.h"
#include "ffmpegvideoencoder.h"


VideoGenerator::VideoGenerator( Project * prj )
{
	m_project = prj;
	m_size = QSize( 720, 480 );
	m_fps = 25;
}

void VideoGenerator::generate( const Lyrics& lyrics, qint64 total_length, const QString& outfile )
{
	// Prepare the renderer
	TextRenderer lyricrenderer( m_size.width(), m_size.height() );
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
								 5000 );

	// Preamble
	lyricrenderer.setPreambleData( 4, 5000, 8 );

	// Video encoder
	FFMpegVideoEncoder encoder;

	if ( !encoder.createFile( outfile,
							 m_size.width(),
							 m_size.height(),
							 2000000,
							 25,
							 25,
							 0/*pAudioPlayer*/) )
	{
		QMessageBox::critical( 0, "Cannot write video", "Cannot create video file" );
		return;
	}

	// Pop up progress dialog
	QProgressDialog dlg ("Rendering video lyrics",
						 QString::null,
						 0,
						 99 );

	dlg.setMinimumDuration( 2000 );
	dlg.setValue( 0 );

	qint64 dialog_step = total_length / 100;
	qint64 time_step = 1000 / m_fps;

	// Rendering
	for ( qint64 time = 0; time < total_length; time += time_step )
	{
		// Should we show the next step?
		if ( time / dialog_step > dlg.value() )
		{
			dlg.setValue( time / dialog_step );
			qApp->processEvents( QEventLoop::ExcludeUserInputEvents );
		}

		lyricrenderer.update( time );
		encoder.encodeImage( time, lyricrenderer.image() );
	}

	encoder.close();
}
