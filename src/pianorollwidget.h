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

#ifndef PIANOROLLWIDGET_H
#define PIANOROLLWIDGET_H

#include <QWidget>
#include <QDockWidget>


class PianoRollWidget : public QWidget
{
	Q_OBJECT

	public:
		PianoRollWidget( QWidget * parent, int showOctaves = 4 );

		// Returns note and possibly octave
		static QString pitchToNote( int pitch, int * octave = 0 );

	signals:
		void	noteMouseOver( unsigned int pitch );
		void	noteClicked( unsigned int pitch );

	protected:
		void	paintEvent( QPaintEvent *event );
		QSize	minimumSizeHint() const;
		QSize	sizeHint() const;
		QSizePolicy sizePolicy () const;
		int		heightForWidth( int w );

		void	leaveEvent(QEvent * e);
		void	mousePressEvent( QMouseEvent * event );
		void	mouseReleaseEvent( QMouseEvent * event );
		void	mouseMoveEvent( QMouseEvent * event );
		bool	event ( QEvent * event ); // tooltips

	private:
		double		m_zoomFactor;
		int			m_maxWhiteKeysShown;

		// A white or black key the mouse is currently over. -1 means none.
		// Those values are updated on mouseMoveEvent() so are actual values
		int			m_whiteKeyMouseOver;
		int			m_blackKeyMouseOver;
		bool		m_noteDown;
		int			m_pitchMouseOver;

		// Those values keep the paint-time calculations

		// How many white keys are shown
		int			m_shownKeys;

		// A width and height of a single white key (width includes spacing on both sides)
		int			m_whiteKeyWidth;
		int			m_whiteKeyHeight;

		// A width and height of a single black key
		int			m_blackKeyHeight;
		int			m_blackKeyWidth;
};


// A docked PianoRoll
class PianoRollDock : public QDockWidget
{
	public:
		PianoRollDock( QWidget * parent );

	private:
		PianoRollWidget	*	m_widget;
};

#endif // PIANOROLLWIDGET_H
