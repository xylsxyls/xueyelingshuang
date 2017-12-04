#include "LineEdit.h"

LineEdit::LineEdit(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	init(L"QLineEdit", L"");
	setDefault();
}

LineEdit::~LineEdit()
{

}

void LineEdit::setDefault()
{
	setBorderWidth(1);
}