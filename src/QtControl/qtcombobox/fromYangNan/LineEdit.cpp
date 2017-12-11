#include "LineEdit.h"

LineEdit::LineEdit(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	INIT(L"");
	setDefault();
}

LineEdit::~LineEdit()
{

}

void LineEdit::setDefault()
{
	setBorderWidth(1);
}