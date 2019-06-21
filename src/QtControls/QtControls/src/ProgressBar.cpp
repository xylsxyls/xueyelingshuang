#include "ProgressBar.h"

ProgressBar::ProgressBar(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	setItemName(L"chunk");
}

ProgressBar::~ProgressBar()
{

}