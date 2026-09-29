#include "Config.h"
#include <stdexcept>
#include <cmath>

Config& Config::instance()
{
    static Config s_config;
    return s_config;
}

Config::Config() :
m_titleCloseIconSize(24),
m_titleCloseStroke(1.2),
m_titleCloseActiveText(QColor(35,61,88)),
m_titleCloseDisabledText(QColor(150,165,180)),
m_aboutIconSize(32),
m_aboutIconColor(QColor(43,125,214)),
m_aboutIconStroke(2.6),
m_titleCloseHeight(40),
m_titleCloseSize(34),
m_titleCloseRight(4),
m_titleCloseTop(4),
m_titleCloseRadius(6),
m_titleCloseColor(QColor(63,84,108)),
m_titleCloseHover(QColor(215,228,241)),
m_titleClosePressed(QColor(199,216,233)),
m_titleCloseHoverBorder(QColor(191,210,227)),
m_titleClosePressedBorder(QColor(176,199,219)),
m_emptyPlusSize(42),
m_emptyPlusMargin(8),
m_emptyPlusHalfLength(10),
m_emptyPlusStroke(3),
m_emptyPlusBackground(QColor(238,238,238)),
m_emptyPlusBorder(QColor(96,96,96)),
m_emptyPlusColor(QColor(55,65,81)),
m_pageBorderWidth(1),
m_zoomComparisonTolerance(0.001),
m_maxPendingRenders(16),
m_dialogType(10731),
m_core(),
m_log(),
m_dialogSize(QSize(540, 260)),
m_aboutDialogSize(QSize(540, 320)),
m_fileDialogSize(QSize(820, 580)),
m_dialogShadowEnabled(true),
m_dialogShadowSize(2),
m_dialogTitleBarHeight(32),
m_useNativeFileDialog(true),
m_dialogMargin(20),
m_dialogSpacing(12),
m_dialogStyle(QStringLiteral("QWidget#pdfReaderDialogView{background:#f5f7fb;}QLabel{color:#283548;}QLabel#dialogTitle{font-size:18px;font-weight:bold;}QPushButton{padding:7px 18px;background:#e4ebf7;border:1px solid #becde3;border-radius:4px;}QPushButton:focus{border:2px solid #306fd2;}QLineEdit{padding:6px;background:white;border:1px solid #becde3;}")),
m_dialogAcceptText(QStringLiteral("确定")),
m_dialogCancelText(QStringLiteral("取消")),
m_buttonNormal(QColor(48,111,210)),
m_buttonHover(QColor(65,129,230)),
m_buttonPressed(QColor(34,84,166)),
m_buttonDisabled(QColor(163,179,204)),
m_buttonTextColor(QColor(255,255,255)),
m_buttonSize(QSize(100,34)),
m_buttonRadius(5),
m_thumbnailItemMinWidth(60),
m_thumbnailItemSideInset(18),
m_thumbnailImagePaddingX(12),
m_thumbnailImagePaddingY(8),
m_dragGhostInset(4),
m_dragGhostBottomInset(24),
m_dragLineOffset(3),
m_dragLineWidth(3),
m_dragLineInset(8),
m_dragGhostColor(QColor(100,170,255,72)),
m_dragLineColor(QColor(49,105,200)),
m_bridgeCreateError(QStringLiteral("PdfReaderCore句柄创建失败")),
m_bridgeInternalError(QStringLiteral("PdfReaderCore内部错误")),
m_pageParameterError(QStringLiteral("页面参数无效")),
m_renderParameterError(QStringLiteral("页面渲染参数无效")),
m_windowSize(QSize(1180, 780)),
m_minimumWindowSize(QSize(860, 560)),
m_sidebarMinimumWidth(150),
m_sidebarWidth(210),
m_bodyWidth(830),
m_bodyMarginX(28),
m_bodyMarginY(24),
m_bodySpacing(22),
m_selectionMargin(12),
m_thumbnailWidth(112),
m_thumbnailMinWidth(42),
m_thumbnailSidePadding(32),
m_thumbnailRowPadding(32),
m_thumbnailLabelHeight(22),
m_statusMessageMs(3000),
m_dragHoldMs(500),
m_dragScrollMs(16),
m_dragWheelPixels(60),
m_dragEdgePixels(70),
m_dragMinSpeed(4),
m_dragMaxSpeed(46),
m_dragAcceleration(3),
m_initialZoom(1.0),
m_minimumZoom(0.25),
m_maximumZoom(4.0),
m_zoomStep(0.05),
m_initialThumbnailZoom(1.0),
m_minimumThumbnailZoom(0.5),
m_maximumThumbnailZoom(2.0),
m_thumbnailZoomStep(0.1),
m_fallbackPageWidth(612.0),
m_fallbackPageHeight(792.0),
m_thumbnailSelectedColor(QColor(226,235,250)),
m_thumbnailBackground(QColor(238,241,245)),
m_selectionColor(QColor(48,111,210)),
m_thumbnailBorderColor(QColor(192,198,207)),
m_thumbnailTextColor(QColor(72,81,95)),
m_aboutTooltip(QStringLiteral("关于与使用说明")),
m_applicationTitle(QStringLiteral("PDF阅读器")),
m_windowStyle(QStringLiteral("QMainWindow{background:#f2f4f7;}QToolBar{background:#ffffff;border:0;border-bottom:1px solid #d9dee7;padding:6px;spacing:5px;}QToolButton{color:#283548;padding:6px 10px;border-radius:5px;}QToolButton:hover{background:#e8eef8;}QListWidget{background:#e8edf4;border:0;padding:12px;}QListWidget::item{background:#ffffff;border:1px solid #d8dee9;border-radius:5px;padding:5px;color:#374151;}QListWidget::item:selected{border:2px solid #3b82f6;background:#eef5ff;}QScrollArea{background:#dfe5ed;border:0;}QLabel#pageLabel{background:#ffffff;border:1px solid #d0d6df;}QMenu{background:#ffffff;color:#283548;border:1px solid #cbd5e1;padding:4px;}QMenu::item{padding:6px 18px;}QMenu::item:selected{background:#e8eef8;color:#283548;}QMenu::item:disabled{color:#94a3b8;}")),
m_toolbarTitle(QStringLiteral("文件")),
m_openText(QStringLiteral("打开")),
m_saveText(QStringLiteral("保存")),
m_saveAsText(QStringLiteral("另存为")),
m_exportRangeText(QStringLiteral("导出范围")),
m_exportEachText(QStringLiteral("逐页导出")),
m_zoomOutText(QStringLiteral("缩小")),
m_zoomResetText(QStringLiteral("100%")),
m_zoomInText(QStringLiteral("放大")),
m_readyText(QStringLiteral("就绪")),
m_passwordTitle(QStringLiteral("输入密码")),
m_passwordPrompt(QStringLiteral("该 PDF 需要密码：")),
m_openFailedText(QStringLiteral("打开失败")),
m_openDialogTitle(QStringLiteral("打开 PDF")),
m_openFilter(QStringLiteral("PDF 文件 (*.pdf);;所有文件 (*.*)")),
m_confirmSaveTitle(QStringLiteral("确认覆盖原 PDF")),
m_confirmSavePrompt(QStringLiteral("当前页面顺序将替换原文件内容，是否继续？\n")),
m_saveFailedText(QStringLiteral("保存失败")),
m_savedText(QStringLiteral("已保存并覆盖原 PDF")),
m_editedSuffix(QStringLiteral("_edited.pdf")),
m_saveFilter(QStringLiteral("PDF 文件 (*.pdf)")),
m_savedAsText(QStringLiteral("已另存为 PDF")),
m_rangeTitle(QStringLiteral("导出页面范围")),
m_rangeExample(QStringLiteral("例如：3-5,7-8")),
m_invalidRangeText(QStringLiteral("页码范围无效")),
m_pagesSuffix(QStringLiteral("_pages.pdf")),
m_saveRangeTitle(QStringLiteral("保存页面范围")),
m_exportFailedText(QStringLiteral("导出失败")),
m_rangeSavedText(QStringLiteral("已导出页面范围")),
m_exportDirectoryTitle(QStringLiteral("选择导出目录")),
m_confirmOverwriteTitle(QStringLiteral("确认覆盖")),
m_confirmOverwritePrompt(QStringLiteral("导出目录中已有同名 PDF，是否覆盖？\n")),
m_eachSavedText(QStringLiteral("已逐页导出")),
m_insertDialogTitle(QStringLiteral("插入 PDF")),
m_insertFailedText(QStringLiteral("插入失败")),
m_selectedPageStyle(QStringLiteral("background:white;border:%dpx solid #306fd2;")),
m_normalPageStyle(QStringLiteral("background:white;border:%dpx solid #808691;")),
m_insertBeforeText(QStringLiteral("在此页之前插入")),
m_insertAfterText(QStringLiteral("在此页之后插入")),
m_reorderFailedText(QStringLiteral("调整页面顺序失败")),
m_aboutTitle(QStringLiteral("PDF阅读器")),
m_aboutVersionText(QStringLiteral("1.0版本")),
m_aboutText(QStringLiteral("1. 页面缩略图：打开 PDF 后在左侧查看页面缩略图。\n2. 页面查看：右侧显示正文，支持缩放和滚动。\n3. 保存与导出：支持保存、另存为、范围导出和逐页导出。\n4. 页面调整：可插入 PDF，并拖动缩略图调整页面顺序。")),
m_documentTitlePrefix(QStringLiteral("PDF阅读器 - ")),
m_openedFormat(QStringLiteral("已打开：%s，共 %d 页")),
m_pageStatusFormat(QStringLiteral("第 %d / %d 页"))
{
    init();
}

void Config::init()
{
    pdfReaderCoreDefaultConfig(&m_core);
    m_log.m_maxFileBytes = 20LL * 1024 * 1024;
    m_log.m_maxFileCount = 8;
    m_log.m_checkFileSizeInterval = 1;
    m_log.m_outputConsole = false;
    m_log.m_archiveOldLog = true;
}

void Config::validate() const
{
    if (m_aboutIconSize <= 0 || m_titleCloseIconSize <= 0 || !std::isfinite(m_aboutIconStroke) || m_aboutIconStroke <= 0 ||
        !std::isfinite(m_titleCloseStroke) || m_titleCloseStroke <= 0 || m_maxPendingRenders <= 0)
    {
        throw std::invalid_argument("invalid PdfReader task configuration");
    }
    if (m_titleCloseHeight <= 0 || m_titleCloseSize <= 0 || m_titleCloseSize > m_titleCloseHeight ||
        m_titleCloseTop < 0 || m_titleCloseTop > m_titleCloseHeight - m_titleCloseSize || m_titleCloseRight < 0 || m_emptyPlusSize <= 0 || m_emptyPlusMargin < 0 ||
        m_emptyPlusHalfLength <= 0 || m_emptyPlusStroke <= 0 || m_pageBorderWidth < 0 ||
        !std::isfinite(m_zoomComparisonTolerance) || m_zoomComparisonTolerance < 0)
    {
        throw std::invalid_argument("invalid PdfReader drawing configuration");
    }
    if (m_dialogSize.isEmpty() || m_aboutDialogSize.isEmpty() || m_fileDialogSize.isEmpty() || m_buttonSize.isEmpty() || m_dialogMargin < 0 || m_dialogSpacing < 0 || m_dialogShadowSize < 0 || m_dialogTitleBarHeight < 0)
    {
        throw std::invalid_argument("invalid PdfReader dialog configuration");
    }
    if (!std::isfinite(m_minimumZoom) || !std::isfinite(m_maximumZoom) || !std::isfinite(m_initialZoom) || !std::isfinite(m_zoomStep) ||
        m_minimumZoom <= 0 || m_maximumZoom < m_minimumZoom || m_maximumZoom > 16.0 || m_initialZoom < m_minimumZoom || m_initialZoom > m_maximumZoom || m_zoomStep <= 0 ||
        !std::isfinite(m_minimumThumbnailZoom) || !std::isfinite(m_maximumThumbnailZoom) || !std::isfinite(m_initialThumbnailZoom) || !std::isfinite(m_thumbnailZoomStep) ||
        m_minimumThumbnailZoom <= 0 || m_maximumThumbnailZoom < m_minimumThumbnailZoom || m_initialThumbnailZoom < m_minimumThumbnailZoom || m_initialThumbnailZoom > m_maximumThumbnailZoom || m_thumbnailZoomStep <= 0 ||
        m_dragHoldMs <= 0 || m_dragWheelPixels <= 0 || m_dragScrollMs <= 0 || m_dragEdgePixels <= 0 || m_dragMinSpeed <= 0 || m_dragMaxSpeed < m_dragMinSpeed || m_dragAcceleration <= 0 ||
        m_sidebarMinimumWidth <= 0 || m_sidebarWidth <= 0 || m_bodyWidth <= 0 || m_thumbnailWidth <= 0 || m_thumbnailMinWidth <= 0 || m_thumbnailMinWidth > m_thumbnailWidth ||
        m_thumbnailRowPadding < m_thumbnailLabelHeight || m_thumbnailLabelHeight <= 0 || m_thumbnailSidePadding < 0 || m_bodyMarginX < 0 || m_bodyMarginY < 0 || m_bodySpacing < 0 ||
        m_windowSize.isEmpty() || m_minimumWindowSize.isEmpty() || !std::isfinite(m_fallbackPageWidth) || !std::isfinite(m_fallbackPageHeight) || m_fallbackPageWidth <= 0 || m_fallbackPageHeight <= 0 ||
        m_thumbnailItemMinWidth <= 0 || m_thumbnailItemSideInset < 0 || m_thumbnailImagePaddingX < 0 || m_thumbnailImagePaddingY < 0 ||
        m_dragGhostInset < 0 || m_dragGhostBottomInset < 0 || m_dragLineOffset < 0 || m_dragLineWidth <= 0 || m_dragLineInset < 0 || m_statusMessageMs < 0 || m_selectionMargin < 0)
    {
        throw std::invalid_argument("invalid PdfReader UI configuration");
    }
}