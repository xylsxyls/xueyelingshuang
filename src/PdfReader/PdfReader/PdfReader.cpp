#include "PdfReader.h"
#include "PdfReaderCoreBridge.h"
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

PdfReader::PdfReader(QWidget* parent, const Config& config)
    : MainWindow(parent)
    , m_config(config)
    , m_core(nullptr)
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
    , m_zoom(config.initialZoom)
    , m_thumbnailZoom(config.initialThumbnailZoom)
    , m_pageRefreshInProgress(false)
    , m_selectionScrollInProgress(false)
{
    m_config.validate();
    m_core = new PdfReaderCoreBridge(m_config);
    buildUi();
    QString error;
    if (!m_core->init(m_config.core, &error))
        PdfReaderDialogHelper::message(this, m_config.applicationTitle, error, m_config);
    updateActions();
}

PdfReader::~PdfReader()
{
    delete m_core;
}

void PdfReader::buildUi()
{
    setWindowTitle(m_config.applicationTitle);
    resize(m_config.windowSize);
    setMinimumSize(m_config.minimumWindowSize);
    setStyleSheet(m_config.windowStyle + QStringLiteral("QLabel#pageLabel{") + m_config.normalPageStyle +
        QStringLiteral("}QLabel#pageLabel[selectedPage=\"true\"]{") + m_config.selectedPageStyle + QStringLiteral("}"));

    m_toolbar = new ToolBar(this);
    m_toolbar->setWindowTitle(m_config.toolbarTitle);
    addToolBar(m_toolbar);
    m_toolbar->setMovable(false);
    m_openAction = m_toolbar->addAction(m_config.openText, this, SLOT(chooseAndOpen()));
    m_saveAction = m_toolbar->addAction(m_config.saveText, this, SLOT(saveMain()));
    m_saveAsAction = m_toolbar->addAction(m_config.saveAsText, this, SLOT(saveAs()));
    m_toolbar->addSeparator();
    m_saveRangeAction = m_toolbar->addAction(m_config.exportRangeText, this, SLOT(savePageRange()));
    m_saveEachAction = m_toolbar->addAction(m_config.exportEachText, this, SLOT(saveEachPage()));
    m_toolbar->addSeparator();
    m_zoomOutAction = m_toolbar->addAction(m_config.zoomOutText, this, SLOT(zoomOut()));
    m_zoomResetAction = m_toolbar->addAction(m_config.zoomResetText, this, SLOT(resetZoom()));
    m_zoomResetAction->setText(QString::number(qRound(m_zoom * 100)) + QStringLiteral("%"));
    m_zoomInAction = m_toolbar->addAction(m_config.zoomInText, this, SLOT(zoomIn()));
    m_toolbar->addSeparator();
    QWidget* spacer = new Widget(m_toolbar);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(spacer);
    m_helpAction = m_toolbar->addAction(PdfReaderIconHelper::aboutIcon(), QString(), this, SLOT(showHelp()));
    m_helpAction->setObjectName(QStringLiteral("aboutAction"));
    m_helpAction->setToolTip(m_config.aboutTooltip);
    setWindowIcon(PdfReaderIconHelper::applicationIcon());

    Splitter* splitter = new Splitter(this);
    splitter->setOrientation(Qt::Horizontal);
    m_thumbnails = new PdfReaderThumbnailList(splitter, m_config);
    m_thumbnails->setMinimumWidth(m_config.sidebarMinimumWidth);
    m_thumbnails->setItemDelegate(new PdfReaderThumbnailDelegate(m_thumbnails, m_config));
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
    m_pageLayout->setContentsMargins(m_config.bodyMarginX, m_config.bodyMarginY, m_config.bodyMarginX, m_config.bodyMarginY);
    m_pageLayout->setSpacing(m_config.bodySpacing);
    m_emptyState = new PdfReaderEmptyState(m_pageContainer);
    connect(m_emptyState, SIGNAL(clicked()), this, SLOT(chooseAndOpen()));
    m_pageLayout->addWidget(m_emptyState, 1);
    m_pageScroll->setWidget(m_pageContainer);
    splitter->setStretchFactor(1, 1);
    setCentralWidget(splitter);
    splitter->setSizes(QList<int>() << m_config.sidebarWidth << m_config.bodyWidth);
    connect(m_pageScroll->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(onPageScrollChanged()));
    connect(m_pageScroll->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(renderVisiblePages()));
    connect(m_thumbnails->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(renderVisiblePages()));
    m_openAction->setShortcut(QKeySequence::Open);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    statusBar()->showMessage(m_config.readyText);
}

bool PdfReader::askForPassword(QString* password)
{
    QString value;
    if (!PdfReaderDialogHelper::input(this, m_config.passwordTitle, m_config.passwordPrompt, value, true, m_config)) return false;
    if (password) *password = value;
    return true;
}

bool PdfReader::openWithPassword(const QString& filePath)
{
    QString password;
    for (;;)
    {
        QString error;
        if (m_core->open(filePath, password, &error))
            return true;
        if (!error.contains(QStringLiteral("password"), Qt::CaseInsensitive))
        {
            PdfReaderDialogHelper::message(this, m_config.openFailedText, error, m_config);
            return false;
        }
        if (!askForPassword(&password)) return false;
    }

}

bool PdfReader::openFile(const QString& filePath)
{
    QString path = filePath;
    if (path.isEmpty())
        path = PdfReaderDialogHelper::file(this, PdfReaderDialogParam::OpenFile, m_config.openDialogTitle, QString(), m_config.openFilter, m_config);
    if (path.isEmpty()) return false;
    if (!openWithPassword(path))
    {
        return false;
    }
    m_currentPath = path;
    m_thumbnails->clear();
    refreshDocument();
    m_thumbnails->setCurrentRow(0);
    m_thumbnails->verticalScrollBar()->setValue(0);
    m_pageScroll->verticalScrollBar()->setValue(0);
    m_pageScroll->horizontalScrollBar()->setValue(0);
    statusBar()->showMessage(QString::fromStdWString(CStringManager::Format(m_config.openedFormat.toStdWString().c_str(), QFileInfo(path).fileName().toStdWString().c_str(), m_core->pageCount())));
    return true;
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
    const int selected = qBound(0, selectedPage(), m_core->pageCount() - 1);
    const int scroll = m_thumbnails->verticalScrollBar()->value();
    m_thumbnails->blockSignals(true);
    m_thumbnails->clear();
    // 放大只扩大条目的灰色可点击背景，PDF 页图保持基准尺寸；缩小时仍保留旧版缩小能力。
    const double pageScale = qMin(1.0, m_thumbnailZoom);
    const int width = qMax(m_config.thumbnailMinWidth, qMin(qRound(m_config.thumbnailWidth * pageScale), m_thumbnails->viewport()->width() - m_config.thumbnailSidePadding));
    int maxHeight = 1;
    for (int i = 0; i < m_core->pageCount(); ++i)
    {
        PdfReaderCoreCPageInfo info;
        if (!m_core->pageInfo(i, &info) || info.width <= 0 || info.height <= 0)
        {
            info.width = m_config.fallbackPageWidth;
            info.height = m_config.fallbackPageHeight;
        }
        const int height = qMax(1, qRound(width * info.height / info.width));
        const int itemHeight = m_thumbnailZoom > 1.0 ?
            qRound((height + m_config.thumbnailRowPadding) * m_thumbnailZoom) : height + m_config.thumbnailRowPadding;
        QListWidgetItem* item = new QListWidgetItem(QString::number(i + 1), m_thumbnails);
        item->setTextAlignment(Qt::AlignHCenter);
        item->setData(Qt::UserRole, i);
        item->setData(Qt::UserRole + 1, QSize(width, height));
        item->setData(Qt::UserRole + 2, itemHeight);
        item->setSizeHint(QSize(qMax(m_config.thumbnailItemMinWidth, m_thumbnails->viewport()->width() - m_config.thumbnailItemSideInset), itemHeight));
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
            info.width = m_config.fallbackPageWidth;
            info.height = m_config.fallbackPageHeight;
        }
        Label* page = new Label(m_pageContainer);
        page->setObjectName(QStringLiteral("pageLabel"));
        page->setProperty("pageIndex", i);
        page->setFixedSize(qMax(1, qRound(info.width * m_zoom)) + 2, qMax(1, qRound(info.height * m_zoom)) + 2);
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
    // 使用实际视口，不创建整份文档的像素副本；离屏缓存即时释放。
    const QRect viewport(QPoint(0, 0), m_pageScroll->viewport()->size());
    for (int i = 0; i < m_core->pageCount(); ++i)
    {
        QLayoutItem* layoutItem = m_pageLayout->itemAt(i);
        Label* page = layoutItem ? dynamic_cast<Label*>(layoutItem->widget()) : nullptr;
        if (page)
        {
            const QRect rect(page->mapTo(m_pageScroll->viewport(), QPoint(0, 0)), page->size());
            if (rect.intersects(viewport))
            {
                if (!page->pixmap())
                {
                    QString error;
                    const QImage image = m_core->renderPage(i, page->width() - 2, page->height() - 2, &error);
                    if (image.isNull())
                    {
                        page->setText(error);
                    }
                    else
                    {
                        page->setPixmap(QPixmap::fromImage(image));
                    }
                }
            }
            else if (page->pixmap())
            {
                page->clear();
            }
        }
        QListWidgetItem* thumb = m_thumbnails->item(i);
        if (!thumb)
        {
            continue;
        }
        if (m_thumbnails->visualItemRect(thumb).intersects(m_thumbnails->viewport()->rect()))
        {
            if (thumb->icon().isNull())
            {
                const QSize size = thumb->data(Qt::UserRole + 1).toSize();
                const QPixmap pixmap = QPixmap::fromImage(m_core->renderPage(i, size.width(), size.height()));
                QIcon icon(pixmap);
                // 选中只改变外框，PDF 预览颜色不交给系统主题重新着色。
                icon.addPixmap(pixmap, QIcon::Selected);
                icon.addPixmap(pixmap, QIcon::Active);
                thumb->setIcon(icon);
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
    const bool open = m_core->isOpen();
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

bool PdfReader::showCoreError(const QString& operation)
{
    PdfReaderDialogHelper::message(this, operation, m_core->lastError(), m_config);
    return false;
}

void PdfReader::saveMain()
{
    if (!m_core->isOpen()) { return; }
    if (!PdfReaderDialogHelper::question(this, m_config.confirmSaveTitle,
        m_config.confirmSavePrompt + m_currentPath, m_config))
    {
        return;
    }
    QString error;
    const bool saved = m_core->saveToMain(&error);
    // 保存会关闭并重开源文档；失败恢复的 Core 状态也必须反映到界面。
    refreshDocument();
    if (!saved)
    {
        PdfReaderDialogHelper::message(this, m_config.saveFailedText, error, m_config);
    }
    else
    {
        statusBar()->showMessage(m_config.savedText, m_config.statusMessageMs);
    }
}

void PdfReader::saveAs()
{
    if (!m_core->isOpen()) { return; }
    const QString suggested = QFileInfo(m_currentPath).completeBaseName() + m_config.editedSuffix;
    const QString path = PdfReaderFileHelper::pdfOutputPath(PdfReaderDialogHelper::file(this, PdfReaderDialogParam::SaveFile, m_config.saveAsText, suggested, m_config.saveFilter, m_config));
    if (path.isEmpty()) { return; }
    if (QFileInfo(path).canonicalFilePath() == QFileInfo(m_currentPath).canonicalFilePath())
    {
        saveMain();
        return;
    }
    QString error;
    if (!m_core->saveTo(path, &error))
    {
        PdfReaderDialogHelper::message(this, m_config.saveFailedText, error, m_config);
    }
    else
    {
        statusBar()->showMessage(m_config.savedAsText, m_config.statusMessageMs);
    }
}

void PdfReader::savePageRange()
{
    if (!m_core->isOpen()) { return; }
    QString range = QStringLiteral("1-") + QString::number(m_core->pageCount());
    const bool accepted = PdfReaderDialogHelper::input(this, m_config.rangeTitle, m_config.rangeExample, range, false, m_config);
    if (!accepted || range.isEmpty()) { return; }
    QString error;
    if (!m_core->validatePageRange(range, &error))
    {
        PdfReaderDialogHelper::message(this, m_config.invalidRangeText, error, m_config);
        return;
    }
    const QString suggested = QFileInfo(m_currentPath).completeBaseName() + m_config.pagesSuffix;
    const QString path = PdfReaderFileHelper::pdfOutputPath(PdfReaderDialogHelper::file(this, PdfReaderDialogParam::SaveFile, m_config.saveRangeTitle, suggested, m_config.saveFilter, m_config));
    if (path.isEmpty()) { return; }
    if (!m_core->savePageRange(range, path, &error))
    {
        PdfReaderDialogHelper::message(this, m_config.exportFailedText, error, m_config);
    }
    else
    {
        statusBar()->showMessage(m_config.rangeSavedText, m_config.statusMessageMs);
    }
}

void PdfReader::saveEachPage()
{
    if (!m_core->isOpen()) { return; }
    const QString directory = PdfReaderDialogHelper::file(this, PdfReaderDialogParam::Directory, m_config.exportDirectoryTitle, QString(), QString(), m_config);
    if (directory.isEmpty()) { return; }
    QString error;
    const QString prefix = QFileInfo(m_currentPath).completeBaseName();
    bool saved = m_core->saveEachPage(directory, prefix, &error);
    if (!saved && error.contains(QStringLiteral("already exists")))
    {
        if (!PdfReaderDialogHelper::question(this, m_config.confirmOverwriteTitle,
            m_config.confirmOverwritePrompt + error, m_config))
        {
            return;
        }
        saved = m_core->saveEachPage(directory, prefix, &error, true);
    }
    if (!saved)
    {
        PdfReaderDialogHelper::message(this, m_config.exportFailedText, error, m_config);
    }
    else
    {
        statusBar()->showMessage(m_config.eachSavedText, m_config.statusMessageMs);
    }
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
    if (index < 0 || !m_core->isOpen())
    {
        return;
    }
    const QString path = PdfReaderDialogHelper::file(this, PdfReaderDialogParam::OpenFile, m_config.insertDialogTitle, QString(), m_config.saveFilter, m_config);
    if (path.isEmpty())
    {
        return;
    }
    QString password;
    QString error;
    while (!m_core->insertDocument(path, password, index, &error))
    {
        if (!error.contains(QStringLiteral("password"), Qt::CaseInsensitive))
        {
            PdfReaderDialogHelper::message(this, m_config.insertFailedText, error, m_config);
            return;
        }
        if (!askForPassword(&password))
        {
            return;
        }
    }
    refreshDocument();
    m_thumbnails->setCurrentRow(index, QItemSelectionModel::ClearAndSelect);
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
        statusBar()->showMessage(QString::fromStdWString(CStringManager::Format(m_config.pageStatusFormat.toStdWString().c_str(), selected + 1, m_core->pageCount())));
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
            m_pageScroll->ensureWidgetVisible(selectedPageWidget, m_config.selectionMargin, m_config.selectionMargin);
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
    if (hitItem) m_thumbnails->setCurrentItem(hitItem, QItemSelectionModel::ClearAndSelect);
    Menu menu(this);
    if (hitItem)
    {
        connect(menu.addAction(m_config.insertBeforeText), SIGNAL(triggered()), this, SLOT(insertBefore()));
        connect(menu.addAction(m_config.insertAfterText), SIGNAL(triggered()), this, SLOT(insertAfter()));
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
    QString error;
    if (!m_core->movePage(fromRow, toRow, &error))
    {
        PdfReaderDialogHelper::message(this, m_config.reorderFailedText, error, m_config);
        refreshDocument();
        return;
    }
    refreshDocument();
    m_thumbnails->setCurrentRow(toRow, QItemSelectionModel::ClearAndSelect);
}

void PdfReader::setZoom(double zoom)
{
    const double bounded = qBound(m_config.minimumZoom, zoom, m_config.maximumZoom);
    if (!m_core->isOpen() || qAbs(bounded - m_zoom) < 0.001)
    {
        return;
    }
    m_zoom = bounded;
    m_zoomResetAction->setText(QString::number(qRound(m_zoom * 100)) + QStringLiteral("%"));
    refreshPages();
}

void PdfReader::zoomIn()
{
    setZoom(m_zoom + m_config.zoomStep);
}

void PdfReader::zoomOut()
{
    setZoom(m_zoom - m_config.zoomStep);
}

void PdfReader::resetZoom()
{
    setZoom(m_config.initialZoom);
}

void PdfReader::showHelp()
{
    PdfReaderDialogHelper::message(this, m_config.aboutTitle,
                             m_config.aboutVersionText + QStringLiteral("\n") + m_config.aboutText, m_config);
}

void PdfReader::setWindowDocumentTitle()
{
    setWindowTitle(m_core->isOpen() ? m_config.documentTitlePrefix + QFileInfo(m_currentPath).fileName() : m_config.applicationTitle);
}

void PdfReader::closeEvent(QCloseEvent* event)
{
    if (m_core) m_core->shutdown();
    event->accept();
}

void PdfReader::wheelEvent(QWheelEvent* event)
{
    MainWindow::wheelEvent(event);
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
        if (wheel->modifiers() & Qt::ControlModifier)
        {
            if (wheel->delta() != 0 && m_core->isOpen())
            {
                if (thumbnail)
                {
                    m_thumbnailZoom = qBound(m_config.minimumThumbnailZoom, m_thumbnailZoom + wheel->delta() / 120.0 * m_config.thumbnailZoomStep, m_config.maximumThumbnailZoom);
                    refreshThumbnails();
                }
                else
                {
                    setZoom(m_zoom + wheel->delta() / 120.0 * m_config.zoomStep);
                }
            }
            wheel->accept();
            return true;
        }
    }
    return MainWindow::eventFilter(watched, event);
}