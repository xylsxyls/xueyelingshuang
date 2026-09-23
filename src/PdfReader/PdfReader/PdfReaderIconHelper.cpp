#include "PdfReaderIconHelper.h"
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>

QIcon PdfReaderIconHelper::aboutIcon()
{
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(43, 125, 214));
    painter.drawEllipse(QRectF(2.0, 2.0, 28.0, 28.0));
    painter.setPen(QPen(Qt::white, 2.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    QPainterPath question;
    question.moveTo(12.0, 12.0);
    question.cubicTo(12.0, 7.2, 20.0, 7.2, 20.0, 12.0);
    question.cubicTo(20.0, 15.2, 16.0, 15.7, 16.0, 19.0);
    painter.drawPath(question);
    painter.drawPoint(QPointF(16.0, 23.5));
    return QIcon(pixmap);
}


QIcon PdfReaderIconHelper::applicationIcon()
{
    return QIcon(QStringLiteral(":/icons/PdfReader.png"));
}