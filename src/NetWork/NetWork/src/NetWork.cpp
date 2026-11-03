#include "NetWorkMacro.h"
#include "NetWorkClientState.h"
#include "NetWork.h"
#include "NetWorkSenderState.h"
#include "NetWorkHostBridge.h"

#include "NetHostSocket.h"
#include <CSystem/CSystemAPI.h>
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
        const int32_t destroyed = NetPeer::instance().clientDestroy(handle->m_peer);
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
    const int32_t connected = product == 0 ? NetPeer::instance().clientConnect(handle->m_peer, options) : NetPeer::instance().clientConnectService(handle->m_peer, product, options);
    if (connected != NI_OK)
    {
        return connected;
    }
    const int32_t started = handle->start();
    if (started != NI_OK)
    {
        NetPeer::instance().clientClose(handle->m_peer, started);
        NetPeer::instance().clientPoll(handle->m_peer);
    }
    return started;
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
            return client->m_lastError.load();
        }
        const NetInterfaceClientCallbacks callbacks = client->callbacks();
        client->m_lastError = NetWork::instance().clientCreate(&callbacks, &client->m_client);
        return client->m_lastError.load();
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
        client->m_lastError = result;
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
        client->m_lastError = result;
    }
    return result;
}

int32_t NetWork::configureCompression(NetWorkSenderState* client, const uint32_t* types, uint32_t count)
{
    const int32_t result = client == nullptr || client->m_client == nullptr ? NI_INVALID_STATE : NetWork::instance().clientConfigureCompression(client->m_client, types, count);
    if (client != nullptr)
    {
        client->m_lastError = result;
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
        client->m_lastError = result;
    }
    return result;
}

int32_t NetWork::beginRequest(NetWorkSenderState* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& id)
{
    id = 0;
    const int32_t result = client == nullptr ? NI_INVALID_STATE : NetWork::instance().clientBegin(client->m_client, product, type, timeoutMs, &id);
    if (client != nullptr)
    {
        client->m_lastError = result;
    }
    return result;
}

int32_t NetWork::postRequest(NetWorkSenderState* client, uint64_t id, const uint8_t* bytes, uint32_t size)
{
    const int32_t result = client == nullptr ? NI_INVALID_STATE : NetWork::instance().clientPost(client->m_client, id, bytes, size);
    if (client != nullptr)
    {
        client->m_lastError = result;
    }
    return result;
}

int32_t NetWork::cancelRequest(NetWorkSenderState* client, uint64_t id)
{
    const int32_t result = client == nullptr ? NI_INVALID_STATE : NetWork::instance().clientCancel(client->m_client, id);
    if (client != nullptr)
    {
        client->m_lastError = result;
    }
    return result;
}

int32_t NetWork::postData(NetWorkSenderState* client, const NetInterfaceFrame& frame)
{
    const int32_t result = client == nullptr ? NI_INVALID_STATE : NetWork::instance().clientPostData(client->m_client, &frame);
    if (client != nullptr)
    {
        client->m_lastError = result;
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
        client->m_lastError = result; return result;
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
    return client == nullptr ? NI_INVALID_STATE : client->m_lastError.load();
}

int32_t NetWork::sendRequest(NetWorkSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    if (client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = NetWork::instance().clientSend(client->m_client, requestId, bytes, size);
    client->m_lastError = result;
    return result;
}

int32_t NetWork::sendData(NetWorkSenderState* client, const NetInterfaceFrame& frame)
{
    if (client == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = NetWork::instance().clientSendData(client->m_client, &frame);
    client->m_lastError = result;
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

#if 0 // NET_WORK_THREAD_UNIT_BEGIN
#include "NetWorkThreadTest.h"
#include <LibuvTcp/LibuvTcpAPI.h>
#include <test/certs.h>
#include <cstdio>
#include <cstring>

int32_t NetWorkThreadTestSink::send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin)
{
    (void)origin;
    NetWorkThreadTestMessage copy = {};
    copy.m_endpoint = endpoint; copy.m_frame = frame;
    if (frame.m_payloadSize != 0) { copy.m_bytes.assign(frame.m_payload, frame.m_payload + frame.m_payloadSize); }
    m_messages.push_back(std::move(copy));
    return NI_OK;
}

void NetWorkThreadTestSink::close(uint64_t endpoint)
{
    (void)endpoint;
}

bool NetWorkThreadTestSink::respond(NetPeerEndpointRouter& router)
{
    for (size_t i = 0; i < m_messages.size(); ++i)
    {
        NetWorkThreadTestMessage& message = m_messages[i];
        if (message.m_frame.m_kind != NI_REQUEST) { continue; }
        message.m_frame.m_payload = message.m_bytes.data();
        NetInterfaceRpcView rpc = {};
        if (NetInterfaceRpc::parse(message.m_frame, rpc) != NI_OK || rpc.m_size != 1 || rpc.m_data[0] != 7)
        {
            return false;
        }
        std::vector<uint8_t> payload(17); uint32_t required = 0;
        if (NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, rpc.m_data, rpc.m_size, payload.data(), 17, required) != NI_OK)
        {
            return false;
        }
        message.m_frame.m_kind = NI_RESPONSE; message.m_frame.m_payload = payload.data(); message.m_frame.m_payloadSize = required;
        const int32_t result = router.receiveService(message.m_endpoint, message.m_frame);
        if (result != NI_OK && result != NI_NOT_FOUND) { return false; }
    }
    m_messages.clear();
    return true;
}

NetWorkThreadTestContext::NetWorkThreadTestContext() :
m_client(nullptr), m_mainThread(NetWorkClientOpaque::currentThreadId()), m_callbackThread(0), m_generation(0), m_ready(0), m_closed(0), m_done(0), m_results(0), m_bad(false)
{

}

void NetWorkThreadTestContext::ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
{
    NetWorkThreadTestContext* test = static_cast<NetWorkThreadTestContext*>(context);
    test->m_bad = test->m_bad.load() || generation == 0 || requests != 32 || chunk != 4096 || NetWorkClientOpaque::currentThreadId() == test->m_mainThread ||
        NetWork::instance().clientDestroy(test->m_client) != NI_INVALID_STATE || NetWork::instance().clientPoll(test->m_client) != NI_INVALID_STATE;
    test->m_callbackThread = NetWorkClientOpaque::currentThreadId();
    test->m_generation = generation;
    ++test->m_ready;
}

void NetWorkThreadTestContext::result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    NetWorkThreadTestContext* test = static_cast<NetWorkThreadTestContext*>(context);
    if (generation != test->m_generation.load() || product != 101 || type != 700 || NetWorkClientOpaque::currentThreadId() != test->m_callbackThread.load() ||
        (status != NI_RPC_SUCCESS && status != NI_RPC_CANCELLED) || (status == NI_RPC_SUCCESS && (size != 1 || bytes == nullptr || bytes[0] != 7))) { test->m_bad = true; }
    {
        std::lock_guard<std::mutex> lock(test->m_mutex);
        if (!test->m_terminal.insert(std::make_pair(id, status)).second)
        {
            test->m_bad = true;
        }
    }
    ++test->m_results;
}

int32_t NetWorkThreadTestContext::frame(void* context, uint64_t generation, const NetInterfaceFrame* frame)
{
    (void)generation; (void)frame; static_cast<NetWorkThreadTestContext*>(context)->m_bad = true; return 1;
}

void NetWorkThreadTestContext::closed(void* context, uint64_t generation, int32_t reason, int32_t detail)
{
    (void)detail;
    NetWorkThreadTestContext* test = static_cast<NetWorkThreadTestContext*>(context);
    if (generation != test->m_generation.load() || reason != NI_OK || NetWorkClientOpaque::currentThreadId() != test->m_callbackThread.load())
    {
        test->m_bad = true;
    }
    ++test->m_closed;
}

NetWorkThreadTestProducer::NetWorkThreadTestProducer(NetWorkThreadTestContext* context) : m_context(context)
{

}

void NetWorkThreadTestProducer::DoTask()
{
    for (uint32_t i = 0; i < 8; ++i)
    {
        uint64_t id = 0; uint8_t bytes = 7;
        if (NetWork::instance().clientBegin(m_context->m_client, 101, 700, 5000, &id) != NI_OK || id == 0 || NetWork::instance().clientPost(m_context->m_client, id, &bytes, 1) != NI_OK) { m_context->m_bad = true; break; }
        bytes = 99;
        if (i % 3 == 0)
        {
            const int32_t cancelled = NetWork::instance().clientCancel(m_context->m_client, id);
            if (cancelled != NI_OK && cancelled != NI_NOT_FOUND) { m_context->m_bad = true; }
        }
        std::lock_guard<std::mutex> lock(m_context->m_mutex);
        m_context->m_accepted.push_back(id);
    }
    ++m_context->m_done;
}

int main()
{
    uint32_t checks = 0;
#define THREAD_CHECK(condition) { ++checks; if (!(condition)) { std::printf("FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    NetHostFunctions host = {};
    host.structSize = sizeof(host); host.version = 1;
    host.create = LibuvTcp::hostCreate; host.listen = LibuvTcp::hostListen; host.connect = LibuvTcp::hostConnect; host.poll = LibuvTcp::hostPoll;
    host.write = LibuvTcp::hostWrite; host.pauseRead = LibuvTcp::hostPauseRead; host.close = LibuvTcp::hostClose; host.shutdown = LibuvTcp::hostShutdown;
    host.destroy = LibuvTcp::hostDestroy; host.localPort = LibuvTcp::hostLocalPort; host.peerCount = LibuvTcp::hostPeerCount; host.entropy = CSystem::secureRandomBytesCallback;
    THREAD_CHECK(NetWork::configureHost(host) == NI_OK);
    const int32_t baseline = CTaskThreadManager::Instance().Count();
    NetWorkThreadTestContext test;
    NetInterfaceClientCallbacks callbacks = {};
    callbacks.structSize = sizeof(callbacks); callbacks.version = 1; callbacks.context = &test;
    callbacks.ready = test.ready; callbacks.result = test.result; callbacks.frame = test.frame; callbacks.closed = test.closed;
    THREAD_CHECK(NetWork::instance().clientCreate(&callbacks, &test.m_client) == NI_OK);
    NetWorkThreadTestSink sink;
    NetPeerEndpointRouter gateway(&sink);
    NetInterfaceTlsOptions tls = {};
    tls.server = 1; tls.certificate = reinterpret_cast<const uint8_t*>(mbedtls_test_srv_crt_rsa_sha256_pem);
    tls.certificateSize = static_cast<uint32_t>(mbedtls_test_srv_crt_rsa_sha256_pem_len);
    tls.privateKey = reinterpret_cast<const uint8_t*>(mbedtls_test_srv_key_rsa_pem); tls.privateKeySize = static_cast<uint32_t>(mbedtls_test_srv_key_rsa_pem_len);
    tls.handshakeTimeoutMs = 5000;
    THREAD_CHECK(gateway.listen("127.0.0.1", 0, tls) == NI_OK);
    uint64_t service = 0;
    THREAD_CHECK(gateway.addService(101, false, 128, service) == NI_OK);
    NetInterfaceClientOptions options = {};
    options.structSize = sizeof(options); options.version = 1; options.address = "127.0.0.1"; options.port = gateway.localPort();
    options.platform = 1; options.capabilities = 7; options.maxRequests = 32; options.maxChunk = 4096; options.handshakeTimeoutMs = 5000;
    std::memset(options.device, 0x31, 16);
    options.tls.handshakeTimeoutMs = 5000; options.tls.hostname = "localhost";
    options.tls.authority = reinterpret_cast<const uint8_t*>(mbedtls_test_ca_crt_rsa_sha256_pem);
    options.tls.authoritySize = static_cast<uint32_t>(mbedtls_test_ca_crt_rsa_sha256_pem_len);
    for (uint32_t round = 0; round < 2; ++round)
    {
        THREAD_CHECK(NetWork::instance().clientConnect(test.m_client, &options) == NI_OK);
        const uint64_t deadline = NetSenderSubmission::now() + 15000;
        while (test.m_ready.load() <= round && NetSenderSubmission::now() < deadline && !test.m_bad.load())
        {
            gateway.poll(); CSystem::sleepPrecise(1);
        }
        THREAD_CHECK(test.m_ready.load() == round + 1 && !test.m_bad.load());
        const uint64_t generation = test.m_generation.load();
        THREAD_CHECK(generation == round + 1);
        uint32_t producers[4] = {};
        for (uint32_t i = 0; i < 4; ++i)
        {
            producers[i] = CTaskThreadManager::Instance().Init();
            CTaskThreadManager::Instance().GetThreadInterface(producers[i])->PostTask(std::shared_ptr<CTask>(new NetWorkThreadTestProducer(&test)));
        }
        // 此处刻意不调用clientPoll；实际工作必须由CTask独立驱动。
        while ((test.m_done.load() != (round + 1) * 4 || test.m_results.load() < (round + 1) * 38 - 6) && NetSenderSubmission::now() < deadline && !test.m_bad.load())
        {
            gateway.poll(); THREAD_CHECK(sink.respond(gateway)); CSystem::sleepPrecise(1);
        }
        for (uint32_t i = 0; i < 4; ++i) { CTaskThreadManager::Instance().Uninit(producers[i]); }
        THREAD_CHECK(!test.m_bad.load() && test.m_results.load() == (round + 1) * 38 - 6);
        for (uint32_t i = 0; i < 6; ++i)
        {
            uint64_t id = 0;
            THREAD_CHECK(NetWork::instance().clientBegin(test.m_client, 101, 700, 5000, &id) == NI_OK);
            std::lock_guard<std::mutex> lock(test.m_mutex); test.m_accepted.push_back(id);
        }
        THREAD_CHECK(NetWork::instance().clientClose(test.m_client, 0) == NI_OK);
        while (NetWork::instance().clientPoll(test.m_client) != NI_OK && NetSenderSubmission::now() < deadline)
        {
            gateway.poll(); CSystem::sleepPrecise(1);
        }
        THREAD_CHECK(test.m_closed.load() == round + 1 && test.m_results.load() == (round + 1) * 38 && !test.m_bad.load());
    }
    THREAD_CHECK(NetWork::instance().clientDestroy(test.m_client) == NI_OK); test.m_client = nullptr;
    THREAD_CHECK(CTaskThreadManager::Instance().Count() == baseline);
    THREAD_CHECK(test.m_accepted.size() == 76 && test.m_terminal.size() == 76);
    for (size_t i = 0; i < test.m_accepted.size(); ++i) { THREAD_CHECK(test.m_terminal.count(test.m_accepted[i]) == 1); }
    gateway.shutdown();
    std::printf("PASS NetWork CTask/TCP/TLS rounds=2 producers=8 unique=76 checks=%u\n", checks);
    return 0;
}

#endif // NET_WORK_THREAD_UNIT_END

/** 同步交付预留请求，不等待远端回复
@param [in] handle 本提供方句柄
@param [in] requestId 当前预留号
@param [in] bytes size非零时不可空
@param [in] size 调用时复制的字节数
@return 已交付或负数失败，不表示远端收到
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
        uint64_t ticket = 0;
        const int32_t accepted = handle->m_submission.sendSynchronous(requestId, bytes, size, ticket);
        return accepted == NI_OK ? handle->waitSynchronous(ticket) : accepted;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

/** 同步交付流帧，不等待远端回复
@param [in] handle 本提供方句柄
@param [in] frame 调用时复制的帧及载荷
@return 已交付或负数失败，不表示远端收到
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
        uint64_t ticket = 0;
        const int32_t accepted = handle->m_submission.dataSynchronous(*frame, ticket);
        return accepted == NI_OK ? handle->waitSynchronous(ticket) : accepted;
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

#if 0 // NET_WORK_LIFECYCLE_UNIT_BEGIN
#include "NetWork.h"
#include <cstdio>
#if defined(_WIN32) && defined(_DEBUG) && !defined(WORK_LIFECYCLE_NO_HEAP)
#include <crtdbg.h>
// 独立main只在本调用线程故障注入，不与后台任务或其他注入并行。
static uint32_t s_workAllocations = 0;
/** 拒绝全部普通堆分配
@param [in] operation 本次分配操作
@param [in] pointer CRT传入的堆块，只在钩子返回前借用；本用例未使用时忽略
@param [in] size CRT请求的字节数，是否参与注入判定由本用例决定
@param [in] block CRT堆块类别，只对普通业务堆块注入
@param [in] request CRT分配序号，本用例不使用
@param [in] filename CRT传入的来源文件名，本用例不读取
@param [in] line 来源代码行号，本用例不使用
@return 是否允许
*/
static int __cdecl WorkLifecycleMemoryHook(int operation, void* pointer, size_t size, int block, long request, const unsigned char* filename, int line)
{
    (void)pointer;
    (void)size;
    (void)request;
    (void)filename;
    (void)line;
    if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
    {
        ++s_workAllocations;
        return 0;
    }
    return 1;
}

#endif

// 无连接、无任务的测试句柄不允许产生任何业务回调。
static uint32_t s_workUnexpectedCallbacks = 0;
/** 记录非预期就绪
@param [in] context 调用者上下文，本用例不使用
@param [in] generation 会话代次，本用例不应产生
@param [in] requests 协商请求数，本用例不使用
@param [in] chunk 协商分块字节数，本用例不使用
*/
static void WorkLifecycleReady(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
{
    (void)context;
    (void)generation;
    (void)requests;
    (void)chunk;
    ++s_workUnexpectedCallbacks;
}

/** 记录非预期结果
@param [in] context 调用者上下文，本用例不使用
@param [in] generation 会话代次，本用例不应产生
@param [in] requestId 请求号，本用例未发送请求
@param [in] product 产品号，本用例不使用
@param [in] type 业务类型，本用例不使用
@param [in] status 终态码，本用例不使用
@param [in] bytes 仅借用至返回的正文，本用例不读取
@param [in] size 正文字节数，本用例不读取
*/
static void WorkLifecycleResult(void* context, uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    (void)context;
    (void)generation;
    (void)requestId;
    (void)product;
    (void)type;
    (void)status;
    (void)bytes;
    (void)size;
    ++s_workUnexpectedCallbacks;
}

/** 记录非预期数据
@param [in] context 调用者上下文，本用例不使用
@param [in] generation 会话代次，本用例不应产生
@param [in] frame 仅借用至返回的帧，本用例不读取
@return 1已消费；本用例另行记录非预期回调
*/
static int32_t WorkLifecycleFrame(void* context, uint64_t generation, const NetInterfaceFrame* frame)
{
    (void)context;
    (void)generation;
    (void)frame;
    ++s_workUnexpectedCallbacks;
    return 1;
}

/** 记录非预期关闭
@param [in] context 调用者上下文，本用例不使用
@param [in] generation 会话代次，本用例不应产生
@param [in] reason 关闭原因，本用例不使用
@param [in] detail 底层诊断码，本用例不使用
*/
static void WorkLifecycleClosed(void* context, uint64_t generation, int32_t reason, int32_t detail)
{
    (void)context;
    (void)generation;
    (void)reason;
    (void)detail;
    ++s_workUnexpectedCallbacks;
}

/** 验证第一次使用同步门时即使内存不足，也可关闭、观察和销毁未连接客户端
@return 全部独立预期成立为true
*/
static bool NetWorkLifecycleAudit()
{
    uint32_t checks = 0;
    s_workUnexpectedCallbacks = 0;
#define WORK_LIFECYCLE_CHECK(value) { ++checks; if (!(value)) { std::printf("Work lifecycle line=%d FAIL\n", __LINE__); return false; } }
    for (uint32_t round = 0; round < 16; ++round)
    {
#if defined(_WIN32) && defined(_DEBUG) && !defined(WORK_LIFECYCLE_NO_HEAP)
        _CrtMemState before = {};
        _CrtMemCheckpoint(&before);
#endif
        NetWorkClientCallbacks callbacks = {};
        callbacks.structSize = sizeof(callbacks);
        callbacks.version = 1;
        callbacks.ready = WorkLifecycleReady;
        callbacks.result = WorkLifecycleResult;
        callbacks.frame = WorkLifecycleFrame;
        callbacks.closed = WorkLifecycleClosed;
        NetWorkClientHandle client = nullptr;
        WORK_LIFECYCLE_CHECK(NetWork::instance().clientCreate(&callbacks, &client) == NI_OK && client != nullptr);
        uint64_t generation = 99;
        int32_t ready = 99;
#if defined(_WIN32) && defined(_DEBUG) && !defined(WORK_LIFECYCLE_NO_HEAP)
        s_workAllocations = 0;
        const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(WorkLifecycleMemoryHook);
#endif
        const int32_t closed = NetWork::instance().clientClose(client, NI_CANCELLED);
        const int32_t repeated = NetWork::instance().clientClose(client, NI_INVALID_STATE);
        const int32_t state = NetWork::instance().clientState(client, &generation, &ready);
        const int32_t polled = NetWork::instance().clientPoll(client);
        const int32_t destroyed = NetWork::instance().clientDestroy(client);
#if defined(_WIN32) && defined(_DEBUG) && !defined(WORK_LIFECYCLE_NO_HEAP)
        _CrtSetAllocHook(old);
        _CrtMemState after = {};
        _CrtMemCheckpoint(&after);
        WORK_LIFECYCLE_CHECK(s_workAllocations == 0);
        WORK_LIFECYCLE_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
#endif
        WORK_LIFECYCLE_CHECK(closed == NI_OK && repeated == NI_OK && state == NI_OK && polled == NI_OK && destroyed == NI_OK);
        WORK_LIFECYCLE_CHECK(generation == 0 && ready == 0 && s_workUnexpectedCallbacks == 0);
    }
    std::printf("Work lifecycle cold close/state/destroy checks=%u PASS\n", checks);
    return true;
#undef WORK_LIFECYCLE_CHECK
}

#ifndef WORK_LIFECYCLE_NO_MAIN
/** 独立测试入口；正式库保留整段注释
@return 零表示成功
*/
int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    return NetWorkLifecycleAudit() ? 0 : 1;
}

#endif

#endif // NET_WORK_LIFECYCLE_UNIT_END

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
    client->m_lastError = result;
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