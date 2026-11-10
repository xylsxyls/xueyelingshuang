#pragma once
#include "LockFreeHashMapEpoch.h"
#include "LockFreeHashMapTypes.h"
#include <type_traits>

template <typename PayloadType> struct LockFreeHashMapPoolBlock;

/** 固定目录项，关闭块的容量观测不再解引用待释放地址
*/
struct LockFreeHashMapPoolEntry
{
public:
    // 零为空，一为申请预留，偶数为开放块，其他奇数仅供唯一维护者释放
    std::atomic<uintptr_t> m_address;
    // 原子容量副本，关闭块统计无需进入块内部
    std::atomic<size_t> m_count;
public:
    /** 构造空目录项
    */
    LockFreeHashMapPoolEntry();
};

/** 统一预算内的一个槽位；有效载荷只由成功取得槽位的线程构造
退役代际与位图发布回收责任，不另行分配历史链或退休记录
*/
template <typename PayloadType>
struct LockFreeHashMapPoolSlot
{
public:
    // 所属块存活到所有槽位和保护读者都退出
    LockFreeHashMapPoolBlock<PayloadType>* m_block;
    // 零为无记录；非零为待摘除或退役代际，位图交换领取退休责任；最大值耗尽后只能静止释放
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_retired;
    // 所属块内的固定下标，单块不超过 65535 槽，无截断
    uint32_t m_index;
    // 零为尚未构造；一为键节点，二为外置值，用于资源观测
    std::atomic<uint32_t> m_kind;
    // 原始对齐存储，不要求有效载荷可默认构造
    typename std::aligned_storage<sizeof(PayloadType), std::alignment_of<PayloadType>::value>::type m_payload;
public:
    /** 构造元数据，不构造用户对象
    */
    LockFreeHashMapPoolSlot();

    /** 返回有效载荷地址，调用方必须拥有槽位或持有保护
    @return 对齐后的存储地址，不代表对象已经构造
    */
    PayloadType* payload();
};

/** 一批固定槽位及两个位图，空闲分配不使用可发生地址 ABA 的链栈
块的占用包含正在申请槽位的短期预留；最高位关闭后不再接纳
*/
template <typename PayloadType>
struct LockFreeHashMapPoolBlock
{
public:
    // 槽数组原始申请地址，与 delete[] 配对
    unsigned char* m_allocation;
    // 手动对齐且已经构造的槽数组
    LockFreeHashMapPoolSlot<PayloadType>* m_slots;
    // 使用中位图与退休位图共用一个申请，各占 m_words 个原子字
    std::atomic<uint64_t>* m_bits;
    // 实际槽位数量，不向上取整突破预算
    size_t m_count;
    // 每个位图的原子字数量
    size_t m_words;
    // 低三十二位为占用和预留，高三十一位为不回绕活动序号，最高位永久关闭
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_leases;
    // 整块关闭代际，零为尚未关闭，最大值为释放者独占
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_retired;
    // 唯一维护者保存的连续空闲起点，零为尚未观察到空闲
    uint64_t m_idle;
    // 对应空闲起点的完整活动序号，重用后重新计时
    uint64_t m_idleLease;
    // 私有构造阶段实际发起的数组申请次数
    size_t m_allocationAttempts;
    // 每块独立推进有界回收位置，避免大块尾部永久得不到扫描
    std::atomic<size_t> m_reclaimCursor;
public:
    /** 初始化空批次，不执行系统申请
    */
    LockFreeHashMapPoolBlock();

    /** 释放已构造载荷和数组，要求没有访问者
    */
    ~LockFreeHashMapPoolBlock();

    /** 分配并构造固定数量槽位，失败可直接析构本块
    @param [in] count 非零且不大于 65535 的实际槽数
    @return 完成所有元数据构造为 true
    */
    bool initialize(size_t count);
};

/** 按块位图分配的无锁池，固定目录、统一容量预留及代际回收
所有槽地址的借用必须在 epoch 保护中；配置和析构要求外部静止
*/
template <typename PayloadType>
class LockFreeHashMapPool
{
public:
    /** 构造未初始化池，不分配资源
    */
    LockFreeHashMapPool();

    /** 外部静止时析构全部载荷、块和目录
    */
    ~LockFreeHashMapPool();

    /** 外部静止时一次性初始化配置和保护域
    @param [in] config 容量、策略及保护记录配置，复制仅共享策略句柄
    @return 明确的初始化状态，失败可安全析构
    */
    LockFreeHashMapStatus initialize(const LockFreeHashMapPoolConfig& config);

    /** 返回由池拥有的保护域
    @return 仅在池存活期间有效的保护域
    */
    LockFreeHashMapEpoch* epoch();

    /** 取得尚未构造的槽位，调用者负责构造成功后设置 m_kind
    @param [in,out] record 当前操作独占且已发布保护的记录
    @param [out] status 成功或容量、预留、策略、系统失败原因
    @return 成功取得槽位，否则为空；失败不隐式等待暂停线程
    */
    LockFreeHashMapPoolSlot<PayloadType>* acquire(LockFreeHashMapEpochRecord& record, LockFreeHashMapStatus& status);

    /** 归还从未发布的候选或已独占的安全退休槽
    @param [in] slot 调用者唯一拥有的槽，空值无操作
    */
    void release(LockFreeHashMapPoolSlot<PayloadType>* slot);

    /** 从所有结构入口摘除后发布退役责任，无额外分配
    @param [in] slot 已受保护且仅允许退役一次的槽
    */
    void retire(LockFreeHashMapPoolSlot<PayloadType>* slot);

    /** 有界清理退休槽；先推进扫描上界，再取得新的保护记录
    @param [in] wordLimit 最多访问的位图字数量，零表示一次完整扫描
    @return 实际析构并归还的槽数，记录耗尽时为零
    */
    size_t reclaim(size_t wordLimit = 0);

    /** 由外部唯一维护者评估动态缩容，不触发增长；可与数据操作并发
    @note 同一池的 shrink 不允许彼此并发；未安装策略也会完成此前关闭块的释放
    @return 策略异常或维护结果，m_count 表示本次实际释放的容量
    */
    LockFreeHashMapResult shrink();

    /** 取得包含元数据及暂存责任的弱一致观测
    @return 保护失败返回全零观测；budget 单独来自合并预算原子
    */
    LockFreeHashMapPoolStats stats();

    /** 外部静止时释放全部块，保留固定目录和保护域以便再次使用
    */
    void freeBlocks();

    /** 查询固定配置，仅允许在池存活期间借用
    @return 初始化后的配置引用
    */
    const LockFreeHashMapPoolConfig& config() const;

    /** 外部静止时替换策略句柄，不复制用户回调对象
    @param [in] grow 增长策略，空句柄恢复默认算法
    @param [in] shrink 缩容策略，空句柄关闭新的自动缩容
    */
    void setPolicies(const LockFreeHashMapGrowPolicy& grow, const LockFreeHashMapShrinkPolicy& shrink);
private:
    /** 在一个已经受保护的块内尝试取得槽位
    @param [in] block 目录中读取到的非空块
    @param [in,out] record 当前记录独占的位图起点及固定槽缓存
    @return 尚未构造的槽位；满块或关闭块为空
    */
    LockFreeHashMapPoolSlot<PayloadType>* take(LockFreeHashMapPoolBlock<PayloadType>* block, LockFreeHashMapEpochRecord& record);

    /** 非阻塞借用一份空闲记录，转移一个缓存槽，不等待记录所有者
    @param [in,out] record 当前独占记录
    @return 转移的槽，所有可检查记录都不可用时为空
    */
    LockFreeHashMapPoolSlot<PayloadType>* transfer(LockFreeHashMapEpochRecord& record);

    /** 析构安全槽并优先放回当前记录的固定缓存，满缓存才归还位图
    @param [in] slot 已独占或安全退休的槽
    @param [in,out] record 当前独占记录
    */
    void recycle(LockFreeHashMapPoolSlot<PayloadType>* slot, LockFreeHashMapEpochRecord& record);

    /** 唯一所有者析构载荷并清理状态，不提前发布可复用位
    @param [in] slot 已独占或已经安全领取的退役槽，析构不得抛异常
    */
    void destroyPayload(LockFreeHashMapPoolSlot<PayloadType>* slot);

    /** 将当前独占记录的原始缓存槽归回块，供缩容和容量回收使用
    @param [in,out] record 当前独占或已经成功借用的空闲记录
    */
    void drain(LockFreeHashMapEpochRecord& record);

    /** 扫描当前可见目录，不调用策略或分配
    @param [in,out] record 当前独占保护记录
    @return 已取得的槽位，否则为空
    */
    LockFreeHashMapPoolSlot<PayloadType>* scan(LockFreeHashMapEpochRecord& record);

    /** 扩容预留、申请并发布一个批次，失败完整退回额度
    @param [in] requested 期望新增槽数，实际还受单块和总额度限制
    @param [in] initial 是否属于初始化批次
    @return 发布成功或明确的失败结果
    */
    LockFreeHashMapStatus grow(size_t requested, bool initial);

    /** 在需求触发点评估用户或默认增长策略
    @param [in] exhausted 已完整扫描并尝试转移后仍未取得槽时为 true
    @return 增长结果，不代表调用者必定取得新增槽
    */
    LockFreeHashMapStatus evaluateGrowth(bool exhausted);

    /** 在保护下求和已发布容量和可复用槽数
    @param [out] capacity 包含关闭待释放块，不含预留中的批次
    @param [out] available 当前可复用槽数的弱一致观测
    */
    void observe(size_t& capacity, size_t& available);

    /** 帮助把已关闭块标记到目录中并发布退休代际
    @param [in] index 固定目录下标
    @param [in] block 当前受保护且永久关闭的块
    */
    void close(size_t index, LockFreeHashMapPoolBlock<PayloadType>* block);

    /** 取得稳态单调毫秒数，异常时禁用本次空闲计时
    @return 非零时间戳；时钟失败为零
    */
    static uint64_t now();

    /** 统计位图中的置位数量
    @param [in] bits 一个六十四位位图字
    @return 零至六十四的数量
    */
    static size_t population(uint64_t bits);

    /** 查找非零位图的最低置位位置
    @param [in] bits 必须非零
    @return 零至六十三的下标
    */
    static size_t firstBit(uint64_t bits);

    /** 饱和累计诊断次数，耗尽不回绕
    @param [in,out] counter 当前池拥有的计数原子
    */
    static void increment(std::atomic<uint64_t>& counter);

    /** 禁止复制独占的目录及池资源
    @param [in] other 不可复制的池
    */
    LockFreeHashMapPool(const LockFreeHashMapPool& other);

    /** 禁止覆盖独占池的所有权
    @param [in] other 不可赋值的池
    @return 无实现
    */
    LockFreeHashMapPool& operator=(const LockFreeHashMapPool& other);
private:
    // 一次初始化的固定配置，运行期间不更换回调
    LockFreeHashMapPoolConfig m_config;
    // 固定保护域，生命周期覆盖所有块
    LockFreeHashMapEpoch m_epoch;
    // 固定目录，只有唯一缩容维护者可以解引用奇数关闭地址
    LockFreeHashMapPoolEntry* m_directory;
    // 曾经使用的目录上界，仅用于缩短扫描，不作为身份
    std::atomic<size_t> m_high;
    // 已保留及尚未发布预留的统一预算
    std::atomic<size_t> m_budget;
    // 统一预算高水位
    std::atomic<size_t> m_peak;
    // 非初始批次成功发布次数
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_grows;
    // 块、槽和位图的系统申请次数
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_allocations;
    // 最近一次成功增长的单调时间
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<uint64_t> m_lastGrow;
    // 有界回收的目录轮转位置
    std::atomic<size_t> m_cursor;
};

/** 未发布候选的独占归还器，供标准 unique_ptr 在所有异常路径归还池槽
*/
template <typename PayloadType>
struct LockFreeHashMapPoolReturn
{
public:
    // 借用的所属池，必须比候选所有者活得更久
    LockFreeHashMapPool<PayloadType>* m_pool;
public:
    /** 绑定候选所属池，不取得池所有权
    @param [in] pool 非空且存活的池
    */
    explicit LockFreeHashMapPoolReturn(LockFreeHashMapPool<PayloadType>* pool);

    /** 归还尚未发布的槽位
    @param [in] slot 唯一拥有的候选，可为空
    */
    void operator()(LockFreeHashMapPoolSlot<PayloadType>* slot) const;
};

#include "LockFreeHashMapPool.inl"