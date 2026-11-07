#include "CTaskAdmission.h"

static const uint64_t kTaskAdmissionClosed = UINT64_C(1) << 63;

CTaskAdmission::CTaskAdmission() :
m_state(0)
{

}

bool CTaskAdmission::enter()
{
    uint64_t state = m_state.load(std::memory_order_acquire);
    while (state < kTaskAdmissionClosed - 1)
    {
        if (m_state.compare_exchange_weak(state, state + 1, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            return true;
        }
    }
    return false;
}

void CTaskAdmission::leave()
{
    m_state.fetch_sub(1, std::memory_order_release);
}

void CTaskAdmission::close()
{
    m_state.fetch_or(kTaskAdmissionClosed, std::memory_order_acq_rel);
}

void CTaskAdmission::reopen()
{
    m_state.store(0, std::memory_order_release);
}

bool CTaskAdmission::closeIfEmpty()
{
    uint64_t expected = 0;
    return m_state.compare_exchange_strong(expected, kTaskAdmissionClosed, std::memory_order_acq_rel, std::memory_order_acquire);
}

uint64_t CTaskAdmission::active() const
{
    return m_state.load(std::memory_order_acquire) & (kTaskAdmissionClosed - 1);
}

bool CTaskAdmission::supported() const
{
    return m_state.is_lock_free();
}