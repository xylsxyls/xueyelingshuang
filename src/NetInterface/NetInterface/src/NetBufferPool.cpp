#include "NetBufferPool.h"
#include "NetInterfaceMacro.h"

NetBufferLease::NetBufferLease() :
m_pool(nullptr),
m_storage(nullptr),
m_bytes(nullptr),
m_capacity(0)
{

}

NetBufferLease::~NetBufferLease()
{
    reset();
}

void NetBufferLease::swap(NetBufferLease& other)
{
    std::swap(m_pool, other.m_pool);
    std::swap(m_storage, other.m_storage);
    std::swap(m_bytes, other.m_bytes);
    std::swap(m_capacity, other.m_capacity);
}

void NetBufferLease::reset()
{
    if (m_storage != nullptr)
    {
        m_pool->release(m_capacity, m_storage);
    }
    m_pool = nullptr;
    m_storage = nullptr;
    m_bytes = nullptr;
    m_capacity = 0;
}

uint8_t* NetBufferLease::data() const
{
    return m_bytes;
}

uint32_t NetBufferLease::capacity() const
{
    return m_capacity;
}

void NetBufferLease::adopt(NetBufferPool& pool, void* storage, uint8_t* bytes, uint32_t size)
{
    reset();
    m_pool = &pool;
    m_storage = storage;
    m_bytes = bytes;
    m_capacity = size;
}

NetBufferPool::NetBufferPool(const NetBufferPoolConfig& config) :
m_configurationStatus(config.status()),
m_small(config.m_classes[0]),
m_short(config.m_classes[1]),
m_medium(config.m_classes[2]),
m_chunk(config.m_classes[3]),
m_large(config.m_classes[4])
{

}

int32_t NetBufferPool::status() const
{
    if (m_configurationStatus != NI_OK)
    {
        return m_configurationStatus;
    }
    const int32_t states[] = {m_small.status(), m_short.status(), m_medium.status(), m_chunk.status(), m_large.status()};
    for (size_t index = 0; index < 5; ++index)
    {
        if (states[index] != NI_OK)
        {
            return states[index];
        }
    }
    return NI_OK;
}

int32_t NetBufferPool::allocate(uint32_t size, NetBufferLease& lease)
{
    if (m_configurationStatus != NI_OK)
    {
        return m_configurationStatus;
    }
    if (lease.data() != nullptr || size > 1048704)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (size == 0)
    {
        return NI_OK;
    }
    if (size <= 64)
    {
        return acquire(m_small, lease);
    }
    if (size <= 1024)
    {
        return acquire(m_short, lease);
    }
    if (size <= 16448)
    {
        return acquire(m_medium, lease);
    }
    if (size <= 131072)
    {
        return acquire(m_chunk, lease);
    }
    return acquire(m_large, lease);
}

void NetBufferPool::release(uint32_t size, void* storage)
{
    switch (size)
    {
    case 64: m_small.release(storage); break;
    case 1024: m_short.release(storage); break;
    case 16448: m_medium.release(storage); break;
    case 131072: m_chunk.release(storage); break;
    case 1048704: m_large.release(storage); break;
    }
}

LockFreeHashMapPoolStats NetBufferPool::statistics(uint32_t index)
{
    switch (index)
    {
    case 0: return m_small.statistics();
    case 1: return m_short.statistics();
    case 2: return m_medium.statistics();
    case 3: return m_chunk.statistics();
    case 4: return m_large.statistics();
    default: return LockFreeHashMapPoolStats();
    }
}