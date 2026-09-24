#pragma once
#include <QString>
#include <QSize>
#include <QColor>
#define PDFREADERCORE_USE_C_API
#include "PdfReaderCore/PdfReaderCoreAPI.h"
#include "LogManager/LogManagerAPI.h"

/** 桌面配置值对象。main/QApplication 初始化后创建，窗口复制快照；不含静态初始化业务资源 */
class Config
{
public:
    /** 创建安全默认配置；Core 默认值来自同一 C API */
    Config();
    /** 检查缩放、布局及拖动边界；无效配置抛出 invalid_argument */
    void validate() const;
public:
    // 五端 Core 的初始化参数，独立于 Qt 布局
    PdfReaderCoreCConfig core;
    // 默认日志：EXE 所在目录，20 MB 滚动、保留 8 个文件、归档旧日志
    LogManagerConfig log;
    // 弹窗默认尺寸、边距、样式及按钮文字
    QSize dialogSize;
    QSize aboutDialogSize;
    QSize fileDialogSize;
    bool dialogShadowEnabled;
    int dialogShadowSize;
    int dialogTitleBarHeight;
    // useNativeFileDialog 配置；默认使用操作系统原生文件窗口，测试可关闭
    bool useNativeFileDialog;
    int dialogMargin;
    int dialogSpacing;
    QString dialogStyle;
    QString dialogAcceptText;
    QString dialogCancelText;
    // 公用按钮的四态配色、尺寸及圆角
    QColor buttonNormal;
    QColor buttonHover;
    QColor buttonPressed;
    QColor buttonDisabled;
    QColor buttonTextColor;
    QSize buttonSize;
    unsigned int buttonRadius;
    // thumbnailItemMinWidth 配置
    int thumbnailItemMinWidth;
    // thumbnailItemSideInset 配置
    int thumbnailItemSideInset;
    // thumbnailImagePaddingX 配置
    int thumbnailImagePaddingX;
    // thumbnailImagePaddingY 配置
    int thumbnailImagePaddingY;
    // dragGhostInset 配置
    int dragGhostInset;
    // dragGhostBottomInset 配置
    int dragGhostBottomInset;
    // dragLineOffset 配置
    int dragLineOffset;
    // dragLineWidth 配置
    int dragLineWidth;
    // dragLineInset 配置
    int dragLineInset;
    // dragGhostColor 配置
    QColor dragGhostColor;
    // dragLineColor 配置
    QColor dragLineColor;
    // bridgeCreateError 配置
    QString bridgeCreateError;
    // bridgeInternalError 配置
    QString bridgeInternalError;
    // pageParameterError 配置
    QString pageParameterError;
    // renderParameterError 配置
    QString renderParameterError;

    // windowSize 配置
    QSize windowSize;
    // minimumWindowSize 配置
    QSize minimumWindowSize;
    // sidebarMinimumWidth 配置
    int sidebarMinimumWidth;
    // sidebarWidth 配置
    int sidebarWidth;
    // bodyWidth 配置
    int bodyWidth;
    // bodyMarginX 配置
    int bodyMarginX;
    // bodyMarginY 配置
    int bodyMarginY;
    // bodySpacing 配置
    int bodySpacing;
    // selectionMargin 配置
    int selectionMargin;
    // thumbnailWidth 配置
    int thumbnailWidth;
    // thumbnailMinWidth 配置
    int thumbnailMinWidth;
    // thumbnailSidePadding 配置
    int thumbnailSidePadding;
    // thumbnailRowPadding 配置
    int thumbnailRowPadding;
    // thumbnailLabelHeight 配置
    int thumbnailLabelHeight;
    // statusMessageMs 配置
    int statusMessageMs;
    // dragHoldMs 配置
    int dragHoldMs;
    // dragScrollMs 配置
    int dragScrollMs;
    // dragEdgePixels 配置
    int dragEdgePixels;
    // dragMinSpeed 配置
    int dragMinSpeed;
    // dragMaxSpeed 配置
    int dragMaxSpeed;
    // dragAcceleration 配置
    int dragAcceleration;
    // initialZoom 配置
    double initialZoom;
    // minimumZoom 配置
    double minimumZoom;
    // maximumZoom 配置
    double maximumZoom;
    // zoomStep 配置
    double zoomStep;
    // initialThumbnailZoom 配置
    double initialThumbnailZoom;
    // minimumThumbnailZoom 配置
    double minimumThumbnailZoom;
    // maximumThumbnailZoom 配置
    double maximumThumbnailZoom;
    // thumbnailZoomStep 配置
    double thumbnailZoomStep;
    // fallbackPageWidth 配置
    double fallbackPageWidth;
    // fallbackPageHeight 配置
    double fallbackPageHeight;
    // thumbnailSelectedColor 配置
    QColor thumbnailSelectedColor;
    // thumbnailBackground 配置
    QColor thumbnailBackground;
    // selectionColor 配置
    QColor selectionColor;
    // thumbnailBorderColor 配置
    QColor thumbnailBorderColor;
    // thumbnailTextColor 配置
    QColor thumbnailTextColor;
    // aboutTooltip 配置
    QString aboutTooltip;
    // applicationTitle 配置
    QString applicationTitle;
    // windowStyle 配置
    QString windowStyle;
    // toolbarTitle 配置
    QString toolbarTitle;
    // openText 配置
    QString openText;
    // saveText 配置
    QString saveText;
    // saveAsText 配置
    QString saveAsText;
    // exportRangeText 配置
    QString exportRangeText;
    // exportEachText 配置
    QString exportEachText;
    // zoomOutText 配置
    QString zoomOutText;
    // zoomResetText 配置
    QString zoomResetText;
    // zoomInText 配置
    QString zoomInText;
    // emptyDocumentText 配置
    QString emptyDocumentText;
    // emptyDocumentStyle 配置
    QString emptyDocumentStyle;
    // readyText 配置
    QString readyText;
    // passwordTitle 配置
    QString passwordTitle;
    // passwordPrompt 配置
    QString passwordPrompt;
    // openFailedText 配置
    QString openFailedText;
    // openDialogTitle 配置
    QString openDialogTitle;
    // openFilter 配置
    QString openFilter;
    // confirmSaveTitle 配置
    QString confirmSaveTitle;
    // confirmSavePrompt 配置
    QString confirmSavePrompt;
    // saveFailedText 配置
    QString saveFailedText;
    // savedText 配置
    QString savedText;
    // editedSuffix 配置
    QString editedSuffix;
    // saveFilter 配置
    QString saveFilter;
    // savedAsText 配置
    QString savedAsText;
    // rangeTitle 配置
    QString rangeTitle;
    // rangeExample 配置
    QString rangeExample;
    // invalidRangeText 配置
    QString invalidRangeText;
    // pagesSuffix 配置
    QString pagesSuffix;
    // saveRangeTitle 配置
    QString saveRangeTitle;
    // exportFailedText 配置
    QString exportFailedText;
    // rangeSavedText 配置
    QString rangeSavedText;
    // exportDirectoryTitle 配置
    QString exportDirectoryTitle;
    // confirmOverwriteTitle 配置
    QString confirmOverwriteTitle;
    // confirmOverwritePrompt 配置
    QString confirmOverwritePrompt;
    // eachSavedText 配置
    QString eachSavedText;
    // insertDialogTitle 配置
    QString insertDialogTitle;
    // insertFailedText 配置
    QString insertFailedText;
    // selectedPageStyle 配置
    QString selectedPageStyle;
    // normalPageStyle 配置
    QString normalPageStyle;
    // insertBeforeText 配置
    QString insertBeforeText;
    // insertAfterText 配置
    QString insertAfterText;
    // reorderFailedText 配置
    QString reorderFailedText;
    // aboutTitle 配置
    QString aboutTitle;
    // aboutVersionText 配置
    QString aboutVersionText;
    // aboutText 配置
    QString aboutText;
    // documentTitlePrefix 配置
    QString documentTitlePrefix;
    // openedFormat 配置
    QString openedFormat;
    // pageStatusFormat 配置
    QString pageStatusFormat;
};