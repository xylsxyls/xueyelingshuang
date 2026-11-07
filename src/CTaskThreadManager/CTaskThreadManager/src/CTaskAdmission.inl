#pragma once

inline bool CTaskAdmission::closed() const
{
    return (m_state.load(std::memory_order_acquire) & (UINT64_C(1) << 63)) != 0;
}