/**************************************************************************
 *  Karlyriceditor - a lyrics editor for Karaoke songs                    *
 *  Copyright (C) 2009-2011 George Yunaev, support@karlyriceditor.com     *
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
#include <QRegExp>
#include <QPainter>

#include "lyricsevents.h"
#include "background.h"

enum
{
	TYPE_IMAGE,
	TYPE_VIDEO,
};

LyricsEvents::LyricsEvents()
{
	m_lastUpdate = -1;
	m_nextUpdate = 0;
	m_eventTiming = 0;
}

LyricsEvents::~LyricsEvents()
{
	cleanPrepared();
}

LyricsEvents::LyricsEvents( const LyricsEvents& ev )
{
	// We do not copy m_preparedEvents and m_cachedImage
	m_events = ev.m_events;

	m_lastUpdate = -1;
	m_nextUpdate = 0;
	m_eventTiming = 0;
}

bool LyricsEvents::addEvent( qint64 timing, const QString& text )
{
	Event ev;

	if ( !parseEvent( text, &ev, 0 ) )
		return false;

	ev.timing = timing;
	m_events[ timing ] = ev;
	return true;
}

bool LyricsEvents::isEmpty() const
{
	return m_events.isEmpty();
}

QString LyricsEvents::validateEvent( const QString& text )
{
	QString errmsg;

	if ( !parseEvent( text, 0, &errmsg) )
		return errmsg;

	return "";
}

bool LyricsEvents::parseEvent( const QString& text, Event * event, QString * errmsg )
{
	QRegExp check("^(\\w+)=(.*)$");

	if ( text.trimmed().indexOf( check ) == -1 )
		return "Invalid event format; must be like IMAGE=path";

	QString key = check.cap( 1 );
	QString value = check.cap( 2 );

	if ( key == "IMAGE" )
	{
		if ( !QFile::exists( value ) )
		{
			if ( errmsg )
				*errmsg = QString("Image file %1 does not exist") .arg(value);

			return false;
		}

		QImage img;

		if ( !img.load( value ) )
		{
			if ( errmsg )
				*errmsg = QString("File %1 is not a supported image") .arg(value);

			return false;
		}

		if ( event )
		{
			event->type = TYPE_IMAGE;
			event->data = value;
		}

		return true;
	}
	else if ( key == "VIDEO" )
	{
		if ( !QFile::exists( value ) )
		{
			if ( errmsg )
				*errmsg = QString("Video file %1 does not exist") .arg(value);

			return false;
		}

		FFMpegVideoDecoder vd;

		if ( !vd.openFile( value ) )
		{
			if ( errmsg )
				*errmsg = QString("File %1 is not a supported video") .arg(value);

			return false;
		}

		if ( event )
		{
			event->type = TYPE_VIDEO;
			event->data = value;
		}

		return true;
	}

	if ( errmsg )
		*errmsg = QString("Invalid event name '%1'") .arg(key);

	return false;
}

void LyricsEvents::cleanPrepared()
{
	for ( QMap< qint64, Background* >::iterator it = m_preparedEvents.begin(); it != m_preparedEvents.end(); ++it )
		delete it.value();

	m_preparedEvents.clear();
}

bool LyricsEvents::prepare( QString * errmsg )
{
	cleanPrepared();

	for ( QMap< qint64, Event >::const_iterator it = m_events.begin(); it != m_events.end(); ++it )
	{
		Background * bgev = 0;

		switch ( it.value().type )
		{
			case TYPE_IMAGE:
				bgev = new BackgroundImage( it.value().data );
				break;

			case TYPE_VIDEO:
				bgev = new BackgroundVideo( it.value().data );
				break;
		}

		if ( !bgev )
			continue;

		if ( !bgev->isValid() )
		{
			delete bgev;

			if ( errmsg )
				*errmsg = "Invalid event";

			return false;
		}

		m_preparedEvents[ it.key() ] = bgev;
	}

	return true;
}

bool LyricsEvents::updated( qint64 timing ) const
{
	if ( m_nextUpdate == -1 )
		return false;

	if ( m_nextUpdate == 0 )
		return true;

	return ( m_nextUpdate <= timing );
}

void LyricsEvents::draw( qint64 timing, QImage& image )
{
	// Do we have precompiled events?
	if ( !m_events.isEmpty() && m_preparedEvents.isEmpty() )
		prepare();

	bool cache_changed = false;

	// Find current event
	QMap< qint64, Background* >::const_iterator found = m_preparedEvents.end();

	for ( QMap< qint64, Background* >::const_iterator it = m_preparedEvents.begin(); it != m_preparedEvents.end(); ++it )
	{
		if ( it.key() <= timing )
			found = it;
	}

	if ( found == m_preparedEvents.end() )
		return;

	Background * bg = found.value();

	// Same event as before?
	if ( found.key() != m_eventTiming )
	{
		m_eventTiming = found.key();
		cache_changed = true;
		bg->reset();
	}
	else if ( m_lastUpdate > timing )
	{
		// Time went backward?
		bg->reset();
	}

	m_lastUpdate = timing;

	if ( cache_changed || ( m_nextUpdate != -1 && ( m_nextUpdate == 0 || timing >= m_nextUpdate ) ) )
		m_nextUpdate = bg->doDraw( m_cachedImage, timing - m_eventTiming );

	QPainter p( &image );
	p.drawImage( 0, 0, m_cachedImage.scaled( image.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );

	// Adjust nextUpdate as there may be more events
	if ( m_nextUpdate == -1 )
	{
		found++;

		if ( found != m_preparedEvents.end() )
			m_nextUpdate = found.key() - 250; // in advance
	}
}
