#pragma once
#include "CTaskThreadModels.h"
#include "CTaskAdmission.h"
#include "CTaskStorageModels.h"

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

public:
    // 不变优先级
    const int32_t m_level;
    // 队列生产者的接纳计数；关闭空桶后拒绝旧引用发布
    CTaskAdmission m_writers;
    // 接纳计数不与 FIFO 的首个缓存行共用
    char m_writerPadding[64];
    // 单消费者队列
    LockFreeQueue<CTaskStorageSlot*> m_queue;
    // 仅消费者访问，优先于本级 FIFO；重做不重新分配队列节点
    CTaskStorageSlot* m_redo;
    // 仅消费者访问的一个已发布队首；执行之前仍重新选择最高优先级
    CTaskStorageSlot* m_front;
    // 消费者本地前缀的下一个位置；与队首指针相邻，空桶探测不跨越载荷数组
    size_t m_readyHead;
    // 消费者本地前缀的有效项数，尚未取出的范围为 [m_readyHead, m_readyCount)
    size_t m_readyCount;
    // 最多预读 64 个记录基类地址，合并归还 FIFO 额度；记录仍计入原优先级排队数
    LockFreeQueueEntry<CTaskStorageSlot*>* m_readyEntries[64];
    // 消费者独占引用与生产者可写计数隔离
    char m_frontPadding[64];
    // 排队数量，包含正在发布的预留和重做槽
    std::atomic<uint64_t> m_queued;
    // 生产者累计量不与消费者取出量争用缓存行
    char m_countPadding[64];
    // 常用桶只由消费者写，和各生产者的分级累计量作模差
    std::atomic<uint64_t> m_taken;
    // 隔离消费者写入的取出计数
    char m_readyPadding[64];

};