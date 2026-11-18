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
    // 发布前固定：是否兼作领取状态；原始零地址表示空闲，标记一表示已领取但无源
    bool m_claimRoot;
    // 与原布局保留相同的隔离空间
    unsigned char m_padding[63];

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
    @param [in] order 合法的原子加载内存序，默认seq_cst；不可使用release或acq_rel
    @return 已发布的受保护地址
    */
    void* load(std::memory_order order = std::memory_order_seq_cst) const;
};

/** 每实例固定保护记录；亲和提示辅助查找，活动拥有者用于保守识别重入
*/
struct LockFreeMapHazardRecord
{
public:
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    // 查找亲和记录时由其他线程读取，领取后通常不再修改
    std::atomic<size_t> m_threadHint;
    // 隔开只读提示与拥有者每次调用写入的领取、根保护字段，减少扫描引起的缓存行争用
    unsigned char m_hintPadding[64];
    // 当前调用的线程标识，领取后发布、开放复用前清零；与可能过期的亲和提示分离
    std::atomic<size_t> m_activeOwner;
#else
    // 保留原有隔离布局，不再维护独立领取状态
    unsigned char m_claimPadding[sizeof(std::atomic<uint32_t>)];
    // 线程亲和提示，碰撞或线程 ID 复用都不授予所有权
    std::atomic<size_t> m_threadHint;
#endif
    // 顺序一致保护：源根、值版本、分配块；私有候选由状态和源保护覆盖
    LockFreeMapHazardPointer m_pointer[3];
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
    // 读取竞争失败累计次数，仅记录独占者访问，每次让步后归零
    uint32_t m_contentionFailures;
    // 写入及条件提取竞争的累计失败次数，查询或提交成功均不重置
    uint32_t m_writeFailures;
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    // 整数顺序提示的逻辑数，零表示尚未建立
    size_t m_rankCount;
    // 视图首键的无符号表示，仅用于生成待验证坐标
    uint64_t m_rankOrigin;
    // 等距键步长，零表示该规模下已停用预测
    uint64_t m_rankStep;
    // 最近已命中的无条件覆盖位置，只保存坐标，每次使用重新验证键
    size_t m_writeHint;
    // 预测步长范围为负十六至十六；零表示尚未建立提示
    int32_t m_writeStep;
    // 完整保留末尾隔离，避免位置提示的频繁写入与下一记录的只读亲和提示争用缓存行。
    unsigned char m_padding[64];
#else
    // 利用原有六十四字节隔离空间，保持池缓存和下一条记录的原有位置
    unsigned char m_padding[64 - 2 * sizeof(uint32_t)];
#endif

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
    @param [in] source 同步发布的初始源提示，可为nullptr；取得记录后必须验证实际源
    @return 是否取得独占权
    */
    bool claim(std::memory_order success = std::memory_order_seq_cst,
        std::memory_order failure = std::memory_order_seq_cst, void* source = nullptr);

    /** 清空源保护并归还记录；调用者须已清空其他保护且结束私有访问
    */
    void release();

    /** 当前独占调用合并值竞争统计，达到上限后保持饱和
    @param [in] amount 尚未公布的失败次数；调用者须独占本记录
    */
    void addValueRetries(uint32_t amount);
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

    /** 取得记录时同时发布初始源提示，所有后端均在登记后验证该源
    @param [in] source 初始源提示，只可在完成登记和源验证后解引用
    @return 取得的记录，全部占用时返回 nullptr
    */
    LockFreeMapHazardRecord* acquire(void* source);

    /** 选定可用记录后读取当前源，并在领取CAS中发布
    @param [in] source 调用期间有效的原子来源，返回后仍须重新验证
    @return 取得的记录，全部占用时返回nullptr
    */
    template <typename T>
    LockFreeMapHazardRecord* acquire(std::atomic<T*>& source);

    /** 扫描全部已登记保护指针
    @param [in] pointer 已摘除对象的地址
    @param [in] ignored 已确认不再访问计划字段的调用记录，无则为 nullptr
    @return 仍有调用可能借用该地址时返回 true
    */
    bool protects(void* pointer, const LockFreeMapHazardRecord* ignored = nullptr) const;

    /** 扫描索引根保护；索引地址只发布在根槽，不需要访问值和块的缓存行
    @param [in] pointer 已摘除的非空索引地址，不接受领取标记
    @param [in] ignored 已确认结束源借用的记录，无则为 nullptr
    @return 仍有调用发布该根地址时返回 true
    */
    bool protectsRoot(void* pointer, const LockFreeMapHazardRecord* ignored = nullptr) const;

    /** 扫描已关闭分配块的保护；分配块只发布在块槽
    @param [in] pointer 已从目录摘除的非空分配块地址
    @return 仍有调用发布该块地址时返回 true
    */
    bool protectsBlock(void* pointer) const;

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

    /** 从实例共享记录的活动拥有者识别当前线程外层借用，兼容跨模块重入
    @return 活动拥有者标识相同则返回true；线程标识哈希碰撞也只会拒绝容量等待
    @note 活动拥有者不授予记录所有权，不访问其他线程的非原子字段
    */
    bool mayBeHeldByCurrentThread() const;

    /** 多次凭据失效时请求有限次提交后退避，不保留请求者身份或资源所有权
    */
    void requestConditional();

    /** 条件提交且守卫已释放后响应竞争提示；没有请求时直接返回
    @note 不等候请求者或计数归零，单次响应的本地工作量有固定上限
    */
    void finishConditional();

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
    /** 共享记录选择流程，按实例化参数决定领取前是否重新加载源
    @param [in] source 固定源提示，仅ReadSource为false时使用
    @param [in] address 原子来源，仅ReadSource为true时必须非空且有效
    @return 已取得的独占记录，资源不足时返回nullptr
    */
    template <typename T, bool ReadSource>
    LockFreeMapHazardRecord* acquireSource(void* source, std::atomic<T*>* address);

    /** 取得不授予所有权的线程亲和提示，无需线程局部登记
    @return 提示允许复用或碰撞，只有记录 CAS 才授予独占权
    */
    static size_t threadHint();

    /** 尝试消耗一次退避提示并作有界让步，调用方已结束容器借用
    @param [in] pending 先前读到的非零提示计数；只影响调度，不授予访问权限
    */
#if defined(_MSC_VER)
    __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
    __attribute__((noinline))
#endif
    void pauseConditional(uint32_t pending);

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
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    // 有限的条件提交退避提示；没有线程拥有它，暂停请求者不会阻挡其他调用
    std::atomic<uint32_t> m_conditionalPressure;
#endif
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
    @param [in,out] domain 借用的所属保护域，必须覆盖守卫生命期
    */
    explicit LockFreeMapGuard(LockFreeMapReclamation& domain);

    /** 取得记录并预先发布源提示，调用方仍须核对源指针后才能访问
    @param [in,out] domain 所属保护域
    @param [in] source 尚未解引用的源提示
    */
    LockFreeMapGuard(LockFreeMapReclamation& domain, void* source);

    /** 读取原子来源并领取记录，返回后由调用方完成来源验证
    @param [in,out] domain 所属保护域
    @param [in] source 调用期间有效的原子来源
    */
    template <typename T>
    LockFreeMapGuard(LockFreeMapReclamation& domain, std::atomic<T*>& source);

    /** 先清空全部保护，再归还记录
    */
    ~LockFreeMapGuard();

    /** 累计竞争失败后分摊运行库调度提示，不影响无竞争的成功路径
    @param [in] writing 是否为写入或提取的提交竞争，源保护重试使用默认读取计数
    */
#if defined(_MSC_VER)
    __declspec(noinline)
#elif !defined(__EMSCRIPTEN__) && (defined(__GNUC__) || defined(__clang__))
    __attribute__((noinline))
#endif
    void conflict(bool writing = false);

    /** 根索引发布竞争时分散重试节奏，不等待其他调用；须持有有效记录
    */
#if defined(_MSC_VER)
    __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
    __attribute__((noinline))
#endif
    void structuralConflict();

    /** 查询成功且全部借用访问已结束时，归还保护记录
    @note 仅供未使用块保护的读取完成分支调用；返回后不得再访问借用对象或记录
    */
    void finishRead();

    /** 写入已成功且值、块保护均已清空时，释放根保护和记录
    @note 仅供 write 的提交完成分支使用；异常、冲突和提前返回仍由析构完整清理
    */
    void finishWrite();

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