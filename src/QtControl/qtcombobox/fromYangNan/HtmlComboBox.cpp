#include "HtmlComboBox.h"
#include <QPainter>
#include <QTextDocument>

HtmlComboBox::HtmlComboBox(QWidget* parent) :
ControlBase(parent)
{

}

HtmlComboBox::~HtmlComboBox()
{

}

void HtmlComboBox::paintEvent(QPaintEvent* eve)
{
	QComboBox::paintEvent(eve);
	QPainter paint(this);
	paint.drawLine(0, 0, 50, 10);
	QTextDocument doc;
	doc.setHtml("asdf<font color = \"red\">15ms</font>");
	doc.drawContents(&paint, QRect(0, 0, 100, 20));
}