#include "RPGContentBase.h"

RPGContentBase::RPGContentBase(QWidget* parent):
QWidget(parent)
{

}

void RPGContentBase::setState(int32_t state)
{
	emit stateSignal(state);
}

