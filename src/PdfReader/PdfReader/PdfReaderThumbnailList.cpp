#include "PdfReaderThumbnailList.h"
#include <QTimer>
#include <QScrollBar>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QStyle>
#include "QtControls/ScrollBar.h"

PdfReaderThumbnailList::PdfReaderThumbnailList(QWidget* parent, const Config& config) :
ListWidget(parent),
m_config(config),
m_holdTimer(new QTimer(this)),
m_scrollTimer(new QTimer(this)),
m_sourceRow(-1),
m_dragging(false)
{
    setObjectName(QStringLiteral("thumbnails"));
    setVerticalScrollBar(new ScrollBar(Qt::Vertical, this));
    setHorizontalScrollBar(new ScrollBar(Qt::Horizontal, this));
    // 列表模式让每个条目随侧栏横向铺满；委托再把页图放到条目中心，侧栏拉伸时不会停在旧的窄条目位置。
    setViewMode(QListView::ListMode);
    setFlow(QListView::TopToBottom);
    setWrapping(false);
    setMovement(QListView::Static);
    setResizeMode(QListView::Adjust);
    setDragDropMode(QAbstractItemView::NoDragDrop);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // ListMode 的条目要覆盖视口全宽；间距会给每个条目额外的横向偏移，导致页图在侧栏拉伸后偏离中心。
    setSpacing(0);
    setContextMenuPolicy(Qt::CustomContextMenu);
    m_holdTimer->setSingleShot(true);
    m_holdTimer->setInterval(m_config.dragHoldMs);
    m_scrollTimer->setInterval(m_config.dragScrollMs);
    connect(m_holdTimer, SIGNAL(timeout()), this, SLOT(beginDrag()));
    connect(m_scrollTimer, SIGNAL(timeout()), this, SLOT(autoScrollDrag()));
}

void PdfReaderThumbnailList::mousePressEvent(QMouseEvent* event)
{
    cancelDrag();
    QListWidgetItem* hit = itemAt(event->pos());
    if (hit)
    {
        setCurrentItem(hit, QItemSelectionModel::ClearAndSelect);
    }
    if (event->button() == Qt::LeftButton && hit)
    {
        setFocus();
        m_sourceRow = row(hit);
        m_dragPoint = event->pos();
        m_holdTimer->start();
    }
    event->accept();
}

void PdfReaderThumbnailList::mouseMoveEvent(QMouseEvent* event)
{
    // 不调用 QListWidget 的框选/拖放处理；移动期间只选择按下时的源项。
    m_dragPoint = event->pos();
    viewport()->update();
    event->accept();
}

void PdfReaderThumbnailList::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragPoint = event->pos();
    const int from = m_sourceRow;
    const int gap = insertionRow();
    const int to = gap > from ? gap - 1 : gap;
    const bool move = event->button() == Qt::LeftButton && m_dragging &&
        from >= 0 && to >= 0 && to < count() && from != to &&
        event->pos().x() >= 0 && event->pos().x() < viewport()->width();
    cancelDrag();
    if (move)
    {
        // Core 接受移动后由窗口重建视图，不让 Qt 另行复制或删除 model 行。
        emit reordered(from, to);
    }
    event->accept();
}

void PdfReaderThumbnailList::beginDrag()
{
    if (m_sourceRow >= 0 && m_sourceRow < count())
    {
        m_dragging = true;
        m_scrollTimer->start();
        viewport()->setCursor(Qt::ClosedHandCursor);
        viewport()->update();
    }
}

int PdfReaderThumbnailList::insertionRow() const
{
    for (int i = 0; i < count(); ++i)
    {
        if (m_dragPoint.y() < visualItemRect(item(i)).center().y())
        {
            return i;
        }
    }
    return count();
}

void PdfReaderThumbnailList::cancelDrag()
{
    m_holdTimer->stop();
    m_scrollTimer->stop();
    m_dragging = false;
    m_sourceRow = -1;
    viewport()->unsetCursor();
    viewport()->update();
}

void PdfReaderThumbnailList::autoScrollDrag()
{
    int speed = 0;
    if (m_dragPoint.y() < m_config.dragEdgePixels)
    {
        speed = -qBound(m_config.dragMinSpeed, m_config.dragMinSpeed + (m_config.dragEdgePixels - m_dragPoint.y()) / m_config.dragAcceleration, m_config.dragMaxSpeed);
    }
    else if (m_dragPoint.y() > viewport()->height() - m_config.dragEdgePixels)
    {
        speed = qBound(m_config.dragMinSpeed, m_config.dragMinSpeed + (m_dragPoint.y() - viewport()->height() + 70) / m_config.dragAcceleration, m_config.dragMaxSpeed);
    }
    verticalScrollBar()->setValue(verticalScrollBar()->value() + speed);
    viewport()->update();
}

void PdfReaderThumbnailList::paintEvent(QPaintEvent* event)
{
    ListWidget::paintEvent(event);
    if (m_dragging && m_sourceRow >= 0 && m_sourceRow < count())
    {
        QPainter painter(viewport());
        const QRect source = visualItemRect(item(m_sourceRow));
        painter.fillRect(source.adjusted(m_config.dragGhostInset, m_config.dragGhostInset, -m_config.dragGhostInset, -m_config.dragGhostBottomInset), m_config.dragGhostColor);
        const int gap = insertionRow();
        const int y = gap < count() ? visualItemRect(item(gap)).top() - m_config.dragLineOffset :
            visualItemRect(item(count() - 1)).bottom() + m_config.dragLineOffset;
        painter.setPen(QPen(m_config.dragLineColor, m_config.dragLineWidth));
        painter.drawLine(m_config.dragLineInset, y, viewport()->width() - m_config.dragLineInset, y);
    }
}

void PdfReaderThumbnailList::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
    {
        cancelDrag();
        event->accept();
        return;
    }
    if (m_sourceRow < 0)
    {
        ListWidget::keyPressEvent(event);
    }
}

bool PdfReaderThumbnailList::event(QEvent* event)
{
    if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Hide)
    {
        cancelDrag();
    }
    return ListWidget::event(event);
}
