#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <functional>

/** 有限命令任务；结果通过调用方的Qt队列连接返回GUI */
class TestCommandTask : public CTask
{
public:
    /** 保存只捕获有明确生命周期的命令
    @param [in] command 待执行命令，不操作GUI控件
    */
    explicit TestCommandTask(const std::function<void(const std::atomic<bool>&)>& command);

    /** 安全边界执行一次命令 */
    virtual void DoTask();

    /** 可重复请求退出，不等待线程 */
    virtual void StopTask();

private:
    // 协作退出标记
    std::atomic<bool> m_exit;
    // 窗口退出事件循环后先回收本线程，再析构Qt对象
    std::function<void(const std::atomic<bool>&)> m_command;
};