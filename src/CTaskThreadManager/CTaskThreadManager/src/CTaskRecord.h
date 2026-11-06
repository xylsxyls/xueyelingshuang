#pragma once
#include "CTask.h"
#include <memory>
#include <atomic>

/** 已发布后载荷不再修改的任务记录；目录和 FIFO 各自持有独立强引用
*/
class CTaskRecord
{
public:
    /** 构造尚未提交的记录
    @param [in] task 任务强引用
    @param [in] sequence 不回绕序号
    @param [in] level 原始优先级
    */
    CTaskRecord(const std::shared_ptr<CTask>& task, uint64_t sequence, int32_t level);

public:
    // 不可变任务强引用
    const std::shared_ptr<CTask> m_task;
    // 不复用的任务记录编号
    const uint64_t m_sequence;
    // 不可变任务优先级
    const int32_t m_level;
    // 0 未提交/重做预备，1 排队，2 执行，3 完成或拒绝
    std::atomic<int32_t> m_phase;
    // 位 0 请求重做，位 1 已经完成本次执行的决策
    std::atomic<uint32_t> m_flags;
};