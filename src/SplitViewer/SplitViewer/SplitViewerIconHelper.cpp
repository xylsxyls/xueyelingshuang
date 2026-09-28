#include "SplitViewerIconHelper.h"
#include "Config.h"
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

QIcon SplitViewerIconHelper::aboutIcon()
{
    QPixmap pixmap(g_config.m_aboutGlyphCanvas, g_config.m_aboutGlyphCanvas);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(g_config.m_aboutGlyphColor);
    painter.drawEllipse(QRectF(2.0, 2.0, 28.0, 28.0));
    painter.setPen(QPen(Qt::white, g_config.m_aboutGlyphStroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    QPainterPath question;
    question.moveTo(12.0, 12.0);
    question.cubicTo(12.0, 7.2, 20.0, 7.2, 20.0, 12.0);
    question.cubicTo(20.0, 15.2, 16.0, 15.7, 16.0, 19.0);
    painter.drawPath(question);
    painter.drawPoint(QPointF(16.0, 23.5));
    return QIcon(pixmap);
}