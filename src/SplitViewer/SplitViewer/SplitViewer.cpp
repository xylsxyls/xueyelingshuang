#include "SplitViewer.h"

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
#include "LogManager/LogManagerAPI.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <QtCore/QBuffer>
#include <QtCore/QScopedValueRollback>
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
#include "QtControls/Label.h"
#include "QtControls/Menu.h"
#include "QtControls/StatusBar.h"
#include "QtControls/ToolBar.h"
#include "QtControls/Widget.h"

SplitViewer::SplitViewer(QWidget* parent) :
    MainWindow(parent),
    m_canvas(NULL),
    m_toolbar(NULL),
    m_saveProfileAction(NULL),
    m_statusLabel(NULL),
    m_exporting(false),
    m_contentFullscreen(false),
    m_wasMaximized(false),
    m_zoomTip(nullptr),
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
    SplitViewerWatchNativeMouse(this,[this](int event,const QPoint& point,WId id)
    {
        nativeMouseEvent(event,point,id);
    });
}

SplitViewer::~SplitViewer()
{
    SplitViewerUnwatchNativeMouse(this);
    foreach (QWidget* container, m_embedded)
    {
        SplitViewerDetachForeignWindow(container);
    }
    m_embedded.clear();
}

void SplitViewer::buildUi()
{
    setWindowTitle(QStringLiteral("分屏看图"));
    const QRect screen = QApplication::desktop()->screenGeometry(this);
    const int stageWidth = (std::max)(1, screen.width() / 2);
    const int stageHeight = (std::max)(1, screen.height() / 2);
    m_document.setStageAspect(static_cast<double>(screen.width()) / static_cast<double>(screen.height()));
    resize(stageWidth + 20, stageHeight + 42 + 20);
    setAcceptDrops(true);

    m_toolbar = new ToolBar(this);
    m_toolbar->setWindowTitle(QStringLiteral("工具"));
    addToolBar(m_toolbar);
    m_toolbar->setMovable(false);
    m_toolbar->setIconSize(QSize(20, 20));
    m_saveProfileAction = m_toolbar->addAction(QStringLiteral("加载配置"));
    m_saveProfileAction->setToolTip(QStringLiteral("空工作区加载配置，有内容时保存配置"));
    connect(m_saveProfileAction, SIGNAL(triggered()), this, SLOT(saveProfile()));
    QAction* exportAction = m_toolbar->addAction(QStringLiteral("另存图片"));
    exportAction->setToolTip(QStringLiteral("另存当前工作区图片"));
    connect(exportAction, SIGNAL(triggered()), this, SLOT(saveImage()));
    m_toolbar->addSeparator();
    QAction* layerAction = m_toolbar->addAction(QStringLiteral("新建图层"));
    connect(layerAction, SIGNAL(triggered()), this, SLOT(newLayer()));
    QAction* fullAction = m_toolbar->addAction(QStringLiteral("全屏"));
    addAction(fullAction);
    fullAction->setShortcut(QKeySequence(Qt::Key_F11));
    connect(fullAction, SIGNAL(triggered()), this, SLOT(toggleFullscreen()));
    QAction* borderAction = new QAction(QStringLiteral("边框"), this);
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
    aboutAction->setToolTip(QStringLiteral("关于"));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(showAbout()));

    m_canvas = new SplitViewerCanvas(this, this);
    setCentralWidget(m_canvas);
    m_statusLabel = new Label(this);
    m_statusLabel->setMinimumWidth(280);
    statusBar()->addPermanentWidget(m_statusLabel);
    statusBar()->hide();
    m_canvas->setObjectName(QStringLiteral("SplitViewerCanvas"));
    m_zoomTip = new Label(m_canvas);
    m_zoomTip->setObjectName(QStringLiteral("zoomTip"));
    m_zoomTip->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_zoomTip->hide();
    setStyleSheet(Config::windowStyle());
    const QRect desktop=QApplication::desktop()->availableGeometry(this);
    move(desktop.center()-QPoint(width()/2,height()/2));
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
    return !m_profilePath.isEmpty() || m_document.layerCount() > 0 || SplitViewerCoreNode::hasContent(m_document.baseRoot());
}

QRectF SplitViewer::plusButtonRect(const QRectF& rect) const
{
    return SplitViewerLayoutHelper::plusButtonRect(rect);
}

void SplitViewer::paintCanvas(QPainter& painter, const QSize& size)
{
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(QRect(QPoint(0, 0), size), m_contentFullscreen ? QColor(18,18,18) : QColor(238,238,238));
    SplitViewerRenderer renderer(m_document, m_imageCache, m_embedded, m_canvas->mapFromGlobal(QCursor::pos()), false, false);
    renderer.drawStage(painter, stageRect(size));
    if (m_nativePreview.isValid())
    {
        painter.setPen(QPen(QColor(32,144,255),3));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(m_nativePreview.adjusted(2,2,-2,-2));
    }
}

QRectF SplitViewer::contentRect(const QRectF& owner) const
{
    return SplitViewerLayoutHelper::contentRect(owner, m_document.borderVisible());
}

void SplitViewer::nodeRects(const QRectF& owner, SplitViewerCoreNode* node, QRectF& first, QRectF& splitter, QRectF& second) const
{
    SplitViewerLayoutHelper::nodeRects(owner, node, first, splitter, second, m_document.borderVisible());
}

bool SplitViewer::hitAll(const QPointF& point, SplitViewerHit& hit) const
{
    return SplitViewerHitTestHelper::hitAll(m_document, stageRect(m_canvas ? m_canvas->size() : QSize(1, 1)), point, hit);
}

void SplitViewer::setSelectedHit(const SplitViewerHit& hit)
{
    m_activeHit = hit;
    m_document.setSelectedLayer(hit.layer);
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
    if (!(modifiers & Qt::ControlModifier) && !hit.splitter && hit.node && hit.node->isLeaf() && !hit.node->view.hasContent() &&
        plusButtonRect(hit.rect).contains(point))
    {
        const QString path = browseFile(false, QStringLiteral("选择图片"), QString(),
            QStringLiteral("图片 (*.png *.jpg *.jpeg *.bmp *.gif *.webp *.tif *.tiff);;所有文件 (*)"));
        if (!path.isEmpty())
        {
            loadImageToLeaf(hit.node, path);
        }
        return;
    }
    m_dragStart = point;
    m_dragStartRect = hit.rect;
    m_resizeEdges = 0;
    m_resizingLayer = false;
    bool moveFromTopLeft = false;
    if (hit.layer >= 0)
    {
        const qreal edge = 8.0;
        moveFromTopLeft = std::abs(point.x() - hit.layerRect.left()) <= edge &&
            std::abs(point.y() - hit.layerRect.top()) <= edge;
        if (!moveFromTopLeft)
        {
            if (std::abs(point.x() - hit.layerRect.left()) <= edge) m_resizeEdges |= 1;
            if (std::abs(point.x() - hit.layerRect.right()) <= edge) m_resizeEdges |= 2;
            if (std::abs(point.y() - hit.layerRect.top()) <= edge) m_resizeEdges |= 4;
            if (std::abs(point.y() - hit.layerRect.bottom()) <= edge) m_resizeEdges |= 8;
            m_resizingLayer = m_resizeEdges != 0;
        }
    }
    m_draggingImage = !moveFromTopLeft && !hit.splitter && hit.node && hit.node->isLeaf() && hit.node->view.hasImage && !m_resizingLayer && !(hit.layer >= 0 && (modifiers & Qt::ControlModifier));
    m_draggingLayer = hit.layer >= 0 && !m_resizingLayer &&
        (moveFromTopLeft || (modifiers & Qt::ControlModifier));
}

void SplitViewer::canvasMouseMove(const QPoint& point, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(modifiers);
    if (!(buttons & Qt::LeftButton))
    {
        SplitViewerHit hover;
        Qt::CursorShape cursor=Qt::ArrowCursor;
        if (hitAll(point,hover))
        {
            const bool nearLeft=hover.layer>=0 && std::abs(point.x()-hover.layerRect.left())<=7;
            const bool nearTop=hover.layer>=0 && std::abs(point.y()-hover.layerRect.top())<=7;
            const bool nearRight=hover.layer>=0 && std::abs(point.x()-hover.layerRect.right())<=7;
            const bool nearBottom=hover.layer>=0 && std::abs(point.y()-hover.layerRect.bottom())<=7;
            const bool topLeftMove=nearLeft && nearTop;
            const bool x=nearLeft || nearRight;
            const bool y=nearTop || nearBottom;
            if (topLeftMove) cursor=Qt::SizeAllCursor;
            else if (x && y) cursor=((point.x()<hover.layerRect.center().x())==(point.y()<hover.layerRect.center().y())) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor;
            else if (x) cursor=Qt::SizeHorCursor;
            else if (y) cursor=Qt::SizeVerCursor;
            else if (hover.layer>=0 && (modifiers & Qt::ControlModifier)) cursor=Qt::SizeAllCursor;
            else if (hover.splitter) cursor=hover.node->direction==SPLITVIEWER_CORE_SPLIT_VERTICAL ? Qt::SizeHorCursor : Qt::SizeVerCursor;
            else if (!hover.node->view.hasContent() && plusButtonRect(hover.rect).contains(point)) cursor=Qt::PointingHandCursor;
        }
        m_canvas->setCursor(cursor);
        m_canvas->update();
        return;
    }
    if (!m_resizingLayer && !m_draggingLayer && m_activeHit.splitter && m_activeHit.node)
    {
        const QRectF owner = m_activeHit.ownerRect;
        const bool vertical=m_activeHit.node->direction == SPLITVIEWER_CORE_SPLIT_VERTICAL;
        const double length=vertical ? owner.width() : owner.height();
        const double thickness=m_document.borderVisible() ? 2.0 : 0.0;
        const double ratio=((vertical ? point.x()-owner.left() : point.y()-owner.top())-thickness/2.0)/(std::max)(1.0,length-thickness);
        const double minimum=(std::min)(0.5,48.0/(std::max)(1.0,length-thickness));
        m_activeHit.node->ratio=(std::max)(minimum,(std::min)(1.0-minimum,ratio));
        m_canvas->update();
        return;
    }
    if (m_draggingImage && m_activeHit.node)
    {
        if (m_activeHit.node->view.autoFit)
        {
            const QString imagePath = SplitViewerImageHelper::path(m_activeHit.node->view.path);
            if (!m_imageCache.contains(imagePath))
            {
                m_imageCache.insert(imagePath, QImage(imagePath));
            }
            const QImage image = m_imageCache.value(imagePath);
            if (!image.isNull())
            {
                m_activeHit.node->view.scale = SplitViewerCoreFitScale(image.width(), image.height(), m_activeHit.rect.width(), m_activeHit.rect.height());
            }
            m_activeHit.node->view.autoFit = false;
        }
        const QPoint delta = point - m_dragStart;
        m_activeHit.node->view.offsetX += delta.x();
        m_activeHit.node->view.offsetY += delta.y();
        m_dragStart = point;
        m_canvas->update();
        return;
    }
    if (m_draggingLayer && m_activeHit.layer >= 0 && m_activeHit.layer < m_document.layerCount())
    {
        SplitViewerCoreLayer* layer = m_document.layerAt(m_activeHit.layer);
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
            SplitViewerCoreConstrainLayerRect(layer->rect, bounds, 90.0/stage.width(), 70.0/stage.height());
            m_dragStart = point;
            m_canvas->update();
        }
    }
    if (m_resizingLayer && m_activeHit.layer >= 0 && m_activeHit.layer < m_document.layerCount())
    {
        SplitViewerCoreLayer* layer = m_document.layerAt(m_activeHit.layer);
        if (layer)
        {
            const QRectF stage = stageRect(m_canvas->size());
            const QPoint delta = point - m_dragStart;
            const double dx = delta.x() / (std::max)(1.0, stage.width());
            const double dy = delta.y() / (std::max)(1.0, stage.height());
            if (m_resizeEdges & 1) layer->rect.left=(std::max)(0.0,(std::min)(layer->rect.right-90.0/stage.width(),layer->rect.left+dx));
            if (m_resizeEdges & 2) layer->rect.right=(std::min)(1.0,(std::max)(layer->rect.left+90.0/stage.width(),layer->rect.right+dx));
            if (m_resizeEdges & 4) layer->rect.top=(std::max)(0.0,(std::min)(layer->rect.bottom-70.0/stage.height(),layer->rect.top+dy));
            if (m_resizeEdges & 8) layer->rect.bottom=(std::min)(1.0,(std::max)(layer->rect.top+70.0/stage.height(),layer->rect.bottom+dy));
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
    if (delta == 0 || !hitAll(point, hit) || hit.splitter || !hit.node || !hit.node->view.hasImage)
    {
        return;
    }
    const QImage image=m_imageCache.value(SplitViewerImageHelper::path(hit.node->view.path));
    if (image.isNull())
    {
        return;
    }
    const double fit=SplitViewerCoreFitScale(image.width(),image.height(),hit.rect.width(),hit.rect.height());
    SplitViewerCoreZoom(hit.node->view,fit,delta,(modifiers & Qt::ControlModifier)!=0);
    setSelectedHit(hit);
    m_zoomTip->setText(QString::number(hit.node->view.scale/fit*100.0,'f',0)+QStringLiteral("%"));
    m_zoomTip->adjustSize();
    m_zoomTip->move((std::max)(0,point.x()-m_zoomTip->width()/2),(std::max)(0,point.y()-40));
    m_zoomTip->show();
    m_zoomTip->raise();
    m_zoomTip->setProperty("lastZoom",QDateTime::currentMSecsSinceEpoch());
    QTimer::singleShot(1000,this,[this]()
    {
        if (QDateTime::currentMSecsSinceEpoch()-m_zoomTip->property("lastZoom").toLongLong()>=950)
        {
            m_zoomTip->hide();
        }
    });
    m_canvas->update();
}

void SplitViewer::canvasContextMenu(const QPoint& point, const QPoint& globalPoint)
{
    SplitViewerHit hit;
    if (!hitAll(point, hit) || hit.splitter)
    {
        return;
    }
    setSelectedHit(hit);
    Menu menu(this);
    QAction* horizontal = menu.addAction(QStringLiteral("水平分割"));
    QAction* vertical = menu.addAction(QStringLiteral("垂直分割"));
    QAction* deleteLayerAction = menu.addAction(QStringLiteral("删除图层"));
    deleteLayerAction->setEnabled(hit.layer >= 0);
    QAction* deleteAction = menu.addAction(QStringLiteral("删除当前分屏"));
    deleteAction->setEnabled(hit.node != hit.root);
    menu.addSeparator();
    QAction* load = menu.addAction(QStringLiteral("载入图片到当前分屏"));
    QAction* embed = menu.addAction(QStringLiteral("嵌入外部窗口"));
    QAction* detach = menu.addAction(QStringLiteral("解除嵌入"));
    detach->setEnabled(m_embedded.contains(hit.node));
    QAction* chosen = menu.exec(globalPoint);
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
    if (chosen == horizontal)
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
        deleteLayerAt(hit.layer);
    }
    else if (chosen == load && hit.node)
    {
        const QString path = browseFile(false, QStringLiteral("选择图片"), QString(), QStringLiteral("图片 (*.png *.jpg *.jpeg *.bmp *.gif *.webp);;所有文件 (*)"));
        if (!path.isEmpty())
        {
            loadImageToLeaf(hit.node, path);
        }
    }
}

void SplitViewer::splitAt(const SplitViewerHit& hit, SplitViewerCoreSplitDirection direction)
{
    if (!hit.node || !hit.node->isLeaf())
    {
        return;
    }
    hit.node->makeSplit(direction);
    if (m_embedded.contains(hit.node))
    {
        m_embedded.insert(hit.node->first,m_embedded.take(hit.node));
    }
    clearInteraction();
    m_canvas->update();
    updateStatus();
}

void SplitViewer::deleteAt(const SplitViewerHit& hit)
{
    if (!hit.node || !hit.root || hit.node == hit.root || !hit.node->isLeaf())
    {
        return;
    }
    // The surviving sibling may move into its parent; rebind its native window before deleting nodes.
    SplitViewerCoreNode* parent=SplitViewerCoreFindParent(hit.root,hit.node);
    SplitViewerCoreNode* sibling=parent->first == hit.node ? parent->second : parent->first;
    if (m_embedded.contains(hit.node))
    {
        SplitViewerDetachForeignWindow(m_embedded.take(hit.node));
    }
    if (m_embedded.contains(sibling))
    {
        m_embedded.insert(parent,m_embedded.take(sibling));
    }
    m_document.deleteLeaf(hit.root,hit.node);
    clearInteraction();
    updateStatus();
    m_canvas->update();
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
        QWidget* container = m_embedded.take(leaves[i]);
        if (container)
        {
            SplitViewerDetachForeignWindow(container);
        }
    }
}

bool SplitViewer::deleteLayerAt(int index)
{
    SplitViewerCoreLayer* layer = m_document.layerAt(index);
    if (!layer)
    {
        return false;
    }
    detachEmbeddedTree(layer->root);
    if (!m_document.deleteLayer(index))
    {
        return false;
    }
    clearInteraction();
    updateStatus();
    m_canvas->update();
    return true;
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
    if (!leaf || !leaf->isLeaf() || path.isEmpty())
    {
        return;
    }
    QImage image(path);
    if (qApp->property("debug").toBool()) LOGINFO("Load image size=%dx%d valid=%d",image.width(),image.height(),!image.isNull());
    if (image.isNull())
    {
        reportError(QStringLiteral("无法读取图片：")+path);
        return;
    }
    if (m_embedded.contains(leaf))
    {
        SplitViewerDetachForeignWindow(m_embedded.take(leaf));
    }
    m_imageCache.insert(path, image);
    leaf->view.path = path.toStdWString();
    leaf->view.contentKind = SPLITVIEWER_CORE_CONTENT_IMAGE;
    leaf->view.hasImage = true;
    leaf->view.autoFit = true;
    leaf->view.scale = 1.0;
    leaf->view.offsetX = 0.0;
    leaf->view.offsetY = 0.0;
    m_canvas->update();
    updateStatus();
}

void SplitViewer::canvasFileDropped(const QString& path)
{
    if (path.isEmpty())
    {
        return;
    }
    if (QFileInfo(path).suffix().compare(QStringLiteral("sv"), Qt::CaseInsensitive) == 0)
    {
        readProfile(path);
        return;
    }
    SplitViewerHit hit = m_activeHit;
    if (!hit.node || !hit.node->isLeaf())
    {
        hit.node = m_document.firstLeaf(m_document.baseRoot());
        hit.layer = -1;
    }
    loadImageToLeaf(hit.node, path);
}

void SplitViewer::newLayer()
{
    m_document.addLayer();
    clearInteraction();
    m_canvas->update();
    updateStatus();
}

QByteArray SplitViewer::renderPng()
{
    const QRectF stage=stageRect(m_canvas->size());
    QSize size=stage.size().toSize();
    size.scale(320,320,Qt::KeepAspectRatio);
    const QImage image=renderStage(size,false);
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer,"PNG");
    return buffer.data();
}

bool SplitViewer::writeProfile(const QString& path)
{
    const QRect windowRect = m_contentFullscreen ? normalGeometry() : geometry();
    m_document.setWindowRect(windowRect.left(), windowRect.top(), windowRect.x()+windowRect.width(), windowRect.y()+windowRect.height());
    const bool written = SplitViewerProfileStore::write(path, m_document, renderPng());
    if (written)
    {
        m_profilePath = path;
    }
    return written;
}

bool SplitViewer::readProfile(const QString& path)
{
    QString error;
    if (!SplitViewerProfileStore::read(path, m_document, error))
    {
        reportError(error);
        return false;
    }
    foreach (QWidget* container,m_embedded)
    {
        SplitViewerDetachForeignWindow(container);
    }
    m_embedded.clear();
    clearInteraction();
    m_lastStage=QRectF();
    m_imageCache.clear();
    SplitViewerImageHelper::setImageStatus(m_document.baseRoot(), m_imageCache);
    for (int i = 0; i < m_document.layerCount(); ++i)
    {
        SplitViewerCoreLayer* layer = m_document.layerAt(i);
        if (layer)
        {
            SplitViewerImageHelper::setImageStatus(layer->root, m_imageCache);
        }
    }
    if (m_document.windowRight() > m_document.windowLeft() && m_document.windowBottom() > m_document.windowTop())
    {
        setGeometry(m_document.windowLeft(), m_document.windowTop(),
            m_document.windowRight() - m_document.windowLeft(),
            m_document.windowBottom() - m_document.windowTop());
    }
    m_profilePath = path;
    LOGINFO("Load profile success; layers=%d",m_document.layerCount());
    m_canvas->update();
    updateStatus();
    return true;
}

void SplitViewer::openProfile()
{
    const QString path = browseFile(false, QStringLiteral("加载配置"), QString(), QStringLiteral("SplitViewer配置 (*.sv);;所有文件 (*)"));
    if (!path.isEmpty())
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
    QString path = m_profilePath;
    if (path.isEmpty())
    {
        path = browseFile(true, QStringLiteral("保存配置"), QStringLiteral("layout.sv"), QStringLiteral("SplitViewer配置 (*.sv)"));
    }
    if (!path.isEmpty())
    {
        if (!path.endsWith(QStringLiteral(".sv"), Qt::CaseInsensitive))
        {
            path += QStringLiteral(".sv");
        }
        if (!writeProfile(path))
        {
            reportError(QStringLiteral("无法写入配置文件。"));
        }
    }
}

void SplitViewer::saveImage()
{
    QString path=browseFile(true,QStringLiteral("另存图片"),QStringLiteral("split-view.png"),
        QStringLiteral("PNG图片 (*.png);;JPEG图片 (*.jpg *.jpeg);;BMP图片 (*.bmp);;TIFF图片 (*.tif *.tiff);;所有文件 (*)"));
    if (path.isEmpty())
    {
        return;
    }
    if (QFileInfo(path).suffix().isEmpty()) path+=QStringLiteral(".png");
    const QRect screen=QApplication::desktop()->screenGeometry(this);
    QSize target(screen.width(),screen.height());
    QSize aspectSize(4000,static_cast<int>(4000/m_document.stageAspect()));
    aspectSize.scale(target,Qt::KeepAspectRatio);
    const QImage image=renderStage(aspectSize,true);
    QImageWriter writer(path);
    if (!writer.write(image))
    {
        reportError(QStringLiteral("图片保存失败：")+writer.errorString());
    }
}

void SplitViewer::toggleFullscreen()
{
    clearInteraction();
    if (m_contentFullscreen)
    {
        leaveFullscreen();
        return;
    }
    m_normalGeometry=saveGeometry();
    m_wasMaximized=isMaximized();

    m_contentFullscreen=true;
    LOGINFO("Content fullscreen entered; wasMaximized=%d",m_wasMaximized);
    m_toolbar->hide();
    statusBar()->hide();
    showFullScreen();
    canvasResized();
    QTimer::singleShot(0, this, SLOT(canvasResized()));
    QTimer::singleShot(50, this, SLOT(canvasResized()));
}

void SplitViewer::leaveFullscreen()
{
    if (!m_contentFullscreen) return;
    clearInteraction();
    m_contentFullscreen=false;
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
    QTimer::singleShot(50, this, SLOT(canvasResized()));
}

void SplitViewer::toggleBorder()
{
    const bool old=m_document.borderVisible();
    m_document.setBorderVisible(!old);
    clearInteraction();
    m_canvas->update();
}

void SplitViewer::showAbout()
{
    SplitViewerAboutDialogParam param;
    param.m_title = QStringLiteral("关于分屏看图");
    param.m_parent = windowHandle();
    param.m_hasShadow = true;
    param.m_shadowSize = 2;
    param.m_titleBarHeight = 42;
    param.centerRect = QRect(m_canvas->mapToGlobal(QPoint(0, 0)), m_canvas->size());
    param.message = QStringLiteral("把多张图片的精彩区域组合到同一屏幕，也可以嵌入动态窗口。");
    DialogManager::instance().makeDialog(param);
}

void SplitViewer::embedExternalWindow()
{
    bool ok = false;
    const QString text = SplitViewerDialogHelper::inputText(this, QStringLiteral("嵌入外部窗口"), QStringLiteral("输入窗口句柄（十进制或0x十六进制）："), ok);
    if (!ok || text.trimmed().isEmpty())
    {
        return;
    }
    QString normalized = text.trimmed();
    bool parsed = false;
    qulonglong value = normalized.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive) ? normalized.mid(2).toULongLong(&parsed, 16) : normalized.toULongLong(&parsed, 10);
    if (!parsed || value == 0)
    {
        SplitViewerDialogHelper::showMessage(this, QStringLiteral("句柄错误"), QStringLiteral("无法解析窗口句柄。"));
        return;
    }
    if (!embedWindowId(static_cast<WId>(value)))
    {
        SplitViewerDialogHelper::showMessage(this, QStringLiteral("嵌入失败"), QStringLiteral("当前平台无法创建外部窗口容器。"));
    }
}

void SplitViewer::embedWindowUnderCursor()
{
    const WId windowId = SplitViewerWindowUnderCursor(winId());
    if (!windowId || !embedWindowId(windowId))
    {
        SplitViewerDialogHelper::showMessage(this, QStringLiteral("未找到窗口"), QStringLiteral("当前平台未找到可嵌入的外部窗口。"));
    }
}

bool SplitViewer::embedWindowId(WId windowId)
{
    SplitViewerHit hit=m_activeHit;
    if (!windowId || !hit.node || !hit.node->isLeaf() || hit.node->view.hasContent())
    {
        return false;
    }
    QWidget* container=SplitViewerEmbedForeignWindow(windowId,m_canvas);
    if (!container)
    {
        return false;
    }
    m_embedded.insert(hit.node,container);
    LOGINFO("Native window embedded; layer=%d",hit.layer);
    hit.node->view.clear();
    hit.node->view.contentKind=SPLITVIEWER_CORE_CONTENT_EMBEDDED;
    container->show();
    SplitViewerSyncForeignWindow(container, hit.rect.toAlignedRect());
    m_canvas->update();
    updateStatus();
    return true;
}

void SplitViewer::detachExternalWindow()
{
    SplitViewerCoreNode* node=m_activeHit.node;
    if (m_embedded.contains(node))
    {
        SplitViewerDetachForeignWindow(m_embedded.take(node));
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
    m_statusLabel->setText(QStringLiteral("图层 %1 · 当前 %2 · 滚轮缩放 · 拖动平移 · 右键分割").arg(m_document.layerCount()).arg(m_document.selectedLayer() < 0 ? QStringLiteral("基础层") : QString::number(m_document.selectedLayer() + 1)));
    if (m_saveProfileAction)
    {
        m_saveProfileAction->setText(hasContent() ? QStringLiteral("保存配置") : QStringLiteral("加载配置"));
    }
}

void SplitViewer::closeEvent(QCloseEvent* event)
{
    SplitViewerUnwatchNativeMouse(this);
    foreach (QWidget* container,m_embedded)
    {
        SplitViewerDetachForeignWindow(container);
    }
    m_embedded.clear();
    clearInteraction();
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
    LOGERROR("%s",message.toUtf8().constData());
    SplitViewerDialogHelper::showMessage(this,QStringLiteral("操作失败"),message);
}

void SplitViewer::clearInteraction()
{
    m_nativeDragWindow=0;
    m_nativeDragNode=nullptr;
    m_nativePreview=QRectF();
    m_activeHit=SplitViewerHit();
    m_draggingImage=false;
    m_draggingLayer=false;
    m_resizingLayer=false;
    m_resizeEdges=0;
}

void SplitViewer::canvasDoubleClick(const QPoint& point)
{
    if (stageRect(m_canvas->size()).contains(point))
    {
        toggleFullscreen();
    }
}

void SplitViewer::canvasFileDroppedAt(const QString& path,const QPoint& point)
{
    if (QFileInfo(path).suffix().compare(QStringLiteral("sv"),Qt::CaseInsensitive)==0)
    {
        readProfile(path);
        return;
    }
    SplitViewerHit hit;
    if (hitAll(point,hit) && !hit.splitter)
    {
        setSelectedHit(hit);
        loadImageToLeaf(hit.node,path);
    }
}

void SplitViewer::scaleViews(SplitViewerCoreNode* node,const QRectF& oldRect,const QRectF& newRect)
{
    if (!node || oldRect.width()<=0 || oldRect.height()<=0 || newRect.width()<=0 || newRect.height()<=0) return;
    if (node->isLeaf())
    {
        SplitViewerCoreResizeView(node->view,oldRect.width(),oldRect.height(),newRect.width(),newRect.height());
        return;
    }
    QRectF a,line,b,c,newLine,d;
    nodeRects(oldRect,node,a,line,b);
    nodeRects(newRect,node,c,newLine,d);
    scaleViews(node->first,a,c);
    scaleViews(node->second,b,d);
}

void SplitViewer::canvasResized()
{
    if (!m_canvas) return;
    const QRectF stage=stageRect(m_canvas->size());
    if (m_lastStage.isValid() && m_lastStage!=stage)
    {
        scaleViews(m_document.baseRoot(),contentRect(m_lastStage),contentRect(stage));
        for (int i=0;i<m_document.layerCount();++i)
        {
            SplitViewerCoreLayer* layer=m_document.layerAt(i);
            scaleViews(layer->root,contentRect(normalizedToPixel(layer->rect,m_lastStage)),contentRect(normalizedToPixel(layer->rect,stage)));
        }
    }
    m_lastStage=stage;
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
            syncEmbeddedNode(layer->root,
                contentRect(normalizedToPixel(layer->rect, stage)));
        }
    }
}

QImage SplitViewer::renderStage(const QSize& size,bool includeEmbedded)
{
    const QRectF source=stageRect(m_canvas->size());
    const QRectF target(QPointF(0,0),QSizeF(size));
    // Export is a projection; restore exact live view states afterwards, including manual scale.
    std::vector<SplitViewerCoreNode*> leaves;
    SplitViewerCoreNode::collectLeaves(m_document.baseRoot(),leaves);
    for (int i=0;i<m_document.layerCount();++i) SplitViewerCoreNode::collectLeaves(m_document.layerAt(i)->root,leaves);
    std::vector<SplitViewerCoreLeafState> states;
    for (size_t i=0;i<leaves.size();++i) states.push_back(leaves[i]->view);
    scaleViews(m_document.baseRoot(),contentRect(source),contentRect(target));
    for (int i=0;i<m_document.layerCount();++i)
    {
        SplitViewerCoreLayer* layer=m_document.layerAt(i);
        scaleViews(layer->root,contentRect(normalizedToPixel(layer->rect,source)),contentRect(normalizedToPixel(layer->rect,target)));
    }
    QImage image(size,QImage::Format_RGB32);
    image.fill(QColor(128,128,128));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::SmoothPixmapTransform,true);
    QScopedValueRollback<bool> exporting(m_exporting, true);
    SplitViewerRenderer renderer(m_document, m_imageCache, m_embedded, QPoint(), true, includeEmbedded);
    renderer.drawStage(painter,target);
    for (size_t i=0;i<leaves.size();++i) leaves[i]->view=states[i];
    return image;
}

void SplitViewer::nativeMouseEvent(int event,const QPoint& screenPoint,WId id)
{
    if (!isVisible() || QApplication::activeModalWidget() || QApplication::activePopupWidget()) return;
    const QPoint point=m_canvas->mapFromGlobal(screenPoint);
    SplitViewerHit hit;
    const bool found=hitAll(point,hit) && !hit.splitter;
    if (event==1)
    {
        m_nativeDragStart=screenPoint;
        m_nativeDragNode=nullptr;
        m_nativeDragWindow=0;
        if (id==winId() && found && m_embedded.contains(hit.node))
        {
            const qint64 now=QDateTime::currentMSecsSinceEpoch();
            if (now-m_nativeClickTime<QApplication::doubleClickInterval() && (screenPoint-m_nativeLastClick).manhattanLength()<QApplication::startDragDistance())
            {
                m_nativeClickTime=0;
                toggleFullscreen();
                return;
            }
            m_nativeClickTime=now;
            m_nativeLastClick=screenPoint;
            m_nativeDragNode=hit.node;
            m_nativeDragWindow=static_cast<WId>(m_embedded.value(hit.node)->property("foreignId").toULongLong());
            setSelectedHit(hit);
        }
        else if (SplitViewerIsExternalWindow(id,winId()))
        {
            m_nativeDragWindow=id;
        }
        return;
    }
    if (!m_nativeDragWindow) return;
    const bool moved=(screenPoint-m_nativeDragStart).manhattanLength()>=QApplication::startDragDistance();
    const bool outside=!stageRect(m_canvas->size()).contains(point);
    m_nativePreview=moved && !m_nativeDragNode && found && !hit.node->view.hasContent() ? hit.rect : QRectF();
    m_canvas->update();
    if (event!=3) return;
    const WId dragged=m_nativeDragWindow;
    SplitViewerCoreNode* source=m_nativeDragNode;
    m_nativeDragWindow=0;
    m_nativeDragNode=nullptr;
    m_nativePreview=QRectF();
    if (source && moved && outside && m_embedded.contains(source))
    {
        SplitViewerDetachForeignWindow(m_embedded.take(source));
        source->view.clear();
        SplitViewerMoveDetachedWindow(dragged,screenPoint);
    }
    else if (!source && moved && found && !hit.node->view.hasContent() && SplitViewerIsExternalWindow(dragged,winId()))
    {
        setSelectedHit(hit);
        embedWindowId(dragged);
    }
    updateStatus();
    m_canvas->update();
}