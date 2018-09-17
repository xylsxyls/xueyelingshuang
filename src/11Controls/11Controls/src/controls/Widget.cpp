#include "Widget.h"

Widget::Widget(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
}

Widget::~Widget()
{

}