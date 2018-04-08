#include "IdItemComboBox.h"
#include "ListWidgetIdItem.h"
#include "ListWidget.h"

IdItemComboBox::IdItemComboBox(QWidget* parent) :
ComboBox(parent)
{
	INIT(L"drop-down");
	QObject::connect(this, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(curIndexChanged(const QString&)));
}

IdItemComboBox::~IdItemComboBox()
{

}

void IdItemComboBox::addItem(const QString& text, qint64 id)
{
	ListWidgetIdItem* widgetItem = new ListWidgetIdItem;
	widgetItem->setText(text);
	widgetItem->setToolTip(text);
	widgetItem->setId(id);
	m_listWidget->addItem(widgetItem);
}

void IdItemComboBox::addItems(const QStringList& textList, const QList<qint64>& idList)
{
	if ((idList.size() != textList.size()) && (idList.empty() == false))
	{
		return;
	}
	int32_t index = -1;
	while (index++ != textList.size() - 1)
	{
		ListWidgetIdItem* widgetItem = new ListWidgetIdItem;
		widgetItem->setText(textList[index]);
		widgetItem->setToolTip(textList[index]);
		if (idList.empty())
		{
			widgetItem->setId(0);
		}
		else
		{
			widgetItem->setId(idList[index]);
		}
		m_listWidget->addItem(widgetItem);
	}
}

void IdItemComboBox::setCurrentItemByFirstId(qint64 id)
{
	setCurrentIndex(itemIndexByFirstId(id));
}

int32_t IdItemComboBox::itemIndexByFirstId(qint64 id)
{
	int32_t index = -1;
	while (index++ != m_listWidget->count() - 1)
	{
		ListWidgetIdItem* item = (ListWidgetIdItem*)m_listWidget->item(index);
		if (item == nullptr)
		{
			continue;
		}
		if (item->getId() == id)
		{
			return index;
		}
	}
	return -1;
}

qint64 IdItemComboBox::currentItemId()
{
	return itemId(currentIndex());
}

qint64 IdItemComboBox::itemId(int32_t index)
{
	ListWidgetIdItem* item = (ListWidgetIdItem*)m_listWidget->item(index);
	if (item == nullptr)
	{
		return -1;
	}
	return item->getId();
}

void IdItemComboBox::curIndexChanged(const QString& str)
{
	ListWidgetIdItem* item = (ListWidgetIdItem*)m_listWidget->item(currentIndex());
	if (item == nullptr)
	{
		return;
	}
	emit currentItemChanged(item->getId(), str);
}