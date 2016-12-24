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

#include <QObject>
#include "videoencodingprofiles.h"

VideoEncodingProfiles * pVideoEncodingProfiles = 0;


// Embedded video encoding formats from openshot
static VideoFormat video_formats[] =
{
	{ "DV/DVD Widescreen PAL (Anamorphic)", 720, 576, 25, 1, 64, 45, 16, 9, VIFO_INTERLACED, 0         },
	{ "QVGA Widescreen 29.97 fps", 426, 240, 30000, 1001, 1, 1, 16, 9, VIFO_INTERLACED, 601    },
	{ "HD 720p 23.98 fps", 1280, 720, 24000, 1001, 1, 1, 16, 9, VIFO_INTERLACED, 709   },
	{ "SVCD Widescreen NTSC", 480, 480, 30000, 1001, 20, 11, 16, 9, 0, 601      },
	{ "CIF PAL", 352, 288, 25, 1, 59, 54, 4, 3, VIFO_INTERLACED, 601   },
	{ "QVGA 29.97 fps", 320, 240, 30000, 1001, 1, 1, 4, 3, VIFO_INTERLACED, 601        },
	{ "1024x576 16:9 PAL", 1024, 576, 25, 1, 1, 1, 16, 9, VIFO_INTERLACED, 601         },
	{ "HD 1080p 23.98 fps", 1920, 1080, 24000, 1001, 1, 1, 16, 9, VIFO_INTERLACED, 709         },
	{ "HD 720p 24 fps", 1280, 720, 24, 1, 1, 1, 16, 9, VIFO_INTERLACED, 709    },
	{ "HD 1080p 29.97 fps", 1920, 1080, 30000, 1001, 1, 1, 16, 9, VIFO_INTERLACED, 709         },
	{ "HDV 1440x1080p 29.97 fps", 1440, 1080, 30000, 1001, 4, 3, 16, 9, VIFO_INTERLACED, 709   },
	{ "VCD PAL", 352, 288, 25, 1, 59, 54, 4, 3, VIFO_INTERLACED, 601   },
	{ "HD 720p 50 fps", 1280, 720, 50, 1, 1, 1, 16, 9, VIFO_INTERLACED, 709    },
	{ "CIF 15 fps", 352, 288, 15, 1, 59, 54, 4, 3, VIFO_INTERLACED, 601        },
	{ "384x288 4:3 PAL", 384, 288, 25, 1, 1, 1, 4, 3, VIFO_INTERLACED, 601     },
	{ "512x288 16:9 PAL", 512, 288, 25, 1, 1, 1, 16, 9, VIFO_INTERLACED, 601   },
	{ "HD 720p 29.97 fps", 1280, 720, 30000, 1001, 1, 1, 16, 9, VIFO_INTERLACED, 709   },
	{ "HDV 1440x1080i 25 fps", 1440, 1080, 25, 1, 4, 3, 16, 9, 0, 709   },
	{ "HD 1080i 25 fps", 1920, 1080, 25, 1, 1, 1, 16, 9, 0, 709         },
	{ "VCD NTSC", 352, 240, 30000, 1001, 10, 11, 4, 3, VIFO_INTERLACED, 601    },
	{ "HDV 720 24p", 1280, 720, 24, 1, 1, 1, 16, 9, VIFO_INTERLACED, 0         },
	{ "HD 1080p 25 fps", 1920, 1080, 25, 1, 1, 1, 16, 9, VIFO_INTERLACED, 709  },
	{ "NTSC 29.97 fps", 720, 480, 30000, 1001, 8, 9, 4, 3, 0, 601       },
	{ "HD 1080i 30 fps", 1920, 1080, 30, 1, 1, 1, 16, 9, 0, 709         },
	{ "HDV 1440x1080i 29.97 fps", 1440, 1080, 30000, 1001, 4, 3, 16, 9, 0, 709  },
	{ "SVCD PAL", 480, 576, 25, 1, 59, 36, 4, 3, 0, 601         },
	{ "HDV 1080 25i 1920x1080", 1920, 1080, 25, 1, 1, 1, 16, 9, 0, 0    },
	{ "DV/DVD NTSC", 720, 480, 30000, 1001, 8, 9, 4, 3, 0, 601  },
	{ "HD 720p 60 fps", 1280, 720, 60, 1, 1, 1, 16, 9, VIFO_INTERLACED, 709    },
	{ "HD 720p 30 fps", 1280, 720, 30, 1, 1, 1, 16, 9, VIFO_INTERLACED, 709    },
	{ "HD 720p 25 fps", 1280, 720, 25, 1, 1, 1, 16, 9, VIFO_INTERLACED, 709    },
	{ "CVD NTSC", 352, 480, 30000, 1001, 20, 11, 4, 3, 0, 601   },
	{ "HD 1080p 24 fps", 1920, 1080, 24, 1, 1, 1, 16, 9, VIFO_INTERLACED, 709  },
	{ "Mobile 360p", 320, 240, 30000, 1001, 1, 1, 4, 3, VIFO_INTERLACED, 0     },
	{ "QVGA 15 fps", 320, 240, 15, 1, 1, 1, 4, 3, VIFO_INTERLACED, 601         },
	{ "CIF NTSC", 352, 288, 30000, 1001, 10, 11, 4, 3, VIFO_INTERLACED, 601    },
	{ "HD 1080p 30 fps", 1920, 1080, 30, 1, 1, 1, 16, 9, VIFO_INTERLACED, 709  },
	{ "DV/DVD PAL", 720, 576, 25, 1, 16, 15, 4, 3, 0, 601       },
	{ "VGA Widescreen NTSC", 854, 480, 30000, 1001, 1, 1, 16, 9, VIFO_INTERLACED, 601  },
	{ "HD 1080i 29.97 fps", 1920, 1080, 30000, 1001, 1, 1, 16, 9, 0, 709        },
	{ "HDV 1440x1080p 25 fps", 1440, 1080, 25, 1, 4, 3, 16, 9, VIFO_INTERLACED, 709    },
	{ "HD 1080p 50 fps", 1920, 1080, 50, 1, 1, 1, 16, 9, VIFO_INTERLACED, 709  },
	{ "CVD PAL", 352, 576, 25, 1, 59, 27, 4, 3, 0, 601  },
	{ "DV/DVD Widescreen NTSC", 720, 480, 30000, 1001, 32, 27, 16, 9, 0, 601    },
	{ "768x576 4:3 PAL", 768, 576, 25, 1, 1, 1, 4, 3, VIFO_INTERLACED, 601     },
	{ "VGA NTSC", 640, 480, 30000, 1001, 1, 1, 4, 3, VIFO_INTERLACED, 601      },
	{ "HD 720p 59.94 fps", 1280, 720, 60000, 1001, 1, 1, 16, 9, VIFO_INTERLACED, 709   },
	{ "HD 1080p 60 fps", 1920, 1080, 60, 1, 1, 1, 16, 9, VIFO_INTERLACED, 709  },
	{ "SVCD NTSC", 480, 480, 30000, 1001, 15, 11, 4, 3, 0, 601  },
	{ "SVCD Widescreen PAL", 480, 576, 25, 1, 59, 27, 16, 9, 0, 601     },
	{ "QCIF NTSC", 176, 144, 30000, 1001, 10, 11, 4, 3, VIFO_INTERLACED, 601   },
	{ "DV/DVD Widescreen PAL", 720, 576, 25, 1, 64, 45, 16, 9, 0, 601   },
	{ "QCIF 15 fps", 176, 144, 15, 1, 59, 54, 4, 3, VIFO_INTERLACED, 601       },
	{ "QCIF PAL", 176, 144, 25, 1, 59, 54, 4, 3, VIFO_INTERLACED, 601  },
	{ "HDV 1080 25p 1920x1080", 1920, 1080, 25, 1, 1, 1, 16, 9, VIFO_INTERLACED, 0     },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};



VideoEncodingProfiles::VideoEncodingProfiles()
{
	initVideoFormats();
	initInternalProfiles();
}

QStringList VideoEncodingProfiles::videoMediumTypes() const
{
	// Should match enum Type order
	QStringList list;
	list << QObject::tr("Video file") << QObject::tr("Device") << QObject::tr("Blu Ray/AVCHD") << QObject::tr("DVD") << QObject::tr("Web");

	return list;
}

QStringList VideoEncodingProfiles::videoProfiles() const
{
	return m_videoProfiles.keys();
}

const VideoEncodingProfile *VideoEncodingProfiles::videoProfile(const QString &name)
{
	QMap< QString, VideoEncodingProfile >::iterator it = m_videoProfiles.find( name );

	if ( it == m_videoProfiles.end() )
		return 0;

	return &(it.value());
}

QStringList VideoEncodingProfiles::videoFormats() const
{
	return m_videoFormats.keys();
}

const VideoFormat *VideoEncodingProfiles::videoFormat(const QString &name)
{
	QMap< QString, VideoFormat * >::iterator it = m_videoFormats.find( name );

	if ( it == m_videoFormats.end() )
		return 0;

	return it.value();
}

void VideoEncodingProfiles::initInternalProfiles()
{
	// This is auto-generated code loading openshot profile info
	VideoEncodingProfile p;

	// Metacafe
	p.type = VideoEncodingProfile::TYPE_WEB;
	p.name = "Metacafe";
	p.videoContainer = "mp4";
	p.videoCodec = "mpeg4";
	p.audioCodec = "libmp3lame";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "VGA NTSC";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 2048;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 8192;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 256;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 320;

	m_videoProfiles[ "Metacafe" ] = p;


	// Vimeo-SD Widescreen
	p.type = VideoEncodingProfile::TYPE_WEB;
	p.name = "Vimeo-SD Widescreen";
	p.videoContainer = "mp4";
	p.videoCodec = "libx264";
	p.audioCodec = "libmp3lame";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "VGA Widescreen NTSC";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 2048;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 8192;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 256;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 320;

	m_videoProfiles[ "Vimeo-SD Widescreen" ] = p;


	// OGG (theora/flac)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "OGG (theora/flac)";
	p.videoContainer = "ogg";
	p.videoCodec = "libtheora";
	p.audioCodec = "flac";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "OGG (theora/flac)" ] = p;


	// MPEG (mpeg2)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "MPEG (mpeg2)";
	p.videoContainer = "mpeg";
	p.videoCodec = "mpeg2video";
	p.audioCodec = "mp2";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "MPEG (mpeg2)" ] = p;


	// MP4 (Xvid)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "MP4 (Xvid)";
	p.videoContainer = "mp4";
	p.videoCodec = "libxvid";
	p.audioCodec = "ac3";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "MP4 (Xvid)" ] = p;


	// Vimeo-SD
	p.type = VideoEncodingProfile::TYPE_WEB;
	p.name = "Vimeo-SD";
	p.videoContainer = "mp4";
	p.videoCodec = "libx264";
	p.audioCodec = "libmp3lame";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "VGA NTSC";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 2048;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 8192;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 256;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 320;

	m_videoProfiles[ "Vimeo-SD" ] = p;


	// Flickr-HD
	p.type = VideoEncodingProfile::TYPE_WEB;
	p.name = "Flickr-HD";
	p.videoContainer = "mov";
	p.videoCodec = "libx264";
	p.audioCodec = "ac3";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "HD 720p 25 fps"<< "HD 720p 29.97 fps";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "Flickr-HD" ] = p;


	// MP4 (h.264)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "MP4 (h.264)";
	p.videoContainer = "mp4";
	p.videoCodec = "libx264";
	p.audioCodec = "ac3";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "MP4 (h.264)" ] = p;


	// Vimeo-HD
	p.type = VideoEncodingProfile::TYPE_WEB;
	p.name = "Vimeo-HD";
	p.videoContainer = "mp4";
	p.videoCodec = "libx264";
	p.audioCodec = "libmp3lame";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "HD 720p 25 fps"<< "HD 720p 29.97 fps";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 2048;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 8192;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 256;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 320;

	m_videoProfiles[ "Vimeo-HD" ] = p;


	// AVI (h.264)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "AVI (h.264)";
	p.videoContainer = "avi";
	p.videoCodec = "libx264";
	p.audioCodec = "mp2";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "AVI (h.264)" ] = p;


	// AVCHD Disks
	p.type = VideoEncodingProfile::TYPE_BLURAY;
	p.name = "AVCHD Disks";
	p.videoContainer = "mp4";
	p.videoCodec = "libx264";
	p.audioCodec = "ac3";
	p.sampleRate = 48000;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "HD 1080i 25 fps"<< "HD 1080i 30 fps"<< "HD 1080p 25 fps";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = false;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 15360;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 0;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 40960;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 256;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 0;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 256;

	m_videoProfiles[ "AVCHD Disks" ] = p;


	// OGG (theora/vorbis)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "OGG (theora/vorbis)";
	p.videoContainer = "ogg";
	p.videoCodec = "libtheora";
	p.audioCodec = "libvorbis";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "OGG (theora/vorbis)" ] = p;


	// Picasa
	p.type = VideoEncodingProfile::TYPE_WEB;
	p.name = "Picasa";
	p.videoContainer = "mp4";
	p.videoCodec = "libx264";
	p.audioCodec = "libmp3lame";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "VGA NTSC";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 2048;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 8192;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 256;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 320;

	m_videoProfiles[ "Picasa" ] = p;


	// Wikipedia
	p.type = VideoEncodingProfile::TYPE_WEB;
	p.name = "Wikipedia";
	p.videoContainer = "ogg";
	p.videoCodec = "libtheora";
	p.audioCodec = "libvorbis";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "QVGA 29.97 fps";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "Wikipedia" ] = p;


	// MOV (h.264)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "MOV (h.264)";
	p.videoContainer = "mov";
	p.videoCodec = "libx264";
	p.audioCodec = "ac3";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "MOV (h.264)" ] = p;


	// DVD-NTSC
	p.type = VideoEncodingProfile::TYPE_DVD;
	p.name = "DVD-NTSC";
	p.videoContainer = "dvd";
	p.videoCodec = "mpeg2video";
	p.audioCodec = "ac3";
	p.sampleRate = 48000;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "DV/DVD NTSC"<< "DV/DVD Widescreen NTSC";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 1024;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 3072;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 5120;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 192;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 256;

	m_videoProfiles[ "DVD-NTSC" ] = p;


	// AVI (mpeg2)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "AVI (mpeg2)";
	p.videoContainer = "avi";
	p.videoCodec = "mpeg2video";
	p.audioCodec = "mp2";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "AVI (mpeg2)" ] = p;


	// WEBM (vpx)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "WEBM (vpx)";
	p.videoContainer = "webm";
	p.videoCodec = "libvpx";
	p.audioCodec = "libvorbis";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "WEBM (vpx)" ] = p;


	// FLV (h.264)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "FLV (h.264)";
	p.videoContainer = "flv";
	p.videoCodec = "libx264";
	p.audioCodec = "libmp3lame";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "FLV (h.264)" ] = p;


	// MP4 (mpeg4)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "MP4 (mpeg4)";
	p.videoContainer = "mp4";
	p.videoCodec = "mpeg4";
	p.audioCodec = "libmp3lame";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "MP4 (mpeg4)" ] = p;


	// Nokia nHD
	p.type = VideoEncodingProfile::TYPE_DEVICE;
	p.name = "Nokia nHD";
	p.videoContainer = "avi";
	p.videoCodec = "libxvid";
	p.audioCodec = "ac3";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "Mobile 360p";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 1024;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 3072;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 5120;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 256;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 320;

	m_videoProfiles[ "Nokia nHD" ] = p;


	// Xbox 360
	p.type = VideoEncodingProfile::TYPE_DEVICE;
	p.name = "Xbox 360";
	p.videoContainer = "avi";
	p.videoCodec = "libxvid";
	p.audioCodec = "ac3";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "DV/DVD Widescreen NTSC"<< "HD 720p 29.97 fps";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 2048;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 8192;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 256;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 320;

	m_videoProfiles[ "Xbox 360" ] = p;


	// AVI (mpeg4)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "AVI (mpeg4)";
	p.videoContainer = "avi";
	p.videoCodec = "mpeg4";
	p.audioCodec = "libmp3lame";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "AVI (mpeg4)" ] = p;


	// YouTube
	p.type = VideoEncodingProfile::TYPE_WEB;
	p.name = "YouTube";
	p.videoContainer = "mpeg";
	p.videoCodec = "mpeg2video";
	p.audioCodec = "mp2";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "VGA NTSC";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 2048;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 8192;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 256;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 320;

	m_videoProfiles[ "YouTube" ] = p;


	// Apple TV
	p.type = VideoEncodingProfile::TYPE_DEVICE;
	p.name = "Apple TV";
	p.videoContainer = "mp4";
	p.videoCodec = "libx264";
	p.audioCodec = "ac3";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "HD 720p 30 fps";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = false;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = false;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 0;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 0;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 5120;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 0;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 0;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 256;

	m_videoProfiles[ "Apple TV" ] = p;


	// MOV (mpeg4)
	p.type = VideoEncodingProfile::TYPE_FILE;
	p.name = "MOV (mpeg4)";
	p.videoContainer = "mov";
	p.videoCodec = "mpeg4";
	p.audioCodec = "libmp3lame";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 384;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 15360;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 96;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 192;

	m_videoProfiles[ "MOV (mpeg4)" ] = p;


	// DVD-PAL
	p.type = VideoEncodingProfile::TYPE_DVD;
	p.name = "DVD-PAL";
	p.videoContainer = "dvd";
	p.videoCodec = "mpeg2video";
	p.audioCodec = "ac3";
	p.sampleRate = 48000;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "DV/DVD PAL"<< "DV/DVD Widescreen PAL";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 1024;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 3072;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 5120;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 192;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 256;

	m_videoProfiles[ "DVD-PAL" ] = p;


	// YouTube-HD
	p.type = VideoEncodingProfile::TYPE_WEB;
	p.name = "YouTube-HD";
	p.videoContainer = "mp4";
	p.videoCodec = "libx264";
	p.audioCodec = "libmp3lame";
	p.sampleRate = 44100;
	p.channels = 2;
	p.limitFormats.clear();
	p.limitFormats << "HD 720p 25 fps"<< "HD 720p 29.97 fps";

	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_LOW ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_MEDIUM ] = true;
	p.bitratesEnabled[ VideoEncodingProfile::BITRATE_HIGH ] = true;

	p.bitratesVideo[ VideoEncodingProfile::BITRATE_LOW ] = 2048;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_MEDIUM ] = 5120;
	p.bitratesVideo[ VideoEncodingProfile::BITRATE_HIGH ] = 8192;

	p.bitratesAudio[ VideoEncodingProfile::BITRATE_LOW ] = 128;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_MEDIUM ] = 256;
	p.bitratesAudio[ VideoEncodingProfile::BITRATE_HIGH ] = 320;

	m_videoProfiles[ "YouTube-HD" ] = p;
}

void VideoEncodingProfiles::initVideoFormats()
{
	for ( VideoFormat * f = video_formats; f->name; f++ )
	{
		m_videoFormats[ f->name ] = f;
	}
}
