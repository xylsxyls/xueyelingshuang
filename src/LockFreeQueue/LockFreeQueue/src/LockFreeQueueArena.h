#pragma once

#include "LockFreeQueuePolicy.h"
#include "LockFreeQueueChannel.h"
#include <atomic>
#include <new>
#include <limits>
#include <type_traits>
#include <chrono>
#include <utility>
#if defined(_MSC_VER) && _MSC_VER < 1900
#pragma push_macro("WIN32_LEAN_AND_MEAN")
#pragma push_macro("NOMINMAX")
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#pragma pop_macro("NOMINMAX")
#pragma pop_macro("WIN32_LEAN_AND_MEAN")
#endif

#include "LockFreeQueueArenaModels.h"

// All payload blocks share this allocator, including initial and grown blocks.
// A claimed bitmap bit owns storage without per-node hazard retirement. Channel
// producers never dereference a published payload, so pop returns its bit directly.
template <class T>
class LockFreeQueueArena
{
public:
    typedef LockFreeQueueArenaNode<T> Node;
    typedef LockFreeQueueArenaBlock<T> Block;
    typedef LockFreeQueueArenaRecord<T> Record;
    typedef LockFreeQueueChannelSegment<Node> Segment;
    typedef LockFreeQueueArenaAuxiliary<T> Auxiliary;

    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    */
    LockFreeQueueArena();

    /** 结束本对象拥有或借用的状态；调用前必须满足所属类型的静止条件
    */
    ~LockFreeQueueArena();

    /** 校验并保存池参数；仅允许未初始化且外部静止时调用
    @param [in] config 节点数配置，最大容量仅允许 -1 或不小于初始容量；借用至返回
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult configure(const LockFreeQueuePoolConfig& config);

    /** 申请登记区和初始块；外部静止，失败完整回滚
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult initialize();

    /** 外部静止下析构有效载荷并释放本实例全部存储；可重复调用
    */
    void destroy();

    /** 外部静止下替换扩容算法；空策略恢复默认，复制失败保留原策略
    @param [in] policy 线程安全、短小、不递归调用本池的策略；保存其副本
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult setGrowPolicy(const LockFreeQueueGrowPolicy& policy);

    /** 外部静止下替换缩容算法；空策略禁用自动缩容，已退役块继续回收
    @param [in] policy 线程安全、短小、不递归调用本池的策略；保存其副本
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult setShrinkPolicy(const LockFreeQueueShrinkPolicy& policy);

    /** 非阻塞申请一个登记槽；TLS 只缓存数字，不保存对象指针
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Record* enter();

    /** 结束一次节点保护并按需归还登记槽；不得干预活跃的其他生产者
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    @param [in] releaseLease 是否在离开时归还登记租约
    @param [in] retainPublication 是否保留旧段提示保护，回收者仅可从空闲租约清理
    */
    void leave(Record& record, bool releaseLease, bool retainPublication = false);

    /** 先复用安全节点，必要时同步评估增长；失败不接纳载荷
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    @param [out] result 申请结果；指针形式允许 nullptr 表示忽略结果
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Node* acquireNode(Record& record, LockFreeQueueResult& result);

    /** 析构载荷并归还位图；consumer 路径同时提交消费计数
    @param [in,out] node 本次操作拥有或唯一消费者借用的非空节点
    @param [in,out] producer 空闲上下文或可空生产者保护记录；借用至返回
    */
    void releaseNode(Node* node, Record* producer = nullptr);

    /** 归还当前独占登记槽中的未用节点；槽必须已取得租约
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    */
    void releaseCache(Record& record);

    /** 申请独占发布段；初始化或唯一消费者可仅使用初始块
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    @param [in] initialOnly 是否只从不可摘除的初始块领取发布段
    @param [out] result 申请结果；指针形式允许 nullptr 表示忽略结果
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Segment* acquireSegment(Record& record, bool initialOnly = false, LockFreeQueueResult* result = nullptr);

    /** 归还未发布或已证明无保护的发布段；归还后不得再解引用
    @param [in,out] segment 受保护或独占的发布段；借用至返回
    @param [in,out] producer 空闲上下文或可空生产者保护记录；借用至返回
    */
    void releaseSegment(Segment* segment, Record* producer = nullptr);

    /** 唯一消费者将摘除段加入退役链；不立即复用
    @param [in,out] segment 受保护或独占的发布段；借用至返回
    */
    void retireSegment(Segment* segment);

    /** 唯一消费者按访问预算扫描退役段；被保护段轮转，不能阻塞其他段
    @param [in] budget 本轮访问类别或退役对象次数上限，零不做工作
    @return 上述操作对应的数值、状态或弱一致快照
    */
    size_t collectSegments(size_t budget);

    /** 唯一消费者观察是否存在待回收发布段
    @return 本地退役链非空
    */
    bool hasRetiredSegments() const;

    /** 唯一消费者执行有预算回收；不启动后台线程，纯空闲需要 owner 主动调用
    @param [in] budget 本轮访问类别或退役对象次数上限，零不做工作
    @return 上述操作对应的数值、状态或弱一致快照
    */
    LockFreeQueueMaintenanceResult maintain(size_t budget);

    /** 取得弱一致池资源快照；发布 CAS 重试统计不包含分配器全部竞争
    @return 上述操作对应的数值、状态或弱一致快照
    */
    LockFreeQueuePoolStatistics statistics() const;

    /** 观察唯一消费者提交的模计数；与生产者基数配合计算容量和 size
    @return 上述操作对应的数值、状态或弱一致快照
    */
    size_t consumed() const;

    /** 观察初始化状态；不能与 init 或 destroy 并发
    @return 是否满足上述条件或完成上述操作
    */
    bool initialized() const;

    /** 核实当前目标使用的原子宽度是否由无锁后端支持
    @return 是否满足上述条件或完成上述操作
    */
    bool atomicsLockFree() const;

    /** 观察是否配置缩容回调；策略更换必须外部静止
    @return 是否满足上述条件或完成上述操作
    */
    bool shrinkEnabled() const;

    /** 判断唯一消费者是否需摊销策略或待释放块维护
    @return 是否满足上述条件或完成上述操作
    */
    bool automaticMaintenanceNeeded() const;

    /** 取得生命周期内不可自动摘除的基础块；借用至 destroy
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Block* initialBlock() const;

private:
    // 静止期设定的容量配置，单位为物理节点
    LockFreeQueuePoolConfig m_config;
    // 拥有的扩容算法副本；普通申请可并发调用
    LockFreeQueueGrowPolicy m_growPolicy;
    // 拥有的缩容算法副本；只由唯一消费者调用
    LockFreeQueueShrinkPolicy m_shrinkPolicy;
    // 对齐的生产者登记数组，无线程退出回调或外部登记器
    Record* m_records;
    // 每个登记槽的次块保护，扫描顺序在主保护之前
    LockFreeQueueAtomic<Block*>* m_traversal;
    // 拥有的登记区原始存储，与辅助段槽位登记共用一次申请
    unsigned char* m_recordStorage;
    // 登记区实际保留字节，含对齐余量
    size_t m_recordBytes;
    // 有界辅助发布段登记表
    Auxiliary* m_auxiliary;
    // 辅助段槽数，4 倍生产者上限加 4
    size_t m_auxiliaryCount;
    // 辅助段登记表字节数，独立于已申请段的字节
    size_t m_auxiliaryBytes;
    // 唯一消费者的辅助槽维护游标
    size_t m_auxiliaryScan;
    // 已使用登记槽下标上界；生命周期内单调增加
    std::atomic<size_t> m_high;
    // 已发布块链头；新增块原子插入，唯一消费者摘除
    LockFreeQueueAtomic<Block*> m_blocks;
    // 不可自动摘除的初始块身份或指针
    Block* m_initial;
    // 已发布且尚保留的物理节点数，或块内节点数
    std::atomic<size_t> m_capacity;
    // 全局生产者空闲基数，批次领取或回滚时更新
    std::atomic<size_t> m_availableBase;
    // The consumer is the sole writer of returned; producers update base only
    // when claiming a batch or rolling back. Unsigned lifetime wrap is intended.
    // 缓存行隔离填充，不承载业务状态
    char m_returnPadding[64];
    // 唯一消费者提交的模计数，同时服务可用容量和队列 size
    LockFreeQueueAtomic<size_t> m_returned;
    // 缓存行隔离填充，不承载业务状态
    char m_returnEndPadding[64];
    // 实际保留加未发布预留的总节点额度
    std::atomic<size_t> m_budget;
    // 生命周期内总预留额度峰值
    std::atomic<size_t> m_peak;
    // 已摘除但尚未物理释放的块容量
    std::atomic<size_t> m_retiringCapacity;
    // 实际拥有的动态存储字节数，含元数据
    std::atomic<size_t> m_bytes;
    // 实际保留的节点块数量
    std::atomic<size_t> m_blockCount;
    // 累计成功发布增长次数，饱和不回绕
    std::atomic<uint64_t> m_grows;
    // 实际系统申请尝试次数，含失败申请，饱和不回绕
    std::atomic<uint64_t> m_allocations;
    // 最后成功增长的单调毫秒时刻，零表示无有效时钟记录
    std::atomic<uint64_t> m_lastGrow;
    // 发布段代次来源，耗尽显式拒绝且不回绕
    std::atomic<uint64_t> m_segmentSequence;
    // 校验后的有效物理节点上限
    size_t m_limit;
    // 唯一消费者拥有的待安全回收段链头
    Segment* m_retiredSegments;
    // 唯一消费者拥有的待安全回收段链尾
    Segment* m_retiredSegmentTail;
    // 唯一消费者拥有的待物理释放块链头
    Block* m_retiredBlocks;
    // 唯一消费者拥有的待物理释放块链尾
    Block* m_retiredBlockTail;
    // 唯一消费者的块维护游标
    Block* m_scan;
    // 维护游标前驱；仅唯一消费者可摘除
    Block* m_previous;
    // 唯一消费者的空闲登记槽维护游标
    size_t m_recordScan;
    // 跨调用保持的轮转维护类别，预算 1 也不饿死其他类别
    size_t m_maintenanceTurn;

private:
    /** 统计位图中置位的数量
    @param [in] bits 待统计位图
    @return 上述操作对应的数值、状态或弱一致快照
    */
    static size_t bitCount(uint32_t bits);

    /** 取得非零位图最低置位下标
    @param [in] bit 非零位图
    @return 上述操作对应的数值、状态或弱一致快照
    */
    static size_t lowBit(uint32_t bit);

    /** 将诊断计数饱和递增；不得回绕
    @param [in] value 待复制的载荷或待更新的原子计数；借用至本次调用返回
    */
    static void increment(std::atomic<uint64_t>& value);

    /** 读取单调毫秒时钟；不可用时返回零
    @return 上述操作对应的数值、状态或弱一致快照
    */
    static uint64_t now();

    /** 校验节点数、负上限、登记规模与寻址溢出
    @param [in] config 节点数配置，最大容量仅允许 -1 或不小于初始容量；借用至返回
    @param [out] limit 本机寻址与配置共同限制的最大物理节点数
    @return 是否满足上述条件或完成上述操作
    */
    static bool validate(const LockFreeQueuePoolConfig& config, size_t& limit);

    /** 一次申请块描述、节点和发布段存储；仅初始化分配位图
    @param [in] amount 待申请的物理节点数，已取得相同数量的额度
    @param [in] initial 是否基础块，基础块预留一个哨兵额度
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Block* createBlock(size_t amount, bool initial);

    /** 释放无人访问的整块；静止销毁时逐项析构仍有效的载荷
    @param [in,out] block 受保护或调用者独占的块；借用至返回
    @param [in] empty 是否已经证明全空且无人引用；true 可跳过载荷扫描
    */
    static void deleteBlock(Block* block, bool empty = false);

    /** 从当前独占登记槽私有批次取出一个未构造载荷的节点
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Node* takeCached(Record& record);

    /** 从受保护的开放块领取有界节点批次；关闭竞争必须回滚
    @param [in,out] block 受保护或调用者独占的块；借用至返回
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Node* takeBlock(Block* block, Record& record);

    /** 沿受保护块链查找实际可用位图；不将一次 CAS 失败当作满池
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    @param [out] interrupted 是否遇到正在摘除的拓扑，需要稍后重试
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Node* findNode(Record& record, bool& interrupted);

    /** 发布保护并验证块链头；重试只依赖竞争进展
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    @param [out] interrupted 是否遇到正在摘除的拓扑，需要稍后重试
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Block* protectedFirst(Record& record, bool& interrupted);

    /** 先保护后继，再验证仍连接的前驱；次保护向主保护转交
    @param [in,out] block 受保护或调用者独占的块；借用至返回
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    @param [out] interrupted 是否遇到正在摘除的拓扑，需要稍后重试
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Block* protectedNext(Block* block, Record& record, bool& interrupted);

    /** 按次保护、主保护顺序扫描；避免遗漏保护转交窗口
    @param [in,out] block 受保护或调用者独占的块；借用至返回
    @return 是否满足上述条件或完成上述操作
    */
    bool protectedBlock(Block* block) const;

    /** 检查发布段保护；只可取得已空闲登记槽后清除其旧提示保护
    @param [in,out] segment 受保护或独占的发布段；借用至返回
    @return 是否满足上述条件或完成上述操作
    */
    bool protectedSegment(Segment* segment) const;

    /** 检查整块空闲候选；释放还必须关闭、摘除并通过保护扫描
    @param [in,out] block 受保护或调用者独占的块；借用至返回
    @return 是否满足上述条件或完成上述操作
    */
    bool completelyFree(Block* block) const;

    /** 尝试归还不活跃登记槽的私有缓存；不等待暂停生产者
    @param [in,out] caller 当前申请者独占记录，不回收它自身的缓存
    @return 是否满足上述条件或完成上述操作
    */
    bool reclaimCaches(Record& caller);

    /** 检查有界私有节点缓存，用于区分真实满池与临时占用
    @return 是否满足上述条件或完成上述操作
    */
    bool hasCachedClaims() const;

    /** 调用一次策略、原子预留额度并发布增长块；失败退还预留
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult grow();

    /** 合成可复用空闲快照；混合快照溢出按零处理，不作为位图接纳许可
    @return 上述操作对应的数值、状态或弱一致快照
    */
    size_t available() const;

    /** 复用或申请有界辅助发布段；不突破登记槽总量限制
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    @param [out] result 申请结果；指针形式允许 nullptr 表示忽略结果
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Segment* acquireAuxiliary(Record& record, LockFreeQueueResult* result);

    /** 领取不回绕的发布段代次；耗尽显式拒绝新发布
    @param [in,out] segment 受保护或独占的发布段；借用至返回
    @param [out] result 申请结果；指针形式允许 nullptr 表示忽略结果
    @return 是否满足上述条件或完成上述操作
    */
    bool assignGeneration(Segment* segment, LockFreeQueueResult* result);

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueArena(const LockFreeQueueArena& other) = delete;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueArena& operator=(const LockFreeQueueArena& other) = delete;
};

#include "LockFreeQueueArena.inl"
#include "LockFreeQueueArenaAllocation.inl"
#include "LockFreeQueueArenaMaintenance.inl"
#include "LockFreeQueueArenaAuxiliary.inl"