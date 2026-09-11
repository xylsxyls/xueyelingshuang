#include "LumaPlayerButton.h"
#include "Config.h"
#include <QPainter>

LumaPlayerButton::LumaPlayerButton(bool closeButton, QWidget* parent) :
QPushButton(parent),
m_closeButton(closeButton)
{

}

void LumaPlayerButton::paintEvent(QPaintEvent* event)
{
    (void)event;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    const QColor background = m_closeButton ?
        (isDown() ? g_config.m_closeButtonColor.darker(g_config.m_closePressDarkening) : g_config.m_closeButtonColor) :
        g_config.m_themeColor;
    painter.setBrush(background);
    painter.drawRoundedRect(rect(), g_config.m_cornerRadius, g_config.m_cornerRadius);
    if (isDown() && !m_closeButton)
    {
        painter.setOpacity(g_config.m_buttonPressOpacity);
        painter.setBrush(g_config.m_buttonPressColor);
        painter.drawRoundedRect(rect(), g_config.m_cornerRadius, g_config.m_cornerRadius);
        painter.setOpacity(1.0);
    }
    if (m_closeButton)
    {
        QPen pen(g_config.m_iconColor);
        pen.setWidthF(g_config.m_iconStroke);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.scale(width() / g_config.m_iconCanvasSize, height() / g_config.m_iconCanvasSize);
        painter.drawPath(g_config.m_closeIcon);
    }
    else
    {
        painter.setPen(g_config.m_textColor);
        painter.setFont(font());
        painter.drawText(rect(), Qt::AlignCenter, text());
    }
}