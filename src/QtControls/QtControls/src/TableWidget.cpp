#include "TableWidget.h"

#include <QVariant>

TableWidget::TableWidget(QWidget* parent) :
ControlShow(parent)
{
	setProperty("class", QVariant(QString("TableWidget")));
	ControlBase::setControlShow(this);
	setItemName(L"item");
	setDefault();
}

TableWidget::~TableWidget()
{

}

void TableWidget::setDefault()
{
	setBorderWidth(1);
	setShowGrid(true);
	setAlternatingRowColors(false);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
}