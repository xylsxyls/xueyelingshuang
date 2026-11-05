#pragma once
#include "LockFreeMapContract.h"
#include "LockFreeMapReclamation.h"
#include "LockFreeMapClock.h"
#include <cstddef>
#include <atomic>
#include <chrono>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

template <typename T> struct LockFreeMapPoolBlock;

/** 槽载荷析构入口，内部键类型可把操作记录传给其拥有的值池
*/
template <typename T>
struct LockFreeMapPoolDestroy
{
public:
    /** 析构独占载荷；用户析构须遵守不抛出约定
    @param [in,out] value 待析构的已构造对象
    @param [in] record 本次独占操作记录，允许 nullptr
    */
    static void destroy(T& value, LockFreeMapHazardRecord* record);
};

/** 可复用有效载荷槽，生命期由拥有引用和值保护共同决定
*/
template <typename T>
struct LockFreeMapPoolSlot
{
public:
    // 拥有引用数；0 空闲，最大值析构中，减一退役，减二构造中，减三为原始槽缓存或复用构造
    std::atomic<size_t> m_references;
    // 所属内存块，在占用数归零前保持有效
    LockFreeMapPoolBlock<T>* m_block;
    // 不回绕的条目版本，键槽固定为零
    uint64_t m_version;
    // 满足类型对齐的原始存储，仅在槽被占用时构造对象
    typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type m_storage;

public:
    /** 构造未含有效载荷的空槽
    */
    LockFreeMapPoolSlot();

    /** 在拥有引用或保护范围内读取已构造对象
    @return 借用的不可变有效载荷
    */
    const T& value() const;

    /** 访问其可变字段已独立原子同步的对象
    @return 在拥有引用或保护范围内借用的对象
    */
    T& mutableValue();
};

/** 可整体释放的分配批次，占用数最高位表示停止分配
*/
template <typename T>
struct LockFreeMapPoolBlock
{
public:
    // 在途申请及有效槽位占用数，最高位关闭整块
    std::atomic<size_t> m_occupancy;
    // 验证连续空闲的饱和复用序号
    std::atomic<uint64_t> m_activity;
    // 本块拥有的原始槽数组
    LockFreeMapPoolSlot<T>* m_slots;
    // 本批次槽位数
    size_t m_count;
    // 下次查找空槽的位置提示，本身不授予所有权
    std::atomic<size_t> m_hint;
    // 固定目录位置，仅用作需重新校验的分配提示
    size_t m_directory;
    // 开始空闲计时时观察到的活动序号
    uint64_t m_idleActivity;
    // 单调毫秒加一，零表示尚未观察到空闲起点
    uint64_t m_idleStart;

public:
    /** 分配尚未构造有效载荷的槽数组
    @param [in] count 槽位数，须先通过容量和字节校验
    */
    explicit LockFreeMapPoolBlock(size_t count);

    /** 释放空块及槽数组
    */
    ~LockFreeMapPoolBlock();
};

/** 稳定块目录：0 空闲、1 私有或维护占用、2 活动、3 退役
*/
template <typename T>
struct LockFreeMapPoolRecord
{
public:
    // 目录生命周期状态，占用不会要求其他线程等待
    std::atomic<uint32_t> m_state;
    // 所属内存块，在占用数归零前保持有效
    std::atomic<LockFreeMapPoolBlock<T>*> m_block;
    // 已摘除的块，仅退役扫描器负责释放
    LockFreeMapPoolBlock<T>* m_retired;

public:
    /** 构造未占用的目录记录
    */
    LockFreeMapPoolRecord();
};

/** 元数据有界、扩容独立预留的无锁槽池
@note 分配使用每操作记录的有界提示，不等待其他分配者；用户析构不得抛异常
*/
template <typename T>
class LockFreeMapPool
{
public:
    /** 构造池元数据，初始有效载荷容量由 initialize 显式申请
    @param [in] config 外部静止期取得的策略快照
    @param [in] domain 借用共享保护域，必须活到池析构后
    @param [in] deferred 是否将值版本保留到全部借用者离开
    */
    LockFreeMapPool(const LockFreeMapPoolConfig& config, LockFreeMapReclamation& domain, bool deferred = false);

    /** 外部静止期析构并释放资源
    */
    ~LockFreeMapPool();

    /** 无分配地校验数量及字节换算
    @param [in] config 拟采用的池配置
    @return 所有边界均可表示时返回 true
    */
    static bool valid(const LockFreeMapPoolConfig& config);

    /** 发布初始容量，不计入累计成功扩容次数
    @return 返回 OK 或申请失败状态
    */
    LockFreeMapStatus initialize();

    /** 取得实际槽位并复制构造对象
    @param [in] value 在复制期间保持有效的源对象
    @param [in] version 唯一条目版本，键对象为零
    @param [in] guard 当前操作的保护守卫
    @param [out] status 明确的分配或构造结果
    @return 拥有引用的槽位，失败返回 nullptr
    */
    LockFreeMapPoolSlot<T>* create(const T& value, uint64_t version, LockFreeMapGuard& guard, LockFreeMapStatus& status);

    /** 在独占槽位中直接构造，避免临时键的额外复制
    @param [in] version 唯一条目版本，键对象为零
    @param [in] guard 当前操作的保护守卫
    @param [out] status 明确的分配或构造结果
    @param [in] arguments 仅在取得槽位后传给对象构造函数的参数
    @return 拥有引用的槽位，失败返回 nullptr
    */
    template <typename... Arguments>
    LockFreeMapPoolSlot<T>* emplace(uint64_t version, LockFreeMapGuard& guard, LockFreeMapStatus& status, Arguments&&... arguments);

    /** 为已有拥有引用的槽增加一个拥有引用
    @param [in] slot 已构造且仍由拥有引用保护的槽
    */
    void retain(LockFreeMapPoolSlot<T>* slot);

    /** 仅在仍有拥有者时将保护借用转换为拥有引用
    @param [in] slot 受保护指针保护的值版本
    @return 已退役返回 false，不复活退役槽
    */
    bool tryRetain(LockFreeMapPoolSlot<T>* slot);

    /** 释放一个拥有引用，在安全时析构最后的有效载荷
    @param [in] slot 已构造槽位，nullptr 表示无需释放
    @param [in] record 可选独占记录，用刚释放的稳定槽位更新分配提示
    */
    void release(LockFreeMapPoolSlot<T>* slot, LockFreeMapHazardRecord* record = nullptr);

    /** 归还已从所有可见入口摘除的拥有引用，省去独占末引用的 CAS
    @param [in] slot 已摘除槽，允许 nullptr；引用数为一时须保证没有新的引用取得路径
    @param [in] record 当前独占记录，允许 nullptr；已有其他拥有引用仍走普通归还
    */
    void releaseDetached(LockFreeMapPoolSlot<T>* slot, LockFreeMapHazardRecord* record = nullptr);

    /** 评估策略，并按预算访问块记录
    @param [in] budget 最多检查的目录记录数
    @param [out] released 实际释放整块内存的槽位数
    @return 返回 OK 或策略异常状态
    */
    LockFreeMapStatus maintain(size_t budget, size_t& released);

    /** 观察池字节及各类槽位数量
    @return 各字段独立原子观察，不保证整组事务一致
    */
    LockFreeMapPoolStats stats() const;

    /** 外部静止期显式释放，包括初始容量
    */
    void freeMemory();

    /** 取得固定目录元数据字节
    @return 有效载荷申请之外的目录字节数
    */
    size_t metadataBytes() const;

    /** 内部饱和计数操作，达到上限后不再递增
    @param [in,out] counter 整个池生命期的累计计数
    */
    static void increment(std::atomic<uint64_t>& counter);

private:
    /** 原子预留额度后申请并发布一个批次
    @param [in] initial 仅初始容量申请时为 true
    @return 区分容量预留、策略和系统申请失败的状态
    */
    LockFreeMapStatus grow(bool initial);

    /** 扫描已有槽并取得原始存储的独占权
    @param [in] guard 本次操作保护守卫
    @return 已独占但未构造的槽，失败返回 nullptr
    */
    LockFreeMapPoolSlot<T>* take(LockFreeMapGuard& guard);

    /** 析构已取得独占销毁权的槽并归还存储
    @param [in] slot 处于析构独占状态的槽
    @param [in] record 可选的调用方独占分配提示
    */
    void destroy(LockFreeMapPoolSlot<T>* slot, LockFreeMapHazardRecord* record = nullptr);

    /** 回收本次维护占用块内的退役值
    @param [in] block 受目录维护状态保护的块
    */
    void collect(LockFreeMapPoolBlock<T>* block);

    /** 观察所有已取槽位，包含构造及析构中的占用
    @return 池占用数的弱一致观察
    */
    size_t used() const;

    /** 读取单调毫秒样本
    @return 单调时钟毫秒加一，不可用时为零
    */
    static uint64_t now();

    /** 禁止复制拥有资源的池
    @param [in] other 源对象
    */
    LockFreeMapPool(const LockFreeMapPool& other);

    /** 禁止复制拥有资源的池
    @param [in] other 源对象
    @return 当前池引用，函数禁用
    */
    LockFreeMapPool& operator=(const LockFreeMapPool& other);

private:
    // 不变配置及其拥有的策略对象
    LockFreeMapPoolConfig m_config;
    // 借用共享有界保护域，其生命期覆盖本池
    LockFreeMapReclamation* m_domain;
    // 固定块目录，包含私有及退役批次
    LockFreeMapPoolRecord<T>* m_records;
    // 原子保留加预留额度，所有分配先预留
    std::atomic<size_t> m_budget;
    // 已发布保留槽数，包含退役块
    std::atomic<size_t> m_capacity;
    // 仍开放分配的块容量
    std::atomic<size_t> m_openCapacity;
    // 可缩容块的已构造对象计数；固定块统计直接观察槽状态
    std::atomic<size_t> m_retained;
    // 可缩容块的取槽数，包含构造及析构阶段
    std::atomic<size_t> m_inUse;
    // 块及槽数组申请字节，不含分配器开销
    std::atomic<size_t> m_bytes;
    // 有界维护轮转游标，允许无符号回绕
    std::atomic<size_t> m_cursor;
    // 初始化后成功发布批次数，达到上限后饱和
    std::atomic<uint64_t> m_grows;
    // 成功块及数组申请次数，达到上限后饱和
    std::atomic<uint64_t> m_allocations;
    // 最近成功扩容时间，单调时钟不可用时为零
    std::atomic<uint64_t> m_lastGrow;
    // 值版本池在存在保护借用时延迟析构
    bool m_deferred;
    // 仅空策略或已知内置禁用缩容策略时为 false
    bool m_canShrink;
    // 等待保护借用者离开的退役已构造槽数
    std::atomic<size_t> m_pending;
    // 曾发布的最高目录位置加一
    std::atomic<size_t> m_span;
    // 分配目录起点提示，由成功取槽更新
    std::atomic<size_t> m_hint;
};

#include "LockFreeMapPool.inl"