#include "GroupBox.h"
#include <QVariant>

GroupBox::GroupBox(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	setProperty("class", QVariant(QString("GroupBox")));
}
