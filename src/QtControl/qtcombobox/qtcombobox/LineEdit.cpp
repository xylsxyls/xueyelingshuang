#include "LineEdit.h"

LineEdit::LineEdit(QWidget* parent) :
ControlBase(parent)
{
	ControlBase::init(L"QLineEdit", L"");
	init();
}

void LineEdit::init()
{
	setBorderWidth(1);
}