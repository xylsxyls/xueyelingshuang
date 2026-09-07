#pragma once

#include "PdfReaderPlatform.h"

// 主窗口类名
static const wchar_t kWindowClass[] = L"PdfReaderWindow";
// 缩略图区域子窗口类名
static const wchar_t kThumbClass[] = L"PdfReaderThumbView";
// PDF正文区域子窗口类名
static const wchar_t kDocumentClass[] = L"PdfReaderDocumentView";
// 简单输入弹窗窗口类名
static const wchar_t kPromptClass[] = L"PdfReaderPrompt";
// 关于弹窗窗口类名
static const wchar_t kAboutClass[] = L"PdfReaderAbout";
// 顶部按钮提示窗口类名
static const wchar_t kButtonToolTipClass[] = L"PdfReaderButtonToolTip";
// 软件标题
static const wchar_t kAppTitle[] = L"PDF阅读器";
// 软件版本号
static const wchar_t kAppVersion[] = L"1.0";

// 缩略图拖拽时自动滚动定时器ID
static const UINT_PTR kTimerDragScroll = 1;
// 缩略图长按开始拖拽定时器ID
static const UINT_PTR kTimerDragStart = 2;
// 缩略图长按触发拖拽的等待时间
static const UINT kThumbnailDragHoldMilliseconds = 1000;
// 缩略图长按检查定时器间隔
static const UINT kThumbnailDragHoldTimerInterval = 50;
// 自绘标题栏高度
static const int kTitleHeight = 32;
// 标题栏下方工具栏高度
static const int kToolbarHeight = 32;
// 左侧缩略图区和右侧正文区之间的分隔条宽度
static const int kSplitterWidth = 5;
// 左侧缩略图区最小宽度
static const int kLeftMinWidth = 150;
// 右侧正文区最小宽度
static const int kRightMinWidth = 260;
// 主窗口最小宽度
static const int kWindowMinWidth = 760;
// 主窗口最小高度
static const int kWindowMinHeight = 520;
// 标题栏按钮宽度
static const int kTitleButtonWidth = 42;
// 工具栏按钮边长
static const int kToolButtonSize = 24;
// 标题栏按钮数量
static const int kTitleButtonCount = 3;
// 工具栏按钮数量
static const int kToolbarButtonCount = 4;
// 顶部按钮提示窗口背景色
static const COLORREF kToolTipBackgroundColor = RGB(255, 253, 232);
// 顶部按钮提示窗口边框色
static const COLORREF kToolTipBorderColor = RGB(174, 151, 92);
// 顶部按钮提示窗口文字色
static const COLORREF kToolTipTextColor = RGB(47, 48, 52);
// 顶部按钮提示窗口最小宽度
static const int kToolTipMinWidth = 64;
// 顶部按钮提示窗口最小高度
static const int kToolTipMinHeight = 28;
// 顶部按钮提示窗口相对鼠标的水平偏移
static const int kToolTipOffsetX = 14;
// 顶部按钮提示窗口相对鼠标的垂直偏移
static const int kToolTipOffsetY = 20;
// 顶部按钮提示窗口文本水平边距
static const int kToolTipPaddingX = 8;
// 顶部按钮提示窗口文本垂直边距
static const int kToolTipPaddingY = 4;
// 顶部按钮提示窗口文本宽度补偿
static const int kToolTipWidthPadding = 18;
// 顶部按钮提示窗口文本高度补偿
static const int kToolTipHeightPadding = 10;
// 简单输入弹窗窗口宽度
static const int kPromptDialogWidth = 360;
// 简单输入弹窗窗口高度
static const int kPromptDialogHeight = 150;
// 简单输入弹窗内容水平边距
static const int kPromptContentMarginX = 14;
// 简单输入弹窗标签顶部坐标
static const int kPromptLabelTop = 16;
// 简单输入弹窗标签高度
static const int kPromptLabelHeight = 20;
// 简单输入弹窗输入框顶部坐标
static const int kPromptEditTop = 44;
// 简单输入弹窗输入框高度
static const int kPromptEditHeight = 24;
// 简单输入弹窗按钮顶部坐标
static const int kPromptButtonTop = 82;
// 简单输入弹窗按钮宽度
static const int kPromptButtonWidth = 74;
// 简单输入弹窗按钮高度
static const int kPromptButtonHeight = 26;
// 简单输入弹窗按钮右侧边距
static const int kPromptButtonRightMargin = 18;
// 简单输入弹窗按钮之间的间距
static const int kPromptButtonGap = 8;
// 简单输入弹窗编辑框控件ID
static const int kPromptEditControlId = 1001;
// 关于弹窗窗口宽度
static const int kAboutDialogWidth = 430;
// 关于弹窗窗口高度
static const int kAboutDialogHeight = 292;
// 关于弹窗内容水平边距
static const int kAboutContentMarginX = 24;
// 关于弹窗正文顶部坐标
static const int kAboutTextTop = 20;
// 关于弹窗正文底部预留高度
static const int kAboutTextBottomReserve = 58;
// 关于弹窗按钮宽度
static const int kAboutButtonWidth = 84;
// 关于弹窗按钮高度
static const int kAboutButtonHeight = 26;
// 关于弹窗按钮右侧边距
static const int kAboutButtonRightMargin = 24;
// 关于弹窗按钮底部边距
static const int kAboutButtonBottomMargin = 18;
// 模态弹窗背景色
static const COLORREF kDialogBackgroundColor = RGB(255, 255, 255);
// 模态弹窗正文文字色
static const COLORREF kDialogTextColor = RGB(48, 56, 70);
// 禁用DWM窗口过渡动画的属性编号
static const DWORD kDwmTransitionsForcedDisabledAttribute = 3;
// 缩略图内容边距
static const int kThumbPadding = 12;
// 缩略图条目间距
static const int kThumbGap = 14;
// 缩略图页码标签高度
static const int kThumbLabelHeight = 18;
// 正文页面边距
static const int kDocPadding = 22;
// 正文页面间距
static const int kDocGap = 18;
// 普通滚轮滚动步长
static const int kScrollStep = 42;
// 正文Ctrl+滚轮缩放步长
static const int kZoomStepDoc = 5;
// 缩略图Ctrl+滚轮缩放步长
static const int kZoomStepThumb = 10;
// 缩略图最小缩放百分比
static const int kMinThumbScale = 50;
// 缩略图最大缩放百分比
static const int kMaxThumbScale = 200;
// 正文最小缩放百分比
static const int kMinDocZoom = 25;
// 正文最大缩放百分比
static const int kMaxDocZoom = 400;

/** 顶部标题栏和工具栏按钮ID
*/
enum TopButton
{
    TOP_BUTTON_NONE = 0,
    TOP_BUTTON_OPEN = 1,
    TOP_BUTTON_SAVE = 2,
    TOP_BUTTON_SAVE_AS = 3,
    TOP_BUTTON_MINIMIZE = 4,
    TOP_BUTTON_MAXIMIZE = 5,
    TOP_BUTTON_CLOSE = 6,
    TOP_BUTTON_HELP = 7
};

/** 左侧缩略图区右键菜单命令ID
*/
enum MenuCommand
{
    IDM_THUMB_INSERT_BEFORE = 41001,
    IDM_THUMB_INSERT_AFTER = 41002,
    IDM_LEFT_SAVE_EACH_PAGE = 41003,
    IDM_LEFT_SAVE_PAGE_RANGE = 41004
};