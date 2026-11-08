#pragma once

inline uint64_t CTaskReadDomain::current() const
{
    return m_epoch.load(std::memory_order_acquire);
}