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

#include <QPainter>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QToolTip>

#include "mainwindow.h"
#include "pianorollwidget.h"


// Some drawing constants
static const int WHITEKEY_H2W_RATIO_PERC = 20; // White key width is 20% of height
static const int BLACKKEY_WIDTH_RATIO_PERC = 60; // Black key width is 60% of white key width
static const int WHITEKEY_BLACKKEY_HEIGHT_RATIO = 70; // Black key height is 60% of white key heigth

static const int SPACE_BETWEEN_KEYS = 1; // pixels
static const int MINIMUM_WHITEKEY_WIDTH = 20; // pixels; does not include space
static const int MINIMUM_KEY_WIDTH = MINIMUM_WHITEKEY_WIDTH + 2 * SPACE_BETWEEN_KEYS;
static const int MINIMUM_WHITE_KEYS_SHOWN = 7; // one octave


PianoRollDock::PianoRollDock( QWidget * parent )
	: QDockWidget( tr("Piano roll"), parent )
{
	PianoRollWidget * w = new PianoRollWidget( this );
	setWidget( w );

	// Connect piano roll signals
	connect( w, SIGNAL(noteMouseOver(unsigned int)), pMainWindow, SLOT(noteMouseOver(unsigned int) ) );
	connect( w, SIGNAL(noteClicked(unsigned int)), pMainWindow, SLOT(noteClicked(unsigned int)) );
}


PianoRollWidget::PianoRollWidget( QWidget * parent, int showOctaves )
	: QWidget( parent )
{
	m_zoomFactor = 1.0;
	m_maxWhiteKeysShown = 7 * showOctaves;
	m_shownKeys = 0;
	m_whiteKeyMouseOver = -1;
	m_blackKeyMouseOver = -1;
	m_noteDown = false;
	m_pitchMouseOver = -1;

	// Enable mouseMove event
	setMouseTracking( true );
}

QSize PianoRollWidget::minimumSizeHint() const
{
	int height = (100 * MINIMUM_WHITEKEY_WIDTH) / WHITEKEY_H2W_RATIO_PERC;
	int width = MINIMUM_KEY_WIDTH * MINIMUM_WHITE_KEYS_SHOWN;

	return QSize( width, height );
}

QSize PianoRollWidget::sizeHint() const
{
	int height = (100 * MINIMUM_WHITEKEY_WIDTH) / WHITEKEY_H2W_RATIO_PERC;
	int width = MINIMUM_KEY_WIDTH * m_maxWhiteKeysShown;

	return QSize( width, height );
}

int PianoRollWidget::heightForWidth( int w )
{
	int height = (100 * MINIMUM_WHITEKEY_WIDTH) / WHITEKEY_H2W_RATIO_PERC;
	int keys_fit = w / MINIMUM_KEY_WIDTH;

	// Can we fit all keys?
	if ( keys_fit > m_maxWhiteKeysShown )
		height = (100 * w / m_maxWhiteKeysShown) / WHITEKEY_H2W_RATIO_PERC;

	return height;
}

QSizePolicy PianoRollWidget::sizePolicy () const
{
	QSizePolicy policy ( QSizePolicy::Preferred, QSizePolicy::Preferred );
	policy.setHeightForWidth( true );
	return policy;
}

void PianoRollWidget::paintEvent( QPaintEvent * event )
{
	QWidget::paintEvent( event );

	// Calculate how many keys we can show in the width we have
	m_shownKeys = qMin( width() / MINIMUM_KEY_WIDTH, m_maxWhiteKeysShown );

	// If we can show more than m_maxWhiteKeysShown, the key will be wider
	m_whiteKeyWidth = qMax( MINIMUM_KEY_WIDTH, width() / m_shownKeys );

	// Calculate other stuff
	m_whiteKeyHeight = height();
	m_blackKeyHeight = (m_whiteKeyHeight * WHITEKEY_BLACKKEY_HEIGHT_RATIO) / 100;
	m_blackKeyWidth = (BLACKKEY_WIDTH_RATIO_PERC * m_whiteKeyWidth) / 100;

	// Prepare painter objects
	QPainter painter( this );

	QPen whitekeypen( Qt::black );
	whitekeypen.setWidth( 2 * SPACE_BETWEEN_KEYS );
	QPen blackkeypen( Qt::black );

	QColor whitekey( 255, 255, 255 );
	QColor whitekeyover( 240, 240, 240 );
	QColor whitekeydown( 255, 128, 128 );

	QColor blackkey( 0, 0, 0 );
	QColor blackkeyover( 32, 32, 32 );
	QColor blackkeydown( 255, 128, 128 );

	if ( !isEnabled() )
	{
		whitekey = QColor( 240, 240, 240 );
		blackkey = QColor( 64, 64, 64 );
	}

	// Draw white keys
	painter.setPen( whitekeypen );
	painter.setBrush( whitekey );

	for ( int i = 0; i < m_shownKeys; i++ )
	{
		int key_offset = i * m_whiteKeyWidth;

		if ( i == m_whiteKeyMouseOver )
		{
			if ( m_noteDown )
				painter.setBrush( whitekeydown );
			else
				painter.setBrush( whitekeyover );

			painter.drawRect( key_offset, 0, m_whiteKeyWidth, m_whiteKeyHeight );

			painter.setPen( whitekeypen );
			painter.setBrush( whitekey );
		}
		else
			painter.drawRect( key_offset, 0, m_whiteKeyWidth, m_whiteKeyHeight );
	}

	// Draw black keys (they will overwrite white keys)
	painter.setPen( blackkeypen );
	painter.setBrush( blackkey );

	for ( int i = 0; i < m_shownKeys - 1; i++ )
	{
		int note = i % 7;

		if ( note == 2 || note == 6 )
			continue;

		int key_offset = (i + 1) * m_whiteKeyWidth - m_blackKeyWidth / 2;

		if ( i == m_blackKeyMouseOver )
		{
			if ( m_noteDown )
				painter.setBrush( blackkeydown );
			else
				painter.setBrush( blackkeyover );

			painter.drawRect( key_offset, 0, m_blackKeyWidth, m_blackKeyHeight );

			painter.setPen( blackkeypen );
			painter.setBrush( blackkey );
		}
		else
			painter.drawRect( key_offset, 0, m_blackKeyWidth, m_blackKeyHeight );
	}
}

void PianoRollWidget::mouseMoveEvent( QMouseEvent * event )
{
	QPoint p = event->pos();

	// Any keys shown?
	if ( m_shownKeys == 0 )
	{
		m_whiteKeyMouseOver = -1;
		m_blackKeyMouseOver = -1;
		m_pitchMouseOver = -1;

		return;
	}

	// Are we in keyboard range?
	if ( p.x() > m_shownKeys * m_whiteKeyWidth || p.y() > m_whiteKeyHeight )
	{
		if ( m_pitchMouseOver != -1 )
		{
			m_pitchMouseOver = -1;
			m_whiteKeyMouseOver = -1;
			m_blackKeyMouseOver = -1;

			update();
		}

		return;
	}

	// Get a white key
	int whitekey = p.x() / m_whiteKeyWidth;
	int blackkey = -1;
	int xoffset = p.x() - (whitekey * m_whiteKeyWidth );

	// Calculate a note/octave
	int octave = whitekey / 7;
	int note = whitekey % 7;

	// Check whether it may be a black key
	if ( p.y() <= m_blackKeyHeight )
	{
		// Left side of white key; not applicable for C and F
		if ( xoffset < m_blackKeyWidth / 2 && note != 0 && note != 3 )
			blackkey = whitekey - 1;
		// Right side of white key; not applicable for E and B
		else if ( xoffset > (m_whiteKeyWidth - m_blackKeyWidth / 2) && note != 2 && note != 6 )
			blackkey = whitekey;
	}

	if ( blackkey != -1 )
	{
		m_blackKeyMouseOver = blackkey;
		m_whiteKeyMouseOver = -1;
	}
	else
	{
		m_blackKeyMouseOver = -1;
		m_whiteKeyMouseOver = whitekey;
	}

	// Calculate the note text
	static const int whitetone[] = { 0, 2, 4, 5, 7, 9, 11 };
	static const int blacktone[] = { 1, 3, -1, 6, 8, 10, -1 };

	int pitch = 12 * octave;

	if ( m_whiteKeyMouseOver != -1 )
		pitch += whitetone[ m_whiteKeyMouseOver % 7 ];
	else
		pitch += blacktone[ m_blackKeyMouseOver % 7 ];

	qDebug("pitch: %d", pitch );
	if ( pitch != m_pitchMouseOver )
	{
		m_pitchMouseOver = pitch;

		emit noteMouseOver( m_pitchMouseOver );
		update();
	}
}


bool PianoRollWidget::event ( QEvent * event )
{
	if ( event->type() == QEvent::ToolTip )
	{
		if ( m_pitchMouseOver != -1 )
		{
			QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
			int octave;

			QString note = pitchToNote( m_pitchMouseOver, &octave );
			QToolTip::showText( helpEvent->globalPos(), tr("%1, octave %2") .arg( note ) .arg( octave ) );
		}

		return true;
	}

	return QWidget::event( event );
}

void PianoRollWidget::mouseReleaseEvent( QMouseEvent * )
{
	if ( m_pitchMouseOver != -1 )
		emit noteClicked( m_pitchMouseOver );

	m_noteDown = false;
	update();
}

void PianoRollWidget::mousePressEvent( QMouseEvent * )
{
	m_noteDown = true;
	update();
}

void PianoRollWidget::leaveEvent(QEvent *)
{
	m_whiteKeyMouseOver = -1;
	m_blackKeyMouseOver = -1;
	update();
}

QString PianoRollWidget::pitchToNote( int pitch, int * octave )
{
	static const char * notes[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

	if ( octave )
		*octave = pitch / 12;

	return QString::fromAscii( notes[ pitch % 12 ] );
}
