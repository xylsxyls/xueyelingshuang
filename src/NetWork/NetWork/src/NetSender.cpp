#include "NetWorkMacro.h"
#include "NetSender.h"
#include "NetWork.h"

NetWorkNetSender::NetWorkNetSender()
{

}

NetWorkNetSender::~NetWorkNetSender()
{

}

NetWorkNetSender& NetWorkNetSender::instance()
{
    static NetWorkNetSender s_instance;
    return s_instance;
}

int32_t NetWorkNetSender::createClient(NetWorkSenderState*& client)
{
    return NetWork::instance().createClient(client);
}

int32_t NetWorkNetSender::prepareGateway(NetWorkSenderState* client)
{
    return NetWork::instance().prepareGateway(client);
}

int32_t NetWorkNetSender::connectGateway(NetWorkSenderState* client, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    return NetWork::instance().connectGateway(client, options, listener);
}

int32_t NetWorkNetSender::connectServiceGateway(NetWorkSenderState* client, uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    return NetWork::instance().connectServiceGateway(client, product, options, listener);
}

int32_t NetWorkNetSender::configureConnectionPools(NetWorkSenderState* client, const NetConnectionPoolConfig& config)
{
    return NetWork::instance().configureConnectionPools(client, config);
}

int32_t NetWorkNetSender::configureRequestPool(NetWorkSenderState* client, const LockFreeHashMapPoolConfig& config)
{
    return NetWork::instance().configureRequestPool(client, config);
}

int32_t NetWorkNetSender::configureSubmissionPools(NetWorkSenderState* client, const NetSenderSubmissionPools& pools)
{
    return NetWork::instance().configureSubmissionPools(client, pools);
}

int32_t NetWorkNetSender::configureCompression(NetWorkSenderState* client, const uint32_t* types, uint32_t count)
{
    return NetWork::instance().configureCompression(client, types, count);
}

int32_t NetWorkNetSender::pollGateway(NetWorkSenderState* client)
{
    return NetWork::instance().pollGateway(client);
}

int32_t NetWorkNetSender::beginRequest(NetWorkSenderState* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& id)
{
    return NetWork::instance().beginRequest(client, product, type, timeoutMs, id);
}

int32_t NetWorkNetSender::postRequest(NetWorkSenderState* client, uint64_t id, const uint8_t* bytes, uint32_t size)
{
    return NetWork::instance().postRequest(client, id, bytes, size);
}

int32_t NetWorkNetSender::cancelRequest(NetWorkSenderState* client, uint64_t id)
{
    return NetWork::instance().cancelRequest(client, id);
}

int32_t NetWorkNetSender::postData(NetWorkSenderState* client, const NetInterfaceFrame& frame)
{
    return NetWork::instance().postData(client, frame);
}

void NetWorkNetSender::closeGateway(NetWorkSenderState* client, int32_t reason)
{
    NetWork::instance().closeGateway(client, reason);
}

void NetWorkNetSender::uninitGateway(NetWorkSenderState* client)
{
    NetWork::instance().uninitGateway(client);
}

int32_t NetWorkNetSender::destroyClient(NetWorkSenderState*& client)
{
    return NetWork::instance().destroyClient(client);
}

bool NetWorkNetSender::gatewayReady(NetWorkSenderState* client) const
{
    return NetWork::instance().gatewayReady(client);
}

uint64_t NetWorkNetSender::gatewayGeneration(NetWorkSenderState* client) const
{
    return NetWork::instance().gatewayGeneration(client);
}

int32_t NetWorkNetSender::lastGatewayError(NetWorkSenderState* client) const
{
    return NetWork::instance().lastGatewayError(client);
}

int32_t NetWorkNetSender::sendRequest(NetWorkSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    return NetWork::instance().sendRequest(client, requestId, bytes, size);
}

int32_t NetWorkNetSender::sendData(NetWorkSenderState* client, const NetInterfaceFrame& frame)
{
    return NetWork::instance().sendData(client, frame);
}

int32_t NetWorkNetSender::gatewayState(NetWorkSenderState* client, uint64_t& generation, bool& ready)
{
    return NetWork::instance().gatewayState(client, generation, ready);
}

int32_t NetWorkNetSender::beginSendRequest(NetWorkSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t& ticket)
{
    return NetWork::instance().beginSendRequest(client, requestId, bytes, size, ticket);
}

int32_t NetWorkNetSender::beginSendData(NetWorkSenderState* client, const NetInterfaceFrame& frame, uint64_t& ticket)
{
    return NetWork::instance().beginSendData(client, frame, ticket);
}

int32_t NetWorkNetSender::sendResult(NetWorkSenderState* client, uint64_t ticket)
{
    return NetWork::instance().sendResult(client, ticket);
}

NetFileProtocol& NetWorkNetSender::fileProtocol()
{
    return NetWork::instance().fileProtocol();
}

uint32_t NetWorkNetSender::pointerBytes()
{
    return NetWork::pointerBytes();
}

int32_t NetWorkNetSender::randomBytes(uint8_t* output, uint32_t size)
{
    return NetWork::randomBytes(output, size);
}

NetSenderBackend& NetWorkNetSender::backend()
{
    return NetWork::backend();
}

int32_t NetWorkNetSender::configureWorkers(uint32_t count)
{
    return NetWork::configureWorkers(count);
}

int32_t NetWorkNetSender::configureDispatch(NetSenderClient* client, bool parallel)
{
    return NetWork::instance().configureDispatch(client, parallel);
}