#pragma once
#include <QtCore/QPoint>
#include <QtGui/qwindowdefs.h>
#ifdef Q_OS_WIN
/** Mouse click held until it can be distinguished from a host double click
*/
struct SplitViewerNativeClick
{
public:
    // 借用的原生消息目标
    WId m_target;
    // 按下时屏幕坐标
    QPoint m_screenPoint;
    // 按下时客户区坐标
    QPoint m_clientPoint;
    // 系统按下时键盘标志
    quint32 m_modifiers;
    // 是否已收到抬起
    bool m_released;
    // 取消判定使用的唯一代次
    quint64 m_generation;

public:
    /** 初始化尚未释放的空点击记录
    */
    SplitViewerNativeClick();
};
#endif