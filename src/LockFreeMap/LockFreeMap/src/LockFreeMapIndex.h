#pragma once
#include "LockFreeMapPool.h"
#include <cstring>

/** 稳定键身份，当前不可变值版本通过独立原子指针发布
*/
template <typename K, typename V>
struct LockFreeMapKey
{
public:
    // 发布后不修改的比较键
    K m_key;
    // 拥有当前值版本，nullptr 表示该键身份永久关闭
    std::atomic<LockFreeMapPoolSlot<V>*> m_value;
    // 借用值池，池必须活到所有键及其当前值释放后
    LockFreeMapPool<V>* m_pool;

public:
    /** 构造尚未发布值的私有键对象
    @param [in] key 用于排序和等价判定的键
    @param [in] pool 拥有值槽的池，借用至键对象析构
    */
    LockFreeMapKey(const K& key, LockFreeMapPool<V>& pool);

    /** 构造完整的私有键对象，成功后接管值引用
    @param [in] key 用于排序和等价判定的键
    @param [in] pool 拥有值槽的池，借用至键对象析构
    @param [in] value 待转交的值；键复制失败时仍由调用方负责
    */
    LockFreeMapKey(const K& key, LockFreeMapPool<V>& pool, LockFreeMapPoolSlot<V>* value);

    /** 只复制私有键；值所有权单独转交
    @param [in] other 尚未发布的源键对象
    */
    LockFreeMapKey(const LockFreeMapKey& other);

    /** 最后一个索引拥有者退出后释放当前值
    */
    ~LockFreeMapKey();

};

/** 键载荷的析构入口，把已析构值槽归还给同一次操作的值缓存
*/
template <typename K, typename V>
struct LockFreeMapPoolDestroy<LockFreeMapKey<K, V>>
{
public:
    /** 在键槽最后一个引用释放后析构其值和键
    @param [in,out] value 没有读者可以借用的键对象
    @param [in] record 当前独占操作记录，允许 nullptr
    */
    static void destroy(LockFreeMapKey<K, V>& value, LockFreeMapHazardRecord* record);

};

/** 不可变有序索引中的一个拥有引用
*/
template <typename K, typename V>
struct LockFreeMapIndexEntry
{
    // 包含此键的各索引代次共同持有的稳定键身份
    LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* m_key;
};

/** 有界数组缓存记录，零引用时可以重新领取容量，不保留键对象引用
*/
template <typename K, typename V>
struct LockFreeMapIndexBuffer
{
public:
    // 正式索引持有的数组引用，私有借用由其构造者的源保护保证有效
    std::atomic<size_t> m_references;
    // 本记录拥有的数组；覆盖描述屏蔽的位置在独占数组引用时允许补写
    LockFreeMapIndexEntry<K, V>* m_entries;
    // 已申请容量，包含空闲缓存
    size_t m_capacity;

public:
    /** 初始化没有分配数组的空记录
    */
    LockFreeMapIndexBuffer();

};

/** 稳定索引记录：0 空闲、1 私有、2 发布完成、3 部分回收、4 扫描占用
*/
template <typename K, typename V>
struct LockFreeMapIndex
{
public:
    // 空闲、私有、已发布、退役、扫描的生命周期状态
    std::atomic<uint32_t> m_state;
    // 拥有的有序引用数组，发布期间不修改可见条目
    LockFreeMapIndexEntry<K, V>* m_entries;
    // 原始条目数，逻辑计数跳过已提交删除项
    std::atomic<size_t> m_count;
    // 扫描器已经释放的条目引用数
    size_t m_reclaimed;
    // 删除决策：0 普通索引、1 待帮助、2 已提交、3 冲突
    std::atomic<uint32_t> m_decision;
    // 额外键拥有引用，保证暂停帮助者完成访问前键仍有效
    LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* m_target;
    // 额外版本拥有引用，防止帮助者比较地址时发生 ABA
    LockFreeMapPoolSlot<V>* m_expected;
    // 仅在删除提交后跳过的原始数组位置
    size_t m_omitted;
    // 本索引的省略项拥有引用是否已经释放；不修改共享数组中的原始指针
    bool m_omittedReleased;
    // 稳定数组记录，活动索引拥有引用，私有候选可以暂时借用源引用
    LockFreeMapIndexBuffer<K, V>* m_buffer;
    // 候选独有的新键引用，转换为正式索引后清空此标记
    LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* m_added;
    // 私有候选是否借用源数组引用，完成前由构造者保护源记录
    bool m_borrowedBuffer;
    // 单项覆盖的原始位置；等于原始条目数时没有覆盖
    size_t m_overridePosition;
    // 已发布视图中的单项新键，在补写共享数组前由此处提供
    LockFreeMapIndexEntry<K, V> m_overrideEntry;
    // 非零表示覆盖已写入数组；后继只有 acquire 观察后才能省略覆盖描述
    std::atomic<uint32_t> m_merged;

public:
    /** 帮助完成待定删除后读取逻辑条目数
    @return 原始数减去已提交删除项，无删除时返回原始数
    */
    size_t count() const;

    /** 选择逻辑有序条目，跳过已提交删除项
    @param [in] position 小于 count() 的逻辑位置
    @return 借用不可变键引用，期限由受保护索引决定
    */
    const LockFreeMapIndexEntry<K, V>& entry(size_t position) const;

    /** 读取原始位置对应的不可变视图，优先使用已发布的单项覆盖
    @param [in] position 小于原始条目数，省略项只能由拥有引用的维护路径访问
    @return 本视图的原始条目，不读取被覆盖位置的数组存储
    */
    const LockFreeMapIndexEntry<K, V>& rawEntry(size_t position) const;

    /** 构造可复用的空索引记录
    */
    LockFreeMapIndex();

};

/** 容器内部存储；键集合变更发布索引，值更新仅发布一个槽
*/
template <typename K, typename V, typename Less>
class LockFreeMapIndexStore
{
public:
    // 实例生命周期内稳定且线程安全的严格弱序比较器
    Less m_less;
    // 初始化后不再修改的配置快照
    LockFreeMapConfig m_config;
    // 每实例有界操作登记域
    LockFreeMapReclamation m_domain;
    // 独立键槽预算
    LockFreeMapPool<LockFreeMapKey<K, V>> m_nodes;
    // 独立值版本预算
    LockFreeMapPool<V> m_values;
    // 仅与外部凭据共享的小型实例身份
    std::shared_ptr<const LockFreeMapIdentity> m_identity;
    // 结构发布位置，nullptr 表示空容器
    std::atomic<LockFreeMapIndex<K, V>*> m_root;
    // 不回绕的版本预留高水位，显式释放内存不重置
    std::atomic<uint64_t> m_version;
    // 写入 CAS 冲突次数，达到上限后饱和
    std::atomic<uint64_t> m_retries;
    // 全部私有、活动及退役引用数组的申请字节
    std::atomic<size_t> m_indexBytes;
    // 稳定描述符登记表，并发使用期间不移动
    LockFreeMapIndex<K, V>* m_indices;
    // 与索引记录同样有界的数组缓存目录
    LockFreeMapIndexBuffer<K, V>* m_buffers;
    // 固定描述符数，每操作四个再加四个
    size_t m_indexCount;
    // 轮转回收游标，允许无符号回绕
    std::atomic<size_t> m_cursor;
    // 曾经占用的最高描述符位置加一
    std::atomic<size_t> m_span;
    // 曾成功领取的缓冲区上界；维护不扫描从未使用的预留记录
    std::atomic<size_t> m_bufferSpan;

public:
    /** 申请有界元数据
    @param [in] config 已校验的静止期配置
    */
    explicit LockFreeMapIndexStore(const LockFreeMapConfig& config);

    /** 外部静止期释放全部拥有资源
    */
    ~LockFreeMapIndexStore();

    /** 保护不可变根，仅在根实际变化时重试
    @param [in] guard 当前操作的保护守卫
    @return 受保护索引，空容器可能返回 nullptr
    */
    LockFreeMapIndex<K, V>* protect(LockFreeMapGuard& guard);

    /** 借用受保护键的当前不可变值
    @param [in] key 由受保护索引持有的键槽
    @param [in] guard 使用保护指针 1 发布值借用
    @return 受保护版本，键已关闭时返回 nullptr
    */
    LockFreeMapPoolSlot<V>* protectValue(LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* key, LockFreeMapGuard& guard);

    /** 帮助完成已发布删除，不等待原发布线程
    @param [in] index 受保护删除计划，或调用方独占的候选
    @return 普通、提交成功或冲突的稳定决策
    */
    uint32_t help(LockFreeMapIndex<K, V>* index);

    /** 按用户严格弱序及局部位置提示执行二分查找
    @param [in] index 受保护索引
    @param [in] record 调用方独占的可选位置提示
    @param [in] key 按比较器查询的键
    @param [out] found 是否存在比较器等价的键
    @return 下界位置；比较异常由上层公开接口边界转换
    */
    size_t find(LockFreeMapIndex<K, V>* index, const K& key, bool& found, LockFreeMapHazardRecord* record = nullptr) const;

    /** 取得不回绕的条目版本
    @param [out] version 非零版本号
    @return 版本达到生命周期上限时返回 false
    */
    bool nextVersion(uint64_t& version);

    /** 从每操作记录的有界预留段取得唯一版本
    @param [out] version 不透明非零版本号，不表示时间戳
    @param [in] guard 独占保护记录的守卫；预留段跨清空和释放保留
    @return 无法在不回绕的前提下取得版本时返回 false
    */
    bool nextVersion(uint64_t& version, LockFreeMapGuard& guard);

    /** 构造借用共同键的候选索引，成功后接管新键引用，尚不修改根
    @param [in] source 受保护的源索引，空容器时为 nullptr
    @param [in] position 拟插入或删除条目的逻辑位置
    @param [in] found 源索引在指定位置是否有匹配条目
    @param [in] key 候选键槽，nullptr 表示删除
    @param [in] value 删除计划期待的值版本，插入时为 nullptr
    @param [in] guard 始终保护源索引，直到 complete 或 discard 完成
    @param [out] status 失败原因或 OK
    @param [in] logicalOnly 无输出的无条件删除允许在根 CAS 处提交
    @return 私有候选，失败为 nullptr；根 CAS 后必须调用 complete 或 discard
    */
    LockFreeMapIndex<K, V>* build(LockFreeMapIndex<K, V>* source, size_t position, bool found,
        LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* key, LockFreeMapPoolSlot<V>* value,
        LockFreeMapGuard& guard, LockFreeMapStatus& status, bool logicalOnly = false);

    /** 将成功发布的私有候选转为独立拥有者，不能分配或等待其他调用
    @param [in] candidate 本调用状态为私有的已发布候选
    @param [in] source 构造期间始终由 guard 保护的源，允许 nullptr
    @param [in] guard 仍保护源记录的构造者
    */
    void complete(LockFreeMapIndex<K, V>* candidate, LockFreeMapIndex<K, V>* source, LockFreeMapGuard& guard);

    /** 归还从未发布的私有候选，不释放借用的共同键
    @param [in] candidate build 成功返回、根 CAS 失败的私有记录
    */
    void discard(LockFreeMapIndex<K, V>* candidate);

    /** 清理已提交省略项及无帮助者使用的额外计划引用，调用方须独占状态1或4
    @param [in] index 独占维护的索引
    @param [in] completed 已完成计划访问的本次记录，允许 nullptr
    @return 本次释放的省略条目引用数，只可能为零或一
    */
    size_t clean(LockFreeMapIndex<K, V>* index, LockFreeMapHazardRecord* completed);

    /** 领取可复用数组容量，失败不改变其他索引
    @param [in] count 最低条目容量
    @param [out] status 明确的容量、记录或系统申请结果
    @return 带一个拥有引用的缓存记录，零容量或失败为 nullptr
    */
    LockFreeMapIndexBuffer<K, V>* buffer(size_t count, LockFreeMapStatus& status);

    /** 分段复制有序指针视图，跳过源已省略的位置，不增加键引用
    @param [out] destination 已有足够空间的私有目标数组
    @param [in] source 受保护源索引
    @param [in] begin 逻辑起点
    @param [in] count 复制条目数，零允许空源
    */
    void copyEntries(LockFreeMapIndexEntry<K, V>* destination, LockFreeMapIndex<K, V>* source, size_t begin, size_t count);

    /** 在预算内释放退役索引引用，不等待读者
    @param [in] budget 最多释放的条目引用数
    @param [in] completed 已完成 help 的调用记录，仅释放计划字段时可忽略其保护
    @param [in] releaseBuffers 是否释放零引用数组缓存，显式维护时启用
    @return 实际释放的条目引用数
    */
    size_t collect(size_t budget, LockFreeMapHazardRecord* completed = nullptr, bool releaseBuffers = false);

    /** 静止期清空并释放内存，保留版本计数
    */
    void freeMemory();

};

#include "LockFreeMapIndex.inl"