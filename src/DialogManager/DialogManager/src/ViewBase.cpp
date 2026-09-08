#include "ViewBase.h"
#include "ViewBaseController.h"

ViewBase::ViewBase(QWidget* parent) :
QWidget(parent),
m_controller(nullptr)
{

}

ViewBase::~ViewBase()
{
	m_controller = nullptr;
}

bool ViewBase::initView(const DialogParam& param)
{
	Q_UNUSED(param);
	return true;
}

QSize ViewBase::preferredSize() const
{
	return sizeHint();
}

QSize ViewBase::minimumViewSize() const
{
	return minimumSizeHint();
}

QWidget* ViewBase::defaultFocusWidget() const
{
	return nullptr;
}

void ViewBase::updateView(const DialogParam& param)
{
	Q_UNUSED(param);
}

void ViewBase::setController(ViewBaseController* controller)
{
	m_controller = controller;
}

void ViewBase::notifyResultChanged(DialogResult result, qint32 userResult)
{
	if (m_controller == nullptr)
	{
		return;
	}
	m_controller->viewResultChanged(this, result, userResult);
}

void ViewBase::notifyCloseRequested(DialogResult result, qint32 userResult)
{
	if (m_controller == nullptr)
	{
		return;
	}
	m_controller->viewCloseRequested(this, result, userResult);
}

void ViewBase::notifyResizeRequested(const QSize& size)
{
	if (m_controller == nullptr)
	{
		return;
	}
	m_controller->viewResizeRequested(this, size);
}