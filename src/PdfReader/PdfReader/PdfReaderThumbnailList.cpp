#include "PdfReaderThumbnailList.h"
#include <QTimer>
#include <QScrollBar>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QStyle>
#include <QWheelEvent>
#include <QAbstractItemModel>
#include "QtControls/ScrollBar.h"

PdfReaderThumbnailList::PdfReaderThumbnailList(QWidget* parent) :
ListWidget(parent),
m_holdTimer(new QTimer(this)),
m_scrollTimer(new QTimer(this)),
m_sourceRow(-1),
m_targetGap(-1),
m_wheelRemainder(0.0),
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
    m_holdTimer->setTimerType(Qt::PreciseTimer);
    m_holdTimer->setInterval(g_config.m_dragHoldMs);
    m_scrollTimer->setInterval(g_config.m_dragScrollMs);
    connect(m_holdTimer, SIGNAL(timeout()), this, SLOT(beginDrag()));
    connect(m_scrollTimer, SIGNAL(timeout()), this, SLOT(autoScrollDrag()));
    connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateDragTarget()));
    connect(model(), SIGNAL(modelAboutToBeReset()), this, SLOT(cancelDrag()));
    connect(model(), SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(cancelDrag()));
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
    if (gestureActive() && !(event->buttons() & Qt::LeftButton))
    {
        cancelDrag();
    }
    if (m_dragPoint != event->pos())
    {
        m_dragPoint = event->pos();
        updateDragTarget();
    }
    event->accept();
}

void PdfReaderThumbnailList::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_dragPoint != event->pos())
    {
        m_dragPoint = event->pos();
        updateDragTarget();
    }
    const int from = m_sourceRow;
    const int gap = m_targetGap;
    const int to = gap > from ? gap - 1 : gap;
    const bool move = event->button() == Qt::LeftButton && m_dragging &&
        gap >= 0 && from >= 0 && to >= 0 && to < count() && from != to &&
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
        // 初始指向源页之前；按住源页下半部也不改变初始反馈。
        m_targetGap = m_sourceRow;
        m_scrollTimer->start();
        viewport()->setCursor(Qt::ClosedHandCursor);
        viewport()->update();
    }
}

int PdfReaderThumbnailList::insertionRow() const
{
    // 纵向移出视口时仍以可见边缘确定落点，继续自动滚动而不跳到隐藏的远页。
    const int pointerY = qBound(0, m_dragPoint.y(), qMax(0, viewport()->height() - 1));
    for (int i = 0; i < count(); ++i)
    {
        if (pointerY < visualItemRect(item(i)).center().y())
        {
            return i == m_sourceRow + 1 ? m_sourceRow : i;
        }
    }
    return count() == m_sourceRow + 1 ? m_sourceRow : count();
}

void PdfReaderThumbnailList::cancelDrag()
{
    m_holdTimer->stop();
    m_scrollTimer->stop();
    m_dragging = false;
    m_sourceRow = -1;
    m_targetGap = -1;
    m_wheelRemainder = 0.0;
    viewport()->unsetCursor();
    viewport()->update();
}

void PdfReaderThumbnailList::autoScrollDrag()
{
    if (!m_dragging || m_dragPoint.x() < 0 || m_dragPoint.x() >= viewport()->width())
    {
        return;
    }
    int speed = 0;
    if (m_dragPoint.y() < g_config.m_dragEdgePixels)
    {
        speed = -qBound(g_config.m_dragMinSpeed, g_config.m_dragMinSpeed + (g_config.m_dragEdgePixels - m_dragPoint.y()) / g_config.m_dragAcceleration, g_config.m_dragMaxSpeed);
    }
    else if (m_dragPoint.y() > viewport()->height() - g_config.m_dragEdgePixels)
    {
        speed = qBound(g_config.m_dragMinSpeed, g_config.m_dragMinSpeed + (m_dragPoint.y() - viewport()->height() + g_config.m_dragEdgePixels) / g_config.m_dragAcceleration, g_config.m_dragMaxSpeed);
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
        painter.fillRect(source.adjusted(g_config.m_dragGhostInset, g_config.m_dragGhostInset, -g_config.m_dragGhostInset, -g_config.m_dragGhostBottomInset), g_config.m_dragGhostColor);
        if (m_targetGap >= 0 && m_targetGap <= count() && viewport()->height() > g_config.m_dragLineWidth)
        {
            const int desired = m_targetGap < count() ? visualItemRect(item(m_targetGap)).top() - g_config.m_dragLineOffset :
                visualItemRect(item(count() - 1)).bottom() + g_config.m_dragLineOffset;
            // 包括首前和尾后间隙，完整笔画都要保留在视口内。
            const int half = (g_config.m_dragLineWidth + 1) / 2;
            const int y = qBound(half, desired, viewport()->height() - 1 - half);
            const int inset = qMin(g_config.m_dragLineInset, viewport()->width() / 2);
            painter.setPen(QPen(g_config.m_dragLineColor, g_config.m_dragLineWidth));
            painter.drawLine(inset, y, viewport()->width() - 1 - inset, y);
        }
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
    if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::Hide || event->type() == QEvent::UngrabMouse)
    {
        cancelDrag();
    }
    return ListWidget::event(event);
}

bool PdfReaderThumbnailList::gestureActive() const
{
    return m_sourceRow >= 0;
}

void PdfReaderThumbnailList::updateDragTarget()
{
    if (m_dragging)
    {
        m_targetGap = m_dragPoint.x() >= 0 && m_dragPoint.x() < viewport()->width() ? insertionRow() : -1;
    }
    viewport()->update();
}

void PdfReaderThumbnailList::wheelEvent(QWheelEvent* event)
{
    if (!gestureActive())
    {
        ListWidget::wheelEvent(event);
        return;
    }
    m_dragPoint = event->pos();
    // Qt标准刻度为120角度单位，像素级设备优先采用直接像素增量。
    const double delta = event->pixelDelta().y() != 0 ? event->pixelDelta().y() :
        event->angleDelta().y() / 120.0 * g_config.m_dragWheelPixels;
    m_wheelRemainder += delta;
    const int pixels = qRound(m_wheelRemainder);
    m_wheelRemainder -= pixels;
    verticalScrollBar()->setValue(verticalScrollBar()->value() - pixels);
    updateDragTarget();
    event->accept();
}

void PdfReaderThumbnailList::scrollContentsBy(int dx, int dy)
{
    ListWidget::scrollContentsBy(dx, dy);
    updateDragTarget();
}