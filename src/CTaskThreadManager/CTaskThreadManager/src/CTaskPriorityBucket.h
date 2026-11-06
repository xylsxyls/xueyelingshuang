#pragma once
#include "CTaskThreadModels.h"
#include "CTaskAdmission.h"
#include "CTaskRecord.h"

/** 一个优先级的 MPSC FIFO；取出、重做槽和维护只由工作线程访问
*/
class CTaskPriorityBucket
{
public:
    /** 建立空桶
    @param [in] level 优先级
    */
    explicit CTaskPriorityBucket(int32_t level);

    /** 静止期配置队列
    @param [in] config 每线程配置
    @return 初始化结果
    */
    CTaskThreadResult initialize(const CTaskThreadConfig& config);
    // 不变优先级
    const int32_t m_level;
    // 队列生产者的接纳计数；关闭空桶后拒绝旧引用发布
    CTaskAdmission m_writers;
    // 接纳计数不与 FIFO 的首个缓存行共用
    char m_writerPadding[64];
    // 单消费者队列
    LockFreeQueue<std::shared_ptr<CTaskRecord>> m_queue;
    // 仅消费者访问，优先于本级 FIFO；重做不重新分配队列节点
    std::shared_ptr<CTaskRecord> m_redo;
    // 仅消费者访问的一个已发布队首；用于检查优先级，不批量转移或反转
    std::shared_ptr<CTaskRecord> m_front;
    // 消费者独占引用与生产者可写计数隔离
    char m_frontPadding[64];
    // 排队数量，包含正在发布的预留和重做槽
    std::atomic<uint64_t> m_queued;
};