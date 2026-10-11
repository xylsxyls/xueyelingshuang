#include "LumaPlayerMicrophoneView.h"
#include "LumaPlayer.h"
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QApplication>

LumaPlayerMicrophoneParam::LumaPlayerMicrophoneParam(LumaPlayer* player) :
CustomDialogParam(static_cast<DialogType>(CUSTOM_DIALOG_TYPE_BEGIN + 3)),
m_player(player)
{

}

LumaPlayerMicrophoneView::LumaPlayerMicrophoneView(LumaPlayer* player) :
m_player(player)
{

}

bool LumaPlayerMicrophoneView::initView(const DialogParam&)
{
    if (!m_player)
    {
        return false;
    }
    CustomDialog* dialog = dynamic_cast<CustomDialog*>(window());
    if (!dialog)
    {
        return false;
    }
    dialog->setWindowBorderEnabled(false);
    dialog->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    dialog->setObjectName(QStringLiteral("LumaPlayerMicrophonePanel"));
    setObjectName(QStringLiteral("microphonePanelContent"));
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    dialog->installEventFilter(this);
    m_player->attachMicrophonePanel(this);
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, timer]()
        {
            if (!m_player || !m_player->microphonePanelVisible())
            {
                timer->stop();
                notifyCloseRequested(IGNORE_BUTTON, 0);
                return;
            }
            window()->setGeometry(m_player->microphonePanelGeometry());
            update();
        });
    timer->start(16);
    dialog->setGeometry(m_player->microphonePanelGeometry());
    return true;
}

QSize LumaPlayerMicrophoneView::preferredSize() const
{
    return QSize(360, 430);
}
bool LumaPlayerMicrophoneView::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == window() && event->type() == QEvent::Close && m_player)
    {
        m_player->dismissMicrophonePanel();
    }
    if (watched == window() && event->type() == QEvent::WindowDeactivate)
    {
        QTimer::singleShot(0, this, [this]()
            {
                if (m_player && !m_player->isActiveWindow() && !window()->isActiveWindow() && !QApplication::activePopupWidget())
                {
                    m_player->dismissMicrophonePanel();
                }
            });
    }
    return CustomViewBase::eventFilter(watched, event);
}
void LumaPlayerMicrophoneView::paintEvent(QPaintEvent*)
{
    if (!m_player)
    {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    m_player->paintMicrophonePanel(painter);
}
void LumaPlayerMicrophoneView::mousePressEvent(QMouseEvent* event)
{
    if (m_player)
    {
        m_player->microphonePanelMouse(event);
    }
    event->accept();
}
void LumaPlayerMicrophoneView::mouseDoubleClickEvent(QMouseEvent* event)
{
    mousePressEvent(event);
}
void LumaPlayerMicrophoneView::mouseMoveEvent(QMouseEvent* event)
{
    mousePressEvent(event);
}
void LumaPlayerMicrophoneView::mouseReleaseEvent(QMouseEvent* event)
{
    mousePressEvent(event);
}
void LumaPlayerMicrophoneView::keyPressEvent(QKeyEvent* event)
{
    if (m_player)
    {
        m_player->microphonePanelKey(event);
    }
    event->accept();
}
CustomViewBase* LumaPlayerMicrophoneFactory::createView(const DialogParam& param)
{
    const LumaPlayerMicrophoneParam* value = dynamic_cast<const LumaPlayerMicrophoneParam*>(&param);
    return value && value->m_player ? new LumaPlayerMicrophoneView(value->m_player) : nullptr;
}
void LumaPlayerMicrophoneFactory::destroy(CustomViewFactory* factory)
{
    delete factory;
}