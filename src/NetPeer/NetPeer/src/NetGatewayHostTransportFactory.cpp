#include "NetPeerMacro.h"
#include "NetGatewayHostTransportFactory.h"

NetGatewayTransport* NetGatewayHostTransportFactory::create()
{
    return new NetGatewayNativeTransport();
}