#include "DateTimeEdit.h"

#include <QVariant>

DateTimeEdit::DateTimeEdit(QWidget* parent) :
ControlShow(parent)
{
	setProperty("class", QVariant(QString("DateTimeEdit")));
	ControlBase::setControlShow(this);
	setDefault();
}

DateTimeEdit::~DateTimeEdit()
{

}

void DateTimeEdit::setDefault()
{
	setBorderWidth(1);
	setSpinButtonSize(14, 8);
	setSpinArrowSize(7, 4);
}