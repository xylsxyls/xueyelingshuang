#pragma once
#include "CTaskThreadManager/CTask.h"
#include <atomic>
#include <functional>

/** 将有限次压力工作交给统一任务线程管理器，不创建业务裸线程。
*/
class CookServerTestStressTask : public CTask
{
public:
    /** 构造压力任务。
    @param [in] operation 接收协作取消标志的工作函数
    */
    explicit CookServerTestStressTask(const std::function<void(const std::atomic<bool>&)>& operation);
    /** 在线程管理器上执行工作；异常记入任务结果。
    */
    virtual void DoTask();
    /** 请求终止后续循环；已开始的HTTP请求受网络超时保护。
    */
    virtual void StopTask();
    /** 查询工作是否发生未捕获异常。
    @return 返回异常标志
    */
    bool failed() const;
private:
    enum TaskId { StressWork = 1 };
    std::function<void(const std::atomic<bool>&)> m_operation;
    std::atomic<bool> m_stop;
    std::atomic<bool> m_failed;
};