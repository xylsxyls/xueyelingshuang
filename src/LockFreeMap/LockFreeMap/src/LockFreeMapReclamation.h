#pragma once
#include "LockFreeMapClock.h"
#include <atomic>
#include <stddef.h>
#include <stdint.h>
#include <thread>
#include <functional>
#include <new>
#include <limits>

/** 同一操作独占写入的保护指针，隔离相邻热点字段
*/
struct LockFreeMapHazardPointer
{
public:
    // 受保护地址，参与安全协议的发布均使用顺序一致内存序
    std::atomic<void*> m_value;
#ifdef __EMSCRIPTEN__
    // 发布前固定：是否兼作领取状态；原始零地址表示空闲，标记一表示已领取但无源
    bool m_claimRoot;
    // 与原布局保留相同的隔离空间
    unsigned char m_padding[63];
#else
    // 避免相邻保护指针被不同扫描路径共同写入缓存行
    unsigned char m_padding[64];
#endif

public:
    /** 初始化空保护指针
    */
    LockFreeMapHazardPointer();

    /** 发布或清空保护指针
    @param [in] value 受保护地址，清空时为 nullptr
    @param [in] order 原子内存序，发布默认顺序一致，清空可用 release
    */
    void store(void* value, std::memory_order order = std::memory_order_seq_cst);

    /** 读取保护指针
    @param [in] order 原子内存序，发布默认顺序一致，清空可用 release
    @return 已发布的受保护地址
    */
    void* load(std::memory_order order = std::memory_order_seq_cst) const;
};

/** 每实例固定保护记录，不依赖线程局部变量或线程退出登记
*/
struct LockFreeMapHazardRecord
{
public:
    // 原生后端的领取状态；Wasm 使用源保护位合并记录所有权
    std::atomic<uint32_t> m_claimed;
    // 线程亲和提示，碰撞或线程 ID 复用都不授予所有权
    std::atomic<size_t> m_threadHint;
    // 顺序一致保护：源根、值版本、分配块及保留位；私有候选由状态和源保护覆盖
    LockFreeMapHazardPointer m_pointer[4];
    // 本调用独占的键池和值池目录提示
    size_t m_directoryHint[2];
    // 本调用独占的槽位置，每次使用重新检查块边界
    size_t m_slotHint[2];
    // 上次逻辑查找位置，仅坐标提示，不保存借用指针
    size_t m_searchHint;
    // 上次命中重复或相邻位置后才启用局部查找提示
    bool m_searchLocality;
    // 本记录独占的版本预留段，零表示没有剩余编号
    uint64_t m_versionNext;
    // 预留段的闭区间末尾，不超过 UINT64_MAX
    uint64_t m_versionLast;
    // 本记录有界维护节奏，与版本预留计数独立
    uint32_t m_maintenanceTick;
    // 本记录上的值 CAS 竞争次数，由独占调用写入，统计接口可并发读取
    std::atomic<uint64_t> m_valueRetries;
    // 本记录独占的空索引描述符，静止期释放同步清空此指针
    void* m_indexCache;
    // 值池优先搜索的分散起点，仅影响探测顺序，不预占槽或限制其他调用
    size_t m_poolHome;
    // 两个通道各保留一个已析构槽，记录独占者可直接复用，空闲记录允许别人转取
    void* m_poolCache[2];
    // 空槽所属池，防止同一个保护域内的不同池混用地址
    void* m_poolOwner[2];
    // 隔开相邻调用记录的频繁写字段，避免共享同一缓存行
    unsigned char m_padding[64];

public:
    /** 初始化可取得的空操作记录
    */
    LockFreeMapHazardRecord();

    /** 观察记录是否已被调用独占，不授予访问私有字段的权限
    @param [in] order 观察使用的原子内存序
    @return 是否已占用
    */
    bool claimed(std::memory_order order = std::memory_order_seq_cst) const;

    /** 仅尝试取得空闲记录，不等待当前持有者
    @param [in] success 成功取得独占权的内存序
    @param [in] failure 失败观察的内存序
    @param [in] source Wasm 同步发布的初始源提示，取得记录后必须验证实际源
    @return 是否取得独占权
    */
    bool claim(std::memory_order success = std::memory_order_seq_cst,
        std::memory_order failure = std::memory_order_seq_cst, void* source = nullptr);

    /** 清空源保护并归还记录；调用者须已清空其他保护且结束私有访问
    */
    void release();
};

/** 有界保护域，析构要求外部静止
*/
class LockFreeMapReclamation
{
public:
    /** 分配固定数量的保护记录
    @param [in] count 并发及重入操作上限
    */
    explicit LockFreeMapReclamation(size_t count);

    /** 释放固定登记数组
    */
    ~LockFreeMapReclamation();

    /** 尝试独占一个保护记录，不等待已占用记录
    @return 取得的记录，全部占用时返回 nullptr
    */
    LockFreeMapHazardRecord* acquire();

    /** 取得记录时同时发布初始源提示，原生后端仍单独发布保护
    @param [in] source 初始源提示，只可在完成登记和源验证后解引用
    @return 取得的记录，全部占用时返回 nullptr
    */
    LockFreeMapHazardRecord* acquire(void* source);

    /** 扫描全部已登记保护指针
    @param [in] pointer 已摘除对象的地址
    @param [in] ignored 已确认不再访问计划字段的调用记录，无则为 nullptr
    @return 仍有调用可能借用该地址时返回 true
    */
    bool protects(void* pointer, const LockFreeMapHazardRecord* ignored = nullptr) const;

    /** 扫描值版本借用，仅检查专用保护指针 1
    @param [in] pointer 已从当前值位置摘除的槽地址
    @return 是否仍有读者借用该版本
    */
    bool protectsValue(void* pointer) const;

    /** 观察当前已占用记录数
    @return 弱一致的活动操作数量
    */
    size_t active() const;

    /** 取得固定元数据大小
    @return 已分配的记录数组字节
    */
    size_t bytes() const;

    /** 汇总已登记记录的值竞争次数，达到上限时饱和
    @return 弱一致的累计重试次数
    */
    uint64_t valueRetries() const;

    /** 判断本实例是否已经登记过多个同时或重入调用记录
    @return 高水位超过一时返回 true，仅用于提高维护频率，不授予访问权限
    */
    bool multipleRecords() const;

    /** 外部静止期清空无资源的索引提示，禁止跨索引记录重置继续使用
    */
    void clearIndexCaches();

    /** 尝试转取空闲记录缓存的已析构槽，不等待活动调用
    @param [in] pool 请求的池身份，必须与缓存所有者一致
    @param [in] channel 键池为零，值池为一
    @return 独占空槽地址，没有可转取槽时为 nullptr
    */
    void* takeIdleCache(void* pool, size_t channel);

    /** 外部静止期清空指定池的空槽提示，在释放块前调用
    @param [in] pool 即将释放内存的池身份
    */
    void clearPoolCaches(void* pool);

private:
    /** 取得不授予所有权的线程亲和提示，无需线程局部登记
    @return 提示允许复用或碰撞，只有记录 CAS 才授予独占权
    */
    static size_t threadHint();

    /** 禁止复制保护域
    @param [in] other 源对象
    */
    LockFreeMapReclamation(const LockFreeMapReclamation& other);

    /** 禁止复制保护域
    @param [in] other 源对象
    @return 当前保护域引用，函数禁用
    */
    LockFreeMapReclamation& operator=(const LockFreeMapReclamation& other);

private:
    // 本实例拥有的固定登记数组
    LockFreeMapHazardRecord* m_records;
    // 同时活动操作上限，包含重入调用
    size_t m_count;
    // 曾登记的最高记录位置加一，在读取源指针前发布
    std::atomic<size_t> m_span;
};

/** 操作登记守卫，保护根、值版本和池块，并持有私有候选的源保护
*/
class LockFreeMapGuard
{
public:
    // 本调用独占记录，容量不足时为 nullptr
    LockFreeMapHazardRecord* m_record;

public:
    /** 取得一个独占操作记录
    @param [in] domain 所属容器保护域，必须覆盖守卫生命期
    */
    explicit LockFreeMapGuard(LockFreeMapReclamation& domain);

    /** 取得记录并预先发布源提示，调用方仍须核对源指针后才能访问
    @param [in,out] domain 所属保护域
    @param [in] source 尚未解引用的源提示
    */
    LockFreeMapGuard(LockFreeMapReclamation& domain, void* source);

    /** 先清空全部保护，再归还记录
    */
    ~LockFreeMapGuard();

private:
    /** 禁止复制操作守卫
    @param [in] other 源对象
    */
    LockFreeMapGuard(const LockFreeMapGuard& other);

    /** 禁止复制操作守卫
    @param [in] other 源对象
    @return 当前守卫引用，函数禁用
    */
    LockFreeMapGuard& operator=(const LockFreeMapGuard& other);
};

#include "LockFreeMapReclamation.inl"