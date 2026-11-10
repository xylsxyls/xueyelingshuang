#pragma once

#include "LockFreeQueueChannelModels.h"
#include "LockFreeQueueProgress.h"
#include <type_traits>

template <class T>
struct LockFreeQueueArenaBlock;
template <class T>
struct LockFreeQueueArenaNode;

// Bounded auxiliary publication storage. Low bit denotes exclusive ownership;
// unavailable slots are skipped. No address bits are truncated on any platform.
template <class T>
struct LockFreeQueueArenaAuxiliary
{
public:
    typedef LockFreeQueueChannelSegment<LockFreeQueueArenaNode<T>> Segment;
    // 0 empty; 1 allocation reserved; aligned pointer cached; pointer|1 owned.
    // 辅助段所有权字：0 空槽，1 预留，偶数地址缓存，地址低位置位表示占用
    LockFreeQueueAtomic<uintptr_t> m_bits;

public:
    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    */
    LockFreeQueueArenaAuxiliary();

};

// 池内载荷存储；低位合并有效标记，T 原地构造，必须具有不抛异常的析构函数
template <class T>
struct LockFreeQueueArenaNode
{
public:
    // Blocks are aligned to 64 bytes. Bit zero records an engaged payload;
    // the remaining bits preserve the complete owner address on every target.
    // 借用的所属对象或带有效标志的完整所有者地址，见所属类型契约
    uintptr_t m_owner;
    unsigned char m_storage[sizeof(T) + (std::alignment_of<T>::value > std::alignment_of<uintptr_t>::value
                                                ? std::alignment_of<T>::value - 1
                                                : 0)];

public:
    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    @param [in,out] block 受保护或调用者独占的块；借用至返回
    */
    explicit LockFreeQueueArenaNode(LockFreeQueueArenaBlock<T>* block);

    /** 结束本对象拥有或借用的状态；调用前必须满足所属类型的静止条件
    */
    ~LockFreeQueueArenaNode();

    /** 读取节点完整所有者地址；低位只保存载荷有效标志
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    LockFreeQueueArenaBlock<T>* block() const;

    /** 取得满足 T 对齐的节点内存；有效载荷由 construct 和 reset 管理
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    T* data();

    /** 在独占节点内复制构造 T；异常不设置有效标志
    @param [in] value 待复制的载荷或待更新的原子计数；借用至本次调用返回
    */
    void construct(const T& value);

    /** 重置独占或外部静止状态；不得与仍借用该对象的线程并发
    */
    void reset();

};

// 一次系统申请的节点与发布段集合；唯一消费者负责关闭、摘除及最终释放
template <class T>
struct LockFreeQueueArenaBlock
{
public:
    typedef LockFreeQueueArenaNode<T> Node;
    typedef LockFreeQueueChannelSegment<Node> Segment;
    // 发布后的后继链接；读取遵守对应保护协议
    LockFreeQueueAtomic<LockFreeQueueArenaBlock*> m_next;
    // 块状态：0 发布记账中，1 开放，2 关闭中，3 已摘除
    LockFreeQueueAtomic<uint32_t> m_state;
    // 生产者空闲基数，与唯一消费者归还数按无符号模合成
    std::atomic<size_t> m_freeNodes;
    // 尚未被领取的块内发布段数
    std::atomic<size_t> m_freeSegments;
    // 块使用代次，饱和时保守禁止自动摘除
    std::atomic<uint64_t> m_activity;
    // 外部载荷累计领取额度；达到 uint64_t 上限后明确拒绝，禁止回绕 ABA
    std::atomic<uint64_t> m_externalClaims;
    // 生产者回滚及私有缓存归还总量；不减小累计领取数
    std::atomic<uint64_t> m_externalRefunds;
    // 常量地址元数据与生产者更新的领取计数隔离，消费者归还节点只读以下地址
    char m_layoutPadding[64];
    // 块内节点存储，位图领取后才开始对象生命周期
    Node* m_nodes;
    // 块内发布段存储，位图领取后才初始化
    Segment* m_segments;
    // 节点分配位图，1 表示可领取；基础块第零位保留
    std::atomic<uint32_t>* m_nodeBits;
    // 发布段分配位图，1 表示可领取
    std::atomic<uint32_t>* m_segmentBits;
    // 已发布且尚保留的物理节点数，或块内节点数
    size_t m_capacity;
    // 节点位图的 32 位字数量
    size_t m_nodeWords;
    // 块内发布段数量
    size_t m_segmentCount;
    // 发布段位图的 32 位字数量
    size_t m_segmentWords;
    // 实际拥有的动态存储字节数，含元数据
    size_t m_bytes;
    // 本对象拥有的原始存储；载荷生命周期显式管理
    unsigned char* m_storage;
    // 不可自动摘除的初始块身份或指针
    bool m_initial;
    // 上次观察连续空闲时的块使用代次
    uint64_t m_seenActivity;
    // 连续空闲起点，单调毫秒；重新使用或保护后重置
    uint64_t m_idleSince;
    // 是否已经建立有效的连续空闲观察
    bool m_idleObserved;
    // 唯一消费者拥有的退役链链接
    LockFreeQueueArenaBlock* m_retiredNext;
    // 缓存行隔离填充，不承载业务状态
    char m_returnPadding[64];
    // Sole-consumer returns do not contend on the producers' batch counter.
    // 普通节点的唯一消费者归还计数；Entry 专用块复用 m_externalReturns
    LockFreeQueueAtomic<size_t> m_returnedNodes;
    // 唯一消费者累计归还外部额度；与生产者领取及退款组成非回绕预算
    LockFreeQueueAtomic<uint64_t> m_externalReturns;
    // 缓存行隔离填充，不承载业务状态
    char m_returnEndPadding[64];

public:
    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    @param [in] capacity 块内物理节点数
    @param [in] initial 是否基础块，基础块预留一个哨兵额度
    */
    LockFreeQueueArenaBlock(size_t capacity, bool initial);

    /** 观察块内可复用空闲数量；与位图及保护协议共同使用
    @return 上述操作对应的数值、状态或弱一致快照
    */
    size_t freeNodes() const;

};

// 对齐的独占生产者登记槽；缓存最多 31 个普通节点或 127 个外部额度并保护发布段
template <class T>
struct LockFreeQueueArenaRecord
{
public:
    // 发布段保护，解引用前发布并验证，回收时扫描
    LockFreeQueueAtomic<LockFreeQueueChannelSegment<LockFreeQueueArenaNode<T>>*> m_hazard;
    // 本生产者在当前段中已验证的发布前缀位置
    size_t m_hint;
    // 本执行流的高频调度状态复用原诊断计数位置，普通成功提交只访问首个缓存行。
    LockFreeQueueProgress m_progress;
    // 本次慢速额度领取是否观察到累计序号耗尽；成功领取时不影响结果。
    bool m_creditExhausted;
    // 非回绕发布段代次，用于使旧位置提示失效
    uint64_t m_generation;
    // 登记槽租约：0 空闲，1 独占；不能等待另一持有者。
    LockFreeQueueAtomic<uint32_t> m_active;
    // 普通模式为未用节点位图，外部额度模式为未用数量；均已计入物理占用
    LockFreeQueueAtomic<uint32_t> m_cachedMask;
    // 分配遍历主保护；跨段时与次保护共同验证
    LockFreeQueueAtomic<LockFreeQueueArenaBlock<T>*> m_block;
    // 当前私有批次所属块，非空缓存位使其不能释放
    LockFreeQueueArenaBlock<T>* m_cachedBlock;
    // 私有批次位图字下标加一，同时用作下次查找提示
    size_t m_wordHint;
    // 分散计数模式下由当前租约持有者更新的累计发布预留
    std::atomic<size_t> m_published;
    // 仅竞争失败时更新的诊断计数，饱和不回绕；排在高频操作状态之后。
    LockFreeQueueAtomic<uint64_t> m_retries;
#if UINTPTR_MAX == UINT32_MAX
    // 缓存行隔离填充，不承载业务状态
    char m_arenaPadding[7];
#else
    // 保持完整缓存行步长，避免相邻生产者相互失效
    char m_arenaPadding[47];
#endif

public:
    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    */
    LockFreeQueueArenaRecord();

};

#include "LockFreeQueueArenaModels.inl"