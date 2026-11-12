#include "LibuvPoolSettings.h"

LibuvPoolSettings::LibuvPoolSettings()
{
    m_closeQueue.m_initialCapacity = 32;
    m_closeQueue.m_maximumCapacity = 256;
}

bool LibuvPoolSettings::valid() const
{
    return m_sendQueue.valid() && m_closeQueue.valid() && m_connections.valid() &&
        m_send.valid() && m_read.valid() && m_send.m_producerCacheCapacity < m_sendQueue.m_maximumProducers;
}