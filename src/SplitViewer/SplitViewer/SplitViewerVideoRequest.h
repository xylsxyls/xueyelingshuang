#pragma once
#include <QtCore/QString>
#include <QtCore/QtGlobal>
#include <QtGui/qwindowdefs.h>

enum SplitViewerVideoRequestPhase
{
    SPLITVIEWER_VIDEO_QUEUED,
    SPLITVIEWER_VIDEO_STARTING,
    SPLITVIEWER_VIDEO_READY,
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