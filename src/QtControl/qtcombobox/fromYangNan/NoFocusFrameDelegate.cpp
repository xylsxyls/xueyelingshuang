#include "nofocusframedelegate.h"
#include <QPainter>
#include <QTextDocument>

NoFocusFrameDelegate::NoFocusFrameDelegate(QWidget *parent) :
QStyledItemDelegate(parent)
{

}

void NoFocusFrameDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyleOptionViewItem itemOption(option);
	if (itemOption.state & QStyle::State_HasFocus)
	{
		itemOption.state ^= QStyle::State_HasFocus;
	}
	
	/*QTextDocument doc;
	doc.setHtml("asdf<font color = \"red\">15ms</font>");
	doc.drawContents(painter, QRect(0, 0, 100, 20));*/

	QStyledItemDelegate::paint(painter, itemOption, index);
}