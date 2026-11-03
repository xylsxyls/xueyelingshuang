#include "NetGatewayTestCases.h"
#include "NetWorkTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <ctime>
#include "NetInterface/NetInterfaceAPI.h"

NetGatewaySessionProbe::NetGatewaySessionProbe() : m_client(nullptr), m_generation(0), m_closes(0),
    m_completions(0), m_lastId(0), m_status(0), m_bad(false), m_reissueOnTimeout(false), m_reissuedId(0), m_reissueResult(NI_INVALID_STATE)
{

}

NetWorkClientCallbacks NetGatewaySessionProbe::callbacks()
{
    NetWorkClientCallbacks result = {};
    result.structSize = sizeof(result);
    result.version = 1;
    result.context = this;
    result.ready = &Ready;
    result.result = &Result;
    result.frame = &Frame;
    result.closed = &Closed;
    return result;
}

NetGatewaySessionProbe::~NetGatewaySessionProbe()
{
    if (m_client != nullptr)
    {
        NetPeer::instance().clientClose(m_client, 0);
        while (NetPeer::instance().clientPoll(m_client) != NI_OK || NetPeer::instance().clientDestroy(m_client) != NI_OK)
        {
            CSystem::sleepPrecise(1);
        }
        m_client = nullptr;
    }
}

void NetGatewaySessionProbe::Ready(void* context, uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    NetGatewaySessionProbe* self = static_cast<NetGatewaySessionProbe*>(context);
    self->m_bad = self->m_bad || generation <= self->m_generation || maxRequests != 4 || maxChunk != 4096 ||
        NetPeer::instance().clientDestroy(self->m_client) != NI_INVALID_STATE ||
        NetPeer::instance().clientSend(self->m_client, 0, nullptr, 0) != NI_INVALID_STATE;
    self->m_generation = generation;
}

void NetGatewaySessionProbe::Result(void* context, uint64_t generation, uint64_t id, uint32_t product,
    uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    NetGatewaySessionProbe* self = static_cast<NetGatewaySessionProbe*>(context);
    self->m_bad = self->m_bad || generation != self->m_generation || id == 0 || product == 0 || type != 700 ||
        (status != NI_RPC_SUCCESS && size != 0);
    ++self->m_completions;
    self->m_lastId = id;
    self->m_status = status;
    self->m_body.clear();
    if (size != 0) self->m_body.assign(bytes, bytes + size);
    if (self->m_reissueOnTimeout && status == NI_RPC_TIMEOUT)
    {
        self->m_reissueOnTimeout = false;
        // 强制跨过poll的毫秒快照，模拟回调结束前提交新的独立操作。
        CSystem::sleepPrecise(3);
        self->m_reissueResult = NetPeer::instance().clientBegin(self->m_client, 4, 700, 20, &self->m_reissuedId);
    }
}

int32_t NetGatewaySessionProbe::Frame(void* context, uint64_t generation, const NetInterfaceFrame* frame)
{
    (void)generation;
    (void)frame;
    static_cast<NetGatewaySessionProbe*>(context)->m_bad = true;
    return 1;
}

void NetGatewaySessionProbe::Closed(void* context, uint64_t generation, int32_t reason, int32_t detail)
{
    (void)reason;
    (void)detail;
    NetGatewaySessionProbe* self = static_cast<NetGatewaySessionProbe*>(context);
    self->m_bad = self->m_bad || generation != self->m_generation;
    ++self->m_closes;
}

/** 检查独立断言及统一取消点
@param [in] expected 必需条件
@param [in] reason 失败原因
*/
static void CheckGateway(bool expected, const char* reason)
{
    netTestCheckpoint();
    if (!expected)
    {
        throw std::runtime_error(reason);
    }
}

/** 单调毫秒
@return 当前时间
*/
static uint64_t GatewayTestNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetGatewayTestSink::NetGatewayTestSink() : m_busy(false)
{

}

int32_t NetGatewayTestSink::send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetGatewayOrigin& origin)
{
    if (m_busy)
    {
        return NI_LIMIT_EXCEEDED;
    }
    NetGatewayTestDelivery delivery;
    delivery.m_endpoint = endpoint;
    delivery.m_origin = origin;
    delivery.m_message.m_frame = frame;
    delivery.m_message.m_frame.m_payload = nullptr;
    if (frame.m_payloadSize != 0)
    {
        delivery.m_message.m_payload.assign(frame.m_payload, frame.m_payload + frame.m_payloadSize);
    }
    m_deliveries.push_back(delivery);
    return NI_OK;
}

void NetGatewayTestSink::close(uint64_t endpoint)
{
    m_closed.insert(endpoint);
}

NetGatewayTestClient::NetGatewayTestClient(uint32_t limit) :
    m_peer(0), m_limit(limit), m_holdHello(false), m_applicationReady(false)
{
    m_transport = &m_network;
}

void NetGatewayTestClient::onReady(uint64_t peer)
{
    NetInterfaceTransportProbe::onReady(peer);
    m_peer = peer;
    NetInterfaceHandshakeOptions options = {};
    options.m_platform = 3;
    options.m_capabilities = 7;
    options.m_maxRequests = m_limit;
    options.m_maxChunk = 4096;
    options.m_timeoutMs = 5000;
    CheckGateway(NetTransport::randomBytes(options.m_nonce, 16) == NI_OK, "gateway client nonce");
    std::memset(options.m_device, 0x42, 16);
    m_handshake.reset(new NetInterfaceHandshake(options, GatewayTestNow()));
}

bool NetGatewayTestClient::onFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    const int32_t accepted = m_handshake->accept(frame, GatewayTestNow());
    CheckGateway(accepted >= 0, "gateway application handshake or frame failed");
    if (accepted == NI_READY)
    {
        m_applicationReady = true;
    }
    return accepted != NI_OK || NetInterfaceTransportProbe::onFrame(peer, frame);
}

void NetGatewayTestClient::poll()
{
    m_network.poll();
    if (m_handshake && !m_holdHello)
    {
        NetInterfaceFrame frame = {};
        if (m_handshake->peekOutput(frame) == NI_READY)
        {
            CheckGateway(m_network.send(m_peer, frame) == NI_OK, "gateway application HELLO send");
            m_handshake->consumeOutput();
        }
    }
}

/** 隔离回环监听和客户端连接
@param [in,out] server 网关
@param [in,out] client 客户端
@param [in] start 是否启动监听
*/
static void ConnectGateway(NetGatewayServer& server, NetGatewayTestClient& client, bool start = true)
{
    NetInterfaceTlsOptions tls = NetInterfaceTlsTestPair::options(start, "localhost");
    tls.entropy = nullptr;
    tls.entropyContext = nullptr;
    if (start)
    {
        CheckGateway(server.listen("127.0.0.1", 0, tls) == NI_OK, "gateway listen");
    }
    tls = NetInterfaceTlsTestPair::options(false, "localhost");
    tls.entropy = nullptr;
    tls.entropyContext = nullptr;
    CheckGateway(client.m_network.connect("127.0.0.1", server.localPort(), tls, &client, client.m_peer) == NI_OK,
        "gateway client connect");
}

/** 驱动真实I/O至条件满足，外层runner约束最大时间
@param [in,out] server 网关
@param [in,out] client 客户端
@param [in] complete 完成条件
@param [in] phase 等待阶段，用于失败诊断
*/
static void WaitGateway(NetGatewayServer& server, NetGatewayTestClient& client, const std::function<bool()>& complete,
    const char* phase = "gateway operation")
{
    try
    {
        while (!complete())
        {
            netTestCheckpoint();
            server.poll();
            client.poll();
            CheckGateway(client.m_closed.empty(), "gateway client unexpectedly closed");
        }
    }
    catch (const std::exception& error)
    {
        std::ostringstream diagnostic;
        diagnostic << error.what() << " at " << phase << " pending=" << server.pendingCount() <<
            " tls=" << client.m_ready.size() << " app=" << client.m_applicationReady <<
            " received=" << client.m_received.size() << " tcp=" << client.m_network.peerCount();
        throw std::runtime_error(diagnostic.str());
    }
}

/** 构造借用的完整RPC帧
@param [in] kind REQUEST/RESPONSE
@param [in] id 原请求号或路由号
@param [in] product 产品号
@param [in] type 操作
@param [in] parent 内部父号
@param [in,out] bytes 输出信封拥有者
@return 完整借用帧
*/
static NetInterfaceFrame GatewayRpc(uint16_t kind, uint64_t id, uint32_t product, uint32_t type,
    uint64_t parent, std::vector<uint8_t>& bytes)
{
    const uint8_t body[] = { 0, 0xff, 0x80, 0x33 };
    bytes.resize(20);
    uint32_t required = 0;
    CheckGateway(NetInterfaceRpc::encode(kind, kind == NI_REQUEST ? 5000 : NI_RPC_SUCCESS, parent,
        body, sizeof(body), bytes.data(), static_cast<uint32_t>(bytes.size()), required) == NI_OK,
        "gateway RPC preparation");
    NetInterfaceFrame frame = {};
    frame.m_kind = kind;
    frame.m_requestId = id;
    frame.m_productId = product;
    frame.m_messageType = type;
    frame.m_payload = bytes.data();
    frame.m_payloadSize = required;
    return frame;
}

/** 读取测试已保存响应的协议状态
@param [in] message 自有快照
@return 网关状态
*/
static uint32_t GatewayResponseStatus(const NetInterfaceTransportMessage& message)
{
    NetInterfaceFrame frame = message.m_frame;
    frame.m_payload = message.m_payload.data();
    NetInterfaceRpcView response = {};
    CheckGateway(NetInterfaceRpc::parse(frame, response) == NI_OK, "gateway response parse");
    return response.m_value;
}

void NetGatewayTestCases::nestedRoute()
{
    // 在填充过的分配区构造真实生产端点，复现旧编译器值初始化差异。
    void* storage = ::operator new(sizeof(NetGatewayEndpoint));
    std::memset(storage, 0xa5, sizeof(NetGatewayEndpoint));
    NetGatewayEndpoint* initial = new (storage) NetGatewayEndpoint();
    uint8_t closing = 0;
    std::memcpy(&closing, &initial->m_closing, 1);
    initial->~NetGatewayEndpoint();
    ::operator delete(storage);
    CheckGateway(closing == 0, "endpoint closing flag not initialized in reused allocation");
    NetGatewayTestSink sink;
    NetGatewayServer gateway(&sink);
    NetGatewayTestClient client;
    uint64_t cook = 0;
    uint64_t account = 0;
    CheckGateway(gateway.addService(4, false, 32, cook) == NI_OK &&
        gateway.addService(0, true, 32, account) == NI_OK, "gateway trusted service slots");
    ConnectGateway(gateway, client);
    WaitGateway(gateway, client, [&]() { return client.m_applicationReady; });
    const uint64_t original = UINT64_C(0xfedcba9876543210);
    std::vector<uint8_t> bytes;
    NetInterfaceFrame request = GatewayRpc(NI_REQUEST, original, 4, 700, 0, bytes);
    CheckGateway(client.m_network.send(client.m_peer, request) == NI_OK, "gateway root request");
    WaitGateway(gateway, client, [&]() { return sink.m_deliveries.size() == 1; });
    const NetGatewayTestDelivery outer = sink.m_deliveries[0];
    CheckGateway(outer.m_endpoint == cook && outer.m_origin.m_endpoint != 0 &&
        outer.m_origin.m_platform == 3 && outer.m_origin.m_device[0] == 0x42 &&
        outer.m_message.m_frame.m_requestId != original && gateway.pendingCount() == 1,
        "gateway did not attach root context or replace route A");
    const uint64_t innerOriginal = UINT64_C(0x8123456789abcdef);
    request = GatewayRpc(NI_REQUEST, innerOriginal, 4, 900, outer.m_message.m_frame.m_requestId, bytes);
    CheckGateway(gateway.receiveService(cook, request) == NI_OK && sink.m_deliveries.size() == 2,
        "gateway internal request B");
    const NetGatewayTestDelivery inner = sink.m_deliveries[1];
    CheckGateway(inner.m_endpoint == account && inner.m_origin.m_endpoint == outer.m_origin.m_endpoint &&
        inner.m_message.m_frame.m_requestId != outer.m_message.m_frame.m_requestId && gateway.pendingCount() == 2,
        "gateway lost independent B or trusted origin");
    NetInterfaceFrame response = GatewayRpc(NI_RESPONSE, inner.m_message.m_frame.m_requestId, 4, 900, 0, bytes);
    CheckGateway(gateway.receiveService(account, response) == NI_OK && sink.m_deliveries.size() == 3,
        "gateway account response");
    CheckGateway(sink.m_deliveries[2].m_endpoint == cook &&
        sink.m_deliveries[2].m_message.m_frame.m_requestId == innerOriginal && gateway.pendingCount() == 1,
        "gateway response B went to wrong original request");
    response = GatewayRpc(NI_RESPONSE, outer.m_message.m_frame.m_requestId, 4, 700, 0, bytes);
    CheckGateway(gateway.receiveService(cook, response) == NI_OK, "gateway Cook final response");
    WaitGateway(gateway, client, [&]() { return !client.m_received.empty(); });
    CheckGateway(client.m_received[0].m_frame.m_requestId == original &&
        GatewayResponseStatus(client.m_received[0]) == NI_RPC_SUCCESS && gateway.pendingCount() == 0,
        "gateway final A response did not preserve full identity");
    client.m_network.shutdown();
    gateway.shutdown();
    CheckGateway(gateway.pendingCount() == 0, "gateway shutdown retained requests");
}

void NetGatewayTestCases::budgetsAndHandshakeOrder()
{
    NetGatewayTestSink sink;
    NetGatewayServer gateway(&sink);
    NetGatewayTestClient first(1);
    NetGatewayTestClient second(1);
    uint64_t cook = 0;
    CheckGateway(gateway.addService(4, false, 32, cook) == NI_OK, "gateway product register");
    first.m_holdHello = true;
    ConnectGateway(gateway, first);
    WaitGateway(gateway, first, [&]() { return !first.m_ready.empty(); }, "first TLS");
    ConnectGateway(gateway, second, false);
    WaitGateway(gateway, second, [&]() { return second.m_applicationReady; }, "second application handshake");
    first.m_holdHello = false;
    WaitGateway(gateway, first, [&]() { return first.m_applicationReady; }, "delayed first application handshake");
    std::vector<uint8_t> bytes;
    NetInterfaceFrame frame = GatewayRpc(NI_REQUEST, 1, 4, 700, 0, bytes);
    CheckGateway(first.m_network.send(first.m_peer, frame) == NI_OK, "one allowed request");
    frame.m_requestId = 2;
    CheckGateway(first.m_network.send(first.m_peer, frame) == NI_OK, "one excess request");
    WaitGateway(gateway, first, [&]() { return !first.m_received.empty(); }, "negotiated limit response");
    CheckGateway(sink.m_deliveries.size() == 1 && gateway.pendingCount() == 1 &&
        GatewayResponseStatus(first.m_received[0]) == NI_RPC_BUSY, "negotiated request limit ignored");
    const uint64_t route = sink.m_deliveries[0].m_message.m_frame.m_requestId;
    NetInterfaceFrame cancel = {};
    cancel.m_kind = NI_CANCEL;
    cancel.m_requestId = 1;
    cancel.m_productId = 4;
    cancel.m_messageType = 700;
    CheckGateway(first.m_network.send(first.m_peer, cancel) == NI_OK, "cancel root request");
    WaitGateway(gateway, first, [&]() { return sink.m_deliveries.size() == 2; }, "cancel delivery");
    CheckGateway(sink.m_deliveries[1].m_message.m_frame.m_kind == NI_CANCEL &&
        sink.m_deliveries[1].m_message.m_frame.m_requestId == route && gateway.pendingCount() == 0,
        "cancel did not translate original to routed ID");
    sink.m_busy = true;
    frame.m_requestId = 3;
    CheckGateway(first.m_network.send(first.m_peer, frame) == NI_OK, "request against stalled local sink");
    WaitGateway(gateway, first, [&]() { return first.m_received.size() == 2; }, "local sink backpressure response");
    CheckGateway(GatewayResponseStatus(first.m_received[1]) == NI_RPC_BUSY && gateway.pendingCount() == 0 &&
        sink.m_closed.empty(), "local backpressure blocked gateway or closed usable service");
    first.m_network.shutdown();
    second.m_network.shutdown();
    gateway.shutdown();
}

void NetGatewayTestCases::serviceGeneration()
{
    NetGatewayTestSink sink;
    NetGatewayServer gateway(&sink);
    NetGatewayTestClient client;
    uint64_t cook = 0;
    uint64_t wrong = 0;
    CheckGateway(gateway.addService(4, false, 32, cook) == NI_OK &&
        gateway.addService(5, false, 32, wrong) == NI_OK, "register independent services");
    ConnectGateway(gateway, client);
    WaitGateway(gateway, client, [&]() { return client.m_applicationReady; }, "generation handshake");
    std::vector<uint8_t> bytes;
    NetInterfaceFrame frame = GatewayRpc(NI_REQUEST, 1, 99, 700, 0, bytes);
    CheckGateway(client.m_network.send(client.m_peer, frame) == NI_OK, "unavailable product request");
    WaitGateway(gateway, client, [&]() { return client.m_received.size() == 1; }, "unknown product response");
    CheckGateway(GatewayResponseStatus(client.m_received[0]) == NI_RPC_UNAVAILABLE &&
        gateway.pendingCount() == 0, "unknown product was not rejected");
    frame = GatewayRpc(NI_REQUEST, 2, 4, 700, 0, bytes);
    CheckGateway(client.m_network.send(client.m_peer, frame) == NI_OK, "root before service loss");
    WaitGateway(gateway, client, [&]() { return sink.m_deliveries.size() == 1; }, "request before service loss");
    const uint64_t route = sink.m_deliveries[0].m_message.m_frame.m_requestId;
    frame = GatewayRpc(NI_RESPONSE, route, 4, 700, 0, bytes);
    CheckGateway(gateway.receiveService(wrong, frame) == NI_PROTOCOL_ERROR && sink.m_closed.count(wrong) &&
        gateway.pendingCount() == 1, "wrong service completed another target response");
    gateway.closeEndpoint(cook, NI_NOT_FOUND);
    WaitGateway(gateway, client, [&]() { return client.m_received.size() == 2; }, "service loss response");
    CheckGateway(GatewayResponseStatus(client.m_received[1]) == NI_RPC_UNAVAILABLE &&
        gateway.pendingCount() == 0, "service loss left root pending");
    uint64_t replacement = 0;
    CheckGateway(gateway.addService(4, false, 32, replacement) == NI_OK && replacement > cook &&
        gateway.receiveService(cook, frame) == NI_NOT_FOUND &&
        gateway.receiveService(replacement, frame) == NI_OK && client.m_received.size() == 2,
        "old service response crossed new generation");
    client.m_network.shutdown();
    gateway.shutdown();
}

void NetGatewayTestCases::clientSession()
{
    NetGatewayTestSink sink;
    NetGatewaySessionProbe events;
    NetGatewayServer gateway(&sink);
    auto serverTls = NetInterfaceTlsTestPair::options(true, "localhost");
    serverTls.entropy = nullptr;
    serverTls.entropyContext = nullptr;
    CheckGateway(gateway.listen("127.0.0.1", 0, serverTls) == NI_OK, "session gateway listen");
    uint64_t service = 0;
    CheckGateway(gateway.addService(4, false, 4, service) == NI_OK, "session service register");
    auto callbacks = events.callbacks();
    CheckGateway(NetPeer::instance().clientCreate(&callbacks, &events.m_client) == NI_OK, "session C create");
    NetWorkClientOptions options = {};
    options.structSize = sizeof(options);
    options.version = 1;
    options.address = "127.0.0.1";
    options.port = gateway.localPort();
    options.tls = NetInterfaceTlsTestPair::options(false, "localhost");
    options.tls.entropy = nullptr;
    options.tls.entropyContext = nullptr;
    options.platform = 2;
    options.capabilities = 7;
    options.maxRequests = 4;
    options.maxChunk = 4096;
    options.handshakeTimeoutMs = 5000;
    std::memset(options.device, 0x42, 16);
    CheckGateway(NetPeer::instance().clientConnect(events.m_client, &options) == NI_OK, "session C connect");
    while (events.m_generation == 0)
    {
        netTestCheckpoint();
        gateway.poll();
        CheckGateway(NetPeer::instance().clientPoll(events.m_client) == NI_READY && !events.m_bad, "session handshake");
    }
    const std::vector<uint8_t> body = {0, 17, 0, 255, 42};
    uint64_t request = 0;
    CheckGateway(NetPeer::instance().clientBegin(events.m_client, 4, 700, 5000, &request) == NI_OK && request == 1 &&
        NetPeer::instance().clientPost(events.m_client, request, body.data(), static_cast<uint32_t>(body.size())) == NI_OK,
        "session request admission");
    while (sink.m_deliveries.empty())
    {
        netTestCheckpoint();
        gateway.poll();
        NetPeer::instance().clientPoll(events.m_client);
    }
    const auto& delivery = sink.m_deliveries.back();
    NetInterfaceFrame frame = delivery.m_message.m_frame;
    frame.m_payload = delivery.m_message.m_payload.data();
    NetInterfaceRpcView rpc = {};
    CheckGateway(NetInterfaceRpc::parse(frame, rpc) == NI_OK && rpc.m_parent == 0 &&
        delivery.m_origin.m_platform == 2 && delivery.m_origin.m_device[0] == 0x42,
        "session route context");
    std::vector<uint8_t> reply(rpc.m_size + 16);
    uint32_t required = 0;
    CheckGateway(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, rpc.m_data, rpc.m_size,
        reply.data(), static_cast<uint32_t>(reply.size()), required) == NI_OK, "session reply encode");
    frame.m_kind = NI_RESPONSE;
    frame.m_payload = reply.data();
    frame.m_payloadSize = required;
    CheckGateway(gateway.receiveService(service, frame) == NI_OK, "session gateway reply");
    while (events.m_completions == 0)
    {
        netTestCheckpoint();
        gateway.poll();
        NetPeer::instance().clientPoll(events.m_client);
    }
    CheckGateway(!events.m_bad && events.m_status == NI_RPC_SUCCESS && events.m_lastId == request && events.m_body == body,
        "session binary result");
    CheckGateway(NetPeer::instance().clientBegin(events.m_client, 4, 700, 5000, &request) == NI_OK &&
        NetPeer::instance().clientCancel(events.m_client, request) == NI_OK && events.m_completions == 1,
        "session cancellation admission must not invoke callback inline");
    while (events.m_completions < 2)
    {
        netTestCheckpoint();
        gateway.poll();
        NetPeer::instance().clientPoll(events.m_client);
    }
    CheckGateway(events.m_completions == 2 && events.m_lastId == request && events.m_status == NI_RPC_CANCELLED &&
        NetPeer::instance().clientCancel(events.m_client, request) == NI_NOT_FOUND && !events.m_bad,
        "session unsent cancellation unique terminal");
    CheckGateway(NetPeer::instance().clientBegin(events.m_client, 4, 700, 100, &request) == NI_OK, "session timeout reserve");
    while (events.m_completions < 3)
    {
        netTestCheckpoint();
        gateway.poll();
        NetPeer::instance().clientPoll(events.m_client);
    }
    CheckGateway(events.m_status == NI_RPC_TIMEOUT && !events.m_bad, "session total deadline");
    CheckGateway(NetPeer::instance().clientBegin(events.m_client, 4, 700, 5000, &request) == NI_OK &&
        NetPeer::instance().clientClose(events.m_client, 0) == NI_OK, "session close pending");
    while (events.m_closes == 0)
    {
        netTestCheckpoint();
        gateway.poll();
        NetPeer::instance().clientPoll(events.m_client);
    }
    CheckGateway(events.m_completions == 4 && events.m_status == NI_RPC_CANCELLED && !events.m_bad,
        "session close completes pending once");
    CheckGateway(NetPeer::instance().clientConnect(events.m_client, &options) == NI_OK, "session reconnect admission");
    while (events.m_generation < 2)
    {
        netTestCheckpoint();
        gateway.poll();
        NetPeer::instance().clientPoll(events.m_client);
    }
    const uint64_t previous = request;
    CheckGateway(!events.m_bad && NetPeer::instance().clientBegin(events.m_client, 4, 700, 5000, &request) == NI_OK && request > previous,
        "session new generation must not reuse an old request id");
    CheckGateway(NetPeer::instance().clientDestroy(events.m_client) == NI_INVALID_STATE, "session active destroy rejected");
    NetPeer::instance().clientClose(events.m_client, 0);
    while (NetPeer::instance().clientPoll(events.m_client) != NI_OK) { netTestCheckpoint(); gateway.poll(); }
    CheckGateway(NetPeer::instance().clientDestroy(events.m_client) == NI_OK, "session owned destruction");
    events.m_client = nullptr;
    CheckGateway(events.m_completions == 5 && events.m_closes == 2 && !events.m_bad, "session final lifecycle");
}

void NetGatewayTestCases::sessionCallbackDeadline()
{
    NetGatewayTestSink sink;
    NetGatewaySessionProbe events;
    NetGatewayServer gateway(&sink);
    NetInterfaceTlsOptions serverTls = NetInterfaceTlsTestPair::options(true, "localhost");
    serverTls.entropy = nullptr;
    serverTls.entropyContext = nullptr;
    CheckGateway(gateway.listen("127.0.0.1", 0, serverTls) == NI_OK, "callback deadline listen");
    NetWorkClientCallbacks callbacks = events.callbacks();
    CheckGateway(NetPeer::instance().clientCreate(&callbacks, &events.m_client) == NI_OK, "callback deadline create");
    NetWorkClientOptions options = {};
    options.structSize = sizeof(options);
    options.version = 1;
    options.address = "127.0.0.1";
    options.port = gateway.localPort();
    options.tls = NetInterfaceTlsTestPair::options(false, "localhost");
    options.tls.entropy = nullptr;
    options.tls.entropyContext = nullptr;
    options.platform = 2;
    options.capabilities = 7;
    options.maxRequests = 4;
    options.maxChunk = 4096;
    options.handshakeTimeoutMs = 5000;
    std::memset(options.device, 0x42, sizeof(options.device));
    CheckGateway(NetPeer::instance().clientConnect(events.m_client, &options) == NI_OK, "callback deadline connect");
    while (events.m_generation == 0)
    {
        netTestCheckpoint();
        gateway.poll();
        NetPeer::instance().clientPoll(events.m_client);
    }
    uint64_t first = 0;
    events.m_reissueOnTimeout = true;
    CheckGateway(NetPeer::instance().clientBegin(events.m_client, 4, 700, 1, &first) == NI_OK, "callback deadline reserve");
    while (events.m_completions == 0)
    {
        netTestCheckpoint();
        gateway.poll();
        NetPeer::instance().clientPoll(events.m_client);
    }
    CheckGateway(!events.m_bad && events.m_status == NI_RPC_TIMEOUT && events.m_lastId == first &&
        events.m_reissueResult == NI_OK && events.m_reissuedId > first, "timeout callback new operation rejected");
    uint64_t next = 0;
    CheckGateway(NetPeer::instance().clientBegin(events.m_client, 4, 700, 1000, &next) == NI_OK,
        "poll reused its old timestamp after timeout callback and poisoned the request table");
    CheckGateway(NetPeer::instance().clientCancel(events.m_client, next) == NI_OK, "callback deadline cancel independent request");
    while (events.m_completions < 3)
    {
        netTestCheckpoint();
        gateway.poll();
        NetPeer::instance().clientPoll(events.m_client);
    }
    CheckGateway(!events.m_bad && events.m_completions == 3 && events.m_lastId == events.m_reissuedId &&
        events.m_status == NI_RPC_TIMEOUT && events.m_closes == 0 && sink.m_deliveries.empty(),
        "new request lost its timeout or an unsent request reached the service");
    CheckGateway(NetPeer::instance().clientDestroy(events.m_client) == NI_INVALID_STATE, "callback deadline active destroy rejected");
    NetPeer::instance().clientClose(events.m_client, 0);
    while (NetPeer::instance().clientPoll(events.m_client) != NI_OK) { netTestCheckpoint(); gateway.poll(); }
    CheckGateway(NetPeer::instance().clientDestroy(events.m_client) == NI_OK, "callback deadline destroy");
    events.m_client = nullptr;
    CheckGateway(events.m_completions == 3 && events.m_closes == 1 && !events.m_bad, "callback deadline unique terminal results");
}

void NetGatewayTestCases::fileRouting()
{
    NetGatewayTestSink sink;
    NetGatewayServer gateway(&sink);
    NetGatewayTestClient client;
    uint64_t service = 0;
    uint64_t common = 0;
    CheckGateway(gateway.addService(4, false, 32, service) == NI_OK &&
        gateway.addService(0, true, 32, common) == NI_OK, "file route service registration");
    ConnectGateway(gateway, client);
    WaitGateway(gateway, client, [&]() { return client.m_applicationReady; });
    std::vector<uint8_t> bytes;
    NetInterfaceFrame request = GatewayRpc(NI_REQUEST, 1, 4, NI_FILE_OPEN_UPLOAD, 0, bytes);
    CheckGateway(client.m_network.send(client.m_peer, request) == NI_OK, "file open request");
    WaitGateway(gateway, client, [&]() { return sink.m_deliveries.size() == 1; });
    const uint64_t route = sink.m_deliveries[0].m_message.m_frame.m_requestId;
    NetInterfaceFileInfo info = {};
    info.direction = 1;
    info.state = NI_FILE_TRANSFERRING;
    info.total = 3;
    info.chunk = 1024;
    info.window = 2;
    info.stream = UINT64_C(0x9912345678abcdef);
    info.expiresUtc = static_cast<uint64_t>(std::time(nullptr)) + 1800;
    std::memset(info.transfer, 0x42, 16);
    std::memset(info.token, 0x13, 32);
    std::memset(info.scope, 0x77, 16);
    CheckGateway(NetInterface::sha256(reinterpret_cast<const uint8_t*>("abc"), 3, info.hash, 32) == NI_OK, "file route SHA");
    uint8_t metadata[NET_INTERFACE_FILE_INFO_SIZE] = {};
    CheckGateway(NetInterfaceFile::encodeInfo(info, metadata, sizeof(metadata)) == NI_OK, "file info encode");
    bytes.resize(sizeof(metadata) + 16);
    uint32_t required = 0;
    CheckGateway(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, metadata, sizeof(metadata),
        bytes.data(), static_cast<uint32_t>(bytes.size()), required) == NI_OK, "file reply RPC");
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_RESPONSE;
    frame.m_productId = 4;
    frame.m_requestId = route;
    frame.m_messageType = NI_FILE_OPEN_UPLOAD;
    frame.m_payload = bytes.data();
    frame.m_payloadSize = required;
    CheckGateway(gateway.receiveService(service, frame) == NI_OK, "file service reply");
    WaitGateway(gateway, client, [&]() { return client.m_received.size() == 1; });
    auto received = client.m_received[0].m_frame;
    received.m_payload = client.m_received[0].m_payload.data();
    NetInterfaceRpcView rpc = {};
    NetInterfaceFileInfo publicInfo = {};
    CheckGateway(NetInterfaceRpc::parse(received, rpc) == NI_OK && rpc.m_value == NI_RPC_SUCCESS &&
        NetInterfaceFile::parseInfo(rpc.m_data, rpc.m_size, publicInfo) == NI_OK &&
        publicInfo.stream != info.stream && publicInfo.stream != 0, "file public stream mapping");
    NetInterfaceFileData data = {};
    data.size = 3;
    data.bytes = reinterpret_cast<const uint8_t*>("abc");
    std::memcpy(data.token, info.token, 32);
    std::memcpy(data.hash, info.hash, 32);
    bytes.resize(NET_INTERFACE_FILE_DATA_HEADER_SIZE + 3);
    CheckGateway(NetInterfaceFile::encodeData(data, bytes.data(), static_cast<uint32_t>(bytes.size()), required) == NI_OK,
        "file chunk encode");
    frame.m_kind = NI_DATA;
    frame.m_requestId = publicInfo.stream;
    frame.m_messageType = NI_FILE_DATA_TYPE;
    frame.m_payload = bytes.data();
    frame.m_payloadSize = required;
    CheckGateway(client.m_network.send(client.m_peer, frame) == NI_OK, "file source send");
    WaitGateway(gateway, client, [&]() { return sink.m_deliveries.size() == 2; });
    const auto& forwarded = sink.m_deliveries[1];
    CheckGateway(forwarded.m_endpoint == service && forwarded.m_message.m_frame.m_requestId == info.stream &&
        forwarded.m_origin.m_endpoint == sink.m_deliveries[0].m_origin.m_endpoint, "file stream source mapping");
    NetInterfaceFileAck ack = {};
    ack.offset = 3;
    std::memcpy(ack.token, info.token, 32);
    bytes.resize(NET_INTERFACE_FILE_ACK_SIZE);
    CheckGateway(NetInterfaceFile::encodeAck(ack, bytes.data(), static_cast<uint32_t>(bytes.size())) == NI_OK, "file ack encode");
    frame.m_kind = NI_DATA_ACK;
    frame.m_requestId = info.stream;
    frame.m_payload = bytes.data();
    frame.m_payloadSize = static_cast<uint32_t>(bytes.size());
    CheckGateway(gateway.receiveService(service, frame) == NI_OK, "file ack return");
    WaitGateway(gateway, client, [&]() { return client.m_received.size() == 2; });
    CheckGateway(client.m_received[1].m_frame.m_kind == NI_DATA_ACK &&
        client.m_received[1].m_frame.m_requestId == publicInfo.stream, "file ack did not restore public stream");
    bytes.resize(40);
    CheckGateway(NetInterfaceEvent::encode(0, 1, info.scope, 16, bytes.data(), static_cast<uint32_t>(bytes.size()), required) == NI_OK,
        "file revoke scope encode");
    frame.m_kind = NI_EVENT;
    frame.m_productId = 0;
    frame.m_requestId = 1;
    frame.m_messageType = NI_FILE_REVOKE_SCOPE;
    frame.m_payload = bytes.data();
    frame.m_payloadSize = required;
    CheckGateway(gateway.receiveService(common, frame) == NI_OK, "file scope revoke");
    WaitGateway(gateway, client, [&]() { return client.m_received.size() == 3; });
    const auto& stopped = client.m_received[2];
    auto noticeFrame = stopped.m_frame;
    noticeFrame.m_payload = stopped.m_payload.data();
    NetInterfaceFileNotice notice = {};
    CheckGateway(NetInterfaceFile::parseNotice(noticeFrame, notice) == NI_OK && notice.result == NI_CANCELLED &&
        noticeFrame.m_requestId == publicInfo.stream && sink.m_deliveries.back().m_message.m_frame.m_kind == NI_EVENT,
        "file revoke did not notify both ends");
    client.m_network.shutdown();
    gateway.shutdown();
}

void NetGatewayTestCases::senderDelivery()
{
    runSenderDelivery(false);
}

void NetGatewayTestCases::senderMixedOrder()
{
    runSenderDelivery(true);
}

void NetGatewayTestCases::runSenderDelivery(bool orderedMix)
{
    NetGatewayTestSink sink;
    NetGatewaySessionProbe events;
    NetGatewayServer gateway(&sink);
    NetInterfaceTlsOptions tls = NetInterfaceTlsTestPair::options(true, "localhost");
    tls.entropy = nullptr;
    tls.entropyContext = nullptr;
    CheckGateway(gateway.listen("127.0.0.1", 0, tls) == NI_OK, "sender delivery listen");
    uint64_t service = 0;
    CheckGateway(gateway.addService(4, false, 4, service) == NI_OK, "sender delivery service");
    NetWorkClientCallbacks callbacks = events.callbacks();
    // 本测试仅接纳普通回显与文件OPEN；其余字段仍由共同观察者逐项校验。
    callbacks.result = [](void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type,
        uint32_t status, const uint8_t* bytes, uint32_t size) {
        NetGatewaySessionProbe* value = static_cast<NetGatewaySessionProbe*>(context);
        value->m_bad = value->m_bad || (type != 700 && type != NI_FILE_OPEN_UPLOAD);
        NetGatewaySessionProbe::Result(context, generation, id, product, 700, status, bytes, size);
    };
    CheckGateway(NetPeer::instance().clientCreate(&callbacks, &events.m_client) == NI_OK, "sender delivery create");
    NetPeerClientOptions options = {};
    options.structSize = sizeof(options); options.version = 1;
    options.address = "127.0.0.1"; options.port = gateway.localPort();
    options.tls = NetInterfaceTlsTestPair::options(false, "localhost");
    options.tls.entropy = nullptr; options.tls.entropyContext = nullptr;
    options.platform = 3; options.capabilities = 7; options.maxRequests = 4; options.maxChunk = 4096;
    options.handshakeTimeoutMs = 5000;
    std::memset(options.device, 0x42, sizeof(options.device));
    CheckGateway(NetPeer::instance().clientConnect(events.m_client, &options) == NI_OK, "sender delivery connect");
    while (events.m_generation == 0)
    {
        netTestCheckpoint(); gateway.poll(); NetPeer::instance().clientPoll(events.m_client);
    }
    for (uint32_t mode = 0; mode < 2; ++mode)
    {
        std::vector<uint8_t> body(65537);
        for (uint32_t i = 0; i < body.size(); ++i) { body[i] = static_cast<uint8_t>(i * 31U + 17U); }
        const std::vector<uint8_t> expected = body;
        uint64_t id = 0;
        CheckGateway(NetPeer::instance().clientBegin(events.m_client, 4, 700, 5000, &id) == NI_OK, "delivery reserve");
        const size_t before = sink.m_deliveries.size();
        const int32_t sent = mode == 0 ? NetPeer::instance().clientPost(events.m_client, id, body.data(), static_cast<uint32_t>(body.size())) :
            NetPeer::instance().clientSend(events.m_client, id, body.data(), static_cast<uint32_t>(body.size()));
        CheckGateway(sent == NI_OK && events.m_completions == mode, "send must not wait for response");
        std::memset(body.data(), 0xee, body.size());
        // send的保证是交给底层运输，不是远端在20毫秒内完成调度。
        // 只推进接收网关，始终不再poll发送Peer；若仍留在Peer队列，5秒后必定失败。
        const uint64_t until = GatewayTestNow() + (mode == 0 ? 20 : 5000);
        while (GatewayTestNow() < until && (mode == 0 || sink.m_deliveries.size() == before))
        {
            netTestCheckpoint();
            gateway.poll();
            CSystem::sleepPrecise(1);
        }
        if (mode == 0)
        {
            CheckGateway(sink.m_deliveries.size() == before, "post transmitted without subsequent Peer progress");
            while (sink.m_deliveries.size() == before)
            {
                netTestCheckpoint(); gateway.poll(); NetPeer::instance().clientPoll(events.m_client);
            }
        }
        CheckGateway(sink.m_deliveries.size() == before + 1, "send returned before raw network handoff");
        const NetGatewayTestDelivery delivered = sink.m_deliveries.back();
        NetInterfaceFrame frame = delivered.m_message.m_frame;
        frame.m_payload = delivered.m_message.m_payload.data();
        NetInterfaceRpcView rpc = {};
        CheckGateway(NetInterfaceRpc::parse(frame, rpc) == NI_OK && rpc.m_size == expected.size() &&
            std::memcmp(rpc.m_data, expected.data(), expected.size()) == 0, "sender input was not copied");
        std::vector<uint8_t> reply(rpc.m_size + 16);
        uint32_t required = 0;
        CheckGateway(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, expected.data(),
            static_cast<uint32_t>(expected.size()), reply.data(), static_cast<uint32_t>(reply.size()), required) == NI_OK, "delivery echo");
        frame.m_kind = NI_RESPONSE; frame.m_payload = reply.data(); frame.m_payloadSize = required;
        CheckGateway(gateway.receiveService(service, frame) == NI_OK, "delivery reply");
        while (events.m_completions <= mode)
        {
            netTestCheckpoint(); gateway.poll(); NetPeer::instance().clientPoll(events.m_client);
        }
        CheckGateway(!events.m_bad && events.m_lastId == id && events.m_body == expected, "delivery unique result");
    }
    uint64_t open = 0;
    CheckGateway(NetPeer::instance().clientBegin(events.m_client, 4, NI_FILE_OPEN_UPLOAD, 5000, &open) == NI_OK &&
        NetPeer::instance().clientPost(events.m_client, open, nullptr, 0) == NI_OK, "delivery file open");
    while (sink.m_deliveries.size() < 3)
    {
        netTestCheckpoint(); gateway.poll(); NetPeer::instance().clientPoll(events.m_client);
    }
    NetInterfaceFileInfo info = {};
    info.direction = 1; info.state = NI_FILE_TRANSFERRING; info.total = 6; info.chunk = 1024; info.window = 2;
    info.stream = UINT64_C(0x9912345678abcdef);
    info.expiresUtc = static_cast<uint64_t>(std::time(nullptr)) + 1800;
    std::memset(info.transfer, 0x42, 16); std::memset(info.token, 0x13, 32); std::memset(info.scope, 0x77, 16);
    CheckGateway(NetInterface::sha256(reinterpret_cast<const uint8_t*>("abcdef"), 6, info.hash, 32) == NI_OK, "delivery file hash");
    uint8_t metadata[NET_INTERFACE_FILE_INFO_SIZE] = {};
    CheckGateway(NetInterfaceFile::encodeInfo(info, metadata, sizeof(metadata)) == NI_OK, "delivery file metadata");
    std::vector<uint8_t> response(sizeof(metadata) + 16);
    uint32_t required = 0;
    CheckGateway(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, metadata, sizeof(metadata), response.data(),
        static_cast<uint32_t>(response.size()), required) == NI_OK, "delivery file response");
    NetInterfaceFrame frame = sink.m_deliveries.back().m_message.m_frame;
    frame.m_kind = NI_RESPONSE; frame.m_payload = response.data(); frame.m_payloadSize = required;
    CheckGateway(gateway.receiveService(service, frame) == NI_OK, "delivery file authorization");
    while (events.m_completions < 3)
    {
        netTestCheckpoint(); gateway.poll(); NetPeer::instance().clientPoll(events.m_client);
    }
    NetInterfaceFileInfo authorized = {};
    CheckGateway(NetInterfaceFile::parseInfo(events.m_body.data(), static_cast<uint32_t>(events.m_body.size()), authorized) == NI_OK, "delivery public stream");
    for (uint32_t mode = 0; mode < 2; ++mode)
    {
        const uint8_t* expected = reinterpret_cast<const uint8_t*>(mode == 0 ? "abc" : "def");
        NetInterfaceFileData data = {};
        data.offset = mode * 3; data.size = 3; data.bytes = expected;
        std::memcpy(data.token, authorized.token, 32);
        CheckGateway(NetInterface::sha256(expected, 3, data.hash, 32) == NI_OK, "delivery chunk hash");
        std::vector<uint8_t> bytes(NET_INTERFACE_FILE_DATA_HEADER_SIZE + 3);
        CheckGateway(NetInterfaceFile::encodeData(data, bytes.data(), static_cast<uint32_t>(bytes.size()), required) == NI_OK, "delivery encode data");
        frame = NetInterfaceFrame(); frame.m_kind = NI_DATA; frame.m_productId = 4; frame.m_requestId = authorized.stream;
        frame.m_messageType = NI_FILE_DATA_TYPE; frame.m_payload = bytes.data(); frame.m_payloadSize = required;
        const size_t before = sink.m_deliveries.size();
        const uint32_t completions = events.m_completions;
        uint64_t mixedIds[2] = {};
        uint8_t markers[2] = {7, 9};
        if (orderedMix)
        {
            CheckGateway(NetPeer::instance().clientBegin(events.m_client, 4, 700, 5000, &mixedIds[0]) == NI_OK &&
                NetPeer::instance().clientPost(events.m_client, mixedIds[0], &markers[0], 1) == NI_OK, "mixed first RPC admission");
        }
        CheckGateway((mode == 0 || orderedMix ? NetPeer::instance().clientPostData(events.m_client, &frame) : NetPeer::instance().clientSendData(events.m_client, &frame)) == NI_OK, "delivery data admission");
        if (orderedMix)
        {
            CheckGateway(NetPeer::instance().clientBegin(events.m_client, 4, 700, 5000, &mixedIds[1]) == NI_OK &&
                NetPeer::instance().clientPost(events.m_client, mixedIds[1], &markers[1], 1) == NI_OK, "mixed last RPC admission");
        }
        std::memset(bytes.data(), 0xee, bytes.size());
        // sendData同样只给接收执行者有界调度机会，不帮助发送端排空本地队列。
        const bool posted = mode == 0 || orderedMix;
        const uint64_t until = GatewayTestNow() + (posted ? 20 : 5000);
        while (GatewayTestNow() < until && (posted || sink.m_deliveries.size() == before))
        {
            netTestCheckpoint();
            gateway.poll();
            CSystem::sleepPrecise(1);
        }
        if (mode == 0 || orderedMix)
        {
            CheckGateway(sink.m_deliveries.size() == before, "postData transmitted without Peer progress");
            while (sink.m_deliveries.size() < before + (orderedMix ? 3 : 1)) { netTestCheckpoint(); gateway.poll(); NetPeer::instance().clientPoll(events.m_client); }
        }
        CheckGateway(sink.m_deliveries.size() == before + (orderedMix ? 3 : 1), "data/mixed submissions not delivered");
        if (orderedMix)
        {
            CheckGateway(sink.m_deliveries[before].m_message.m_frame.m_kind == NI_REQUEST &&
                sink.m_deliveries[before + 1].m_message.m_frame.m_kind == NI_DATA &&
                sink.m_deliveries[before + 2].m_message.m_frame.m_kind == NI_REQUEST, "RPC/DATA/RPC arrived out of submission order");
        }
        const NetGatewayTestDelivery delivered = orderedMix ? sink.m_deliveries[before + 1] : sink.m_deliveries.back();
        NetInterfaceFileData parsed = {};
        NetInterfaceFrame received = delivered.m_message.m_frame;
        received.m_payload = delivered.m_message.m_payload.data();
        CheckGateway(delivered.m_message.m_frame.m_requestId == info.stream &&
            NetInterfaceFile::parseData(received, parsed) == NI_OK &&
            parsed.offset == mode * 3 && parsed.size == 3 && std::memcmp(parsed.bytes, expected, 3) == 0, "delivery data copy and route");
        if (orderedMix)
        {
            for (uint32_t index = 0; index < 2; ++index)
            {
                NetGatewayTestDelivery copy = sink.m_deliveries[before + index * 2];
                NetInterfaceFrame reply = copy.m_message.m_frame;
                reply.m_payload = copy.m_message.m_payload.data();
                NetInterfaceRpcView rpc = {};
                CheckGateway(NetInterfaceRpc::parse(reply, rpc) == NI_OK && rpc.m_size == 1 && rpc.m_data[0] == markers[index], "mixed RPC body order");
                uint8_t encoded[17] = {}; uint32_t required = 0;
                CheckGateway(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, rpc.m_data, rpc.m_size, encoded, sizeof(encoded), required) == NI_OK, "mixed response encode");
                reply.m_kind = NI_RESPONSE; reply.m_payload = encoded; reply.m_payloadSize = required;
                CheckGateway(gateway.receiveService(service, reply) == NI_OK, "mixed response association");
            }
            while (events.m_completions < completions + 2) { netTestCheckpoint(); gateway.poll(); NetPeer::instance().clientPoll(events.m_client); }
            CheckGateway(!events.m_bad && events.m_lastId == mixedIds[1] && events.m_body.size() == 1 && events.m_body[0] == markers[1], "mixed original callback ID");
        }
    }
    NetPeer::instance().clientClose(events.m_client, 0);
    while (NetPeer::instance().clientPoll(events.m_client) != NI_OK) { netTestCheckpoint(); gateway.poll(); }
    CheckGateway(NetPeer::instance().clientDestroy(events.m_client) == NI_OK && !events.m_bad, "delivery shutdown");
    events.m_client = nullptr;
}

void NetGatewayTestCases::senderRequestOrder()
{
    NetGatewayTestSink sink;
    NetGatewaySessionProbe events;
    NetGatewayServer gateway(&sink);
    NetInterfaceTlsOptions tls = NetInterfaceTlsTestPair::options(true, "localhost");
    tls.entropy = nullptr; tls.entropyContext = nullptr;
    CheckGateway(gateway.listen("127.0.0.1", 0, tls) == NI_OK, "ordered sender listen");
    uint64_t service = 0;
    CheckGateway(gateway.addService(4, false, 4, service) == NI_OK, "ordered sender service");
    NetPeerClientCallbacks callbacks = events.callbacks();
    CheckGateway(NetPeer::instance().clientCreate(&callbacks, &events.m_client) == NI_OK, "ordered sender create");
    NetPeerClientOptions options = {};
    options.structSize = sizeof(options); options.version = 1; options.address = "127.0.0.1"; options.port = gateway.localPort();
    options.tls = NetInterfaceTlsTestPair::options(false, "localhost"); options.tls.entropy = nullptr; options.tls.entropyContext = nullptr;
    options.platform = 2; options.capabilities = 7; options.maxRequests = 4; options.maxChunk = 4096; options.handshakeTimeoutMs = 5000;
    std::memset(options.device, 0x42, sizeof(options.device));
    CheckGateway(NetPeer::instance().clientConnect(events.m_client, &options) == NI_OK, "ordered sender connect");
    while (events.m_generation == 0) { netTestCheckpoint(); gateway.poll(); NetPeer::instance().clientPoll(events.m_client); }
    uint64_t low = 0, high = 0;
    const uint8_t first = 7, second = 9;
    CheckGateway(NetPeer::instance().clientBegin(events.m_client, 4, 700, 5000, &low) == NI_OK &&
        NetPeer::instance().clientBegin(events.m_client, 4, 700, 5000, &high) == NI_OK && high > low &&
        NetPeer::instance().clientPost(events.m_client, high, &second, 1) == NI_OK, "high body ready before lower body");
    // 等待真实接收条件；保留low未提交，不能靠固定20毫秒的调度假设判断阻塞。
    const uint64_t until = GatewayTestNow() + 5000;
    while (GatewayTestNow() < until && sink.m_deliveries.empty())
    {
        netTestCheckpoint();
        gateway.poll();
        NetPeer::instance().clientPoll(events.m_client);
        CSystem::sleepPrecise(1);
    }
    CheckGateway(sink.m_deliveries.size() == 1 && !events.m_bad && events.m_closes == 0,
        "a mere lower reservation must not block an earlier actual submission");
    CheckGateway(NetPeer::instance().clientSend(events.m_client, low, &first, 1) == NI_OK, "late lower body sync handoff");
    while (sink.m_deliveries.size() < 2) { netTestCheckpoint(); gateway.poll(); NetPeer::instance().clientPoll(events.m_client); }
    for (size_t index = 0; index < 2; ++index)
    {
        const NetGatewayTestDelivery delivered = sink.m_deliveries[index];
        NetInterfaceFrame frame = delivered.m_message.m_frame; frame.m_payload = delivered.m_message.m_payload.data();
        NetInterfaceRpcView rpc = {};
        CheckGateway(NetInterfaceRpc::parse(frame, rpc) == NI_OK && rpc.m_size == 1 && rpc.m_data[0] == (index == 0 ? second : first),
            "wire request order and bodies");
        std::vector<uint8_t> response(17); uint32_t required = 0;
        CheckGateway(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, rpc.m_data, rpc.m_size, response.data(), 17, required) == NI_OK, "ordered echo");
        frame.m_kind = NI_RESPONSE; frame.m_payload = response.data(); frame.m_payloadSize = required;
        CheckGateway(gateway.receiveService(service, frame) == NI_OK, "ordered response");
    }
    while (events.m_completions < 2) { netTestCheckpoint(); gateway.poll(); NetPeer::instance().clientPoll(events.m_client); }
    CheckGateway(!events.m_bad && events.m_closes == 0 && events.m_lastId == low && events.m_body.size() == 1 &&
        events.m_body[0] == first, "ordered public request association");
    uint64_t waiting = 0, expired = 0;
    CheckGateway(NetPeer::instance().clientBegin(events.m_client, 4, 700, 5000, &waiting) == NI_OK &&
        NetPeer::instance().clientBegin(events.m_client, 4, 700, 25, &expired) == NI_OK, "ordered timeout reserve");
    CSystem::sleepPrecise(30);
    CheckGateway(NetPeer::instance().clientSend(events.m_client, expired, &second, 1) == NI_TIMEOUT, "unsent request retains its original reservation deadline");
    while (events.m_completions < 3) { netTestCheckpoint(); gateway.poll(); NetPeer::instance().clientPoll(events.m_client); }
    uint64_t generation = 0; int32_t ready = 0;
    CheckGateway(NetPeer::instance().clientState(events.m_client, &generation, &ready) == NI_OK && ready == 1 && events.m_closes == 0 &&
        events.m_lastId == expired && events.m_status == NI_RPC_TIMEOUT && sink.m_deliveries.size() == 2,
        "unsent timeout must retain healthy connection and not send expired input");
    CheckGateway(NetPeer::instance().clientPost(events.m_client, waiting, &first, 1) == NI_OK, "lower reservation remains usable after higher timeout");
    while (sink.m_deliveries.size() < 3) { netTestCheckpoint(); gateway.poll(); NetPeer::instance().clientPoll(events.m_client); }
    NetPeer::instance().clientClose(events.m_client, 0);
    while (NetPeer::instance().clientPoll(events.m_client) != NI_OK) { netTestCheckpoint(); gateway.poll(); }
    CheckGateway(NetPeer::instance().clientDestroy(events.m_client) == NI_OK, "ordered sender shutdown");
    events.m_client = nullptr;
}

NetGatewaySessionFaultProbe::NetGatewaySessionFaultProbe(uint32_t mode) :
m_session(nullptr), m_mode(mode), m_ready(0), m_results(0), m_frames(0), m_closed(0), m_bad(false)
{

}

void NetGatewaySessionFaultProbe::onSessionReady(uint64_t generation, uint32_t requests, uint32_t chunk)
{
    ++m_ready;
    m_bad = m_bad || generation != 1 || requests == 0 || chunk < 1024;
    if (m_mode == 0)
    {
        throw std::runtime_error("controlled low session ready");
    }
}

void NetGatewaySessionFaultProbe::onSessionResult(uint64_t generation, uint64_t id, uint32_t product, uint32_t type,
    uint32_t status, const uint8_t* bytes, uint32_t size)
{
    ++m_results;
    m_bad = m_bad || generation != 1 || product != 4 || type != 700 || !m_ids.insert(id).second ||
        (status == NI_RPC_SUCCESS && (size != 1 || bytes == nullptr || bytes[0] != 7));
    if (m_session->poll())
    {
        m_bad = true;
    }
    if (m_mode >= 6 && status != (m_mode == 7 ? NI_RPC_UNAVAILABLE : NI_RPC_CANCELLED)) { m_bad = true; }
    if (m_results == 1 && (m_mode == 1 || (m_mode >= 3 && m_mode <= 5)))
    {
        throw std::runtime_error("controlled low session result");
    }
}

bool NetGatewaySessionFaultProbe::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    ++m_frames;
    m_bad = m_bad || generation != 1 || frame.m_kind != NI_EVENT || frame.m_payloadSize != 1 ||
        frame.m_payload == nullptr || frame.m_payload[0] != 7;
    throw std::runtime_error("controlled low session frame");
}

void NetGatewaySessionFaultProbe::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)reason;
    (void)detail;
    ++m_closed;
    m_bad = m_bad || generation != 1 || m_closed != 1;
    if (m_mode == 5)
    {
        throw std::runtime_error("controlled low session closed");
    }
}

void NetGatewayTestCases::lowSessionCallbackFailures()
{
    for (uint32_t mode = 0; mode < 6; ++mode)
    {
        NetGatewayTestSink sink;
        NetGatewayServer gateway(&sink);
        NetGatewaySessionFaultProbe events(mode);
        NetTransport channel;
        NetSession session;
        events.m_session = &session;
        bool escaped = false;
        try
        {
            NetInterfaceTlsOptions tls = NetInterfaceTlsTestPair::options(true, "localhost");
            tls.entropy = nullptr;
            tls.entropyContext = nullptr;
            CheckGateway(gateway.listen("127.0.0.1", 0, tls) == NI_OK, "low callback gateway listen");
            uint64_t service = 0;
            CheckGateway(gateway.addService(4, false, 8, service) == NI_OK, "low callback real service");
            tls = NetInterfaceTlsTestPair::options(false, "localhost");
            tls.entropy = CSystem::secureRandomBytesCallback;
            tls.entropyContext = nullptr;
            NetInterfaceHandshakeOptions hello = {};
            hello.m_platform = 2;
            hello.m_capabilities = 7;
            hello.m_maxRequests = 8;
            hello.m_maxChunk = 4096;
            hello.m_timeoutMs = 5000;
            std::memset(hello.m_device, 0x42, sizeof(hello.m_device));
            CheckGateway(session.bindChannel(&channel) == NI_OK &&
                session.connect("127.0.0.1", gateway.localPort(), tls, hello, &events) == NI_OK, "low callback connect");
            const uint64_t deadline = NetSenderSubmission::now() + 5000;
            while (events.m_ready == 0 && events.m_closed == 0 && NetSenderSubmission::now() < deadline)
            {
                netTestCheckpoint();
                gateway.poll();
                session.poll();
                CSystem::sleepPrecise(1);
            }
            CheckGateway(events.m_ready == 1, "low callback actual handshake");
            if (mode != 0)
            {
                const uint32_t count = mode == 5 ? 3 : 1;
                uint64_t id = 0;
                for (uint32_t index = 0; index < count; ++index)
                {
                    CheckGateway(session.beginRequest(4, 700, mode == 4 ? 1 : 5000, id) == NI_OK, "low callback reserve");
                }
                if (mode == 3)
                {
                    try
                    {
                        CheckGateway(session.cancel(id) == NI_OK, "low callback cancel result");
                    }
                    catch (...)
                    {
                        escaped = true;
                    }
                }
                else if (mode == 5)
                {
                    session.close(0);
                }
                else if (mode == 1 || mode == 2)
                {
                    const uint8_t body = 7;
                    CheckGateway(session.sendRequest(id, &body, 1) == NI_OK, "low callback request");
                    while (sink.m_deliveries.empty() && NetSenderSubmission::now() < deadline)
                    {
                        netTestCheckpoint();
                        gateway.poll();
                        session.poll();
                        CSystem::sleepPrecise(1);
                    }
                    CheckGateway(sink.m_deliveries.size() == 1, "low callback actual request delivery");
                    const NetGatewayTestDelivery& request = sink.m_deliveries.front();
                    NetInterfaceFrame reply = request.m_message.m_frame;
                    uint8_t payload[32] = {};
                    uint32_t required = 0;
                    reply.m_kind = mode == 1 ? NI_RESPONSE : NI_EVENT;
                    if (mode == 1)
                    {
                        CheckGateway(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, &body, 1, payload, sizeof(payload), required) == NI_OK,
                            "low callback response envelope");
                    }
                    else
                    {
                        CheckGateway(NetInterfaceEvent::encode(request.m_origin.m_endpoint, 0, &body, 1, payload, sizeof(payload), required) == NI_OK,
                            "low callback event envelope");
                    }
                    reply.m_payload = payload;
                    reply.m_payloadSize = required;
                    CheckGateway(gateway.receiveService(service, reply) == NI_OK, "low callback authenticated service reply");
                }
            }
            while (events.m_closed == 0 && NetSenderSubmission::now() < deadline)
            {
                netTestCheckpoint();
                gateway.poll();
                try
                {
                    session.poll();
                }
                catch (...)
                {
                    escaped = true;
                    break;
                }
                CSystem::sleepPrecise(1);
            }
        }
        catch (...)
        {
            channel.shutdown();
            session.shutdown();
            throw;
        }
        // 先收敛实际运输，即使改前会话的callback标志已坏，也不留下借用回调目标。
        channel.shutdown();
        session.shutdown();
        const uint32_t expected = mode == 0 ? 0 : mode == 5 ? 3 : 1;
        std::printf("LOW_SESSION_CALLBACK mode=%u ready=%u results=%u frames=%u closed=%u escaped=%u bad=%u\n", mode,
            events.m_ready, events.m_results, events.m_frames, events.m_closed, escaped ? 1U : 0U, events.m_bad ? 1U : 0U);
        CheckGateway(!escaped && !events.m_bad && events.m_closed == 1 && events.m_results == expected &&
            events.m_frames == (mode == 2 ? 1U : 0U), "low session callback fault lost a terminal or escaped its boundary");
        CheckGateway(session.bindChannel(nullptr) == NI_OK, "low session callback/poll flags were not restored");
    }
}

void NetGatewayTestCases::lowSessionCloseStatus()
{
    for (uint32_t mode = 6; mode < 10; ++mode)
    {
        NetGatewayTestSink sink;
        NetGatewayServer gateway(&sink);
        NetGatewaySessionFaultProbe events(mode);
        NetTransport channel;
        NetSession session;
        events.m_session = &session;
        NetInterfaceTlsOptions tls = NetInterfaceTlsTestPair::options(true, "localhost");
        tls.entropy = nullptr; tls.entropyContext = nullptr;
        CheckGateway(gateway.listen("127.0.0.1", 0, tls) == NI_OK, "close status listen");
        uint64_t service = 0;
        CheckGateway(gateway.addService(4, false, 8, service) == NI_OK, "close status service");
        tls = NetInterfaceTlsTestPair::options(false, "localhost");
        tls.entropy = CSystem::secureRandomBytesCallback; tls.entropyContext = nullptr;
        NetInterfaceHandshakeOptions hello = {};
        hello.m_platform = 1; hello.m_capabilities = 7; hello.m_maxRequests = 8;
        hello.m_maxChunk = 4096; hello.m_timeoutMs = 5000;
        std::memset(hello.m_device, 0x71, sizeof(hello.m_device));
        CheckGateway(session.bindChannel(&channel) == NI_OK &&
            session.connect("127.0.0.1", gateway.localPort(), tls, hello, &events) == NI_OK, "close status connect");
        const uint64_t deadline = NetSenderSubmission::now() + 5000;
        while (events.m_ready == 0 && NetSenderSubmission::now() < deadline)
        {
            netTestCheckpoint(); gateway.poll(); session.poll(); CSystem::sleepPrecise(1);
        }
        CheckGateway(events.m_ready == 1, "close status real ready");
        uint64_t first = 0, pending = 0;
        CheckGateway(session.beginRequest(4, 700, 5000, first) == NI_OK, "close status first reserve");
        for (uint32_t index = 0; index < 2; ++index)
        {
            CheckGateway(session.beginRequest(4, 700, 5000, pending) == NI_OK, "close status unsent reserve");
        }
        const uint8_t payload = 7;
        CheckGateway(session.sendRequest(first, &payload, 1) == NI_OK, "close status sent request");
        while (sink.m_deliveries.empty() && NetSenderSubmission::now() < deadline)
        {
            netTestCheckpoint(); gateway.poll(); session.poll(); CSystem::sleepPrecise(1);
        }
        CheckGateway(sink.m_deliveries.size() == 1, "close status actual remote admission");
        if (mode == 7) { gateway.shutdown(); }
        else if (mode == 8) { session.shutdown(); }
        else { session.close(mode == 9 ? NI_CANCELLED : NI_OK); }
        while (events.m_closed == 0 && NetSenderSubmission::now() < deadline)
        {
            netTestCheckpoint(); gateway.poll(); session.poll(); CSystem::sleepPrecise(1);
        }
        channel.shutdown(); session.shutdown();
        std::printf("LOCAL_CLOSE_STATUS mode=%u results=%u closed=%u bad=%u\n", mode, events.m_results, events.m_closed, events.m_bad ? 1U : 0U);
        CheckGateway(events.m_results == 3 && events.m_ids.size() == 3 && events.m_closed == 1 && !events.m_bad,
            "normal local stop must cancel every request; remote loss must remain unavailable");
    }
}