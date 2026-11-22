#include "NetPeerMacro.h"
#include "NetSender.h"
#include "NetPeer.h"

NetPeerNetSender::NetPeerNetSender()
{

}

NetPeerNetSender::~NetPeerNetSender()
{

}

NetPeerNetSender& NetPeerNetSender::instance()
{
    static NetPeerNetSender s_instance;
    return s_instance;
}

int32_t NetPeerNetSender::createClient(NetPeerSenderState*& client)
{
    return NetPeer::instance().createClient(client);
}

int32_t NetPeerNetSender::prepareGateway(NetPeerSenderState* client)
{
    return NetPeer::instance().prepareGateway(client);
}

int32_t NetPeerNetSender::connectGateway(NetPeerSenderState* client, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    return NetPeer::instance().connectGateway(client, options, listener);
}

int32_t NetPeerNetSender::connectServiceGateway(NetPeerSenderState* client, uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    return NetPeer::instance().connectServiceGateway(client, product, options, listener);
}

int32_t NetPeerNetSender::configureConnectionPools(NetPeerSenderState* client, const NetConnectionPoolConfig& config)
{
    return NetPeer::instance().configureConnectionPools(client, config);
}

int32_t NetPeerNetSender::configureRequestPool(NetPeerSenderState* client, const LockFreeHashMapPoolConfig& config)
{
    return NetPeer::instance().configureRequestPool(client, config);
}

int32_t NetPeerNetSender::configureSubmissionPools(NetPeerSenderState* client, const NetSenderSubmissionPools& pools)
{
    return NetPeer::instance().configureSubmissionPools(client, pools);
}

int32_t NetPeerNetSender::configureCompression(NetPeerSenderState* client, const uint32_t* types, uint32_t count)
{
    return NetPeer::instance().configureCompression(client, types, count);
}

int32_t NetPeerNetSender::beginRequest(NetPeerSenderState* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId)
{
    return NetPeer::instance().beginRequest(client, product, type, timeoutMs, requestId);
}

int32_t NetPeerNetSender::postRequest(NetPeerSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    return NetPeer::instance().postRequest(client, requestId, bytes, size);
}

int32_t NetPeerNetSender::cancelRequest(NetPeerSenderState* client, uint64_t requestId)
{
    return NetPeer::instance().cancelRequest(client, requestId);
}

int32_t NetPeerNetSender::postData(NetPeerSenderState* client, const NetInterfaceFrame& frame)
{
    return NetPeer::instance().postData(client, frame);
}

int32_t NetPeerNetSender::sendRequest(NetPeerSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    return NetPeer::instance().sendRequest(client, requestId, bytes, size);
}

int32_t NetPeerNetSender::sendData(NetPeerSenderState* client, const NetInterfaceFrame& frame)
{
    return NetPeer::instance().sendData(client, frame);
}

int32_t NetPeerNetSender::pollGateway(NetPeerSenderState* client)
{
    return NetPeer::instance().pollGateway(client);
}

void NetPeerNetSender::closeGateway(NetPeerSenderState* client, int32_t reason)
{
    NetPeer::instance().closeGateway(client, reason);
}

void NetPeerNetSender::uninitGateway(NetPeerSenderState* client)
{
    NetPeer::instance().uninitGateway(client);
}

int32_t NetPeerNetSender::destroyClient(NetPeerSenderState*& client)
{
    return NetPeer::instance().destroyClient(client);
}

bool NetPeerNetSender::gatewayReady(NetPeerSenderState* client) const
{
    return NetPeer::instance().gatewayReady(client);
}

uint64_t NetPeerNetSender::gatewayGeneration(NetPeerSenderState* client) const
{
    return NetPeer::instance().gatewayGeneration(client);
}

int32_t NetPeerNetSender::lastGatewayError(NetPeerSenderState* client) const
{
    return NetPeer::instance().lastGatewayError(client);
}

int32_t NetPeerNetSender::gatewayState(NetPeerSenderState* client, uint64_t& generation, bool& ready)
{
    return NetPeer::instance().gatewayState(client, generation, ready);
}

int32_t NetPeerNetSender::beginSendRequest(NetPeerSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t& ticket)
{
    return NetPeer::instance().beginSendRequest(client, requestId, bytes, size, ticket);
}

int32_t NetPeerNetSender::beginSendData(NetPeerSenderState* client, const NetInterfaceFrame& frame, uint64_t& ticket)
{
    return NetPeer::instance().beginSendData(client, frame, ticket);
}

int32_t NetPeerNetSender::sendResult(NetPeerSenderState* client, uint64_t ticket)
{
    return NetPeer::instance().sendResult(client, ticket);
}

NetFileProtocol& NetPeerNetSender::fileProtocol()
{
    return NetPeer::instance().fileProtocol();
}

uint32_t NetPeerNetSender::pointerBytes()
{
    return NetPeer::pointerBytes();
}

int32_t NetPeerNetSender::randomBytes(uint8_t* output, uint32_t size)
{
    return NetPeer::randomBytes(output, size);
}

NetSenderBackend& NetPeerNetSender::backend()
{
    return NetPeer::backend();
}

int32_t NetPeerNetSender::configureWorkers(uint32_t count)
{
    return NetPeer::configureWorkers(count);
}

int32_t NetPeerNetSender::configureDispatch(NetSenderClient* client, bool parallel)
{
    return NetPeer::instance().configureDispatch(client, parallel);
}