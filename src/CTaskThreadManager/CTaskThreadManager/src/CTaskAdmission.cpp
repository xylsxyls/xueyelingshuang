#include "CTaskAdmission.h"

static const uint64_t kTaskAdmissionClosed = UINT64_C(1) << 63;

CTaskAdmission::CTaskAdmission() :
m_state(0)
{

}

bool CTaskAdmission::enter()
{
    if (closed())
    {
        return false;
    }
    // 关闭位与登记数仍在同一个原子字中。原子加法不会因其他调用更新计数而重试。
    // 登记晚于 close 时立即归还；关闭前暂停的调用也不能越过关闭位进入。
    // 低 63 位只计算同时在途调用，远大于本进程能建立的线程及调用栈数量。
    const uint64_t previous = m_state.fetch_add(1, std::memory_order_acq_rel);
    if ((previous & kTaskAdmissionClosed) != 0)
    {
        m_state.fetch_sub(1, std::memory_order_release);
        return false;
    }
    return true;
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
    // 只重开接纳位。关闭前暂停的 enter 仍可能正在撤销登记，不能覆盖其计数。
    m_state.fetch_and(~kTaskAdmissionClosed, std::memory_order_release);
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