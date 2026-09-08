#include "TableView.h"

#include <QVariant>

TableView::TableView(QWidget* parent) :
ControlShow(parent)
{
	setProperty("class", QVariant(QString("TableView")));
	ControlBase::setControlShow(this);
	setItemName(L"item");
	setDefault();
}

TableView::~TableView()
{

}

void TableView::setDefault()
{
	setBorderWidth(1);
	setShowGrid(true);
	setAlternatingRowColors(false);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
}