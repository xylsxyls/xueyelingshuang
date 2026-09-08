#include "SpinBox.h"

#include <QVariant>

SpinBox::SpinBox(QWidget* parent) :
ControlShow(parent)
{
	setProperty("class", QVariant(QString("SpinBox")));
	ControlBase::setControlShow(this);
	setDefault();
}

SpinBox::~SpinBox()
{

}

void SpinBox::setDefault()
{
	setBorderWidth(1);
	setSpinButtonSize(14, 8);
	setSpinArrowSize(7, 4);
}