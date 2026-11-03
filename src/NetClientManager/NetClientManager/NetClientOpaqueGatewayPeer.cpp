#include "NetClientOpaqueGatewayPeer.h"
#include "NetClientOpaqueGateway.h"

NetClientOpaqueGatewayPeer::NetClientOpaqueGatewayPeer(NetClientOpaqueGateway& owner, uint64_t local) :
m_owner(&owner),
m_local(local),
m_remote(0),
m_address(),
m_connected(false),
m_closed(false)
{
    m_address.m_structSize = sizeof(m_address);
    m_address.m_version = 1;
}

NetClientOpaqueGatewayPeer::~NetClientOpaqueGatewayPeer()
{
    m_transport.shutdown();
}

void NetClientOpaqueGatewayPeer::onRelayConnected(uint64_t peer)
{
    m_remote = peer;
    m_connected = true;
}

bool NetClientOpaqueGatewayPeer::onRelayRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (m_closed || peer != m_remote) { return true; }
    const int32_t sent = m_owner->m_local.sendRecord(m_local, record);
    if (sent == NI_LIMIT_EXCEEDED) { return false; }
    if (sent != NI_OK)
    {
        m_closed = true;
        m_transport.close(peer, sent);
        m_owner->m_local.close(m_local, sent);
    }
    return true;
}

void NetClientOpaqueGatewayPeer::onRelayClosed(uint64_t peer, int32_t reason)
{
    if (peer != m_remote && m_remote != 0) { return; }
    m_connected = false;
    m_owner->remoteClosed(m_local, reason);
    m_closed = true;
}