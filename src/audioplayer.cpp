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

AudioPlayer  * pAudioPlayer;

//
// Audio player wrapper class
//

AudioPlayer::AudioPlayer()
{
	d = new AudioPlayerPrivate();
}

AudioPlayer::~AudioPlayer()
{
	delete d;
}

bool AudioPlayer::init()
{
	return d->init();
}

bool AudioPlayer::open( const QString& filename )
{
    return d->openAudio( filename );
}

void AudioPlayer::play()
{
	d->play();
}

void AudioPlayer::reset()
{
    d->resetAudio();
}

void AudioPlayer::stop()
{
	d->stop();
}

void AudioPlayer::seekTo( qint64 value )
{
	d->seekTo( value );
}

bool AudioPlayer::isPlaying() const
{
	return d->isPlaying();
}

qint64 AudioPlayer::currentTime() const
{
	return d->currentTime();
}

qint64 AudioPlayer::totalTime() const
{
	return d->totalTime();
}

QString	AudioPlayer::errorMsg() const
{
	return d->errorMsg();
}

void AudioPlayer::close()
{
    d->closeAudio();
}

AudioPlayerPrivate * AudioPlayer::impl()
{
	return d;
}

QString	AudioPlayer::metaTitle() const
{
	return d->m_metaTitle;
}

QString	AudioPlayer::metaArtist() const
{
	return d->m_metaArtist;
}

QString	AudioPlayer::metaAlbum() const
{
	return d->m_metaAlbum;
}
