#pragma once
#include "SplitViewerTaskContext.h"
#include "CTaskThreadManager/CTask.h"

/** 隔离窗口数据的有限业务任务
*/
class SplitViewerLogicTask : public CTask
{
public:
    /** 捕获请求的强引用，不创建线程或执行工作
    @param [in] context 关闭期仍然存活的队列上下文
    @param [in] request 独立输入与完成协议
    @param [in] completion 是否处理执行队列返回的实际结果
    */
    explicit SplitViewerLogicTask(const std::shared_ptr<SplitViewerTaskContext>& context,
        const std::shared_ptr<SplitViewerTaskRequest>& request, bool completion = false);

    /** 执行有限步骤，异常转换为可观察的失败
    */
    void DoTask();

    /** 幂等提出协作停止，不等待或释放共享数据
    */
    void StopTask();

private:
    // 整个任务期间保持上下文有效
    std::shared_ptr<SplitViewerTaskContext> m_context;
    // 当前请求，只在有序步骤中修改结果
    std::shared_ptr<SplitViewerTaskRequest> m_request;
    // 区分提交与完成阶段
    bool m_completion;
    // 本任务自身的协作停止标志
    std::atomic<bool> m_exit;
};