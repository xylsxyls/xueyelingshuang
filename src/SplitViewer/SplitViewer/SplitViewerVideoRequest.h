#pragma once
#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <QtGui/qwindowdefs.h>

/** 视频会话请求的异步阶段；已嵌入且受关闭策略管理的记录继续保留进程身份
*/
enum SplitViewerVideoRequestPhase
{
    // 请求已接纳，尚未创建播放器进程；下次轮询尝试启动
    SPLITVIEWER_VIDEO_QUEUED,
    // 播放器进程已创建，正在等待本次PID的可见窗口
    SPLITVIEWER_VIDEO_STARTING,
    // 已发现窗口并通知嵌入；成功接纳后按关闭策略继续跟踪稳定进程身份
    SPLITVIEWER_VIDEO_READY,
    // 已请求正常关闭，非阻塞观察退出；超过关闭预算后仅处理所持进程句柄
    SPLITVIEWER_VIDEO_CLOSING
};

/** 一个启动或关闭请求；进程句柄只由VideoSession中的记录释放
*/
struct SplitViewerVideoRequest
{
public:
    // 本会话的唯一请求编号
    quint64 m_id;
    // 本次程序绝对路径
    QString m_program;
    // 本次视频绝对路径
    QString m_path;
    // 借用的进程编号
    qint64 m_processId;
    // 本记录独占的稳定原生进程身份
    quintptr m_processToken;
    // 借用的播放器窗口，不负责释放
    WId m_windowId;
    // 单调时钟截止点，毫秒
    qint64 m_deadline;
    // 当前启动或关闭阶段
    SplitViewerVideoRequestPhase m_phase;
    // 是否已经请求过强制结束
    bool m_forced;

public:
    /** 初始化未启动请求，不持有进程资源
    */
    SplitViewerVideoRequest();
};