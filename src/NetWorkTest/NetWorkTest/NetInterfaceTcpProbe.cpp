#include "NetInterfaceTcpProbe.h"
#include <stdexcept>

NetInterfaceTcpProbe::NetInterfaceTcpProbe() :
m_peer(0),
m_connections(0),
m_writable(0),
m_throwOnData(false)
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
}

void NetInterfaceTcpProbe::onClosed(uint64_t peer, int32_t reason)
{
    ++m_closed[peer];
    m_reasons[peer] = reason;
}