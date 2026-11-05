#ifndef LOCK_FREE_MAP_RECLAMATION_INL
#define LOCK_FREE_MAP_RECLAMATION_INL

inline LockFreeMapHazardPointer::LockFreeMapHazardPointer() :
m_value(nullptr)
#ifdef __EMSCRIPTEN__
, m_claimRoot(false)
#endif
{

}

inline void LockFreeMapHazardPointer::store(void* value, std::memory_order order)
{
#ifdef __EMSCRIPTEN__
    if (value == nullptr && m_claimRoot)
    {
        // 标记仅表示已领取但没有源对象；所有实际索引和块地址均满足自然对齐
        value = reinterpret_cast<void*>(static_cast<uintptr_t>(1));
    }
#endif
    m_value.store(value, order);
}

inline void* LockFreeMapHazardPointer::load(std::memory_order order) const
{
#ifdef __EMSCRIPTEN__
    void* value = m_value.load(order);
    return value == reinterpret_cast<void*>(static_cast<uintptr_t>(1)) ? nullptr : value;
#else
    return m_value.load(order);
#endif
}

inline LockFreeMapHazardRecord::LockFreeMapHazardRecord() :
m_claimed(0),
m_threadHint(0),
m_searchHint(0),
m_searchLocality(false),
m_versionNext(0),
m_versionLast(0),
m_maintenanceTick(0),
m_valueRetries(0),
m_indexCache(nullptr),
m_poolHome(0)
{
#ifdef __EMSCRIPTEN__
    m_pointer[0].m_claimRoot = true;
#endif
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
#ifdef __EMSCRIPTEN__
    return m_pointer[0].m_value.load(order) != nullptr;
#else
    return m_claimed.load(order) != 0;
#endif
}

inline bool LockFreeMapHazardRecord::claim(std::memory_order success, std::memory_order failure, void* source)
{
#ifdef __EMSCRIPTEN__
    void* expected = nullptr;
    return m_pointer[0].m_value.compare_exchange_strong(expected,
        source == nullptr ? reinterpret_cast<void*>(static_cast<uintptr_t>(1)) : source, success, failure);
#else
    (void)source;
    uint32_t expected = 0;
    return m_claimed.compare_exchange_strong(expected, 1, success, failure);
#endif
}

inline void LockFreeMapHazardRecord::release()
{
#ifdef __EMSCRIPTEN__
    // 根保护最后清空，同一次 release 同时开放记录复用，避免先开放私有字段
    m_pointer[0].m_value.store(nullptr, std::memory_order_release);
#else
    m_claimed.store(0, std::memory_order_release);
#endif
}

inline LockFreeMapReclamation::LockFreeMapReclamation(size_t count) :
m_records(new (std::nothrow) LockFreeMapHazardRecord[count]),
m_count(m_records == nullptr ? 0 : count),
m_span(0)
{
    for (size_t index = 0; index < m_count; ++index)
    {
        m_records[index].m_poolHome = index * 4;
    }
}

inline LockFreeMapReclamation::~LockFreeMapReclamation()
{
    delete[] m_records;
}

inline size_t LockFreeMapReclamation::threadHint()
{
#ifdef _WIN32
    return static_cast<size_t>(GetCurrentThreadId());
#else
    const size_t value = std::hash<std::thread::id>()(std::this_thread::get_id());
    return value == 0 ? 1 : value;
#endif
}

inline LockFreeMapHazardRecord* LockFreeMapReclamation::acquire()
{
#ifdef __EMSCRIPTEN__
    return acquire(nullptr);
#else
    const size_t registered = m_span.load();
    if (registered == 1)
    {
        // 记录在源指针读取之前登记，此快捷路径仍然执行 CAS
        // 因此不依赖实例只有单线程使用的假设
        // 记录被暂停线程或竞争者占用时，继续尝试独立记录
        // 其他记录登记后回到普通亲和查找路径
        uint32_t expected = 0;
        if (m_records[0].m_claimed.load(std::memory_order_relaxed) == 0 &&
            m_records[0].m_claimed.compare_exchange_strong(expected, 1))
        {
            return &m_records[0];
        }
    }
    const size_t hint = threadHint();
    for (size_t index = 0; index < registered; ++index)
    {
        if (m_records[index].m_threadHint.load(std::memory_order_relaxed) == hint)
        {
            uint32_t expected = 0;
            if (m_records[index].m_claimed.load(std::memory_order_relaxed) == 0 &&
                m_records[index].m_claimed.compare_exchange_strong(expected, 1))
            {
                return &m_records[index];
            }
        }
    }
    for (size_t index = 0; index < m_count; ++index)
    {
        uint32_t expected = 0;
        if (m_records[index].m_claimed.load(std::memory_order_relaxed) == 0 &&
            m_records[index].m_claimed.compare_exchange_strong(expected, 1))
        {
            m_records[index].m_threadHint.store(hint, std::memory_order_relaxed);
            size_t span = m_span.load();
            while (span <= index && !m_span.compare_exchange_weak(span, index + 1))
            {

            }
            return &m_records[index];
        }
    }
    return nullptr;
#endif
}

inline LockFreeMapHazardRecord* LockFreeMapReclamation::acquire(void* source)
{
#ifdef __EMSCRIPTEN__
    const size_t registered = m_span.load();
    if (registered == 1)
    {
        // 记录在源指针验证之前登记，此快捷路径仍然执行 CAS
        // 因此不依赖实例只有单线程使用的假设
        // 记录被暂停线程或竞争者占用时，继续尝试独立记录
        // 其他记录登记后回到普通亲和查找路径
        if (!m_records[0].claimed(std::memory_order_relaxed) &&
            m_records[0].claim(std::memory_order_seq_cst, std::memory_order_seq_cst, source))
        {
            return &m_records[0];
        }
    }
    const size_t hint = threadHint();
    for (size_t index = 0; index < registered; ++index)
    {
        if (m_records[index].m_threadHint.load(std::memory_order_relaxed) == hint)
        {
            if (!m_records[index].claimed(std::memory_order_relaxed) &&
                m_records[index].claim(std::memory_order_seq_cst, std::memory_order_seq_cst, source))
            {
                return &m_records[index];
            }
        }
    }
    for (size_t index = 0; index < m_count; ++index)
    {
        if (!m_records[index].claimed(std::memory_order_relaxed) &&
            m_records[index].claim(std::memory_order_seq_cst, std::memory_order_seq_cst, source))
        {
            m_records[index].m_threadHint.store(hint, std::memory_order_relaxed);
            size_t span = m_span.load();
            while (span <= index && !m_span.compare_exchange_weak(span, index + 1))
            {

            }
            return &m_records[index];
        }
    }
    return nullptr;
#else
    (void)source;
    return acquire();
#endif
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
        for (size_t slot = 0; slot < 4; ++slot)
        {
            if (m_records[index].m_pointer[slot].load() == pointer)
            {
                return true;
            }
        }
    }
    return false;
}

inline bool LockFreeMapReclamation::protectsValue(void* pointer) const
{
    const size_t span = m_span.load();
    for (size_t index = 0; index < span; ++index)
    {
        if (m_records[index].m_pointer[1].load() == pointer)
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
#ifdef __EMSCRIPTEN__
        result += m_records[index].claimed() ? 1 : 0;
#else
        result += m_records[index].m_claimed.load() != 0 ? 1 : 0;
#endif
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
    for (size_t index = 0; index < m_count; ++index)
    {
        m_records[index].m_indexCache = nullptr;
    }
}

inline void* LockFreeMapReclamation::takeIdleCache(void* pool, size_t channel)
{
#ifdef __EMSCRIPTEN__
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
#else
    const size_t span = m_span.load();
    for (size_t index = 0; index < span; ++index)
    {
        LockFreeMapHazardRecord& record = m_records[index];
        uint32_t expected = 0;
        if (record.m_claimed.load(std::memory_order_relaxed) != 0 ||
            !record.m_claimed.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_relaxed))
        {
            continue;
        }
        void* slot = record.m_poolOwner[channel] == pool ? record.m_poolCache[channel] : nullptr;
        if (slot != nullptr)
        {
            record.m_poolCache[channel] = nullptr;
        }
        record.m_claimed.store(0, std::memory_order_release);
        if (slot != nullptr)
        {
            return slot;
        }
    }
    return nullptr;
#endif
}

inline void LockFreeMapReclamation::clearPoolCaches(void* pool)
{
    for (size_t index = 0; index < m_count; ++index)
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

inline LockFreeMapGuard::~LockFreeMapGuard()
{
    if (m_record != nullptr)
    {
#ifdef __EMSCRIPTEN__
        // 源保护同时承载记录所有权，必须在其他保护清空之后一并释放
        for (size_t index = 1; index < 4; ++index)
#else
        for (size_t index = 0; index < 4; ++index)
#endif
        {
            if (m_record->m_pointer[index].load(std::memory_order_relaxed) != nullptr)
            {
                // 发布需要顺序一致的写入再验证协议，清空只需 release
                // 扫描器读到清空时取得此前已完成的有效载荷访问
                // 跳过空指针写入，避免并发扫描时无谓的缓存行失效
                m_record->m_pointer[index].store(nullptr, std::memory_order_release);
            }
        }
#ifdef __EMSCRIPTEN__
        m_record->release();
#else
        m_record->m_claimed.store(0, std::memory_order_release);
#endif
    }
}

#endif