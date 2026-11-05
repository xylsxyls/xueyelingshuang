#pragma once

#include "LockFreeQueueAtomic.h"
#include <stddef.h>
#include <stdint.h>

// Private publication core. Payload ownership/allocation belongs to the pool.
// A segment's cells are write-once until the segment is detached AND hazard-safe.
// No producer reads a payload through a cell; only the sole consumer does so.
template <class Node>
struct LockFreeQueueChannelSegment
{
public:
    // 每个发布段固定 256 个逻辑槽位
    static const size_t kCellCount = 256;
    // 写一次的发布槽，0 空，1 结束标记，其他值为完整载荷地址
    LockFreeQueueAtomic<uintptr_t> m_cells[kCellCount];
    // 发布后的后继链接；读取遵守对应保护协议
    LockFreeQueueAtomic<LockFreeQueueChannelSegment*> m_next;
    // 发布尾段或已验证的前缀提示，均不预占载荷位置
    LockFreeQueueAtomic<size_t> m_tail;
    // 发布段所属块或辅助登记中的索引
    size_t m_poolIndex;
    // 非回绕发布段代次，用于使旧位置提示失效
    uint64_t m_generation;
    // 借用的所属对象或带有效标志的完整所有者地址，见所属类型契约
    void* m_owner;
    // 唯一消费者拥有的退役链链接
    LockFreeQueueChannelSegment* m_retiredNext;
    // 连续空闲起点，单调毫秒；重新使用或保护后重置
    uint64_t m_idleSince;

public:
    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    */
    LockFreeQueueChannelSegment();

    /** 重置独占或外部静止状态；不得与仍借用该对象的线程并发
    */
    void reset();

    /** 将逻辑发布位置映射到分散缓存行的物理槽位
    @param [in] position 已验证的逻辑槽位下标
    @return 上述操作对应的数值、状态或弱一致快照
    */
    static size_t mapped(size_t position);
};

#include "LockFreeQueueChannelModels.inl"