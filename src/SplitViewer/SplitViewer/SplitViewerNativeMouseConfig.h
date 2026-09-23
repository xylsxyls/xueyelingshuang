#pragma once
#include "SplitViewerNativeClick.h"
#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QObject>
#include <functional>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
/** 进程级鼠标钩子配置与运行状态；仅由GUI线程访问。 */
class SplitViewerNativeMouseConfig
{
public:
    /** 获取唯一钩子上下文，生命周期覆盖所有宿主窗口。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static SplitViewerNativeMouseConfig& instance();
    QSet<HWND> embeddedWindows;
    QMap<HWND,DWORD> lastClickTicks;
    QMap<HWND,QPoint> lastClickPoints;
    QSet<HWND> suppressButtonUp;
    QMap<HWND, SplitViewerNativeClick> pendingClicks;
    quint64 clickGeneration;
    HHOOK mouseHook;
    QMap<QObject*,std::function<void(int,const QPoint&,WId)> > mouseClients;
private:
    /** 初始化对象及其默认状态。
    */
    SplitViewerNativeMouseConfig();
    /** 释放本对象持有的资源。
    */
    ~SplitViewerNativeMouseConfig();
    /** 初始化对象及其默认状态。
    */
    SplitViewerNativeMouseConfig(const SplitViewerNativeMouseConfig&);
    SplitViewerNativeMouseConfig& operator=(const SplitViewerNativeMouseConfig&);
};
#endif