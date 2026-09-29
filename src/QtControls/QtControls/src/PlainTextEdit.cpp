#include "PlainTextEdit.h"
#include <QtCore/QVariant>

PlainTextEdit::PlainTextEdit(QWidget* parent) :
ControlShow(parent)
{
    ControlBase::setControlShow(this);
    setProperty("class", QVariant(QStringLiteral("PlainTextEdit")));
    setBorderWidth(1);
}