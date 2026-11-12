#include "LibuvTcpPoolConfig.h"

LibuvQueuePoolConfig::LibuvQueuePoolConfig() :
m_initialCapacity(128U),
m_maximumCapacity(8192U),
m_growThreshold(0U),
m_maximumProducers(1024U)
{

}

bool LibuvQueuePoolConfig::valid() const
{
    return m_initialCapacity >= 2 &&
        m_maximumCapacity >= m_initialCapacity &&
        m_maximumCapacity <= 0x01000000U &&
        m_maximumProducers != 0 &&
        m_maximumProducers <= 65536U &&
        m_growThreshold <= m_maximumCapacity;
}

LibuvConnectionPoolConfig::LibuvConnectionPoolConfig() :
m_initialCapacity(128U),
m_maximumCapacity(65536U),
m_growFreeThreshold(0U),
m_directoryCapacity(64U),
m_protectionRecordCount(128U),
m_lookupCacheCapacity(1024U),
m_bucketCount(1024U)
{

}

bool LibuvConnectionPoolConfig::valid() const
{
    return m_initialCapacity != 0 &&
        m_maximumCapacity >= m_initialCapacity &&
        m_maximumCapacity <= 0x01000000U &&
        m_growFreeThreshold <= m_maximumCapacity &&
        m_directoryCapacity != 0 &&
        m_directoryCapacity <= 65536U &&
        uint64_t(m_directoryCapacity) * 65535U >= m_initialCapacity &&
        m_protectionRecordCount != 0 &&
        m_protectionRecordCount <= 65536U &&
        m_lookupCacheCapacity <= 0x01000000U &&
        (m_lookupCacheCapacity == 0 || (m_lookupCacheCapacity & (m_lookupCacheCapacity - 1)) == 0) &&
        m_bucketCount != 0 &&
        m_bucketCount <= 0x01000000U &&
        (m_bucketCount & (m_bucketCount - 1)) == 0;
}

LibuvSendPoolConfig::LibuvSendPoolConfig() :
m_maximumPendingBytes(67108864U),
m_laneCapacity(1024U),
m_producerCacheCapacity(16U),
m_maximumActiveWrites(256U),
m_maximumCachedWrites(256U)
{

}

bool LibuvSendPoolConfig::valid() const
{
    return m_maximumPendingBytes != 0 &&
        m_maximumPendingBytes <= 0x7fffffffU &&
        m_laneCapacity <= 1048576U &&
        (m_laneCapacity == 0 || (m_laneCapacity & (m_laneCapacity - 1)) == 0) &&
        m_producerCacheCapacity <= 65536U &&
        m_maximumActiveWrites != 0 &&
        m_maximumActiveWrites <= 65536U &&
        m_maximumCachedWrites <= 65536U;
}

LibuvReadPoolConfig::LibuvReadPoolConfig() :
m_bufferBytes(65536U),
m_maximumCachedBuffers(8U)
{

}

bool LibuvReadPoolConfig::valid() const
{
    return m_bufferBytes != 0 &&
        m_bufferBytes <= 16777216U &&
        m_maximumCachedBuffers <= 65536U &&
        uint64_t(m_bufferBytes) * m_maximumCachedBuffers <= 0x7fffffffU;
}

LibuvChannelWritePoolConfig::LibuvChannelWritePoolConfig() :
m_maximumWriteBytes(32768U),
m_maximumPendingBytesPerPeer(65536U),
m_maximumWritesPerPeer(64U),
m_maximumCachedWrites(32U),
m_maximumCachedBytes(262144U)
{

}

bool LibuvChannelWritePoolConfig::valid() const
{
    return m_maximumWriteBytes != 0 &&
        m_maximumWriteBytes <= m_maximumPendingBytesPerPeer &&
        m_maximumPendingBytesPerPeer <= 0x7fffffffU &&
        m_maximumWritesPerPeer != 0 &&
        m_maximumWritesPerPeer <= 65536U &&
        m_maximumCachedWrites <= 65536U &&
        m_maximumCachedBytes <= 0x7fffffffU;
}