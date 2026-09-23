#include "Splitter.h"
#include <QVariant>

Splitter::Splitter(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	setProperty("class", QVariant(QString("Splitter")));
}
