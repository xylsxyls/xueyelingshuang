#include "NetSenderMacro.h"
#include "NetSenderIpc.h"
#include "NetSender.h"

NetSender::NetSender()
{

}

NetSender::~NetSender()
{

}

NetSender& NetSender::instance()
{
    static NetSender s_instance;
    return s_instance;
}

int32_t NetSender::createClient(NetSenderGatewayState*& client)
{
    return NetSenderIpc::instance().createClient(client);
}

int32_t NetSender::prepareGateway(NetSenderGatewayState* client)
{
    return NetSenderIpc::instance().prepareGateway(client);
}

int32_t NetSender::connectGateway(NetSenderGatewayState* client, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    return NetSenderIpc::instance().connectGateway(client, options, listener);
}

int32_t NetSender::connectServiceGateway(NetSenderGatewayState* client, uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    return NetSenderIpc::instance().connectServiceGateway(client, product, options, listener);
}

int32_t NetSender::configureConnectionPools(NetSenderGatewayState* client, const NetConnectionPoolConfig& config)
{
    return NetSenderIpc::instance().configureConnectionPools(client, config);
}

int32_t NetSender::configureRequestPool(NetSenderGatewayState* client, const LockFreeHashMapPoolConfig& config)
{
    return NetSenderIpc::instance().configureRequestPool(client, config);
}

int32_t NetSender::configureSubmissionPools(NetSenderGatewayState* client, const NetSenderSubmissionPools& pools)
{
    return NetSenderIpc::instance().configureSubmissionPools(client, pools);
}

int32_t NetSender::configureCompression(NetSenderGatewayState* client, const uint32_t* types, uint32_t count)
{
    return NetSenderIpc::instance().configureCompression(client, types, count);
}

int32_t NetSender::pollGateway(NetSenderGatewayState* client)
{
    return NetSenderIpc::instance().pollGateway(client);
}

int32_t NetSender::beginRequest(NetSenderGatewayState* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId)
{
    return NetSenderIpc::instance().beginRequest(client, product, type, timeoutMs, requestId);
}

int32_t NetSender::postRequest(NetSenderGatewayState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    return NetSenderIpc::instance().postRequest(client, requestId, bytes, size);
}

int32_t NetSender::cancelRequest(NetSenderGatewayState* client, uint64_t requestId)
{
    return NetSenderIpc::instance().cancelRequest(client, requestId);
}

int32_t NetSender::postData(NetSenderGatewayState* client, const NetInterfaceFrame& frame)
{
    return NetSenderIpc::instance().postData(client, frame);
}

void NetSender::closeGateway(NetSenderGatewayState* client, int32_t reason)
{
    NetSenderIpc::instance().closeGateway(client, reason);
}

void NetSender::uninitGateway(NetSenderGatewayState* client)
{
    NetSenderIpc::instance().uninitGateway(client);
}

int32_t NetSender::destroyClient(NetSenderGatewayState*& client)
{
    return NetSenderIpc::instance().destroyClient(client);
}

bool NetSender::gatewayReady(NetSenderGatewayState* client) const
{
    return NetSenderIpc::instance().gatewayReady(client);
}

uint64_t NetSender::gatewayGeneration(NetSenderGatewayState* client) const
{
    return NetSenderIpc::instance().gatewayGeneration(client);
}

int32_t NetSender::lastGatewayError(NetSenderGatewayState* client) const
{
    return NetSenderIpc::instance().lastGatewayError(client);
}

int32_t NetSender::sendRequest(NetSenderGatewayState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    return NetSenderIpc::instance().sendRequest(client, requestId, bytes, size);
}

int32_t NetSender::sendData(NetSenderGatewayState* client, const NetInterfaceFrame& frame)
{
    return NetSenderIpc::instance().sendData(client, frame);
}

int32_t NetSender::gatewayState(NetSenderGatewayState* client, uint64_t& generation, bool& ready)
{
    return NetSenderIpc::instance().gatewayState(client, generation, ready);
}

int32_t NetSender::beginSendRequest(NetSenderGatewayState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t& ticket)
{
    return NetSenderIpc::instance().beginSendRequest(client, requestId, bytes, size, ticket);
}

int32_t NetSender::beginSendData(NetSenderGatewayState* client, const NetInterfaceFrame& frame, uint64_t& ticket)
{
    return NetSenderIpc::instance().beginSendData(client, frame, ticket);
}

int32_t NetSender::sendResult(NetSenderGatewayState* client, uint64_t ticket)
{
    return NetSenderIpc::instance().sendResult(client, ticket);
}

NetFileProtocol& NetSender::fileProtocol()
{
    return NetSenderIpc::instance().fileProtocol();
}

uint32_t NetSender::pointerBytes()
{
    return NetSenderIpc::pointerBytes();
}

int32_t NetSender::randomBytes(uint8_t* output, uint32_t size)
{
    return NetSenderIpc::randomBytes(output, size);
}

NetSenderBackend& NetSender::backend()
{
    return NetSenderIpc::backend();
}

int32_t NetSender::configureWorkers(uint32_t count)
{
    return NetSenderIpc::configureWorkers(count);
}

int32_t NetSender::configureDispatch(NetSenderClient* client, bool parallel)
{
    return NetSenderIpc::instance().configureDispatch(client, parallel);
}