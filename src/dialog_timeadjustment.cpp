#include <QMessageBox>
#include "dialog_timeadjustment.h"
#include "editor.h"

DialogTimeAdjustment::DialogTimeAdjustment(QWidget *parent) :
	QDialog(parent)
{
	setupUi( this );
	m_valueAdd = 0;
	m_valueMultiply = 1.0;

	connect( leAdd, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)) );
	connect( leMultiply, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)) );
	connect( leTestIn, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)) );

	leTestIn->setText( "0:20.11" );
	leAdd->setText( QString::number( m_valueAdd ) );
	leMultiply->setText( QString::number( m_valueMultiply ) );
}

void DialogTimeAdjustment::textChanged ( const QString & )
{
	if ( !getAndValidate( false ) )
		return;

	qint64 timing = timeToMark( leTestIn->text() );
	timing *= m_valueMultiply;
	timing += m_valueAdd;

	leTestOut->setText( markToTime( timing ));
}

void DialogTimeAdjustment::accept()
{
	if ( !getAndValidate( true ) )
		return;

	QDialog::accept();
}


bool DialogTimeAdjustment::getAndValidate( bool msgboxIfError )
{
	bool ok;
	QString errtxt;

	m_valueAdd = leAdd->text().toLongLong( &ok );

	if ( ok )
	{
		m_valueMultiply = leMultiply->text().toDouble( &ok );

		if ( !ok )
			errtxt = "Specified Multiply value is not valid";
	}
	else
		errtxt = "Specified Add value is not valid";

	if ( ok )
	{
		txtError->setText("");
		return true;
	}

	if ( msgboxIfError )
		QMessageBox::critical( 0, "Invalid value", errtxt );

	txtError->setText( "<font color='red'>" + errtxt + "</font>" );
	return false;
}
