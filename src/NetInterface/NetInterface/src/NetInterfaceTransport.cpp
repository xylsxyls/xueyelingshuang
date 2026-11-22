#include "NetInterfaceMacro.h"
#include "NetInterfaceTransport.h"

NetTransportListener::~NetTransportListener()
{

}

NetClientChannel::~NetClientChannel()
{

}

int32_t NetClientChannel::configureConnectionPools(const NetConnectionPoolConfig& config)
{
    (void)config;
    return NI_NOT_SUPPORTED;
}

NetServerChannel::~NetServerChannel()
{

}

int32_t NetClientChannel::configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated)
{
    (void)peer;
    (void)types;
    (void)count;
    (void)negotiated;
    return NI_NOT_SUPPORTED;
}

NetLinkTransportListener::~NetLinkTransportListener()
{

}

NetGatewayTransportFactory::~NetGatewayTransportFactory()
{

}

NetGatewayTransport::~NetGatewayTransport()
{

}


int32_t NetClientChannel::outputState(uint64_t peer) const
{
    (void)peer;
    return NI_NOT_SUPPORTED;
}

int32_t NetGatewayTransport::outputState(uint64_t peer) const
{
    (void)peer;
    return NI_NOT_SUPPORTED;
}

int32_t NetClientChannel::configureCompact(uint64_t peer, bool enabled)
{
    (void)peer;
    return enabled ? NI_NOT_SUPPORTED : NI_OK;
}