#include "DialogShow.h"
#include "Label.h"
#include "COriginalButton.h"
#include "DialogHelper.h"

DialogShow::DialogShow() :
m_acceptButton(nullptr),
m_userResult(nullptr),
m_time(nullptr),
m_exit(nullptr),
m_result(nullptr)
{
    m_time = new Label(this);
    m_exit = new COriginalButton(this);
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
}

qint32* DialogShow::userResultPtr()
{
	return m_userResult;
}

void DialogShow::initAcceptButton(COriginalButton* button)
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

void DialogShow::escEvent()
{
    if (m_result != nullptr)
    {
        *m_result = ESC_EXIT;
    }
    DialogBase::escEvent();
}

void DialogShow::altF4PressedEvent()
{
    if (m_result != nullptr)
    {
        *m_result = ALT_F4_EXIT;
    }
    DialogBase::altF4PressedEvent();
}

void DialogShow::showEvent(QShowEvent* eve)
{
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
    DialogBase::closeEvent(eve);
	emit closedSignal(m_result);
}
