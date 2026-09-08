#include "CustomDialog.h"
#include "CustomViewBase.h"
#include <QResizeEvent>

CustomDialog::CustomDialog() :
m_view(nullptr),
m_showMode(POP_DIALOG_SHOW_MODE)
{
	setExitVisible(false);
	setTimeRestVisible(false);
}

CustomDialog::~CustomDialog()
{
	if (m_view != nullptr)
	{
		m_view->setController(nullptr);
		m_view = nullptr;
	}
}

bool CustomDialog::initDialog(const DialogParam& param)
{
	setWindowTitle(param.m_title);
	setTimeRest(param.m_timeOut);
	setTimeRestVisible(param.m_isCountDownVisible);
	setTransientWindow(param.m_parent);
	if (m_view == nullptr)
	{
		return true;
	}
	if (!m_view->initView(param))
	{
		return false;
	}
	QSize minSize = m_view->minimumViewSize();
	if (minSize.isValid() && minSize.width() > 0 && minSize.height() > 0)
	{
		setMinimumSize(minSize);
	}
	QSize preferred = m_view->preferredSize();
	if (preferred.isValid() && preferred.width() > 0 && preferred.height() > 0)
	{
		resize(preferred);
	}
	QWidget* focusWidget = m_view->defaultFocusWidget();
	if (focusWidget != nullptr)
	{
		focusWidget->setFocus();
	}
	return true;
}

bool CustomDialog::setView(CustomViewBase* view)
{
	if (view == nullptr)
	{
		return false;
	}
	if (m_view == view)
	{
		return true;
	}
	if (m_view != nullptr)
	{
		m_view->setController(nullptr);
		delete m_view;
		m_view = nullptr;
	}
	m_view = view;
	m_view->setParent(this);
	m_view->setController(this);
	m_view->show();
	m_view->setGeometry(rect());
	return true;
}

CustomViewBase* CustomDialog::view() const
{
	return m_view;
}

void CustomDialog::setShowMode(DialogShowMode showMode)
{
	if (showMode != POP_DIALOG_SHOW_MODE && showMode != MODELESS_DIALOG_SHOW_MODE)
	{
		return;
	}
	m_showMode = showMode;
}

DialogShowMode CustomDialog::showMode() const
{
	return m_showMode;
}

bool CustomDialog::isModalMode() const
{
	return m_showMode == POP_DIALOG_SHOW_MODE;
}

void CustomDialog::updateCustomView(const DialogParam& param)
{
	if (m_view == nullptr)
	{
		return;
	}
	m_view->updateView(param);
}

void CustomDialog::viewResultChanged(ViewBase* view, DialogResult result, qint32 userResult)
{
	if (view == nullptr || view != m_view)
	{
		return;
	}
	setWindowResult(result);
	setUserResult(userResult);
}

void CustomDialog::viewCloseRequested(ViewBase* view, DialogResult result, qint32 userResult)
{
	if (view == nullptr || view != m_view)
	{
		return;
	}
	setWindowResult(result);
	setUserResult(userResult);
	close();
}

void CustomDialog::viewResizeRequested(ViewBase* view, const QSize& size)
{
	if (view == nullptr || view != m_view)
	{
		return;
	}
	if (!size.isValid() || size.width() <= 0 || size.height() <= 0)
	{
		return;
	}
	resize(size);
}

void CustomDialog::resizeEvent(QResizeEvent* eve)
{
	DialogShow::resizeEvent(eve);
	if (m_view == nullptr)
	{
		return;
	}
	m_view->setGeometry(rect());
}