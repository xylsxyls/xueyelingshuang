#include "RPGContentBase.h"

RPGContentBase::RPGContentBase(QWidget* parent):
QWidget(parent)
{

}

void RPGContentBase::setState(ContentState state)
{
	emit stateSignal(state);
}

