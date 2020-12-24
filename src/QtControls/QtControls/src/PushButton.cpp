#include "PushButton.h"

PushButton::PushButton(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
}

PushButton::~PushButton()
{

}