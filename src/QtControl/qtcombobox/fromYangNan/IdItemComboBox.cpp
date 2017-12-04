#include "IdItemComboBox.h"
#include <QListWidgetItem>
#include "ListWidget.h"
#include <QDebug>

IdItemComboBox::IdItemComboBox(QWidget* parent) :
ComboBox(parent)
{
	setEditable(true);
	QObject::connect(this, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(curIndexChanged(const QString &)));
	QAbstractItemView * tview = this->view();
	/*QObject::connect(tview, &QAbstractItemView::clicked, [this](const QModelIndex& tindex){
		qDebug() << tindex.data(Qt::DisplayRole).toString();
	}
	);*/
}



IdItemComboBox::~IdItemComboBox()
{

}

void IdItemComboBox::addItem(qint64 id, const QString& text)
{
	QListWidgetItem* widgetItem = new QListWidgetItem;
	widgetItem->setText(text);
	widgetItem->setToolTip(text);
	m_listWidget->addItem(widgetItem);
	m_mapIndex[widgetItem] = id;
}

void IdItemComboBox::addItems(const QList<qint64>& idList, const QStringList& textList)
{

}

void IdItemComboBox::curIndexChanged(const QString& str)
{
	//QObject::connect(id, &QComboBox::currentTextChanged, this, &QSendWidget::currentTextChanged2);
	int x = 3;
}