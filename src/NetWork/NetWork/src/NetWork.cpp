#include "NetWorkMacro.h"
#include "NetWorkClientState.h"
#include "NetWorkWorker.h"
#include "NetWork.h"
#include "NetWorkSenderState.h"
#include "NetWorkHostBridge.h"

#include "NetHostSocket.h"
#include <memory>
#include <new>

NetWork::NetWork()
{

}

NetWork::~NetWork()
{

}

NetWork& NetWork::instance()
{
    static NetWork s_instance;
    return s_instance;
}

int32_t NetWork::clientCreate(const NetWorkClientCallbacks* callbacks, NetWorkClientHandle* handle)
{
    if (handle != nullptr)
    {
        *handle = nullptr;
    }
    if (handle == nullptr || callbacks == nullptr || callbacks->structSize != sizeof(*callbacks) || callbacks->version != 1 ||
        callbacks->ready == nullptr || callbacks->result == nullptr || callbacks->frame == nullptr || callbacks->closed == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::unique_ptr<NetWorkClientOpaque> client(new NetWorkClientOpaque(*callbacks));

        NetInterfaceClientCallbacks table = *callbacks;
        table.context = client.get();
        table.ready = NetWorkClientOpaque::ready;
        table.result = NetWorkClientOpaque::result;
        table.frame = NetWorkClientOpaque::frame;
        table.closed = NetWorkClientOpaque::closed;
        int32_t result = client->m_error.load();
#ifndef __EMSCRIPTEN__
        if (result == NI_OK)
        {
            result = NetWork::configureNativeFileIo();
        }
#endif
        if (result == NI_OK)
        {
            result = NetPeer::instance().clientCreate(&table, &client->m_peer);
        }
        if (result == NI_OK)
        {
            result = NetPeerClientBackend::bindSubmission(client->m_peer, &client->m_submission);
        }
        if (result != NI_OK)
        {
            NetPeer::instance().clientDestroy(client->m_peer);
            return result;
        }
        *handle = client.release();
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetWork::clientDestroy(NetWorkClientHandle handle)
{
    try
    {
        if (handle == nullptr)
        {
            return NI_OK;
        }
        // 这里只回收已完成任务；活跃时不等待，且禁止在自己的回调中join。
        if (handle->m_running.load() || handle->onOwner())
        {
            return NI_INVALID_STATE;
        }
        const int32_t reaped = handle->reap();
        if (reaped != NI_OK)
        {
            return reaped;
        }
        int32_t destroyed = NI_OK;
        {
            destroyed = handle->m_worker ? handle->m_worker->invoke([&]() { return NetPeer::instance().clientDestroy(handle->m_peer); }) :
                NetPeer::instance().clientDestroy(handle->m_peer);
        }
        if (destroyed != NI_OK)
        {
            return destroyed;
        }
        delete handle;
        return NI_OK;
    }
    catch (const std::bad_alloc&)
    {
        return NI_NO_MEMORY;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

int32_t NetWork::clientConfigureConnectionPools(NetWorkClientHandle handle, const NetConnectionPoolConfig& config)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_running.load() || handle->onOwner())
    {
        return NI_INVALID_STATE;
    }
    return NetPeer::instance().clientConfigureConnectionPools(handle->m_peer, config);
}

int32_t NetWork::clientConfigureRequestPool(NetWorkClientHandle handle, const LockFreeHashMapPoolConfig& config)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_running.load() || handle->onOwner())
    {
        return NI_INVALID_STATE;
    }
    return NetPeer::instance().clientConfigureRequestPool(handle->m_peer, config);
}

int32_t NetWork::clientConfigureSubmissionPools(NetWorkClientHandle handle, const NetSenderSubmissionPools& pools)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_running.load() || handle->onOwner())
    {
        return NI_INVALID_STATE;
    }
    return NetPeer::instance().clientConfigureSubmissionPools(handle->m_peer, pools);
}

int32_t NetWork::clientConfigureCompression(NetWorkClientHandle handle, const uint32_t* types, uint32_t count)
{
    try
    {
        if (handle == nullptr)
        {
            return NI_INVALID_ARGUMENT;
        }
        if (handle->m_running.load())
        {
            return NI_INVALID_STATE;
        }
        return NetPeer::instance().clientConfigureCompression(handle->m_peer, types, count);
    }
    catch (const std::bad_alloc&)
    {
        return NI_NO_MEMORY;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

int32_t NetWork::clientConfigureGateways(NetWorkClientHandle handle, const char* seeds, const char* cachePath)
{
    try
    {
        if (handle == nullptr)
        {
            return NI_INVALID_ARGUMENT;
        }
        if (handle->m_running.load())
        {
            return NI_INVALID_STATE;
        }
        return NetPeer::instance().clientConfigureGateways(handle->m_peer, seeds, cachePath);
    }
    catch (const std::bad_alloc&)
    {
        return NI_NO_MEMORY;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

/** 静止生命期owner接纳连接，再把唯一Peer所有权交给CTask
@param [in] handle 已验证句柄
@param [in] product 0为旧直连，非0为逻辑服务
@param [in] options 调用时借用的配置
@return OK或明确未接纳错误
*/
static int32_t NetWorkConnect(NetWorkClientHandle handle, uint32_t product, const NetWorkClientOptions* options)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t reaped = handle->reap();
    if (reaped != NI_OK)
    {
        return reaped;
    }
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    // NetWork 的协议执行者需要独立线程；单线程宿主通过 NetPeer 驱动会话。
    (void)product;
    (void)options;
    return NI_NOT_SUPPORTED;
#else
    if (!handle->m_worker) { handle->m_worker = NetWorkWorker::acquire(handle->m_parallel); }
    if (!handle->m_worker) { return NI_NO_MEMORY; }
    return handle->m_worker->invoke([&]() {
        const int32_t prepared = handle->m_worker->prepareClient();
        if (prepared != NI_OK) { return prepared; }
        const int32_t connected = product == 0 ? NetPeer::instance().clientConnect(handle->m_peer, options) :
            NetPeer::instance().clientConnectService(handle->m_peer, product, options);
        if (connected != NI_OK) { return connected; }
        const int32_t started = handle->start();
        if (started != NI_OK)
        {
            NetPeer::instance().clientClose(handle->m_peer, started);
            NetPeer::instance().clientPoll(handle->m_peer);
        }
        return started;
    });
#endif
}

int32_t NetWork::clientConnect(NetWorkClientHandle handle, const NetWorkClientOptions* options)
{
    try
    {
        return NetWorkConnect(handle, 0, options);
    }
    catch (const std::bad_alloc&)
    {
        return NI_NO_MEMORY;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

int32_t NetWork::clientConnectService(NetWorkClientHandle handle, uint32_t product, const NetWorkClientOptions* options)
{
    try
    {
        if (product == 0 || product > INT32_MAX)
        {
            return NI_INVALID_ARGUMENT;
        }
        return NetWorkConnect(handle, product, options);
    }
    catch (const std::bad_alloc&)
    {
        return NI_NO_MEMORY;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

int32_t NetWork::clientPoll(NetWorkClientHandle handle)
{
    try
    {
        if (handle == nullptr)
        {
            return NI_INVALID_ARGUMENT;
        }
        if (handle->onOwner())
        {
            return NI_INVALID_STATE;
        }
        // 观察不会调用Peer；定时推进和回调不依赖调用者继续poll。
        return handle->m_running.load() ? NI_READY : NI_OK;
    }
    catch (const std::bad_alloc&)
    {
        return NI_NO_MEMORY;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

int32_t NetWork::clientBegin(NetWorkClientHandle handle, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t* requestId)
{
    try
    {
        if (requestId != nullptr)
        {
            *requestId = 0;
        }
        if (handle == nullptr || requestId == nullptr)
        {
            return NI_INVALID_ARGUMENT;
        }
        return handle->m_submission.begin(product, type, timeoutMs, *requestId);
    }
    catch (const std::bad_alloc&)
    {
        return NI_NO_MEMORY;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

int32_t NetWork::clientPost(NetWorkClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    try
    {
        return handle == nullptr ? NI_INVALID_ARGUMENT : handle->m_submission.send(requestId, bytes, size);
    }
    catch (const std::bad_alloc&)
    {
        return NI_NO_MEMORY;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

int32_t NetWork::clientCancel(NetWorkClientHandle handle, uint64_t requestId)
{
    try
    {
        return handle == nullptr ? NI_INVALID_ARGUMENT : handle->m_submission.cancel(requestId);
    }
    catch (const std::bad_alloc&)
    {
        return NI_NO_MEMORY;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

int32_t NetWork::clientPostData(NetWorkClientHandle handle, const NetInterfaceFrame* frame)
{
    try
    {
        return handle == nullptr || frame == nullptr ? NI_INVALID_ARGUMENT : handle->m_submission.data(*frame);
    }
    catch (const std::bad_alloc&)
    {
        return NI_NO_MEMORY;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

int32_t NetWork::clientClose(NetWorkClientHandle handle, int32_t reason)
{
    try
    {
        if (handle == nullptr)
        {
            return NI_INVALID_ARGUMENT;
        }
        handle->m_submission.close(reason);
        return NI_OK;
    }
    catch (const std::bad_alloc&)
    {
        return NI_NO_MEMORY;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

int32_t NetWork::clientState(NetWorkClientHandle handle, uint64_t* generation, int32_t* ready)
{
    try
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
        bool value = false;
        handle->m_submission.state(*generation, value);
        *ready = value ? 1 : 0;
        return NI_OK;
    }
    catch (const std::bad_alloc&)
    {
        return NI_NO_MEMORY;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

uint32_t NetWork::pointerBytes()
{
    return static_cast<uint32_t>(sizeof(void*));
}

int32_t NetWork::createClient(NetWorkSenderState*& client)
{
    if (client != nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        client = new NetWorkSenderState();
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetWork::prepareGateway(NetWorkSenderState* client)
{
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        if (client->m_client != nullptr)
        {
            client->m_lastError = client->m_client->onOwner() ? NI_INVALID_STATE : NI_OK;
            return client->m_lastError.load(std::memory_order_relaxed);
        }
        const NetInterfaceClientCallbacks callbacks = client->callbacks();
        client->m_lastError = NetWork::instance().clientCreate(&callbacks, &client->m_client);
        return client->m_lastError.load(std::memory_order_relaxed);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

/** 在生命期owner上验证可更换监听者，不阻塞等待工作线程
@param [in] state 已准备适配或空
@param [in] options 完整连接配置
@param [in] listener 借用至连接及回调收敛
@return OK或明确错误
*/
static int32_t NetWorkSenderPrepareConnect(NetWorkSenderState* state, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    if (state == nullptr || state->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    if (listener == nullptr || options.m_localGatewayPid != 0 || options.m_network.structSize != sizeof(NetInterfaceClientOptions) || options.m_network.version != 1)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (NetWork::instance().clientPoll(state->m_client) != NI_OK)
    {
        return NI_INVALID_STATE;
    }
    state->m_listener = listener;
    return NI_OK;
}

int32_t NetWork::connectGateway(NetWorkSenderState* client, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    int32_t result = NetWorkSenderPrepareConnect(client, options, listener);
    if (result == NI_OK)
    {
        result = NetWork::instance().clientConnect(client->m_client, &options.m_network);
    }
    if (client != nullptr)
    {
        client->m_lastError.store(result, std::memory_order_relaxed);
    }
    return result;
}

int32_t NetWork::connectServiceGateway(NetWorkSenderState* client, uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    int32_t result = NetWorkSenderPrepareConnect(client, options, listener);
    if (result == NI_OK)
    {
        result = NetWork::instance().clientConnectService(client->m_client, product, &options.m_network);
    }
    if (client != nullptr)
    {
        client->m_lastError.store(result, std::memory_order_relaxed);
    }
    return result;
}

int32_t NetWork::configureConnectionPools(NetWorkSenderState* client, const NetConnectionPoolConfig& config)
{
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = clientConfigureConnectionPools(client->m_client, config);
    client->m_lastError.store(result, std::memory_order_relaxed);
    return result;
}

int32_t NetWork::configureRequestPool(NetWorkSenderState* client, const LockFreeHashMapPoolConfig& config)
{
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = clientConfigureRequestPool(client->m_client, config);
    client->m_lastError.store(result, std::memory_order_relaxed);
    return result;
}

int32_t NetWork::configureSubmissionPools(NetWorkSenderState* client, const NetSenderSubmissionPools& pools)
{
    if (client == nullptr || client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = clientConfigureSubmissionPools(client->m_client, pools);
    client->m_lastError.store(result, std::memory_order_relaxed);
    return result;
}

int32_t NetWork::configureCompression(NetWorkSenderState* client, const uint32_t* types, uint32_t count)
{
    const int32_t result = client == nullptr || client->m_client == nullptr ? NI_INVALID_STATE : NetWork::instance().clientConfigureCompression(client->m_client, types, count);
    if (client != nullptr)
    {
        client->m_lastError.store(result, std::memory_order_relaxed);
    }
    return result;
}

int32_t NetWork::pollGateway(NetWorkSenderState* client)
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
        client->m_lastError.store(result, std::memory_order_relaxed);
    }
    return result;
}

int32_t NetWork::beginRequest(NetWorkSenderState* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& id)
{
    id = 0;
    const int32_t result = client == nullptr ? NI_INVALID_STATE : NetWork::instance().clientBegin(client->m_client, product, type, timeoutMs, &id);
    if (client != nullptr)
    {
        client->m_lastError.store(result, std::memory_order_relaxed);
    }
    return result;
}

int32_t NetWork::postRequest(NetWorkSenderState* client, uint64_t id, const uint8_t* bytes, uint32_t size)
{
    const int32_t result = client == nullptr ? NI_INVALID_STATE : NetWork::instance().clientPost(client->m_client, id, bytes, size);
    if (client != nullptr)
    {
        client->m_lastError.store(result, std::memory_order_relaxed);
    }
    return result;
}

int32_t NetWork::cancelRequest(NetWorkSenderState* client, uint64_t id)
{
    const int32_t result = client == nullptr ? NI_INVALID_STATE : NetWork::instance().clientCancel(client->m_client, id);
    if (client != nullptr)
    {
        client->m_lastError.store(result, std::memory_order_relaxed);
    }
    return result;
}

int32_t NetWork::postData(NetWorkSenderState* client, const NetInterfaceFrame& frame)
{
    const int32_t result = client == nullptr ? NI_INVALID_STATE : NetWork::instance().clientPostData(client->m_client, &frame);
    if (client != nullptr)
    {
        client->m_lastError.store(result, std::memory_order_relaxed);
    }
    return result;
}

void NetWork::closeGateway(NetWorkSenderState* client, int32_t reason)
{
    if (client != nullptr)
    {
        NetWork::instance().clientClose(client->m_client, reason);
    }
}

void NetWork::uninitGateway(NetWorkSenderState* client)
{
    closeGateway(client, 0);
}

int32_t NetWork::destroyClient(NetWorkSenderState*& client)
{
    if (client == nullptr)
    {
        return NI_OK;
    }
    const int32_t result = NetWork::instance().clientDestroy(client->m_client);
    if (result != NI_OK)
    {
        client->m_lastError.store(result, std::memory_order_relaxed); return result;
    }
    delete client;
    client = nullptr;
    return NI_OK;
}

bool NetWork::gatewayReady(NetWorkSenderState* client) const
{
    uint64_t generation = 0; int32_t ready = 0;
    if (client != nullptr)
    {
        NetWork::instance().clientState(client->m_client, &generation, &ready);
    }
    return ready != 0;
}

uint64_t NetWork::gatewayGeneration(NetWorkSenderState* client) const
{
    uint64_t generation = 0; int32_t ready = 0;
    if (client != nullptr)
    {
        NetWork::instance().clientState(client->m_client, &generation, &ready);
    }
    return generation;
}

int32_t NetWork::lastGatewayError(NetWorkSenderState* client) const
{
    return client == nullptr ? NI_INVALID_STATE : client->m_lastError.load(std::memory_order_relaxed);
}

int32_t NetWork::sendRequest(NetWorkSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = NetWork::instance().clientSend(client->m_client, requestId, bytes, size);
    client->m_lastError.store(result, std::memory_order_relaxed);
    return result;
}

int32_t NetWork::sendData(NetWorkSenderState* client, const NetInterfaceFrame& frame)
{
    if (client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = NetWork::instance().clientSendData(client->m_client, &frame);
    client->m_lastError.store(result, std::memory_order_relaxed);
    return result;
}

int32_t NetWork::randomBytes(uint8_t* output, uint32_t size)
{
    return NetPeer::randomBytes(output, size);
}

int32_t NetWork::configureNativeFileIo()
{
#ifdef __EMSCRIPTEN__
    // 浏览器没有原生文件执行者；文件能力由宿主显式提供，不能伪造登记成功。
    return NI_NOT_SUPPORTED;
#else
    // 注册已经完成表示宿主自选后端；不能用原生默认实现覆盖它。
    const int32_t result = NetFileIo::configureBackend(NetNativeFileIo::create);
    return result == NI_INVALID_STATE ? NI_OK : result;
#endif
}

int32_t NetWork::configureFilePools(const NetWorkFilePools& pools)
{
#ifdef __EMSCRIPTEN__
    (void)pools;
    return NI_NOT_SUPPORTED;
#else
    return NetNativeFileIo::configurePools(pools);
#endif
}

int32_t NetWork::configureHostPools(const NetWorkHostPools& pools)
{
    return NetWorkHostBridge::configurePools(pools);
}

int32_t NetWork::configureHost(const NetHostFunctions& functions)
{
    const int32_t result = NetWorkHostBridge::configure(&functions);
    if (result != NI_OK)
    {
        return result;
    }
#ifdef __EMSCRIPTEN__
    return NI_OK;
#else
    return configureNativeFileIo();
#endif
}



/** 并发提交预留请求，由所属会话owner按FIFO组包加密，不等待远端回复
@param [in] handle 本提供方句柄
@param [in] requestId 当前预留号
@param [in] bytes size非零时不可空
@param [in] size 调用时复制的字节数
@return 本地已接纳或负数失败，不表示已发送或远端收到
*/
int32_t NetWork::clientSend(NetWorkClientHandle handle, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->onOwner())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return handle->m_submission.send(requestId, bytes, size);
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

/** 并发提交流帧，由所属会话owner按FIFO组包加密，不等待远端回复
@param [in] handle 本提供方句柄
@param [in] frame 调用时复制的帧及载荷
@return 本地已接纳或负数失败，不表示已发送或远端收到
*/
int32_t NetWork::clientSendData(NetWorkClientHandle handle, const NetInterfaceFrame* frame)
{
    if (handle == nullptr || frame == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->onOwner())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return handle->m_submission.data(*frame);
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}



int32_t NetWork::gatewayState(NetWorkSenderState* client, uint64_t& generation, bool& ready)
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

int32_t NetWork::beginSendRequest(NetWorkSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t& ticket)
{
    ticket = 0;
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    if (client->m_client->onOwner())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return client->m_client->m_submission.sendSynchronous(requestId, bytes, size, ticket);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetWork::beginSendData(NetWorkSenderState* client, const NetInterfaceFrame& frame, uint64_t& ticket)
{
    ticket = 0;
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    if (client->m_client->onOwner())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return client->m_client->m_submission.dataSynchronous(frame, ticket);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetWork::sendResult(NetWorkSenderState* client, uint64_t ticket)
{
    if (client == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (client->m_client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return client->m_client->m_submission.synchronousResult(ticket, true);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetWork::configureGateways(NetWorkSenderState* client, const char* seeds, const char* cachePath)
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
    client->m_lastError.store(result, std::memory_order_relaxed);
    return result;
}

NetFileProtocol& NetWork::fileProtocol()
{
    return NetPeer::instance().fileProtocol();
}

NetWorkBackend& NetWork::backend()
{
    static NetWorkBackend s_backend;
    return s_backend;
}

int32_t NetWork::configureWorkers(uint32_t count)
{
    return NetWorkWorker::configure(count);
}

int32_t NetWork::configureDispatch(NetWorkSenderState* client, bool parallel)
{
    return client == nullptr ? NI_INVALID_ARGUMENT : clientConfigureDispatch(client->m_client, parallel);
}

int32_t NetWork::clientConfigureDispatch(NetWorkClientHandle client, bool parallel)
{
    if (client == nullptr) { return NI_INVALID_ARGUMENT; }
    if (client->m_worker || client->m_running.load()) { return NI_INVALID_STATE; }
    client->m_parallel = parallel;
    return NI_OK;
}
