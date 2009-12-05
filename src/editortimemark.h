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

#ifndef EDITORTIMEMARK_H
#define EDITORTIMEMARK_H

#include <QObject>
#include <QTextObjectInterface>

// This class represents a fake "text object", which represents a timing image.
class EditorTimeMark : public QObject, public QTextObjectInterface
{
	Q_OBJECT
	Q_INTERFACES(QTextObjectInterface)

	public:
		enum { TimeTextFormat = QTextFormat::UserObject + 1 };

		enum
		{
			TimeProperty = 1,
			PitchProperty,
			IdProperty,
		};

		EditorTimeMark();

		QSizeF intrinsicSize( QTextDocument *doc, int posInDocument, const QTextFormat &format );

		void drawObject( QPainter *painter, const QRectF &rect, QTextDocument *doc,
						 int posInDocument, const QTextFormat &format );

		QString toolTip() const { return m_tooltip; }

	private:
		void updatePixmapIfNecessary( const QTextFormat &format );

	private:
		qint64		m_timing;
		QPixmap		m_pixmap;
		QString		m_tooltip;
};


#endif // EDITORTIMEMARK_H
