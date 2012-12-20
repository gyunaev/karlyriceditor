#ifndef DIALOG_TIMEADJUSTMENT_H
#define DIALOG_TIMEADJUSTMENT_H

#include <QDialog>
#include "ui_dialog_timeadjustment.h"

class DialogTimeAdjustment : public QDialog, Ui::DialogTimeAdjustment
{
	Q_OBJECT

	public:
		explicit DialogTimeAdjustment( QWidget *parent = 0 );

	private:
		bool	getAndValidate( bool msgboxIfError );

	protected slots:
		void	textChanged ( const QString & text );
		void	accept();

	public:
		qint64	m_valueAdd;
		double	m_valueMultiply;
};

#endif // DIALOG_TIMEADJUSTMENT_H
