#include "LumaPlayerButton.h"
#include "Config.h"
#include <QPainter>
#include <QFontInfo>
#include <QVariant>

LumaPlayerButton::LumaPlayerButton(bool closeButton, QWidget* parent) :
PushButton(parent),
m_closeButton(closeButton)
{
    setProperty("class", QVariant(QStringLiteral("LumaPlayerButton")));
    const QColor normal = closeButton ? g_config.m_closeButtonColor : g_config.m_themeColor;
    const qreal opacity = g_config.m_buttonPressOpacity * g_config.m_buttonPressColor.alphaF();
    const QColor pressed = closeButton ? normal.darker(g_config.m_closePressDarkening) : QColor(
        qRound(normal.red() * (1.0 - opacity) + g_config.m_buttonPressColor.red() * opacity),
        qRound(normal.green() * (1.0 - opacity) + g_config.m_buttonPressColor.green() * opacity),
        qRound(normal.blue() * (1.0 - opacity) + g_config.m_buttonPressColor.blue() * opacity));
    setBkgColor(normal, normal, pressed, normal);
    setFontColor(g_config.m_textColor, g_config.m_textColor, g_config.m_textColor, g_config.m_textColor);
    setBorderRadius(static_cast<quint32>(g_config.m_cornerRadius));
    const QFont textFont(g_config.m_fontFamily, g_config.m_fontSize);
    setFontFace(textFont.family());
    setFontSize(static_cast<quint32>(QFontInfo(textFont).pixelSize()));
    setClickBreathTime(0);
}

void LumaPlayerButton::paintEvent(QPaintEvent* event)
{
    PushButton::paintEvent(event);
    if (m_closeButton)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(g_config.m_iconColor);
        pen.setWidthF(g_config.m_iconStroke);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.scale(width() / g_config.m_iconCanvasSize, height() / g_config.m_iconCanvasSize);
        painter.drawPath(g_config.m_closeIcon);
    }
}