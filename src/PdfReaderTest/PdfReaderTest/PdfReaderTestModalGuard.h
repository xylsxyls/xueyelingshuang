#pragma once
#include <QTimer>
#include <QElapsedTimer>
#include <QPointer>
#include <QWidget>

/** 每次弹窗最多5秒；防止测试缺少按钮或行为变化造成无限嵌套事件循环 */
class PdfReaderTestModalGuard
{
public:
    /** 在当前GUI线程启动观察，不创建业务线程 */
    PdfReaderTestModalGuard();
    // 任一弹窗超时后保留失败记录，不能因强行关闭算通过
    bool m_timedOut;
private:
    // 同GUI线程定时观察，析构自动停止
    QTimer m_timer;
    QElapsedTimer m_elapsed;
    QPointer<QWidget> m_modal;
};