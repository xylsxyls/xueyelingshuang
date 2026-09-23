#include "SplitViewerCanvas.h"

#include "SplitViewer.h"

#include <QtCore/QMimeData>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QWheelEvent>

SplitViewerCanvas::SplitViewerCanvas(SplitViewer* owner, QWidget* parent) : Widget(parent), m_owner(owner)
{
    setAcceptDrops(true);
    setMouseTracking(true);
    setMinimumSize(320, 220);
    setAutoFillBackground(false);
}

void SplitViewerCanvas::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    if (m_owner)
    {
        m_owner->paintCanvas(painter, size());
    }
}

void SplitViewerCanvas::mousePressEvent(QMouseEvent* event)
{
    if (m_owner)
    {
        m_owner->canvasMousePress(event->pos(), event->button(), event->modifiers());
    }
}

void SplitViewerCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (m_owner)
    {
        m_owner->canvasMouseMove(event->pos(), event->buttons(), event->modifiers());
    }
}

void SplitViewerCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_owner)
    {
        m_owner->canvasMouseRelease(event->pos(), event->button(), event->modifiers());
    }
}

void SplitViewerCanvas::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (m_owner && event->button() == Qt::LeftButton)
    {
        m_owner->canvasDoubleClick(event->pos());
    }
    event->accept();
}

void SplitViewerCanvas::wheelEvent(QWheelEvent* event)
{
    if (m_owner)
    {
        m_owner->canvasWheel(event->pos(), event->angleDelta().y(), event->modifiers());
    }
    event->accept();
}

void SplitViewerCanvas::contextMenuEvent(QContextMenuEvent* event)
{
    if (m_owner)
    {
        m_owner->canvasContextMenu(event->pos(), event->globalPos());
    }
    event->accept();
}

void SplitViewerCanvas::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void SplitViewerCanvas::dropEvent(QDropEvent* event)
{
    if (m_owner && event->mimeData()->hasUrls() && !event->mimeData()->urls().isEmpty())
    {
        m_owner->canvasFileDroppedAt(event->mimeData()->urls().first().toLocalFile(),event->pos());
        event->acceptProposedAction();
    }
}

void SplitViewerCanvas::resizeEvent(QResizeEvent* event)
{
    Widget::resizeEvent(event);
    if (m_owner) m_owner->canvasResized();
}