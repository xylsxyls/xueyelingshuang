#include "DialogShow.h"
#include "Label.h"
#include <QPainter>
#include <QMouseEvent>
#include "COriginalButton.h"
#include <QWindow>
#include <Windows.h>
#include "../core/CStringManager.h"
#include "CGeneralStyle.h"
#include "NotifyDialogManager.h"
#include <fstream>
#include "DialogHelper.h"

DialogShow::DialogShow() :
m_acceptButton(nullptr),
m_userParam(-1),
m_time(nullptr),
m_exit(nullptr),
m_result(0)
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

void DialogShow::setUserParam(int32_t userParam)
{
    m_userParam = userParam;
}

int32_t DialogShow::userParam()
{
    return m_userParam;
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

void DialogShow::onTimeUpdate(int32_t timeOut)
{
    if (!check())
    {
        return;
    }
    m_time->setText(QString::fromStdWString(CStringManager::Format(L"����%d���Զ��ر�", timeOut)));
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

void DialogShow::EscEvent()
{
    m_result = ESC_EXIT;
    setResult(m_result);
    DialogBase::EscEvent();
}

void DialogShow::altF4PressedEvent()
{
    m_result = ALT_F4_EXIT;
    setResult(m_result);
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
