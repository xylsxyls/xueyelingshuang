#include "TreeWidget.h"
#include <QTreeWidgetItem>
#include "NoFocusFrameDelegate.h"

TreeWidget::TreeWidget(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	INIT(L"item");

	setIndentation(0);
	setHeaderHidden(true);
	setSelectionMode(QAbstractItemView::NoSelection);
	setItemDelegate(new NoFocusFrameDelegate(this));
}

TreeWidget::~TreeWidget()
{

}

void TreeWidget::setMaxHeight(int32_t maxHeight, bool rePaint)
{
	ControlBase::setPxValue(L"max-height", maxHeight, false, rePaint);
}

void TreeWidget::addWidget(int32_t itemHeight, QWidget* widget, QWidget* parent, int32_t column)
{
	if (widget == nullptr)
	{
		return;
	}
	if (parent == nullptr)
	{
		QTreeWidgetItem* item = new QTreeWidgetItem(this);
		if (item == nullptr)
		{
			return;
		}
		item->setSizeHint(0, QSize(width(), itemHeight));
		setItemWidget(item, column, widget);
		m_itemData[widget] = item;
	}
	else
	{
		auto itData = m_itemData.find(parent);
		if (itData == m_itemData.end())
		{
			return;
		}
		QTreeWidgetItem* item = new QTreeWidgetItem(itData->second);
		if (item == nullptr)
		{
			return;
		}
		item->setSizeHint(0, QSize(width(), itemHeight));
		itData->second->addChild(item);
		setItemWidget(item, 0, widget);
		m_itemData[widget] = item;
	}
}