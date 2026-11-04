#pragma once
#include "LockFreeHashMapTypes.h"
#include "LockFreeHashMapAtomic.h"
#include <atomic>
#include <chrono>
#include <limits>
#include <new>
#include <type_traits>

/** 固定保护记录，保存根、前驱、当前节点、值版本和池块地址，由一次操作独占借用
*/
struct LockFreeHashMapHazards
{
public:
    // 0 未借用，1 普通保护，非零偶地址保护值，奇地址还启用五个保护格
    std::atomic<uintptr_t> m_used;
    // 第 g 条记录保存第 64*g 至 64*g+63 条记录的永久登记位
    std::atomic<uint64_t> m_registered;
    // 固定保护地址，第 1、2 格只交换本地游标角色
    std::atomic<void*> m_pointer[5];
    // 仅借用者推进回收节奏，回绕不作为身份使用
    uint32_t m_writeCount;
    // 仅借用者读写，在永久登记位和上界发布后才置为 true
    bool m_registeredOnce;
    // 当前记录独占且永不复用的值版本范围起点，0 表示范围已用尽
    uint64_t m_nextVersion;
    // 当前独占版本范围的包含性上界
    uint64_t m_lastVersion;
    // 八个可转移格中的第一个，空槽归还块前持续保留占用
    std::atomic<void*> m_free;
    // 其余七格，偶地址为已析构空槽，奇地址唯一拥有一个退役值
    std::atomic<void*> m_spare[7];
    // 隔开不同记录的写字段，减少共享缓存行，不要求扩展对齐的 new
    unsigned char m_separation[64];
public:
    /** 构造未借用且指针为空的记录，不构造键值载荷
    */
    LockFreeHashMapHazards();

    /** 选择八个固定回收格之一
    @param [in] index 下标，必须小于 8
    @return 对应原子格的借用引用，记录必须存活
    */
    std::atomic<void*>& cache(size_t index);

    /** 从固定记录中接纳一次操作，并在返回前完成永久登记发布
    @param [in] records 记录数组，空表示接纳失败，调用结束前必须存活
    @param [in] count 数组长度，每条记录最多探测一次
    @param [in,out] span 永久登记范围上界，不减小
    @param [in] hint 调用方栈地址，仅用于分散探测起点，不代表所有权
    @param [in] admission 非零接纳字，1 为普通保护，其他值必须为合法直接地址编码
    @return 独占借到的记录，失败为空；调用方必须在所有访问结束后释放 m_used
    */
    static LockFreeHashMapHazards* acquire(LockFreeHashMapHazards* records, size_t count,
        std::atomic<size_t>& span, uintptr_t hint, uintptr_t admission);
};

/** 作用域内的操作登记，不使用 TLS 或线程退出回调，不等待其他持有者
*/
class LockFreeHashMapGuard
{
public:
    // 本次独占借用的记录，接纳失败为空
    LockFreeHashMapHazards* m_record;
    // 本地前驱游标下标，实际保护地址不会在格之间搬移
    size_t m_predecessorSlot;
    // 本地当前节点游标下标
    size_t m_currentSlot;
    // 与查找得到的同一受保护节点对应的标量值和版本
    LockFreeHashMapScalarState m_scalar;
public:
    /** 每条记录最多尝试一次借用，可同时发布直接地址保护
    @param [in] records 固定记录数组，空表示无法接纳，须活到本对象析构
    @param [in] count 数组记录数，至少为 1
    @param [in,out] span 已登记记录上界，首次登记在源校验前发布
    @param [in] directPointer 可为空，非空时借用 CAS 同时保护此地址
    @param [in] directNode 非空直接地址是否为节点，为 true 时同时启用五个游标保护格
    */
    LockFreeHashMapGuard(LockFreeHashMapHazards* records, size_t count, std::atomic<size_t>& span, void* directPointer = nullptr, bool directNode = false);

    /** 清除本次所有保护后归还记录，不释放记录数组
    */
    ~LockFreeHashMapGuard();

    /** 向固定格发布保护地址，非空发布参与顺序一致校验
    @param [in] index 保护格下标，范围 0 至 4，要求已经借到记录
    @param [in] pointer 待保护地址，空表示放弃保护
    */
    void protect(size_t index, void* pointer);

    /** 放弃旧链接借用后重置本地游标角色
    */
    void resetLinks();

    /** 交换本地游标下标，使当前节点成为前驱，不移动实际保护地址
    */
    void advance();
private:
    /** 禁止复制独占借用记录
    @param [in] other 不可复制的源保护对象
    */
    LockFreeHashMapGuard(const LockFreeHashMapGuard& other);

    /** 禁止赋值独占借用记录
    @param [in] other 不可赋值的源保护对象
    @return 无实现，不可调用
    */
    LockFreeHashMapGuard& operator=(const LockFreeHashMapGuard& other);
};

template <typename KeyType, typename ValueType> struct LockFreeHashMapBlock;

/** 统一槽位，可保存键及可选标量值、不可变值，或标量版本前缀记录，由所属块拥有
*/
template <typename KeyType, typename ValueType>
struct LockFreeHashMapSlot
{
public:
    // 0 空闲、1 键、2 值、3 退役键、4 退役值、5 回收中、6 缓存空槽、7 独占退役值
    std::atomic<int32_t> m_state;
    // 空闲栈后继必须是原子字段，失败弹出者可能在其他弹出归还后读取
    std::atomic<uint16_t> m_freeNext;
    // 批次内不可变索引，65535 保留为空闲栈空标记
    uint16_t m_index;
    // 最低位标记键节点永久物理删除
    std::atomic<uintptr_t> m_next;
    // 空值永久关闭此键实例，标量前缀转移通过此根帮助完成
    std::atomic<LockFreeHashMapSlot*> m_value;
    // 不可变所属批次，归还前由占用计数保护
    LockFreeHashMapBlock<KeyType, ValueType>* m_block;
    // 槽保存键时不可变的可见代次
    uint64_t m_generation;
    // 槽保存普通值时不可变的值身份
    uint64_t m_version;
    // 完整哈希缓存，哈希相同仍需检查等价关系
    size_t m_hash;
    // 槽内标量载荷，仅由运行中不可变的后端配置启用
    typename LockFreeHashMapScalar<ValueType>::Word m_scalar;
    // 键的定位构造存储
    typename std::aligned_storage<sizeof(KeyType), std::alignment_of<KeyType>::value>::type m_keyStorage;
    // 值的定位构造存储
    typename std::aligned_storage<sizeof(ValueType), std::alignment_of<ValueType>::value>::type m_valueStorage;
public:
    /** 初始化槽位元数据，不构造键和值
    */
    LockFreeHashMapSlot();

    /** 借用已经构造的键
    @return 键地址，调用方必须唯一拥有或保护槽位且确认键已构造
    */
    KeyType* key();

    /** 借用已经构造的值
    @return 值地址，调用方必须唯一拥有或保护槽位且确认值已构造
    */
    ValueType* value();
};

/** 有界批次，独占槽位数组，占用数与接纳序号共用 CAS 字，防止空闲期 ABA
*/
template <typename KeyType, typename ValueType>
struct LockFreeHashMapBlock
{
public:
    // 固定槽数组，只有占用和块保护全部消失后才能释放
    LockFreeHashMapSlot<KeyType, ValueType>* m_slots;
    // 已构造的槽位数，最多 65535
    size_t m_count;
    // 独占手动对齐前的原始槽数组申请
    unsigned char* m_slotAllocation;
    // 第 63 位为关闭，16 至 62 位为接纳序号，低 16 位为占用或预留数
    std::atomic<uint64_t> m_activity;
    // 低 16 位为空闲索引或 65535，高 48 位为不重复的出入栈身份
    std::atomic<uint64_t> m_freeHead;
    // 发布计账完成后才允许领取槽位
    std::atomic<bool> m_ready;
    // 仅外部维护者读写的最近空闲活动快照
    uint64_t m_idleActivity;
    // 首次观察到同一空闲接纳序号的单调毫秒时间
    uint64_t m_idleSince;
public:
    /** 构造尚未发布的空批次，不申请槽位数组
    */
    LockFreeHashMapBlock();
};

/** 固定缓存格，数据操作期间不释放，地址仅为弱提示，解引用前必须保护并复查
*/
template <typename KeyType, typename ValueType>
struct LockFreeHashMapCache
{
    // 键节点的弱提示，必须保护并重新检查来源后才能解引用
    std::atomic<LockFreeHashMapSlot<KeyType, ValueType>*> m_node;
    // 不可变值的弱提示，退役前撤除，并以唯一标签校验
    std::atomic<LockFreeHashMapSlot<KeyType, ValueType>*> m_cachedValue;
    // 最后一次提交前失效标签，0 表示缓存副本均无效
    std::atomic<uint64_t> m_stamp;
    // 键与值副本均携带和标签一致的低 32 位版本，拒绝撕裂和迟到发布
    std::atomic<uint64_t> m_readKey;
    // 带相同唯一版本的标量值副本，与键及标签共同校验
    std::atomic<uint64_t> m_readValue;
    // 完整哈希碰撞后关闭无效节点提示，直到静止重置
    std::atomic<bool> m_colliding;
};

/** 本库独占的有界元数据、带身份空闲栈和保护指针回收器，无后台线程
*/
template <typename KeyType, typename ValueType>
class LockFreeHashMapStorage
{
public:
    typedef LockFreeHashMapSlot<KeyType, ValueType> Slot;
    typedef LockFreeHashMapBlock<KeyType, ValueType> Block;
    typedef LockFreeHashMapCache<KeyType, ValueType> Cache;
    // 运行中不变的配置，策略仅在外部静止时替换
    LockFreeHashMapPoolConfig m_config;
    // 运行中不变的标量后端选择
    bool m_inlineScalar;
    // 固定节点和值提示，发布和撤回参与保护校验
    Cache* m_lookup;
    // 独占手动对齐前的缓存格原始申请
    unsigned char* m_lookupAllocation;
    // 每实例固定数量的操作保护记录
    LockFreeHashMapHazards* m_records;
    // 已登记记录最大下标加 1，必须先于源校验发布
    std::atomic<size_t> m_recordSpan;
    // 固定块目录，低位标记的地址等待安全释放
    std::atomic<Block*>* m_directory;
    // 已发布、关闭及申请中的批次共用的硬预算
    std::atomic<size_t> m_budget;
    // 已发布容量，包含尚未安全释放的关闭块
    std::atomic<size_t> m_capacity;
    // 尚未完成发布的申请容量诊断值
    std::atomic<size_t> m_reserved;
    // 仅计块空闲栈，固定转移格在观察时另行计入
    std::atomic<size_t> m_reusable;
    // 已发布且尚未安全释放的批次数
    std::atomic<size_t> m_blocks;
    // 已发布目录最大下标加 1，允许空洞，不向读者移动目录项
    std::atomic<size_t> m_directorySpan;
    // 合并已保留容量和预留额度的单字高水位
    std::atomic<size_t> m_peakBudget;
    // 非初始批次成功发布次数，饱和后不回绕
    std::atomic<uint64_t> m_grows;
    // 块及槽数组申请尝试次数，饱和后不回绕
    std::atomic<uint64_t> m_allocations;
    // 结构和池的 CAS 重试次数，饱和后不回绕
    std::atomic<uint64_t> m_retries;
    // 上次成功非初始扩容的单调毫秒数
    std::atomic<uint64_t> m_lastGrow;
    // 供 const 查找帮助操作使用的空桶身份，仅外部静止时重置
    std::atomic<uint64_t> m_emptySequence;
    // 并发退役扫描位置，不作为版本身份
    std::atomic<size_t> m_reclaimCursor;
    // 唯一外部维护者推进的块扫描游标
    size_t m_maintenanceCursor;
    // 唯一维护者推进的缓存格释放游标
    size_t m_cacheMaintenanceCursor;
    // 并发退役缓存领取的轮转游标，防止只处理靠前记录
    std::atomic<size_t> m_cacheReclaimCursor;
    // 关闭块在元数据安全释放前继续占用硬预算的槽位数
    size_t m_shrinkPending;
    // 所属表计入的结构字节数，直接测试内部池时不计入
    size_t m_ownerBytes;
public:
    /** 创建空元数据状态，操作前必须 init 成功
    @param [in] config 复制配置和策略所有权，运行中保持不变
    @param [in] ownerBytes 所属表的对象、桶和身份字节预算，内部单池测试可为 0
    */
    explicit LockFreeHashMapStorage(const LockFreeHashMapPoolConfig& config, size_t ownerBytes = 0);

    /** 外部静止后析构全部载荷和存储，键值析构不得抛异常
    */
    ~LockFreeHashMapStorage();

    /** 申请固定元数据及初始批次，失败资源由本对象析构回滚
    @return Ok 可用，参数、原子能力或内存失败有明确状态
    */
    LockFreeHashMapStatus init();

    /** 销毁槽位元数据和手动对齐的原始申请
    @param [in] item 唯一拥有的未发布或安全摘除批次，可为空；载荷必须已析构
    */
    static void destroyBlock(Block* item);

    /** 外部静止时释放所有块，保留配置、值身份范围和保护登记元数据
    */
    void freeQuiescent();

    /** 从八个固定格中领取已经析构的槽位，块占用计数持续保护存储
    @param [in] kind 要设定的槽位类别，1 为键，2 为值
    @param [in,out] record 可以并发领取的记录，成功 CAS 才取得所有权
    @return 独占槽位，无可用格返回空，不构造载荷
    */
    Slot* cachedSlot(int32_t kind, LockFreeHashMapHazards& record);

    /** 申请实际槽位，增长前先尝试安全回收
    @param [in] kind 1 为键，2 为值
    @param [in,out] guard 当前操作独占保护记录
    @param [out] status 取得槽位为 Ok，否则为未接纳原因
    @return 独占槽位，失败为空且不消费业务所有权
    */
    Slot* acquire(int32_t kind, LockFreeHashMapGuard& guard, LockFreeHashMapStatus& status);

    /** 归还私有或安全回收的槽位，先按需析构载荷
    @param [in] slot 非空独占槽位，归还后不可再解引用载荷
    @param [in] constructed 是否已有需要析构的键或值
    @param [in,out] guard 可为空；非空时优先缓存到当前操作独占记录
    */
    void release(Slot* slot, bool constructed, LockFreeHashMapGuard* guard = nullptr);

    /** 唯一摘除者发布一次退役状态，保留载荷类别
    @param [in] slot 已从所有源链接摘除的槽位，不能重复退役
    */
    void retire(Slot* slot);

    /** 分摊扫描退役槽位，可以与其他回收者并发
    @param [in,out] guard 当前操作的保护记录，用于池块保护
    @param [in] budget 退役缓存领取和普通槽位检查的合并预算
    @return 实际析构并归还的槽位数量
    */
    size_t reclaim(LockFreeHashMapGuard& guard, size_t budget);

    /** 尝试回收本操作仍保护的已摘除槽位，调用后禁止再访问其载荷
    @param [in] slot 非空且仍由本操作保护的退役槽位
    @param [in,out] guard 当前保护记录，仅扫描时排除自身
    @return 领取回收权且没有其他保护时为 true，否则留待其他回收
    */
    bool reclaimProtected(Slot* slot, LockFreeHashMapGuard& guard);

    /** 将唯一摘除的普通值存入固定转移格，批量扫描保护记录
    @param [in] slot 已撤除所有缓存源、尚未退役的非标量值槽
    @param [in,out] guard 当前记录的唯一借用者，调用前放弃该值保护
    */
    void deferValue(Slot* slot, LockFreeHashMapGuard& guard);

    /** 唯一外部维护者释放空闲缓存并按整块缩容，可与数据操作并发
    @param [in,out] guard 当前操作独占保护记录
    @param [in] blockBudget 缓存格和目录块检查各自的预算
    @return 状态和实际释放的整块槽位数 m_count
    */
    LockFreeHashMapResult maintain(LockFreeHashMapGuard& guard, size_t blockBudget);

    /** 使用调用方的块保护读取弱一致资源统计
    @param [in,out] guard 当前操作保护记录
    @return 各字段的弱一致观测，外部静止时槽位计数准确
    */
    LockFreeHashMapPoolStats stats(LockFreeHashMapGuard& guard) const;

    /** 扫描已登记保护，首次登记必须先于源校验并参与顺序一致协议
    @param [in] pointer 要判断的非空地址，不解引用该地址
    @param [in] excluded 可为空，非空时仅排除调用者自己独占的记录
    @return 扫描观察到仍受保护为 true
    */
    bool protectedPointer(void* pointer, const LockFreeHashMapHazards* excluded = nullptr) const;

    /** 取得最低置位的位置
    @param [in] bits 非零位图
    @return 最低置位下标，范围 0 至 63
    */
    static size_t firstRecord(uint64_t bits);

    /** 读取单调时钟毫秒数，保留 0 为无历史标记
    @return 当前单调毫秒数加 1；旧 Windows 后端取时失败为 0，保守地不启动缩容计时
    */
    static uint64_t now();

    /** 饱和递增诊断计数，不回绕
    @param [in,out] counter 原子计数，达到 UINT64_MAX 后保持
    */
    static void increment(std::atomic<uint64_t>& counter);
private:
    /** 保护并复查目录中的块地址
    @param [in] index 有效目录下标
    @param [in,out] guard 当前记录，第 4 格用于块保护
    @return 已保护块，空或关闭及校验失败返回空
    */
    Block* block(size_t index, LockFreeHashMapGuard& guard) const;

    /** 先独占领取退役缓存格，再批量扫描保护
    @param [in,out] guard 当前记录，安全析构后的空槽优先放回此记录
    @param [in,out] budget 领取预算，只有成功领取才减少
    @param [in,out] record 指定记录，空表示轮转所有已登记记录
    @return 实际析构并归还的值槽数量
    */
    size_t reclaimCached(LockFreeHashMapGuard& guard, size_t& budget, LockFreeHashMapHazards* record = nullptr);

    /** 一次保护快照检查最多八个已独占领取的退役值
    @param [in] slots 退役值地址数组，调用期间不更换元素
    @param [in] count 有效元素数量，不超过 8
    @return 低 count 位中，置位表示对应地址仍受保护
    */
    uint32_t protectedValues(Slot** slots, size_t count) const;

    /** 弱一致汇总块空闲栈和固定转移格，外部静止时准确
    @return 当前可复用槽位数量
    */
    size_t reusableFree() const;

    /** 轮转检查已发布块，先预留占用再弹出带身份的空闲索引
    @param [in] kind 1 为键，2 为值
    @param [in,out] guard 当前独占保护记录，缓存优先从自身领取
    @param [in,out] exhausted 观察到接纳序号耗尽时置为 true，保留已有 true
    @return 领取的独占槽位，无可用槽位为空
    */
    Slot* available(int32_t kind, LockFreeHashMapGuard& guard, bool& exhausted);

    /** 先原子预留额度，再申请和发布一个有界批次
    @param [in] requested 希望新增槽位数，内部按剩余预算和 65535 截断
    @param [in] initial 是否为初始化批次，为 true 时不计成功扩容历史
    @return 发布成功为 Ok，失败退还预留并提供明确原因
    */
    LockFreeHashMapStatus grow(size_t requested, bool initial);

    /** 执行一次用户或默认增长评估，不在 CAS 重试中重复调用策略
    @param [in,out] guard 当前独占保护记录，先尝试安全回收
    @param [in] empty 是否因实际无可用槽位触发，而非剩余阈值触发
    @return 评估或增长结果，不撤销已领取槽位
    */
    LockFreeHashMapStatus evaluateGrow(LockFreeHashMapGuard& guard, bool empty);

    /** 扣除完整固定元数据后计算地址空间允许的槽位上限
    @return 最大可寻址槽位数，要求 init 已校验配置
    */
    size_t maximumSlots() const;

    /** 禁止复制独占池及计数
    @param [in] other 不可复制的源池
    */
    LockFreeHashMapStorage(const LockFreeHashMapStorage& other);

    /** 禁止赋值独占池，配置只能在外部静止时事务替换
    @param [in] other 不可赋值的源池
    @return 无实现，不可调用
    */
    LockFreeHashMapStorage& operator=(const LockFreeHashMapStorage& other);
};

#include "LockFreeHashMapStorage.inl"