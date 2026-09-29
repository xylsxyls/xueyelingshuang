#ifndef PDFREADER_H
#define PDFREADER_H

#include "Config.h"
#include "PdfReaderSession.h"
#include <QSet>
#include "PdfReaderThumbnailList.h"
#include "PdfReaderThumbnailDelegate.h"
#include "PdfReaderEmptyState.h"
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

/** 桌面窗口：编排文档交互与视图，通过 C API 桥接访问 Core，不持有 PDF 引擎
*/
class PdfReader : public MainWindow
{
    Q_OBJECT
public:
    /** 建立桌面视图和异步会话，不在GUI初始化PDF引擎
    @param [in] parent Qt父对象，可空，父子所有权由Qt管理
    */
    explicit PdfReader(QWidget* parent = nullptr);

    /** 断开GUI回调并异步释放Core会话
    */
    ~PdfReader();

public slots:
    /** 提交打开请求，不等待Core；完成后发送operationFinished
    @param [in] filePath 文件路径；空时选择文件
    @return 接纳返回true，取消选择或忙碌时返回false
    */
    bool openFile(const QString& filePath = QString());
public:
    /** 查询实际请求和渲染是否全部收尾
    @return 无在途请求返回true
    */
    bool idle() const;

    /** 最近完成业务请求的成功状态，不能作为提交回执
    @return 实际成功返回true
    */
    bool lastOperationSucceeded() const;
signals:
    /** 业务请求实际完成；密码重试等会生成新的请求
    @param [in] requestId 会话内唯一请求序号
    @param [in] success 本次实际成功标志
    */
    void operationFinished(quint64 requestId, bool success);

protected:
    /** 等待异步Core释放终态后接受关闭，不在GUI等待线程
    @param [in] event 当前Qt事件，调用期间借用
    */
    void closeEvent(QCloseEvent* event) override;

    /** 转发缩放、页选择和尺寸变化交互
    @param [in] watched 事件所属Qt对象
    @param [in] event 当前Qt事件，调用期间借用
    @return 已消费事件返回true，否则委托基类
    */
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    /** QueuedConnection进入GUI，处理实际完成回报
    */
    void processCoreResults();

    /** 选择文件并提交打开请求
    */
    void chooseAndOpen();

    /** 确认后异步覆盖原PDF
    */
    void saveMain();

    /** 选择输出路径并异步另存为
    */
    void saveAs();

    /** 按有效范围输出PDF
    */
    void savePageRange();

    /** 逐页输出，覆盖须显式允许
    */
    void saveEachPage();

    /** 在当前页之前选择并插入文档
    */
    void insertBefore();

    /** 在当前页之后选择并插入文档
    */
    void insertAfter();

    /** 显示DialogManager管理的关于弹窗
    */
    void showHelp();

    /** 由左侧选择同步正文焦点和可见位置
    */
    void onSelectionChanged();

    /** 按命中条目显示QtControls右键菜单
    @param [in] position 缩略图视口坐标
    */
    void onThumbnailContextMenu(const QPoint& position);

    /** 提交当前拖动的页序调整，失败恢复实际页序
    @param [in] fromRow 拖动源行
    @param [in] toRow 拖动目标行
    */
    void onThumbnailReordered(int fromRow, int toRow);

    /** 按配置步进放大正文
    */
    void zoomIn();

    /** 按配置步进缩小正文
    */
    void zoomOut();

    /** 恢复配置的默认正文倍率
    */
    void resetZoom();

    /** 同步滚动后的可见页焦点并请求当前视口图像
    */
    void onPageScrollChanged();

    /** 仅渲染视口相交的缩略图和正文，释放离开视口的位图
    */
    void renderVisiblePages();

private:
    /** 按配置建立QtControls控件和交互连接
    */
    void buildUi();

    /** 用已完成的元数据快照重建视图
    */
    void refreshDocument();

    /** 按当前倍率重建单列缩略图条目并保持选择
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    Q_SLOT void refreshThumbnails();

    /** 重建正文页尺寸并使旧预览代次失效
    */
    void refreshPages();

    /** 按实际文档和在途状态更新按钮可用性
    */
    void updateActions();

    /** 更新唯一选中页边框并按需滚动正文
    @param [in] ensureVisible 是否将选中正文页滚动到可见位置
    */
    void updateSelectionState(bool ensureVisible);

    /** 选择视口内最可见的页面，联动左侧且避免滚动回环
    */
    void syncSelectionFromPageScroll();

    /** 接纳业务操作并统一更新动作状态
    @param [in] request 操作参数
    @param [in] completion GUI完成回调
    @return 是否接纳
    */
    bool submitOperation(const PdfReaderRequest& request, const std::function<void(const PdfReaderResult&)>& completion);

    /** 打开或插入，失败只在GUI询问密码并显式重试
    @param [in] request 打开/插入参数
    @return 是否接纳
    */
    bool openWithPassword(const PdfReaderRequest& request);

    /** 导出处理，遇到冲突时在GUI询问是否重试覆盖
    @param [in] request 保存参数
    @param [in] successText 成功提示
    */
    void exportDocument(const PdfReaderRequest& request, const QString& successText);

    /** 请求当前视图的图像，限制在途数量并隔离旧视图
    @param [in] index 页码
    @param [in] size 像素尺寸
    @param [in] thumbnail 是否用于缩略图
    */
    void requestImage(int32_t index, const QSize& size, bool thumbnail);

    /** 通过DialogManager询问密码，取消不改变输出
    @param [in] password 密码；空字符串表示未提供
    @return 操作成功返回true；失败返回false
    */
    bool askForPassword(QString* password);

    /** 读取当前缩略图选择
    @return 当前状态或结果值；返回对象的所有权保持不变
    */
    int selectedPage() const;

    /** 按已完成的文档更新中文标题和文件名
    */
    void setWindowDocumentTitle();

    /** 共用前后插入流程，成功后选择第一张插入页
    */
    void insertDocument(int index);

    /** 改变正文缩放，范围 25% 到 400%，保留滚动偏移
    */
    void setZoom(double zoom);

private:
    // GUI会话门面；PDF资源在唯一执行者操作及释放
    std::unique_ptr<PdfReaderSession> m_core;
    // 关闭须等待异步资源释放回报；GUI不join
    bool m_closeRequested;
    bool m_closeReady;
    bool m_lastOperationSucceeded;
    // GUI视图代次及在途/失败图像键，均不跨线程
    uint64_t m_viewGeneration;
    QSet<QString> m_pendingImages;
    QSet<QString> m_failedImages;
    PdfReaderThumbnailList* m_thumbnails;
    ScrollArea* m_pageScroll;
    QWidget* m_pageContainer;
    QVBoxLayout* m_pageLayout;
    PdfReaderEmptyState* m_emptyState;
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
    bool m_pageRefreshInProgress;
    bool m_selectionScrollInProgress;
    QString m_currentPath;
};

#endif