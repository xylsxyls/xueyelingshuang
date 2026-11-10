#pragma once
#include "CTask.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <atomic>
#include <memory>
#include <stddef.h>

struct CTaskStoragePage;
struct CTaskStorageBlock;
class CTaskPriorityBucket;

/** 独立执行状态；与生产者填写的载荷分离，回收前不得复用
*/
struct CTaskStorageState
{
public:
    // 低两位：0 尚未完成、3 已完成；位 2 请求重做。
    // 执行身份由当前指针发布，不重复写入本状态；完成与重做请求原子合并。
    std::atomic<uint32_t> m_bits;

public:
    /** 建立未提交状态，所在页的发布位初始为 false
    */
    CTaskStorageState();

};

/** 地址在池批次存活期间不变的执行记录，同时作为 Queue 外部 Entry
基类地址可安全向下转型恢复记录，不依赖 offsetof 或读取尚未预取的载荷
出队后基类的 value/cookie 复用为退休链后继和 epoch；只有安全回收后才能重写载荷
*/
struct CTaskStorageSlot : public LockFreeQueueEntry<CTaskStorageSlot*>
{
public:
    // 发布后不可改写的任务引用，安全回收时释放
    std::shared_ptr<CTask> m_task;
    // 借用本页紧凑状态数组，消费者写状态不使任务载荷缓存行失效
    CTaskStorageState* m_state;
    // 原始任务优先级
    int32_t m_level;
    // 原始分配位置，用于将安全退休引用交回对应生产者缓存
    uint32_t m_ownerIndex;
    // 本槽所在页，池销毁前有效
    CTaskStoragePage* m_page;

public:
    /** 构造空闲槽位，不申请任务载荷
    */
    CTaskStorageSlot();

    /** 根据稳定页地址计算页内位置，不另存冗余索引
    @return 0 至 63；要求本槽已经属于有效页
    */
    size_t index() const;

};

/** 唯一消费者记录的退休地址与精确 epoch；相邻存放以减少双数组访问
*/
struct CTaskRetiredRecord
{
public:
    // 已完成但尚不能重新分配的执行记录
    CTaskStorageSlot* m_slot;
    // 退休时的精确 epoch，不近似合并不同保护区间
    uint64_t m_epoch;

public:
    /** 构造尚未加入退休窗口的空记录
    */
    CTaskRetiredRecord();

};

/** 64 槽分配页；生产者按小批次领取位图，消费者合并归还
*/
struct CTaskStoragePage
{
public:
    // 槽排列在对齐页首；所有槽生命周期覆盖整个批次，不并发 placement-new 任务字段
    CTaskStorageSlot m_slots[64];
    // 每状态 4 字节，页内统一存放，生命周期覆盖所有槽借用
    CTaskStorageState m_states[64];
    // 1 表示可领取；末页越界位保持 0
    std::atomic<uint64_t> m_free;
    // 本页所属批次
    CTaskStorageBlock* m_block;
    // 页尾元数据占一个完整缓存行，保持后续页内载荷和状态的对齐
    unsigned char m_padding[64 - sizeof(std::atomic<uint64_t>) - sizeof(CTaskStorageBlock*)];
    // 生产者的提交可见位，独立于消费者更新的任务记录
    std::atomic<bool> m_publications[64];

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
    // 低 63 位为占用和私有缓存预留，最高位为关闭；保护域限制同时申请数
    std::atomic<uint64_t> m_leases;
    // 成功进入开放块即置位；消费者交换清除并在关闭后复核，避免空闲时间跨重用累计
    std::atomic<bool> m_touched;
    // 关闭后用于最终释放的 epoch
    uint64_t m_retiredEpoch;
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

/** 可独立帮助的回收批次；暂停的帮助者只占用本批次
*/
struct CTaskStorageReclaimBatch
{
public:
    // 0 空，-1 已被一个处理者取得，正数为剩余记录数
    std::atomic<int32_t> m_count;
    // 处理者独占期间使用的下一个记录下标
    size_t m_begin;
    // 发布后只能由成功取得本批次的处理者访问
    CTaskStorageSlot* m_slots[64];
    // 相邻批次的竞争互不共享缓存行
    unsigned char m_padding[64];

public:
    /** 建立空批次
    */
    CTaskStorageReclaimBatch();

};

/** 每个读侧位置对应的独占分配缓存；租约外保留缓存位，但不保留借用
*/
struct CTaskStorageCache
{
public:
    // 四个独立批次吸收生产者领取不同优先级页时的归还间隙
    CTaskStorageReclaimBatch m_reclaims[4];
    // 唯一消费者暂存同一分配位置的安全退休记录
    CTaskStorageSlot* m_stagedSlots[64];
    // 唯一消费者暂存数量
    size_t m_stagedCount;
    // 以下链字段只有消费者访问；每个缓存至多出现一次
    CTaskStorageCache* m_stagedNext;
    bool m_stagedListed;
    // 消费者暂存写入不使生产者的分配游标和固定编号所在缓存行失效
    unsigned char m_stagedPadding[64];
    // 固定缓存编号，初始化后不变；与生产者独占分配字段放在一起
    uint32_t m_ownerIndex;
    // 常用优先级各自连续分配，其他级别使用位置 0
    CTaskStorageAllocation m_allocations[16];
    // 所有分配游标中的未使用槽位合计，供维护者弱一致观察
    std::atomic<size_t> m_cachedCount;
    // 其他调用遇到容量压力时请求归还私有空闲位；持有者在下一次分配或本次调用退出时响应
    std::atomic<bool> m_flushRequested;
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

#include "CTaskStorageModels.inl"