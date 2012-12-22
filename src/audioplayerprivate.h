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

#include "ffmpeg_headers.h"

class FFMpegVideoEncoderPriv;

typedef uint8_t		Uint8;

class AudioPlayerPrivate
{
	public:
		AudioPlayerPrivate();

		bool	init();
		bool	open( const QString& filename );
		void	close();
		void	play();
		void	reset();
		void	stop();
		void	seekTo( qint64 value );
		bool	isPlaying() const;
		qint64	currentTime() const;
		qint64	totalTime() const;
		QString	errorMsg() const;

		// Called from SDL in a different thread
		void	SDL_audio_callback( Uint8 *stream, int len);

	private:
		// Called from the callback
		bool	MoreAudio();
		void	queueClear();

	private:
		// Video encoder private class gets direct access to ffmpeg stuff
		friend class FFMpegVideoEncoderPriv;

		QString			m_errorMsg;
		bool			m_audioOpened;

		// Access to everything below is guarded by mutex
		mutable QMutex	m_mutex;

		AVFormatContext *pFormatCtx;
		int				 audioStream;
		AVCodecContext  *aCodecCtx;
		AVCodec         *pCodec;

		bool			m_playing;
		qint64			m_currentTime;
		qint64			m_totalTime;

		// Currently processed frame
		QByteArray		m_sample_buffer;
		unsigned int	m_sample_buf_size;
		unsigned int	m_sample_buf_idx;

		AVFrame		*	m_frame;
};

#endif // AUDIOPLAYERPRIVATE_H
