#include "TreeViewModel.h"
#include "TreeView.h"
#include "TreeViewHeader.h"

TreeViewModel::TreeViewModel(QObject* parent):
QStandardItemModel(parent)
{
	init();
}

TreeViewModel::~TreeViewModel()
{

}

QStandardItem* TreeViewModel::getItem(int32_t row, int32_t column)
{
	if (row < 0 || column < 0)
	{
		return nullptr;
	}
	QStandardItem* itemPtr = item(row, column);
	if (itemPtr != nullptr)
	{
		return itemPtr;
	}
	QStandardItem* item = new QStandardItem;
	QStandardItemModel::setItem(row, column, item);
	return item;
}

void TreeViewModel::setHeaderText(int32_t index, const TreeText& text)
{
	if (index < 0)
	{
		return;
	}
	m_headerTextMap[index] = text;

}

std::map<int32_t, TreeText>* TreeViewModel::headerText()
{
	return &m_headerTextMap;
}

void TreeViewModel::setBodyText(int32_t row, int32_t column, const TreeText& text)
{
	QStandardItem* item = getItem(row, column);
	if (item == nullptr)
	{
		return;
	}
	m_bodyTextMap[row][column] = text;

	if (text.m_widget == nullptr)
	{
		return;
	}
	TreeView* view = qobject_cast<TreeView*>(parent());
	if (view == nullptr)
	{
		return;
	}
	view->openPersistentEditor(index(row, column));
}

std::map<int32_t, std::map<int32_t, TreeText>>* TreeViewModel::bodyText()
{
	return &m_bodyTextMap;
}

std::map<int32_t, std::vector<QColor>>* TreeViewModel::itemBackgroundColor()
{
	return &m_itemBackgroundColorMap;
}

void TreeViewModel::setHeaderHeight(int32_t height)
{
	const int32_t validHeight = qMax(height, 0);
	int32_t count = columnCount();
	while (count-- != 0)
	{
		setHeaderData(count, Qt::Horizontal, QSize(0, validHeight), Qt::SizeHintRole);
	}
}

void TreeViewModel::setRowHeight(int32_t height, int32_t row)
{
	if (row < -1)
	{
		return;
	}
	const int32_t validHeight = qMax(height, 0);
	if (row == -1)
	{
		int32_t lines = rowCount();
		while (lines-- != 0)
		{
			int32_t count = columnCount();
			while (count-- != 0)
			{
				setData(index(lines, count), QSize(0, validHeight), Qt::SizeHintRole);
			}
		}
		return;
	}
	int32_t count = columnCount();
	while (count-- != 0)
	{
		setData(index(row, count), QSize(0, validHeight), Qt::SizeHintRole);
	}
}

void TreeViewModel::setColumnWidth(int32_t width, int32_t column)
{
	if (column < -1)
	{
		return;
	}
	const int32_t validWidth = qMax(width, 0);
	TreeView* view = qobject_cast<TreeView*>(parent());
	if (view == nullptr)
	{
		return;
	}
	if (column == -1)
	{
		int32_t columns = columnCount();
		while (columns-- != 0)
		{
			view->setColumnWidth(columns, validWidth);
		}
		return;
	}
	view->setColumnWidth(column, validWidth);
}

void TreeViewModel::setListItemColor(const QColor& normalColor,
									 const QColor& hoverColor,
									 const QColor& selectedColor,
									 const QColor& disabledColor,
									 int32_t row)
{
	m_itemBackgroundColorMap[row].clear();
	m_itemBackgroundColorMap[row].push_back(normalColor);
	m_itemBackgroundColorMap[row].push_back(hoverColor);
	m_itemBackgroundColorMap[row].push_back(selectedColor);
	m_itemBackgroundColorMap[row].push_back(disabledColor);
}

void TreeViewModel::setColumnWidthFixedMode()
{
	TreeView* view = qobject_cast<TreeView*>(parent());
	if (view == nullptr)
	{
		return;
	}
	QHeaderView* header = view->header();
	if (header == nullptr)
	{
		return;
	}
	int32_t columns = columnCount();
	while (columns-- != 0)
	{
		header->setSectionResizeMode(columns, QHeaderView::Fixed);
	}
}

void TreeViewModel::init()
{
	if (!check())
	{
		return;
	}

}

bool TreeViewModel::check()
{
	return true;
}