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
/** 进程级鼠标钩子与运行状态管理；仅由GUI线程访问。 */
class SplitViewerNativeMouseManager
{
public:
    /** 获取唯一钩子上下文，生命周期覆盖所有宿主窗口
    @return 计算结果或借用对象，具体语义见函数说明
    */
    static SplitViewerNativeMouseManager& instance();
    // 当前宿主借用的外部窗口句柄，不负责销毁原窗口
public:
    QSet<HWND> m_embeddedWindows;
    // 各窗口上一次点击时刻
    QMap<HWND,DWORD> m_lastClickTicks;
    // 各窗口上一次点击位置
    QMap<HWND,QPoint> m_lastClickPoints;
    // 需要抑制双击抬起事件的窗口
    QSet<HWND> m_suppressButtonUp;
    // 等待单击或双击判定的原生输入
    QMap<HWND, SplitViewerNativeClick> m_pendingClicks;
    // 拒绝已撤销点击的单调代次
    quint64 m_clickGeneration;
    // 本管理器拥有的鼠标钩子，最后客户端退出时释放
    HHOOK m_mouseHook;
    // 借用GUI接收者；销毁前注销，其上下文控制回调有效期
    QMap<QObject*,std::function<void(int,const QPoint&,WId)> > m_mouseClients;
private:
    /** 初始化对象及其默认状态
    */
    SplitViewerNativeMouseManager();

    /** 释放本对象持有的资源
    */
    ~SplitViewerNativeMouseManager();

    /** 禁止复制钩子所有权
    @param [in] other 不可复制的源对象
    */
    SplitViewerNativeMouseManager(const SplitViewerNativeMouseManager& other);

    /** 禁止复制赋值
    @param [in] other 不可复制的源对象
    @return 不提供实现
    */
    SplitViewerNativeMouseManager& operator=(const SplitViewerNativeMouseManager& other);
};
#endif