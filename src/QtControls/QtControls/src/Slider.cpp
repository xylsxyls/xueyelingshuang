#include "Slider.h"
#include <QEvent>
#include <QApplication>
#include <QMouseEvent>

Slider::Slider(QWidget* parent):
ControlShow(parent)
{
	ControlBase::setControlShow(this);
	installEventFilter(this);
	setMouseTracking(true);
	setItemName(L"handle");
}

Slider::~Slider()
{
	installEventFilter(nullptr);
}