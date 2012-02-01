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

#include <QMessageBox>

#include "audioplayer.h"
#include "audioplayerprivate.h"
#include <SDL/SDL.h>

// SDL defines its own main() function in SDL_main. And so does Qt, so if we continue without
// the #define below we'll end up with the following link error:
// libqtmain.a(qtmain_win.o):qtmain_win.cpp:(.text+0x159): undefined reference to `qMain(int, char**)'
#undef main

#define SDL_AUDIO_BUFFER_SIZE 1024

// SDL audio callback
void sdl_audio_callback( void *userdata, Uint8 *stream, int len)
{
	AudioPlayerPrivate * player = (AudioPlayerPrivate*) userdata;
	player->SDL_audio_callback( stream, len );
}


AudioPlayerPrivate::AudioPlayerPrivate()
{
	pFormatCtx = 0;
	aCodecCtx = 0;
	pCodec = 0;

	m_audioOpened = false;
	m_playing = false;
	m_currentTime = 0;
	m_totalTime = 0;

	m_sample_buf_size = 0;
	m_sample_buf_idx = 0;
}

bool AudioPlayerPrivate::isPlaying() const
{
	QMutexLocker m( &m_mutex );
	return m_playing;
}

qint64 AudioPlayerPrivate::currentTime() const
{
	QMutexLocker m( &m_mutex );
	return m_currentTime;
}

qint64 AudioPlayerPrivate::totalTime() const
{
	QMutexLocker m( &m_mutex );
	return m_totalTime;
}

QString	AudioPlayerPrivate::errorMsg() const
{
	return m_errorMsg;
}


bool AudioPlayerPrivate::init()
{
	// Init FFMpeg stuff
	ffmpeg_init_once();

	// Init the SDL
	if ( SDL_Init (SDL_INIT_AUDIO ) )
	{
		QMessageBox::critical( 0,
							  QObject::tr("SDL init failed"),
							  QObject::tr("Cannot initialize audio subsystem:\n\n%1").arg(SDL_GetError()) );
		return false;
	}

	return true;
}

void AudioPlayerPrivate::close()
{
	QMutexLocker m( &m_mutex );
	SDL_PauseAudio( 1 );

	// Close the codec
	if ( aCodecCtx )
		avcodec_close( aCodecCtx );

	// Close the video file
	if ( pFormatCtx )
		av_close_input_file( pFormatCtx );

	if ( m_audioOpened )
	{
		SDL_CloseAudio();
		m_audioOpened = false;
	}

	pFormatCtx = 0;
	aCodecCtx = 0;
	pCodec = 0;
}

bool AudioPlayerPrivate::open( const QString& filename )
{
	// Close if opened
	close();

	QMutexLocker m( &m_mutex );

	// Open the file
	if ( av_open_input_file( &pFormatCtx, FFMPEG_FILENAME( filename ), NULL, 0, NULL ) != 0 )
	{
		m_errorMsg = "Could not open the audio file";
		return false;
	}

	// Retrieve stream information
	if ( av_find_stream_info( pFormatCtx ) < 0 )
	{
		m_errorMsg = "Could not find stream information in the audio file";
		return false;
	}

	// Find the first audio stream
	audioStream = -1;

	for ( unsigned i = 0; i < pFormatCtx->nb_streams; i++ )
	{
		if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
		{
			audioStream = i;
			break;
		}
	}

	if ( audioStream == -1 )
	{
		m_errorMsg = "This file does not contain any playable audio";
		return false;
	}

	if ( pFormatCtx->streams[audioStream]->duration == AV_NOPTS_VALUE )
	{
		m_errorMsg = "Cannot determine the total audio length";
		return false;
	}

	m_totalTime = av_rescale_q( pFormatCtx->streams[audioStream]->duration,
							   pFormatCtx->streams[audioStream]->time_base,
							   AV_TIME_BASE_Q ) / 1000;

	aCodecCtx = pFormatCtx->streams[audioStream]->codec;

	// Open audio codec
	AVCodec * aCodec = avcodec_find_decoder( aCodecCtx->codec_id );

	if ( !aCodec )
	{
		m_errorMsg = "Unsupported audio codec";
		return false;
	}

	avcodec_open( aCodecCtx, aCodec );

	// Now initialize the SDL audio device
	SDL_AudioSpec wanted_spec, spec;

	wanted_spec.freq = aCodecCtx->sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = aCodecCtx->channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
	wanted_spec.callback = sdl_audio_callback;
	wanted_spec.userdata = this;

	if ( SDL_OpenAudio( &wanted_spec, &spec ) < 0 )
	{
		m_errorMsg = QObject::tr("Cannot initialize audio device: %1") .arg( SDL_GetError() );
		return false;
	}

	m_audioOpened = true;

	// Init the packet queue
	queueClear();
	return true;
}

void AudioPlayerPrivate::queueClear()
{
	m_sample_buf_idx = 0;
	m_sample_buf_size = 0;
	m_sample_buffer.clear();
}


void AudioPlayerPrivate::play()
{
	QMutexLocker m( &m_mutex );
	m_playing = true;
	SDL_PauseAudio( 0 );
}

void AudioPlayerPrivate::reset()
{
	seekTo( 0 );
}

void AudioPlayerPrivate::stop()
{
	QMutexLocker m( &m_mutex );
	m_playing = false;
	SDL_PauseAudio( 1 );
}

void AudioPlayerPrivate::seekTo( qint64 value )
{
	QMutexLocker m( &m_mutex );

	av_seek_frame( pFormatCtx, -1, value * 1000, 0 );
	avcodec_flush_buffers( aCodecCtx );

	queueClear();
}

// Called from SDL thread - no GUI/Widget functions!
void AudioPlayerPrivate::SDL_audio_callback( Uint8 *stream, int len)
{
	QMutexLocker m( &m_mutex );

	while ( len > 0 )
	{
		if ( m_sample_buf_idx >= m_sample_buf_size )
		{
			// We have already sent all our data; get more
			if ( !MoreAudio() )
			{
				// If error, output silence
				memset( stream, 0, len );
				return;
			}
		}

		int len1 = m_sample_buf_size - m_sample_buf_idx;

		if ( len1 > len )
			len1 = len;

		memcpy( stream, (uint8_t *) m_sample_buffer.data() + m_sample_buf_idx, len1 );
		len -= len1;
		stream += len1;
		m_sample_buf_idx += len1;
	}
}

// Called from the callback - no GUI/Widget functions!
bool AudioPlayerPrivate::MoreAudio()
{
	while ( m_playing )
	{
		AVPacket packet;

		// Read a frame
		if ( av_read_frame( pFormatCtx, &packet ) < 0 )
			return false;  // Frame read failed (e.g. end of stream)

		if ( packet.stream_index != audioStream )
		{
			av_free_packet( &packet );
			continue;
		}

		m_sample_buf_idx = 0;
		m_sample_buf_size = 0;
		m_sample_buffer.clear();

		m_currentTime = av_rescale_q( packet.pts,
									 pFormatCtx->streams[audioStream]->time_base,
									 AV_TIME_BASE_Q ) / 1000;

		QMetaObject::invokeMethod( pAudioPlayer, "emitTickSignal", Qt::QueuedConnection, Q_ARG( qint64, m_currentTime ) );

		// Save the orig data so we can call av_free_packet() on it
		void * porigdata = packet.data;

		while ( packet.size > 0 )
		{
			int data_size = AVCODEC_MAX_AUDIO_FRAME_SIZE * 2;
			m_sample_buffer.resize( m_sample_buffer.size() + data_size );

			int len = avcodec_decode_audio3( aCodecCtx, (int16_t *) (m_sample_buffer.data() + m_sample_buf_size), &data_size, &packet );

			if ( len < 0 )
			{
				// if error, skip frame
				break;
			}

			packet.data += len;
			packet.size -= len;

			m_sample_buf_size += data_size;
		}

		packet.data = (uint8_t*) porigdata;
		av_free_packet( &packet );
		return true;
	}

	return false;
}
