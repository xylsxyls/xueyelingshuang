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
    if ((seeds != nullptr && std::strlen(seeds) > 4096) || (cachePath != nullptr && std::strlen(cachePath) > 4096))
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

// NET_PEER_UNIT_TEST_BEGIN
//#include "NetPeer.h"
//#if defined(STATIC_LIB) || defined(__EMSCRIPTEN__)
//#include "NetPeer/NetPeerSenderState.h"
//#include "NetPeer/NetPeerClientState.h"
//#endif
//#include "NetPeer/NetPeerTest.h"
//#include <cstdio>
//#include <cstring>
//
//uint32_t NetPeerTest::m_created = 0;
//
//NetPeerTest::NetPeerTest() :
//m_closed(0),
//m_other(0),
//m_generation(0), m_client(nullptr), m_cpp(nullptr), m_reenter(nullptr), m_throwEvents(0), m_bad(false)
//{
//}
//void NetPeerTest::onSessionReady(uint64_t generation, uint32_t requests, uint32_t chunk)
//{
//    (void)generation; (void)requests; (void)chunk; ++m_other; if (m_reenter) { m_reenter(*this); } if (m_throwEvents & 1) { throw 1; }
//}
//void NetPeerTest::onSessionResult(uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
//{
//    (void)generation; (void)product; (void)type; (void)status; m_resultIds.push_back(id); m_resultBytes.push_back(size == 0 ? 0 : bytes[0]); ++m_other; if (m_reenter) { m_reenter(*this); } if (m_throwEvents & 2) { throw 2; }
//}
//bool NetPeerTest::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
//{
//    (void)generation; (void)frame; ++m_other; if (m_reenter) { m_reenter(*this); } if (m_throwEvents & 4) { throw 4; } return false;
//}
//void NetPeerTest::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
//{
//    (void)reason; (void)detail; ++m_closed; m_generation = generation; if (m_reenter) { m_reenter(*this); } if (m_throwEvents & 8) { throw 8; }
//}
//int32_t NetPeerTest::entropy(void* context, uint8_t* bytes, uint32_t size)
//{
//    (void)context; std::memset(bytes, 0x71, size); return 0;
//}
//NetHostFunctions NetPeerTest::functions()
//{
//    NetHostFunctions value = {};
//    value.structSize = sizeof(value); value.version = 1;
//    value.create = create; value.connect = connect; value.poll = operation; value.write = write;
//    value.pauseRead = control; value.close = control; value.shutdown = operation; value.destroy = operation;
//    value.localPort = query; value.peerCount = query; value.entropy = entropy;
//    return value;
//}
//int32_t NetPeerTest::create(void* context, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance)
//{
//    ++m_created;
//    (void)context; (void)observer; (void)connected; (void)data; (void)writable; (void)closed; *instance = nullptr; return -1;
//}
//int32_t NetPeerTest::connect(void* instance, const char* address, uint32_t port, uint64_t* peer)
//{
//    (void)instance; (void)address; (void)port; *peer = 0; return -1;
//}
//int32_t NetPeerTest::operation(void* instance)
//{
//    (void)instance; return 0;
//}
//int32_t NetPeerTest::write(void* instance, uint64_t peer, const uint8_t* bytes, uint32_t size)
//{
//    (void)instance; (void)peer; (void)bytes; (void)size; return -1;
//}
//int32_t NetPeerTest::control(void* instance, uint64_t peer, int32_t value)
//{
//    (void)instance; (void)peer; (void)value; return 0;
//}
//uint32_t NetPeerTest::query(void* instance)
//{
//    (void)instance; return 0;
//}
//void NetPeerTest::ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
//{
//    static_cast<NetPeerTest*>(context)->onSessionReady(generation, requests, chunk);
//}
//void NetPeerTest::result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
//{
//    static_cast<NetPeerTest*>(context)->onSessionResult(generation, id, product, type, status, bytes, size);
//}
//int32_t NetPeerTest::frame(void* context, uint64_t generation, const NetInterfaceFrame* frame)
//{
//    return static_cast<NetPeerTest*>(context)->onSessionFrame(generation, *frame) ? 1 : 0;
//}
//void NetPeerTest::closed(void* context, uint64_t generation, int32_t reason, int32_t detail)
//{
//    static_cast<NetPeerTest*>(context)->onSessionClosed(generation, reason, detail);
//}
//
//NetPeerClientOptions NetPeerTest::options()
//{
//    NetPeerClientOptions value = {};
//    value.structSize = sizeof(value);
//    value.version = 1;
//    value.address = "127.0.0.1";
//    value.port = 1;
//    value.platform = 1;
//    value.capabilities = 7;
//    value.maxRequests = 4;
//    value.maxChunk = 4096;
//    value.handshakeTimeoutMs = 1000;
//    value.tls.handshakeTimeoutMs = 1000;
//    value.tls.entropy = entropy;
//    std::memset(value.device, 0x31, sizeof(value.device));
//    return value;
//}
//
//NetPeerClientCallbacks NetPeerTest::callbacks()
//{
//    NetPeerClientCallbacks value = {};
//    value.structSize = sizeof(value);
//    value.version = 1;
//    value.context = this;
//    value.ready = ready;
//    value.result = result;
//    value.frame = frame;
//    value.closed = closed;
//    return value;
//}
//
//void NetPeerTest::inspectC(NetPeerTest& test)
//{
//    const NetPeerClientOptions options = NetPeerTest::options();
//    uint64_t ticket = 99;
//    NetInterfaceFrame frame = {};
//    test.m_bad = test.m_bad || NetPeer::instance().clientDestroy(test.m_client) != NI_INVALID_STATE ||
//        NetPeer::instance().clientPoll(test.m_client) != NI_INVALID_STATE ||
//        NetPeer::instance().clientConnect(test.m_client, &options) != NI_INVALID_STATE ||
//        NetPeer::instance().clientConnectService(test.m_client, 101, &options) != NI_INVALID_STATE ||
//        NetPeer::instance().clientConfigureCompression(test.m_client, nullptr, 0) != NI_INVALID_STATE ||
//        NetPeer::instance().clientConfigureGateways(test.m_client, nullptr, nullptr) != NI_INVALID_STATE ||
//        NetPeer::instance().clientSend(test.m_client, 1, nullptr, 0) != NI_INVALID_STATE ||
//        NetPeer::instance().clientSendData(test.m_client, &frame) != NI_INVALID_STATE ||
//        NetPeer::instance().clientSendBegin(test.m_client, 1, nullptr, 0, &ticket) != NI_INVALID_STATE || ticket != 0;
//}
//
//#include "NetInterface/NetInterfaceAPI.h"
//NetSenderDispatchTestChannel::NetSenderDispatchTestChannel(const NetInterfaceHandshakeOptions& options) :
//m_reply(),
//m_handshake(new NetInterfaceHandshake(options, NetSenderSubmission::now())),
//m_listener(nullptr),
//m_announced(false),
//m_closing(false),
//m_reason(0)
//{
//
//}
//
//NetSenderDispatchTestChannel::~NetSenderDispatchTestChannel()
//{
//    shutdown();
//}
//
//int32_t NetSenderDispatchTestChannel::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
//    NetTransportListener* listener, uint64_t& peer)
//{
//    (void)address;
//    (void)port;
//    (void)options;
//    peer = 0;
//    if (m_listener != nullptr || listener == nullptr)
//    {
//        return NI_INVALID_STATE;
//    }
//    m_listener = listener;
//    peer = 1;
//    return NI_OK;
//}
//
//bool NetSenderDispatchTestChannel::poll()
//{
//    if (m_closing)
//    {
//        shutdown();
//        return false;
//    }
//    if (m_listener == nullptr)
//    {
//        return false;
//    }
//    if (!m_announced)
//    {
//        m_announced = true;
//        m_listener->onReady(1);
//    }
//    NetInterfaceFrame frame = {};
//    if (!m_closing && m_handshake->peekOutput(frame) == NI_READY && m_listener->onFrame(1, frame))
//    {
//        m_handshake->consumeOutput();
//    }
//    if (!m_closing && m_reply.m_kind != 0 && m_listener->onFrame(1, m_reply))
//    {
//        m_reply = NetInterfaceFrame();
//        m_payload.clear();
//    }
//    return true;
//}
//
//int32_t NetSenderDispatchTestChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
//{
//    if (peer != 1 || m_listener == nullptr || m_closing)
//    {
//        return NI_INVALID_STATE;
//    }
//    if (frame.m_kind == NI_REQUEST)
//    {
//        if (m_reply.m_kind != 0) { return NI_LIMIT_EXCEEDED; }
//        NetInterfaceRpcView view = {};
//        if (NetInterfaceRpc::parse(frame, view) != NI_OK)
//{
//    return NI_PROTOCOL_ERROR;
//}

//        m_requests.push_back(frame.m_requestId);
//        m_bytes.push_back(view.m_size == 0 ? 0 : view.m_data[0]);
//        m_payload.resize(view.m_size + 16);
//        uint32_t required = 0;
//        const int32_t encoded = NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, view.m_data, view.m_size, m_payload.data(), static_cast<uint32_t>(m_payload.size()), required);
//        if (encoded != NI_OK) { return encoded; }
//        m_reply = frame;
//        m_reply.m_kind = NI_RESPONSE;
//        m_reply.m_payload = m_payload.data();
//        m_reply.m_payloadSize = required;
//        return NI_OK;
//    }
//    if (frame.m_kind == NI_CANCEL) { return NI_OK; }
//    const int32_t accepted = m_handshake->accept(frame, NetSenderSubmission::now());
//    return accepted < 0 ? accepted : NI_OK;
//}
//
//void NetSenderDispatchTestChannel::close(uint64_t peer, int32_t reason)
//{
//    if (peer == 1 && !m_closing)
//    {
//        m_closing = true;
//        m_reason = reason;
//    }
//}
//
//int32_t NetSenderDispatchTestChannel::outputState(uint64_t peer) const
//{
//    return peer == 1 && m_listener != nullptr && !m_closing ? NI_OK : NI_INVALID_STATE;
//}
//
//void NetSenderDispatchTestChannel::shutdown()
//{
//    m_closing = true;
//    NetTransportListener* listener = m_listener;
//    m_listener = nullptr;
//    if (listener != nullptr)
//    {
//        listener->onClosed(1, m_reason, 0);
//    }
//}
//
//
//
//void NetPeerTest::inspectCpp(NetPeerTest& test)
//{
//    NetSenderGatewayOptions options = {};
//    options.m_network = NetPeerTest::options();
//    NetPeerTest wrong;
//    test.m_bad = test.m_bad || NetPeer::instance().connectServiceGateway(test.m_cpp, 101, options, &wrong) != NI_INVALID_STATE ||
//        NetPeer::instance().configureCompression(test.m_cpp, nullptr, 0) != NI_INVALID_STATE ||
//        NetPeer::instance().prepareGateway(test.m_cpp) != NI_INVALID_STATE || NetPeer::instance().sendRequest(test.m_cpp, 1, nullptr, 0) != NI_INVALID_STATE;
//    NetPeer::instance().destroyClient(test.m_cpp);
//    test.m_bad = test.m_bad || NetPeer::instance().lastGatewayError(test.m_cpp) != NI_INVALID_STATE || NetPeer::instance().gatewayGeneration(test.m_cpp) == 0;
//}
//
//#if defined(_WIN32) && defined(_DEBUG)
//#include <crtdbg.h>
//static int32_t g_peerRejectAfter = -1;
///** 单元专用逐次构造分配故障
//@param [in] operation CRT操作
//@param [in] data 未使用
//@param [in] size 未使用
//@param [in] block 内存类型
//@param [in] request 未使用
//@param [in] file 未使用
//@param [in] line 未使用
//@return 是否允许分配
//*/
//static int PeerMemoryHook(int operation, void*, size_t, int block, long, const unsigned char*, int)
//{
//    if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK && g_peerRejectAfter >= 0)
//    {
//        if (g_peerRejectAfter == 0) { return 0; }
//        --g_peerRejectAfter;
//    }
//    return 1;
//}
//#endif
//
//int main()
//{
//    uint32_t checks = 0;
//#define PEER_CHECK(condition) { ++checks; if (!(condition)) { std::printf("FAIL line=%u condition=%s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
//    NetFileIo noBackend;
//    PEER_CHECK(noBackend.init() == NI_NOT_SUPPORTED && noBackend.done());
//    NetPeerClientHandle invalid = nullptr;
//    PEER_CHECK(NetPeer::instance().clientCreate(nullptr, &invalid) == NI_INVALID_ARGUMENT && invalid == nullptr);
//    const NetHostFunctions host = NetPeerTest::functions();
//    PEER_CHECK(NetPeer::configureHost(host) == NI_OK);
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState before;
//    _CrtMemCheckpoint(&before);
//#endif
//    {
//
//    {
//        NetPeerTest test;
//        NetPeerClientCallbacks callbacks = test.callbacks();
//        NetPeerClientHandle handle = nullptr;
//        PEER_CHECK(NetPeer::instance().clientCreate(&callbacks, &handle) == NI_OK);
//        test.m_client = handle;
//        test.m_reenter = NetPeerTest::inspectC;
//        NetPeerClientOptions options = NetPeerTest::options();
//        uint64_t id = 99;
//        PEER_CHECK(NetPeer::instance().clientBegin(handle, 101, 700, 1000, &id) == NI_INVALID_STATE && id == 0);
//        PEER_CHECK(NetPeer::instance().clientConfigureGateways(handle, "bad", nullptr) == NI_INVALID_ARGUMENT);
//        PEER_CHECK(NetPeer::instance().clientConfigureGateways(handle, "127.0.0.1:1", nullptr) == NI_OK);
//        for (uint32_t iteration = 0; iteration < 64; ++iteration)
//        {
//            options.platform = iteration % 3 + 1;
//            PEER_CHECK(NetPeer::instance().clientConnectService(handle, 101, &options) == NI_OK);
//            uint64_t generation = 0;
//            int32_t ready = 1;
//            PEER_CHECK(NetPeer::instance().clientState(handle, &generation, &ready) == NI_OK && generation == iteration + 1 && ready == 0);
//            PEER_CHECK(NetPeer::instance().clientDestroy(handle) == NI_INVALID_STATE);
//            PEER_CHECK(NetPeer::instance().clientConnect(handle, &options) == NI_INVALID_STATE);
//            PEER_CHECK(NetPeer::instance().clientConnectService(handle, 102, &options) == NI_INVALID_STATE);
//            PEER_CHECK(NetPeer::instance().clientConfigureCompression(handle, nullptr, 0) == NI_INVALID_STATE);
//            PEER_CHECK(NetPeer::instance().clientConfigureGateways(handle, nullptr, nullptr) == NI_INVALID_STATE);
//            id = 99;
//            PEER_CHECK(NetPeer::instance().clientBegin(handle, 101, 700, 1000, &id) == NI_INVALID_STATE && id == 0);
//            test.m_throwEvents = (iteration & 1) ? 8 : 0;
//            PEER_CHECK(NetPeer::instance().clientClose(handle, NI_CANCELLED) == NI_OK);
//            PEER_CHECK(NetPeer::instance().clientPoll(handle) == NI_OK);
//            PEER_CHECK(test.m_closed == iteration + 1 && test.m_generation == iteration + 1 && test.m_other == 0 && !test.m_bad);
//            PEER_CHECK(NetPeer::instance().clientPoll(handle) == NI_OK && test.m_closed == iteration + 1);
//        }
//        const uint32_t invalidPlatforms[] = {0, 4, UINT32_MAX};
//        for (uint32_t index = 0; index < 3; ++index)
//        {
//            options.platform = invalidPlatforms[index];
//            PEER_CHECK(NetPeer::instance().clientConnectService(handle, 101, &options) == NI_INVALID_ARGUMENT);
//        }
//        PEER_CHECK(NetPeer::instance().clientConfigureCompression(handle, nullptr, 0) == NI_OK);
//        PEER_CHECK(NetPeer::instance().clientDestroy(handle) == NI_OK);
//        PEER_CHECK(NetPeer::instance().clientDestroy(nullptr) == NI_OK);
//    }
//
//    {
//        NetPeer& sender = NetPeer::instance();
//        NetPeerSenderState* client = nullptr;
//        PEER_CHECK(sender.createClient(client) == NI_OK);
//        NetPeerTest original;
//        NetPeerTest wrong;
//        original.m_cpp = client;
//        original.m_reenter = NetPeerTest::inspectCpp;
//        PEER_CHECK(sender.prepareGateway(client) == NI_OK && sender.prepareGateway(client) == NI_OK);
//        NetSenderGatewayOptions options = {};
//        options.m_network = NetPeerTest::options();
//        for (uint32_t iteration = 0; iteration < 64; ++iteration)
//        {
//            options.m_network.platform = iteration % 3 + 1;
//            PEER_CHECK(sender.connectServiceGateway(client, 101, options, &original) == NI_OK);
//            PEER_CHECK(!sender.gatewayReady(client) && sender.gatewayGeneration(client) == iteration + 1);
//            PEER_CHECK(sender.connectServiceGateway(client, 101, options, &wrong) == NI_INVALID_STATE);
//            PEER_CHECK(sender.connectGateway(client, options, &wrong) == NI_INVALID_STATE);
//            sender.destroyClient(client);
//            PEER_CHECK(sender.lastGatewayError(client) == NI_INVALID_STATE && sender.gatewayGeneration(client) == iteration + 1);
//            original.m_throwEvents = (iteration & 1) ? 8 : 0;
//            sender.closeGateway(client, NI_CANCELLED);
//            PEER_CHECK(!sender.pollGateway(client));
//            PEER_CHECK(original.m_closed == iteration + 1 && original.m_other == 0 && !original.m_bad && wrong.m_closed == 0);
//            PEER_CHECK(!sender.pollGateway(client));
//        }
//        options.m_localGatewayPid = 1;
//        PEER_CHECK(sender.connectServiceGateway(client, 101, options, &wrong) == NI_INVALID_ARGUMENT);
//        options.m_localGatewayPid = 0;
//        PEER_CHECK(sender.connectServiceGateway(client, 0, options, &wrong) == NI_INVALID_ARGUMENT);
//        PEER_CHECK(sender.connectServiceGateway(client, UINT32_MAX, options, &wrong) == NI_INVALID_ARGUMENT);
//        sender.destroyClient(client);
//        PEER_CHECK(sender.gatewayGeneration(client) == 0 && !sender.gatewayReady(client));
//    }
//
//#if defined(STATIC_LIB) || defined(__EMSCRIPTEN__)
//
//    {
//        NetInterfaceHandshakeOptions server = {};
//        server.m_server = 1;
//        server.m_capabilities = 7;
//        server.m_maxRequests = 4;
//        server.m_maxChunk = 4096;
//        server.m_timeoutMs = 5000;
//        std::memset(server.m_nonce, 0x43, sizeof(server.m_nonce));
//        NetSenderDispatchTestChannel channel(server);
//        NetPeerTest observer;
//        NetPeerClientHandle handle = nullptr;
//        const NetPeerClientCallbacks callbacks = observer.callbacks();
//        PEER_CHECK(NetPeer::instance().clientCreate(&callbacks, &handle) == NI_OK);
//        observer.m_client = handle;
//        observer.m_reenter = NetPeerTest::inspectC;
//        PEER_CHECK(handle->m_session.bindChannel(&channel) == NI_OK);
//        const NetPeerClientOptions options = NetPeerTest::options();
//        PEER_CHECK(NetPeer::instance().clientConnect(handle, &options) == NI_OK);
//        for (uint32_t step = 0; step < 16 && observer.m_other == 0; ++step) { NetPeer::instance().clientPoll(handle); }
//        PEER_CHECK(observer.m_other == 1 && !observer.m_bad);
//        uint64_t generation = 0;
//        int32_t ready = 0;
//        PEER_CHECK(NetPeer::instance().clientState(handle, &generation, &ready) == NI_OK && ready == 1);
//        std::vector<uint64_t> submitted;
//        for (uint32_t round = 0; round < 128; ++round)
//        {
//            uint64_t ids[4] = {};
//            for (uint32_t index = 0; index < 4; ++index)
//            {
//                PEER_CHECK(NetPeer::instance().clientBegin(handle, 101, 700, 1000, &ids[index]) == NI_OK);
//            }
//            for (uint32_t index = 0; index < 4; ++index)
//            {
//                uint8_t byte = static_cast<uint8_t>(index + 1);
//                const uint64_t id = ids[3 - index];
//                submitted.push_back(id);
//                if (index != 3)
//                {
//                    PEER_CHECK(NetPeer::instance().clientPost(handle, id, &byte, 1) == NI_OK);
//                    PEER_CHECK(channel.m_requests.size() == round * 4);
//                }
//                else
//                {
//                    uint64_t ticket = 0;
//                    PEER_CHECK(NetPeer::instance().clientSendBegin(handle, id, &byte, 1, &ticket) == NI_OK && ticket != 0);
//                    int32_t result = NI_READY;
//                    for (uint32_t step = 0; step < 32 && result == NI_READY; ++step)
//                    {
//                        NetPeer::instance().clientPoll(handle);
//                        result = NetPeer::instance().clientSendResult(handle, ticket);
//                    }
//                    PEER_CHECK(result == NI_OK && channel.m_requests.size() == (round + 1) * 4);
//                    PEER_CHECK(NetPeer::instance().clientSendResult(handle, ticket) == NI_NOT_FOUND);
//                }
//                byte = 99;
//            }
//            for (uint32_t step = 0; step < 16 && observer.m_resultIds.size() != submitted.size(); ++step) { NetPeer::instance().clientPoll(handle); }
//            PEER_CHECK(observer.m_resultIds == submitted && !observer.m_bad);
//        }
//        for (uint32_t index = 0; index < 512; ++index)
//        {
//            PEER_CHECK(channel.m_requests[index] == index + 1);
//            PEER_CHECK(channel.m_bytes[index] == index % 4 + 1 && observer.m_resultBytes[index] == index % 4 + 1);
//        }
//        uint64_t cancelled = 0;
//        PEER_CHECK(NetPeer::instance().clientBegin(handle, 101, 700, 1000, &cancelled) == NI_OK);
//        PEER_CHECK(NetPeer::instance().clientCancel(handle, cancelled) == NI_OK);
//        NetPeer::instance().clientPoll(handle);
//        PEER_CHECK(observer.m_resultIds.size() == 513 && observer.m_resultIds.back() == cancelled && channel.m_requests.size() == 512);
//        PEER_CHECK(NetPeer::instance().clientClose(handle, NI_CANCELLED) == NI_OK);
//        PEER_CHECK(NetPeer::instance().clientPoll(handle) == NI_OK && observer.m_closed == 1 && !observer.m_bad);
//        PEER_CHECK(NetPeer::instance().clientDestroy(handle) == NI_OK);
//    }
//    // 注入四类事件只核对C/C++桥接和异常边界；真实TLS另由进程集成测试核对。
//    for (uint32_t mode = 0; mode < 16; ++mode)
//    {
//        NetPeerSenderState bridge;
//        NetPeerTest observer;
//        bridge.m_listener = &observer;
//        const NetPeerClientCallbacks callbacks = bridge.callbacks();
//        PEER_CHECK(NetPeer::instance().clientCreate(&callbacks, &bridge.m_client) == NI_OK);
//        observer.m_client = bridge.m_client;
//        observer.m_reenter = NetPeerTest::inspectC;
//        observer.m_throwEvents = mode;
//        bridge.m_client->onSessionReady(7, 4, 4096);
//        bridge.m_client->onSessionResult(7, 1, 101, 700, 0, nullptr, 0);
//        NetInterfaceFrame frame = {};
//        const bool accepted = bridge.m_client->onSessionFrame(7, frame);
//        PEER_CHECK(accepted == ((mode & 4) != 0));
//        bridge.m_client->onSessionClosed(7, NI_CANCELLED, 0);
//        PEER_CHECK(observer.m_other == 3 && observer.m_closed == 1 && !observer.m_bad);
//        PEER_CHECK(bridge.m_client->m_callbackDepth == 0 && bridge.m_client->inactive());
//        PEER_CHECK(NetPeer::instance().clientDestroy(bridge.m_client) == NI_OK);
//    }
//#endif
//#if defined(_WIN32) && defined(_DEBUG)
//    // 逐次拒绝构造中每一处分配，恢复后允许同一门面重试，不能泄漏部分核心。
//    uint32_t testedFailures = 0;
//    bool complete = false;
//    for (int32_t step = 0; step < 128 && !complete; ++step)
//    {
//        NetPeer& sender = NetPeer::instance();
//        NetPeerSenderState* client = nullptr;
//        PEER_CHECK(sender.createClient(client) == NI_OK);
//        g_peerRejectAfter = step;
//        const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(PeerMemoryHook);
//        const int32_t prepared = sender.prepareGateway(client);
//        _CrtSetAllocHook(previous);
//        g_peerRejectAfter = -1;
//        PEER_CHECK(prepared == NI_OK || prepared == NI_NO_MEMORY);
//        complete = prepared == NI_OK;
//        if (!complete) { ++testedFailures; }
//        PEER_CHECK(sender.prepareGateway(client) == NI_OK);
//        sender.destroyClient(client);
//        PEER_CHECK(sender.gatewayGeneration(client) == 0);
//    }
//    PEER_CHECK(complete && testedFailures != 0);
//#endif
//
//    }
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState after;
//    _CrtMemCheckpoint(&after);
//    PEER_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
//#endif
//    PEER_CHECK(NetPeerTest::m_created == 0);
//    std::printf("Peer facade checks=%u PASS\n", checks);
//    return 0;
//#undef PEER_CHECK
//}
// NET_PEER_UNIT_TEST_END

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