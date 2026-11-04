#pragma once

inline LockFreeHashMapDirectGuard::LockFreeHashMapDirectGuard(LockFreeHashMapHazards* records, size_t count,
    std::atomic<size_t>& span, void* value) :
m_record(LockFreeHashMapHazards::acquire(records, count, span, reinterpret_cast<uintptr_t>(this), reinterpret_cast<uintptr_t>(value)))
{

}

inline LockFreeHashMapDirectGuard::~LockFreeHashMapDirectGuard()
{
    if (m_record != nullptr)
    {
        m_record->m_used.store(0, std::memory_order_release);
    }
}