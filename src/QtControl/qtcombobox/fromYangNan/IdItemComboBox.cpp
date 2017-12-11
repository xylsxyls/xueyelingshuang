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

void IdItemComboBox::addItem(qint64 id, const QString& text)
{
	ListWidgetIdItem* widgetItem = new ListWidgetIdItem;
	widgetItem->setText(text);
	widgetItem->setToolTip(text);
	widgetItem->setId(id);
	m_listWidget->addItem(widgetItem);
}

void IdItemComboBox::addItems(const QList<qint64>& idList, const QStringList& textList)
{
	if (idList.size() != textList.size())
	{
		return;
	}
	int32_t index = -1;
	while (index++ != idList.size() - 1)
	{
		ListWidgetIdItem* widgetItem = new ListWidgetIdItem;
		widgetItem->setText(textList[index]);
		widgetItem->setToolTip(textList[index]);
		widgetItem->setId(idList[index]);
		m_listWidget->addItem(widgetItem);
	}
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