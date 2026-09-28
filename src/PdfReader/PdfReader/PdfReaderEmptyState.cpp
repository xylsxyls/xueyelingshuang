#include "PdfReaderEmptyState.h"
#include "Config.h"
#include <algorithm>
#include <QMouseEvent>
#include <QPainter>

PdfReaderEmptyState::PdfReaderEmptyState(QWidget* parent) :
Widget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::PointingHandCursor);
    setMinimumSize(1, 1);
}

QRectF PdfReaderEmptyState::plusButtonRect() const
{
    const qreal side = (std::min<qreal>)(g_config.m_emptyPlusSize, (std::min<qreal>)(width(), height()) - g_config.m_emptyPlusMargin);
    if (side <= 0.0)
    {
        return QRectF();
    }
    return QRectF((width() - side) / 2.0, (height() - side) / 2.0, side, side);
}

void PdfReaderEmptyState::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    const QRectF rect = plusButtonRect();
    if (rect.isEmpty())
    {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.fillRect(rect, g_config.m_emptyPlusBackground);
    painter.setPen(QPen(g_config.m_emptyPlusBorder, 1));
    painter.drawRect(rect.adjusted(0.5, 0.5, -0.5, -0.5));
    const QPointF center = rect.center();
    painter.setPen(QPen(g_config.m_emptyPlusColor, g_config.m_emptyPlusStroke, Qt::SolidLine, Qt::SquareCap));
    painter.drawLine(QPointF(center.x() - g_config.m_emptyPlusHalfLength, center.y()), QPointF(center.x() + g_config.m_emptyPlusHalfLength, center.y()));
    painter.drawLine(QPointF(center.x(), center.y() - g_config.m_emptyPlusHalfLength), QPointF(center.x(), center.y() + g_config.m_emptyPlusHalfLength));
}

void PdfReaderEmptyState::mousePressEvent(QMouseEvent* event)
{
    if (event != nullptr && event->button() == Qt::LeftButton && plusButtonRect().contains(event->pos()))
    {
        emit clicked();
        event->accept();
        return;
    }
    Widget::mousePressEvent(event);
}