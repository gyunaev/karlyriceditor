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

#include <QTextBlock>
#include <QDataStream>
#include <QMessageBox>
#include <QMouseEvent>
#include <QToolTip>
#include <QScrollBar>
#include <QStack>

#include "mainwindow.h"
#include "project.h"
#include "editor.h"
#include "editortimemark.h"
#include "settings.h"
#include "dialog_edittimemark.h"


Editor::Editor( QWidget * parent )
	: QTextEdit( parent )
{
	m_project = 0;
	m_timeId = 0;

	connect( this, SIGNAL( undoAvailable(bool)), pMainWindow, SLOT( editor_undoAvail(bool)));
	connect( this, SIGNAL( redoAvailable(bool)), pMainWindow, SLOT( editor_redoAvail(bool)));

	connect( this, SIGNAL(textChanged()), this, SLOT(textModified()) );

	QObject *timeMark = new EditorTimeMark;
	document()->documentLayout()->registerHandler( EditorTimeMark::TimeTextFormat, timeMark );

	setAcceptRichText( false );

	QFont font( pSettings->m_editorFontFamily, pSettings->m_editorFontSize );
	setFont( font );

	setMouseTracking( true );
}

void Editor::setProject( Project* proj )
{
	m_project = proj;
	m_lastAutosave.start();
}

static inline bool isCRLF( QChar ch )
{
	return (ch == QChar::LineSeparator) || (ch == QChar::ParagraphSeparator ) || (ch == '\n' );
}

void Editor::insertTimeTag( qint64 timing )
{
	// If we're replacing existing time tag, remove it first
	bool was_time_mark_deleted = false;

	QTextCursor cur = textCursor();
	cur.beginEditBlock();

	if ( cur.charFormat().objectType() == EditorTimeMark::TimeTextFormat
	&& timing > 0 // only when playing
	&& qVariantValue<qint64>( cur.charFormat().property( EditorTimeMark::TimeProperty ) ) == 0 ) // only placeholders
	{
		cur.deletePreviousChar();
		was_time_mark_deleted = true;
	}

	// Increase the current ID
	m_timeId++;

	QTextCharFormat timemark;
	timemark.setObjectType( EditorTimeMark::TimeTextFormat );

	// Since the EditorTimeMark object is not constructed directly, pass the values
	timemark.setProperty( EditorTimeMark::TimeProperty, timing );
	timemark.setProperty( EditorTimeMark::IdProperty, m_timeId );

	// Add the image
	cur.insertText( QString(QChar::ObjectReplacementCharacter), timemark );
	cur.endEditBlock();

	// Move the cursor according to policy
	if ( pSettings->m_editorDoubleTimeMark && was_time_mark_deleted )
		return; // the cursor has been moved already

	int curPos = cur.position();
	bool separator_found = false, tagged_word_ended = false;
	int word_start_offset = -1;

	while ( 1 )
	{
		// Find the block
		QTextBlock block = document()->findBlock( curPos );

		// Cursor position in the block
		int blockPos = curPos - block.position();
		QChar ch;

		// If we're out of range, this is the end of block.
		if ( blockPos >= block.text().size() )
		{
			// Text end?
			if ( !block.next().isValid() )
				break;

			// Tell the rest of the code this is end of line
			ch = QChar::LineSeparator;
		}
		else
			ch = block.text().at( blockPos );

		// Get the current character at pos
		//qDebug("char: %s (%d), pos %d, blockpos %d", qPrintable( QString(ch)), ch.unicode(), curPos, blockPos );

		// If QChar::ObjectReplacementCharacter - it's a time mark
		if ( ch == QChar::ObjectReplacementCharacter )
		{
			curPos++;
			break;
		}

		// We check for separator_found here because if the first character is timing mark, we want
		// it to be skipped too by a conditional check above
		if ( separator_found )
		{
			if ( pSettings->m_editorSkipEmptyLines && isCRLF( ch ) )
			{
				curPos++;
				continue;
			}

			break;
		}

		// New line is always a separator
		if ( isCRLF( ch ) )
		{
			// If this is not the first character, stop on previous one
			if ( cur.position() != curPos )
			{
				if ( pSettings->m_editorStopAtLineEnd )
					break;
			}

			separator_found = true;
		}

		// Timing mark is always before a word, so if we find a space, this means the word is ended,
		// and a new word is about to start. So if we have multiple tags per line enabled, check it.
		if ( pSettings->m_editorStopNextWord )
		{
			if ( ch.isSpace() )
			{
				// If word_start_offset is not -1, this means this is the second word which is ended
				if ( word_start_offset != -1 )
				{
					// Check the word length
					if ( curPos - word_start_offset > pSettings->m_editorWordChars )
					{
						// Word size is more than needed, and there was no time mark.
						// Roll the cursor back abd break
						curPos = word_start_offset;
						break;
					}
					else
					{
						// The word is too small. Reset the word_start_offset and continue
						word_start_offset = -1;
					}
				}
				else
					tagged_word_ended = true;
			}
			else
			{
				if ( tagged_word_ended && word_start_offset == -1 )
					word_start_offset = curPos;
			}
		}

		curPos++;
	}

	cur.setPosition( curPos, QTextCursor::MoveAnchor );
	setTextCursor( cur );
	ensureCursorMiddle();
}

void Editor::removeLastTimeTag()
{
	if ( m_timeId == 0 )
		return;

	// Iterate over the whole text until we find the last time tag
	for ( QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next() )
	{
		for ( QTextBlock::iterator it = block.begin(); it != block.end(); ++it )
		{
			QTextCharFormat fmt = it.fragment().charFormat();

			if ( fmt.objectType() == EditorTimeMark::TimeTextFormat )
			{
				unsigned int mark = qVariantValue<unsigned int>( fmt.property( EditorTimeMark::IdProperty ) );

				if ( mark == m_timeId )
				{
					QTextCursor cur = textCursor();
					cur.setPosition( it.fragment().position() );
					cur.deleteChar();
					setTextCursor( cur );
					ensureCursorVisible();
					m_timeId--;
					break;
				}
			}
		}
	}
}

void Editor::textModified()
{
	if ( !m_project )
		return;

	// Store the current text in settings if more than 15 seconds passed
	if ( m_lastAutosave.elapsed() > 15 )
	{
		m_lastAutosave.restart();
		QString lyrictext = exportToString();

		QSettings settings;
		settings.setValue( "editor/currentlyrics", lyrictext );
		settings.setValue( "editor/projectmusic", m_project->musicFile() );
		settings.setValue( "editor/lyricssize", lyrictext.length() );
	}

	m_project->setModified();
}

void Editor::removeAllTimeTags()
{
	QStack<int> positions;

	// Iterate over the whole text and store the positions of time tags
	// We cannot remove them here because it will break iterators!
	for ( QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next() )
	{
		for ( QTextBlock::iterator it = block.begin(); it != block.end(); ++it )
		{
			QTextCharFormat fmt = it.fragment().charFormat();

			if ( fmt.objectType() == EditorTimeMark::TimeTextFormat )
				positions.push( it.fragment().position() );
		}
	}

	// Now iterate over stack, and store those backward
	QTextCursor cur = textCursor();
	cur.beginEditBlock();

	while ( !positions.isEmpty() )
	{
		cur.setPosition( positions.pop() );
		cur.deleteChar();
	}

	cur.endEditBlock();
	m_timeId = 0;
}

QString	Editor::exportToString()
{
	QString outstr;

	for ( QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next() )
	{
		for ( QTextBlock::iterator it = block.begin(); ; ++it )
		{
			// Add an empty line at the end of block unless last block
			if ( it == block.end() )
			{
				if ( block.next().isValid() )
					outstr += QChar::LineSeparator;

				break;
			}

			QTextCharFormat fmt = it.fragment().charFormat();

			if ( fmt.objectType() == EditorTimeMark::TimeTextFormat )
				outstr += QString("<%1>").arg( qVariantValue<qint64>( fmt.property( EditorTimeMark::TimeProperty ) ) );
			else
			{
				QString saved = it.fragment().text();

				saved.replace( "&", "&amp;" );
				saved.replace( "<", "&lt;" );
				saved.replace( ">", "&gt;" );

				outstr += saved;
			}
		}
	}

	return outstr;
}

bool Editor::importFromString( const QString& strlyrics )
{
	QString lyricstr = strlyrics;
	clear();

	// If we have autosaved values, restore them instead
	QSettings settings;

	if ( settings.contains( "editor/currentlyrics" )
	&& settings.value( "editor/projectmusic" ).toString() == m_project->musicFile() )
	{
		int size = settings.value( "editor/lyricssize" ).toInt();

		if ( size == settings.value( "editor/currentlyrics" ).toString().length() )
		{
			qDebug("Restoring presaved lyrics from a project");
			lyricstr = settings.value( "editor/currentlyrics" ).toString();
		}
	}

	// A simple state machine
	bool timing = false;
	QString saved;

	textCursor().beginEditBlock();

	for ( int i = 0; ; ++i )
	{
		// Store the presaved text
		if ( i == lyricstr.length() || lyricstr[i] == '<' )
		{
			// There is no qt::unescape
			saved.replace( "&lt;", "<" );
			saved.replace( "&gt;", ">" );
			saved.replace( "&amp;", "&" );

			textCursor().insertText( saved );

			if ( i == lyricstr.length() )
			{
				textCursor().endEditBlock();
				return true;
			}

			saved.clear();
			timing = true;
		}
		else if ( lyricstr[i] == '>' )
		{
			QTextCharFormat timemark;
			timemark.setObjectType( EditorTimeMark::TimeTextFormat );

			// Since the EditorTimeMark object is not constructed directly, pass the values
			timemark.setProperty( EditorTimeMark::TimeProperty, (qint64) saved.toLongLong() );
			timemark.setProperty( EditorTimeMark::IdProperty, 0 );

			textCursor().insertText( QString(QChar::ObjectReplacementCharacter), timemark );

			saved.clear();
			timing = false;
		}
		else
			saved.push_back( lyricstr[i] );
	}

	textCursor().endEditBlock();
	return true;
}

Lyrics Editor::exportLyrics()
{
	Lyrics lyrics;

	if ( !validate() )
		return lyrics;

	lyrics.beginLyrics();

	for ( QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next() )
	{
		for ( QTextBlock::iterator it = block.begin(); ; ++it )
		{
			if ( it == block.end() )
			{
				lyrics.curLyricAddEndOfLine();
				break;
			}

			QTextCharFormat fmt = it.fragment().charFormat();

			if ( fmt.objectType() == EditorTimeMark::TimeTextFormat )
			{
				// Store old entry if any
				lyrics.curLyricAdd();

				lyrics.curLyricSetTime( qVariantValue<qint64>( fmt.property( EditorTimeMark::TimeProperty ) ) );
			}
			else
			{
				// Remove line feeds from the original line and append it
				QString text = it.fragment().text();
				text.remove( QChar::LineSeparator );

				lyrics.curLyricAppendText( text );

				// Now get the line back, and count removed CRLFs
				text = it.fragment().text();

				while ( !text.isEmpty() && text.endsWith( QChar::LineSeparator ) )
				{
					text.chop( 1 );
					lyrics.curLyricAddEndOfLine();
				}
			}
		}
	}

	lyrics.endLyrics();
	return lyrics;
}

void Editor::importLyrics( const Lyrics& lyrics )
{
	// clear the editor
	clear();

	textCursor().beginEditBlock();

	// Fill the editor
	for ( int bl = 0; bl < lyrics.totalBlocks(); bl++ )
	{
		const Lyrics::Block& block = lyrics.block( bl );

		for ( int ln = 0; ln < block.size(); ln++ )
		{
			const Lyrics::Line& line = block[ln];

			for ( int pos = 0; pos < line.size(); pos++ )
			{
				Lyrics::Syllable lentry = line[pos];

				// Insert timing mark
				QTextCharFormat timemark;
				timemark.setObjectType( EditorTimeMark::TimeTextFormat );
				timemark.setProperty( EditorTimeMark::TimeProperty, lentry.timing );
				timemark.setProperty( EditorTimeMark::PitchProperty, lentry.pitch );
				timemark.setProperty( EditorTimeMark::IdProperty, 0 );

				textCursor().insertText( QString(QChar::ObjectReplacementCharacter), timemark );
				textCursor().insertText( lentry.text );
			}

			textCursor().insertText( QString( QChar::LineSeparator ) );
		}

		textCursor().insertText( QString( QChar::LineSeparator ) );
	}

	textCursor().endEditBlock();
}

void Editor::cursorToLine( int line )
{
	QTextCursor cur = textCursor();
	cur.movePosition( QTextCursor::Start, QTextCursor::MoveAnchor );
	cur.movePosition( QTextCursor::Down, QTextCursor::MoveAnchor, line - 1 );
	setTextCursor( cur );
	ensureCursorVisible();
}

// validate:
bool Editor::validate()
{
	int linenumber = 1;
	int linesinblock = 0;
	qint64 last_time = 0;
	bool time_required = true;

	for ( QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next() )
	{
		for ( QTextBlock::iterator it = block.begin(); ; ++it )
		{
			if ( it == block.end() )
			{
				linenumber++;
				time_required = true;
				break;
			}

			QTextCharFormat fmt = it.fragment().charFormat();

			if ( fmt.objectType() == EditorTimeMark::TimeTextFormat )
			{
				qint64 mark = qVariantValue<qint64>( fmt.property( EditorTimeMark::TimeProperty ) );

				// 0 - placeholder
				if ( mark == 0 )
				{
					QMessageBox::critical( 0,
						 tr("Placeholder found"),
						 tr("Placeholder found at line %1\n"
							"Placeholders are not allowed in final lyrics"). arg(linenumber) );

					cursorToLine( linenumber - 1 );
					return false;
				}

				// Time going backward?
				if ( mark < last_time )
				{
					QMessageBox::critical( 0,
							 tr("Time goes backward"),
							 tr("Some time marks at line %1 are going backward.\n"
								"Time marks must increase toward the line end."). arg(linenumber) );

					cursorToLine( linenumber - 1 );
					return false;
				}

				last_time = mark;
				time_required = false;
			}
			else
			{
				QString text = it.fragment().text();

				if ( text.endsWith( QChar::LineSeparator ) )
				{
					int linefeeds = 0;

					while ( !text.isEmpty() && text.endsWith( QChar::LineSeparator ) )
					{
						text.chop( 1 );
						linefeeds++;

						if ( !pSettings->m_editorSupportBlocks && linefeeds > 1 )
						{
							QMessageBox::critical( 0,
								 tr("Empty line found"),
								 tr("Empty line found at line %1\n"
									"An empty line represents a block boundary, but blocks "
									"are currently disabled in settings"). arg(linenumber) );

							cursorToLine( linenumber - 1 );
							return false;
						}

						linenumber++;
					}

					// linefeeds == 1 - simple enter
					// linefeeds == 2 - block boundary
					// linefeeds > 2 - more than one empty line, error
					if ( linefeeds > 2 )
					{
						QMessageBox::critical( 0,
							 tr("Double empty line found"),
							 tr("Double empty line found at line %1\n"
								"A single empty line represents a block boundary; "
								"double lines are not supported."). arg(linenumber) );

						cursorToLine( linenumber - 1 );
						return false;
					}
					else if ( linefeeds == 1 )
					{
						linesinblock++;

						if ( pSettings->m_editorSupportBlocks && linesinblock > pSettings->m_editorMaxBlock )
						{
							QMessageBox::critical( 0,
								 tr("Block size exceeded"),
								 tr("The block contains more than %1 lines at line %2. Most karaoke players cannot "
									"show too large blocks because of limited screen space.\n\n"
									"Please split the block by adding a block separator (an empty line).\n")
									.arg(pSettings->m_editorMaxBlock) .arg(linenumber) );

							cursorToLine( linenumber - 1 );
							return false;
						}
					}
					else if ( linefeeds == 2 )
					{
						linesinblock = 0;
					}

					time_required = true;
					continue;
				}

				if ( time_required || text.indexOf( QChar::LineSeparator ) != -1 )
				{
					QMessageBox::critical( 0,
								 tr("Missing time tag"),
								 tr("Time tags is not present at first position at line %1"). arg(linenumber) );

					cursorToLine( linenumber - 1 );
					return false;
				}
			}
		}
	}

	return true;
}

QTextCursor Editor::timeMark( const QPoint& point )
{
	QAbstractTextDocumentLayout * layout = document()->documentLayout();
	int pos;

	// from QTextEditPrivate::mapToContents
	QPoint mapped = QPoint( point.x() + horizontalScrollBar()->value(), point.y() + verticalScrollBar()->value() );

	if ( layout && (pos = layout->hitTest( mapped, Qt::ExactHit )) != -1 )
	{
		QTextCursor cur = textCursor();
		cur.setPosition( pos + 1 );

		if ( cur.charFormat().objectType() == EditorTimeMark::TimeTextFormat )
			return cur;
	}

	return QTextCursor();
}

qint64 Editor::timeMarkValue( const QPoint& point )
{
	QTextCursor cur = timeMark( point );

	if ( !cur.isNull() )
		return qVariantValue<qint64>( cur.charFormat().property( EditorTimeMark::TimeProperty ) );
	else
		return -1;
}

void Editor::mouseMoveEvent( QMouseEvent * event )
{
	if ( timeMarkValue( event->pos() ) != -1 )
		viewport()->setCursor( Qt::PointingHandCursor );
	else
		viewport()->setCursor( Qt::IBeamCursor );
}

bool Editor::event ( QEvent * event )
{
	if ( event->type() == QEvent::ToolTip )
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
		qint64 mark = timeMarkValue( helpEvent->pos() );

		if ( mark == 0 )
			QToolTip::showText( helpEvent->globalPos(), tr("Placeholder") );
		else if ( mark > 0 )
			QToolTip::showText( helpEvent->globalPos(), tr("Time mark: %1 ms") .arg( mark ) );

		return true;
	}

	return QTextEdit::event( event );
}

void Editor::mouseReleaseEvent ( QMouseEvent * event )
{
	if ( event->button() == Qt::LeftButton )
	{
		QTextCursor cur = timeMark( event->pos() );

		if ( !cur.isNull() )
		{
			qint64 mark = qVariantValue<qint64>( cur.charFormat().property( EditorTimeMark::TimeProperty ) );
			int pitch = qVariantValue<qint64>( cur.charFormat().property( EditorTimeMark::PitchProperty ) );

			DialogEditTimeMark dlg( m_project->type() == Project::LyricType_UStar, this );
			dlg.move( event->globalPos() );
			dlg.setTimemark( mark );
			dlg.setPitch( pitch );

			if ( dlg.exec() == QDialog::Accepted )
			{
				QTextCharFormat timemark;
				timemark.setObjectType( EditorTimeMark::TimeTextFormat );
				timemark.setProperty( EditorTimeMark::TimeProperty, dlg.timemark() );
				timemark.setProperty( EditorTimeMark::PitchProperty, dlg.pitch() );
				timemark.setProperty( EditorTimeMark::IdProperty, cur.charFormat().property( EditorTimeMark::IdProperty ).toInt() );

				cur.beginEditBlock();
				cur.deletePreviousChar();
				cur.insertText( QString(QChar::ObjectReplacementCharacter), timemark );
				cur.endEditBlock();
			}
		}
	}

	QTextEdit::mouseReleaseEvent ( event );
}

void Editor::cleanupAutoSave()
{
	QSettings settings;
	settings.remove( "editor/currentlyrics" );
	settings.remove( "editor/projectmusic" );
	settings.remove( "editor/lyricssize" );
}


void Editor::ensureCursorMiddle()
{
	// Adjust for non-common cases and horizontally
	ensureCursorVisible();

	// Now adjust vertically
	QScrollBar * vbar = verticalScrollBar();
	QRect crect = cursorRect( textCursor() );
	const int halfHeight = viewport()->height() / 2;
	const int curBottom = crect.y() + crect.height() + vbar->value();

	if ( curBottom > vbar->value() + halfHeight )
		vbar->setValue( qMax( 0, curBottom - halfHeight ) );
}
