#include "TimeEdit.h"

#include <QVariant>

TimeEdit::TimeEdit(QWidget* parent) :
ControlShow(parent)
{
	setProperty("class", QVariant(QString("TimeEdit")));
	ControlBase::setControlShow(this);
	setDefault();
}

TimeEdit::~TimeEdit()
{

}

void TimeEdit::setDefault()
{
	setBorderWidth(1);
	setSpinButtonSize(14, 8);
	setSpinArrowSize(7, 4);
}