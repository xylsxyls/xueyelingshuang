#include "NetSenderLocalBuffer.h"
#include <utility>

NetSenderLocalBuffer::NetSenderLocalBuffer() :
m_size(0)
{

}

NetSenderLocalBuffer::NetSenderLocalBuffer(NetSenderLocalBuffer&& other) :
m_size(0)
{
    swap(other);
}

NetSenderLocalBuffer& NetSenderLocalBuffer::operator=(NetSenderLocalBuffer&& other)
{
    if (this != &other)
    {
        clear();
        swap(other);
    }
    return *this;
}

NetSenderLocalBuffer::~NetSenderLocalBuffer()
{
    clear();
}

int32_t NetSenderLocalBuffer::allocate(NetBufferPool& pool, uint32_t size)
{
    if (m_lease.data() != nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = pool.allocate(size, m_lease);
    if (result == NI_OK)
    {
        m_size = size;
    }
    return result;
}

void NetSenderLocalBuffer::swap(NetSenderLocalBuffer& other)
{
    m_lease.swap(other.m_lease);
    std::swap(m_size, other.m_size);
}

void NetSenderLocalBuffer::clear()
{
    // 本类型仅存已加密片或公开运输信封，不另做线性擦除；明文仍由上层池拥有者擦除。
    m_size = 0;
    m_lease.reset();
}

uint32_t NetSenderLocalBuffer::size() const
{
    return m_size;
}

uint8_t* NetSenderLocalBuffer::data() const
{
    return m_lease.data();
}

bool NetSenderLocalBuffer::empty() const
{
    return m_size == 0;
}