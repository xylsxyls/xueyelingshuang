#include "NetGatewayNodeRecordPool.h"
#include <cstring>

LockFreeHashMapPoolConfig NetGatewayNodeRecordPool::defaultRecords()
{
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 128;
    config.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetGatewayNodeRecord>>);
    config.directoryCapacity = 128;
    config.lookupCacheCapacity = 0;
    return config;
}

NetGatewayNodeRecordPool::NetGatewayNodeRecordPool() :
m_bodies(NetBufferPoolConfig()),
m_records(defaultRecords())
{

}

NetGatewayNodeRecordPool::NetGatewayNodeRecordPool(const NetBufferPoolConfig& bodies, const LockFreeHashMapPoolConfig& records) :
m_bodies(bodies),
m_records(records)
{

}

int32_t NetGatewayNodeRecordPool::status() const
{
    return m_bodies.status() == NI_OK ? m_records.status() : m_bodies.status();
}

int32_t NetGatewayNodeRecordPool::copy(const uint8_t* data, uint32_t size, NetObjectReference<NetGatewayNodeRecord>& output)
{
    if (output || data == nullptr || size == 0 || size > 16384)
    {
        return NI_INVALID_ARGUMENT;
    }
    int32_t result = status();
    if (result != NI_OK)
    {
        return result;
    }
    NetObjectReference<NetGatewayNodeRecord> record;
    result = m_records.create(record);
    if (result == NI_OK)
    {
        result = m_bodies.allocate(size, record->m_bytes);
    }
    if (result != NI_OK)
    {
        return result;
    }
    std::memcpy(record->m_bytes.data(), data, size);
    record->m_size = size;
    output = std::move(record);
    return NI_OK;
}