#include "NetPeerMacro.h"
#include "NetPeer.h"
#include <new>
#include "NetPeerSenderState.h"
#include "NetPeerClientState.h"
#include <cstring>

NetPeer::NetPeer()
{

}

NetPeer::~NetPeer()
{

}

NetPeer& NetPeer::instance()
{
    static NetPeer s_instance;
    return s_instance;
}

int32_t NetPeer::clientCreate(const NetPeerClientCallbacks* callbacks, NetPeerClientHandle* handle)
{
    if (handle != nullptr)
    {
        *handle = nullptr;
    }
    if (handle == nullptr || callbacks == nullptr || callbacks->structSize != sizeof(NetPeerClientCallbacks) || callbacks->version != 1 ||
        callbacks->ready == nullptr || callbacks->result == nullptr || callbacks->frame == nullptr || callbacks->closed == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        *handle = new NetPeerClientOpaque(*callbacks);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::clientDestroy(NetPeerClientHandle handle)
{
    if (handle == nullptr)
    {
        return NI_OK;
    }
    if (!handle->inactive())
    {
        return NI_INVALID_STATE;
    }
    delete handle;
    return NI_OK;
}

int32_t NetPeer::clientConfigureConnectionPools(NetPeerClientHandle handle, const NetConnectionPoolConfig& config)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (!handle->inactive())
    {
        return NI_INVALID_STATE;
    }
    return handle->m_session.configureConnectionPools(config);
}

int32_t NetPeer::clientConfigureRequestPool(NetPeerClientHandle handle, const LockFreeHashMapPoolConfig& config)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (!handle->inactive())
    {
        return NI_INVALID_STATE;
    }
    return handle->m_session.configureRequestPool(config);
}

int32_t NetPeer::clientConfigureSubmissionPools(NetPeerClientHandle handle, const NetSenderSubmissionPools& pools)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (!handle->inactive())
    {
        return NI_INVALID_STATE;
    }
    return handle->m_dispatch.submission().configurePools(pools);
}

int32_t NetPeer::clientConfigureCompression(NetPeerClientHandle handle, const uint32_t* types, uint32_t count)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (!handle->inactive())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return handle->m_session.configureCompression(types, count);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::clientConnect(NetPeerClientHandle handle, const NetPeerClientOptions* options)
{
    if (handle == nullptr || options == nullptr || options->structSize != sizeof(NetPeerClientOptions) || options->version != 1)
    {
        return NI_INVALID_ARGUMENT;
    }
    return handle->connect(*options, 0);
}

int32_t NetPeer::clientConnectService(NetPeerClientHandle handle, uint32_t product, const NetPeerClientOptions* options)
{
    if (product == 0 || product > INT32_MAX || handle == nullptr || options == nullptr ||
        options->structSize != sizeof(NetPeerClientOptions) || options->version != 1)
    {
        return NI_INVALID_ARGUMENT;
    }
    return handle->connect(*options, product);
}

int32_t NetPeer::clientConfigureGateways(NetPeerClientHandle handle, const char* seeds, const char* cachePath)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (!handle->inactive())
    {
        return NI_INVALID_STATE;
    }
    if (cachePath != nullptr && std::strlen(cachePath) > 4096)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return handle->m_session.configureGateways(seeds == nullptr ? "" : seeds, cachePath == nullptr ? "" : cachePath);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::clientPoll(NetPeerClientHandle handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_callbackDepth != 0 || handle->m_polling || handle->m_dispatch.executing())
    {
        return NI_INVALID_STATE;
    }
    handle->m_polling = true;
    int32_t result = NI_OK;
    try
    {
        result = handle->m_dispatch.poll() ? NI_READY : NI_OK;
    }
    catch (...)
    {
        handle->m_session.close(NI_INVALID_STATE);
        result = NI_NO_MEMORY;
    }
    handle->m_polling = false;
    return result;
}

int32_t NetPeer::clientBegin(NetPeerClientHandle handle, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t* requestId)
{
    if (requestId != nullptr)
    {
        *requestId = 0;
    }
    if (handle == nullptr || requestId == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return handle->m_dispatch.submission().begin(product, type, timeoutMs, *requestId);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::clientPost(NetPeerClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return handle->m_dispatch.submission().send(requestId, bytes, size);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::clientCancel(NetPeerClientHandle handle, uint64_t requestId)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return handle->m_dispatch.submission().cancel(requestId);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::clientPostData(NetPeerClientHandle handle, const NetInterfaceFrame* frame)
{
    if (handle == nullptr || frame == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return handle->m_dispatch.submission().data(*frame);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::clientClose(NetPeerClientHandle handle, int32_t reason)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    handle->m_dispatch.submission().close(reason);
    return NI_OK;
}

int32_t NetPeer::clientState(NetPeerClientHandle handle, uint64_t* generation, int32_t* ready)
{
    if (generation != nullptr)
    {
        *generation = 0;
    }
    if (ready != nullptr)
    {
        *ready = 0;
    }
    if (handle == nullptr || generation == nullptr || ready == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    bool available = false;
    handle->m_dispatch.submission().state(*generation, available);
    *ready = available ? 1 : 0;
    return NI_OK;
}

uint32_t NetPeer::pointerBytes()
{
    return static_cast<uint32_t>(sizeof(void*));
}

/** 同步交付预留请求，不等待远端回复
@param [in] handle 本提供方句柄
@param [in] requestId 当前预留号
@param [in] bytes size非零时不可空
@param [in] size 调用时复制的字节数
@return 已交付或负数失败，不表示远端收到
*/
int32_t NetPeer::clientSend(NetPeerClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_callbackDepth != 0 || handle->m_polling || handle->m_dispatch.executing())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return handle->m_dispatch.sendRequest(requestId, bytes, size);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

/** 同步交付流帧，不等待远端回复
@param [in] handle 本提供方句柄
@param [in] frame 调用时复制的帧及载荷
@return 已交付或负数失败，不表示远端收到
*/
int32_t NetPeer::clientSendData(NetPeerClientHandle handle, const NetInterfaceFrame* frame)
{
    if (handle == nullptr || frame == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_callbackDepth != 0 || handle->m_polling || handle->m_dispatch.executing())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return handle->m_dispatch.sendData(*frame);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::clientSendBegin(NetPeerClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t* ticket)
{
    if (ticket != nullptr)
    {
        *ticket = 0;
    }
    if (handle == nullptr || ticket == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_callbackDepth != 0 || handle->m_polling || handle->m_dispatch.executing())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        const int32_t capability = handle->m_session.outputState();
        return capability < 0 ? capability : handle->m_dispatch.submission().sendSynchronous(requestId, bytes, size, *ticket);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::clientSendDataBegin(NetPeerClientHandle handle, const NetInterfaceFrame* frame, uint64_t* ticket)
{
    if (ticket != nullptr)
    {
        *ticket = 0;
    }
    if (handle == nullptr || frame == nullptr || ticket == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_callbackDepth != 0 || handle->m_polling || handle->m_dispatch.executing())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        const int32_t capability = handle->m_session.outputState();
        return capability < 0 ? capability : handle->m_dispatch.submission().dataSynchronous(*frame, *ticket);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::clientSendResult(NetPeerClientHandle handle, uint64_t ticket)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        return handle->m_dispatch.submission().synchronousResult(ticket, true);
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

int32_t NetPeer::createClient(NetPeerSenderState*& client)
{
    if (client != nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        client = new NetPeerSenderState();
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::prepareGateway(NetPeerSenderState* client)
{
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        if (client->m_client == nullptr)
        {
            const NetPeerClientCallbacks callbacks = client->callbacks();
            client->m_lastError = NetPeer::instance().clientCreate(&callbacks, &client->m_client);
        }
        else
        {
            client->m_lastError = client->m_client->m_dispatch.executing() ? NI_INVALID_STATE : NI_OK;
        }
        return client->m_lastError;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::connectGateway(NetPeerSenderState* client, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    if (client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = client->connect(0, options, listener);
    return client->m_lastError;
}

int32_t NetPeer::connectServiceGateway(NetPeerSenderState* client, uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    if (client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = product == 0 ? NI_INVALID_ARGUMENT : client->connect(product, options, listener);
    return client->m_lastError;
}

int32_t NetPeer::configureConnectionPools(NetPeerSenderState* client, const NetConnectionPoolConfig& config)
{
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = clientConfigureConnectionPools(client->m_client, config);
    client->m_lastError = result;
    return result;
}

int32_t NetPeer::configureRequestPool(NetPeerSenderState* client, const LockFreeHashMapPoolConfig& config)
{
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = clientConfigureRequestPool(client->m_client, config);
    client->m_lastError = result;
    return result;
}

int32_t NetPeer::configureSubmissionPools(NetPeerSenderState* client, const NetSenderSubmissionPools& pools)
{
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = clientConfigureSubmissionPools(client->m_client, pools);
    client->m_lastError = result;
    return result;
}

int32_t NetPeer::configureCompression(NetPeerSenderState* client, const uint32_t* types, uint32_t count)
{
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientConfigureCompression(client->m_client, types, count);
    return client->m_lastError;
}

int32_t NetPeer::beginRequest(NetPeerSenderState* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId)
{
    requestId = 0;
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientBegin(client->m_client, product, type, timeoutMs, &requestId);
    return client->m_lastError;
}

int32_t NetPeer::postRequest(NetPeerSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientPost(client->m_client, requestId, bytes, size);
    return client->m_lastError;
}

int32_t NetPeer::cancelRequest(NetPeerSenderState* client, uint64_t requestId)
{
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientCancel(client->m_client, requestId);
    return client->m_lastError;
}

int32_t NetPeer::postData(NetPeerSenderState* client, const NetInterfaceFrame& frame)
{
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientPostData(client->m_client, &frame);
    return client->m_lastError;
}

int32_t NetPeer::sendRequest(NetPeerSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientSend(client->m_client, requestId, bytes, size);
    return client->m_lastError;
}

int32_t NetPeer::sendData(NetPeerSenderState* client, const NetInterfaceFrame& frame)
{
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientSendData(client->m_client, &frame);
    return client->m_lastError;
}

int32_t NetPeer::pollGateway(NetPeerSenderState* client)
{
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (client->m_client == nullptr)
    {
        return NI_OK;
    }
    const int32_t result = clientPoll(client->m_client);
    if (result < 0)
    {
        client->m_lastError = result;
    }
    return result;
}

void NetPeer::closeGateway(NetPeerSenderState* client, int32_t reason)
{
    if (client != nullptr && client->m_client != nullptr)
    {
        NetPeer::instance().clientClose(client->m_client, reason);
    }
}

void NetPeer::uninitGateway(NetPeerSenderState* client)
{
    closeGateway(client, 0);
}

int32_t NetPeer::destroyClient(NetPeerSenderState*& client)
{
    if (client == nullptr)
    {
        return NI_OK;
    }
    client->m_lastError = NetPeer::instance().clientDestroy(client->m_client);
    if (client->m_lastError != NI_OK)
    {
        return client->m_lastError;
    }
    delete client;
    client = nullptr;
    return NI_OK;
}

bool NetPeer::gatewayReady(NetPeerSenderState* client) const
{
    uint64_t generation = 0;
    int32_t ready = 0;
    if (client != nullptr)
    {
        NetPeer::instance().clientState(client->m_client, &generation, &ready);
    }
    return ready != 0;
}

uint64_t NetPeer::gatewayGeneration(NetPeerSenderState* client) const
{
    uint64_t generation = 0;
    int32_t ready = 0;
    if (client != nullptr)
    {
        NetPeer::instance().clientState(client->m_client, &generation, &ready);
    }
    return generation;
}

int32_t NetPeer::lastGatewayError(NetPeerSenderState* client) const
{
    return client == nullptr ? NI_INVALID_STATE : client->m_lastError;
}

int32_t NetPeer::randomBytes(uint8_t* output, uint32_t size)
{
    return NetHostSocket::randomBytes(output, size);
}

int32_t NetPeer::configureHost(const NetHostFunctions& functions)
{
    return NetHostSocket::configureDefault(functions);
}

int32_t NetPeer::gatewayState(NetPeerSenderState* client, uint64_t& generation, bool& ready)
{
    generation = 0;
    ready = false;
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (client->m_client == nullptr)
    {
        return NI_OK;
    }
    int32_t value = 0;
    const int32_t result = clientState(client->m_client, &generation, &value);
    ready = value != 0;
    return result;
}

int32_t NetPeer::beginSendRequest(NetPeerSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t& ticket)
{
    ticket = 0;
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return clientSendBegin(client->m_client, requestId, bytes, size, &ticket);
}

int32_t NetPeer::beginSendData(NetPeerSenderState* client, const NetInterfaceFrame& frame, uint64_t& ticket)
{
    ticket = 0;
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return clientSendDataBegin(client->m_client, &frame, &ticket);
}

int32_t NetPeer::sendResult(NetPeerSenderState* client, uint64_t ticket)
{
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return clientSendResult(client->m_client, ticket);
}

int32_t NetPeer::configureGateways(NetPeerSenderState* client, const char* seeds, const char* cachePath)
{
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = clientConfigureGateways(client->m_client, seeds, cachePath);
    client->m_lastError = result;
    return result;
}

NetFileProtocol& NetPeer::fileProtocol()
{
    static NetFileProtocol s_protocol;
    return s_protocol;
}

NetPeerBackend& NetPeer::backend()
{
    static NetPeerBackend s_backend;
    return s_backend;
}

int32_t NetPeer::configureWorkers(uint32_t count)
{
    return count == 0 ? NI_INVALID_ARGUMENT : (count == 1 ? NI_OK : NI_NOT_SUPPORTED);
}

int32_t NetPeer::configureDispatch(NetPeerSenderState* client, bool parallel)
{
    (void)parallel;
    if (client == nullptr) { return NI_INVALID_ARGUMENT; }
    return gatewayGeneration(client) == 0 ? NI_OK : NI_INVALID_STATE;
}