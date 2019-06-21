#include "IdItemComboBox.h"
#include <QListWidgetItem>
#include "ListWidget.h"

IdItemComboBox::IdItemComboBox(QWidget* parent) :
ComboBox(parent)
{
	init();
}

IdItemComboBox::~IdItemComboBox()
{

}

void IdItemComboBox::addItem(const QString& text, qint64 id)
{
	if (!check())
	{
		return;
	}
	QListWidgetItem* widgetItem = new QListWidgetItem;
	if (widgetItem == nullptr)
	{
		return;
	}
	widgetItem->setText(text);
	widgetItem->setToolTip(text);
	widgetItem->setData(s_idRole, id);
	m_listWidget->addItem(widgetItem);
}

void IdItemComboBox::addItems(const QStringList& textList, const QList<qint64>& idList)
{
	if (!check())
	{
		return;
	}
	if ((idList.size() != textList.size()) && (idList.empty() == false))
	{
		return;
	}
	qint32 index = -1;
	while (index++ != textList.size() - 1)
	{
		QListWidgetItem* widgetItem = new QListWidgetItem;
		if (widgetItem != nullptr)
		{
			widgetItem->setText(textList[index]);
			widgetItem->setToolTip(textList[index]);
			widgetItem->setData(s_idRole, idList.empty() ? 0 : idList[index]);
			m_listWidget->addItem(widgetItem);
		}
	}
}

void IdItemComboBox::setCurrentItemByFirstId(qint64 id)
{
	setCurrentIndex(itemIndexByFirstId(id));
}

qint32 IdItemComboBox::itemIndexByFirstId(qint64 id)
{
	if (!check())
	{
		return -1;
	}
	qint32 index = -1;
	while (index++ != m_listWidget->count() - 1)
	{
		QListWidgetItem* item = m_listWidget->item(index);
		if (item != nullptr)
		{
			if (item->data(s_idRole).toLongLong() == id)
			{
				return index;
			}
		}
	}
	return -1;
}

qint64 IdItemComboBox::currentItemId()
{
	return itemId(currentIndex());
}

qint64 IdItemComboBox::itemId(qint32 index)
{
	if (!check())
	{
		return -1;
	}
	QListWidgetItem* item = m_listWidget->item(index);
	if (item == nullptr)
	{
		return -1;
	}
	return item->data(s_idRole).toLongLong();
}

void IdItemComboBox::init()
{
	QObject::connect(this, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(curIndexChanged(const QString&)));
	QObject::connect(this, &ComboBox::itemPressed, this, &IdItemComboBox::onItemPressed);
}

void IdItemComboBox::curIndexChanged(const QString& str)
{
	if (!check())
	{
		return;
	}
	QListWidgetItem* item = m_listWidget->item(currentIndex());
	if (item == nullptr)
	{
		return;
	}
	emit currentItemChanged(item->data(s_idRole).toLongLong(), str);
}

void IdItemComboBox::onItemPressed(qint32 index)
{
	emit idItemPressed(itemId(index), itemText(index));
}