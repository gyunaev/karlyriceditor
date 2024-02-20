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

#ifndef AUDIOPLAYERPRIVATE_H
#define AUDIOPLAYERPRIVATE_H

#include <QMutex>
#include <QString>
#include <QAudioSink>

#include "ffmpeg_headers.h"

class FFMpegVideoEncoderPriv;

typedef uint8_t		Uint8;

class AudioPlayerPrivate : public QIODevice
{
    Q_OBJECT

	public:
		AudioPlayerPrivate();

		bool	init();
        bool	openAudio( const QString& filename );
        void	closeAudio();
		void	play();
        void	resetAudio();
		void	stop();
		void	seekTo( qint64 value );
		bool	isPlaying() const;
		qint64	currentTime() const;
		qint64	totalTime() const;
		QString	errorMsg() const;

		// Meta tags
		QString			m_metaTitle;
		QString			m_metaArtist;
		QString			m_metaAlbum;

    protected:
        virtual qint64 readData(char *data, qint64 maxlen) override;
        virtual qint64 writeData(const char *data, qint64 len) override;
        virtual qint64 bytesAvailable() const;

    private slots:
        void    audioStateChanged(QAudio::State newState);

	private:
		// Called from the callback
		bool	MoreAudio();
		void	queueClear();

	private:
		// Video encoder private class gets direct access to ffmpeg stuff
		friend class FFMpegVideoEncoderPriv;

		QString			m_errorMsg;

		// Access to everything below is guarded by mutex
		mutable QMutex	m_mutex;

        QAudioSink     * m_audioDevice;

        // FFMpeg decoder specific data
		AVFormatContext *pFormatCtx;
		int				 audioStream;
		AVCodecContext  *aCodecCtx;
        const AVCodec   *pCodec;

        // Software audio resampler
        SwrContext      *pAudioResampler;

        QAtomicInt      m_playing;
		qint64			m_currentTime;
		qint64			m_totalTime;

		// Currently processed frame
        AVFrame		*	m_decodedFrame;

        // Output buffer keeping the audio data
        QByteArray		m_sample_buffer;
        unsigned int	m_sample_buf_size;
        unsigned int	m_sample_buf_idx;
};

#endif // AUDIOPLAYERPRIVATE_H
