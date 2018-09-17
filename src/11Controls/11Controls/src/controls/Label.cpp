#include "Label.h"

Label::Label(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
}

Label::~Label()
{

}

void Label::setLineHeight(qint32 lineHeight)
{
	setText("<html><head/><body><p style='line-height:" + QString::number(lineHeight) + "px;'>" + text().replace(" ", "&nbsp;") + "</p></body></html>");
}
