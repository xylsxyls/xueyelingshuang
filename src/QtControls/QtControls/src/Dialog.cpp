#include "Dialog.h"

Dialog::Dialog(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
}

Dialog::~Dialog()
{

}