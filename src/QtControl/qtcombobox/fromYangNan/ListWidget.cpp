#include "ListWidget.h"

ListWidget::ListWidget(QWidget* parent) :
ControlBase(parent)
{
	init(L"QListWidget", L"item");
}

ListWidget::~ListWidget()
{

}

void ListWidget::setMaxHeight(int32_t maxHeight, bool rePaint)
{
	ControlBase::setPxValue(L"max-height", maxHeight, false, rePaint);
}