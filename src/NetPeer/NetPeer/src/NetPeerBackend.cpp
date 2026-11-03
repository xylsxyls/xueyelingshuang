#include "NetPeerBackend.h"
#include "NetPeer.h"

int32_t NetPeerBackend::configureHost(const NetHostFunctions& functions)
{
    return NetPeer::configureHost(functions);
}

int32_t NetPeerBackend::configureGateways(NetPeerSenderState* client, const char* seeds, const char* cachePath)
{
    return NetPeer::instance().configureGateways(client, seeds, cachePath);
}