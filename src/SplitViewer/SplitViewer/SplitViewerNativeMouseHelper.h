#pragma once
#include "SplitViewerPlatform.h"
#include "SplitViewerNativeMouseConfig.h"

/** 原生鼠标事件分发及嵌入窗口单双击仲裁。 */
class SplitViewerNativeMouseHelper
{
public:
#ifdef Q_OS_WIN
    /** 向原目标重放已判定的单击；拖动开始时仅重放按下。
    @param [in] embedded 已登记的嵌入窗口
    @param [in] downOnly 是否仅重放按下
    */
    static void replayClick(HWND embedded, bool downOnly);
#endif
#ifdef Q_OS_WIN
    /** 查找已登记的嵌入祖先窗口；找不到返回空。
    @param [in] window 待查询的窗口
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static HWND embeddedAncestor(HWND window);
#endif
#ifdef Q_OS_WIN
    /** 仲裁单双击并将鼠标通知排入接收者GUI事件循环。
    @param [in] code 系统钩子通知代码
    @param [in] message 系统鼠标消息
    @param [in] data 输入数据
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static LRESULT CALLBACK mouseHook(int code,WPARAM message,LPARAM data);
#endif
    /** 登记GUI线程接收者，按需安装进程级鼠标钩子。
    @param [in] owner 父区域或事件接收者
    @param [in] callback GUI线程通知回调
    */
    static void watchNativeMouse(QObject* owner,const std::function<void(int,const QPoint&,WId)>& callback);
    /** 移除接收者；无接收者时卸载钩子。
    @param [in] owner 父区域或事件接收者
    */
    static void unwatchNativeMouse(QObject* owner);
};