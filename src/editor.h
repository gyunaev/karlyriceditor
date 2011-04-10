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

#ifndef EDITOR_H
#define EDITOR_H

#include <QTextEdit>
#include <QImage>
#include <QTime>
#include <QTextObjectInterface>

#include "lyrics.h"
#include "validator.h"

class Project;
class BackgroundEvents;


static inline QString markToTime( qint64 mark )
{
	int min = mark / 60000;
	int sec = (mark - min * 60000) / 1000;
	int msec = mark - (min * 60000 + sec * 1000 );

	return QString().sprintf( "%02d:%02d.%02d", min, sec, msec / 10 );
}

static inline qint64 timeToMark( QString data )
{
	QRegExp rxtime( "^(\\d+):(\\d+)\\.(\\d+)$");

	if ( data.indexOf( rxtime ) == -1 )
		return -1;

	return rxtime.cap( 1 ).toInt() * 60000 + rxtime.cap( 2 ).toInt() * 1000 + rxtime.cap( 3 ).toInt() * 10;
}


//
// Architecturally only editor is responsible for formats, and know everything about
// any specific lyrics format. This is done here and not in Project to provide syntax
// highlighting.
//
class Editor : public QTextEdit
{
	Q_OBJECT

	public:
		static const char * PLACEHOLDER;
		static const char * PLACEHOLDER_VALUE;

		Editor( QWidget * parent );

		void	setProject( Project* proj );

		void	insertTimeTag( qint64 timing );
		void	removeLastTimeTag();
		void	removeAllTimeTags();
		void	removeExtraWhitespace();

		// Validate the lyrics
		bool	validate( const QFont * font = 0, const QSize * fitsize = 0 );
		void	validate( QList<ValidatorError>& errors, const QFont * font = 0, const QSize * fitsize = 0 );

		// Export/Import functions to process lyrics. This generally does not work
		// for lyrics in "editing" phase, and requires the lyrics to be validated.
		bool	exportLyrics( Lyrics * lyrics );
		void	importLyrics( const Lyrics& lyrics );

		// Export/Import functions to store/load lyrics from a project file or
		// temporary storage. Not really useful for anything else.
		QString	exportToString();
		bool	importFromString( const QString& lyricstr );
		bool	importFromOldString( const QString& lyricstr );

	public slots:
		void	textModified();
		void	splitLine();

	protected:
		bool canInsertFromMimeData ( const QMimeData * source ) const;
		QMimeData * createMimeDataFromSelection () const;
		void insertFromMimeData ( const QMimeData * source );
		bool event ( QEvent * event );

	private:
		QString		validateSpecial( const QString& string );
		QTextCursor cursorAtPoint( const QPoint& point );
		qint64		timeForPosition( QTextCursor cur );

		// Ensure the cursor is in the middle of the screen if possible
		void		ensureCursorMiddle();

		void	cursorToLine( int line, int column );

		Project		 *	m_project;
		unsigned int	m_timeId; // for remove tag
};

#endif // EDITOR_H
