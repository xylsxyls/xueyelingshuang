#include "IdItemComboBox.h"
#include <QListWidgetItem>
#include "ListWidget.h"
#include <QDebug>

IdItemComboBox::IdItemComboBox(QWidget* parent) :
ControlBase(parent)
{
	//m_pSendWidget->hide();
	const QObject* ss = (const QObject*)this;

	QObject* ssss = this;

	//QObject::connect(m_pSendWidget, &QSendWidget::currentTextChanged3, this, &IdItemComboBox::currentTextChanged4);
	//QObject::connect(m_pSendWidget, SIGNAL(currentTextChanged3(const QString&)), this, SLOT(currentTextChanged4(const QString&)));
	//QObject::connect(this, SIGNAL(currentTextChanged(const QString&)), this, SLOT(currentTextChanged4(const QString&)));
	//QObject::connect(this, &QComboBox::currentTextChanged, this, &IdItemComboBox::currentTextChanged4);
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

void IdItemComboBox::currentTextChanged4(const QString& str)
{
	//QObject::connect(id, &QComboBox::currentTextChanged, this, &QSendWidget::currentTextChanged2);
	int x = 3;
}