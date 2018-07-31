#include "ListWidget.h"
#include <QMouseEvent>

ListWidget::ListWidget(QWidget* parent) :
ControlShow(parent),
m_click(true)
{
	ControlBase::setControlShow(this);
	setItemName(L"item");
}

ListWidget::~ListWidget()
{

}

void ListWidget::setMaxHeight(qint32 maxHeight, bool rePaint)
{
	ControlBase::setPxValue(L"max-height", maxHeight, false, rePaint);
}

void ListWidget::setClickEnable(bool enable)
{
	m_click = enable;
}

void ListWidget::mousePressEvent(QMouseEvent* eve)
{
	if (eve == nullptr)
	{
		QListWidget::mousePressEvent(eve);
		return;
	}
	if (m_click == false)
	{
		eve->ignore();
	}
	
	emit itemPressed(itemAt(eve->pos()));
	QListWidget::mousePressEvent(eve);
}