#include "ProgressBar.h"

ProgressBar::ProgressBar(QWidget* parent) :
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	INIT(L"chunk");
}

ProgressBar::~ProgressBar()
{

}