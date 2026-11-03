#include "LibuvHostState.h"

LibuvHostState::LibuvHostState(void* context, LibuvHostConnected connected, LibuvHostData data,
    LibuvHostWritable writable, LibuvHostClosed closed) :
m_context(context),
m_connected(connected),
m_data(data),
m_writable(writable),
m_closed(closed),
m_depth(0),
m_polling(false),
m_stopping(false)
{

}

LibuvHostState::~LibuvHostState()
{
    m_stopping = true;
    m_channel.shutdown();
}

int32_t LibuvHostState::listen(const char* address, uint32_t port, uint32_t maxPeers)
{
    return m_channel.listen(address, port, this, maxPeers);
}

int32_t LibuvHostState::connect(const char* address, uint32_t port, uint64_t& peer)
{
    return m_channel.connect(address, port, this, peer);
}

void LibuvHostState::onConnected(uint64_t peer)
{
    ++m_depth;
    try
    {
        m_connected(m_context, peer);
    }
    catch (...)
    {
        m_channel.close(peer, -1);
    }
    --m_depth;
}

void LibuvHostState::onData(uint64_t peer, const uint8_t* data, uint32_t size)
{
    ++m_depth;
    try
    {
        m_data(m_context, peer, data, size);
    }
    catch (...)
    {
        m_channel.close(peer, -1);
    }
    --m_depth;
}

void LibuvHostState::onWritable(uint64_t peer)
{
    ++m_depth;
    try
    {
        m_writable(m_context, peer);
    }
    catch (...)
    {
        m_channel.close(peer, -1);
    }
    --m_depth;
}

void LibuvHostState::onClosed(uint64_t peer, int32_t reason)
{
    ++m_depth;
    try
    {
        m_closed(m_context, peer, reason);
    }
    catch (...)
    {
        // 句柄已经关闭，外部终态异常不能再次回调或逃出C边界
    }
    --m_depth;
}