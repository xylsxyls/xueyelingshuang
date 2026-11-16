#ifndef LOCK_FREE_MAP_RECLAMATION_INL
#define LOCK_FREE_MAP_RECLAMATION_INL

inline LockFreeMapHazardPointer::LockFreeMapHazardPointer() :
m_value(nullptr),
m_claimRoot(false)
{

}

inline void LockFreeMapHazardPointer::store(void* value, std::memory_order order)
{
    if (value == nullptr && m_claimRoot)
    {
        // 标记仅表示已领取但没有源对象；所有实际索引和块地址均满足自然对齐
        value = reinterpret_cast<void*>(static_cast<uintptr_t>(1));
    }
    m_value.store(value, order);
}

inline void* LockFreeMapHazardPointer::load(std::memory_order order) const
{
    void* value = m_value.load(order);
    return value == reinterpret_cast<void*>(static_cast<uintptr_t>(1)) ? nullptr : value;
}

inline LockFreeMapHazardRecord::LockFreeMapHazardRecord() :
m_threadHint(0),
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
m_activeOwner(0),
#endif
m_searchHint(0),
m_searchLocality(false),
m_versionNext(0),
m_versionLast(0),
m_maintenanceTick(0),
m_valueRetries(0),
m_indexCache(nullptr),
m_poolHome(0),
m_contentionFailures(0),
m_writeFailures(0)
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
, m_rankCount(0), m_rankOrigin(0), m_rankStep(0), m_writeHint(0), m_writeStep(0)
#endif
{
    m_pointer[0].m_claimRoot = true;
    for (size_t index = 0; index < 2; ++index)
    {
        m_directoryHint[index] = 0;
        m_slotHint[index] = 0;
        m_poolCache[index] = nullptr;
        m_poolOwner[index] = nullptr;
    }
}

inline bool LockFreeMapHazardRecord::claimed(std::memory_order order) const
{
    return m_pointer[0].m_value.load(order) != nullptr;
}

inline bool LockFreeMapHazardRecord::claim(std::memory_order success, std::memory_order failure, void* source)
{
    void* expected = nullptr;
    return m_pointer[0].m_value.compare_exchange_strong(expected,
        source == nullptr ? reinterpret_cast<void*>(static_cast<uintptr_t>(1)) : source, success, failure);
}

inline void LockFreeMapHazardRecord::release()
{
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    // 先结束当前归属，再开放复用；新领取者发布归属前只能观察到零，不能继承旧线程。
    m_activeOwner.store(0, std::memory_order_release);
#endif
    // 根保护最后清空，同一次 release 同时开放记录复用，避免先开放私有字段
    m_pointer[0].m_value.store(nullptr, std::memory_order_release);
}

inline void LockFreeMapHazardRecord::addValueRetries(uint32_t amount)
{
    const uint64_t maximum = (std::numeric_limits<uint64_t>::max)();
    const uint64_t current = m_valueRetries.load(std::memory_order_relaxed);
    m_valueRetries.store(amount > maximum - current ? maximum : current + amount, std::memory_order_relaxed);
}

inline LockFreeMapReclamation::LockFreeMapReclamation(size_t count) :
m_records(new (std::nothrow) LockFreeMapHazardRecord[count]),
m_count(m_records == nullptr ? 0 : count),
m_span(0)
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
, m_conditionalPressure(0)
#endif
{
    for (size_t index = 0; index < m_count; ++index)
    {
        m_records[index].m_poolHome = index * 4;
    }
}

inline void LockFreeMapReclamation::requestConditional()
{
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    // 覆盖为固定预算，不随竞争次数累加；请求者暂停后预算仍能自行耗尽
    m_conditionalPressure.store(16, std::memory_order_relaxed);
#endif
}

inline void LockFreeMapReclamation::finishConditional()
{
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    const uint32_t pending = m_conditionalPressure.load(std::memory_order_relaxed);
    if (pending != 0)
    {
        pauseConditional(pending);
    }
#endif
}

inline void LockFreeMapReclamation::pauseConditional(uint32_t pending)
{
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    // 只尝试一次减额，竞争失败也不等待；并发提示不参与对象发布或回收协议
    const uint32_t observed = pending;
    m_conditionalPressure.compare_exchange_strong(pending, observed - 1, std::memory_order_relaxed);
    // 给较慢的观察及条件提交者留下机会；volatile 只保留有限的本地迭代
#ifdef __EMSCRIPTEN__
    // 线性内存中的易失计数循环采用较小预算，仍给同类条件调用留下有限提交间隔
    volatile uint32_t remaining = 1024 + (observed & 15) * 32;
#else
    volatile uint32_t remaining = 4096 + (observed & 15) * 128;
#endif
    while (remaining != 0)
    {
        --remaining;
    }
#ifndef __EMSCRIPTEN__
    // Wasm 的调度提示不让出宿主线程；有限本地退避已完成，不再重复进入运行库
    std::this_thread::yield();
#endif
#else
    (void)pending;
#endif
}

inline LockFreeMapReclamation::~LockFreeMapReclamation()
{
    delete[] m_records;
}

inline size_t LockFreeMapReclamation::threadHint()
{
#ifdef _WIN32
#if defined(_MSC_VER) && !defined(_M_CEE_PURE) && (defined(_M_IX86) || defined(_M_X64))
    // 使用SDK提供的当前线程环境地址作为不透明身份，不读取或硬编码其内部字段。
    // 同一线程跨DLL取得相同地址；记录归还前清零活动身份，地址复用不授予旧所有权。
    return reinterpret_cast<size_t>(NtCurrentTeb());
#else
    return static_cast<size_t>(GetCurrentThreadId());
#endif
#else
    const size_t value = std::hash<std::thread::id>()(std::this_thread::get_id());
    return value == 0 ? 1 : value;
#endif
}

inline bool LockFreeMapReclamation::mayBeHeldByCurrentThread() const
{
    const size_t hint = threadHint();
    const size_t span = m_span.load();
    for (size_t index = 0; index < span; ++index)
    {
        // 亲和提示可跨复用保留，不能作为重入证据；只检查活动调用发布的拥有者。
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
        if (m_records[index].m_activeOwner.load() == hint)
#else
        if (m_records[index].claimed() && m_records[index].m_threadHint.load() == hint)
#endif
        {
            return true;
        }
    }
    return false;
}

inline LockFreeMapHazardRecord* LockFreeMapReclamation::acquire()
{
    return acquire(nullptr);
}

inline LockFreeMapHazardRecord* LockFreeMapReclamation::acquire(void* source)
{
    return acquireSource<void, false>(source, nullptr);
}

template <typename T>
inline LockFreeMapHazardRecord* LockFreeMapReclamation::acquire(std::atomic<T*>& source)
{
    return acquireSource<T, true>(nullptr, &source);
}

template <typename T, bool ReadSource>
#ifdef _MSC_VER
__forceinline
#else
inline
#endif
LockFreeMapHazardRecord* LockFreeMapReclamation::acquireSource(void* source, std::atomic<T*>* address)
{
    // 记录数组只在构造时确定；本次调用保存地址，避免跨运行库调用重复读取成员。
    LockFreeMapHazardRecord* const records = m_records;
    const size_t registered = m_span.load();
    if (registered == 1)
    {
        // 记录在源指针验证之前登记，此快捷路径仍然执行 CAS
        // 因此不依赖实例只有单线程使用的假设
        // 记录被暂停线程或竞争者占用时，继续尝试独立记录
        // 其他记录登记后回到普通亲和查找路径
        if (!records[0].claimed(std::memory_order_relaxed) &&
            records[0].claim(std::memory_order_seq_cst, std::memory_order_seq_cst,
                ReadSource ? address->load() : source))
        {
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
            // 单记录快捷路径不依赖亲和提示；只发布本次实际拥有者，避免重复写只读提示缓存行。
            records[0].m_activeOwner.store(threadHint(), std::memory_order_relaxed);
#endif
            return &records[0];
        }
    }
    const size_t hint = threadHint();
    for (size_t index = 0; index < registered; ++index)
    {
        if (records[index].m_threadHint.load(std::memory_order_relaxed) == hint)
        {
            if (!records[index].claimed(std::memory_order_relaxed) &&
                records[index].claim(std::memory_order_seq_cst, std::memory_order_seq_cst,
                    ReadSource ? address->load() : source))
            {
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
                // 只在并发后端复查拥有者；单线程宿主不能在这两条原子操作间复用记录。
                if (records[index].m_threadHint.load(std::memory_order_relaxed) != hint)
                {
                    records[index].m_threadHint.store(hint, std::memory_order_relaxed);
                }
                records[index].m_activeOwner.store(hint, std::memory_order_relaxed);
#endif
                return &records[index];
            }
        }
    }
    for (size_t index = 0; index < m_count; ++index)
    {
        if (!records[index].claimed(std::memory_order_relaxed) &&
            records[index].claim(std::memory_order_seq_cst, std::memory_order_seq_cst,
                ReadSource ? address->load() : source))
        {
            records[index].m_threadHint.store(hint, std::memory_order_relaxed);
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
            records[index].m_activeOwner.store(hint, std::memory_order_relaxed);
#endif
            size_t span = m_span.load();
            while (span <= index && !m_span.compare_exchange_weak(span, index + 1))
            {

            }
            return &records[index];
        }
    }
    return nullptr;
}

inline bool LockFreeMapReclamation::protects(void* pointer, const LockFreeMapHazardRecord* ignored) const
{
    const size_t span = m_span.load();
    // 登记先于源指针验证，后来扩展的记录不能借用扫描前已摘除对象
    // 始终扫描保护指针，不按 claimed 标志跳过，避免观察间隙的记录复用
    // 发布使用顺序一致内存序，清空以 release 结束此前的借用
    for (size_t index = 0; index < span; ++index)
    {
        if (&m_records[index] == ignored)
        {
            continue;
        }
        for (size_t slot = 0; slot < 3; ++slot)
        {
            if (m_records[index].m_pointer[slot].load() == pointer)
            {
                return true;
            }
        }
    }
    return false;
}

inline bool LockFreeMapReclamation::protectsRoot(void* pointer, const LockFreeMapHazardRecord* ignored) const
{
    const size_t span = m_span.load();
    // 登记和源验证的顺序不变；调用方只传实际索引，领取标记不会与它相等。
    for (size_t index = 0; index < span; ++index)
    {
        if (&m_records[index] != ignored && m_records[index].m_pointer[0].m_value.load() == pointer)
        {
            return true;
        }
    }
    return false;
}

inline bool LockFreeMapReclamation::protectsBlock(void* pointer) const
{
    const size_t span = m_span.load();
    for (size_t index = 0; index < span; ++index)
    {
        if (m_records[index].m_pointer[2].m_value.load() == pointer)
        {
            return true;
        }
    }
    return false;
}

inline bool LockFreeMapReclamation::protectsValue(void* pointer) const
{
    const size_t span = m_span.load();
    for (size_t index = 0; index < span; ++index)
    {
        if (m_records[index].m_pointer[1].m_value.load() == pointer)
        {
            return true;
        }
    }
    return false;
}

inline size_t LockFreeMapReclamation::active() const
{
    size_t result = 0;
    for (size_t index = 0; index < m_count; ++index)
    {
        result += m_records[index].claimed() ? 1 : 0;
    }
    return result;
}

inline size_t LockFreeMapReclamation::bytes() const
{
    return m_count * sizeof(LockFreeMapHazardRecord);
}

inline uint64_t LockFreeMapReclamation::valueRetries() const
{
    uint64_t total = 0;
    const uint64_t maximum = (std::numeric_limits<uint64_t>::max)();
    const size_t span = m_span.load();
    for (size_t index = 0; index < span; ++index)
    {
        const uint64_t value = m_records[index].m_valueRetries.load(std::memory_order_relaxed);
        total = value > maximum - total ? maximum : total + value;
    }
    return total;
}

inline bool LockFreeMapReclamation::multipleRecords() const
{
    return m_span.load() > 1;
}

inline void LockFreeMapReclamation::clearIndexCaches()
{
    // 从未登记的记录仅有构造时的空提示，静止期无需重复清空。
    const size_t span = m_span.load();
    for (size_t index = 0; index < span; ++index)
    {
        m_records[index].m_indexCache = nullptr;
    }
}

inline void* LockFreeMapReclamation::takeIdleCache(void* pool, size_t channel)
{
    const size_t span = m_span.load();
    for (size_t index = 0; index < span; ++index)
    {
        LockFreeMapHazardRecord& record = m_records[index];
        if (record.claimed(std::memory_order_relaxed) ||
            !record.claim(std::memory_order_acquire, std::memory_order_relaxed))
        {
            continue;
        }
        void* slot = record.m_poolOwner[channel] == pool ? record.m_poolCache[channel] : nullptr;
        if (slot != nullptr)
        {
            record.m_poolCache[channel] = nullptr;
        }
        record.release();
        if (slot != nullptr)
        {
            return slot;
        }
    }
    return nullptr;
}

inline void LockFreeMapReclamation::clearPoolCaches(void* pool)
{
    // 从未登记的记录仅有构造时的空提示，静止期无需重复清空。
    const size_t span = m_span.load();
    for (size_t index = 0; index < span; ++index)
    {
        for (size_t channel = 0; channel < 2; ++channel)
        {
            if (m_records[index].m_poolOwner[channel] == pool)
            {
                m_records[index].m_poolOwner[channel] = nullptr;
                m_records[index].m_poolCache[channel] = nullptr;
            }
        }
    }
}

inline LockFreeMapGuard::LockFreeMapGuard(LockFreeMapReclamation& domain) :
m_record(domain.acquire())
{

}

inline LockFreeMapGuard::LockFreeMapGuard(LockFreeMapReclamation& domain, void* source) :
m_record(domain.acquire(source))
{

}

template <typename T>
inline LockFreeMapGuard::LockFreeMapGuard(LockFreeMapReclamation& domain, std::atomic<T*>& source) :
#ifdef __EMSCRIPTEN__
m_record(domain.acquire(source.load()))
#else
m_record(domain.acquire(source))
#endif
{
    // 原生后端缩短首次加载与保护发布的间隔；Wasm保留已验证的入口。
}

inline void LockFreeMapGuard::conflict(bool writing)
{
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    // 失败次数跨调用累计，每三十二次失败让步一次；成功路径不读写退避状态
    uint32_t& count = writing ? m_record->m_writeFailures : m_record->m_contentionFailures;
    const uint32_t failures = ++count;
    if (failures == 32)
    {
        count = 0;
        std::this_thread::yield();
    }
#else
    (void)writing;
#endif
}

inline void LockFreeMapGuard::structuralConflict()
{
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    // 失败清理已完成；每八次累计写竞争作一次有界本地退避，随后仍由 conflict 维护让步计数
    const uint32_t failures = m_record->m_writeFailures;
    if ((failures & 7) == 7)
    {
        const uintptr_t seed = (reinterpret_cast<uintptr_t>(m_record) >> 4) ^ failures;
        // volatile 仅保留当前线程的固定迭代，不作跨线程同步
        volatile uint32_t remaining = 16 + static_cast<uint32_t>(seed & 31);
        while (remaining != 0)
        {
            --remaining;
        }
    }
#endif
}

inline void LockFreeMapGuard::finishRead()
{
    // 查询不会申请池块，块保护为空；键值、凭据输出和用户回调均已结束。
    m_record->m_pointer[1].m_value.store(nullptr, std::memory_order_release);
    m_record->release();
    m_record = nullptr;
}

inline void LockFreeMapGuard::finishWrite()
{
    // 值更新分支或索引 complete 已结束值借用；池 take 成功返回前也已解除块保护
    // 只在这些前提均满足的提交分支使用，避免重复读取两个已为空的保护槽
    m_record->release();
    // 从此不再访问记录，防止析构重复清理已经被下一调用领取的记录
    m_record = nullptr;
}

inline LockFreeMapGuard::~LockFreeMapGuard()
{
    if (m_record != nullptr)
    {
        // 源保护同时承载记录所有权，必须在其他保护清空之后一并释放。
        // 值槽和块槽不承载领取标记，直接读写同一原子地址，保留原有清理内存序。
        for (size_t index = 1; index < 3; ++index)
        {
            if (m_record->m_pointer[index].m_value.load(std::memory_order_relaxed) != nullptr)
            {
                // 发布需要顺序一致的写入再验证协议，清空只需 release
                // 扫描器读到清空时取得此前已完成的有效载荷访问
                // 跳过空指针写入，避免并发扫描时无谓的缓存行失效
                m_record->m_pointer[index].m_value.store(nullptr, std::memory_order_release);
            }
        }
        m_record->release();
    }
}

#endif