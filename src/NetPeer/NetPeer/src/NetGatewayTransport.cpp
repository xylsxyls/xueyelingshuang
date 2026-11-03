#include "NetPeerMacro.h"
#include "NetGatewayTransport.h"

NetGatewayNativeTransport::~NetGatewayNativeTransport()
{
    shutdown();
}

int32_t NetGatewayNativeTransport::connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
    NetLinkTransportListener* listener, uint64_t& peer)
{
    return m_transport.connect(address, port, options, listener, peer);
}

bool NetGatewayNativeTransport::poll()
{
    return m_transport.poll();
}

int32_t NetGatewayNativeTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    return m_transport.sendControl(peer, frame);
}

int32_t NetGatewayNativeTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    return m_transport.sendRecord(peer, record);
}

void NetGatewayNativeTransport::shutdown()
{
    m_transport.shutdown();
}

int32_t NetGatewayNativeTransport::outputState(uint64_t peer) const
{
    return m_transport.outputState(peer);
}