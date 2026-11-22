#include "LibuvPoolSettings.h"

LibuvPoolSettings::LibuvPoolSettings() :
m_sendQueue(true)
{
    m_sendQueue.initialCapacity = 128;
    m_sendQueue.maximumCapacity = 8192;
    m_connections.initialCapacity = 128;
    m_connections.maximumCapacity = 65536;
    m_connections.directoryCapacity = 64;
    m_connections.lookupCacheCapacity = 1024;
    m_closeQueue.initialCapacity = 32;
    m_closeQueue.maximumCapacity = 256;
}

bool LibuvPoolSettings::valid() const
{
    return m_send.valid() && m_read.valid() && m_send.m_producerCacheCapacity < m_sendQueue.maximumProducers;
}