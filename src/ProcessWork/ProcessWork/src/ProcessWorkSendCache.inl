#pragma once

inline ProcessWorkSendCacheLease::ProcessWorkSendCacheLease() :
m_cache(nullptr), m_record(nullptr)
{

}

inline ProcessWorkSendCacheLease::~ProcessWorkSendCacheLease()
{
    reset();
}

inline void ProcessWorkSendCacheLease::reset()
{
    if (m_record != nullptr)
    {
        m_record->m_pointer.store(nullptr, std::memory_order_release);
        m_cache->collect();
        m_record = nullptr;
        m_cache = nullptr;
    }
}

inline void ProcessWorkSendCache::collect()
{
    if (m_retired.load(std::memory_order_acquire) != nullptr)
    {
        collectRetired();
    }
}

inline uint64_t ProcessWorkSendCache::epoch() const
{
    return m_identity == 0 ? 0 : m_epoch.load(std::memory_order_acquire);
}