#pragma once
#include "CTask.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <atomic>
#include <memory>
#include <stddef.h>

struct CTaskStoragePage;
struct CTaskStorageBlock;
class CTaskPriorityBucket;

/** 与发布载荷分离的紧凑执行状态；正常入队不写入，消费者回收时复位
*/
struct CTaskStorageState
{
public:
    // 1 待提交或排队，2 执行，3 完成；还须结合所在页的独立发布位
    std::atomic<int32_t> m_phase;
    // 位 0 请求重做；位 1 已完成本次重做决策
    std::atomic<uint32_t> m_flags;

public:
    /** 建立未提交状态，所在页的发布位初始为 false
    */
    CTaskStorageState();

};

/** 地址在池批次存活期间不变的执行记录；只有安全回收后才能重写载荷
*/
struct CTaskStorageSlot
{
public:
    // 发布后不可改写的任务引用，安全回收时释放
    std::shared_ptr<CTask> m_task;
    // 所在页中的执行状态，地址在本页生命周期内不变
    CTaskStorageState* m_state;
    // 原始任务优先级
    int32_t m_level;
    // 页内索引，0 至 63
    uint32_t m_index;
    // 本槽所在页，池销毁前有效
    CTaskStoragePage* m_page;
    // 排队时借给 Queue；pop 返回后 value/cookie 复用为退休链后继和 epoch
    // 生产者发布后不再访问此对象，槽本身仍由读域保护；x64 槽大小保持 64 字节
    LockFreeQueueEntry<CTaskStorageSlot*> m_entry;

public:
    /** 构造空闲槽位，不申请任务载荷
    */
    CTaskStorageSlot();

};

/** 64 槽分配页；生产者按小批次领取位图，消费者合并归还
*/
struct CTaskStoragePage
{
public:
    // 槽排列在对齐页首；所有槽生命周期覆盖整个批次，不并发 placement-new 任务字段
    CTaskStorageSlot m_slots[64];
    // 1 表示可领取；末页越界位保持 0
    std::atomic<uint64_t> m_free;
    // 本页所属批次
    CTaskStorageBlock* m_block;
    // 唯一消费者尚未发布的归还位
    uint64_t m_pending;
    // 是否已经加入消费者的待归还页链
    bool m_dirty;
    // 唯一消费者维护的待归还页链
    CTaskStoragePage* m_dirtyNext;
    // 补齐页尾元数据，64 位平台下一页仍按缓存行对齐
    unsigned char m_padding[24];
    // 生产者的提交可见位，独立于消费者更新的任务记录
    std::atomic<bool> m_publications[64];
    // 消费者集中写入状态，避免逐任务改写刚从生产者接收的载荷缓存行
    CTaskStorageState m_states[64];

public:
    /** 构造空页，发布前补齐所属批次及有效位
    */
    CTaskStoragePage();

};

/** 固定目录中的一个可独立缩容批次
*/
struct CTaskStorageBlock
{
public:
    // 页对齐前的原始分配地址，只在全部页析构后释放
    unsigned char* m_storage;
    // 实际拥有的页数组
    CTaskStoragePage* m_pages;
    // 实际槽数，不向上取整突破上限
    size_t m_capacity;
    // 页数量
    size_t m_pageCount;
    // 低 32 位为占用和私有缓存预留，中间 31 位为活动代次，最高位永久关闭
    std::atomic<uint64_t> m_leases;
    // 关闭后用于最终释放的 epoch
    uint64_t m_retiredEpoch;
    // 观察空闲时的完整活动字，避免把重用前后的空闲时间拼接
    uint64_t m_idleLease;
    // 单调毫秒空闲起点
    uint64_t m_idleSince;

public:
    /** 构造空批次
    */
    CTaskStorageBlock();

    /** 仅外部静止或全部保护结束后析构
    */
    ~CTaskStorageBlock();

    /** 私有构建完整页数组
    @param [in] capacity 非零实际槽数，至多 65535
    @return 是否完成所有分配与初始化
    */
    bool initialize(size_t capacity);

};

/** 按优先级分开的独占分配游标，不改变共享池总容量
*/
struct CTaskStorageAllocation
{
public:
    // 非零 mask 使对应块保持占用，不能被缩容释放
    CTaskStoragePage* m_page;
    // 本位置独占的尚未使用槽位
    uint64_t m_mask;
    // 下次分配的固定目录提示
    size_t m_blockHint;
    // 下次分配的页提示
    size_t m_pageHint;

public:
    /** 建立空游标
    */
    CTaskStorageAllocation();

};

/** 每个读侧位置对应的独占分配缓存；租约外保留缓存位，但不保留借用
*/
struct CTaskStorageCache
{
public:
    // 常用优先级各自连续分配，其他级别使用位置 0
    CTaskStorageAllocation m_allocations[16];
    // 所有分配游标中的未使用槽位合计，供维护者弱一致观察
    std::atomic<size_t> m_cachedCount;
    // 本位置累计业务额度；仅租约持有者写，观察者只读
    std::atomic<uint64_t> m_reserved;
    // 本位置累计入队预留，未发布失败时回滚
    std::atomic<uint64_t> m_published;
    // 常用优先级分开的累计入队预留
    std::atomic<uint64_t> m_levels[16];
    // 本位置保留序号段中已经使用的最大序号
    uint64_t m_nextSequence;
    // 本位置已预留段的末尾，禁止回绕
    uint64_t m_lastSequence;
    // 常用且不退役优先级的生产者上下文；仅本位置的租约持有者访问
    LockFreeQueueProducer<CTaskStorageSlot*> m_producers[16];
    // 维护方先观察计数，再尝试借用空闲位置；不得并发操作上下文本身
    std::atomic<size_t> m_producerCount;
    // 使相邻借用位置的可写部分不共用缓存行
    unsigned char m_padding[64];

public:
    /** 构造空缓存
    */
    CTaskStorageCache();

};