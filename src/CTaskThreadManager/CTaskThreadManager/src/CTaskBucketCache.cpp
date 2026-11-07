#include "CTaskBucketCache.h"

CTaskBucketCache::CTaskBucketCache() :
m_raw(nullptr),
m_state(0)
{

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
        // 先让消费者能够看到桶，再允许 get 通过；初始化者暂停不能隐藏后续投递。
        m_raw.store(bucket.get(), std::memory_order_release);
        m_state.store(2, std::memory_order_release);
        return true;
    }
    return expected == 2 && m_bucket == bucket;
}

void CTaskBucketCache::reset()
{
    m_raw.store(nullptr, std::memory_order_relaxed);
    m_state.store(0, std::memory_order_relaxed);
    m_bucket.reset();
}