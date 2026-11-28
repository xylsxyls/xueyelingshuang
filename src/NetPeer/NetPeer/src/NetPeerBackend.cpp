#include "NetPeerBackend.h"
#include "NetPeer.h"

int32_t NetPeerBackend::configureHost(const NetHostFunctions& functions)
{
    return NetPeer::configureHost(functions);
}

int32_t NetPeerBackend::prepareGateway(NetPeerSenderState* client, const NetPeerConfig& config)
{
    return NetPeer::instance().prepareGateway(client, config);
}

int32_t NetPeerBackend::configureSubmissionPools(NetPeerSenderState* client, const NetSenderSubmissionPools& pools,
    const NetSenderSubmissionOptions& options)
{
    return NetPeer::instance().configureSubmissionPools(client, pools, options);
}

int32_t NetPeerBackend::configureGateways(NetPeerSenderState* client, const char* seeds, const char* cachePath)
{
    return NetPeer::instance().configureGateways(client, seeds, cachePath);
}