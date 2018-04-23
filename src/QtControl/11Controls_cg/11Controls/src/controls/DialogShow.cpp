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

std::wstring DialogShow::s_countDownString = L"还有%d秒自动关闭";

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

void DialogShow::initKeyboardAccept(COriginalButton* button)
{
    auto childs = children();
    for each (QObject* var in childs)
    {
        QWidget* widget = qobject_cast<QWidget*>(var);
        if (widget)
        {
            widget->installEventFilter(this);
        }
    }
    m_acceptButton = button;
    QObject::connect(this, &DialogShow::keyboardAccept, this, &DialogShow::onKeyboardAccept);
}

void DialogShow::onTimeUpdate(int32_t timeOut)
{
	m_time->setText(QString::fromStdWString(CStringManager::Format(s_countDownString.c_str(), timeOut)));
}

void DialogShow::onKeyboardAccept(QObject* tar, Qt::Key key)
{
    if (m_acceptButton != nullptr)
    {
        m_acceptButton->setFocus();
        m_acceptButton->click();
    }
}

void DialogShow::endDialog()
{
    //childAt(mapFromGlobal(QWidget::cursor().pos()))
    auto itResult = m_mapResult.find(focusWidget());
    if (itResult != m_mapResult.end())
    {
        done(itResult->second);
    }
}

void DialogShow::showEvent(QShowEvent* eve)
{
    if (m_timeRest <= 0)
    {
        m_time->setText("");
    }
    DialogBase::showEvent(eve);
}
