#include "PdfReader.h"
#include "PdfReaderSession.h"
#include "PdfReaderIconHelper.h"
#include "PdfReaderFileHelper.h"
#include "PdfReaderDialogHelper.h"
#include "QtControls/Widget.h"
#include "QtControls/Menu.h"

#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtGui/QCloseEvent>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtGui/QPixmap>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QMenu>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollBar>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QStyledItemDelegate>
#include <QtWidgets/QStyle>
#include <QtCore/QDir>
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include <QPointer>

PdfReader::PdfReader(QWidget* parent)
    : MainWindow(parent)
    , m_core(nullptr)
    , m_closeRequested(false)
    , m_closeReady(false)
    , m_lastOperationSucceeded(false)
    , m_viewGeneration(1)
    , m_thumbnails(nullptr)
    , m_pageScroll(nullptr)
    , m_pageContainer(nullptr)
    , m_pageLayout(nullptr)
    , m_emptyState(nullptr)
    , m_toolbar(nullptr)
    , m_openAction(nullptr)
    , m_saveAction(nullptr)
    , m_saveAsAction(nullptr)
    , m_saveRangeAction(nullptr)
    , m_saveEachAction(nullptr)
    , m_zoomInAction(nullptr)
    , m_zoomOutAction(nullptr)
    , m_zoomResetAction(nullptr)
    , m_helpAction(nullptr)
    , m_zoom(g_config.m_initialZoom)
    , m_thumbnailZoom(g_config.m_initialThumbnailZoom)
    , m_pageRefreshInProgress(false)
    , m_selectionScrollInProgress(false)
{
    g_config.validate();
    m_core.reset(new PdfReaderSession(this));
    buildUi();
    updateActions();
}

PdfReader::~PdfReader()
{

}

void PdfReader::buildUi()
{
    setWindowTitle(g_config.m_applicationTitle);
    resize(g_config.m_windowSize);
    setMinimumSize(g_config.m_minimumWindowSize);
    setStyleSheet(g_config.m_windowStyle + QStringLiteral("QLabel#pageLabel{") + QString::fromStdWString(CStringManager::Format(g_config.m_normalPageStyle.toStdWString().c_str(), g_config.m_pageBorderWidth)) +
        QStringLiteral("}QLabel#pageLabel[selectedPage=\"true\"]{") + QString::fromStdWString(CStringManager::Format(g_config.m_selectedPageStyle.toStdWString().c_str(), g_config.m_pageBorderWidth)) + QStringLiteral("}"));

    m_toolbar = new ToolBar(this);
    m_toolbar->setWindowTitle(g_config.m_toolbarTitle);
    addToolBar(m_toolbar);
    m_toolbar->setMovable(false);
    m_openAction = m_toolbar->addAction(g_config.m_openText, this, SLOT(chooseAndOpen()));
    m_saveAction = m_toolbar->addAction(g_config.m_saveText, this, SLOT(saveMain()));
    m_saveAsAction = m_toolbar->addAction(g_config.m_saveAsText, this, SLOT(saveAs()));
    m_toolbar->addSeparator();
    m_saveRangeAction = m_toolbar->addAction(g_config.m_exportRangeText, this, SLOT(savePageRange()));
    m_saveEachAction = m_toolbar->addAction(g_config.m_exportEachText, this, SLOT(saveEachPage()));
    m_toolbar->addSeparator();
    m_zoomOutAction = m_toolbar->addAction(g_config.m_zoomOutText, this, SLOT(zoomOut()));
    m_zoomResetAction = m_toolbar->addAction(g_config.m_zoomResetText, this, SLOT(resetZoom()));
    m_zoomResetAction->setText(QString::number(qRound(m_zoom * 100)) + QStringLiteral("%"));
    m_zoomInAction = m_toolbar->addAction(g_config.m_zoomInText, this, SLOT(zoomIn()));
    m_toolbar->addSeparator();
    QWidget* spacer = new Widget(m_toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(spacer);
    m_helpAction = m_toolbar->addAction(PdfReaderIconHelper::aboutIcon(), QString(), this, SLOT(showHelp()));
    m_helpAction->setObjectName(QStringLiteral("aboutAction"));
    m_helpAction->setToolTip(g_config.m_aboutTooltip);
    setWindowIcon(PdfReaderIconHelper::applicationIcon());

    Splitter* splitter = new Splitter(this);
    splitter->setOrientation(Qt::Horizontal);
    m_thumbnails = new PdfReaderThumbnailList(splitter);
    m_thumbnails->setMinimumWidth(g_config.m_sidebarMinimumWidth);
    m_thumbnails->setItemDelegate(new PdfReaderThumbnailDelegate(m_thumbnails));
    m_thumbnails->installEventFilter(this);
    m_thumbnails->viewport()->installEventFilter(this);
    connect(m_thumbnails, SIGNAL(currentRowChanged(int)), this, SLOT(onSelectionChanged()));
    connect(m_thumbnails, SIGNAL(reordered(int,int)), this, SLOT(onThumbnailReordered(int,int)));
    connect(m_thumbnails, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onThumbnailContextMenu(QPoint)));

    m_pageScroll = new ScrollArea(splitter);
    m_pageScroll->setObjectName(QStringLiteral("documentScroll"));
    m_pageScroll->setWidgetResizable(true);
    m_pageScroll->viewport()->installEventFilter(this);
    m_pageScroll->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_pageContainer = new Widget(m_pageScroll);
    m_pageLayout = new QVBoxLayout(m_pageContainer);
    m_pageLayout->setContentsMargins(g_config.m_bodyMarginX, g_config.m_bodyMarginY, g_config.m_bodyMarginX, g_config.m_bodyMarginY);
    m_pageLayout->setSpacing(g_config.m_bodySpacing);
    m_emptyState = new PdfReaderEmptyState(m_pageContainer);
    connect(m_emptyState, SIGNAL(clicked()), this, SLOT(chooseAndOpen()));
    m_pageLayout->addWidget(m_emptyState, 1);
    m_pageScroll->setWidget(m_pageContainer);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);
    splitter->setSizes(QList<int>() << g_config.m_sidebarWidth << g_config.m_bodyWidth);
    connect(m_pageScroll->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(onPageScrollChanged()));
    connect(m_pageScroll->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(renderVisiblePages()));
    connect(m_thumbnails->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(renderVisiblePages()));
    m_openAction->setShortcut(QKeySequence::Open);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    statusBar()->showMessage(g_config.m_readyText);
}

bool PdfReader::askForPassword(QString* password)
{
    QString value;
    if (!PdfReaderDialogHelper::input(this, g_config.m_passwordTitle, g_config.m_passwordPrompt, value, true))
    {
        return false;
    }
    if (password)
    {
        *password = value;
    }
    return true;
}

bool PdfReader::openWithPassword(const PdfReaderRequest& request)
{
    return submitOperation(request, [this](const PdfReaderResult& result) {
        if (!result.m_success)
        {
            if (result.m_code == PdfReaderCoreCResultPasswordRequired)
            {
                PdfReaderRequest retry = result.m_request;
                if (askForPassword(&retry.m_password))
                {
                    openWithPassword(retry);
                }
            }
            else
            {
                PdfReaderDialogHelper::message(this,
                    result.m_request.m_operation == PdfReaderOpen ? g_config.m_openFailedText : g_config.m_insertFailedText,
                    result.m_error);
            }
            return;
        }
        const bool opening = result.m_request.m_operation == PdfReaderOpen;
        if (opening)
        {
            m_currentPath = result.m_request.m_path;
            m_thumbnails->clear();
        }
        refreshDocument();
        m_thumbnails->setCurrentRow(opening ? 0 : result.m_request.m_index, QItemSelectionModel::ClearAndSelect);
        if (opening)
        {
            m_thumbnails->verticalScrollBar()->setValue(0);
            m_pageScroll->verticalScrollBar()->setValue(0);
            m_pageScroll->horizontalScrollBar()->setValue(0);
            statusBar()->showMessage(QString::fromStdWString(CStringManager::Format(g_config.m_openedFormat.toStdWString().c_str(),
                QString::fromUtf8(CSystem::GetName(m_currentPath.toUtf8().constData(), 1).c_str()).toStdWString().c_str(), m_core->pageCount())));
        }
    });
}

bool PdfReader::openFile(const QString& filePath)
{
    if (m_core->busy())
    {
        return false;
    }
    QString path = filePath;
    if (path.isEmpty())
    {
        path = PdfReaderDialogHelper::file(this, PdfReaderDialogOpenFile, g_config.m_openDialogTitle, QString(), g_config.m_openFilter);
    }
    if (path.isEmpty())
    {
        return false;
    }
    PdfReaderRequest request;
    request.m_operation = PdfReaderOpen;
    request.m_path = path;
    return openWithPassword(request);
}

void PdfReader::chooseAndOpen()
{
    openFile();
}

void PdfReader::refreshDocument()
{
    refreshThumbnails();
    refreshPages();
    setWindowDocumentTitle();
    updateActions();
}

void PdfReader::refreshThumbnails()
{
    m_thumbnails->cancelDrag();
    ++m_viewGeneration;
    m_failedImages.clear();
    m_core->invalidateRenders();
    const int selected = qBound(0, selectedPage(), m_core->pageCount() - 1);
    const int scroll = m_thumbnails->verticalScrollBar()->value();
    m_thumbnails->blockSignals(true);
    m_thumbnails->clear();
    // 放大只扩大条目的灰色可点击背景，PDF 页图保持基准尺寸；缩小时仍保留旧版缩小能力。
    const double pageScale = qMin(1.0, m_thumbnailZoom);
    const int width = qMax(g_config.m_thumbnailMinWidth, qMin(qRound(g_config.m_thumbnailWidth * pageScale), m_thumbnails->viewport()->width() - g_config.m_thumbnailSidePadding));
    int maxHeight = 1;
    for (int i = 0; i < m_core->pageCount(); ++i)
    {
        PdfReaderCoreCPageInfo info;
        if (!m_core->pageInfo(i, &info) || info.width <= 0 || info.height <= 0)
        {
            info.width = g_config.m_fallbackPageWidth;
            info.height = g_config.m_fallbackPageHeight;
        }
        const int height = qMax(1, qRound(width * info.height / info.width));
        const int itemHeight = m_thumbnailZoom > 1.0 ?
            qRound((height + g_config.m_thumbnailRowPadding) * m_thumbnailZoom) : height + g_config.m_thumbnailRowPadding;
        QListWidgetItem* item = new QListWidgetItem(QString::number(i + 1), m_thumbnails);
        item->setTextAlignment(Qt::AlignHCenter);
        item->setData(Qt::UserRole, i);
        item->setData(Qt::UserRole + 1, QSize(width, height));
        item->setData(Qt::UserRole + 2, itemHeight);
        item->setSizeHint(QSize(qMax(g_config.m_thumbnailItemMinWidth, m_thumbnails->viewport()->width() - g_config.m_thumbnailItemSideInset), itemHeight));
        maxHeight = qMax(maxHeight, height);
    }
    m_thumbnails->setIconSize(QSize(width, maxHeight));
    m_thumbnails->setCurrentRow(selected, QItemSelectionModel::ClearAndSelect);
    m_thumbnails->doItemsLayout();
    m_thumbnails->verticalScrollBar()->setValue(scroll);
    m_thumbnails->blockSignals(false);
    QTimer::singleShot(0, this, SLOT(renderVisiblePages()));
}

void PdfReader::refreshPages()
{
    ++m_viewGeneration;
    m_failedImages.clear();
    m_core->invalidateRenders();
    m_pageRefreshInProgress = true;
    const int scrollY = m_pageScroll->verticalScrollBar()->value();
    const int scrollX = m_pageScroll->horizontalScrollBar()->value();
    m_pageScroll->blockSignals(true);
    while (m_pageLayout->count() > 0)
    {
        QLayoutItem* item = m_pageLayout->takeAt(0);
        delete item->widget();
        delete item;
    }
    m_emptyState = nullptr;
    if (!m_core->isOpen())
    {
        m_emptyState = new PdfReaderEmptyState(m_pageContainer);
        connect(m_emptyState, SIGNAL(clicked()), this, SLOT(chooseAndOpen()));
        m_pageLayout->addWidget(m_emptyState, 1);
    }
    for (int i = 0; i < m_core->pageCount(); ++i)
    {
        PdfReaderCoreCPageInfo info;
        if (!m_core->pageInfo(i, &info) || info.width <= 0 || info.height <= 0)
        {
            info.width = g_config.m_fallbackPageWidth;
            info.height = g_config.m_fallbackPageHeight;
        }
        Label* page = new Label(m_pageContainer);
        page->setObjectName(QStringLiteral("pageLabel"));
        page->setProperty("pageIndex", i);
        page->setFixedSize(qMax(1, qRound(info.width * m_zoom)) + g_config.m_pageBorderWidth * 2, qMax(1, qRound(info.height * m_zoom)) + g_config.m_pageBorderWidth * 2);
        page->setAlignment(Qt::AlignCenter);
        page->installEventFilter(this);
        m_pageLayout->addWidget(page, 0, Qt::AlignHCenter);
    }
    if (m_core->isOpen())
    {
        m_pageLayout->addStretch(1);
    }
    m_pageLayout->activate();
    m_pageContainer->adjustSize();
    m_pageScroll->blockSignals(false);
    m_pageScroll->verticalScrollBar()->setValue(scrollY);
    m_pageScroll->horizontalScrollBar()->setValue(scrollX);
    m_pageRefreshInProgress = false;
    onSelectionChanged();
    QTimer::singleShot(0, this, SLOT(renderVisiblePages()));
}

void PdfReader::renderVisiblePages()
{
    if (m_closeRequested || m_core->busy() || m_pageRefreshInProgress)
    {
        return;
    }
    const QRect viewport(QPoint(0, 0), m_pageScroll->viewport()->size());
    for (int32_t index = 0; index < m_core->pageCount(); ++index)
    {
        QLayoutItem* item = m_pageLayout->itemAt(index);
        Label* page = item ? dynamic_cast<Label*>(item->widget()) : nullptr;
        if (page)
        {
            const QRect rect(page->mapTo(m_pageScroll->viewport(), QPoint(0, 0)), page->size());
            if (rect.intersects(viewport))
            {
                if (!page->pixmap())
                {
                    requestImage(index, page->size() - QSize(g_config.m_pageBorderWidth * 2, g_config.m_pageBorderWidth * 2), false);
                }
            }
            else if (page->pixmap())
            {
                page->clear();
            }
        }
        QListWidgetItem* thumb = m_thumbnails->item(index);
        if (!thumb)
        {
            continue;
        }
        if (m_thumbnails->visualItemRect(thumb).intersects(m_thumbnails->viewport()->rect()))
        {
            if (thumb->icon().isNull())
            {
                requestImage(index, thumb->data(Qt::UserRole + 1).toSize(), true);
            }
        }
        else if (!thumb->icon().isNull())
        {
            thumb->setIcon(QIcon());
        }
    }
}

void PdfReader::updateActions()
{
    const bool open = m_core->isOpen() && !m_core->busy();
    m_openAction->setEnabled(!m_core->busy());
    m_thumbnails->setEnabled(!m_core->busy());
    m_saveAction->setEnabled(open);
    m_saveAsAction->setEnabled(open);
    m_saveRangeAction->setEnabled(open);
    m_saveEachAction->setEnabled(open);
    m_zoomInAction->setEnabled(open);
    m_zoomOutAction->setEnabled(open);
    m_zoomResetAction->setEnabled(open);
}

int PdfReader::selectedPage() const
{
    return m_thumbnails ? m_thumbnails->currentRow() : -1;
}

bool PdfReader::submitOperation(const PdfReaderRequest& request,
    const std::function<void(const PdfReaderResult&)>& completion)
{
    const uint64_t id = m_core->submit(request, [this, completion](const PdfReaderResult& result) {
        m_lastOperationSucceeded = result.m_success;
        updateActions();
        const QPointer<PdfReader> guard(this);
        completion(result);
        if (guard)
        {
            emit operationFinished(result.m_request.m_id, result.m_success);
        }
    });
    if (id)
    {
        updateActions();
    }
    return id != 0;
}

void PdfReader::saveMain()
{
    if (!m_core->isOpen() || m_core->busy())
    {
        return;
    }
    if (!PdfReaderDialogHelper::question(this, g_config.m_confirmSaveTitle, g_config.m_confirmSavePrompt + m_currentPath))
    {
        return;
    }
    PdfReaderRequest request;
    request.m_operation = PdfReaderSaveMain;
    exportDocument(request, g_config.m_savedText);
}

void PdfReader::saveAs()
{
    if (!m_core->isOpen() || m_core->busy())
    {
        return;
    }
    const QString suggested = QString::fromUtf8(CSystem::GetName(m_currentPath.toUtf8().constData(), 3).c_str()) + g_config.m_editedSuffix;
    const QString path = PdfReaderDialogHelper::file(this, PdfReaderDialogSaveFile, g_config.m_saveAsText, suggested, g_config.m_saveFilter);
    if (path.isEmpty())
    {
        return;
    }
    const QString currentCanonicalPath = QFileInfo(m_currentPath).canonicalFilePath();
    if (!currentCanonicalPath.isEmpty() && QFileInfo(path).canonicalFilePath() == currentCanonicalPath)
    {
        saveMain();
        return;
    }
    PdfReaderRequest request;
    request.m_operation = PdfReaderSave;
    request.m_path = path;
    exportDocument(request, g_config.m_savedAsText);
}

void PdfReader::savePageRange()
{
    if (!m_core->isOpen() || m_core->busy())
    {
        return;
    }
    QString range = QStringLiteral("1-") + QString::number(m_core->pageCount());
    if (!PdfReaderDialogHelper::input(this, g_config.m_rangeTitle, g_config.m_rangeExample, range, false, true) || range.isEmpty())
    {
        return;
    }
    PdfReaderRequest request;
    request.m_operation = PdfReaderValidateRange;
    request.m_text = range;
    submitOperation(request, [this](const PdfReaderResult& result) {
        if (!result.m_success)
        {
            PdfReaderDialogHelper::message(this, g_config.m_invalidRangeText, result.m_error);
            return;
        }
        const QString suggested = QString::fromUtf8(CSystem::GetName(m_currentPath.toUtf8().constData(), 3).c_str()) + g_config.m_pagesSuffix;
        const QString path = PdfReaderDialogHelper::file(this, PdfReaderDialogSaveFile, g_config.m_saveRangeTitle, suggested, g_config.m_saveFilter);
        if (path.isEmpty())
        {
            return;
        }
        PdfReaderRequest save = result.m_request;
        save.m_operation = PdfReaderSaveRange;
        save.m_path = path;
        exportDocument(save, g_config.m_rangeSavedText);
    });
}

void PdfReader::saveEachPage()
{
    if (!m_core->isOpen() || m_core->busy())
    {
        return;
    }
    const QString directory = PdfReaderDialogHelper::file(this, PdfReaderDialogDirectory, g_config.m_exportDirectoryTitle, QString(), QString());
    if (directory.isEmpty())
    {
        return;
    }
    PdfReaderRequest request;
    request.m_operation = PdfReaderSaveEach;
    request.m_path = directory;
    request.m_text = QString::fromUtf8(CSystem::GetName(m_currentPath.toUtf8().constData(), 3).c_str());
    exportDocument(request, g_config.m_eachSavedText);
}

void PdfReader::insertBefore()
{
    insertDocument(selectedPage());
}

void PdfReader::insertAfter()
{
    if (selectedPage() >= 0)
    {
        insertDocument(selectedPage() + 1);
    }
}

void PdfReader::insertDocument(int index)
{
    if (index < 0 || !m_core->isOpen() || m_core->busy())
    {
        return;
    }
    const QString path = PdfReaderDialogHelper::file(this, PdfReaderDialogOpenFile, g_config.m_insertDialogTitle, QString(), g_config.m_saveFilter);
    if (path.isEmpty())
    {
        return;
    }
    PdfReaderRequest request;
    request.m_operation = PdfReaderInsert;
    request.m_path = path;
    request.m_index = index;
    openWithPassword(request);
}

void PdfReader::onSelectionChanged()
{
    updateSelectionState(true);
}

void PdfReader::updateSelectionState(bool ensureVisible)
{
    const int selected = selectedPage();
    if (selected >= 0)
    {
        statusBar()->showMessage(QString::fromStdWString(CStringManager::Format(g_config.m_pageStatusFormat.toStdWString().c_str(), selected + 1, m_core->pageCount())));
    }
    for (int i = 0; i < m_core->pageCount(); ++i)
    {
        QLayoutItem* item = m_pageLayout->itemAt(i);
        if (item && item->widget())
        {
            item->widget()->setProperty("selectedPage", i == selected);
            item->widget()->style()->unpolish(item->widget());
            item->widget()->style()->polish(item->widget());
            item->widget()->update();
        }
    }
    if (ensureVisible && selected >= 0 && selected < m_pageLayout->count())
    {
        QLayoutItem* selectedItem = m_pageLayout->itemAt(selected);
        QWidget* selectedPageWidget = selectedItem ? selectedItem->widget() : nullptr;
        if (selectedPageWidget != nullptr)
        {
            // 左侧点击后让对应正文页进入右侧视口，灰色条目点击也走同一选择路径。
            m_selectionScrollInProgress = true;
            m_pageScroll->ensureWidgetVisible(selectedPageWidget, g_config.m_selectionMargin, g_config.m_selectionMargin);
            m_selectionScrollInProgress = false;
        }
    }
}

void PdfReader::onPageScrollChanged()
{
    if (m_pageRefreshInProgress || m_selectionScrollInProgress)
    {
        renderVisiblePages();
        return;
    }
    syncSelectionFromPageScroll();
    renderVisiblePages();
}

void PdfReader::syncSelectionFromPageScroll()
{
    if (!m_core->isOpen() || !m_pageScroll || !m_thumbnails || m_core->pageCount() <= 0)
    {
        return;
    }
    const QRect viewport(QPoint(0, 0), m_pageScroll->viewport()->size());
    int visiblePage = -1;
    int mostVisibleHeight = 0;
    const int currentPage = selectedPage();
    for (int i = 0; i < m_core->pageCount(); ++i)
    {
        QLayoutItem* item = m_pageLayout->itemAt(i);
        QWidget* page = item ? item->widget() : nullptr;
        if (!page)
        {
            continue;
        }
        const QRect pageRect(page->mapTo(m_pageScroll->viewport(), QPoint(0, 0)), page->size());
        const QRect visibleRect = pageRect.intersected(viewport);
        if (visibleRect.isEmpty())
        {
            continue;
        }
        const int visibleHeight = visibleRect.height();
        if (visibleHeight > mostVisibleHeight || (visibleHeight == mostVisibleHeight && i == currentPage))
        {
            mostVisibleHeight = visibleHeight;
            visiblePage = i;
        }
    }
    if (visiblePage < 0 || visiblePage == selectedPage())
    {
        return;
    }
    m_thumbnails->blockSignals(true);
    m_thumbnails->setCurrentRow(visiblePage, QItemSelectionModel::ClearAndSelect);
    m_thumbnails->blockSignals(false);
    updateSelectionState(false);
    QListWidgetItem* item = m_thumbnails->item(visiblePage);
    if (item)
    {
        m_thumbnails->scrollToItem(item, QAbstractItemView::EnsureVisible);
    }
}

void PdfReader::onThumbnailContextMenu(const QPoint& position)
{
    QListWidgetItem* hitItem = m_thumbnails->itemAt(position);
    if (hitItem)
    {
        m_thumbnails->setCurrentItem(hitItem, QItemSelectionModel::ClearAndSelect);
    }
    Menu menu(this);
    if (hitItem)
    {
        connect(menu.addAction(g_config.m_insertBeforeText), SIGNAL(triggered()), this, SLOT(insertBefore()));
        connect(menu.addAction(g_config.m_insertAfterText), SIGNAL(triggered()), this, SLOT(insertAfter()));
    }
    else
    {
        menu.QMenu::addAction(m_saveEachAction);
        menu.QMenu::addAction(m_saveRangeAction);
    }
    menu.exec(m_thumbnails->viewport()->mapToGlobal(position));
}

void PdfReader::onThumbnailReordered(int fromRow, int toRow)
{
    PdfReaderRequest request;
    request.m_operation = PdfReaderMove;
    request.m_index = fromRow;
    request.m_target = toRow;
    if (!submitOperation(request, [this](const PdfReaderResult& result) {
        refreshDocument();
        if (result.m_success)
        {
            m_thumbnails->setCurrentRow(result.m_request.m_target, QItemSelectionModel::ClearAndSelect);
        }
        else
        {
            PdfReaderDialogHelper::message(this, g_config.m_reorderFailedText, result.m_error);
        }
    }))
    {
        refreshDocument();
    }
}

void PdfReader::setZoom(double zoom)
{
    const double bounded = qBound(g_config.m_minimumZoom, zoom, g_config.m_maximumZoom);
    if (!m_core->isOpen() || qAbs(bounded - m_zoom) < g_config.m_zoomComparisonTolerance)
    {
        return;
    }
    m_zoom = bounded;
    m_zoomResetAction->setText(QString::number(qRound(m_zoom * 100)) + QStringLiteral("%"));
    refreshPages();
}

void PdfReader::zoomIn()
{
    setZoom(m_zoom + g_config.m_zoomStep);
}

void PdfReader::zoomOut()
{
    setZoom(m_zoom - g_config.m_zoomStep);
}

void PdfReader::resetZoom()
{
    setZoom(g_config.m_initialZoom);
}

void PdfReader::showHelp()
{
    PdfReaderDialogHelper::message(this, g_config.m_aboutTitle,
                             g_config.m_aboutVersionText + QStringLiteral("\n") + g_config.m_aboutText, true);
}

void PdfReader::setWindowDocumentTitle()
{
    setWindowTitle(m_core->isOpen() ? g_config.m_documentTitlePrefix + QString::fromUtf8(CSystem::GetName(m_currentPath.toUtf8().constData(), 1).c_str()) : g_config.m_applicationTitle);
}

void PdfReader::closeEvent(QCloseEvent* event)
{
    if (m_closeReady)
    {
        event->accept();
        return;
    }
    event->ignore();
    if (!m_closeRequested)
    {
        m_closeRequested = true;
        m_core->close([this](const PdfReaderResult& result) {
            if (result.m_success)
            {
                m_closeReady = true;
                close();
            }
            else
            {
                m_closeRequested = false;
                updateActions();
                statusBar()->showMessage(result.m_error, g_config.m_statusMessageMs);
            }
        });
        updateActions();
    }
}

bool PdfReader::eventFilter(QObject* watched, QEvent* event)
{
    const bool thumbnail = watched == m_thumbnails || watched == m_thumbnails->viewport();
    if (event->type() == QEvent::Resize)
    {
        if (thumbnail)
        {
            QTimer::singleShot(0, this, SLOT(refreshThumbnails()));
        }
        QTimer::singleShot(0, this, SLOT(renderVisiblePages()));
    }
    if (event->type() == QEvent::MouseButtonPress && watched->property("pageIndex").isValid())
    {
        QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
        if (mouse->button() == Qt::LeftButton)
        {
            m_thumbnails->setCurrentRow(watched->property("pageIndex").toInt(), QItemSelectionModel::ClearAndSelect);
        }
    }
    if (event->type() == QEvent::Wheel)
    {
        QWheelEvent* wheel = static_cast<QWheelEvent*>(event);
        if (thumbnail && m_thumbnails->gestureActive())
        {
            return false;
        }
        if (wheel->modifiers() & Qt::ControlModifier)
        {
            if (wheel->delta() != 0 && m_core->isOpen())
            {
                if (thumbnail)
                {
                    m_thumbnailZoom = qBound(g_config.m_minimumThumbnailZoom, m_thumbnailZoom + wheel->delta() / 120.0 * g_config.m_thumbnailZoomStep, g_config.m_maximumThumbnailZoom);
                    refreshThumbnails();
                }
                else
                {
                    setZoom(m_zoom + wheel->delta() / 120.0 * g_config.m_zoomStep);
                }
            }
            wheel->accept();
            return true;
        }
    }
    return MainWindow::eventFilter(watched, event);
}

void PdfReader::processCoreResults()
{
    const QPointer<PdfReader> guard(this);
    m_core->processResults();
    if (guard)
    {
        renderVisiblePages();
    }
}

bool PdfReader::idle() const
{
    return m_core->idle();
}

bool PdfReader::lastOperationSucceeded() const
{
    return m_lastOperationSucceeded;
}

void PdfReader::exportDocument(const PdfReaderRequest& request, const QString& successText)
{
    submitOperation(request, [this, successText](const PdfReaderResult& result) {
        if (result.m_request.m_operation == PdfReaderSaveMain)
        {
            refreshDocument();
        }
        if (result.m_success)
        {
            statusBar()->showMessage(successText, g_config.m_statusMessageMs);
        }
        else if (result.m_request.m_operation == PdfReaderSaveEach && result.m_code == PdfReaderCoreCResultFileExists)
        {
            if (PdfReaderDialogHelper::question(this, g_config.m_confirmOverwriteTitle, g_config.m_confirmOverwritePrompt + result.m_error))
            {
                PdfReaderRequest retry = result.m_request;
                retry.m_overwrite = true;
                exportDocument(retry, successText);
            }
        }
        else
        {
            PdfReaderDialogHelper::message(this,
                result.m_request.m_operation == PdfReaderSave || result.m_request.m_operation == PdfReaderSaveMain ?
                    g_config.m_saveFailedText : g_config.m_exportFailedText, result.m_error);
        }
    });
}

void PdfReader::requestImage(int32_t index, const QSize& size, bool thumbnail)
{
    const uint64_t generation = m_viewGeneration;
    const QString key = QString::number(generation) + ":" + QString::number(thumbnail) + ":" +
        QString::number(index) + ":" + QString::number(size.width()) + ":" + QString::number(size.height());
    if (m_pendingImages.contains(key) || m_failedImages.contains(key))
    {
        return;
    }
    PdfReaderRequest request;
    request.m_operation = PdfReaderRender;
    request.m_index = index;
    request.m_size = size;
    const uint64_t id = m_core->submit(request, [this, generation, key, thumbnail](const PdfReaderResult& result) {
        m_pendingImages.remove(key);
        if (result.m_cancelled || generation != m_viewGeneration)
        {
            return;
        }
        if (!result.m_success)
        {
            m_failedImages.insert(key);
        }
        const int index = result.m_request.m_index;
        if (thumbnail)
        {
            QListWidgetItem* item = m_thumbnails->item(index);
            if (item && result.m_success && m_thumbnails->visualItemRect(item).intersects(m_thumbnails->viewport()->rect()))
            {
                const QPixmap pixmap = QPixmap::fromImage(result.m_image);
                QIcon icon(pixmap);
                icon.addPixmap(pixmap, QIcon::Selected);
                icon.addPixmap(pixmap, QIcon::Active);
                item->setIcon(icon);
            }
        }
        else
        {
            QLayoutItem* item = m_pageLayout->itemAt(index);
            Label* page = item ? dynamic_cast<Label*>(item->widget()) : nullptr;
            if (page && QRect(page->mapTo(m_pageScroll->viewport(), QPoint(0, 0)), page->size()).intersects(m_pageScroll->viewport()->rect()))
            {
                if (result.m_success)
                {
                    page->setPixmap(QPixmap::fromImage(result.m_image));
                }
                else
                {
                    page->setText(result.m_error);
                }
            }
        }
    });
    if (id)
    {
        m_pendingImages.insert(key);
    }
}