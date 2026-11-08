#pragma once

inline void CTaskRunningIndex::publish(CTaskStorageSlot& record, int32_t level)
{
    if (level >= 1 && level <= 16)
    {
        const size_t index = static_cast<size_t>(level - 1);
        m_slots[index].m_record.store(&record, std::memory_order_release);
    }
}

inline CTaskStorageSlot* CTaskRunningIndex::below(int32_t level, int32_t minimum) const
{
    if (level <= 1 || minimum > 16)
    {
        return nullptr;
    }
    const int32_t begin = level > 17 ? 16 : level - 1;
    const int32_t end = minimum < 1 ? 1 : minimum;
    for (int32_t current = begin; current >= end; --current)
    {
        CTaskStorageSlot* record = m_slots[current - 1].m_record.load(std::memory_order_acquire);
        if (record != nullptr && (record->m_state->m_bits.load(std::memory_order_acquire) & 3) != 3)
        {
            return record;
        }
    }
    return nullptr;
}

inline void CTaskRunningIndex::clear()
{
    // 固定十六项统一摘除，省去每次任务发布时维护非空掩码。
    // 此方法只在回收或等待前调用，不增加逐任务的共享指针清零。
    for (size_t i = 0; i < 16; ++i)
    {
        m_slots[i].m_record.store(nullptr, std::memory_order_release);
    }
}