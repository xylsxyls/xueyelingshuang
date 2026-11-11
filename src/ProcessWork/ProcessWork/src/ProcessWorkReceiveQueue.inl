#pragma once
#include <cstring>

inline bool ProcessWorkReceiveQueue::take(char** packet, uint32_t milliseconds)
{
    if (packet == nullptr)
    {
        return false;
    }
    if (m_active == nullptr)
    {
        if (!m_ready.pop(&m_active))
        {
            m_notified.store(false, std::memory_order_seq_cst);
            if (!m_ready.pop(&m_active))
            {
                m_notification.waitResult(milliseconds);
                if (!m_ready.pop(&m_active))
                {
                    return false;
                }
            }
        }
        m_readOffset = 0;
    }
    *packet = m_active->m_data + m_readOffset + sizeof(uint32_t);
    return true;
}

inline void ProcessWorkReceiveQueue::release(char* packet)
{
    if (m_active == nullptr || packet != m_active->m_data + m_readOffset + sizeof(uint32_t))
    {
        return;
    }
    uint32_t bytes = 0;
    std::memcpy(&bytes, m_active->m_data + m_readOffset, sizeof(bytes));
    m_readOffset += sizeof(bytes) + bytes;
    m_completed += m_config.m_localQueueBytes == -1 ? 1 : static_cast<uint64_t>(bytes) + 264;
    m_released.store(m_completed, std::memory_order_release);
    if (m_readOffset == m_active->m_size)
    {
        ProcessWorkReceiveBatch* completed = m_active;
        m_active = nullptr;
        recycle(completed);
    }
}