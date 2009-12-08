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

#include "project.h"
#include "lyrics.h"
#include "dialog_edittimemark.h"


DialogEditTimeMark::DialogEditTimeMark( bool has_pitch, QWidget * parent )
	: QDialog( parent ), Ui::TimeMarkEdit()
{
	setupUi( this );

	if ( !has_pitch )
	{
		framePitch->hide();
		buttonBox->hide();

		connect( timeEdit, SIGNAL(editingFinished()), this, SLOT(accept()) );
		resize( width(), 1 );
	}
	else
	{
		connect( pitchSpin, SIGNAL(valueChanged(int)), this, SLOT(pitchValueChanged(int)) );
	}
}

void DialogEditTimeMark::setPitch( int pitch )
{
	pitchSpin->setValue( pitch );
}

void DialogEditTimeMark::setTimemark( qint64 timemark )
{
	int minute, second, msecond;
	Project::splitTimeMark( timemark, &minute, &second, &msecond );

	timeEdit->setTime( QTime ( 0, minute, second, msecond ) );
}

int	DialogEditTimeMark::pitch() const
{
	return pitchSpin->value();
}

qint64 DialogEditTimeMark::timemark() const
{
	QTime tm = timeEdit->time();

	return tm.minute() * 60000 + tm.second() * 1000 + tm.msec();
}

void DialogEditTimeMark::pitchValueChanged( int newpitch )
{
	if ( newpitch == -1 )
		pitchNote->setText( tr("Note: none") );
	else
		pitchNote->setText( tr("Note: %1") .arg( Lyrics::pitchToNote( newpitch ) ) );
}
