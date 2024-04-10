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

#include <QFile>
#include <QMessageBox>
#include <QAudioFormat>
#include <QAudioDevice>
#include <QMediaDevices>

#include "audioplayer.h"
#include "audioplayerprivate.h"


AudioPlayerPrivate::AudioPlayerPrivate()
    : QIODevice()
{
    m_audioDevice = 0;
	pFormatCtx = 0;
	aCodecCtx = 0;
	pCodec = 0;

    m_playing = 0;
	m_currentTime = 0;
	m_totalTime = 0;

    m_decodedFrame = 0;
    pAudioResampler = 0;

	m_sample_buf_size = 0;
	m_sample_buf_idx = 0;

    // Open the QIODevice unbuffered so we wouldn't have to deal with QIODevice's own buffer
    QIODevice::open( QIODevice::ReadOnly | QIODevice::Unbuffered );
}

bool AudioPlayerPrivate::isPlaying() const
{
	QMutexLocker m( &m_mutex );
    return m_playing > 0;
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

    if ( m_decodedFrame )
        av_free( m_decodedFrame );

    if ( pAudioResampler )
        swr_free( &pAudioResampler );

    pAudioResampler = 0;
    m_audioDevice = 0;
    m_decodedFrame = 0;
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

    // Find the first decodable audio stream
	audioStream = -1;

	for ( unsigned i = 0; i < pFormatCtx->nb_streams; i++ )
	{
        AVStream *stream = pFormatCtx->streams[i];
        const AVCodec *dec = avcodec_find_decoder( stream->codecpar->codec_id );

        if ( !dec )
            continue;

        AVCodecContext * codec_ctx = avcodec_alloc_context3( dec );

        if ( !codec_ctx )
            continue;

        if ( avcodec_parameters_to_context(codec_ctx, stream->codecpar ) < 0 )
        {
            avcodec_free_context( &codec_ctx );
            continue;
        }

        // Must be audio stream
        if ( codec_ctx->codec_type != AVMEDIA_TYPE_AUDIO )
        {
            avcodec_free_context( &codec_ctx );
            continue;
        }

        // Open a decoder
        if ( avcodec_open2(codec_ctx, dec, NULL) < 0 )
        {
            avcodec_free_context( &codec_ctx );
            continue;
        }

        // We got our stream
        audioStream = i;
        aCodecCtx = codec_ctx;
        pCodec = dec;

        break;
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

    AVRational baserate;
    baserate.num = 1;
    baserate.den = AV_TIME_BASE;

	m_totalTime = av_rescale_q( pFormatCtx->streams[audioStream]->duration,
							   pFormatCtx->streams[audioStream]->time_base,
                               baserate ) / 1000;

    // Now initialize the audio device
    QAudioFormat format;
    format.setSampleRate( aCodecCtx->sample_rate );
    format.setChannelCount( 2 );
    format.setSampleFormat( QAudioFormat::Int16 );

    QAudioDevice info( QMediaDevices::defaultAudioOutput() );

    if ( !info.isFormatSupported(format) )
    {
        m_errorMsg = "Your audio device cannot play this format";
        return false;
    }   

    m_audioDevice = new QAudioSink( format );

    // Allocate the first frame
    m_decodedFrame = av_frame_alloc();

    if ( !m_decodedFrame )
	{
		m_errorMsg = QObject::tr("Cannot allocate frame memory buffer");
		return false;
	}

    // Setup audio resampler
    pAudioResampler = swr_alloc();

    if ( !pAudioResampler )
    {
        m_errorMsg = QObject::tr("Cannot allocate audio resampler");
        return false;
    }

    av_opt_set_channel_layout( pAudioResampler, "in_channel_layout",  aCodecCtx->channel_layout, 0);
    av_opt_set_channel_layout( pAudioResampler, "out_channel_layout", AV_CH_LAYOUT_STEREO,  0);

    av_opt_set_int( pAudioResampler, "in_sample_rate",     aCodecCtx->sample_rate, 0);
    av_opt_set_int( pAudioResampler, "out_sample_rate",    aCodecCtx->sample_rate, 0);

    av_opt_set_sample_fmt( pAudioResampler, "in_sample_fmt",  aCodecCtx->sample_fmt, 0);
    av_opt_set_sample_fmt( pAudioResampler, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);

    if ( swr_init(pAudioResampler) < 0 )
    {
        m_errorMsg = QObject::tr("Cannot initialize audio resampler");
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
    m_playing = 1;
    m_audioDevice->start( this );
}

void AudioPlayerPrivate::resetAudio()
{
	seekTo( 0 );
}

void AudioPlayerPrivate::stop()
{
	QMutexLocker m( &m_mutex );
    m_playing = 0;

    if ( m_audioDevice )
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
                m_playing = 0;
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

qint64 AudioPlayerPrivate::writeData(const char *, qint64 )
{
    return -1;
}

// Called from the callback - no GUI/Widget functions!
bool AudioPlayerPrivate::MoreAudio()
{
    while ( m_playing > 0 )
	{
        AVPacket * packet = av_packet_alloc();

		// Read a frame
        if ( av_read_frame( pFormatCtx, packet ) < 0 )
        {
            QMetaObject::invokeMethod( pAudioPlayer, "finished", Qt::QueuedConnection );
            av_packet_free( &packet );
			return false;  // Frame read failed (e.g. end of stream)
        }

        if ( packet->stream_index != audioStream || packet->size == 0 )
        {
            av_packet_free( &packet );
            continue;
        }

		m_sample_buf_idx = 0;
		m_sample_buf_size = 0;
		m_sample_buffer.clear();

        AVRational baserate;
        baserate.num = 1;
        baserate.den = AV_TIME_BASE;

        m_currentTime = av_rescale_q( packet->pts,
									 pFormatCtx->streams[audioStream]->time_base,
                                     baserate ) / 1000;

        QMetaObject::invokeMethod( pAudioPlayer, "tick", Qt::QueuedConnection, Q_ARG( qint64, m_currentTime ) );

        // Send the packet with the compressed data to the decoder
        if ( avcodec_send_packet( aCodecCtx, packet ) < 0)
        {
            qWarning( "Error while submitting packet to decoder" );
            QMetaObject::invokeMethod( pAudioPlayer, "finished", Qt::QueuedConnection );
            av_packet_free( &packet );
            return false;
        }

        // Read all the output frames (in general there may be any number of them)
        while ( true )
        {
            int ret = avcodec_receive_frame( aCodecCtx, m_decodedFrame );

            if ( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF )
                break;

            if ( ret < 0 )
            {
                qWarning( "Error %d during decoding", ret );
                QMetaObject::invokeMethod( pAudioPlayer, "finished", Qt::QueuedConnection );
                av_packet_free( &packet );
                return false;
            }

            // For output we use AV_SAMPLE_FMT_S16 (2 bytes) and 2 channels
            int total = m_decodedFrame->nb_samples * 2 * 2;
            m_sample_buffer.resize( m_sample_buf_size + total );

            // We need the interleaved data, so we make it a single item array
            uint8_t* outputdata[ 1 ];
            outputdata[0] = (uint8_t*) (m_sample_buffer.data() + m_sample_buf_size);

            swr_convert( pAudioResampler,
                         outputdata,
                         m_decodedFrame->nb_samples,
                         (const uint8_t**) m_decodedFrame->extended_data,
                         m_decodedFrame->nb_samples );

            m_sample_buf_size += total;
        }

        // We should have data in our buffer
        av_packet_free( &packet );
        return true;
	}

    return false;
}
