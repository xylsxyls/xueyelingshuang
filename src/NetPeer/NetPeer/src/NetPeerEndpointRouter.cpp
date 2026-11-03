#include "NetPeerMacro.h"
#include "NetPeerEndpointRouter.h"
#include "NetTransportEntropy.h"

NetPeerEndpointRouter::NetPeerEndpointRouter(NetEndpointServiceSink* services, NetEndpointServiceSink* clients) :
NetEndpointRouter(services, clients)
{
    if (clients == nullptr)
    {
        bindTransport(&m_transport);
    }
}

NetPeerEndpointRouter::~NetPeerEndpointRouter()
{
    shutdown();
    bindTransport(nullptr);
}

int32_t NetPeerEndpointRouter::listen(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls, uint32_t peers)
{
    NetInterfaceTlsOptions configured = tls;
    if (configured.entropy == nullptr)
    {
        configured.entropy = NetTransportEntropy;
        configured.entropyContext = nullptr;
    }
    return NetEndpointRouter::listen(address, port, configured, peers);
}