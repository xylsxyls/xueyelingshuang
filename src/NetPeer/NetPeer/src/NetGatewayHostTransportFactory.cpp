#include "NetPeerMacro.h"
#include "NetGatewayHostTransportFactory.h"

NetGatewayHostTransportFactory::NetGatewayHostTransportFactory(const NetLinkTransportConfig& config) : m_config(config)
{
}

NetGatewayTransport* NetGatewayHostTransportFactory::create()
{
    return new NetGatewayNativeTransport(m_config);
}