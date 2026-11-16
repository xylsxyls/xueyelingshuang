#include "NetInterfaceTcpProbe.h"
#include <stdexcept>

NetInterfaceTcpProbe::NetInterfaceTcpProbe() :
m_peer(0),
m_connections(0),
m_writable(0),
m_throwOnData(false),
m_closeChannel(nullptr),
m_closePeer(0)
{

}

void NetInterfaceTcpProbe::onConnected(uint64_t peer)
{
    m_peer = peer;
    ++m_connections;
}

void NetInterfaceTcpProbe::onData(uint64_t peer, const uint8_t* data, uint32_t size)
{
    (void)peer;
    if (m_throwOnData)
    {
        throw std::runtime_error("intentional listener failure");
    }
    m_received.insert(m_received.end(), data, data + size);
}

void NetInterfaceTcpProbe::onWritable(uint64_t peer)
{
    (void)peer;
    ++m_writable;
    if (m_closeChannel != nullptr)
    {
        LibuvChannel* channel = m_closeChannel;
        m_closeChannel = nullptr;
        if (m_closePeer == 0) { channel->shutdown(); }
        else { channel->close(m_closePeer, 42); }
    }
}

void NetInterfaceTcpProbe::onClosed(uint64_t peer, int32_t reason)
{
    ++m_closed[peer];
    m_reasons[peer] = reason;
}