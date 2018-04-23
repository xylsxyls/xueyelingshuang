#include "NotifyDialog.h"
#include "DialogHelper.h"
#include "Label.h"
#include "COriginalButton.h"
#include "CGeneralStyle.h"
#include "src/core/CSystem.h"
#include "DialogType.h"
#include <QShowEvent>

NotifyDialog::NotifyDialog():
m_titleBar(nullptr),
m_icon(nullptr),
m_animation(this, "geometry"),
m_isShow(false)
{
    m_titleBar = new Label(this);
    m_icon = new Label(m_titleBar);

    setExistFocus(false);

    DialogHelper::setLabel(m_titleBar, "", QColor(0, 0, 0, 0), 12);//QColor(163, 175, 191, 255)
    m_titleBar->setBackgroundColor(QColor(67, 81, 117, 255));

    DialogHelper::setLabel(m_icon, "", QColor(0, 0, 0, 0), 12);
    m_icon->setBackgroundImage(CGeneralStyle::instance()->platformResourcePath() + "/Common/Image/NotificationView/11Logo.png", 1, 1, 1, 1);

    DialogHelper::setLabel(m_title, "title", QColor(221, 213, 198, 255), 12);//QColor(163, 175, 191, 255)
    m_title->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_title->raise();

    setNotifyButtonConfig(m_exit, "", 0, 12);
    m_exit->setBkgImage(CGeneralStyle::instance()->platformResourcePath() + "/Common/Image/NotificationView/CloseButton.png");
    m_exit->raise();

    DialogHelper::setLabel(m_time, "", QColor("#abb3d3"), 12);
    m_time->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QObject::connect(&m_animation, &QPropertyAnimation::finished, this, &NotifyDialog::end);
}

void NotifyDialog::init(const std::string& typeName)
{
    std::string className = typeName;
    CStringManager::Replace(className, "class ", "");
    setStyleSheet(("." + className + "{ background-color:rgba(44, 52, 74, 255); border-top-left-radius:2px; border-top-right-radius:2px; border:1px solid; border-color:rgba(78, 146, 212, 255); }").c_str());
}

void NotifyDialog::done(int32_t result)
{
    //去掉定时器
    if (m_timeId != -1)
    {
        killTimer(m_timeId);
        m_timeId = -1;
    }

    //代码关闭
    if (result == CODE_DESTROY)
    {
        DialogBase::done(result);
        return;
    }

    //先将所有控件设为禁用
    auto childs = children();
    for each (QObject* var in childs)
    {
        QWidget* widget = qobject_cast<QWidget*>(var);
        if (widget)
        {
            widget->setEnabled(false);
        }
    }

    m_result = result;
    m_isShow = false;
    m_animation.setDuration(250);
    m_animation.setStartValue(m_beginRect);
    m_animation.setEndValue(m_endRect);
    m_animation.start();
}

void NotifyDialog::setWindowTiTle(const QString& title,
                                  const QColor& color,
                                  int32_t fontSize,
                                  Qt::Alignment align,
                                  int32_t origin,
                                  const QString& fontName)
{
    DialogShow::setWindowTiTle(title, color, fontSize, align, origin, fontName);
}

void NotifyDialog::showEvent(QShowEvent* eve)
{
    DialogBase::showEvent(eve);
    m_isShow = true;
    m_animation.setDuration(500);
    m_animation.setStartValue(m_endRect);
    m_animation.setEndValue(m_beginRect);
    m_animation.start();
}

void NotifyDialog::resizeEvent(QResizeEvent* eve)
{
    DialogShow::resizeEvent(eve);
    m_titleBar->setGeometry(QRect(1, 1, width() - 2, 31));
    m_icon->setGeometry(QRect(6, 6, 13, 18));
    m_title->setGeometry(QRect(23, 1, width() - 20 - 34, 28));
    m_exit->setGeometry(QRect(width() - 34, 1, 34, 31));
    m_time->setGeometry(QRect(7, height() - 13, width() - 5, 13));

    POINT rightBottom = CSystem::taskbarRightBottomPoint();
    m_beginRect.setRect(rightBottom.x - width(), rightBottom.y - height(), width(), height());
    m_endRect.setRect(rightBottom.x - width(), rightBottom.y, width(), height());
}

void NotifyDialog::end()
{
    if (m_isShow == false)
    {
        DialogBase::done(m_result);
    }
}