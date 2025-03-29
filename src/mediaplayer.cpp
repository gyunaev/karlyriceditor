/**************************************************************************
 *  Karlyriceditor - a lyrics editor and CD+G / video export for Karaoke  *
 *  songs.                                                                *
 *  Copyright (C) 2009-2025 George Yunaev, gyunaev@ulduzsoft.com          *
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


#include <QCoreApplication>
#include <QObject>
#include <QPainter>
#include <QFile>
#include <QUrl>

#include <gst/gst.h>

#include "mediaplayer.h"
#include "settings.h"

MediaPlayer::MediaPlayer( QObject * parent )
    : QObject( parent )
{
    m_gst_pipeline = 0;
    m_gst_bus = 0;
    m_gst_source = 0;
    m_lastVideoSample = 0;

    m_duration = -1;
    m_tempoRatePercent = 100;
    m_playState = StateInitial;
    m_errorsDetected = false;
}

MediaPlayer::~MediaPlayer()
{
    reset();
}

void MediaPlayer::loadMedia(const QString &file, int load_options )
{
    reset();

    m_mediaFile = file;
    m_loadOptions = load_options;

    loadMediaGeneric();
}

MediaPlayer::State MediaPlayer::loadMediaSync( const QString &filename, int load_options  )
{
    loadMedia( filename, load_options );

    // Player will load things asynchronously so we will not return until it's either loaded or failed
    while ( state() == StateInitial )
    {
        QCoreApplication::processEvents( QEventLoop::ExcludeUserInputEvents, 250 );
    }

    return state();
}

void MediaPlayer::play()
{
    setPipelineState( GST_STATE_PLAYING );
}

void MediaPlayer::pause()
{
    GstState state = GST_STATE(m_gst_pipeline);

    if ( state != GST_STATE_PAUSED )
        setPipelineState( GST_STATE_PAUSED );
}

void MediaPlayer::seekTo(qint64 pos)
{
    // Calculate the new tempo rate as it is used to change the tempo (that's actually the only way in GStreamer)
    //
    double temporate = m_tempoRatePercent / 100.0;

    GstEvent * seek_event = gst_event_new_seek( temporate,
                                                GST_FORMAT_TIME,
                                                (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                                                GST_SEEK_TYPE_SET,
                                                pos * GST_MSECOND,
                                                GST_SEEK_TYPE_NONE,
                                                0 );

    gst_element_send_event( m_gst_pipeline, seek_event );
}

void MediaPlayer::stop()
{
    seekTo( 0 );
    setPipelineState( GST_STATE_PAUSED );
    m_lastKnownPosition = 0;
}

qint64 MediaPlayer::position()
{
    gint64 pos;

    if ( !gst_element_query_position( m_gst_pipeline, GST_FORMAT_TIME, &pos ) )
    {
        // this may happen while we're seeking, for example
        addlog( "DEBUG",  "GstMediaPlayer: querying position failed" );
        return m_lastKnownPosition;
    }

    m_lastKnownPosition = pos / GST_MSECOND;

    return m_lastKnownPosition;
}

qint64 MediaPlayer::duration()
{
    // If we didn't know it yet, query the stream duration
    if ( m_duration == -1 )
    {
        gint64 dur;

        if ( !gst_element_query_duration( m_gst_pipeline, GST_FORMAT_TIME, &dur) )
            return -1;

        m_duration = dur / GST_MSECOND;
    }

    return (m_duration * 100) / m_tempoRatePercent;
}

MediaPlayer::State MediaPlayer::state() const
{
    return (State) m_playState.loadAcquire();
}

bool MediaPlayer::isMediaReady() const
{
    return state() != State::StateInitial;
}

bool MediaPlayer::isPlaying() const
{
    return state() == State::StatePlaying;
}

void MediaPlayer::mediaTags(QString &artist, QString &title)
{
    artist = m_mediaArtist;
    title = m_mediaTitle;
}

bool MediaPlayer::setCapabilityValue( MediaPlayer::Capability cap, int value)
{
    switch ( cap )
    {
    case MediaPlayer::CapChangeVolume:
        g_object_set( G_OBJECT( getElement("volume")), "volume", (double) value / 100, NULL );
        return true;

    case MediaPlayer::CapChangePitch:
        return adjustPitch( value );

    case MediaPlayer::CapChangeTempo:

        // The UI gives us tempo rate as percentage, from 0 to 100 with 50 being normal value.
        // Thus we convert it into 75% - 125% range
        m_tempoRatePercent = 75 + value / 2;
        addlog( "DEBUG",  "MediaPlayer_GStreamer: tempo change: UI %d -> player %d", value, m_tempoRatePercent );
        position();
        seekTo( m_lastKnownPosition );

        // Because the total song duration now changed, we need to inform the widgets
        QMetaObject::invokeMethod( this,
                                   "durationChanged",
                                   Qt::QueuedConnection );

        break;
    }

    return false;
}

MediaPlayer::Capabilities MediaPlayer::capabilities()
{
    MediaPlayer::Capabilities caps( 0 );

    if ( getElement("volume") )
        caps |= MediaPlayer::CapChangeVolume;

    if ( pSettings->isRegistered() && getElement(pSettings->registeredDigest()) )
        caps |= MediaPlayer::CapChangePitch;

    if ( getElement("tempo") )
        caps |= MediaPlayer::CapChangeTempo;

    return caps;
}


bool MediaPlayer::adjustPitch( int newvalue )
{
    GstElement * pitch = getElement(pSettings->registeredDigest() );

    if ( !pitch )
        return false;

    // Let's spread it to 0.5 ... +1.5
    double value = (newvalue / 200.0) + 0.75;

    g_object_set( G_OBJECT(pitch), "pitch", value, NULL );
    return true;
}

void MediaPlayer::drawVideoFrame(QPainter &p, const QRect &rect)
{
    QMutexLocker m( &m_lastVideoSampleMutex );

    if ( !m_lastVideoSample )
        return;

    // get the snapshot buffer format now. We set the caps on the appsink so
    // that it can only be an rgb buffer.
    GstCaps *caps = gst_sample_get_caps( m_lastVideoSample );

    if ( !caps )
    {
        reportError( "could not get caps for the new video sample" );
        return;
    }

    GstStructure * structure = gst_caps_get_structure( caps, 0 );

    // We need to get the final caps on the buffer to get the size
    int width = 0;
    int height = 0;

    gst_structure_get_int( structure, "width", &width );
    gst_structure_get_int( structure, "height", &height );

    if ( !width || !height )
    {
        reportError( "could not get video height and width" );
        return;
    }

    // Create pixmap from buffer and save, gstreamer video buffers have a stride that
    // is rounded up to the nearest multiple of 4
    GstBuffer *buffer = gst_sample_get_buffer( m_lastVideoSample );
    GstMapInfo map;

    if ( !gst_buffer_map( buffer, &map, GST_MAP_READ ) )
    {
        reportError( "could not map video buffer" );
        return;
    }

    p.drawImage( rect, QImage( map.data, width, height, GST_ROUND_UP_4 (width * 4), QImage::Format_RGB32 ), QRect( 0, 0, width, height ) );

    // And clean up
    gst_buffer_unmap( buffer, &map );
}

void MediaPlayer::loadMediaGeneric()
{
    m_duration = -1;
    m_errorsDetected = false;

    // Initialize gstreamer if not initialized yet
    if ( !gst_is_initialized() )
    {
        //qputenv( "GST_DEBUG", "*:4" );
        gst_init(0, 0);
    }

    // The content of the pipeline - which could be video-only, audio-only or audio-video
    // See https://gstreamer.freedesktop.org/documentation/tutorials/basic/gstreamer-tools.html
    QString pipeline = "filesrc name=filesource ! decodebin name=decoder";

    // Video decoding part
    if ( (m_loadOptions & MediaPlayer::LoadVideoStream) != 0 )
       pipeline += " decoder. ! video/x-raw ! videoconvert ! video/x-raw,format=BGRA ! appsink name=videosink";

    // Audio decoding part
    if ( (m_loadOptions & MediaPlayer::LoadAudioStream) != 0 )
    {
       pipeline += " decoder. ! audio/x-raw ! audioconvert ! scaletempo name=tempo";

       if ( pSettings->isRegistered() )
           pipeline += " ! pitch name=" + pSettings->registeredDigest();

        pipeline += " ! volume name=volume ! autoaudiosink";
    }

    qDebug( "Gstreamer: setting up pipeline: '%s'", qPrintable( pipeline ) );

    GError * error = 0;
    m_gst_pipeline = gst_parse_launch( qPrintable( pipeline ), &error );

    if ( !m_gst_pipeline )
    {
        qWarning( "Gstreamer pipeline '%s' cannot be created; error %s\n", qPrintable( pipeline ), error->message );
        reportError( tr("Pipeline could not be created; most likely your GStreamer installation is incomplete.") );
        return;
    }

    //
    // Setup various elements if we have them
    //

    // Video sink - video output
    if ( getElement( "videosink" ) )
    {
        // Setup the video sink callbacks
        GstAppSinkCallbacks callbacks;
        memset( &callbacks, 0, sizeof(callbacks) );
        callbacks.new_sample = cb_new_sample;

        gst_app_sink_set_callbacks( GST_APP_SINK(getElement( "videosink" )), &callbacks, this, NULL );
    }

    // Set up the input source
    g_object_set( getElement( "filesource" ), "location", m_mediaFile.toUtf8().constData(), NULL);

    // Get the pipeline bus - store it since it has to be "unref after usage"
    m_gst_bus = gst_element_get_bus( m_gst_pipeline );

    if ( !m_gst_bus )
    {
        reportError( "Can't obtrain the pipeline bus." );
        return;
    }

    // Set the handler for the bus
    gst_bus_set_sync_handler( m_gst_bus, cb_busMessageDispatcher, this, 0 );

    setPipelineState( GST_STATE_PAUSED );
}


void MediaPlayer::prepareVideoEncoder(const QString &inputAudioFile, const QString &outputFile, const QString &encodingProfile, const QSize& resolution, std::function<const QImage (qint64)> frameRetriever )
{
    reset();

    m_errorsDetected = false;
    m_frameRetriever = frameRetriever;
    m_videoResolution = resolution;
    m_videoPosition = 0;
    m_EOFseen = false;

    // The content of the pipeline - which could be video-only, audio-only or audio-video
    // See https://gstreamer.freedesktop.org/documentation/tutorials/basic/gstreamer-tools.html
    QString pipeline = QString( "appsrc name=videosource caps=\"video/x-raw,format=(string)BGRA,width=(int)%1,height=(int)%2,framerate=(fraction)25/1\" \
                        ! enc.video_%u encodebin name=enc profile=\"%3\" \
                        ! filesink name=mediafile \
                        filesrc name=filesource \
                        ! decodebin ! audio/x-raw \
                        ! audioconvert \
                        ! scaletempo name=tempo").arg( resolution.width() ).arg( resolution.height() ).arg( encodingProfile );

    if ( pSettings->isRegistered() )
        pipeline += " ! pitch name=" + pSettings->registeredDigest();

    pipeline += " ! enc.audio_%u";

    qDebug( "Gstreamer: setting up encoding pipeline: '%s'", qPrintable( pipeline ) );

    GError * error = 0;
    m_gst_pipeline = gst_parse_launch( qPrintable( pipeline ), &error );

    if ( !m_gst_pipeline )
    {
        qWarning( "Gstreamer pipeline '%s' cannot be created; error %s\n", qPrintable( pipeline ), error->message );
        reportError( tr("Encoding pipeline could not be created; most likely your GStreamer installation is incomplete.") );
        return;
    }

    // Set up the input audio file
    g_object_set( getElement( "filesource" ), "location", inputAudioFile.toUtf8().constData(), NULL);

    // Set up the output media file
    g_object_set( getElement( "mediafile" ), "location", outputFile.toUtf8().constData(), NULL);

    // Set up input video source
    m_gst_source = getElement( "videosource" );

    gst_app_src_set_size( GST_APP_SRC(m_gst_source), -1 );
    gst_app_src_set_stream_type( GST_APP_SRC(m_gst_source), GST_APP_STREAM_TYPE_STREAM );

    GstAppSrcCallbacks callbacks = { 0, 0, 0, 0, 0 };
    callbacks.need_data = &MediaPlayer::cb_source_need_data;
    callbacks.enough_data = &MediaPlayer::cb_source_enough_data;

    gst_app_src_set_callbacks( GST_APP_SRC(m_gst_source), &callbacks, this, 0 );

    // Our sources have bytes format
    g_object_set( m_gst_source, "format", GST_FORMAT_TIME, NULL);

    // Get the pipeline bus - store it since it has to be "unref after usage"
    m_gst_bus = gst_element_get_bus( m_gst_pipeline );

    if ( !m_gst_bus )
    {
        reportError( "Can't obtrain the pipeline bus." );
        return;
    }

    // Set the handler for the bus
    gst_bus_set_sync_handler( m_gst_bus, cb_busMessageDispatcher, this, 0 );

    setPipelineState( GST_STATE_PAUSED );
}

void MediaPlayer::reset()
{
    if ( m_lastVideoSample )
    {
        gst_sample_unref( m_lastVideoSample );
        m_lastVideoSample = 0;
    }

    if ( m_gst_pipeline )
    {
        gst_element_set_state ( m_gst_pipeline, GST_STATE_NULL );

        // Clean up the bus and pipeline
        gst_bus_set_sync_handler( m_gst_bus, 0, 0, 0 );

        gst_object_unref( m_gst_bus );
        gst_object_unref( m_gst_pipeline );
    }

    m_playState = StateInitial;

    m_gst_pipeline = 0;
    m_gst_bus = 0;
    m_gst_source = 0;
    m_lastVideoSample = 0;
    m_errorsDetected = false;
    m_mediaLoading = true;
    m_tempoRatePercent = 100;

    m_mediaArtist.clear();
    m_mediaTitle.clear();
}

void MediaPlayer::setPipelineState(GstState state)
{
    GstStateChangeReturn ret = gst_element_set_state( m_gst_pipeline, state );

    if ( ret == GST_STATE_CHANGE_FAILURE )
        reportError( QString("Unable to set the pipeline to the playing state") );
}

void MediaPlayer::reportError(const QString &text)
{
    addlog( "ERROR", "GstMediaPlayer: Reported error: %s", qPrintable(text));
    m_errorMsg = text;
    m_playState = StateFailed;

    // If we're loading media, send the error message here
    if ( m_mediaLoading )
    {
        QMetaObject::invokeMethod( this,
                                   "mediaLoadingFinished",
                                   Qt::QueuedConnection,
                                   Q_ARG( State, StateFailed ),
                                   Q_ARG( QString,  m_errorMsg ) );
    }
}

GstElement *MediaPlayer::getElement(const QString& name)
{
    if ( !m_gst_pipeline )
        return 0;

    return gst_bin_get_by_name (GST_BIN(m_gst_pipeline), qPrintable(name));
}

void MediaPlayer::cb_source_need_data(GstAppSrc *src, guint length, gpointer user_data)
{
    // We render with 25FPS
    const unsigned int FRAME_DURATION_MS = 1000 / 25;

    MediaPlayer * self = reinterpret_cast<MediaPlayer*>( user_data );

    if ( self->m_EOFseen )
    {
        gst_app_src_end_of_stream( GST_APP_SRC(self->m_gst_source) );
        return;
    }

    const QImage img = self->m_frameRetriever( self->m_videoPosition );

    if ( img.isNull() )
    {
        gst_app_src_end_of_stream( GST_APP_SRC(self->m_gst_source) );
        return;
    }

    // BGRA format
    GstMapInfo map;
    GstBuffer * buf = gst_buffer_new_allocate( nullptr, img.sizeInBytes(), nullptr);
    gst_buffer_map( buf, &map, GST_MAP_WRITE);
    memcpy( map.data, img.constBits(), img.sizeInBytes() );
    gst_buffer_unmap( buf, &map );

    buf->pts = self->m_videoPosition * GST_MSECOND;
    buf->dts = buf->pts;
    buf->duration = GST_CLOCK_TIME_NONE;
    gst_app_src_push_buffer(GST_APP_SRC(self->m_gst_source), buf);

    self->m_videoPosition += FRAME_DURATION_MS;
}

void MediaPlayer::cb_source_enough_data(GstAppSrc *src, gpointer user_data)
{
    qDebug("cb_source_enough_data");
    return;
}

GstFlowReturn MediaPlayer::cb_new_sample(GstAppSink *appsink, gpointer user_data)
{
    MediaPlayer * self = reinterpret_cast<MediaPlayer*>( user_data );
    GstSample * sample = gst_app_sink_pull_sample( appsink );

    if ( sample )
    {
        QMutexLocker m( &self->m_lastVideoSampleMutex );

        if ( self->m_lastVideoSample )
            gst_sample_unref( self->m_lastVideoSample );

        self->m_lastVideoSample = sample;
    }

    return GST_FLOW_OK;
}

GstBusSyncReply MediaPlayer::cb_busMessageDispatcher( GstBus *bus, GstMessage *msg, gpointer user_data )
{
    MediaPlayer * self = reinterpret_cast<MediaPlayer*>( user_data );
    Q_UNUSED(bus);

    GError *err;
    gchar *debug_info;

    if ( GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR )
    {
        gst_message_parse_error (msg, &err, &debug_info);

        self->reportError( QString("GStreamer error received from element %1: %2, debug info: %3")
                           .arg( GST_OBJECT_NAME (msg->src) )
                           .arg( err->message)
                           .arg( debug_info ? debug_info : "none" ) );

        g_clear_error( &err );
        g_free( debug_info );
    }
    else if ( GST_MESSAGE_TYPE (msg) == GST_MESSAGE_DURATION_CHANGED )
    {
        self->addlog( "DEBUG",  "GstMediaPlayer: duration changed message" );
        self->m_duration = -1;

        // Call the signal invoker
        QMetaObject::invokeMethod( self,
                                   "durationChanged",
                                   Qt::QueuedConnection );
    }
    else if ( GST_MESSAGE_TYPE (msg) == GST_MESSAGE_EOS )
    {
        self->m_EOFseen = true;
        self->addlog( "DEBUG",  "GstMediaPlayer: media playback finished naturally, emitting finished()" );
        QMetaObject::invokeMethod( self, "finished", Qt::QueuedConnection );
    }
    else if ( GST_MESSAGE_TYPE (msg) == GST_MESSAGE_STATE_CHANGED )
    {
        GstState old_state, new_state, pending_state;

        gst_message_parse_state_changed( msg, &old_state, &new_state, &pending_state );

        // We are only interested in state-changed messages from the pipeline
        if (GST_MESSAGE_SRC (msg) == GST_OBJECT (self->m_gst_pipeline))
        {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);

            self->addlog( "DEBUG",  "GstMediaPlayer: pipeline state changed from %s to %s, pending %s (%s)",
                           gst_element_state_get_name (old_state),
                           gst_element_state_get_name (new_state),
                           gst_element_state_get_name (pending_state),
                           self->m_mediaLoading ? "loading" : "not-loading" );

            switch ( new_state )
            {
                case GST_STATE_PLAYING:
                    self->m_playState = (State) StatePlaying;
                    break;

                case GST_STATE_PAUSED:
                    self->m_playState = (State) StatePaused;

                    if ( self->m_mediaLoading )
                    {
                        self->m_mediaLoading = false;

                        self->addlog( "DEBUG",  "GstMediaPlayer: Media state set to PAUSED, %s",
                                       self->m_errorsDetected ? "but errors were detected, no event" : "sending loaded event");

                        if ( self->m_errorsDetected )
                        {
                            qErrnoWarning("strange, pipeline paused while errors!");
                        }
                        else
                        {
                            // Notify that we've finished loading (successfully or not)
                            QMetaObject::invokeMethod( self,
                                                   "mediaLoadingFinished",
                                                   Qt::QueuedConnection,
                                                   Q_ARG( State, StatePaused ),
                                                   Q_ARG( QString, "" ) );
                        }
                    }
                    break;

                case GST_STATE_READY:
                    // We ignore this state as it's meaningless for us
                    break;

                default:
                    self->addlog( "ERROR",  "GStreamerPlayer: warning unhandled state %d", new_state );
                    break;
            }
        }
    }
    else if ( GST_MESSAGE_TYPE (msg) == GST_MESSAGE_TAG )
    {
        GstTagList *tags = 0;
        gchar *value;

        gst_message_parse_tag( msg, &tags );

        if ( self->m_mediaArtist.isEmpty() && gst_tag_list_get_string( tags, "artist", &value ) )
        {
            self->addlog( "DEBUG",  "GstMediaPlayer: got artist tag %s", value );
            self->m_mediaArtist = QString::fromUtf8( value );
            g_free( value );
        }

        if ( self->m_mediaTitle.isEmpty() && gst_tag_list_get_string( tags, "title", &value ) )
        {
            self->addlog( "DEBUG",  "GstMediaPlayer: got title tag %s", value );
            self->m_mediaTitle = QString::fromUtf8( value );
            g_free( value );
        }

        // Call the signal invoker if both tags are present
        if ( !self->m_mediaTitle.isEmpty() && !self->m_mediaArtist.isEmpty() )
            QMetaObject::invokeMethod( self,
                                       "tagsChanged",
                                       Qt::QueuedConnection,
                                       Q_ARG( QString, self->m_mediaArtist ),
                                       Q_ARG( QString, self->m_mediaTitle ) );

        gst_tag_list_unref( tags );
    }

    gst_message_unref (msg);
    return GST_BUS_DROP;
}

void MediaPlayer::addlog(const char *type, const char *fmt, ... )
{
    va_list vl;
    char buf[1024];

    va_start( vl, fmt );
    vsnprintf( buf, sizeof(buf) - 1, fmt, vl );
    va_end( vl );

    qInfo( "%s %s", type, buf );
}
