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

#ifndef EDITOR_H
#define EDITOR_H

#include <QTextEdit>
#include <QImage>
#include <QTime>
#include <QTextObjectInterface>

#include "lyrics.h"
#include "validator.h"

class Project;

//
// Architecturally only editor is responsible for formats, and know everything about
// any specific lyrics format. This is done here and not in Project to provide syntax
// highlighting.
//

class Editor : public QTextEdit
{
	Q_OBJECT

	public:
		Editor( QWidget * parent );

		void	setProject( Project* proj );

		void	insertTimeTag( qint64 timing );
		void	removeLastTimeTag();
		void	removeAllTimeTags();

		// Validate the lyrics
		bool	validate();
		void	validate( QList<ValidatorError>& errors );

		// Export/Import functions to process lyrics. This generally does not work
		// for lyrics in "editing" phase, and requires the lyrics to be validated.
		Lyrics	exportLyrics();
		void importLyrics( const Lyrics& lyrics );

		// Export/Import functions to store/load lyrics from a project file or
		// temporary storage. Not really useful for anything else.
		QString	exportToString();
		bool	importFromString( const QString& lyricstr );
		bool	importFromOldString( const QString& lyricstr );

	public slots:
		void	textModified();
		void	pianoRollClicked( unsigned int tone ); // from piano roll

	protected:
		void mouseReleaseEvent( QMouseEvent * event );
		void mouseMoveEvent( QMouseEvent * event );
		bool event ( QEvent * event ); // tooltips
		bool canInsertFromMimeData ( const QMimeData * source ) const;
		QMimeData * createMimeDataFromSelection () const;
		void insertFromMimeData ( const QMimeData * source );

	private:
		// Returns time mark textchar under cursor. Returns -1 if not on time mark
		qint64		timeMarkValue( const QPoint& point );
		QTextCursor timeMark( const QPoint& point );

		// Ensure the cursor is in the middle of the screen if possible
		void		ensureCursorMiddle();

		void	cursorToLine( int line );

		Project		 *	m_project;
		unsigned int	m_timeId; // for remove tag
};

#endif // EDITOR_H
