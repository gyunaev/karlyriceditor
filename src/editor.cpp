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

#include <QTextBlock>
#include <QDataStream>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTextDocumentFragment>
#include <QMimeData>
#include <QToolTip>
#include <QScrollBar>
#include <QStack>

#include "mainwindow.h"
#include "project.h"
#include "editor.h"
#include "settings.h"
#include "editorhighlighting.h"
#include "lyricsevents.h"
#include "textrenderer.h"
#include "cdg.h"


const char * Editor::PLACEHOLDER = "[--:--]";
const char * Editor::PLACEHOLDER_VALUE = "--:--";

Editor::Editor( QWidget * parent )
	: QTextEdit( parent )
{
	m_project = 0;
	m_timeId = 0;

	connect( this, SIGNAL( undoAvailable(bool)), pMainWindow, SLOT( editor_undoAvail(bool)));
	connect( this, SIGNAL( redoAvailable(bool)), pMainWindow, SLOT( editor_redoAvail(bool)));

	connect( this, SIGNAL(textChanged()), this, SLOT(textModified()) );

	setAcceptRichText( false );

	QFont font( pSettings->m_editorFontFamily, pSettings->m_editorFontSize );
	setFont( font );

	(void) new EditorHighlighting( this );
}

void Editor::setProject( Project* proj )
{
	m_project = proj;
}

static inline bool isCRLF( QChar ch )
{
	return (ch == QChar::LineSeparator) || (ch == QChar::ParagraphSeparator ) || (ch == '\n' );
}

void Editor::textModified()
{
	if ( !m_project )
		return;

	m_project->setModified();
}

QString	Editor::exportToString()
{
	return toPlainText();
}

bool Editor::importFromString( const QString& lyricstr )
{
	setPlainText( lyricstr );
	return true;
}

bool Editor::importFromOldString( const QString& lyricstr )
{
	QString strlyrics;

	clear();
	setEnabled( true );

	// A simple state machine
	bool timing = false;
	QString saved;

	for ( int i = 0; ; ++i )
	{
		// Store the presaved text
		if ( i == lyricstr.length() || lyricstr[i] == '<' )
		{
			// There is no qt::unescape
			saved.replace( "&lt;", "<" );
			saved.replace( "&gt;", ">" );
			saved.replace( "&amp;", "&" );

			strlyrics += saved;

			if ( i == lyricstr.length() )
				break;

			saved.clear();
			timing = true;
		}
		else if ( lyricstr[i] == '>' )
		{
			QString time;

			if ( saved.contains( '|' ) )
			{
				QStringList values = saved.split( '|' );
				time = "[" + markToTime( values[0].toLongLong() ) + "]";
			}
			else
				time = "[" + markToTime( saved.toLongLong() ) + "]";

			strlyrics += time;
			saved.clear();
			timing = false;
		}
		else
			saved.push_back( lyricstr[i] );
	}

	setPlainText( strlyrics );
	return true;
}

bool Editor::exportLyrics( Lyrics * lyrics )
{
	if ( !validate() )
		return false;

	QString text = toPlainText();
	QStringList lines = text.split( '\n' );

	lyrics->beginLyrics();

	foreach( QString line, lines )
	{
		if ( line.trimmed().isEmpty() )
		{
			// end of paragraph
			lyrics->curLyricAddEndOfLine();
			continue;
		}

		QString lyrictext, timing, special;
		bool in_time_tag = false;
		bool in_special_tag = false;
		int added_lyrics = 0;

		for ( int col = 0; col < line.size(); col++ )
		{
			if ( in_special_tag )
			{
				// Special tag ends?
				if ( line[col] == '}' )
					in_special_tag = false;
				else
					special.append( line[col] );
			}
			else if ( in_time_tag )
			{
				// Time tag ends?
				if ( line[col] == ']' )
					in_time_tag = false;
				else
					timing.append( line[col] );
			}
			else if ( line[col] == '{' )
			{
				in_special_tag = true;
			}
			else if ( line[col] == '[' )
			{
				// If we already have the text, add the lyric
				if ( !timing.isEmpty() )
				{
					if ( !special.isEmpty() )
						lyrics->addBackgroundEvent( timeToMark( timing ), special );

					// The first lyric should not be empty
					if ( added_lyrics > 0 || !lyrictext.isEmpty() )
					{
						lyrics->curLyricSetTime( timeToMark( timing ) );
						lyrics->curLyricAppendText( lyrictext );
						lyrics->curLyricAdd();
						added_lyrics++;
					}

					lyrictext.clear();
					special.clear();
					timing.clear();
				}

				in_time_tag = true;
			}
			else
				lyrictext.append( line[col] );
		}

		// There may be lyrics at the end of line
		if ( !timing.isEmpty() )
		{
			if ( !special.isEmpty() )
				lyrics->addBackgroundEvent( timeToMark( timing ), special );

			lyrics->curLyricSetTime( timeToMark( timing ) );
			lyrics->curLyricAppendText( lyrictext );
			lyrics->curLyricAdd();
		}

		lyrics->curLyricAddEndOfLine();
	}

	lyrics->endLyrics();
	return true;
}

void Editor::importLyrics( const Lyrics& lyrics )
{
	// clear the editor
	clear();
	setEnabled( true );

	QString strlyrics;

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

				strlyrics += "[" + markToTime( lentry.timing ) + "]" + lentry.text;
			}

			strlyrics += "\n";
		}

		strlyrics += "\n";
	}

	setPlainText( strlyrics );
}

void Editor::cursorToLine( int line, int column )
{
	QTextCursor cur = textCursor();
	cur.movePosition( QTextCursor::Start, QTextCursor::MoveAnchor );
	cur.movePosition( QTextCursor::Down, QTextCursor::MoveAnchor, line - 1 );

	if ( column )
	{
		cur.movePosition( QTextCursor::Left, QTextCursor::MoveAnchor );
		cur.movePosition( QTextCursor::Right, QTextCursor::MoveAnchor, column );
	}

	setTextCursor( cur );
	ensureCursorVisible();
}

// validators
bool Editor::validate( const QFont * font, const QSize * fitsize )
{
	QList<ValidatorError> errors;
	validate( errors, font, fitsize );

	if ( !errors.isEmpty() )
	{
		QMessageBox::critical( 0,
							   tr("Validation error found"),
								  tr("Error at line %1: ").arg( errors.front().line )
								  + errors.front().error );

		cursorToLine( errors.front().line, errors.front().column );
		return false;
	}

	return true;
}


void Editor::validate( QList<ValidatorError>& errors, const QFont * font, const QSize * fitsize )
{
	int linesinblock = 0;
	qint64 last_time = 0;
	QString paragraphtext;

	// Get the lyrics
	QString text = toPlainText();
	QStringList lines = text.split( '\n' );

	for ( int linenumber = 1; linenumber <= lines.size(); linenumber++ )
	{
		const QString& line = lines[ linenumber - 1];

		// Empty line is a paragraph separator. Handle it.
		if ( line.trimmed().isEmpty() )
		{
			// Is it enabled?
			if ( !pSettings->m_editorSupportBlocks )
			{
				errors.push_back(
						ValidatorError(
								linenumber,
								0,
								tr("Empty line found.\n"
								   "An empty line represents a block boundary, but blocks "
								   "are currently disabled in settings") ) );

				// recoverable
				goto cont_paragraph;
			}

			// Repeat?
			if ( paragraphtext.isEmpty() )
			{
				// A new paragraph already started; duplicate empty line, not allowed
				errors.push_back(
						ValidatorError(
								linenumber,
								0,
								tr("Double empty line found.\n"
									"A single empty line represents a block boundary; "
								"double lines are not supported.") ) );

				// recoverable
				goto cont_paragraph;
			}

			// Paragraph-specific checks
			if ( font && fitsize )
			{
				// Check if we exceed the screen height
				QFont nfont = *font;

				if ( !TextRenderer::checkFit( *fitsize, nfont, paragraphtext ) )
				{
					errors.push_back(
							ValidatorError(
									linenumber - 1,
									0,
									tr("Paragraph height exceed.\nThis paragraph cannot fit into the screen using the selected font" ) ) );
				}
			}

cont_paragraph:
			linesinblock = 0;
			paragraphtext = "";
			continue;
		}

		// If we're here, this is not an empty line.
		linesinblock++;

		// Check if we're out of block line limit
		if ( pSettings->m_editorSupportBlocks && linesinblock > pSettings->m_editorMaxBlock )
		{
			errors.push_back(
					ValidatorError(
							linenumber,
							0,
							tr("Block size exceeded. The block contains more than %1 lines.\n"
								"Most karaoke players cannot show too large blocks because of "
								"limited screen space.\n\nPlease split the block by adding a "
								"block separator (an empty line).\n") .arg(pSettings->m_editorMaxBlock) ) );
		}

		// Should not have lyrics before the first [
		if ( line[0] != '[' )
		{
			errors.push_back(
					ValidatorError(
							linenumber,
							0,
							tr("Missing opening time tag. Every line must start with a [mm:ss.ms] time tag") ) );
		}

		// LRCv2, UStar and CD+G must also end with ]
		if ( m_project->type() != Project::LyricType_LRC1 && !line.trimmed().endsWith( ']' ) )
		{
			errors.push_back(
					ValidatorError(
							linenumber,
							0,
							tr("Missing closing time tag. For this lyrics type every line must end with a [mm:ss.ms] time tag") ) );
		}

		// Go through the line, and verify all time tags
		int time_tag_start = 0;
		bool in_time_tag = false;
		int special_tag_start = 0;
		bool in_special_tag = false;
		QString linetext;

		for ( int col = 0; col < line.size(); col++ )
		{
			if ( in_special_tag )
			{
				// Special tag ends?
				if ( line[col] == '}' )
				{
					// Get the time and special part, if any
					QString special = line.mid( special_tag_start, col - special_tag_start );

					if ( !special.isEmpty() )
					{
						QString errmsg = LyricsEvents::validateEvent( special );

						if ( !errmsg.isEmpty() )
						{
							errors.push_back(
									ValidatorError(
											linenumber,
											time_tag_start,
											tr("Invalid special tag. %1") .arg( errmsg ) ) );
						}
					}

					in_special_tag = false;
					continue;
				}
			}
			else if ( in_time_tag )
			{
				// Time tag ends?
				if ( line[col] == ']' )
				{
					// Get the time and special part, if any
					QString time = line.mid( time_tag_start, col - time_tag_start );

					// Now validate the time
					if ( time == PLACEHOLDER_VALUE )
					{
						errors.push_back(
								ValidatorError(
										linenumber,
										time_tag_start,
										tr("Placeholders should not be present in the production file.") ) );
					}
					else
					{
						QRegExp rxtime( "^(\\d+):(\\d+)\\.(\\d+)$" );

						if ( time.indexOf( rxtime ) != -1 )
						{
							if ( rxtime.cap( 2 ).toInt() >= 60 )
							{
								errors.push_back(
										ValidatorError(
												linenumber,
												time_tag_start,
												tr("Invalid time, number of seconds cannot exceed 59.") ) );
							}

							qint64 timing = timeToMark( time );

							if ( timing < last_time )
							{
								errors.push_back(
										ValidatorError(
												linenumber,
												time_tag_start,
												tr("Time goes backward, previous time value is greater than current value.") ) );
							}

							last_time = timing;
						}
						else
						{
							errors.push_back(
									ValidatorError(
											linenumber,
											time_tag_start,
											tr("Invalid time tag. Time tag must be in format [mm:ss.ms] where mm is minutes, ss is seconds and ms is milliseconds * 10") ) );
						}
					}

					in_time_tag = false;
					continue;
				}

				// Only accept those characters
				if ( !line[col].isDigit() && line[col] != ':' && line[col] != '.' )
				{
					errors.push_back(
							ValidatorError(
									linenumber,
									col,
									tr("Invalid character in the time tag. Time tag must be in format [mm:ss.ms] where mm is minutes, ss is seconds and ms is milliseconds * 10") ) );

					in_time_tag = false;
					break; // done with this line
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
			else if ( line[col] == ']' )
			{
				errors.push_back(
						ValidatorError(
								linenumber,
								col,
								tr("Invalid closing bracket usage outside the time block") ) );
			}
			else if ( line[col] == '}' )
			{
				errors.push_back(
						ValidatorError(
								linenumber,
								col,
								tr("Invalid closing bracket usage outside the special block") ) );
			}
			else
				linetext += line[col];
		}

		// Check if we exceed the screen width
		if ( font && fitsize )
		{
			// Check if we exceed the screen height
			QFont nfont = *font;

			if ( !TextRenderer::checkFit( *fitsize, nfont, paragraphtext ) )
			{
				errors.push_back(
						ValidatorError(
								linenumber,
								0,
								tr("Line width exceed.\nThis line cannot fit into the screen using the selected font" ) ) );
			}
		}

		paragraphtext += linetext + "\n";

		// Verify opened time block
		if ( in_time_tag )
		{
			errors.push_back(
					ValidatorError(
							linenumber,
							line.size() - 1,
							tr("Time tag is not closed properly") ) );
		}
	}
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


bool Editor::canInsertFromMimeData ( const QMimeData * source ) const
{
	return source->hasText() && !source->text().isEmpty();
}

QMimeData * Editor::createMimeDataFromSelection () const
{
	const QTextDocumentFragment fragment( textCursor() );
	QString text = fragment.toPlainText();

	QMimeData * m = new QMimeData();
	m->setText( text );

	return m;
}

void Editor::insertFromMimeData ( const QMimeData * source )
{
	QString text = source->text();

	text.replace( QChar::LineSeparator, "\n" );
	text.replace( QChar::ParagraphSeparator, "\n" );
	text.remove( "\r" );

	if ( !text.isNull() )
	{
		QTextDocumentFragment fragment = QTextDocumentFragment::fromPlainText( text );
		textCursor().insertFragment( fragment );
		ensureCursorVisible();
	}
}

void Editor::removeAllTimeTags()
{
	// No need to do it in a more complex way than a simple regexp
	QString text = toPlainText();

	// Placeholders
	text.remove( PLACEHOLDER );
	text.remove( QRegExp( "\\[\\d+:\\d+\\.\\d+\\]" ) );

	setPlainText( text );
}

void Editor::removeExtraWhitespace()
{
	// No need to do it in a more complex way than a simple regexp
	QStringList lyrics = toPlainText().split( '\n' );

	for ( int i = 0; i < lyrics.size(); i++ )
	{
		lyrics[i] = lyrics[i].trimmed();
	}

	setPlainText( lyrics.join( "\n") );
}

static bool isTimingMark( const QString& text, int * length = 0 )
{
	if ( text.startsWith( Editor::PLACEHOLDER ) )
	{
		if ( length )
			*length = strlen( Editor::PLACEHOLDER );

		return true;
	}

	QRegExp rx( "^\\[\\d+:\\d+\\.\\d+\\]" );

	if ( text.indexOf( rx ) != -1 )
	{
		if ( length )
			*length = rx.matchedLength();

		return true;
	}

	return false;
}


void Editor::insertTimeTag( qint64 timing )
{
	// If we're replacing existing time tag, remove it first
	bool was_time_mark_deleted = false;

	QTextCursor cur = textCursor();
	cur.beginEditBlock();

	QString text = cur.block().text().mid( cur.position() - cur.block().position() );
	int length;

	if ( timing > 0 && isTimingMark( text, &length ) )// only when playing
	{
		if ( text.startsWith( Editor::PLACEHOLDER ) )
			was_time_mark_deleted = true;

		while ( length-- )
			cur.deleteChar();

		text = cur.block().text().mid( cur.position() - cur.block().position() );
	}

	// Add the time
	if ( timing == 0 )
		cur.insertText( PLACEHOLDER );
	else
		cur.insertText( "[" + markToTime( timing ) + "]" );

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

		//qDebug("char: %s (%d), pos %d, blockpos %d", qPrintable( QString(ch)), ch.unicode(), curPos, blockPos );

		// Time mark?
		if ( isTimingMark( block.text().mid( blockPos ) ) )
			break;

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
	undo();
}

QTextCursor Editor::cursorAtPoint( const QPoint& point )
{
	QAbstractTextDocumentLayout * layout = document()->documentLayout();
	int pos;

	// Adjust for non-common cases and horizontally
	ensureCursorVisible();

	// from QTextEditPrivate::mapToContents
	QPoint mapped = QPoint( point.x() + horizontalScrollBar()->value(), point.y() + verticalScrollBar()->value() );

	if ( layout && (pos = layout->hitTest( mapped, Qt::ExactHit )) != -1 )
	{
		QTextCursor cur = textCursor();
		cur.setPosition( pos + 1 );

		return cur;
	}

	return QTextCursor();
}

qint64 Editor::timeForPosition( QTextCursor cur )
{
	QString line = cur.block().text();
	int pos = cur.position() - cur.block().position();

	QString left = line.left( pos );
	QString right = line.mid( pos );
	QRegExp markrx ( "\\[(\\d+:\\d+\\.\\d+)\\]" );

	// Search left
	int offset = left.lastIndexOf( markrx );

	if ( offset == -1 )
		return -1;

	qint64 leftmark = timeToMark( markrx.cap( 1 ) );
	left = left.mid( offset + markrx.matchedLength() );
	left.remove( markrx );

	// Search right
	offset = right.indexOf( markrx );

	if ( offset == -1 )
		return -1;

	qint64 rightmark = timeToMark( markrx.cap( 1 ) );
	right = right.left( offset );
	right.remove( markrx );

	// We have time rightmark-leftmark for (left+right) characters
	int timediff = (int) (rightmark - leftmark);

	if ( timediff <= 0 )
		return -1;

	qint64 timing = leftmark + left.length() * timediff / (left.length() + right.length() );
	return timing;
}

void Editor::splitLine()
{
	QTextCursor cur = textCursor();

	qint64 timing = timeForPosition( cur );

	if ( timing == -1 )
		return;

	cur.beginEditBlock();
	cur.insertText( "[" + markToTime( timing ) + "]" );
	cur.insertText( "\n" );
	cur.insertText( "[" + markToTime( timing + 10 ) + "]" );
	cur.endEditBlock();
}

bool Editor::event ( QEvent * event )
{
	if ( event->type() == QEvent::ToolTip )
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
		QTextCursor cur = cursorAtPoint( helpEvent->pos() );

		if ( !cur.isNull() )
		{
			qint64 mark = timeForPosition( cur );

			if ( mark != -1 )
			{
				QString text = tr("Timing at this point: %1") .arg( markToTime(mark) );

				QToolTip::showText( helpEvent->globalPos(), text );
				return true;
			}
		}
	}

	return QTextEdit::event( event );
}

void Editor::insertImageTag( const QString& file )
{
	QTextCursor cur = textCursor();
	cur.beginEditBlock();
	cur.insertText( "{ IMAGE=" + file + " }" );
	cur.endEditBlock();
}

void Editor::insertVideoTag( const QString& file )
{
	QTextCursor cur = textCursor();
	cur.beginEditBlock();
	cur.insertText( "{ VIDEO=" + file + " }" );
	cur.endEditBlock();
}

void Editor::insertColorChangeTag( const QString& name )
{
	QTextCursor cur = textCursor();
	cur.beginEditBlock();
	cur.insertText( "@@" + name );
	cur.endEditBlock();
}
