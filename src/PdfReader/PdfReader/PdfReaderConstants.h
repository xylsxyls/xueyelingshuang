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
// 软件标题
static const wchar_t kAppTitle[] = L"PDF阅读器";
// 软件版本号
static const wchar_t kAppVersion[] = L"1.0";

// 缩略图拖拽时自动滚动定时器ID
static const UINT_PTR kTimerDragScroll = 1;
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
static const int kToolbarButtonCount = 3;
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
    TOP_BUTTON_MINIMIZE = 3,
    TOP_BUTTON_MAXIMIZE = 4,
    TOP_BUTTON_CLOSE = 5,
    TOP_BUTTON_HELP = 6
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
