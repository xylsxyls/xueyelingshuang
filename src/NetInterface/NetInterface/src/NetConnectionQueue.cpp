#include "NetConnectionQueue.h"
#include "NetInterfaceMacro.h"
#include "NetInterface.h"

NetConnectionWrite::NetConnectionWrite() :
m_pool(nullptr),
m_storage(nullptr),
m_next(nullptr),
m_size(0)
{

}

NetConnectionWrite::~NetConnectionWrite()
{
    NetInterface::wipe(m_buffer.data(), m_size);
}

NetConnectionQueue::NetConnectionQueue(const NetConnectionPoolConfig& config) :
m_dataBytes(config.m_dataBytes),
m_controlBytes(config.m_controlBytes),
m_dataRecords(config.m_dataRecords),
m_controlRecords(config.m_controlRecords),
m_active(nullptr),
m_controlRun(0)
{
    m_heads[0] = m_heads[1] = m_tails[0] = m_tails[1] = nullptr;
    for (uint32_t queue = 0; queue < 2; ++queue)
    {
        for (uint32_t category = 0; category < 5; ++category)
        {
            m_cached[queue][category] = nullptr;
        }
    }
}

NetConnectionQueue::~NetConnectionQueue()
{
    clear();
}

int32_t NetConnectionQueue::status() const
{
    const int32_t states[] = {m_dataBytes.status(), m_controlBytes.status(), m_dataRecords.status(), m_controlRecords.status()};
    for (size_t index = 0; index < 4; ++index)
    {
        if (states[index] != NI_OK)
        {
            return states[index];
        }
    }
    return NI_OK;
}

int32_t NetConnectionQueue::allocate(bool control, uint32_t bytes, NetObjectReference<NetConnectionWrite>& output)
{
    const uint32_t queue = control ? 1 : 0;
    const uint32_t category = bufferClass(bytes);
    NetConnectionWrite* cached = m_cached[queue][category];
    if (cached != nullptr)
    {
        m_cached[queue][category] = cached->m_next;
        cached->m_next = nullptr;
        output.adopt(*cached->m_pool, cached->m_storage);
        return NI_OK;
    }
    // 先复用其他容量类的空闲记录；不能让缓存占满记录池后误报没有可用记录。
    for (uint32_t other = 0; other < 5; ++other)
    {
        cached = m_cached[queue][other];
        if (cached != nullptr)
        {
            m_cached[queue][other] = cached->m_next;
            cached->m_next = nullptr;
            cached->m_buffer.reset();
            output.adopt(*cached->m_pool, cached->m_storage);
            break;
        }
    }
    NetObjectPool<NetConnectionWrite>& records = control ? m_controlRecords : m_dataRecords;
    int32_t result = output ? NI_OK : records.create(output);
    if (result == NI_OK)
    {
        result = (control ? m_controlBytes : m_dataBytes).allocate(bytes, output->m_buffer);
        if (result == NI_OK)
        {
            output->m_pool = &records;
            output->m_storage = output.storage();
        }
        else
        {
            output.reset();
        }
    }
    return result;
}

void NetConnectionQueue::append(bool control, NetConnectionWrite* write)
{
    const uint32_t queue = control ? 1 : 0;
    write->m_next = nullptr;
    if (m_tails[queue] == nullptr)
    {
        m_heads[queue] = write;
    }
    else
    {
        m_tails[queue]->m_next = write;
    }
    m_tails[queue] = write;
}

NetConnectionWrite* NetConnectionQueue::active()
{
    if (m_active == nullptr)
    {
        const uint32_t queue = m_heads[1] != nullptr && (m_heads[0] == nullptr || m_controlRun < 32) ? 1 : 0;
        m_active = m_heads[queue];
        if (m_active != nullptr)
        {
            m_controlRun = queue == 0 ? 0 : (m_controlRun < 32 ? m_controlRun + 1 : 32);
            m_heads[queue] = m_active->m_next;
            if (m_heads[queue] == nullptr)
            {
                m_tails[queue] = nullptr;
            }
            m_active->m_next = nullptr;
        }
    }
    return m_active;
}

void NetConnectionQueue::complete()
{
    if (m_active != nullptr)
    {
        NetConnectionWrite* write = m_active;
        m_active = nullptr;
        NetInterface::wipe(write->m_buffer.data(), write->m_size);
        write->m_size = 0;
        const uint32_t queue = write->m_pool == &m_controlRecords ? 1 : 0;
        const uint32_t category = bufferClass(write->m_buffer.capacity());
        write->m_next = m_cached[queue][category];
        m_cached[queue][category] = write;
    }
}

void NetConnectionQueue::clear()
{
    complete();
    m_controlRun = 0;
    for (uint32_t queue = 0; queue < 2; ++queue)
    {
        while (m_heads[queue] != nullptr)
        {
            NetConnectionWrite* write = m_heads[queue];
            m_heads[queue] = write->m_next;
            write->m_pool->release(write->m_storage);
        }
        m_tails[queue] = nullptr;
        for (uint32_t category = 0; category < 5; ++category)
        {
            while (m_cached[queue][category] != nullptr)
            {
                NetConnectionWrite* write = m_cached[queue][category];
                m_cached[queue][category] = write->m_next;
                write->m_pool->release(write->m_storage);
            }
        }
    }
}

uint32_t NetConnectionQueue::bufferClass(uint32_t bytes)
{
    return bytes <= 64 ? 0 : bytes <= 1024 ? 1 : bytes <= 16448 ? 2 : bytes <= 131072 ? 3 : 4;
}

bool NetConnectionQueue::empty() const
{
    return m_active == nullptr && m_heads[0] == nullptr && m_heads[1] == nullptr;
}