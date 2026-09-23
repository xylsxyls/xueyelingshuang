#include "ToolBar.h"
#include <QVariant>

ToolBar::ToolBar(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	setProperty("class", QVariant(QString("ToolBar")));
}
