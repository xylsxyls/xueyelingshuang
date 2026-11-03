#include "NetGatewayNodeState.h"
#include "NetGatewayNode.h"
#include "NetGatewayAuthority.h"
#include "NetServerEnrollment.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include <cstring>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <vector>
#include <set>
#include <cstdio>

static const uint64_t g_nodeLocalPeer = UINT64_C(0x8000000000000000);

NetGatewayNodeService::NetGatewayNodeService() : m_instance(0), m_product(0), m_capacity(0), m_window(65536), m_weight(0), m_active(0)
{

}

NetGatewayNodeMember::NetGatewayNodeMember() : m_seen(0), m_verified(false)
{

}

NetGatewayNodeCandidate::NetGatewayNodeCandidate() : m_retryAt(0), m_delay(250), m_seed(false)
{

}

NetGatewayNodeRoute::NetGatewayNodeRoute() : m_client(0), m_clientChannel(0), m_service(0), m_serviceChannel(0), m_product(0),
    m_instance(0), m_remoteRequest(0), m_boot(0), m_deadline(0), m_loadRevision(0), m_original(), m_pending(false),
    m_forwardBlocked(0), m_reverseBlocked(0)
{

}

NetGatewayNodeConnection::NetGatewayNodeConnection(NetGatewayNode& owner, uint64_t id, const std::string& address,
    uint32_t port, const std::string& expectedName) : m_address(address), m_expectedName(expectedName), m_port(port),
    m_closed(false), m_owner(&owner), m_id(id), m_peer(0)
{

}

NetGatewayNodeConnection::~NetGatewayNodeConnection()
{
    shutdown();
}

int32_t NetGatewayNodeConnection::connect(const NetInterfaceLinkOptions& options)
{
    return m_transport.connect(m_address.c_str(), m_port, options, this, m_peer);
}

bool NetGatewayNodeConnection::poll()
{
    return m_transport.poll();
}

int32_t NetGatewayNodeConnection::sendControl(const NetInterfaceFrame& frame)
{
    return m_transport.sendControl(m_peer, frame);
}

int32_t NetGatewayNodeConnection::sendRecord(const NetInterfaceTunnelRecord& record)
{
    return m_transport.sendRecord(m_peer, record);
}

void NetGatewayNodeConnection::close(int32_t reason)
{
    m_transport.close(m_peer, reason);
}

void NetGatewayNodeConnection::shutdown()
{
    m_transport.shutdown();
}

void NetGatewayNodeConnection::onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity)
{
    // 缓存中的节点名只是提示；删除identity.dat后允许同地址的新CA有效NODE身份重新加入。
    // 本次身份仍由Link验证精确证书名和新挑战持有证明，随后快照必须匹配这个新身份。
    if (peer != m_peer || identity.m_role != NI_LINK_NODE || identity.m_name == nullptr) { close(NI_PROTOCOL_ERROR); return; }
    m_owner->onLinkReady(m_id, identity);
}

bool NetGatewayNodeConnection::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    return peer != m_peer ? true : m_owner->onLinkControl(m_id, frame);
}

bool NetGatewayNodeConnection::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    return peer != m_peer ? true : m_owner->onLinkRecord(m_id, record);
}

void NetGatewayNodeConnection::onLinkClosed(uint64_t peer, int32_t reason)
{
    if (peer != m_peer) { return; }
    m_closed = true;
    m_owner->onLinkClosed(m_id, reason);
}

NetGatewayNodeControl::NetGatewayNodeControl(const NetInterfaceFrame& frame, const std::string& payload) :
    m_frame(frame), m_payload(payload)
{
    m_frame.m_payload = nullptr;
    m_frame.m_payloadSize = static_cast<uint32_t>(m_payload.size());
}

NetGatewayNodePeer::NetGatewayNodePeer() : m_role(0), m_request(0), m_highestChannel(0), m_product(0),
    m_capacity(0), m_window(65536), m_weight(1), m_active(0), m_opened(0), m_controlBytes(0), m_serviceReady(false), m_closing(false)
{
    m_outRequest = 0;
    m_outChannel = 0;
    m_discoverRequest = 0;
    m_discoverAt = 0;
    m_snapshotReceived = 0;
    m_snapshotRevision = 0;
    m_snapshotSent = 0;
    m_blockedSince = 0;
}

NetGatewayNodeLocalListener::NetGatewayNodeLocalListener(NetGatewayNode& owner) : m_owner(&owner)
{

}

void NetGatewayNodeLocalListener::onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity)
{
    if (peer >= g_nodeLocalPeer) { m_owner->m_local.close(peer, NI_LIMIT_EXCEEDED); return; }
    m_owner->onLinkReady(peer | g_nodeLocalPeer, identity);
}

bool NetGatewayNodeLocalListener::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    return m_owner->onLinkControl(peer | g_nodeLocalPeer, frame);
}

bool NetGatewayNodeLocalListener::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    return m_owner->onLinkRecord(peer | g_nodeLocalPeer, record);
}

void NetGatewayNodeLocalListener::onLinkClosed(uint64_t peer, int32_t reason)
{
    m_owner->onLinkClosed(peer | g_nodeLocalPeer, reason);
}