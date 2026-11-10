#pragma once

#include "LockFreeQueueAtomic.h"
#include <stddef.h>
#include <stdint.h>
#include <type_traits>

// Private publication core. Payload ownership/allocation belongs to the pool.
// A segment's cells are write-once until the segment is detached AND hazard-safe.
// No producer reads a payload through a cell; only the sole consumer does so.
template <class Node>
struct LockFreeQueueChannelSegment
{
public:
    // 每个发布段固定 256 个逻辑槽位
    static const size_t kCellCount = 256;
    // 写一次的发布槽，0 未发布，1 关闭标记，其他值为完整载荷地址
    LockFreeQueueAtomic<uintptr_t> m_cells[kCellCount];
    // 发布后的后继链接；读取遵守对应保护协议
    LockFreeQueueAtomic<LockFreeQueueChannelSegment*> m_next;
    // 已证明发布的前缀提示，只减少扫描；载荷发布 CAS 决定实际 FIFO 顺序
    std::atomic<size_t> m_tail;
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
    // 块内发布段按完整缓存行步进，避免旧段尾部与新段首槽共用缓存行
    unsigned char m_segmentPadding[sizeof(void*) == 8 ? 8 : (std::alignment_of<uint64_t>::value == 8 ? 24 : 28)];

public:
    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    */
    LockFreeQueueChannelSegment();

    /** 重置独占或外部静止状态；不得与仍借用该对象的线程并发
    */
    void reset();

    /** 交错到十六组物理槽位，隔开相邻发布并保留 64 项前缀的局部性
    @param [in] position 已验证的逻辑槽位下标
    @return 同一发布段内一一对应的物理槽位下标
    */
    static size_t mapped(size_t position);

};

#include "LockFreeQueueChannelModels.inl"