#include "StatusBar.h"
#include <QVariant>

StatusBar::StatusBar(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	setProperty("class", QVariant(QString("StatusBar")));
}
