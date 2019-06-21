#include "NoFocusFrameDelegate.h"

NoFocusFrameDelegate::NoFocusFrameDelegate(QWidget* parent) :
QStyledItemDelegate(parent)
{

}

NoFocusFrameDelegate::~NoFocusFrameDelegate()
{

}

void NoFocusFrameDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyleOptionViewItem itemOption(option);
	if (itemOption.state & QStyle::State_HasFocus)
	{
		itemOption.state ^= QStyle::State_HasFocus;
	}
	QStyledItemDelegate::paint(painter, itemOption, index);
}