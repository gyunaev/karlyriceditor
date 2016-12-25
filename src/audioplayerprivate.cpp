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

#include <QMessageBox>

#include "audioplayer.h"
#include "audioplayerprivate.h"


AudioPlayerPrivate::AudioPlayerPrivate()
    : QIODevice()
{
    m_audioDevice = 0;
	pFormatCtx = 0;
	aCodecCtx = 0;
	pCodec = 0;

	m_playing = false;
	m_currentTime = 0;
	m_totalTime = 0;

	m_frame = 0;
	m_sample_buf_size = 0;
	m_sample_buf_idx = 0;

    // Open the QIODevice unbuffered so we wouldn't have to deal with QIODevice's own buffer
    QIODevice::open( QIODevice::ReadOnly | QIODevice::Unbuffered );
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

	return true;
}

void AudioPlayerPrivate::closeAudio()
{
	QMutexLocker m( &m_mutex );

    if ( m_audioDevice )
    {
        m_audioDevice->stop();
        delete m_audioDevice;
    }

	// Close the codec
	if ( aCodecCtx )
		avcodec_close( aCodecCtx );

	// Close the video file
	if ( pFormatCtx )
		avformat_close_input( &pFormatCtx );

	if ( m_frame )
		av_free( m_frame );

    m_audioDevice = 0;
	m_frame = 0;
	pFormatCtx = 0;
	aCodecCtx = 0;
	pCodec = 0;
}

static QString getMetaTag( AVDictionary* meta, const char * tagname )
{
	AVDictionaryEntry * ent = av_dict_get(meta, tagname, NULL, 0);

	if ( ent )
		return QString::fromUtf8( ent->value );
	else
		return "";
}

bool AudioPlayerPrivate::openAudio( const QString& filename )
{
	// Close if opened
    closeAudio();

	QMutexLocker m( &m_mutex );

	// Open the file
	if ( avformat_open_input( &pFormatCtx, FFMPEG_FILENAME( filename ), NULL, 0 ) != 0 )
	{
		m_errorMsg = "Could not open the audio file";
		return false;
	}

	// Retrieve stream information
	if ( avformat_find_stream_info( pFormatCtx, 0 ) < 0 )
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

	if ( pFormatCtx->streams[audioStream]->duration == (int64_t) AV_NOPTS_VALUE )
	{
		m_errorMsg = "Cannot determine the total audio length";
		return false;
	}

	// Extract some metadata
	AVDictionary* metadata = pFormatCtx->metadata;

	if ( metadata )
	{
		m_metaTitle = getMetaTag( metadata, "title" );
		m_metaArtist = getMetaTag( metadata, "artist" );
		m_metaAlbum = getMetaTag( metadata, "album" );
	}

	m_totalTime = av_rescale_q( pFormatCtx->streams[audioStream]->duration,
							   pFormatCtx->streams[audioStream]->time_base,
							   AV_TIME_BASE_Q ) / 1000;

	aCodecCtx = pFormatCtx->streams[audioStream]->codec;
	aCodecCtx->request_sample_fmt = AV_SAMPLE_FMT_S16;

	// Open audio codec
	AVCodec * aCodec = avcodec_find_decoder( aCodecCtx->codec_id );

	if ( !aCodec )
	{
		m_errorMsg = "Unsupported audio codec";
		return false;
	}

	avcodec_open2( aCodecCtx, aCodec, 0 );

    // Now initialize the audio device
    QAudioFormat format;
    format.setSampleRate( aCodecCtx->sample_rate );
    format.setChannelCount( aCodecCtx->channels );
    format.setSampleSize( 16 );
    format.setCodec("audio/pcm");
    format.setByteOrder( QAudioFormat::LittleEndian );
    format.setSampleType( QAudioFormat::SignedInt );

    QAudioDeviceInfo info( QAudioDeviceInfo::defaultOutputDevice() );

    if ( !info.isFormatSupported(format) )
    {
        m_errorMsg = "Your audio device cannot play this format";
        return false;
    }

    m_audioDevice = new QAudioOutput( format );

	// Allocate the buffer
    m_frame = av_frame_alloc();

	if ( !m_frame )
	{
		m_errorMsg = QObject::tr("Cannot allocate frame memory buffer");
		return false;
	}

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
    m_audioDevice->start( this );
}

void AudioPlayerPrivate::resetAudio()
{
	seekTo( 0 );
}

void AudioPlayerPrivate::stop()
{
	QMutexLocker m( &m_mutex );
	m_playing = false;
    m_audioDevice->stop();
}

void AudioPlayerPrivate::seekTo( qint64 value )
{
	QMutexLocker m( &m_mutex );

	av_seek_frame( pFormatCtx, -1, value * 1000, 0 );
	avcodec_flush_buffers( aCodecCtx );

	queueClear();
}

// Called from QAudioOutput thread - no GUI/Widget functions!
qint64 AudioPlayerPrivate::readData(char *data, qint64 maxSize)
{
	QMutexLocker m( &m_mutex );
    int out = 0;

    while ( out < maxSize )
	{
		if ( m_sample_buf_idx >= m_sample_buf_size )
		{
			// We have already sent all our data; get more
			if ( !MoreAudio() )
            {
                m_playing = false;
                return 0;
            }
		}

        int len = qMin( m_sample_buf_size - m_sample_buf_idx, (unsigned int) maxSize - out );
        memcpy( data + out, (uint8_t *) m_sample_buffer.data() + m_sample_buf_idx, len );

        out += len;
        m_sample_buf_idx += len;
    }

    return out;
}

qint64 AudioPlayerPrivate::writeData(const char *data, qint64 len)
{
    return -1;
}

// Called from the callback - no GUI/Widget functions!
bool AudioPlayerPrivate::MoreAudio()
{
	while ( m_playing )
	{
		AVPacket packet;

		// Read a frame
		if ( av_read_frame( pFormatCtx, &packet ) < 0 )
        {
            QMetaObject::invokeMethod( pAudioPlayer, "finished", Qt::QueuedConnection, Q_ARG( qint64, m_currentTime ) );
			return false;  // Frame read failed (e.g. end of stream)
        }

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

        QMetaObject::invokeMethod( pAudioPlayer, "tick", Qt::QueuedConnection, Q_ARG( qint64, m_currentTime ) );

		// Save the orig data so we can call av_free_packet() on it
		void * porigdata = packet.data;

		while ( packet.size > 0 )
		{
			int got_frame_ptr;
			int len = avcodec_decode_audio4( aCodecCtx, m_frame, &got_frame_ptr, &packet );

			if ( len < 0 )
			{
				// if error, skip frame
				break;
			}

			packet.data += len;
			packet.size -= len;

			if ( !got_frame_ptr )
				continue;

			void * samples = m_frame->data[0];
			int decoded_data_size = av_samples_get_buffer_size( NULL,
																aCodecCtx->channels,
																m_frame->nb_samples,
																aCodecCtx->sample_fmt, 1 );

			int cur = m_sample_buf_size;
			m_sample_buf_size += decoded_data_size;
			m_sample_buffer.resize( m_sample_buf_size );
			memcpy( m_sample_buffer.data() + cur, samples, decoded_data_size );
		}

		packet.data = (uint8_t*) porigdata;
		av_free_packet( &packet );
		return true;
	}

	return false;
}
