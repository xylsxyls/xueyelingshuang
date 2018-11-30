#include "TextEdit.h"

TextEdit::TextEdit(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	setDefault();
}

TextEdit::~TextEdit()
{

}

void TextEdit::setDefault()
{
	setBorderWidth(1);
}