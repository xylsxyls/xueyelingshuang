#ifndef LOCK_FREE_QUEUE_ATOMIC_INL
#define LOCK_FREE_QUEUE_ATOMIC_INL

template<class ValueType>
LockFreeQueueAtomic<ValueType>::LockFreeQueueAtomic(ValueType value) :
m_value(LockFreeQueueAtomicStorage<ValueType>{ value })
{
    static_assert(sizeof(LockFreeQueueAtomicStorage<ValueType>) == sizeof(ValueType),
        "Scalar atomic storage must have no padding");
}

template<class ValueType>
ValueType LockFreeQueueAtomic<ValueType>::load(std::memory_order order) const
{
    return m_value.load(order).m_value;
}

template<class ValueType>
void LockFreeQueueAtomic<ValueType>::store(ValueType value, std::memory_order order)
{
    m_value.store(LockFreeQueueAtomicStorage<ValueType>{ value }, order);
}

template<class ValueType>
ValueType LockFreeQueueAtomic<ValueType>::exchange(ValueType value, std::memory_order order)
{
    return m_value.exchange(LockFreeQueueAtomicStorage<ValueType>{ value }, order).m_value;
}

template<class ValueType>
bool LockFreeQueueAtomic<ValueType>::is_lock_free() const
{
    return m_value.is_lock_free();
}

template<class ValueType>
bool LockFreeQueueAtomic<ValueType>::compare_exchange_weak(ValueType& expected, ValueType desired,
    std::memory_order success, std::memory_order failure)
{
    LockFreeQueueAtomicStorage<ValueType> observed = { expected };
    const bool changed = m_value.compare_exchange_weak(observed,
        LockFreeQueueAtomicStorage<ValueType>{ desired }, success, failure);
    expected = observed.m_value;
    return changed;
}

template<class ValueType>
bool LockFreeQueueAtomic<ValueType>::compare_exchange_weak(ValueType& expected, ValueType desired,
    std::memory_order order)
{
    LockFreeQueueAtomicStorage<ValueType> observed = { expected };
    const bool changed = m_value.compare_exchange_weak(observed,
        LockFreeQueueAtomicStorage<ValueType>{ desired }, order);
    expected = observed.m_value;
    return changed;
}

template<class ValueType>
bool LockFreeQueueAtomic<ValueType>::compare_exchange_strong(ValueType& expected, ValueType desired,
    std::memory_order success, std::memory_order failure)
{
    LockFreeQueueAtomicStorage<ValueType> observed = { expected };
    const bool changed = m_value.compare_exchange_strong(observed,
        LockFreeQueueAtomicStorage<ValueType>{ desired }, success, failure);
    expected = observed.m_value;
    return changed;
}

template<class ValueType>
bool LockFreeQueueAtomic<ValueType>::compare_exchange_strong(ValueType& expected, ValueType desired,
    std::memory_order order)
{
    LockFreeQueueAtomicStorage<ValueType> observed = { expected };
    const bool changed = m_value.compare_exchange_strong(observed,
        LockFreeQueueAtomicStorage<ValueType>{ desired }, order);
    expected = observed.m_value;
    return changed;
}

#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER == 1800 && defined(_M_IX86) && _M_IX86_FP >= 2
inline LockFreeQueueAtomic<uint64_t>::LockFreeQueueAtomic(uint64_t value) :
m_value(static_cast<__int64>(value))
{
    static_assert(sizeof(LockFreeQueueAtomic<uint64_t>) == 8, "Atomic quadword size");
    static_assert(__alignof(LockFreeQueueAtomic<uint64_t>) >= 8, "Atomic quadword alignment");
}

inline uint64_t LockFreeQueueAtomic<uint64_t>::load(std::memory_order order) const
{
    (void)order;
    // Microsoft compiler barriers prevent access removal/motion; the single
    // naturally aligned SSE2 read is atomic on the target x86 architecture.
    // All stores/RMWs below are full-barrier locked operations, so this load
    // also supports the standard x86 mapping for sequential consistency.
    _ReadWriteBarrier();
    const __m128i bits = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&m_value));
    uint64_t result;
    _mm_storel_epi64(reinterpret_cast<__m128i*>(&result), bits);
    _ReadWriteBarrier();
    return result;
}

inline void LockFreeQueueAtomic<uint64_t>::store(uint64_t value, std::memory_order order)
{
    (void)order;
    (void)exchange(value, std::memory_order_seq_cst);
}

inline uint64_t LockFreeQueueAtomic<uint64_t>::exchange(uint64_t value, std::memory_order order)
{
    (void)order;
    uint64_t expected = load(std::memory_order_relaxed);
    for (;;)
    {
        const uint64_t old = expected;
        if (compare_exchange_strong(expected, value, std::memory_order_seq_cst,
            std::memory_order_seq_cst))
        {
            return old;
        }
    }
}

inline bool LockFreeQueueAtomic<uint64_t>::compare_exchange_strong(uint64_t& expected,
    uint64_t desired, std::memory_order success, std::memory_order failure)
{
    (void)success;
    (void)failure;
    const uint64_t observed = static_cast<uint64_t>(_InterlockedCompareExchange64(&m_value,
        static_cast<__int64>(desired), static_cast<__int64>(expected)));
    const bool changed = observed == expected;
    expected = observed;
    return changed;
}

inline bool LockFreeQueueAtomic<uint64_t>::compare_exchange_strong(uint64_t& expected,
    uint64_t desired, std::memory_order order)
{
    return compare_exchange_strong(expected, desired, order, order);
}

inline bool LockFreeQueueAtomic<uint64_t>::compare_exchange_weak(uint64_t& expected,
    uint64_t desired, std::memory_order success, std::memory_order failure)
{
    return compare_exchange_strong(expected, desired, success, failure);
}

inline bool LockFreeQueueAtomic<uint64_t>::compare_exchange_weak(uint64_t& expected,
    uint64_t desired, std::memory_order order)
{
    return compare_exchange_strong(expected, desired, order, order);
}

inline bool LockFreeQueueAtomic<uint64_t>::is_lock_free() const
{
    return reinterpret_cast<uintptr_t>(&m_value) % 8 == 0;
}
#endif

#endif