#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

/** 基准仅适配详细结果接口的名称，不改变被测版本的实际投递规则
*/
class CTaskPostCompatibility
{
public:
    /** 当前版本保留提交状态和后续通知诊断
    @param [in,out] thread 被测工作线程
    @param [in] task 预先构造的任务
    @param [in] level 任务优先级
    @param [in] selection 编译期选择当前入口
    @return 被测版本的原始结果
    */
    template <typename Thread>
    static auto submit(Thread& thread, const std::shared_ptr<CTask>& task, int32_t level, int selection)
        -> decltype(thread.postTaskResult(task, level));

    /** 旧版本没有详细接口时使用原入口
    @param [in,out] thread 被测工作线程
    @param [in] task 预先构造的任务
    @param [in] level 任务优先级
    @param [in] selection 编译期回退选择
    @return 被测版本的原始结果
    */
    template <typename Thread>
    static auto submit(Thread& thread, const std::shared_ptr<CTask>& task, int32_t level, long selection)
        -> decltype(thread.postTask(task, level));
};

#include "CTaskPostCompatibility.inl"