/**************************************************************************
 *  Karlyriceditor - a lyrics editor for Karaoke songs                    *
 *  Copyright (C) 2009 George Yunaev, support@karlyriceditor.com          *
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

#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include <QObject>

class AudioPlayerPrivate;


// Must be a single instance
class AudioPlayer : public QObject
{
	Q_OBJECT

	public:
		AudioPlayer();
		virtual ~AudioPlayer();

		// If a function below returned an error, this function will
		// retrieve the error message
		QString	errorMsg() const;

		// Initialize the player
		bool	init();

		// Open the audio file. Returns true on success, false otherwise
		bool	open( const QString& filename );

		// Closes the audio file. If another file is opened, the previous will
		// be closed automatically.
		void	close();

		// True if audio is playing
		bool	isPlaying() const;

		// Current playing time
		qint64	currentTime() const;

		// The audio file length
		qint64	totalTime() const;

	signals:
		// Emitted every time the new audio packet is processed
		void	tick( qint64 tickvalue );

	public slots:
		// Start or continues playing
		void	play();

		// Pauses the playing
		void	stop();

		// Rewinds the music file back; to emulate real "stop" call "stop" and "reset"
		void	reset();

		// Rewinds to a specific position
		void	seekTo( qint64 value );

	private slots:
		friend class AudioPlayerPrivate;
		void	emitTickSignal( qint64 tickvalue );

	private:
		AudioPlayerPrivate *	d;
};

extern AudioPlayer  * pAudioPlayer;

#endif // AUDIOPLAYER_H
