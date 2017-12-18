#include "Label.h"

Label::Label(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	INIT(L"");
}

Label::~Label()
{

}