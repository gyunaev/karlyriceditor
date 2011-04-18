/**************************************************************************
 *  Karlyriceditor - a lyrics editor and CD+G / video export for Karaoke  *
 *  songs.                                                                *
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

#include "editorhighlighting.h"
#include "lyricsevents.h"
#include "settings.h"
#include "editor.h"

EditorHighlighting::EditorHighlighting( QTextEdit *parent )
	: QSyntaxHighlighter( parent )
{
	updateSettings();
	// TODO: configuration change
}

void EditorHighlighting::updateSettings()
{
	m_hlValidTiming.setBackground( pSettings->m_timeMarkTimeBackground );
	m_hlValidTiming.setForeground( pSettings->m_timeMarkTimeText );

	m_hlInvalidTiming.setBackground( Qt::red );
	m_hlInvalidTiming.setForeground( Qt::black );

	m_hlValidSpecial.setBackground( Qt::green );
	m_hlValidSpecial.setForeground( Qt::gray );

	m_hlPlaceholder.setBackground( pSettings->m_timeMarkPlaceholderBackground );
	m_hlPlaceholder.setForeground( pSettings->m_timeMarkPlaceholderText );
}

void EditorHighlighting::highlightBlock ( const QString & line )
{
	if ( line.trimmed().isEmpty() )
		return;

	// from Editor::validate
	int time_tag_start = 0;
	int special_tag_start = 0;
	bool in_time_tag = false;
	bool in_special_tag = false;
	bool errors_in_time_tag = false;
	bool errors_in_special_tag = false;

	for ( int col = 0; col < line.size(); col++ )
	{
		if ( in_special_tag )
		{
			// Special tag ends?
			if ( line[col] == '}' )
			{
				if ( !errors_in_special_tag )
				{
					QString special = line.mid( special_tag_start, col - special_tag_start );

					if ( !special.isEmpty() )
					{
						QString errmsg = LyricsEvents::validateEvent( special );

						if ( !errmsg.isEmpty() )
							setFormat( special_tag_start - 1, special.length() + 2, m_hlInvalidTiming );
						else
							setFormat( special_tag_start - 1, special.length() + 2, m_hlValidSpecial );
					}
					else
						setFormat( special_tag_start - 1, 2, m_hlInvalidTiming );
				}

				in_special_tag = false;
				errors_in_special_tag = false;
				continue;
			}
		}
		else if ( in_time_tag )
		{
			// Time tag ends?
			if ( line[col] == ']' )
			{
				if ( !errors_in_time_tag )
				{
					QString time = line.mid( time_tag_start, col - time_tag_start );

					// If Special is not empty, it hovers the last bracket (extra char)
					int time_len = time.length() + 2;

					if ( time == Editor::PLACEHOLDER_VALUE )
						setFormat( time_tag_start - 1, time_len, m_hlPlaceholder );
					else
					{
						QRegExp rxtime( "^(\\d+):(\\d+)\\.(\\d+)$" );

						if ( time.indexOf( rxtime ) == -1 || rxtime.cap( 2 ).toInt() >= 60 )
							setFormat( time_tag_start - 1, time_len, m_hlInvalidTiming );
						else
							setFormat( time_tag_start - 1, time_len, m_hlValidTiming );
					}
				}

				in_time_tag = false;
				errors_in_time_tag = false;
				continue;
			}

			// Only accept those characters; --:-- is placeholder so also valid
			if ( !line[col].isDigit() && line[col] != ':' && line[col] != '.' && line[col] != '-' )
			{
				setFormat( col, 1, m_hlInvalidTiming );
				errors_in_time_tag = true;
			}
		}
		else if ( line[col] == '[' )
		{
			in_time_tag = true;
			time_tag_start = col + 1;
			continue;
		}
		else if ( line[col] == '{' )
		{
			in_special_tag = true;
			special_tag_start = col + 1;
			continue;
		}
		else if ( line[col] == ']' || line[col] == '}' )
			setFormat( col, 1, m_hlInvalidTiming );
	}
}
