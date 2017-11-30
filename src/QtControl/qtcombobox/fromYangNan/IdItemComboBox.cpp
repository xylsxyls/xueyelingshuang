#include "IdItemComboBox.h"
#include <QListWidgetItem>
#include "ListWidget.h"

IdItemComboBox::IdItemComboBox(QWidget* parent) :
ComboBox(parent)
{
	
}

IdItemComboBox::~IdItemComboBox()
{

}

void IdItemComboBox::addItem(qint64 id, const QString& text)
{
	QListWidgetItem* widgetItem = new QListWidgetItem(m_listWidget);
	widgetItem->setText(text);
	widgetItem->setToolTip(text);
}

void IdItemComboBox::addItems(const QList<qint64>& idList, const QStringList& textList)
{

}