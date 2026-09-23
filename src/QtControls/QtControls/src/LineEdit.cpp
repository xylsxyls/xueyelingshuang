#include "LineEdit.h"

LineEdit::LineEdit(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	setDefault();
}

LineEdit::~LineEdit()
{

}

void LineEdit::currentTextChanged(const QString& str)
{
	setToolTip(echoMode() == QLineEdit::Normal ? str : QString());
}

void LineEdit::setEchoMode(QLineEdit::EchoMode mode)
{
    QLineEdit::setEchoMode(mode);
    currentTextChanged(text());
}

void LineEdit::setDefault()
{
	setBorderWidth(1);
	QObject::connect(this, &LineEdit::textChanged, this, &LineEdit::currentTextChanged);
}
