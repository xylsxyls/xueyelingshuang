#pragma once

inline LockFreeHashMapLink::LockFreeHashMapLink(uintptr_t value) :
m_value(LockFreeHashMapLinkWord{value})
{
    static_assert(sizeof(LockFreeHashMapLinkWord) == sizeof(uintptr_t), "Atomic link must have no padding bits");
}

inline bool LockFreeHashMapLink::isLockFree() const
{
    return m_value.is_lock_free();
}

inline uintptr_t LockFreeHashMapLink::load(std::memory_order order) const
{
    return m_value.load(order).m_bits;
}

inline void LockFreeHashMapLink::store(uintptr_t value, std::memory_order order)
{
    m_value.store(LockFreeHashMapLinkWord{value}, order);
}

LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMapLink::compareExchangeStrong(uintptr_t& expected, uintptr_t desired)
{
    LockFreeHashMapLinkWord observed = {expected};
    const bool exchanged = m_value.compare_exchange_strong(observed, LockFreeHashMapLinkWord{desired},
        std::memory_order_seq_cst, std::memory_order_seq_cst);
    expected = observed.m_bits;
    return exchanged;
}