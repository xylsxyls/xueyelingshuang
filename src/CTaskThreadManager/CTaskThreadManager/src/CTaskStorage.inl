#pragma once

inline void CTaskStorage::retire(CTaskStorageSlot& slot)
{
    if (!m_retiredOverflow && m_retiredCount < 1024)
    {
        const size_t position = (m_retiredRead + m_retiredCount) & 1023;
        m_retiredWindow[position].m_slot = &slot;
        m_retiredWindow[position].m_epoch = m_domain->current();
        ++m_retiredCount;
        return;
    }
    m_retiredOverflow = true;
    slot.m_cookie = m_domain->current();
    slot.m_value = nullptr;
    if (m_retiredTail != nullptr)
    {
        m_retiredTail->m_value = &slot;
    }
    else
    {
        m_retiredHead = &slot;
    }
    m_retiredTail = &slot;
}