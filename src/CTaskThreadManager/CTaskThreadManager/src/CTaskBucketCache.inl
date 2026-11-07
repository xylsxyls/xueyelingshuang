#pragma once

inline CTaskPriorityBucket* CTaskBucketCache::peek() const
{
    return m_raw.load(std::memory_order_acquire);
}