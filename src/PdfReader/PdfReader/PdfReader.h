#ifndef PDFREADER_H
#define PDFREADER_H

#include "Config.h"
#include "PdfReaderThumbnailList.h"
#include "PdfReaderThumbnailDelegate.h"
#include "QtControls/MainWindow.h"
#include <QtWidgets/QListWidget>
#include "QtControls/Label.h"
#include "QtControls/ScrollArea.h"
#include "QtControls/Splitter.h"
#include "QtControls/ToolBar.h"
#include <QtWidgets/QAction>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QStyledItemDelegate>

class QVBoxLayout;
class QCloseEvent;
class QWheelEvent;
class QTimer;
class QMouseEvent;
class QPaintEvent;
class QKeyEvent;
class QStyledItemDelegate;
class QPainter;

class PdfReaderCoreBridge;

/** 桌面窗口：编排文档交互与视图，通过 C API 桥接访问 Core，不持有 PDF 引擎。 */
class PdfReader : public MainWindow
{
    Q_OBJECT
public:
    explicit PdfReader(QWidget* parent = nullptr, const Config& config = Config());
    ~PdfReader();

public slots:
    bool openFile(const QString& filePath = QString());

protected:
    void closeEvent(QCloseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void chooseAndOpen();
    void saveMain();
    void saveAs();
    void savePageRange();
    void saveEachPage();
    void insertBefore();
    void insertAfter();
    void showHelp();
    void onSelectionChanged();
    void onThumbnailContextMenu(const QPoint& position);
    void onThumbnailReordered(int fromRow, int toRow);
    void zoomIn();
    void zoomOut();
    void resetZoom();
    /** 仅渲染视口相交的缩略图和正文，释放离开视口的位图 */
    void renderVisiblePages();

private:
    void buildUi();
    void refreshDocument();
    Q_SLOT void refreshThumbnails();
    void refreshPages();
    void updateActions();
    bool openWithPassword(const QString& filePath);
    bool askForPassword(QString* password);
    bool showCoreError(const QString& operation);
    int selectedPage() const;
    void setWindowDocumentTitle();
    /** 共用前后插入流程，成功后选择第一张插入页 */
    void insertDocument(int index);
    /** 改变正文缩放，范围 25% 到 400%，保留滚动偏移 */
    void setZoom(double zoom);

private:
    const Config m_config;
    PdfReaderCoreBridge* m_core;
    PdfReaderThumbnailList* m_thumbnails;
    ScrollArea* m_pageScroll;
    QWidget* m_pageContainer;
    QVBoxLayout* m_pageLayout;
    Label* m_emptyLabel;
    ToolBar* m_toolbar;
    QAction* m_openAction;
    QAction* m_saveAction;
    QAction* m_saveAsAction;
    QAction* m_saveRangeAction;
    QAction* m_saveEachAction;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_zoomResetAction;
    QAction* m_helpAction;
    double m_zoom;
    double m_thumbnailZoom;
    QString m_currentPath;
};

#endif