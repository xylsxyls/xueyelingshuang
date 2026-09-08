#include "DateEdit.h"

#include <QVariant>

DateEdit::DateEdit(QWidget* parent) :
ControlShow(parent)
{
	setProperty("class", QVariant(QString("DateEdit")));
	ControlBase::setControlShow(this);
	setDefault();
}

DateEdit::~DateEdit()
{

}

void DateEdit::setDefault()
{
	setBorderWidth(1);
	setSpinButtonSize(14, 8);
	setSpinArrowSize(7, 4);
}