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

#include "editorhighlighting.h"
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

	m_hlPlaceholder.setBackground( pSettings->m_timeMarkPlaceholderBackground );
	m_hlPlaceholder.setForeground( pSettings->m_timeMarkPlaceholderText );
}

void EditorHighlighting::highlightBlock ( const QString & line )
{
	if ( line.trimmed().isEmpty() )
		return;

	// from Editor::validate
	int time_tag_start = 0;
	bool in_time_tag = false;
	bool errors_in_time_tag = false;

	for ( int col = 0; col < line.size(); col++ )
	{
		if ( in_time_tag )
		{
			// Time tag ends?
			if ( line[col] == ']' )
			{
				int count = col - time_tag_start + 1;

				if ( !errors_in_time_tag )
				{
					QString time = line.mid( time_tag_start, count );

					if ( time == Editor::PLACEHOLDER )
						setFormat( time_tag_start, count, m_hlPlaceholder );
					else
					{
						QRegExp rxtime( "^\\[(\\d+):(\\d+)\\.(\\d+)\\]$" );

						if ( time.indexOf( rxtime ) == -1 || rxtime.cap( 2 ).toInt() >= 60 )
							setFormat( time_tag_start, count, m_hlInvalidTiming );
						else
							setFormat( time_tag_start, count, m_hlValidTiming );
					}
				}

				in_time_tag = false;
				errors_in_time_tag = false;
				continue;
			}

			// Only accept those characters
			if ( !line[col].isDigit() && line[col] != ':' && line[col] != '.' )
			{
				setFormat( col, 1, m_hlInvalidTiming );
				errors_in_time_tag = true;
			}
		}
		else if ( line[col] == '[' )
		{
			in_time_tag = true;
			time_tag_start = col;
			continue;
		}
		else if ( line[col] == ']' )
			setFormat( col, 1, m_hlInvalidTiming );
	}
}
