#include "CTaskBucketCache.h"

CTaskBucketCache::CTaskBucketCache() :
m_state(0)
{

}

CTaskPriorityBucket* CTaskBucketCache::peek() const
{
    return m_state.load(std::memory_order_acquire) == 2 ? m_bucket.get() : nullptr;
}

bool CTaskBucketCache::get(std::shared_ptr<CTaskPriorityBucket>& bucket) const
{
    if (m_state.load(std::memory_order_acquire) != 2)
    {
        return false;
    }
    bucket = m_bucket;
    return true;
}

bool CTaskBucketCache::publish(const std::shared_ptr<CTaskPriorityBucket>& bucket)
{
    int32_t expected = 0;
    if (m_state.compare_exchange_strong(expected, 1, std::memory_order_acq_rel, std::memory_order_acquire))
    {
        m_bucket = bucket;
        m_state.store(2, std::memory_order_release);
        return true;
    }
    return expected == 2 && m_bucket == bucket;
}

void CTaskBucketCache::reset()
{
    m_state.store(0, std::memory_order_relaxed);
    m_bucket.reset();
}