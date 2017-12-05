#include "Label.h"

Label::Label(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	init(L"QLabel", L"");
}

Label::~Label()
{

}