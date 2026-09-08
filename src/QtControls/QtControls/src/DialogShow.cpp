#include "DialogShow.h"
#include "Label.h"
#include "PushButton.h"

DialogShow::DialogShow() :
m_exit(nullptr),
m_userResult(nullptr),
m_time(nullptr),
m_result(nullptr),
m_internalUserResult(-1),
m_internalResult(ERROR_RESULT),
m_acceptButton(nullptr),
m_isUsingInternalResultStorage(false),
m_closedSignalEmitted(false)
{
    m_time = new Label(this);
    m_exit = new PushButton(this);
    QObject::connect(this, &DialogBase::timeRest, this, &DialogShow::onTimeUpdate);
}

DialogShow::~DialogShow()
{
	
}

void DialogShow::setTimeRestVisible(bool visible)
{
    if (!check())
    {
        return;
    }
    m_time->setVisible(visible);
}

void DialogShow::setUserResult(qint32 userResult)
{
	if (m_userResult == nullptr)
	{
		return;
	}
	*m_userResult = userResult;
}

qint32 DialogShow::userResult()
{
	if (m_userResult == nullptr)
	{
		return -1;
	}
    return *m_userResult;
}

void DialogShow::setUserResultPtr(qint32* userResult)
{
	m_userResult = userResult;
	if (m_userResult != &m_internalUserResult)
	{
		m_isUsingInternalResultStorage = false;
	}
}

qint32* DialogShow::userResultPtr()
{
	return m_userResult;
}

void DialogShow::useInternalResultStorage(DialogResult result, qint32 userResult)
{
	m_internalResult = result;
	m_internalUserResult = userResult;
	m_result = &m_internalResult;
	m_userResult = &m_internalUserResult;
	m_isUsingInternalResultStorage = true;
}

void DialogShow::clearResultStorage()
{
	m_result = nullptr;
	m_userResult = nullptr;
	m_isUsingInternalResultStorage = false;
}

bool DialogShow::closeSignalEmitted() const
{
	return m_closedSignalEmitted;
}

void DialogShow::initAcceptButton(PushButton* button)
{
    if (button == nullptr)
    {
        return;
    }
    addListenKey(Qt::Key_Space);
    addListenKey(Qt::Key_Return);
    addListenKey(Qt::Key_Enter);
    QObject::connect(this, &DialogShow::keyboardAccept, this, &DialogShow::onKeyboardAccept, Qt::QueuedConnection);
    m_acceptButton = button;
}

void DialogShow::setWindowResultAddr(DialogResult* result)
{
    m_result = result;
	if (m_result != &m_internalResult)
	{
		m_isUsingInternalResultStorage = false;
	}
}

void DialogShow::setWindowResult(DialogResult result)
{
    if (m_result != nullptr)
    {
        *m_result = result;
    }
}

void DialogShow::setExitVisible(bool visible)
{
	if (m_exit == nullptr)
	{
		return;
	}
	m_exit->setVisible(visible);
}

void DialogShow::onTimeUpdate(qint32 timeOut)
{
    if (!check())
    {
        return;
    }
    m_time->setText(QString::fromStdWString(CStringManager::Format(L"还有%d秒自动关闭", timeOut)));
}

void DialogShow::onKeyboardAccept(QObject* tar, Qt::Key key)
{
    switch (key)
    {
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
    {
        if (m_acceptButton != nullptr)
        {
            m_acceptButton->setFocus();
            m_acceptButton->click();
        }
        break;
    }
    default:
        break;
    }
}

bool DialogShow::check()
{
    return m_exit != nullptr && m_time != nullptr && DialogBase::check();
}

void DialogShow::resetCloseState()
{
	m_closedSignalEmitted = false;
}

//void DialogShow::escEvent()
//{
//    if (m_result != nullptr)
//    {
//        *m_result = ESC_EXIT;
//    }
//    DialogBase::escEvent();
//}

//void DialogShow::altF4PressedEvent()
//{
//    if (m_result != nullptr)
//    {
//        *m_result = ALT_F4_EXIT;
//    }
//    DialogBase::altF4PressedEvent();
//}

void DialogShow::showEvent(QShowEvent* eve)
{
	if (eve == nullptr)
	{
		return;
	}
	resetCloseState();
    if (!check())
    {
        return;
    }
    if (m_timeRest <= 0)
    {
        m_time->setText("");
    }
    DialogBase::showEvent(eve);
}

void DialogShow::closeEvent(QCloseEvent* eve)
{
	if (eve != nullptr)
	{
		DialogBase::closeEvent(eve);
	}
	if (m_closedSignalEmitted)
	{
		return;
	}
	m_closedSignalEmitted = true;
	emit closedSignal(m_result);
}