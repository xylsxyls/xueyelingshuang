#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <functional>
#include <string>

/** 有限任务适配器；异常必须回到请求失败处理，不逃出工作线程
*/
class PdfReaderTask : public CTask
{
public:
    /** 保存一次任务及失败出口，不在构造时执行工作
    @param [in] action 任务体，段间检查取消标志
    @param [in] failure 异常出口，不能抛异常
    */
    PdfReaderTask(const std::function<void(const std::atomic<bool>&)>& action,
        const std::function<void(const std::string&)>& failure);

    /** 执行并捕获所有异常
    */
    void DoTask() override;

    /** 仅提交取消标志，不等待或释放资源
    */
    void StopTask() override;
private:
    // 仅此任务的协作停止标志
    std::atomic<bool> m_exit;
    // 工作及异常处理，生命周期由任务引用保证
    std::function<void(const std::atomic<bool>&)> m_action;
    std::function<void(const std::string&)> m_failure;
};