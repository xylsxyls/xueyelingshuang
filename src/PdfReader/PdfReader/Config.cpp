#include "Config.h"
#include <stdexcept>
#include <cmath>

Config::Config() :
dialogSize(QSize(540, 260)),
aboutDialogSize(QSize(540, 320)),
fileDialogSize(QSize(820, 580)),
dialogShadowEnabled(true),
dialogShadowSize(2),
dialogTitleBarHeight(32),
useNativeFileDialog(true),
dialogMargin(20),
dialogSpacing(12),
dialogStyle(QStringLiteral("QWidget#pdfReaderDialogView{background:#f5f7fb;}QLabel{color:#283548;}QLabel#dialogTitle{font-size:18px;font-weight:bold;}QPushButton{padding:7px 18px;background:#e4ebf7;border:1px solid #becde3;border-radius:4px;}QPushButton:focus{border:2px solid #306fd2;}QLineEdit{padding:6px;background:white;border:1px solid #becde3;}")),
dialogAcceptText(QStringLiteral("确定")),
dialogCancelText(QStringLiteral("取消")),
buttonNormal(QColor(48,111,210)),
buttonHover(QColor(65,129,230)),
buttonPressed(QColor(34,84,166)),
buttonDisabled(QColor(163,179,204)),
buttonTextColor(QColor(255,255,255)),
buttonSize(QSize(100,34)),
buttonRadius(5),
thumbnailItemMinWidth(60),
thumbnailItemSideInset(18),
thumbnailImagePaddingX(12),
thumbnailImagePaddingY(8),
dragGhostInset(4),
dragGhostBottomInset(24),
dragLineOffset(3),
dragLineWidth(3),
dragLineInset(8),
dragGhostColor(QColor(210,215,224,220)),
dragLineColor(QColor(49,105,200)),
bridgeCreateError(QStringLiteral("PdfReaderCore句柄创建失败")),
bridgeInternalError(QStringLiteral("PdfReaderCore内部错误")),
pageParameterError(QStringLiteral("页面参数无效")),
renderParameterError(QStringLiteral("页面渲染参数无效")),
windowSize(QSize(1180, 780)),
minimumWindowSize(QSize(860, 560)),
sidebarMinimumWidth(150),
sidebarWidth(210),
bodyWidth(830),
bodyMarginX(28),
bodyMarginY(24),
bodySpacing(22),
selectionMargin(12),
thumbnailWidth(112),
thumbnailMinWidth(42),
thumbnailSidePadding(32),
thumbnailRowPadding(32),
thumbnailLabelHeight(22),
statusMessageMs(3000),
dragHoldMs(1000),
dragScrollMs(16),
dragEdgePixels(70),
dragMinSpeed(4),
dragMaxSpeed(46),
dragAcceleration(3),
initialZoom(1.0),
minimumZoom(0.25),
maximumZoom(4.0),
zoomStep(0.05),
initialThumbnailZoom(1.0),
minimumThumbnailZoom(0.5),
maximumThumbnailZoom(2.0),
thumbnailZoomStep(0.1),
fallbackPageWidth(612.0),
fallbackPageHeight(792.0),
thumbnailSelectedColor(QColor(226,235,250)),
thumbnailBackground(QColor(238,241,245)),
selectionColor(QColor(48,111,210)),
thumbnailBorderColor(QColor(192,198,207)),
thumbnailTextColor(QColor(72,81,95)),
aboutTooltip(QStringLiteral("关于与使用说明")),
applicationTitle(QStringLiteral("PDF阅读器")),
windowStyle(QStringLiteral("QMainWindow{background:#f2f4f7;}QToolBar{background:#ffffff;border:0;border-bottom:1px solid #d9dee7;padding:6px;spacing:5px;}QToolButton{color:#283548;padding:6px 10px;border-radius:5px;}QToolButton:hover{background:#e8eef8;}QListWidget{background:#e8edf4;border:0;padding:12px;}QListWidget::item{background:#ffffff;border:1px solid #d8dee9;border-radius:5px;padding:5px;color:#374151;}QListWidget::item:selected{border:2px solid #3b82f6;background:#eef5ff;}QScrollArea{background:#dfe5ed;border:0;}QLabel#pageLabel{background:#ffffff;border:1px solid #d0d6df;}QMenu{background:#ffffff;color:#283548;border:1px solid #cbd5e1;padding:4px;}QMenu::item{padding:6px 18px;}QMenu::item:selected{background:#e8eef8;color:#283548;}QMenu::item:disabled{color:#94a3b8;}")),
toolbarTitle(QStringLiteral("文件")),
openText(QStringLiteral("打开")),
saveText(QStringLiteral("保存")),
saveAsText(QStringLiteral("另存为")),
exportRangeText(QStringLiteral("导出范围")),
exportEachText(QStringLiteral("逐页导出")),
zoomOutText(QStringLiteral("缩小")),
zoomResetText(QStringLiteral("100%")),
zoomInText(QStringLiteral("放大")),
emptyDocumentText(QStringLiteral("打开一个 PDF 开始阅读")),
emptyDocumentStyle(QStringLiteral("color:#64748b;font-size:20px;padding:100px;")),
readyText(QStringLiteral("就绪")),
passwordTitle(QStringLiteral("输入密码")),
passwordPrompt(QStringLiteral("该 PDF 需要密码：")),
openFailedText(QStringLiteral("打开失败")),
openDialogTitle(QStringLiteral("打开 PDF")),
openFilter(QStringLiteral("PDF 文件 (*.pdf);;所有文件 (*.*)")),
confirmSaveTitle(QStringLiteral("确认覆盖原 PDF")),
confirmSavePrompt(QStringLiteral("当前页面顺序将替换原文件内容，是否继续？\n")),
saveFailedText(QStringLiteral("保存失败")),
savedText(QStringLiteral("已保存并覆盖原 PDF")),
editedSuffix(QStringLiteral("_edited.pdf")),
saveFilter(QStringLiteral("PDF 文件 (*.pdf)")),
savedAsText(QStringLiteral("已另存为 PDF")),
rangeTitle(QStringLiteral("导出页面范围")),
rangeExample(QStringLiteral("例如：3-5,7-8")),
invalidRangeText(QStringLiteral("页码范围无效")),
pagesSuffix(QStringLiteral("_pages.pdf")),
saveRangeTitle(QStringLiteral("保存页面范围")),
exportFailedText(QStringLiteral("导出失败")),
rangeSavedText(QStringLiteral("已导出页面范围")),
exportDirectoryTitle(QStringLiteral("选择导出目录")),
confirmOverwriteTitle(QStringLiteral("确认覆盖")),
confirmOverwritePrompt(QStringLiteral("导出目录中已有同名 PDF，是否覆盖？\n")),
eachSavedText(QStringLiteral("已逐页导出")),
insertDialogTitle(QStringLiteral("插入 PDF")),
insertFailedText(QStringLiteral("插入失败")),
selectedPageStyle(QStringLiteral("background:white;border:1px solid #306fd2;")),
normalPageStyle(QStringLiteral("background:white;border:1px solid #808691;")),
insertBeforeText(QStringLiteral("在此页之前插入")),
insertAfterText(QStringLiteral("在此页之后插入")),
reorderFailedText(QStringLiteral("调整页面顺序失败")),
aboutTitle(QStringLiteral("PDF阅读器")),
aboutVersionText(QStringLiteral("1.0版本")),
aboutText(QStringLiteral("1. 页面缩略图：打开 PDF 后在左侧查看页面缩略图。\n2. 页面查看：右侧显示正文，支持缩放和滚动。\n3. 保存与导出：支持保存、另存为、范围导出和逐页导出。\n4. 页面调整：可插入 PDF，并拖动缩略图调整页面顺序。")),
documentTitlePrefix(QStringLiteral("PDF阅读器 - ")),
openedFormat(QStringLiteral("已打开：%s，共 %d 页")),
pageStatusFormat(QStringLiteral("第 %d / %d 页"))
{
    pdfReaderCoreDefaultConfig(&core);
    log.m_maxFileBytes = 20LL * 1024 * 1024;
    log.m_maxFileCount = 8;
    log.m_checkFileSizeInterval = 1;
    log.m_outputConsole = false;
    log.m_archiveOldLog = true;
}

void Config::validate() const
{
    if (!dialogSize.isValid() || !aboutDialogSize.isValid() || !fileDialogSize.isValid() || !buttonSize.isValid() || dialogMargin < 0 || dialogSpacing < 0 || dialogShadowSize < 0 || dialogTitleBarHeight < 0)
        throw std::invalid_argument("invalid PdfReader dialog configuration");
    if (!std::isfinite(minimumZoom) || !std::isfinite(maximumZoom) || !std::isfinite(initialZoom) || !std::isfinite(zoomStep) ||
        minimumZoom <= 0 || maximumZoom < minimumZoom || maximumZoom > 16.0 || initialZoom < minimumZoom || initialZoom > maximumZoom || zoomStep <= 0 ||
        !std::isfinite(minimumThumbnailZoom) || !std::isfinite(maximumThumbnailZoom) || !std::isfinite(initialThumbnailZoom) || !std::isfinite(thumbnailZoomStep) ||
        minimumThumbnailZoom <= 0 || maximumThumbnailZoom < minimumThumbnailZoom || initialThumbnailZoom < minimumThumbnailZoom || initialThumbnailZoom > maximumThumbnailZoom || thumbnailZoomStep <= 0 ||
        dragHoldMs <= 0 || dragScrollMs <= 0 || dragEdgePixels <= 0 || dragMinSpeed <= 0 || dragMaxSpeed < dragMinSpeed || dragAcceleration <= 0 ||
        sidebarMinimumWidth <= 0 || sidebarWidth <= 0 || bodyWidth <= 0 || thumbnailWidth <= 0 || thumbnailMinWidth <= 0 || thumbnailMinWidth > thumbnailWidth ||
        thumbnailRowPadding < thumbnailLabelHeight || thumbnailLabelHeight <= 0 || thumbnailSidePadding < 0 || bodyMarginX < 0 || bodyMarginY < 0 || bodySpacing < 0 ||
        !windowSize.isValid() || !minimumWindowSize.isValid() || !std::isfinite(fallbackPageWidth) || !std::isfinite(fallbackPageHeight) || fallbackPageWidth <= 0 || fallbackPageHeight <= 0 ||
        thumbnailItemMinWidth <= 0 || thumbnailItemSideInset < 0 || thumbnailImagePaddingX < 0 || thumbnailImagePaddingY < 0 ||
        dragGhostInset < 0 || dragGhostBottomInset < 0 || dragLineOffset < 0 || dragLineWidth <= 0 || dragLineInset < 0 || statusMessageMs < 0 || selectionMargin < 0)
    {
        throw std::invalid_argument("invalid PdfReader UI configuration");
    }
}