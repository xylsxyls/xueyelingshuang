#include "WindowTitleBar.h"
#include "FramelessMainWindow.h"
#include "Label.h"
#include "PushButton.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>

WindowTitleBar::WindowTitleBar(FramelessMainWindow* host, const WindowTitleBarConfig& config) :
Widget(host),
m_host(host),
m_config(config),
m_actions(new QHBoxLayout),
m_dragging(false)
{
    setObjectName(QStringLiteral("windowTitleBar"));
    setFixedHeight(m_config.m_height);
    setBackgroundColor(m_config.m_background);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(m_config.m_margin, 0, m_config.m_margin, 0);
    layout->setSpacing(m_config.m_spacing);
    Label* logo = new Label(this);
    logo->setObjectName(QStringLiteral("windowTitleIcon"));
    logo->setFixedSize(m_config.m_iconSize, m_config.m_iconSize);
    logo->setPixmap(host->windowIcon().pixmap(logo->size()));
    logo->setAttribute(Qt::WA_TransparentForMouseEvents);
    Label* caption = new Label(this);
    caption->setObjectName(QStringLiteral("windowTitleCaption"));
    caption->setText(host->windowTitle());
    caption->setFontSize(m_config.m_fontSize);
    caption->setFontBold(true);
    caption->setAttribute(Qt::WA_TransparentForMouseEvents);
    layout->addWidget(logo);
    layout->addWidget(caption, 1);
    m_actions->setContentsMargins(0, 0, 0, 0);
    m_actions->setSpacing(m_config.m_spacing);
    layout->addLayout(m_actions);
    PushButton* minimize = makeButton(0);
    PushButton* maximize = makeButton(1);
    PushButton* closeButton = makeButton(2);
    layout->addWidget(minimize);
    layout->addWidget(maximize);
    layout->addWidget(closeButton);
    QObject::connect(host, &QWidget::windowTitleChanged, caption, &QLabel::setText);
    QObject::connect(host, &QWidget::windowIconChanged, this,
        /** 更新宿主的实际图标，不持有旧pixmap的借用数据。
        @param [in] value 新图标
        */
        [logo](const QIcon& value)
        {
            logo->setPixmap(value.pixmap(logo->size()));
        });
    QObject::connect(minimize, &QPushButton::clicked, host, &QWidget::showMinimized);
    QObject::connect(maximize, &QPushButton::clicked, host,
        /** 保留最大化及恢复语义，不影响产品动作。 */
        [host]()
        {
            if (host->isMaximized())
            {
                host->showNormal();
            }
            else
            {
                host->showMaximized();
            }
        });
    QObject::connect(closeButton, &QPushButton::clicked, host, &QWidget::close);
    host->setTitleBar(this, m_config.m_resizeBorder);
}

void WindowTitleBar::addActionWidget(QWidget* action)
{
    if (action != nullptr)
    {
        action->setParent(this);
        m_actions->addWidget(action);
    }
}

QIcon WindowTitleBar::icon(int kind) const
{
    QPixmap image(m_config.m_controlIconSize, m_config.m_controlIconSize);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(m_config.m_foreground, m_config.m_controlStroke));
    painter.scale(m_config.m_controlIconSize / 20.0, m_config.m_controlIconSize / 20.0);
    if (kind == 0)
    {
        painter.drawLine(QPointF(4, 10), QPointF(16, 10));
    }
    else if (kind == 1)
    {
        painter.drawRect(QRectF(4.5, 4.5, 11, 11));
    }
    else
    {
        painter.drawLine(QPointF(4, 4), QPointF(16, 16));
        painter.drawLine(QPointF(16, 4), QPointF(4, 16));
    }
    return QIcon(image);
}

PushButton* WindowTitleBar::makeButton(int kind)
{
    PushButton* button = new PushButton(this);
    button->setObjectName(kind == 0 ? QStringLiteral("windowMinimize") :
        kind == 1 ? QStringLiteral("windowMaximize") : QStringLiteral("windowClose"));
    button->setFocusPolicy(Qt::NoFocus);
    button->setClickBreathTime(0);
    button->setFixedSize(m_config.m_buttonSize, m_config.m_buttonSize);
    button->setIcon(icon(kind));
    button->setIconSize(QSize(m_config.m_controlIconSize, m_config.m_controlIconSize));
    const QColor hover = kind == 2 ? m_config.m_closeHover : m_config.m_hover;
    button->setBkgColor(Qt::transparent, hover, hover.darker(105), Qt::transparent);
    button->setBorderColor(Qt::transparent, Qt::transparent, Qt::transparent, Qt::transparent);
    button->setBorderWidth(0);
    button->setBorderRadius(static_cast<quint32>(m_config.m_controlRadius));
    return button;
}

void WindowTitleBar::mousePressEvent(QMouseEvent* event)
{
    if (m_host && event->button() == Qt::LeftButton && !m_host->isFullScreen())
    {
        m_dragging = true;
        m_dragOffset = event->globalPos() - m_host->frameGeometry().topLeft();
        event->accept();
        return;
    }
    Widget::mousePressEvent(event);
}

void WindowTitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && m_host && event->buttons().testFlag(Qt::LeftButton))
    {
        if (m_host->isMaximized())
        {
            const double fraction = static_cast<double>(event->pos().x()) / width();
            m_host->showNormal();
            m_dragOffset = QPoint(static_cast<int>(m_host->width() * fraction), height() / 2);
        }
        m_host->move(event->globalPos() - m_dragOffset);
        event->accept();
        return;
    }
    Widget::mouseMoveEvent(event);
}

void WindowTitleBar::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragging = false;
    Widget::mouseReleaseEvent(event);
}

void WindowTitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
    m_dragging = false;
    if (m_host && event->button() == Qt::LeftButton && !m_host->isFullScreen())
    {
        if (m_host->isMaximized())
        {
            m_host->showNormal();
        }
        else
        {
            m_host->showMaximized();
        }
        event->accept();
        return;
    }
    Widget::mouseDoubleClickEvent(event);
}