#include "NetChannelCredit.h"

NetChannelCredit::NetChannelCredit() :
m_capacity(NET_CHANNEL_DEFAULT_WINDOW),
m_used(false),
m_outstanding(0),
m_pending(0),
m_error(NC_OK)
{

}

int32_t NetChannelCredit::configure(uint32_t bytes)
{
    if (bytes < NET_CHANNEL_DEFAULT_WINDOW || bytes > NET_CHANNEL_MAX_WINDOW || bytes % NET_CHANNEL_PAYLOAD_LIMIT != 0)
    {
        return NC_INVALID_ARGUMENT;
    }
    if (m_used || m_error != NC_OK)
    {
        return NC_INVALID_STATE;
    }
    m_capacity = bytes;
    return NC_OK;
}

uint32_t NetChannelCredit::capacity() const
{
    return m_capacity;
}

uint32_t NetChannelCredit::available() const
{
    return m_error == NC_OK ? m_capacity - m_outstanding : 0;
}

int32_t NetChannelCredit::sent(uint32_t size)
{
    if (m_error != NC_OK)
    {
        return m_error;
    }
    if (size == 0 || size > NET_CHANNEL_PAYLOAD_LIMIT)
    {
        return NC_INVALID_ARGUMENT;
    }
    if (size > m_capacity - m_outstanding)
    {
        return NC_LIMIT_EXCEEDED;
    }
    m_outstanding += size;
    m_used = true;
    return NC_OK;
}

int32_t NetChannelCredit::grant(uint32_t size)
{
    if (m_error != NC_OK)
    {
        return m_error;
    }
    if (size == 0 || size > m_outstanding)
    {
        m_error = NC_PROTOCOL_ERROR;
        return m_error;
    }
    m_outstanding -= size;
    return NC_OK;
}

int32_t NetChannelCredit::received(uint32_t size)
{
    if (m_error != NC_OK)
    {
        return m_error;
    }
    if (size == 0 || size > NET_CHANNEL_PAYLOAD_LIMIT || size > m_capacity - m_pending)
    {
        m_error = NC_PROTOCOL_ERROR;
        return m_error;
    }
    m_pending += size;
    m_used = true;
    return NC_OK;
}

uint32_t NetChannelCredit::pending() const
{
    return m_error == NC_OK ? m_pending : 0;
}

int32_t NetChannelCredit::consume(uint32_t size)
{
    if (m_error != NC_OK)
    {
        return m_error;
    }
    if (size == 0 || size > m_pending)
    {
        return NC_INVALID_ARGUMENT;
    }
    m_pending -= size;
    return NC_OK;
}

int32_t NetChannelCredit::status() const
{
    return m_error;
}