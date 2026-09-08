#include "DoubleSpinBox.h"

#include <QVariant>

DoubleSpinBox::DoubleSpinBox(QWidget* parent) :
ControlShow(parent)
{
	setProperty("class", QVariant(QString("DoubleSpinBox")));
	ControlBase::setControlShow(this);
	setDefault();
}

DoubleSpinBox::~DoubleSpinBox()
{

}

void DoubleSpinBox::setDefault()
{
	setBorderWidth(1);
	setSpinButtonSize(14, 8);
	setSpinArrowSize(7, 4);
}