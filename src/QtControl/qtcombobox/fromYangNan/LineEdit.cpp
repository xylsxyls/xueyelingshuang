#include "LineEdit.h"

LineEdit::LineEdit(QWidget* parent) :
ControlBase(parent)
{
	ControlBase::init(L"QLineEdit", L"");
	init();
}

LineEdit::~LineEdit()
{

}

void LineEdit::init()
{
	setBorderWidth(1);
}