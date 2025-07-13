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


#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QObject>
#include <QPainter>
#include <QFlags>
#include <QMutex>

#include <gst/gst.h>
#include <gst/app/app.h>


// Gtreamer-based media player
class MediaPlayer : public QObject
{
    Q_OBJECT

    public:
        MediaPlayer( QObject * parent = 0 );
        ~MediaPlayer();

        // Player state
        enum State
        {
            StateInitial = 1,   // no media loaded
            StatePlaying,
            StatePaused,
            StateFailed     // Media failed to load
        };

        // Options used in loadMedia
        enum
        {
            LoadAudioStream = 0x1,
            LoadVideoStream = 0x2
        };

        // Capabilities used in getCapabilities and changeCapability
        enum Capability
        {
            CapChangeVolume = 0x1,
            CapChangePitch = 0x2,
            CapChangeTempo = 0x4
        };

        Q_DECLARE_FLAGS(Capabilities, Capability)

    signals:
        // The media loading is finished, either successfully or not.
        void    mediaLoadingFinished( State newstate, QString error );

        // The media file has finished naturally
        void    finished();

        // Media tags are available
        void    tagsChanged( QString artist, QString title );

        // Media duration changed/became available
        void    durationChanged();

    public:
        // Loads the media file, and plays audio, video or both
        void    loadMedia( const QString &file, int load_options );

        // Creates a video encoder pipeline
        void    prepareVideoEncoder( const QString &inputAudioFile,
                                     const QString &outputFile,
                                     const QString &encodingProfile,
                                     const QSize &resolution,
                                     std::function<const QImage( qint64 )> frameRetriever );

        // This loads the media synchronously (by holding the process exec loop) and returns the state
        State   loadMediaSync( const QString &file, int load_options );

        //
        // Player actions
        //
        void    play();
        void    pause();
        void    seekTo( qint64 pos );
        void    stop();

        // Reports player media position and duration. Returns -1 if unavailable.
        qint64  position();
        qint64  duration();

        // Reports current player state
        State   state() const;
        bool    isMediaReady() const;
        bool    isPlaying() const;

        // Gets the media tags (if available)
        void    mediaTags( QString& artist, QString& title );

        // Sets the player capacity. The parameter is a value which differs:
        // CapChangeVolume: supports range from 0 (muted) - 100 (full)
        // CapChangePitch: supports range from -50 to +50 up to player interpretation
        // CapChangeTempo: supports range from -50 to +50 up to player interpretation
        bool    setCapabilityValue( Capability cap, int value );

        // Returns the supported player capabilities, which are settable (if available)
        Capabilities capabilities() const;

        // Draws a last video frame using painter p on a rect. Does nothing if video is
        // not played, or not available.
        void    drawVideoFrame( QPainter& p, const QRect& rect );

    private:
        void    loadMediaGeneric();

        // Resets the pipeline
        void    reset();

        // Sets up the source according to file type
        GstElement *getElement(const QString &name) const;

        // Change the pipeline state, and emit error if failed
        void    setPipelineState( GstState state );

        // Stores the error, emits a message and prints into log
        void    reportError( const QString& text );

        // Logging through signal
        void    addlog( const char *type, const char * str, ... );

        // Source callbacks
        static void cb_source_need_data( GstAppSrc *src, guint length, gpointer user_data );
        static void cb_source_enough_data( GstAppSrc *src, gpointer user_data );

        // Video sink callback
        static GstFlowReturn cb_new_sample( GstAppSink *appsink, gpointer user_data );

        // Bus callback
        static GstBusSyncReply cb_busMessageDispatcher( GstBus *bus, GstMessage *message, gpointer userData );

        QString     m_mediaFile;
        QString     m_errorMsg;
        gint64      m_duration;

        // For video encoding
        QSize       m_videoResolution;
        GstElement* m_gst_source;
        std::function<const QImage( qint64 )> m_frameRetriever;
        qint64      m_videoPosition;
        bool        m_EOFseen;
        bool        m_enoughData;

        // Those are created objects
        GstElement *m_gst_pipeline;
        GstBus   *  m_gst_bus;

        qint64      m_lastKnownPosition;
        int         m_loadOptions;

        // Current pipeline state
        QAtomicInt  m_playState;
        bool        m_errorsDetected;
        bool        m_mediaLoading;

        QMutex      m_lastVideoSampleMutex;
        GstSample * m_lastVideoSample;

        // Media information from tags
        QString     m_mediaArtist;
        QString     m_mediaTitle;
};

#endif // MEDIAPLAYER_H
