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

#ifndef VIDEOENCODINGPROFILES_H
#define VIDEOENCODINGPROFILES_H

#include <QMap>
#include <QString>
#include <QStringList>

const static unsigned int VIFO_INTERLACED = (1<<0);

typedef struct
{
	const char *	name;
	unsigned int	width;
	unsigned int	height;
	unsigned int	frame_rate_den;
	unsigned int	frame_rate_num;
	unsigned int	sample_aspect_num;
	unsigned int	sample_aspect_den;
	unsigned int	display_aspect_num;
	unsigned int	display_aspect_den;
	unsigned int	flags;
	unsigned int	colorspace;
} VideoFormat;


class VideoEncodingProfile
{
	public:
		enum
		{
			BITRATE_LOW,
			BITRATE_MEDIUM,
			BITRATE_HIGH
		};

		enum Type
		{
			TYPE_FILE,
			TYPE_DEVICE,
			TYPE_WEB
		};

		Type			type;
		QString			name;

		// Video params
		QString			videoContainer;	// i.e. avi, mp4, flv
		QString			videoCodec;		// i.e. h264, libtheora
		QStringList		limitFormats;	// limited to specific video encoding params

		// Audio params
		QString			audioCodec;		// i.e. ac3, mp3
		unsigned int	sampleRate;		// 44100
		unsigned int	channels;		// 1 or 2

		// Bitrates
		bool			bitratesEnabled[3];
		unsigned int	bitratesVideo[3]; // in KILOBYTES
		unsigned int	bitratesAudio[3]; // in KILOBYTES
};


class VideoEncodingProfiles
{
	public:
		VideoEncodingProfiles();

		// Supported video encoding targets
		QStringList	videoMediumTypes() const;

		// List of supported video encoding profiles (both internal and external)
		QStringList	videoProfiles() const;

		// Retrieve the profile
		const VideoEncodingProfile * videoProfile( const QString& name );

		// List of supported video formats
		QStringList	videoFormats() const;

		// List of supported video formats
		const VideoFormat * videoFormat( const QString& name );

	private:
		void	initInternalProfiles();
		void	initVideoFormats();

		QMap< QString, VideoFormat * >			m_videoFormats;
		QMap< QString, VideoEncodingProfile >	m_videoProfiles;
};


extern VideoEncodingProfiles * pVideoEncodingProfiles;

#endif // VIDEOENCODINGPROFILES_H
