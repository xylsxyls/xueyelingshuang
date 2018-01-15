#include "TreeWidget.h"
#include <QTreeWidgetItem>
#include "NoFocusFrameDelegate.h"
#include <QScrollBar>

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

void TreeWidget::initScrollBar()
{
	verticalScrollBar()->setStyleSheet(
		"QScrollBar:vertical"
		"{"
			"width:6px;"
			"background:rgba(0,0,0,0%);"
			"margin:0px,0px,0px,0px;"
			"padding-top:0px;"
			"padding-bottom:0px;"
		"}"
		"QScrollBar::handle:vertical"
		"{"
			"width:6px;"
			"background:rgba(51,63,98,100%);"
			"border-radius:3px;"
			"min-height:10;"
		"}"
		"QScrollBar::handle:vertical:hover"
		"{"
			"width:6px;"
			"background:rgba(110,139,229,100%);"
			"border-radius:3px;"
			"min-height:10;"
		"}"
		"QScrollBar::add-line:vertical"
		"{"
			"height:0px;"
			"width:6px;"
			"border-image:none;"
			"subcontrol-position:bottom;"
		"}"
		"QScrollBar::sub-line:vertical"
		"{"
			"height:0px;"
			"width:6px;"
			"border-image:none;"
			"subcontrol-position:top;"
		"}"
		"QScrollBar::add-line:vertical:hover"
		"{"
			"height:0px;"
			"width:6px;"
			"border-image:none;"
			"subcontrol-position:bottom;"
		"}"
		"QScrollBar::sub-line:vertical:hover"
		"{"
			"height:0px;"
			"width:6px;"
			"border-image:none;"
			"subcontrol-position:top;"
		"}"
		"QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical"
		"{"
			"background:rgba(28,34,50,100%);"
			"border-radius:0px;"
		"}"
	);
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

void TreeWidget::removeWidget(QWidget* widget, int32_t column)
{
	auto itData = m_itemData.find(widget);
	if (itData == m_itemData.end())
	{
		return;
	}
	
	removeItemWidget(itData->second, column);
	delete widget;
	delete itData->second;
	m_itemData.erase(itData);
}
