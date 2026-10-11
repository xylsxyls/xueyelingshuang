#pragma once
#include "SplitViewerTaskContext.h"
#include "CTaskThreadManager/CTask.h"

/** 只回收其他队列的协调任务，最后由main外层回收本队列
*/
class SplitViewerRetireTask : public CTask
{
public:
    /** 捕获请求的强引用，不创建线程或执行工作
    @param [in] context 关闭期仍然存活的队列上下文
    */
    explicit SplitViewerRetireTask(const std::shared_ptr<SplitViewerTaskContext>& context);

    /** 执行有限步骤，异常转换为可观察的失败
    */
    void DoTask();

    /** 幂等提出协作停止，不等待或释放共享数据
    */
    void StopTask();

private:
    // 整个任务期间保持上下文有效
    std::shared_ptr<SplitViewerTaskContext> m_context;
    // 协调回收为不可中断的清理提交段；记录停止但仍必须完成ID回收
    std::atomic<bool> m_exit;
};