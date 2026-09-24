#include "PdfReaderEmptyState.h"
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
    const qreal side = (std::min<qreal>)(42.0, (std::min<qreal>)(width(), height()) - 8.0);
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
    painter.fillRect(rect, QColor(238, 238, 238));
    painter.setPen(QPen(QColor(96, 96, 96), 1));
    painter.drawRect(rect.adjusted(0.5, 0.5, -0.5, -0.5));
    const QPointF center = rect.center();
    painter.setPen(QPen(QColor(55, 65, 81), 3, Qt::SolidLine, Qt::SquareCap));
    painter.drawLine(QPointF(center.x() - 10.0, center.y()), QPointF(center.x() + 10.0, center.y()));
    painter.drawLine(QPointF(center.x(), center.y() - 10.0), QPointF(center.x(), center.y() + 10.0));
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