#include "NetPeerMacro.h"
#include "NetPeer.h"
#include "NetPeerSenderState.h"
#include "NetPeerClientState.h"
#include <memory>
#include "CStringManager/CStringManagerAPI.h"

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
    try { return clientCreate(callbacks, handle, NetPeerConfig()); }
    catch (...) { if (handle != nullptr) { *handle = nullptr; } return NI_NO_MEMORY; }
}

int32_t NetPeer::clientCreate(const NetPeerClientCallbacks* callbacks, NetPeerClientHandle* handle, const NetPeerConfig& config)
{
    if (handle != nullptr)
    {
        *handle = nullptr;
    }
    if (handle == nullptr || callbacks == nullptr || callbacks->structSize != sizeof(NetPeerClientCallbacks) || callbacks->version != 1 ||
        callbacks->ready == nullptr || callbacks->result == nullptr || callbacks->frame == nullptr || callbacks->closed == nullptr || !config.valid())
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::unique_ptr<NetPeerClientOpaque> created(new NetPeerClientOpaque(*callbacks, config));
        if (config.m_configure != nullptr)
        {
            const int32_t result = config.m_configure(config.m_context, created->m_session, created->m_dispatch);
            if (result != NI_OK)
            {
                return result < NI_OK ? result : NI_INVALID_STATE;
            }
        }
        *handle = created.release();
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
    return clientConfigureSubmissionPools(handle, pools, NetSenderSubmissionOptions());
}

int32_t NetPeer::clientConfigureSubmissionPools(NetPeerClientHandle handle, const NetSenderSubmissionPools& pools,
    const NetSenderSubmissionOptions& options)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (!handle->inactive())
    {
        return NI_INVALID_STATE;
    }
    return handle->m_dispatch.submission().configurePools(pools, options);
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
    const size_t seedSize = CStringManager::boundedLength(seeds, 4097);
    const size_t pathSize = CStringManager::boundedLength(cachePath, 4097);
    if (seedSize > 4096 || pathSize > 4096) { return NI_INVALID_ARGUMENT; }
    try
    {
        return handle->m_session.configureGateways(std::string(seeds ? seeds : "", seedSize),
            std::string(cachePath ? cachePath : "", pathSize));
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

/** 通知当前单执行者，成功接纳的提交须由下一轮 poll 处理
@param [in] result 提交队列返回的结果
@return 原结果
*/
static int32_t NotifyPeerSubmission(int32_t result)
{
    if (result == NI_OK)
    {
        NetExecution::progress();
    }
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
        return NotifyPeerSubmission(handle->m_dispatch.submission().begin(product, type, timeoutMs, *requestId));
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
        return NotifyPeerSubmission(handle->m_dispatch.submission().send(requestId, bytes, size));
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
        return NotifyPeerSubmission(handle->m_dispatch.submission().cancel(requestId));
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
        return NotifyPeerSubmission(handle->m_dispatch.submission().data(*frame));
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
    NetSenderSubmission& submission = handle->m_dispatch.submission();
    int32_t previous = NI_OK;
    const bool firstClose = handle->m_session.generation() != 0 && !submission.closeRequested(previous);
    submission.close(reason);
    if (firstClose)
    {
        NetExecution::progress();
    }
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

/** 检查票据输出与输入的字节范围是否重叠
@param [in] ticket 非空票据输出地址
@param [in] input 输入地址，空指针表示空范围
@param [in] size 输入字节数
@return 两个范围含有公共字节时返回 true
*/
static bool TicketOverlaps(const uint64_t* ticket, const void* input, size_t size)
{
    if (input == nullptr || size == 0)
    {
        return false;
    }
    const uintptr_t outputAddress = reinterpret_cast<uintptr_t>(ticket);
    const uintptr_t inputAddress = reinterpret_cast<uintptr_t>(input);
    return outputAddress >= inputAddress ? outputAddress - inputAddress < size : inputAddress - outputAddress < sizeof(*ticket);
}

int32_t NetPeer::clientSendBegin(NetPeerClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t* ticket)
{
    if (ticket == nullptr || TicketOverlaps(ticket, bytes, size))
    {
        return NI_INVALID_ARGUMENT;
    }
    *ticket = 0;
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
        const int32_t capability = handle->m_session.outputState();
        return capability < 0 ? capability : NotifyPeerSubmission(handle->m_dispatch.submission().sendSynchronous(requestId, bytes, size, *ticket));
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetPeer::clientSendDataBegin(NetPeerClientHandle handle, const NetInterfaceFrame* frame, uint64_t* ticket)
{
    if (ticket == nullptr || (frame != nullptr && (TicketOverlaps(ticket, frame, sizeof(*frame)) ||
        TicketOverlaps(ticket, frame->m_payload, frame->m_payloadSize))))
    {
        return NI_INVALID_ARGUMENT;
    }
    *ticket = 0;
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
        const int32_t capability = handle->m_session.outputState();
        return capability < 0 ? capability : NotifyPeerSubmission(handle->m_dispatch.submission().dataSynchronous(*frame, *ticket));
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
    if (client->m_preparing)
    {
        client->m_lastError = NI_INVALID_STATE;
        return client->m_lastError;
    }
    if (client->m_client != nullptr)
    {
        client->m_lastError = client->m_client->m_dispatch.executing() ? NI_INVALID_STATE : NI_OK;
        return client->m_lastError;
    }
    try { return prepareGateway(client, NetPeerConfig()); }
    catch (...) { client->m_lastError = NI_NO_MEMORY; return client->m_lastError; }
}

int32_t NetPeer::prepareGateway(NetPeerSenderState* client, const NetPeerConfig& config)
{
    if (client == nullptr) { return NI_INVALID_ARGUMENT; }
    if (client->m_preparing || client->m_client != nullptr)
    {
        client->m_lastError = NI_INVALID_STATE;
        return client->m_lastError;
    }
    const NetPeerClientCallbacks callbacks = client->callbacks();
    client->m_preparing = true;
    client->m_lastError = clientCreate(&callbacks, &client->m_client, config);
    client->m_preparing = false;
    return client->m_lastError;
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
    if (!NetPeerSenderState::requirePrepared(client))
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = clientConfigureConnectionPools(client->m_client, config);
    client->m_lastError = result;
    return result;
}

int32_t NetPeer::configureRequestPool(NetPeerSenderState* client, const LockFreeHashMapPoolConfig& config)
{
    if (!NetPeerSenderState::requirePrepared(client))
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = clientConfigureRequestPool(client->m_client, config);
    client->m_lastError = result;
    return result;
}

int32_t NetPeer::configureSubmissionPools(NetPeerSenderState* client, const NetSenderSubmissionPools& pools)
{
    return configureSubmissionPools(client, pools, NetSenderSubmissionOptions());
}

int32_t NetPeer::configureSubmissionPools(NetPeerSenderState* client, const NetSenderSubmissionPools& pools,
    const NetSenderSubmissionOptions& options)
{
    if (!NetPeerSenderState::requirePrepared(client))
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = clientConfigureSubmissionPools(client->m_client, pools, options);
    client->m_lastError = result;
    return result;
}

int32_t NetPeer::configureCompression(NetPeerSenderState* client, const uint32_t* types, uint32_t count)
{
    if (!NetPeerSenderState::requirePrepared(client))
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientConfigureCompression(client->m_client, types, count);
    return client->m_lastError;
}

int32_t NetPeer::beginRequest(NetPeerSenderState* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId)
{
    requestId = 0;
    if (!NetPeerSenderState::requirePrepared(client))
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientBegin(client->m_client, product, type, timeoutMs, &requestId);
    return client->m_lastError;
}

int32_t NetPeer::postRequest(NetPeerSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (!NetPeerSenderState::requirePrepared(client))
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientPost(client->m_client, requestId, bytes, size);
    return client->m_lastError;
}

int32_t NetPeer::cancelRequest(NetPeerSenderState* client, uint64_t requestId)
{
    if (!NetPeerSenderState::requirePrepared(client))
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientCancel(client->m_client, requestId);
    return client->m_lastError;
}

int32_t NetPeer::postData(NetPeerSenderState* client, const NetInterfaceFrame& frame)
{
    if (!NetPeerSenderState::requirePrepared(client))
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientPostData(client->m_client, &frame);
    return client->m_lastError;
}

int32_t NetPeer::sendRequest(NetPeerSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (!NetPeerSenderState::requirePrepared(client))
    {
        return NI_INVALID_STATE;
    }
    client->m_lastError = NetPeer::instance().clientSend(client->m_client, requestId, bytes, size);
    return client->m_lastError;
}

int32_t NetPeer::sendData(NetPeerSenderState* client, const NetInterfaceFrame& frame)
{
    if (!NetPeerSenderState::requirePrepared(client))
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
    if (client->m_preparing)
    {
        client->m_lastError = NI_INVALID_STATE;
        return client->m_lastError;
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
    return clientSendBegin(client != nullptr ? client->m_client : nullptr, requestId, bytes, size, &ticket);
}

int32_t NetPeer::beginSendData(NetPeerSenderState* client, const NetInterfaceFrame& frame, uint64_t& ticket)
{
    return clientSendDataBegin(client != nullptr ? client->m_client : nullptr, &frame, &ticket);
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
    if (!NetPeerSenderState::requirePrepared(client))
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
    client->m_lastError = client->m_client != nullptr && client->m_client->inactive() &&
        gatewayGeneration(client) == 0 ? NI_OK : NI_INVALID_STATE;
    return client->m_lastError;
}

/* 使用示例。宿主驱动单执行者客户端，启动时配置字节宿主后可传入真实连接参数。
// 本文件已包含当前源码头；独立应用改为包含NetPeer/NetPeerAPI.h。
#include "NetPeer.h"
#include <cstdio>

// 在客户端发布前调用一次，返回负数可取消本次准备。
static int32_t Configure(void*, NetClientSession& session, NetSenderDispatch& dispatch)
{
    const int32_t result = session.configureRecovery(30000, 250, 5000);
    return result == NI_OK ? dispatch.configurePollBudgets(256, 131072) : result;
}

// 业务循环在宿主通知后推进。请求正文在成功post时复制。
static int32_t Advance(NetPeer& api, NetPeerSenderState* client)
{
    const int32_t polled = api.pollGateway(client);
    if (polled < 0) { return polled; }
    uint64_t generation = 0;
    bool ready = false;
    const int32_t result = api.gatewayState(client, generation, ready);
    if (result != NI_OK || !ready) { return result; }
    uint64_t id = 0;
    const int32_t reserved = api.beginRequest(client, 1, 7, 1000, id);
    if (reserved != NI_OK) { return reserved; }
    const uint8_t body[] = {1, 2, 3};
    const int32_t posted = api.postRequest(client, id, body, sizeof(body));
    if (posted != NI_OK) { api.cancelRequest(client, id); }
    return posted;
}

// 有字节宿主时，调用方可传入已配置的TLS身份和仍然存活的监听器。
static int32_t Run(NetPeer& api, NetPeerSenderState* client,
    const NetSenderGatewayOptions* options, NetSessionListener* listener)
{
    NetPeerConfig config;
    config.m_configure = Configure;
    config.m_direct.m_outputBudget = 32768;
    // 对端关闭字节流后，最多给接收方 3 秒消费积压的完整帧。
    config.m_direct.m_drainTimeoutMs = 3000;
    // 直连输入最多缓存 32768 字节密文，输出队列允许按需增长。
    if (!config.m_direct.m_ciphertext.set(32768, -1))
    {
        return NI_INVALID_ARGUMENT;
    }
    config.m_service.m_peers.setMaximumCapacity(-1);
    config.m_discovery.m_transport.m_peers.setInitialCapacity(4);
    config.m_discovery.m_retryMs = 500;
    // 直连宿主每轮最多派发 32 个原始事件，目录探测宿主使用自己的上限。
    config.m_direct.m_host.m_eventBudget = 32;
    config.m_discovery.m_transport.m_host.m_eventBudget = 8;
    // 服务中继每次推进16条，目录探测沿用自己的预算。
    config.m_service.m_relay.m_stepBudget = 16;
    // 单独设置服务管理帧的接收背压期限，目录发现连接使用自己的配置。
    config.m_service.m_backpressureMs = 10000;
    if (!config.valid()) { return NI_INVALID_ARGUMENT; }
    int32_t result = api.prepareGateway(client, config);
    if (result != NI_OK) { return result; }
    const uint32_t publicTypes[] = {7};
    NetConnectionPoolConfig connection;
    LockFreeHashMapPoolConfig requests;
    requests.setInitialCapacity(8);
    NetSenderSubmissionPools submission;
    NetSenderSubmissionOptions submissionOptions;
    submissionOptions.m_inputBatchSize = 4;
    result = api.configureConnectionPools(client, connection);
    if (result == NI_OK) { result = api.configureRequestPool(client, requests); }
    if (result == NI_OK) { result = api.configureSubmissionPools(client, submission, submissionOptions); }
    if (result == NI_OK) { result = api.configureCompression(client, publicTypes, 1); }
    if (result == NI_OK) { result = api.configureDispatch(client, false); }
    if (result == NI_OK) { result = api.configureGateways(client, "127.0.0.1:8000", nullptr); }
    if (result == NI_OK && options) { result = api.connectServiceGateway(client, 1, *options, listener); }
    return result == NI_OK ? Advance(api, client) : result;
}

// 已拥有回调表的宿主也可以直接使用句柄接口。
static int32_t RunHandle(NetPeer& api)
{
    NetPeerClientCallbacks events = {};
    events.structSize = sizeof(events);
    events.version = 1;
    events.ready = [](void*, uint64_t, uint32_t, uint32_t) {};
    events.result = [](void*, uint64_t, uint64_t, uint32_t, uint32_t, uint32_t, const uint8_t*, uint32_t) {};
    events.frame = [](void*, uint64_t, const NetInterfaceFrame*) -> int32_t { return 1; };
    events.closed = [](void*, uint64_t, int32_t, int32_t) {};
    NetPeerClientHandle handle = nullptr;
    int32_t result = api.clientCreate(&events, &handle);
    if (result != NI_OK) { return result; }
    NetConnectionPoolConfig connection;
    NetSenderSubmissionPools submission;
    LockFreeHashMapPoolConfig requests;
    requests.setInitialCapacity(8);
    requests.setMaximumCapacity(-1);
    result = api.clientConfigureConnectionPools(handle, connection);
    if (result == NI_OK) { result = api.clientConfigureSubmissionPools(handle, submission); }
    if (result == NI_OK) { result = api.clientConfigureRequestPool(handle, requests); }
    if (result == NI_OK) { result = api.clientConfigureGateways(handle, "127.0.0.1:8000", nullptr); }
    if (result == NI_OK) { result = api.clientConfigureCompression(handle, nullptr, 0); }
    uint64_t generation = 0;
    int32_t ready = 0;
    if (result == NI_OK) { result = api.clientState(handle, &generation, &ready); }
    // 离线句柄尚未握手，代次与就绪位均为零。
    if (result == NI_OK && (generation != 0 || ready != 0)) { result = NI_INVALID_STATE; }
    const int32_t closing = api.clientClose(handle, 0);
    const int32_t polled = api.clientPoll(handle);
    const int32_t released = api.clientDestroy(handle);
    if (result < 0) { return result; }
    if (closing < 0) { return closing; }
    return polled < 0 ? polled : released;
}

int main()
{
    NetPeer& api = NetPeer::instance();
    // 混用不同位宽的宿主和库会使句柄失效，接入时先核对 ABI。
    if (api.pointerBytes() != sizeof(void*))
    {
        return 1;
    }
    if (api.configureWorkers(1) != NI_OK || RunHandle(api) != NI_OK) { return 1; }
    NetPeerSenderState* client = nullptr;
    if (api.createClient(client) != NI_OK) { return 1; }
    const int32_t result = Run(api, client, nullptr, nullptr);
    std::printf("generation %llu, ready %d, status %d\n",
        static_cast<unsigned long long>(api.gatewayGeneration(client)), api.gatewayReady(client) ? 1 : 0,
        static_cast<int>(api.lastGatewayError(client)));
    // uninitGateway 等价于 closeGateway(client, 0)，任选一个发起关闭。
    api.closeGateway(client);
    // 已连接的宿主在后续事件中重复poll并在收敛后销毁；本例运行离线配置路径。
    const int32_t stopped = api.pollGateway(client);
    const int32_t freed = api.destroyClient(client);
    return result == NI_OK && stopped == NI_OK && freed == NI_OK ? 0 : 1;
}
*/