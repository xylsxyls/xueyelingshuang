#include "SplitViewer.h"
#include "SplitViewerTaskManager.h"
#include "SplitViewerDocumentSnapshot.h"
#include "SplitViewerCanvas.h"
#include "Config.h"
#include "SplitViewerDialogHelper.h"
#include "SplitViewerAboutDialogParam.h"
#include "SplitViewerLayoutHelper.h"
#include "SplitViewerHitTestHelper.h"
#include "SplitViewerImageHelper.h"
#include "SplitViewerIconHelper.h"
#include "SplitViewerRenderer.h"
#include "SplitViewerProfileStore.h"
#include "SplitViewerPlatform.h"
#include "SplitViewerMediaManager.h"
#include "CStringManager/CStringManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "QtControls/Label.h"
#include "QtControls/Menu.h"
#include "QtControls/StatusBar.h"
#include "QtControls/ToolBar.h"
#include "QtControls/Widget.h"
#include <QtCore/QBuffer>
#include <QtCore/QSaveFile>
#include <QtCore/QScopedValueRollback>
#include <QtCore/QPointer>
#include <QtCore/QByteArray>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtGui/QImageReader>
#include <QtGui/QImageWriter>
#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QStyle>
#include <QtGui/QCloseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <algorithm>
#include <cmath>
#include <vector>

SplitViewer::SplitViewer(QWidget* parent) :
MainWindow(parent),
m_canvas(nullptr),
m_toolbar(nullptr),
m_saveProfileAction(nullptr),
m_statusLabel(nullptr),
m_document(g_config.m_coreConfig),
m_documentRevision(0),
m_resumeCanvasFocus(false),
m_closing(false),
m_taskManager(new SplitViewerTaskManager(this)),
m_mediaManager(nullptr),
m_exporting(false),
m_contentFullscreen(false),
m_wasMaximized(false),
m_zoomTip(nullptr),
m_zoomTipTimer(new QTimer(this)),
m_embeddedSyncTimer(new QTimer(this)),
m_draggingLayer(false),
m_resizingLayer(false),
m_resizeEdges(0),
m_draggingImage(false),
m_dragStart(),
m_dragStartRect(),
m_activeHit(),
m_profilePath(),
m_nativeDragWindow(0),
m_nativeDragNode(nullptr),
m_nativeClickTime(0)
{
    buildUi();
    if (!m_taskManager->init())
    {
        LOGERROR("Window task queues could not be initialized");
    }
    m_mediaManager =
        new SplitViewerMediaManager(&m_document, &m_imageCache, &m_embedded, m_canvas, m_taskManager, this);
    connect(m_mediaManager, &SplitViewerMediaManager::changed, this,
        /** 媒体状态改变后在GUI线程同步嵌入布局、状态栏及画布
        */
        [this]()
        {
            syncEmbeddedViews();
            updateStatus();
            m_canvas->update();
        });
    connect(m_mediaManager, &SplitViewerMediaManager::error, this,
        /** 仅在宿主未关闭时显示媒体操作错误
        @param [in] message 本次媒体错误正文，回调期间借用
        */
        [this](const QString& message)
        {
            if (!m_closing)
            {
                reportError(message);
            }
        });
    connect(m_mediaManager, &SplitViewerMediaManager::idle, this,
        /** 媒体请求收敛后安排下一轮GUI关闭检查，避免在当前信号栈中销毁窗口
        */
        [this]()
        {
            if (m_closing)
            {
                QTimer::singleShot(0, this,
                    /** 回到GUI事件循环后重新尝试关闭宿主，由closeEvent核验收尾条件
                    */
                    [this]()
                    {
                        close();
                    });
            }
        });
    QTimer* closePoll = new QTimer(this);
    closePoll->setInterval(g_config.m_taskPollIntervalMs);
    connect(closePoll, &QTimer::timeout, this,
        /** 轮询任务完成和媒体收尾，全部收敛后才再次请求窗口关闭
        */
        [this]()
        {
            updateProcessing();
            if (m_closing && !processing() && !m_mediaManager->busy())
            {
                close();
            }
        });
    closePoll->start();
    m_embeddedSyncTimer->setInterval((std::max)(1, g_config.m_embeddedSyncIntervalMs));
    connect(m_embeddedSyncTimer, &QTimer::timeout, this,
        /** 静置期间保持原生窗口填满分屏，关闭中不再同步几何
        */
        [this]()
        {
            if (!m_closing && !m_embedded.isEmpty())
            {
                syncEmbeddedViews();
            }
        });
    m_embeddedSyncTimer->start();
    m_zoomTipTimer->setSingleShot(true);
    m_zoomTipTimer->setTimerType(Qt::PreciseTimer);
    m_zoomTipTimer->setInterval(g_config.m_zoomTipDurationMs);
    connect(m_zoomTipTimer, &QTimer::timeout, m_zoomTip, &QWidget::hide);
    SplitViewerWatchNativeMouse(this,
        /** 将原生鼠标通知交给所属窗口的GUI输入入口
        @param [in] event 原生点击、拖动或释放事件编号
        @param [in] point 事件发生时的屏幕坐标
        @param [in] id 借用的外部窗口句柄
        */
        [this](int event, const QPoint& point, WId id)
        {
            nativeMouseEvent(event, point, id);
        });
}

SplitViewer::~SplitViewer()
{
    m_embeddedSyncTimer->stop();
    SplitViewerUnwatchNativeMouse(this);
    delete m_taskManager;
    m_taskManager = nullptr;
    delete m_mediaManager;
    m_mediaManager = nullptr;
}

void SplitViewer::buildUi()
{
    setWindowTitle(g_config.m_applicationTitle);
    const QRect screen = QApplication::desktop()->screenGeometry(this);
    const int stageWidth = (std::max)(1, qRound(screen.width() * g_config.m_initialScreenFraction));
    const int stageHeight = (std::max)(1, qRound(screen.height() * g_config.m_initialScreenFraction));
    m_document.setStageAspect(static_cast<double>(screen.width()) / static_cast<double>(screen.height()));
    resize(stageWidth + g_config.m_stageMargin * 2,
        stageHeight + g_config.m_toolbarHeightAllowance + g_config.m_stageMargin * 2);
    setAcceptDrops(true);

    m_toolbar = new ToolBar(this);
    m_toolbar->setWindowTitle(g_config.m_toolbarTitle);
    addToolBar(m_toolbar);
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(g_config.m_toolbarIconSize, g_config.m_toolbarIconSize));
    m_saveProfileAction = m_toolbar->addAction(g_config.m_openProfileText);
    m_saveProfileAction->setToolTip(g_config.m_profileToolTip);
    connect(m_saveProfileAction, SIGNAL(triggered()), this, SLOT(saveProfile()));
    QAction* exportAction = m_toolbar->addAction(g_config.m_saveImageText);
    exportAction->setToolTip(g_config.m_saveImageToolTip);
    connect(exportAction, SIGNAL(triggered()), this, SLOT(saveImage()));
    m_toolbar->addSeparator();
    QAction* layerAction = m_toolbar->addAction(g_config.m_newLayerText);
    connect(layerAction, SIGNAL(triggered()), this, SLOT(newLayer()));
    QAction* fullAction = m_toolbar->addAction(g_config.m_fullscreenText);
    addAction(fullAction);
    fullAction->setShortcut(QKeySequence(Qt::Key_F11));
    connect(fullAction, SIGNAL(triggered()), this, SLOT(toggleFullscreen()));
    QAction* borderAction = new QAction(g_config.m_borderText, this);
    addAction(borderAction);
    borderAction->setShortcut(QKeySequence(Qt::Key_F1));
    connect(borderAction, SIGNAL(triggered()), this, SLOT(toggleBorder()));
    QAction* escapeAction = new QAction(this);
    escapeAction->setShortcut(QKeySequence(Qt::Key_Escape));
    escapeAction->setShortcutContext(Qt::ApplicationShortcut);
    addAction(escapeAction);
    connect(escapeAction, SIGNAL(triggered()), this, SLOT(leaveFullscreen()));
    Widget* spacer = new Widget(m_toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(spacer);
    QAction* aboutAction = m_toolbar->addAction(SplitViewerIconHelper::aboutIcon(), QString());
    aboutAction->setObjectName(QStringLiteral("aboutAction"));
    aboutAction->setToolTip(g_config.m_aboutText);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(showAbout()));

    m_canvas = new SplitViewerCanvas(this, this);
    setCentralWidget(m_canvas);
    m_statusLabel = new Label(this);
    m_statusLabel->setMinimumWidth(g_config.m_statusMinimumWidth);
    statusBar()->addPermanentWidget(m_statusLabel);
    statusBar()->hide();
    m_canvas->setObjectName(QStringLiteral("SplitViewerCanvas"));
    m_zoomTip = new Label(m_canvas);
    m_zoomTip->setObjectName(QStringLiteral("zoomTip"));
    m_zoomTip->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_zoomTip->hide();
    setStyleSheet(g_config.m_windowStyle);
    const QRect desktop = QApplication::desktop()->availableGeometry(this);
    move(desktop.center() - QPoint(width() / 2, height() / 2));
    canvasResized();
    updateStatus();
}

QRectF SplitViewer::stageRect(const QSize& size) const
{
    return SplitViewerLayoutHelper::stageRect(size, m_document.stageAspect(), m_contentFullscreen);
}

QRectF SplitViewer::normalizedToPixel(const SplitViewerCoreRect& rect, const QRectF& stage) const
{
    return SplitViewerLayoutHelper::normalizedToPixel(rect, stage);
}

bool SplitViewer::hasContent() const
{
    return !m_profilePath.isEmpty() || m_document.layerCount() > 0 ||
        SplitViewerCoreNode::hasContent(m_document.baseRoot());
}

QRectF SplitViewer::plusButtonRect(const QRectF& rect) const
{
    return SplitViewerLayoutHelper::plusButtonRect(rect);
}

void SplitViewer::paintCanvas(QPainter& painter, const QSize& size)
{
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(
        QRect(QPoint(0, 0), size), m_contentFullscreen ? g_config.m_fullscreenColor : g_config.m_canvasColor);
    SplitViewerRenderer renderer(
        m_document, m_imageCache, m_embedded, m_canvas->mapFromGlobal(QCursor::pos()), false, false);
    renderer.drawStage(painter, stageRect(size));
    if (m_nativePreview.isValid())
    {
        painter.setPen(QPen(g_config.m_previewColor, g_config.m_previewStroke));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(m_nativePreview.adjusted(
            g_config.m_previewInset, g_config.m_previewInset, -g_config.m_previewInset, -g_config.m_previewInset));
    }
}

QRectF SplitViewer::contentRect(const QRectF& owner) const
{
    return SplitViewerLayoutHelper::contentRect(owner, m_document.borderVisible());
}

void SplitViewer::nodeRects(
    const QRectF& owner, SplitViewerCoreNode* node, QRectF& first, QRectF& splitter, QRectF& second) const
{
    SplitViewerLayoutHelper::nodeRects(owner, node, first, splitter, second, m_document.borderVisible());
}

bool SplitViewer::hitAll(const QPointF& point, SplitViewerHit& hit) const
{
    return SplitViewerHitTestHelper::hitAll(
        m_document, stageRect(m_canvas ? m_canvas->size() : QSize(1, 1)), point, hit);
}

void SplitViewer::setSelectedHit(const SplitViewerHit& hit)
{
    m_activeHit = hit;
    m_document.setSelectedLayer(hit.m_layer);
    updateStatus();
    m_canvas->update();
}

void SplitViewer::canvasMousePress(const QPoint& point, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
    if (button != Qt::LeftButton)
    {
        return;
    }
    SplitViewerHit hit;
    if (!hitAll(point, hit))
    {
        return;
    }
    clearInteraction();
    setSelectedHit(hit);
    if (!(modifiers & Qt::ControlModifier) && !hit.m_splitter && hit.m_node && hit.m_node->isLeaf() &&
        !hit.m_node->view.hasImage && !m_embedded.contains(hit.m_node) && !m_mediaManager->pending(hit.m_node) &&
        plusButtonRect(hit.m_rect).contains(point))
    {
        const QPointer<SplitViewer> owner(this);
        const quint64 revision = m_documentRevision;
        const QString path =
            browseFile(false, m_mediaManager->selectionTitle(), QString(), m_mediaManager->fileFilter());
        if (owner && !m_closing && revision == m_documentRevision && !path.isEmpty())
        {
            loadImageToLeaf(hit.m_node, path);
        }
        return;
    }
    m_dragStart = point;
    m_dragStartRect = hit.m_rect;
    m_resizeEdges = 0;
    m_resizingLayer = false;
    bool moveFromTopLeft = false;
    if (hit.m_layer >= 0)
    {
        const qreal edge = g_config.m_layerEdgeHitWidth;
        moveFromTopLeft =
            std::abs(point.x() - hit.m_layerRect.left()) <= edge && std::abs(point.y() - hit.m_layerRect.top()) <= edge;
        if (!moveFromTopLeft)
        {
            if (std::abs(point.x() - hit.m_layerRect.left()) <= edge)
            {
                m_resizeEdges |= 1;
            }
            if (std::abs(point.x() - hit.m_layerRect.right()) <= edge)
            {
                m_resizeEdges |= 2;
            }
            if (std::abs(point.y() - hit.m_layerRect.top()) <= edge)
            {
                m_resizeEdges |= 4;
            }
            if (std::abs(point.y() - hit.m_layerRect.bottom()) <= edge)
            {
                m_resizeEdges |= 8;
            }
            m_resizingLayer = m_resizeEdges != 0;
        }
    }
    m_draggingImage = !moveFromTopLeft && !hit.m_splitter && hit.m_node && hit.m_node->isLeaf() &&
        hit.m_node->view.hasImage && !m_resizingLayer && !(hit.m_layer >= 0 && (modifiers & Qt::ControlModifier));
    m_draggingLayer = hit.m_layer >= 0 && !m_resizingLayer && (moveFromTopLeft || (modifiers & Qt::ControlModifier));
}

void SplitViewer::canvasMouseMove(const QPoint& point, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(modifiers);
    if (!(buttons & Qt::LeftButton))
    {
        SplitViewerHit hover;
        Qt::CursorShape cursor = Qt::ArrowCursor;
        if (hitAll(point, hover))
        {
            const bool nearLeft =
                hover.m_layer >= 0 && std::abs(point.x() - hover.m_layerRect.left()) <= g_config.m_layerEdgeHitWidth;
            const bool nearTop =
                hover.m_layer >= 0 && std::abs(point.y() - hover.m_layerRect.top()) <= g_config.m_layerEdgeHitWidth;
            const bool nearRight =
                hover.m_layer >= 0 && std::abs(point.x() - hover.m_layerRect.right()) <= g_config.m_layerEdgeHitWidth;
            const bool nearBottom =
                hover.m_layer >= 0 && std::abs(point.y() - hover.m_layerRect.bottom()) <= g_config.m_layerEdgeHitWidth;
            const bool topLeftMove = nearLeft && nearTop;
            const bool x = nearLeft || nearRight;
            const bool y = nearTop || nearBottom;
            if (topLeftMove)
            {
                cursor = Qt::SizeAllCursor;
            }
            else if (x && y)
            {
                cursor = ((point.x() < hover.m_layerRect.center().x()) == (point.y() < hover.m_layerRect.center().y()))
                    ? Qt::SizeFDiagCursor
                    : Qt::SizeBDiagCursor;
            }
            else if (x)
            {
                cursor = Qt::SizeHorCursor;
            }
            else if (y)
            {
                cursor = Qt::SizeVerCursor;
            }
            else if (hover.m_layer >= 0 && (modifiers & Qt::ControlModifier))
            {
                cursor = Qt::SizeAllCursor;
            }
            else if (hover.m_splitter)
            {
                cursor =
                    hover.m_node->direction == SPLITVIEWER_CORE_SPLIT_VERTICAL ? Qt::SizeHorCursor : Qt::SizeVerCursor;
            }
            else if (!hover.m_node->view.hasContent() && plusButtonRect(hover.m_rect).contains(point))
            {
                cursor = Qt::PointingHandCursor;
            }
        }
        m_canvas->setCursor(cursor);
        m_canvas->update();
        return;
    }
    if (!m_resizingLayer && !m_draggingLayer && m_activeHit.m_splitter && m_activeHit.m_node)
    {
        const QRectF owner = m_activeHit.m_ownerRect;
        const bool vertical = m_activeHit.m_node->direction == SPLITVIEWER_CORE_SPLIT_VERTICAL;
        const double length = vertical ? owner.width() : owner.height();
        const double thickness = m_document.borderVisible() ? g_config.m_splitterWidth : 0.0;
        const double ratio = ((vertical ? point.x() - owner.left() : point.y() - owner.top()) - thickness / 2.0) /
            (std::max)(1.0, length - thickness);
        const double minimum = (std::min)(0.5, g_config.m_minimumPaneSize / (std::max)(1.0, length - thickness));
        m_activeHit.m_node->ratio = (std::max)(minimum, (std::min)(1.0 - minimum, ratio));
        m_canvas->update();
        return;
    }
    if (m_draggingImage && m_activeHit.m_node)
    {
        if (m_activeHit.m_node->view.autoFit)
        {
            const QString imagePath = SplitViewerImageHelper::path(m_activeHit.m_node->view.path);
            if (!m_imageCache.contains(imagePath))
            {
                m_imageCache.insert(imagePath, QImage(imagePath));
            }
            const QImage image = m_imageCache.value(imagePath);
            if (!image.isNull())
            {
                m_activeHit.m_node->view.scale = SplitViewerCoreFitScale(
                    image.width(), image.height(), m_activeHit.m_rect.width(), m_activeHit.m_rect.height());
            }
            m_activeHit.m_node->view.autoFit = false;
        }
        const QPoint delta = point - m_dragStart;
        m_activeHit.m_node->view.offsetX += delta.x();
        m_activeHit.m_node->view.offsetY += delta.y();
        m_dragStart = point;
        m_canvas->update();
        return;
    }
    if (m_draggingLayer && m_activeHit.m_layer >= 0 && m_activeHit.m_layer < m_document.layerCount())
    {
        SplitViewerCoreLayer* layer = m_document.layerAt(m_activeHit.m_layer);
        if (layer)
        {
            const QRectF stage = stageRect(m_canvas->size());
            const QPoint delta = point - m_dragStart;
            const double dx = delta.x() / (std::max)(1.0, stage.width());
            const double dy = delta.y() / (std::max)(1.0, stage.height());
            layer->rect.left += dx;
            layer->rect.right += dx;
            layer->rect.top += dy;
            layer->rect.bottom += dy;
            SplitViewerCoreRect bounds(0.0, 0.0, 1.0, 1.0);
            SplitViewerCoreConstrainLayerRect(layer->rect, bounds, g_config.m_minimumLayerWidth / stage.width(),
                g_config.m_minimumLayerHeight / stage.height());
            m_dragStart = point;
            m_canvas->update();
        }
    }
    if (m_resizingLayer && m_activeHit.m_layer >= 0 && m_activeHit.m_layer < m_document.layerCount())
    {
        SplitViewerCoreLayer* layer = m_document.layerAt(m_activeHit.m_layer);
        if (layer)
        {
            const QRectF stage = stageRect(m_canvas->size());
            const QPoint delta = point - m_dragStart;
            const double dx = delta.x() / (std::max)(1.0, stage.width());
            const double dy = delta.y() / (std::max)(1.0, stage.height());
            if (m_resizeEdges & 1)
            {
                layer->rect.left = (std::max)(0.0,
                    (std::min)(layer->rect.right - g_config.m_minimumLayerWidth / stage.width(),
                        layer->rect.left + dx));
            }
            if (m_resizeEdges & 2)
            {
                layer->rect.right = (std::min)(1.0,
                    (std::max)(layer->rect.left + g_config.m_minimumLayerWidth / stage.width(),
                        layer->rect.right + dx));
            }
            if (m_resizeEdges & 4)
            {
                layer->rect.top = (std::max)(0.0,
                    (std::min)(layer->rect.bottom - g_config.m_minimumLayerHeight / stage.height(),
                        layer->rect.top + dy));
            }
            if (m_resizeEdges & 8)
            {
                layer->rect.bottom = (std::min)(1.0,
                    (std::max)(layer->rect.top + g_config.m_minimumLayerHeight / stage.height(),
                        layer->rect.bottom + dy));
            }
            m_dragStart = point;
            m_canvas->update();
        }
    }
}

void SplitViewer::canvasMouseRelease(const QPoint& point, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(point);
    Q_UNUSED(modifiers);
    if (button == Qt::LeftButton)
    {
        m_draggingLayer = false;
        m_resizingLayer = false;
        m_resizeEdges = 0;
        m_draggingImage = false;
    }
}

void SplitViewer::canvasWheel(const QPoint& point, int delta, Qt::KeyboardModifiers modifiers)
{
    SplitViewerHit hit;
    if (delta == 0 || !hitAll(point, hit) || hit.m_splitter || !hit.m_node || !hit.m_node->view.hasImage)
    {
        return;
    }
    const QImage image = m_imageCache.value(SplitViewerImageHelper::path(hit.m_node->view.path));
    if (image.isNull())
    {
        return;
    }
    const double fit = SplitViewerCoreFitScale(image.width(), image.height(), hit.m_rect.width(), hit.m_rect.height());
    SplitViewerCoreZoom(hit.m_node->view, fit, delta, (modifiers & Qt::ControlModifier) != 0, m_document.config());
    setSelectedHit(hit);
    m_zoomTip->setText(QString::number(hit.m_node->view.scale / fit * 100.0, 'f', 0) + QStringLiteral("%"));
    m_zoomTip->adjustSize();
    m_zoomTip->move(
        (std::max)(0, point.x() - m_zoomTip->width() / 2), (std::max)(0, point.y() - g_config.m_zoomTipOffset));
    m_zoomTip->show();
    m_zoomTip->raise();
    m_zoomTipTimer->start();
    m_canvas->update();
}

void SplitViewer::canvasContextMenu(const QPoint& point, const QPoint& globalPoint)
{
    SplitViewerHit hit;
    if (!hitAll(point, hit) || hit.m_splitter)
    {
        return;
    }
    setSelectedHit(hit);
    const QPointer<SplitViewer> owner(this);
    const quint64 revision = m_documentRevision;
    Menu menu(this);
    // 栈菜单不能在宿主的QObject析构中被删除；先解除父所有权并退出exec。
    connect(this, &QObject::destroyed, &menu,
        /** 宿主销毁时取消菜单的父对象所有权并退出嵌套菜单循环
        */
        [&menu]()
        {
            menu.setParent(nullptr);
            menu.close();
        });
    QAction* horizontal = menu.addAction(g_config.m_horizontalSplitText);
    QAction* vertical = menu.addAction(g_config.m_verticalSplitText);
    QAction* deleteLayerAction = menu.addAction(g_config.m_deleteLayerText);
    deleteLayerAction->setEnabled(hit.m_layer >= 0);
    QAction* deleteAction = menu.addAction(g_config.m_deleteSplitText);
    deleteAction->setEnabled(hit.m_node != hit.m_root);
    menu.addSeparator();
    QAction* moveLeft = menu.addAction(g_config.m_moveLeftText);
    QAction* moveRight = menu.addAction(g_config.m_moveRightText);
    QAction* moveUp = menu.addAction(g_config.m_moveUpText);
    QAction* moveDown = menu.addAction(g_config.m_moveDownText);
    const bool movable = hit.m_node && hit.m_node->isLeaf() && hit.m_node->view.hasContent();
    moveLeft->setEnabled(
        movable && SplitViewerCoreDirectionalTarget(hit.m_root, hit.m_node, SPLITVIEWER_CORE_MOVE_LEFT));
    moveRight->setEnabled(
        movable && SplitViewerCoreDirectionalTarget(hit.m_root, hit.m_node, SPLITVIEWER_CORE_MOVE_RIGHT));
    moveUp->setEnabled(movable && SplitViewerCoreDirectionalTarget(hit.m_root, hit.m_node, SPLITVIEWER_CORE_MOVE_UP));
    moveDown->setEnabled(
        movable && SplitViewerCoreDirectionalTarget(hit.m_root, hit.m_node, SPLITVIEWER_CORE_MOVE_DOWN));
    menu.addSeparator();
    QAction* load = menu.addAction(m_mediaManager->loadActionText());
    QAction* embed = menu.addAction(g_config.m_embedWindowText);
    QAction* detach = menu.addAction(g_config.m_detachWindowText);
    detach->setEnabled(m_embedded.contains(hit.m_node) || m_mediaManager->pending(hit.m_node) ||
        hit.m_node->view.contentKind == SPLITVIEWER_CORE_CONTENT_VIDEO);
    QAction* chosen = menu.exec(globalPoint);
    if (!owner || m_closing || revision != m_documentRevision)
    {
        return;
    }
    if (chosen == embed)
    {
        embedExternalWindow();
        return;
    }
    if (chosen == detach)
    {
        detachExternalWindow();
        return;
    }
    if (chosen == moveLeft || chosen == moveRight || chosen == moveUp || chosen == moveDown)
    {
        const SplitViewerCoreMoveDirection direction = chosen == moveLeft ? SPLITVIEWER_CORE_MOVE_LEFT
            : chosen == moveRight                                         ? SPLITVIEWER_CORE_MOVE_RIGHT
            : chosen == moveUp                                            ? SPLITVIEWER_CORE_MOVE_UP
                                                                          : SPLITVIEWER_CORE_MOVE_DOWN;
        moveAt(hit, direction);
    }
    else if (chosen == horizontal)
    {
        splitAt(hit, SPLITVIEWER_CORE_SPLIT_HORIZONTAL);
    }
    else if (chosen == vertical)
    {
        splitAt(hit, SPLITVIEWER_CORE_SPLIT_VERTICAL);
    }
    else if (chosen == deleteAction)
    {
        deleteAt(hit);
    }
    else if (chosen == deleteLayerAction)
    {
        deleteLayerAt(hit.m_layer);
    }
    else if (chosen == load && hit.m_node)
    {
        const QString path =
            browseFile(false, m_mediaManager->selectionTitle(), QString(), m_mediaManager->fileFilter());
        if (owner && !m_closing && revision == m_documentRevision && !path.isEmpty())
        {
            loadImageToLeaf(hit.m_node, path);
        }
    }
}

void SplitViewer::moveAt(const SplitViewerHit& hit, SplitViewerCoreMoveDirection direction)
{
    SplitViewerCoreNode* target = SplitViewerCoreDirectionalTarget(hit.m_root, hit.m_node, direction);
    if (!target || !hit.m_node->view.hasContent())
    {
        return;
    }
    const QPointer<SplitViewer> owner(this);
    const quint64 documentRevision = m_documentRevision;
    const quint64 mediaRevision = m_mediaManager->revision();
    if (target->view.hasContent() || m_mediaManager->pending(target))
    {
        AskDialogParam confirm;
        confirm.m_parent = windowHandle();
        confirm.m_title = g_config.m_moveOverwriteTitle;
        confirm.m_tip = g_config.m_moveOverwritePrompt;
        DialogManager::instance().makeDialog(confirm);
        if (!owner || m_closing || documentRevision != m_documentRevision ||
            mediaRevision != m_mediaManager->revision() || confirm.m_result != ACCEPT_BUTTON)
        {
            return;
        }
    }
    const QRectF stage = stageRect(m_canvas->size());
    SplitViewerCoreLayer* layer = hit.m_layer >= 0 ? m_document.layerAt(hit.m_layer) : nullptr;
    const QRectF rootRect = contentRect(layer ? normalizedToPixel(layer->rect, stage) : stage);
    QRectF sourceRect, targetRect;
    if (!SplitViewerLayoutHelper::findLeafRect(
            hit.m_root, rootRect, hit.m_node, sourceRect, m_document.borderVisible()) ||
        !SplitViewerLayoutHelper::findLeafRect(hit.m_root, rootRect, target, targetRect, m_document.borderVisible()))
    {
        return;
    }
    const SplitViewerCoreNode* source = hit.m_node;
    const SplitViewerCoreNode* root = hit.m_root;
    submitDocumentEdit(
        /** 仅在独立文档快照中迁移内容并更新旧节点到新节点的资源映射
        @param [in,out] snapshot 待提交的独占快照，成功后由GUI接纳
        @return 内容迁移实际成功；失败不提交该快照
        */
        [source, target, root, sourceRect, targetRect](SplitViewerDocumentSnapshot& snapshot)
        {
            SplitViewerCoreNode* from = snapshot.m_nodes.at(source);
            SplitViewerCoreNode* to = snapshot.m_nodes.at(target);
            if (!SplitViewerCoreMoveContent(snapshot.m_nodes.at(root), from, to))
            {
                return false;
            }
            SplitViewerCoreResizeView(
                to->view, sourceRect.width(), sourceRect.height(), targetRect.width(), targetRect.height());
            snapshot.m_releaseTarget = target;
            snapshot.m_nodes[source] = to;
            snapshot.m_nodes[target] = nullptr;
            return true;
        });
}

void SplitViewer::splitAt(const SplitViewerHit& hit, SplitViewerCoreSplitDirection direction)
{
    const SplitViewerCoreNode* source = hit.m_node;
    if (!source || !source->isLeaf())
    {
        return;
    }
    submitDocumentEdit(
        /** 在快照中分割叶子，并将原内容映射到第一子区域
        @param [in,out] snapshot 待编辑的独立文档及节点映射
        @return Core分割成功且节点实际成为分割节点
        */
        [source, direction](SplitViewerDocumentSnapshot& snapshot)
        {
            SplitViewerCoreNode* node = snapshot.m_nodes.at(source);
            if (!snapshot.m_document.splitNode(node, direction) || node->isLeaf())
            {
                return false;
            }
            snapshot.m_nodes[source] = node->first;
            return true;
        });
}

void SplitViewer::deleteAt(const SplitViewerHit& hit)
{
    if (!hit.m_node || !hit.m_root || hit.m_node == hit.m_root || !hit.m_node->isLeaf())
    {
        return;
    }
    const SplitViewerCoreNode* source = hit.m_node;
    const SplitViewerCoreNode* root = hit.m_root;
    submitDocumentEdit(
        /** 在快照中删除叶子并修正兄弟节点折叠后的身份映射
        @param [in,out] snapshot 待编辑的独立文档及节点映射
        @return 父节点存在且Core实际删除成功
        */
        [source, root](SplitViewerDocumentSnapshot& snapshot)
        {
            SplitViewerCoreNode* node = snapshot.m_nodes.at(source);
            SplitViewerCoreNode* tree = snapshot.m_nodes.at(root);
            SplitViewerCoreNode* parent = SplitViewerCoreFindParent(tree, node);
            if (!parent)
            {
                return false;
            }
            SplitViewerCoreNode* sibling = parent->first == node ? parent->second : parent->first;
            if (sibling && sibling->isLeaf())
            {
                for (auto& mapping : snapshot.m_nodes)
                {
                    if (mapping.second == sibling)
                    {
                        mapping.second = parent;
                    }
                }
            }
            snapshot.m_nodes[source] = nullptr;
            return snapshot.m_document.deleteLeaf(tree, node);
        });
}

void SplitViewer::detachEmbeddedTree(SplitViewerCoreNode* root)
{
    if (!root)
    {
        return;
    }
    std::vector<SplitViewerCoreNode*> leaves;
    SplitViewerCoreNode::collectLeaves(root, leaves);
    for (size_t i = 0; i < leaves.size(); ++i)
    {
        m_mediaManager->release(leaves[i]);
    }
}

bool SplitViewer::deleteLayerAt(int index)
{
    if (!m_document.layerAt(index))
    {
        return false;
    }
    return submitDocumentEdit(
        /** 在快照中删除整个浮动图层，其全部子分屏一起释放
        @param [in,out] snapshot 待提交的独立文档快照
        @return 指定图层实际存在并删除成功
        */
        [index](SplitViewerDocumentSnapshot& snapshot)
        {
            return snapshot.m_document.deleteLayer(index);
        });
}

bool SplitViewer::deleteSelectedLayer()
{
    if (!m_canvas || !m_canvas->hasFocus())
    {
        return false;
    }
    return deleteLayerAt(m_document.selectedLayer());
}

void SplitViewer::loadImageToLeaf(SplitViewerCoreNode* leaf, const QString& path)
{
    m_mediaManager->load(leaf, path);
    updateProcessing();
}

void SplitViewer::canvasFileDropped(const QString& path)
{
    if (path.isEmpty())
    {
        return;
    }
    if (QFileInfo(path).suffix().compare(g_config.m_profileSuffix, Qt::CaseInsensitive) == 0)
    {
        readProfile(path);
        return;
    }
    SplitViewerHit hit = m_activeHit;
    if (!hit.m_node || !hit.m_node->isLeaf())
    {
        hit.m_node = m_document.firstLeaf(m_document.baseRoot());
        hit.m_layer = -1;
    }
    loadImageToLeaf(hit.m_node, path);
}

void SplitViewer::newLayer()
{
    submitDocumentEdit(
        /** 在快照中创建浮动图层，是否接纳取决于Core实例预算
        @param [in,out] snapshot 待提交的独立文档快照
        @return Core返回了新建图层
        */
        [](SplitViewerDocumentSnapshot& snapshot)
        {
            return snapshot.m_document.addLayer() != nullptr;
        });
}

bool SplitViewer::writeProfile(const QString& path)
{
    if (processing() || m_closing)
    {
        return false;
    }
    const std::shared_ptr<SplitViewerDocumentSnapshot> snapshot =
        std::make_shared<SplitViewerDocumentSnapshot>(m_document.config());
    if (!m_document.copyTo(snapshot->m_document, snapshot->m_nodes))
    {
        return false;
    }
    const QRect rect = m_contentFullscreen ? normalGeometry() : geometry();
    snapshot->m_document.setWindowRect(rect.left(), rect.top(), rect.x() + rect.width(), rect.y() + rect.height());
    const QRectF stage = stageRect(m_canvas->size());
    QSize size = stage.size().toSize();
    size.scale(g_config.m_thumbnailSize, g_config.m_thumbnailSize, Qt::KeepAspectRatio);
    const QImage thumbnail = renderStage(size, false);
    const bool accepted = m_taskManager->submit(
        true,
        /** 后台编码当前快照和PNG预览，提交前检查停止，返回真实写入结果
        @param [in] exit 共享停止标志，只读原子访问
        @return 未在编码阶段取消且配置包实际写入并提交成功
        */
        [snapshot, thumbnail, path](const std::atomic<bool>& exit)
        {
            if (exit.load())
            {
                return false;
            }
            QBuffer buffer;
            if (!buffer.open(QIODevice::WriteOnly) || !thumbnail.save(&buffer, "PNG") || exit.load())
            {
                return false;
            }
            return SplitViewerProfileStore::write(path, snapshot->m_document, buffer.data());
        },
        /** 在GUI线程接纳配置保存结果；关闭中不再弹出失败提示
        @param [in] success 后台实际保存结果，不等于仅提交接受
        */
        [this, path](bool success)
        {
            updateProcessing();
            if (!m_closing)
            {
                if (success)
                {
                    m_profilePath = path;
                }
                else
                {
                    reportError(g_config.m_writeProfileError);
                }
            }
        });
    updateProcessing();
    return accepted;
}

bool SplitViewer::readProfile(const QString& path)
{
    if (processing() || m_closing)
    {
        return false;
    }
    const std::shared_ptr<SplitViewerDocumentSnapshot> snapshot =
        std::make_shared<SplitViewerDocumentSnapshot>(m_document.config());
    const quint64 revision = m_documentRevision;
    const qint64 maximum = g_config.m_profileMaximumBytes;
    const QString readError = g_config.m_readProfileError;
    const QString invalidError = g_config.m_invalidProfileError;
    const QString limitError = g_config.m_profileTooLargeError;
    const bool accepted = m_taskManager->submit(
        true,
        /** 后台读取配置并解码图片资源，失败或取消时保留当前界面文档
        @param [in] exit 共享停止标志，只读原子访问
        @return 配置解析有效且未取消；图片失效通过叶子状态保留，不使整个配置恢复失败
        */
        [snapshot, path, maximum, readError, invalidError, limitError](const std::atomic<bool>& exit)
        {
            if (exit.load() ||
                !SplitViewerProfileStore::read(
                    path, snapshot->m_document, snapshot->m_error, maximum, readError, invalidError, limitError))
            {
                return false;
            }
            std::vector<SplitViewerCoreNode*> leaves;
            SplitViewerCoreNode::collectLeaves(snapshot->m_document.baseRoot(), leaves);
            for (int index = 0; index < snapshot->m_document.layerCount(); ++index)
            {
                SplitViewerCoreNode::collectLeaves(snapshot->m_document.layerAt(index)->root, leaves);
            }
            for (SplitViewerCoreNode* leaf : leaves)
            {
                if (exit.load())
                {
                    return false;
                }
                SplitViewerImageHelper::setImageStatus(leaf, snapshot->m_images);
            }
            return !exit.load();
        },
        /** 核对文档代次后在GUI提交读取快照，过期结果不得替换新文档
        @param [in] success 本次后台配置读取与资源恢复的实际结果
        */
        [this, snapshot, revision, path](bool success)
        {
            updateProcessing();
            if (m_closing || revision != m_documentRevision)
            {
                return;
            }
            if (!success)
            {
                reportError(snapshot->m_error.isEmpty() ? g_config.m_invalidProfileError : snapshot->m_error);
                return;
            }
            m_mediaManager->releaseAll();
            m_document.swap(snapshot->m_document);
            m_imageCache.swap(snapshot->m_images);
            clearInteraction();
            m_lastStage = QRectF();
            const qint64 width = static_cast<qint64>(m_document.windowRight()) - m_document.windowLeft();
            const qint64 height = static_cast<qint64>(m_document.windowBottom()) - m_document.windowTop();
            if (width > 0 && height > 0 && width <= QWIDGETSIZE_MAX && height <= QWIDGETSIZE_MAX)
            {
                setGeometry(
                    m_document.windowLeft(), m_document.windowTop(), static_cast<int>(width), static_cast<int>(height));
            }
            m_profilePath = path;
            m_mediaManager->restoreVideos();
            canvasResized();
            updateStatus();
            LOGINFO("Profile restored; layers=%d", m_document.layerCount());
        });
    updateProcessing();
    return accepted;
}

void SplitViewer::openProfile()
{
    const QPointer<SplitViewer> owner(this);
    const quint64 revision = m_documentRevision;
    const QString path = browseFile(false, g_config.m_openProfileText, QString(), g_config.m_openProfileFilter);
    if (owner && !m_closing && revision == m_documentRevision && !path.isEmpty())
    {
        canvasFileDropped(path);
    }
}

void SplitViewer::saveProfile()
{
    if (!hasContent())
    {
        openProfile();
        return;
    }
    const QPointer<SplitViewer> owner(this);
    const quint64 revision = m_documentRevision;
    QString path = m_profilePath;
    if (path.isEmpty())
    {
        path =
            browseFile(true, g_config.m_saveProfileText, g_config.m_defaultProfileName, g_config.m_saveProfileFilter);
    }
    if (owner && !m_closing && revision == m_documentRevision && !path.isEmpty())
    {
        if (!path.endsWith(g_config.m_profileExtension, Qt::CaseInsensitive))
        {
            path += g_config.m_profileExtension;
        }
        if (!writeProfile(path))
        {
            reportError(g_config.m_writeProfileError);
        }
    }
}

void SplitViewer::saveImage()
{
    const QPointer<SplitViewer> owner(this);
    const quint64 revision = m_documentRevision;
    QString path =
        browseFile(true, g_config.m_saveImageText, g_config.m_defaultImageName, g_config.m_exportImageFilter);
    if (!owner || m_closing || revision != m_documentRevision || path.isEmpty())
    {
        return;
    }
    if (QFileInfo(path).suffix().isEmpty())
    {
        path += g_config.m_imageExtension;
    }
    const QRect screen = QApplication::desktop()->screenGeometry(this);
    QSize target(screen.width(), screen.height());
    QSize aspectSize(g_config.m_exportAspectWidth,
        (std::max)(1, static_cast<int>(g_config.m_exportAspectWidth / m_document.stageAspect())));
    aspectSize.scale(target, Qt::KeepAspectRatio);
    const QImage image = renderStage(aspectSize, true);
    const QByteArray format = QFileInfo(path).suffix().toLatin1();
    const QString failure = g_config.m_writeImageError;
    const std::shared_ptr<QString> error = std::make_shared<QString>();
    if (!m_taskManager->submit(
            true,
            /** 后台编码导出图像并原子提交文件，提交前允许协作取消
            @param [in] exit 共享停止标志，提交前检查
            @return QSaveFile实际提交成功；提交后的取消不伪造回滚
            */
            [image, path, format, error](const std::atomic<bool>& exit)
            {
                if (exit.load())
                {
                    return false;
                }
                QSaveFile file(path);
                if (!file.open(QIODevice::WriteOnly))
                {
                    *error = file.errorString();
                    return false;
                }
                QImageWriter writer(&file, format);
                if (!writer.write(image))
                {
                    *error = writer.errorString();
                    return false;
                }
                if (exit.load())
                {
                    return false;
                }
                const bool committed = file.commit();
                if (!committed)
                {
                    *error = file.errorString();
                }
                return committed;
            },
            /** 在GUI线程报告导出失败并恢复处理状态，成功不重复写文件
            @param [in] success 后台编码和提交的实际结果
            */
            [this, error, failure](bool success)
            {
                updateProcessing();
                if (!m_closing && !success)
                {
                    reportError(failure + *error);
                }
            }))
    {
        reportError(g_config.m_taskBusyError);
    }
    updateProcessing();
}

void SplitViewer::toggleFullscreen()
{
    clearInteraction();
    if (m_contentFullscreen)
    {
        leaveFullscreen();
        return;
    }
    m_normalGeometry = saveGeometry();
    m_wasMaximized = isMaximized();

    m_contentFullscreen = true;
    LOGINFO("Content fullscreen entered; wasMaximized=%d", m_wasMaximized);
    m_toolbar->hide();
    statusBar()->hide();
    showFullScreen();
    canvasResized();
    QTimer::singleShot(0, this, SLOT(canvasResized()));
    QTimer::singleShot(g_config.m_fullscreenSettleMs, this, SLOT(canvasResized()));
}

void SplitViewer::leaveFullscreen()
{
    if (!m_contentFullscreen)
    {
        return;
    }
    clearInteraction();
    m_contentFullscreen = false;
    LOGINFO("Content fullscreen exited");
    m_toolbar->show();
    if (m_wasMaximized)
    {
        showMaximized();
    }
    else
    {
        showNormal();
        restoreGeometry(m_normalGeometry);
    }
    canvasResized();
    QTimer::singleShot(0, this, SLOT(canvasResized()));
    QTimer::singleShot(g_config.m_fullscreenSettleMs, this, SLOT(canvasResized()));
}

void SplitViewer::toggleBorder()
{
    const bool old = m_document.borderVisible();
    m_document.setBorderVisible(!old);
    clearInteraction();
    m_canvas->update();
}

void SplitViewer::showAbout()
{
    SplitViewerAboutDialogParam param;
    param.m_title = g_config.m_aboutTitle;
    param.m_parent = windowHandle();
    param.m_hasShadow = g_config.m_aboutShadowEnabled;
    param.m_shadowSize = g_config.m_aboutShadowSize;
    param.m_titleBarHeight = g_config.m_aboutTitleHeight;
    param.m_centerRect = QRect(m_canvas->mapToGlobal(QPoint(0, 0)), m_canvas->size());
    param.m_message = g_config.m_aboutMessage;
    DialogManager::instance().makeDialog(param);
}

void SplitViewer::embedExternalWindow()
{
    const QPointer<SplitViewer> owner(this);
    const quint64 revision = m_documentRevision;
    bool ok = false;
    const QString text =
        SplitViewerDialogHelper::inputText(this, g_config.m_embedWindowText, g_config.m_windowHandlePrompt, ok);
    if (!owner || m_closing || revision != m_documentRevision || !ok || text.trimmed().isEmpty())
    {
        return;
    }
    QString normalized = text.trimmed();
    bool parsed = false;
    qulonglong value = normalized.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)
        ? normalized.mid(2).toULongLong(&parsed, 16)
        : normalized.toULongLong(&parsed, 10);
    if (!parsed || value == 0)
    {
        SplitViewerDialogHelper::showMessage(this, g_config.m_invalidHandleTitle, g_config.m_invalidHandleMessage);
        return;
    }
    if (!embedWindowId(static_cast<WId>(value)))
    {
        SplitViewerDialogHelper::showMessage(this, g_config.m_embedFailedTitle, g_config.m_embedFailedMessage);
    }
}

void SplitViewer::embedWindowUnderCursor()
{
    const WId windowId = SplitViewerWindowUnderCursor(winId());
    if (!windowId || !embedWindowId(windowId))
    {
        SplitViewerDialogHelper::showMessage(this, g_config.m_windowMissingTitle, g_config.m_windowMissingMessage);
    }
}

bool SplitViewer::embedWindowId(WId windowId)
{
    SplitViewerHit hit = m_activeHit;
    if (!m_mediaManager->embedWindow(hit.m_node, windowId))
    {
        return false;
    }
    LOGINFO("Native window embedded; layer=%d", hit.m_layer);
    return true;
}

void SplitViewer::detachExternalWindow()
{
    SplitViewerCoreNode* node = m_activeHit.m_node;
    if (node && node->isLeaf())
    {
        m_mediaManager->release(node);
        node->view.clear();
    }
    m_canvas->update();
    updateStatus();
}

void SplitViewer::updateStatus()
{
    if (!m_statusLabel)
    {
        return;
    }
    const std::wstring selected =
        (m_document.selectedLayer() < 0 ? g_config.m_baseLayerText : QString::number(m_document.selectedLayer() + 1))
            .toStdWString();
    m_statusLabel->setText(QString::fromStdWString(
        CStringManager::Format(g_config.m_statusFormat, m_document.layerCount(), selected.c_str())));
    if (m_saveProfileAction)
    {
        m_saveProfileAction->setText(hasContent() ? g_config.m_saveProfileText : g_config.m_openProfileText);
    }
}

void SplitViewer::closeEvent(QCloseEvent* event)
{
    m_closing = true;
    SplitViewerUnwatchNativeMouse(this);
    m_canvas->setEnabled(false);
    m_toolbar->setEnabled(false);
    m_taskManager->stop();
    m_mediaManager->shutdown();
    clearInteraction();
    if (m_mediaManager->busy() || processing())
    {
        event->ignore();
        return;
    }
    MainWindow::closeEvent(event);
}

void SplitViewer::resizeEvent(QResizeEvent* event)
{
    MainWindow::resizeEvent(event);
}

QString SplitViewer::browseFile(bool save, const QString& title, const QString& initial, const QString& filter)
{
    return SplitViewerDialogHelper::browseFile(this, save, title, initial, filter);
}

void SplitViewer::reportError(const QString& message)
{
    LOGERROR("%s", message.toUtf8().constData());
    SplitViewerDialogHelper::showMessage(this, g_config.m_operationFailedTitle, message);
}

void SplitViewer::clearInteraction()
{
    ++m_documentRevision;
    m_nativeDragWindow = 0;
    m_nativeDragNode = nullptr;
    m_nativePreview = QRectF();
    m_activeHit = SplitViewerHit();
    m_draggingImage = false;
    m_draggingLayer = false;
    m_resizingLayer = false;
    m_resizeEdges = 0;
}

void SplitViewer::canvasDoubleClick(const QPoint& point)
{
    if (stageRect(m_canvas->size()).contains(point))
    {
        toggleFullscreen();
    }
}

void SplitViewer::canvasFileDroppedAt(const QString& path, const QPoint& point)
{
    if (QFileInfo(path).suffix().compare(g_config.m_profileSuffix, Qt::CaseInsensitive) == 0)
    {
        readProfile(path);
        return;
    }
    SplitViewerHit hit;
    if (hitAll(point, hit) && !hit.m_splitter)
    {
        setSelectedHit(hit);
        loadImageToLeaf(hit.m_node, path);
    }
}

void SplitViewer::scaleViews(SplitViewerCoreNode* node, const QRectF& oldRect, const QRectF& newRect)
{
    if (!node || oldRect.width() <= 0 || oldRect.height() <= 0 || newRect.width() <= 0 || newRect.height() <= 0)
    {
        return;
    }
    if (node->isLeaf())
    {
        SplitViewerCoreResizeView(node->view, oldRect.width(), oldRect.height(), newRect.width(), newRect.height());
        return;
    }
    QRectF a, line, b, c, newLine, d;
    nodeRects(oldRect, node, a, line, b);
    nodeRects(newRect, node, c, newLine, d);
    scaleViews(node->first, a, c);
    scaleViews(node->second, b, d);
}

void SplitViewer::canvasResized()
{
    if (!m_canvas)
    {
        return;
    }
    const QRectF stage = stageRect(m_canvas->size());
    if (m_lastStage.isValid() && m_lastStage != stage)
    {
        scaleViews(m_document.baseRoot(), contentRect(m_lastStage), contentRect(stage));
        for (int i = 0; i < m_document.layerCount(); ++i)
        {
            SplitViewerCoreLayer* layer = m_document.layerAt(i);
            scaleViews(layer->root, contentRect(normalizedToPixel(layer->rect, m_lastStage)),
                contentRect(normalizedToPixel(layer->rect, stage)));
        }
    }
    m_lastStage = stage;
    syncEmbeddedViews();
    m_canvas->update();
}

void SplitViewer::syncEmbeddedNode(SplitViewerCoreNode* node, const QRectF& rect)
{
    if (!node || rect.width() <= 0.0 || rect.height() <= 0.0)
    {
        return;
    }
    if (node->isLeaf())
    {
        if (m_embedded.contains(node))
        {
            QWidget* container = m_embedded.value(node);
            if (!m_mediaManager->windowValid(container))
            {
                m_mediaManager->release(node);
                if (node->view.contentKind != SPLITVIEWER_CORE_CONTENT_VIDEO)
                {
                    node->view.clear();
                }
                return;
            }
            container->show();
            container->raise();
            SplitViewerSyncForeignWindow(container, rect.toAlignedRect());
        }
        return;
    }
    QRectF first, splitter, second;
    nodeRects(rect, node, first, splitter, second);
    syncEmbeddedNode(node->first, first);
    syncEmbeddedNode(node->second, second);
}

void SplitViewer::syncEmbeddedViews()
{
    if (!m_canvas || m_exporting)
    {
        return;
    }
    const QRectF stage = stageRect(m_canvas->size());
    syncEmbeddedNode(m_document.baseRoot(), contentRect(stage));
    for (int i = 0; i < m_document.layerCount(); ++i)
    {
        SplitViewerCoreLayer* layer = m_document.layerAt(i);
        if (layer)
        {
            syncEmbeddedNode(layer->root, contentRect(normalizedToPixel(layer->rect, stage)));
        }
    }
}

QImage SplitViewer::renderStage(const QSize& size, bool includeEmbedded)
{
    SplitViewerCoreDocument projection(m_document.config());
    std::map<const SplitViewerCoreNode*, SplitViewerCoreNode*> nodes;
    if (!m_document.copyTo(projection, nodes))
    {
        return QImage();
    }
    QMap<SplitViewerCoreNode*, QWidget*> containers;
    for (auto it = m_embedded.begin(); it != m_embedded.end(); ++it)
    {
        containers.insert(nodes.at(it.key()), it.value());
    }
    const QRectF source = stageRect(m_canvas->size());
    const QRectF target(QPointF(0, 0), QSizeF(size));
    scaleViews(projection.baseRoot(), contentRect(source), contentRect(target));
    for (int index = 0; index < projection.layerCount(); ++index)
    {
        SplitViewerCoreLayer* layer = projection.layerAt(index);
        scaleViews(layer->root, contentRect(normalizedToPixel(layer->rect, source)),
            contentRect(normalizedToPixel(layer->rect, target)));
    }
    QImage image(size, QImage::Format_RGB32);
    if (image.isNull())
    {
        return image;
    }
    image.fill(g_config.m_stageColor);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    SplitViewerRenderer renderer(projection, m_imageCache, containers, QPoint(), true, includeEmbedded);
    renderer.drawStage(painter, target);
    return image;
}

void SplitViewer::nativeMouseEvent(int event, const QPoint& screenPoint, WId id)
{
    if (m_closing || processing())
    {
        return;
    }
    if (m_closing || !isVisible() || QApplication::activeModalWidget() || QApplication::activePopupWidget())
    {
        return;
    }
    const QPoint point = m_canvas->mapFromGlobal(screenPoint);
    SplitViewerHit hit;
    const bool found = hitAll(point, hit) && !hit.m_splitter;
    if (event == 4)
    {
        if (id == winId() && found && m_embedded.contains(hit.m_node))
        {
            canvasContextMenu(point, screenPoint);
        }
        return;
    }
    if (event == 5)
    {
        return;
    }
    if (event == 1)
    {
        m_nativeDragStart = screenPoint;
        m_nativeDragNode = nullptr;
        m_nativeDragWindow = 0;
        if (id == winId() && found && m_embedded.contains(hit.m_node))
        {
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (now - m_nativeClickTime < QApplication::doubleClickInterval() &&
                (screenPoint - m_nativeLastClick).manhattanLength() < QApplication::startDragDistance())
            {
                m_nativeClickTime = 0;
                toggleFullscreen();
                return;
            }
            m_nativeClickTime = now;
            m_nativeLastClick = screenPoint;
            m_nativeDragNode = hit.m_node;
            m_nativeDragWindow = static_cast<WId>(m_embedded.value(hit.m_node)->property("foreignId").toULongLong());
            setSelectedHit(hit);
        }
        else if (SplitViewerIsExternalWindow(id, winId()))
        {
            m_nativeDragWindow = id;
        }
        return;
    }
    if (!m_nativeDragWindow)
    {
        return;
    }
    const bool moved = (screenPoint - m_nativeDragStart).manhattanLength() >= QApplication::startDragDistance();
    const bool outside = !stageRect(m_canvas->size()).contains(point);
    m_nativePreview = moved && !m_nativeDragNode && found && !hit.m_node->view.hasContent() ? hit.m_rect : QRectF();
    m_canvas->update();
    if (event != 3)
    {
        return;
    }
    const WId dragged = m_nativeDragWindow;
    SplitViewerCoreNode* source = m_nativeDragNode;
    m_nativeDragWindow = 0;
    m_nativeDragNode = nullptr;
    m_nativePreview = QRectF();
    if (source && moved && outside && m_embedded.contains(source))
    {
        const bool restored = m_mediaManager->release(source);
        source->view.clear();
        if (restored)
        {
            SplitViewerMoveDetachedWindow(dragged, screenPoint);
        }
    }
    else if (!source && moved && found && !hit.m_node->view.hasContent() &&
        SplitViewerIsExternalWindow(dragged, winId()))
    {
        setSelectedHit(hit);
        embedWindowId(dragged);
    }
    updateStatus();
    m_canvas->update();
}

bool SplitViewer::processing() const
{
    return m_taskManager && m_taskManager->busy();
}

void SplitViewer::updateProcessing()
{
    if (!m_closing)
    {
        const bool enabled = !processing();
        if (!enabled && m_canvas->isEnabled())
        {
            m_resumeCanvasFocus = m_canvas->hasFocus();
        }
        const bool restoreFocus = enabled && !m_canvas->isEnabled() && m_resumeCanvasFocus;
        m_canvas->setEnabled(enabled);
        m_toolbar->setEnabled(enabled);
        if (restoreFocus)
        {
            m_canvas->setFocus(Qt::OtherFocusReason);
            m_resumeCanvasFocus = false;
        }
    }
}

bool SplitViewer::submitDocumentEdit(const std::function<bool(SplitViewerDocumentSnapshot&)>& work)
{
    if (processing() || m_closing)
    {
        return false;
    }
    const std::shared_ptr<SplitViewerDocumentSnapshot> snapshot =
        std::make_shared<SplitViewerDocumentSnapshot>(m_document.config());
    if (!m_document.copyTo(snapshot->m_document, snapshot->m_nodes))
    {
        return false;
    }
    snapshot->m_images = m_imageCache;
    const quint64 revision = m_documentRevision + 1;
    const quint64 mediaRevision = m_mediaManager->revision();
    const bool accepted = m_taskManager->submit(
        false,
        /** 在逻辑队列执行快照编辑，编辑前后均检查所属窗口的停止标志
        @param [in] exit 本窗口任务上下文的共享停止标志
        @return 快照编辑成功且前后均未收到停止请求
        */
        [snapshot, work](const std::atomic<bool>& exit)
        {
            return !exit.load() && work(*snapshot) && !exit.load();
        },
        /** 仅在窗口、文档和媒体代次仍有效时接纳后台快照
        @param [in] success 后台快照编辑的实际结果
        */
        [this, snapshot, revision, mediaRevision](bool success)
        {
            updateProcessing();
            if (m_closing || revision != m_documentRevision || mediaRevision != m_mediaManager->revision())
            {
                LOGINFO("Discard obsolete document request; revision=%llu", revision);
                return;
            }
            if (success)
            {
                applyDocument(snapshot);
            }
            else
            {
                LOGERROR("Document request failed; revision=%llu", revision);
            }
        });
    if (accepted)
    {
        clearInteraction();
    }
    updateProcessing();
    return accepted;
}

void SplitViewer::applyDocument(const std::shared_ptr<SplitViewerDocumentSnapshot>& snapshot)
{
    std::vector<SplitViewerCoreNode*> leaves;
    SplitViewerCoreNode::collectLeaves(snapshot->m_document.baseRoot(), leaves);
    for (int index = 0; index < snapshot->m_document.layerCount(); ++index)
    {
        SplitViewerCoreNode::collectLeaves(snapshot->m_document.layerAt(index)->root, leaves);
    }
    if (snapshot->m_releaseTarget)
    {
        m_mediaManager->release(const_cast<SplitViewerCoreNode*>(snapshot->m_releaseTarget));
    }
    for (const auto& mapping : snapshot->m_nodes)
    {
        SplitViewerCoreNode* original = const_cast<SplitViewerCoreNode*>(mapping.first);
        if (mapping.second && std::find(leaves.begin(), leaves.end(), mapping.second) != leaves.end())
        {
            m_mediaManager->rebind(original, mapping.second);
        }
        else
        {
            m_mediaManager->release(original);
        }
    }
    m_document.swap(snapshot->m_document);
    clearInteraction();
    SplitViewerImageHelper::pruneUnusedImages(m_document, m_imageCache);
    syncEmbeddedViews();
    updateStatus();
    m_canvas->update();
}