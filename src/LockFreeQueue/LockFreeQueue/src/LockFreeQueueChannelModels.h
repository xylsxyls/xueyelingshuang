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
    // 默认模式为已证明发布的前缀提示；预约模式为下次可预约位置，可能领先于载荷发布
    std::atomic<size_t> m_tail;
    // 发布段所属块或辅助登记中的索引
    size_t m_poolIndex;
    // 非回绕发布段代次，用于使旧位置提示失效
    uint64_t m_generation;
    // 所属块地址；nullptr表示由辅助登记独立分配，生命周期由池管理
    void* m_owner;
    // 唯一消费者拥有的退役链链接
    LockFreeQueueChannelSegment* m_retiredNext;
    // 连续空闲起点，单调毫秒；重新使用或保护后重置
    uint64_t m_idleSince;
    // 块内发布段按完整缓存行步进，避免旧段尾部与新段首槽共用缓存行
    unsigned char m_segmentPadding[sizeof(void*) == 8 ? 8 : (std::alignment_of<uint64_t>::value == 8 ? 24 : 28)];

public:
    /** 建立尚未发布的段元数据；首次使用槽位前必须在独占状态调用reset
    */
    LockFreeQueueChannelSegment();

    /** 重置独占或外部静止状态；不得与仍借用该对象的线程并发
    */
    void reset();

    /** 交错到十六组物理槽位，隔开相邻发布；64项或256项扫描均按逻辑顺序访问
    @param [in] position 逻辑槽位下标，范围0..255；不接受段末游标256
    @return 同一发布段内一一对应的物理槽位下标
    */
    static size_t mapped(size_t position);

};

#include "LockFreeQueueChannelModels.inl"