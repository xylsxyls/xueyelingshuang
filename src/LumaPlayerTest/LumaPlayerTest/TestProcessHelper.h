#pragma once
#include <stdint.h>

/** 进程窗口观察适配，非Windows实现明确返回不支持 */
class TestProcessHelper
{
public:
    /** 观察指定子进程可见顶层窗口
    @param [in] pid 测试程序创建的进程ID
    @return 是否发现可见窗口
    */
    static bool hasWindow(int64_t pid);

    /** 向指定子进程窗口发送正常关闭消息
    @param [in] pid 受控测试进程ID
    @return 是否找到窗口并发送
    */
    static bool closeWindow(int64_t pid);
};

/** Win32枚举窗口时的短期参数，不保存窗口所有权 */
struct TestWindowQuery
{
public:
    // 系统进程ID
    int64_t m_pid;
    // 是否发送关闭
    bool m_close;
    // 是否发现窗口
    bool m_found;
};