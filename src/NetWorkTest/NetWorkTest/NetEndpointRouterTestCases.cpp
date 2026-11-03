#include "NetEndpointRouterTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include <cstring>

NetEndpointRouterTestFrame::NetEndpointRouterTestFrame(uint64_t endpoint, const NetInterfaceFrame& frame,
    const NetInterfaceOrigin& origin) : m_endpoint(endpoint), m_frame(frame), m_origin(origin)
{
    if (frame.m_payloadSize != 0) { m_payload.assign(frame.m_payload, frame.m_payload + frame.m_payloadSize); }
    m_frame.m_payload = nullptr;
}

NetInterfaceFrame NetEndpointRouterTestFrame::view() const
{
    NetInterfaceFrame frame = m_frame;
    frame.m_payload = m_payload.empty() ? nullptr : m_payload.data();
    return frame;
}

int32_t NetEndpointRouterTestSink::send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin)
{
    m_frames.push_back(NetEndpointRouterTestFrame(endpoint, frame, origin));
    return NI_OK;
}

void NetEndpointRouterTestSink::close(uint64_t endpoint)
{
    m_closed.push_back(endpoint);
}

/** 构造普通RPC测试信封，实际编解码仍由公共核心完成
@param [in] kind 请求或响应
@param [in] id 来源号/服务路由号
@param [in] parent 内部父号
@param [in] content 单字节业务差异
@return 拥有信封的帧
*/
static NetEndpointRouterTestFrame EndpointTestRpc(uint16_t kind, uint64_t id, uint64_t parent, uint8_t content)
{
    uint8_t payload[17] = {};
    uint32_t size = 0;
    NetInterfaceTlsTestPair::require(NetInterfaceRpc::encode(kind, kind == NI_REQUEST ? 5000 : NI_RPC_SUCCESS,
        parent, &content, 1, payload, sizeof(payload), size) == NI_OK, "endpoint test RPC encode failed");
    NetInterfaceFrame frame = {};
    frame.m_kind = kind;
    frame.m_productId = 1;
    frame.m_messageType = 0x7201;
    frame.m_requestId = id;
    frame.m_payload = payload;
    frame.m_payloadSize = size;
    return NetEndpointRouterTestFrame(0, frame, NetInterfaceOrigin());
}

void NetEndpointRouterTestCases::attachedEndpoints()
{
    NetEndpointRouterTestSink services;
    NetEndpointRouterTestSink clients;
    NetEndpointRouter router(&services, &clients);
    uint64_t dependency = 0;
    uint64_t handler = 0;
    NetInterfaceTlsTestPair::require(router.addService(0, true, 8, dependency) == NI_OK &&
        router.addHandler(1, 8, handler) == NI_OK, "endpoint handler/dependency registration failed");
    NetInterfaceOrigin first = {};
    first.m_endpoint = 1001;
    first.m_platform = 3;
    std::memset(first.m_device, 1, sizeof(first.m_device));
    NetInterfaceOrigin second = first;
    second.m_endpoint = 1002;
    second.m_device[0] = 2;
    const uint32_t negotiated[] = {3, 15, 8, 65536};
    uint64_t a = 0;
    uint64_t b = 0;
    uint64_t rejected = 99;
    const uint32_t unknownCapabilities[] = {3, 31, 8, 65536};
    NetInterfaceTlsTestPair::require(router.addClient(first, unknownCapabilities, rejected) == NI_INVALID_ARGUMENT && rejected == 0,
        "attached endpoint accepted an unassigned capability bit");
    NetInterfaceTlsTestPair::require(router.addClient(first, negotiated, a) == NI_OK &&
        router.addClient(second, negotiated, b) == NI_OK && a != b && a != first.m_endpoint && b != second.m_endpoint &&
        router.addClient(second, negotiated, rejected) == NI_INVALID_ARGUMENT && rejected == 0,
        "attached source duplicate/public-vs-private identity separation failed");
    const NetEndpointRouterTestFrame requestA = EndpointTestRpc(NI_REQUEST, 1, 0, 0x41);
    const NetEndpointRouterTestFrame requestB = EndpointTestRpc(NI_REQUEST, 1, 0, 0x42);
    NetInterfaceTlsTestPair::require(router.receiveClient(a, requestA.view()) == NI_OK && router.receiveClient(b, requestB.view()) == NI_OK &&
        services.m_frames.size() == 2 && services.m_frames[0].m_endpoint == handler && services.m_frames[1].m_endpoint == handler &&
        services.m_frames[0].m_frame.m_requestId != services.m_frames[1].m_frame.m_requestId &&
        services.m_frames[0].m_origin.m_endpoint == 1001 && services.m_frames[1].m_origin.m_endpoint == 1002,
        "equal original IDs crossed clients or authoritative origins changed");
    const uint64_t parentA = services.m_frames[0].m_frame.m_requestId;
    const uint64_t parentB = services.m_frames[1].m_frame.m_requestId;
    const NetEndpointRouterTestFrame child = EndpointTestRpc(NI_REQUEST, 1, parentA, 0x43);
    NetInterfaceTlsTestPair::require(router.receiveService(handler, child.view()) == NI_OK && services.m_frames.size() == 3 &&
        services.m_frames[2].m_endpoint == dependency && services.m_frames[2].m_origin.m_endpoint == 1001,
        "attached source child did not inherit its real root origin");
    const NetEndpointRouterTestFrame childReply = EndpointTestRpc(NI_RESPONSE, services.m_frames[2].m_frame.m_requestId, 0, 0x43);
    NetInterfaceTlsTestPair::require(router.receiveService(dependency, childReply.view()) == NI_OK && services.m_frames.size() == 4 &&
        services.m_frames[3].m_endpoint == handler && services.m_frames[3].m_frame.m_kind == NI_RESPONSE &&
        services.m_frames[3].m_frame.m_requestId == 1 && services.m_frames[3].m_origin.m_endpoint == 1001,
        "child reply did not restore caller's original ID/context");
    const NetEndpointRouterTestFrame replyB = EndpointTestRpc(NI_RESPONSE, parentB, 0, 0x42);
    NetInterfaceTlsTestPair::require(router.receiveService(handler, replyB.view()) == NI_OK && clients.m_frames.size() == 1 &&
        clients.m_frames[0].m_endpoint == b && clients.m_frames[0].m_frame.m_requestId == 1 && clients.m_frames[0].m_origin.m_endpoint == 1002,
        "root reply returned to the wrong same-ID client");
    NetInterfaceFrame cancel = {};
    cancel.m_kind = NI_CANCEL;
    cancel.m_productId = 1;
    cancel.m_requestId = 1;
    cancel.m_messageType = 0x7201;
    NetInterfaceTlsTestPair::require(router.receiveClient(a, cancel) == NI_OK && router.pendingCount() == 0 &&
        services.m_frames.back().m_frame.m_kind == NI_CANCEL && services.m_frames.back().m_frame.m_requestId == parentA,
        "root cancellation did not reach the mapped service request");
    uint8_t bytes[25] = {};
    uint32_t size = 0;
    const uint8_t content = 0x45;
    NetInterfaceFrame event = {};
    event.m_kind = NI_EVENT;
    event.m_productId = 1;
    event.m_messageType = 0x7202;
    event.m_requestId = 1;
    NetInterfaceTlsTestPair::require(NetInterfaceEvent::encode(1002, 0, &content, 1, bytes, sizeof(bytes), size) == NI_OK,
        "encode public origin event failed");
    event.m_payload = bytes;
    event.m_payloadSize = size;
    NetInterfaceTlsTestPair::require(router.receiveService(handler, event) == NI_OK && clients.m_frames.size() == 2 &&
        clients.m_frames.back().m_endpoint == b && clients.m_frames.back().m_payload.size() == 1 &&
        clients.m_frames.back().m_payload[0] == content, "public source event did not resolve to its private attached endpoint");
    NetInterfaceTlsTestPair::require(NetInterfaceEvent::encode(a, 0, &content, 1, bytes, sizeof(bytes), size) == NI_OK,
        "encode hidden endpoint probe failed");
    NetInterfaceTlsTestPair::require(router.receiveService(handler, event) == NI_OK && clients.m_frames.size() == 2,
        "a guessed internal endpoint bypassed public source targeting");
    const NetEndpointRouterTestFrame illegal = EndpointTestRpc(NI_REQUEST, 2, parentA, 0x49);
    NetInterfaceTlsTestPair::require(router.receiveClient(a, illegal.view()) == NI_PROTOCOL_ERROR && clients.m_closed.size() == 1 &&
        clients.m_closed[0] == a, "external source forged an internal parent");
    router.poll();
    NetEndpointRouterTestSink genericServices;
    NetEndpointRouterTestSink genericClients;
    NetEndpointRouter generic(&genericServices, &genericClients);
    uint64_t genericHandler = 0;
    uint64_t genericClient = 0;
    NetInterfaceTlsTestPair::require(generic.addHandler(0, 8, genericHandler) == NI_OK &&
        generic.addClient(first, negotiated, genericClient) == NI_OK && generic.receiveClient(genericClient, requestA.view()) == NI_OK &&
        genericServices.m_frames.size() == 1 && genericServices.m_frames[0].m_endpoint == genericHandler,
        "generic internal handler did not preserve the caller product");

    NetEndpointRouterTestSink unavailableServices;
    NetEndpointRouterTestSink unavailableClients;
    NetEndpointRouter unavailable(&unavailableServices, &unavailableClients);
    uint64_t waitingHandler = 0;
    uint64_t waitingClient = 0;
    NetInterfaceTlsTestPair::require(unavailable.addHandler(1, 8, waitingHandler) == NI_OK &&
        unavailable.addClient(first, negotiated, waitingClient) == NI_OK &&
        unavailable.receiveClient(waitingClient, requestA.view()) == NI_OK,
        "unavailable dependency fixture failed");
    const uint64_t waitingParent = unavailableServices.m_frames[0].m_frame.m_requestId;
    const NetEndpointRouterTestFrame missingChild = EndpointTestRpc(NI_REQUEST, 1, waitingParent, 0x50);
    NetInterfaceRpcView failure = {};
    NetInterfaceTlsTestPair::require(unavailable.receiveService(waitingHandler, missingChild.view()) == NI_OK &&
        unavailableServices.m_frames.size() == 2 &&
        NetInterfaceRpc::parse(unavailableServices.m_frames.back().view(), failure) == NI_OK && failure.m_value == NI_RPC_UNAVAILABLE &&
        unavailableServices.m_frames.back().m_origin.m_endpoint == first.m_endpoint &&
        unavailableServices.m_frames.back().m_origin.m_platform == first.m_platform &&
        std::memcmp(unavailableServices.m_frames.back().m_origin.m_device, first.m_device, sizeof(first.m_device)) == 0 &&
        unavailable.pendingCount() == 1 && unavailableServices.m_closed.empty(),
        "unavailable dependency lost authoritative origin or closed healthy root");
    NetInterfaceFrame childCancel = cancel;
    childCancel.m_requestId = 2;
    const NetEndpointRouterTestFrame cancelledChild = EndpointTestRpc(NI_REQUEST, 2, waitingParent, 0x51);
    NetInterfaceTlsTestPair::require(unavailable.receiveService(waitingHandler, childCancel) == NI_OK &&
        unavailable.receiveService(waitingHandler, cancelledChild.view()) == NI_OK &&
        NetInterfaceRpc::parse(unavailableServices.m_frames.back().view(), failure) == NI_OK && failure.m_value == NI_RPC_CANCELLED &&
        unavailableServices.m_frames.back().m_origin.m_endpoint == first.m_endpoint &&
        unavailable.pendingCount() == 1 && unavailableServices.m_closed.empty(),
        "rejected child lost parent context before insertion into waiting table");
    const NetEndpointRouterTestFrame waitingReply = EndpointTestRpc(NI_RESPONSE, waitingParent, 0, 0x41);
    NetInterfaceTlsTestPair::require(unavailable.receiveService(waitingHandler, waitingReply.view()) == NI_OK &&
        unavailableClients.m_frames.size() == 1 && unavailableClients.m_frames[0].m_origin.m_endpoint == first.m_endpoint &&
        unavailable.pendingCount() == 0, "root no longer usable after dependency failure");
}

// NET_ENDPOINT_ROUTER_REDRAFT_TESTS
#include "NetInterface/NetInterfaceAPI.h"
#include <algorithm>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#endif
#include <ctime>

/** 构造带固定观察期的已认证端点，只供测试
@param [in,out] router 被测实例
@param [in] publicId 宿主来源号
@param [out] endpoint 私有路由代次
@return 登记结果
*/
static int32_t RouterTestClient(NetEndpointRouter& router, uint64_t publicId, uint64_t& endpoint)
{
    NetEndpointOrigin origin = {};
    origin.m_endpoint = publicId;
    origin.m_platform = 1;
    std::memset(origin.m_device, 1, sizeof(origin.m_device));
    const uint32_t negotiated[] = {1, 7, 128, 65536};
    return router.addClient(origin, negotiated, endpoint);
}

/** 构造测试RPC，同步调用期间借用输出缓冲区
@param [in] kind 请求/回复
@param [in] product 产品
@param [in] type 类型
@param [in] id 来源号或路由号
@param [in] parent 内部父号
@param [in] bytes 消息体
@param [in] size 体长度
@param [out] payload 至少size+16
@return 完整帧
*/
static NetInterfaceFrame RouterTestRpc(uint16_t kind, uint32_t product, uint32_t type, uint64_t id, uint64_t parent,
    const uint8_t* bytes, uint32_t size, uint8_t* payload)
{
    NetInterfaceFrame frame = {};
    frame.m_kind = kind;
    frame.m_productId = product;
    frame.m_messageType = type;
    frame.m_requestId = id;
    frame.m_payload = payload;
    NetInterfaceRpc::encode(kind, kind == NI_REQUEST ? 60000 : NI_RPC_SUCCESS, parent, bytes, size, payload, size + 16, frame.m_payloadSize);
    return frame;
}

/** 构造合法上传流元数据，不代表实际业务授权
@param [in] local 服务本地流号
@return 元数据
*/
static NetInterfaceFileInfo RouterTestInfo(uint64_t local)
{
    NetInterfaceFileInfo info = {};
    info.direction = 1;
    info.state = NI_FILE_TRANSFERRING;
    info.chunk = 1024;
    info.window = 2;
    info.total = 4096;
    info.stream = local;
    info.expiresUtc = static_cast<uint64_t>(std::time(nullptr)) + 1200;
    std::memcpy(info.transfer, &local, sizeof(local));
    std::memset(info.token, 0x41, sizeof(info.token));
    std::memset(info.scope, 0x51, sizeof(info.scope));
    return info;
}

/** 走真实RPC接入并返回网关改写后的文件结果
@param [in,out] router 被测路由
@param [in,out] services 观测服务端目标
@param [in,out] clients 观测客户端回复
@param [in] client 客户端代次
@param [in] service 服务端代次
@param [in] product 产品号
@param [in] original 客户端严格递增号
@param [in] info 服务授权的元数据
@param [out] mapped 公开文件信息
@return RPC结果或错误
*/
static int32_t RouterTestOpen(NetEndpointRouter& router, NetEndpointProbeSink& services, NetEndpointProbeSink& clients,
    uint64_t client, uint64_t service, uint32_t product, uint64_t original, const NetInterfaceFileInfo& info, NetInterfaceFileInfo& mapped)
{
    uint8_t payload[176] = {};
    NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, product, NI_FILE_OPEN_UPLOAD, original, 0, nullptr, 0, payload);
    if (router.receiveClient(client, request) != NI_OK) { return NI_INVALID_STATE; }
    const uint64_t route = services.m_frame.m_requestId;
    uint8_t data[160] = {};
    if (NetInterfaceFile::encodeInfo(info, data, sizeof(data)) != NI_OK)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame reply = RouterTestRpc(NI_RESPONSE, product, NI_FILE_OPEN_UPLOAD, route, 0, data, sizeof(data), payload);
    if (router.receiveService(service, reply) != NI_OK) { return NI_INVALID_STATE; }
    NetInterfaceRpcView view = {};
    if (clients.m_frame.m_requestId != original || NetInterfaceRpc::parse(clients.m_frame, view) != NI_OK)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (view.m_value != NI_RPC_SUCCESS) { return static_cast<int32_t>(view.m_value); }
    return NetInterfaceFile::parseInfo(view.m_data, view.m_size, mapped);
}

/** 测试随机不用于生产安全
@param [in] context 未使用
@param [out] bytes 填充区域
@param [in] size 长度
@return 成功
*/
static int32_t RouterTestEntropy(void* context, uint8_t* bytes, uint32_t size)
{
    (void)context;
    std::memset(bytes, 0x52, size);
    return NI_OK;
}

void NetEndpointRouterTestCases::routerLifecycle()
{
#define R_CHECK(condition) NetInterfaceTlsTestPair::require((condition), #condition)
    // 监听失败、推进异常、宿主静止及停止异常都要收敛，不保留虚假的监听状态。
    for (uint32_t mode = 0; mode < 6; ++mode)
    {
        NetEndpointProbeTransport transport;
        NetEndpointProbeSink services;
        NetEndpointRouter router(&services);
        NetInterfaceTlsOptions tls = {};
        tls.entropy = RouterTestEntropy;
        R_CHECK(router.bindTransport(&transport) == NI_OK && router.localPort() == 0);
        transport.m_fault = 64;
        R_CHECK(router.poll()); // 尚未监听时，宿主静止不代表故障。
        transport.m_fault = mode == 0 ? 1 : mode == 1 ? 32 : mode == 2 ? 3 : 0;
        const int32_t started = router.listen("127.0.0.1", 0, tls, 8);
        if (mode < 3)
        {
            R_CHECK(started < 0 && router.localPort() == 0 && transport.m_stopped == 1);
            transport.m_fault = 0;
            if (mode < 2)
            {
                R_CHECK(router.listen("127.0.0.1", 0, tls, 8) == NI_OK && router.localPort() == 1025);
            }
            else
            {
                R_CHECK(router.listen("127.0.0.1", 0, tls, 8) == NI_INVALID_STATE);
            }
        }
        else
        {
            R_CHECK(started == NI_OK && router.listen("127.0.0.1", 0, tls, 8) == NI_INVALID_STATE && transport.m_listened == 1);
            transport.m_fault = 4;
            R_CHECK(router.localPort() == 0);
            transport.m_fault = mode == 3 ? 10 : mode == 5 ? 64 : 2;
            if (mode == 3 || mode == 5) { R_CHECK(!router.poll() && router.localPort() == 0); }
        }
        router.shutdown();
        R_CHECK(!router.poll() && router.pendingCount() == 0);
        const uint32_t stopped = transport.m_stopped;
        router.shutdown();
        R_CHECK(transport.m_stopped == stopped);
    }
    // 来源输出参数与输入来源号重叠，不得破坏原来源校验；关闭回调异常不丢收尾任务。
    {
        NetEndpointProbeSink services;
        NetEndpointProbeSink clients;
        NetEndpointRouter router(&services, &clients);
        uint64_t handler = 0;
        R_CHECK(router.addHandler(101, 128, handler) == NI_OK);
        NetEndpointOrigin origin = {};
        origin.m_endpoint = 1001;
        origin.m_platform = 1;
        std::memset(origin.m_device, 1, sizeof(origin.m_device));
        const uint32_t negotiated[] = {1, 7, 128, 65536};
        R_CHECK(router.addClient(origin, negotiated, origin.m_endpoint) == NI_OK && origin.m_endpoint != 1001);
        uint8_t payload[16] = {};
        NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, 101, 11, 1, 0, nullptr, 0, payload);
        R_CHECK(router.receiveClient(origin.m_endpoint, request) == NI_OK && router.pendingCount() == 1 && services.m_origin.m_endpoint == 1001);
        services.m_fault = 2;
        router.closeEndpoint(handler, NI_CANCELLED);
        router.closeEndpoint(handler, NI_CANCELLED);
        R_CHECK(services.m_closed == 1 && router.poll() && router.pendingCount() == 0);
        NetInterfaceRpcView failure = {};
        R_CHECK(NetInterfaceRpc::parse(clients.m_frame, failure) == NI_OK && failure.m_value == NI_RPC_UNAVAILABLE);
        uint64_t replacement = 0;
        R_CHECK(router.addHandler(101, 128, replacement) == NI_OK && replacement > handler);
    }
    // 混合多个关闭端点，不让一个端点的128条在途独占整轮回收。
    {
        NetEndpointProbeSink services;
        NetEndpointProbeSink clients;
        NetEndpointRouter router(&services, &clients);
        uint64_t handler[3] = {};
        uint64_t client[3] = {};
        uint8_t bytes[16] = {};
        for (uint32_t group = 0; group < 3; ++group)
        {
            R_CHECK(router.addHandler(101 + group, 128, handler[group]) == NI_OK && RouterTestClient(router, 1001 + group, client[group]) == NI_OK);
            for (uint64_t id = 1; id <= 128; ++id)
            {
                const NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, 101 + group, 11, id, 0, nullptr, 0, bytes);
                R_CHECK(router.receiveClient(client[group], request) == NI_OK);
            }
            router.closeEndpoint(handler[group], NI_CANCELLED);
        }
        R_CHECK(router.pendingCount() == 384 && router.poll() && router.pendingCount() == 320);
        R_CHECK(clients.m_endpoint == client[0]);
        for (uint32_t round = 0; round < 8; ++round) { router.poll(); }
        R_CHECK(router.pendingCount() == 0 && services.m_closed == 3 && clients.m_sent == 384);
    }
    // 回调只请求停止，不能在借用输出帧时释放路由对象或再次推进。
    {
        NetEndpointProbeSink services;
        NetEndpointProbeSink clients;
        NetEndpointRouter router(&services, &clients);
        uint64_t service = 0;
        uint64_t client = 0;
        R_CHECK(router.addHandler(101, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK);
        services.m_fault = 8;
        services.m_router = &router;
        uint8_t bytes[16] = {};
        const NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, 101, 11, 1, 0, nullptr, 0, bytes);
        R_CHECK(router.receiveClient(client, request) == NI_OK && !router.poll() && router.pendingCount() == 0 && services.m_closed == 1);
    }
    // 文件32条客户端限额；第33条父请求失败也立刻结束已发出的内部查询。
    {
        NetEndpointProbeSink services;
        NetEndpointProbeSink clients;
        NetEndpointRouter router(&services, &clients);
        uint64_t service = 0;
        uint64_t dependency = 0;
        uint64_t client = 0;
        R_CHECK(router.addHandler(101, 128, service) == NI_OK && router.addService(0, true, 128, dependency) == NI_OK &&
            RouterTestClient(router, 1001, client) == NI_OK);
        NetInterfaceFileInfo mapped = {};
        for (uint64_t id = 1; id <= 32; ++id)
        {
            R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, id, RouterTestInfo(id + 1000), mapped) == NI_OK &&
                mapped.stream == id && router.pendingCount() == 0);
        }
        uint8_t payload[176] = {};
        NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, 101, NI_FILE_OPEN_UPLOAD, 33, 0, nullptr, 0, payload);
        R_CHECK(router.receiveClient(client, request) == NI_OK);
        const uint64_t parent = services.m_frame.m_requestId;
        request = RouterTestRpc(NI_REQUEST, 101, 11, 1, parent, nullptr, 0, payload);
        R_CHECK(router.receiveService(service, request) == NI_OK && router.pendingCount() == 2 && services.m_endpoint == dependency);
        uint8_t data[160] = {};
        const NetInterfaceFileInfo info = RouterTestInfo(1033);
        R_CHECK(NetInterfaceFile::encodeInfo(info, data, sizeof(data)) == NI_OK);
        const NetInterfaceFrame reply = RouterTestRpc(NI_RESPONSE, 101, NI_FILE_OPEN_UPLOAD, parent, 0, data, sizeof(data), payload);
        R_CHECK(router.receiveService(service, reply) == NI_OK && router.pendingCount() == 0 && services.m_closed == 0);
        NetInterfaceRpcView failure = {};
        R_CHECK(NetInterfaceRpc::parse(clients.m_frame, failure) == NI_OK && failure.m_value == NI_RPC_BUSY);
        // 取消流会释放额度，正常绑定能复用额度但永不复用公开流号。
        NetInterfaceFrame cancel = {};
        cancel.m_kind = NI_CANCEL;
        cancel.m_productId = 101;
        cancel.m_messageType = NI_FILE_DATA_TYPE;
        cancel.m_requestId = 1;
        R_CHECK(router.receiveClient(client, cancel) == NI_OK);
        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 34, RouterTestInfo(1034), mapped) == NI_OK && mapped.stream == 33);
        const uint64_t sameStream = mapped.stream;
        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 35, RouterTestInfo(1034), mapped) == NI_OK && mapped.stream == sameStream);
        NetInterfaceFileInfo forged = RouterTestInfo(1034);
        forged.token[0] ^= 1;
        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 36, forged, mapped) == NI_RPC_FAILED && services.m_closed == 1);
        R_CHECK(router.poll() && router.pendingCount() == 0);
    }
    // DATA及最终ACK背压完全无副作用；成功终态才释放流额度。
    for (uint32_t direction = 1; direction <= 2; ++direction)
    {
        NetEndpointProbeSink services;
        NetEndpointProbeSink clients;
        NetEndpointRouter router(&services, &clients);
        uint64_t service = 0;
        uint64_t client = 0;
        R_CHECK(router.addHandler(101, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK);
        NetInterfaceFileInfo info = RouterTestInfo(1001);
        info.direction = direction;
        NetInterfaceFileInfo mapped = {};
        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 1, info, mapped) == NI_OK);
        uint8_t payload[128] = {};
        uint8_t byte = 0x52;
        NetInterfaceFileData data = {};
        data.bytes = &byte;
        data.size = 1;
        std::memcpy(data.token, info.token, 32);
        uint32_t size = 0;
        R_CHECK(NetInterfaceFile::encodeData(data, payload, sizeof(payload), size) == NI_OK);
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_DATA;
        frame.m_productId = 101;
        frame.m_messageType = NI_FILE_DATA_TYPE;
        frame.m_requestId = direction == 1 ? mapped.stream : info.stream;
        frame.m_payload = payload;
        frame.m_payloadSize = size;
        NetEndpointProbeSink& dataSink = direction == 1 ? services : clients;
        dataSink.m_fault = 4;
        const uint32_t sent = dataSink.m_sent;
        R_CHECK((direction == 1 ? router.receiveClient(client, frame) : router.receiveService(service, frame)) == NI_LIMIT_EXCEEDED && dataSink.m_sent == sent);
        dataSink.m_fault = 0;
        R_CHECK((direction == 1 ? router.receiveClient(client, frame) : router.receiveService(service, frame)) == NI_OK && dataSink.m_sent == sent + 1 &&
            dataSink.m_frame.m_requestId == (direction == 1 ? info.stream : mapped.stream));
        NetInterfaceFileAck ack = {};
        ack.flags = 1;
        ack.offset = info.total;
        std::memcpy(ack.token, info.token, 32);
        R_CHECK(NetInterfaceFile::encodeAck(ack, payload, sizeof(payload)) == NI_OK);
        frame.m_kind = NI_DATA_ACK;
        frame.m_requestId = direction == 1 ? info.stream : mapped.stream;
        frame.m_payloadSize = NET_INTERFACE_FILE_ACK_SIZE;
        NetEndpointProbeSink& ackSink = direction == 1 ? clients : services;
        ackSink.m_fault = 4;
        R_CHECK((direction == 1 ? router.receiveService(service, frame) : router.receiveClient(client, frame)) == NI_LIMIT_EXCEEDED);
        ackSink.m_fault = 0;
        R_CHECK((direction == 1 ? router.receiveService(service, frame) : router.receiveClient(client, frame)) == NI_OK);
        const uint32_t after = ackSink.m_sent;
        R_CHECK((direction == 1 ? router.receiveService(service, frame) : router.receiveClient(client, frame)) == NI_OK && ackSink.m_sent == after);
        // 用新来源请求重建同一服务流号，公开号必须递增。
        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 2, info, mapped) == NI_OK && mapped.stream == 2);
        router.closeEndpoint(client, NI_CANCELLED);
        R_CHECK(router.poll() && services.m_closed == 0);
    }
    // 同一服务128条与全局1024条额度，跨服务同号隔离，撤销只影响授权服务的分组。
    {
        NetEndpointProbeSink services;
        NetEndpointProbeSink clients;
        NetEndpointRouter router(&services, &clients);
        uint64_t service[9] = {};
        uint64_t client[34] = {};
        for (uint32_t index = 0; index < 9; ++index)
        {
            R_CHECK(router.addHandler(101 + index, 128, service[index]) == NI_OK);
        }
        for (uint32_t index = 0; index < 34; ++index)
        {
            R_CHECK(RouterTestClient(router, 1001 + index, client[index]) == NI_OK);
        }
        NetInterfaceFileInfo mapped = {};
        for (uint32_t group = 0; group < 8; ++group)
        {
            for (uint32_t item = 0; item < 128; ++item)
            {
                R_CHECK(RouterTestOpen(router, services, clients, client[group * 4 + item / 32], service[group], 101 + group,
                    item % 32 + 1, RouterTestInfo(item + 1000), mapped) == NI_OK);
            }
            R_CHECK(RouterTestOpen(router, services, clients, client[33], service[group], 101 + group,
                group + 1, RouterTestInfo(2000), mapped) == NI_RPC_BUSY);
        }
        R_CHECK(RouterTestOpen(router, services, clients, client[32], service[8], 109, 1, RouterTestInfo(3000), mapped) == NI_RPC_BUSY);
        uint8_t eventBytes[40] = {};
        uint8_t scope[16];
        std::memset(scope, 0x51, sizeof(scope));
        uint32_t size = 0;
        R_CHECK(NetInterfaceEvent::encode(0, 1, scope, sizeof(scope), eventBytes, sizeof(eventBytes), size) == NI_OK);
        NetInterfaceFrame event = {};
        event.m_kind = NI_EVENT;
        event.m_productId = 101;
        event.m_requestId = 1;
        event.m_messageType = NI_FILE_REVOKE_SCOPE;
        event.m_payload = eventBytes;
        event.m_payloadSize = size;
        const uint32_t notices = clients.m_sent;
        R_CHECK(router.receiveService(service[0], event) == NI_OK && clients.m_sent == notices + 128);
        R_CHECK(RouterTestOpen(router, services, clients, client[32], service[8], 109, 2, RouterTestInfo(3000), mapped) == NI_OK && mapped.stream == 1025);
        R_CHECK(RouterTestOpen(router, services, clients, client[33], service[1], 102, 9, RouterTestInfo(2000), mapped) == NI_RPC_BUSY);
        router.shutdown();
        R_CHECK(!router.poll() && router.pendingCount() == 0 && services.m_closed == 9 && clients.m_closed == 34);
    }

    // 通用处理器0可以承载已授权产品文件；只按真实请求路由绑定，不能跨产品换流。
    {
        NetEndpointProbeSink services;
        NetEndpointProbeSink clients;
        NetEndpointRouter router(&services, &clients);
        uint64_t service = 0;
        uint64_t client = 0;
        R_CHECK(router.addHandler(0, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK);
        NetInterfaceFileInfo mapped = {};
        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 1, RouterTestInfo(1001), mapped) == NI_OK && mapped.stream == 1);
        R_CHECK(RouterTestOpen(router, services, clients, client, service, 102, 2, RouterTestInfo(1001), mapped) == NI_RPC_FAILED && services.m_closed == 1);
        R_CHECK(router.poll() && router.pendingCount() == 0);
    }
    // 已经过期的授权不能登记流；端点关闭后通知存活的另一端。
    {
        NetEndpointProbeSink services;
        NetEndpointProbeSink clients;
        NetEndpointRouter router(&services, &clients);
        uint64_t service = 0;
        uint64_t client = 0;
        R_CHECK(router.addHandler(101, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK);
        NetInterfaceFileInfo mapped = {};
        NetInterfaceFileInfo info = RouterTestInfo(1001);
        info.expiresUtc = static_cast<uint64_t>(std::time(nullptr)) - 1;
        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 1, info, mapped) == NI_RPC_FAILED);
        info = RouterTestInfo(1001);
        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 2, info, mapped) == NI_OK && mapped.stream == 1);
        router.closeEndpoint(service, NI_CANCELLED);
        const uint32_t sent = clients.m_sent;
        R_CHECK(router.poll() && clients.m_sent == sent + 1);
        NetInterfaceFileNotice notice = {};
        R_CHECK(NetInterfaceFile::parseNotice(clients.m_frame, notice) == NI_OK && notice.result == NI_INVALID_STATE && clients.m_frame.m_requestId == mapped.stream);
    }

    // 已有合法文件流也不能跨来源、跨产品、更换令牌、倒转方向或越过文件边界。
    for (uint32_t mode = 0; mode < 5; ++mode)
    {
        NetEndpointProbeSink services;
        NetEndpointProbeSink clients;
        NetEndpointRouter router(&services, &clients);
        uint64_t service = 0;
        uint64_t client = 0;
        uint64_t intruder = 0;
        R_CHECK(router.addHandler(101, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK &&
            RouterTestClient(router, 1002, intruder) == NI_OK);
        NetInterfaceFileInfo mapped = {};
        const NetInterfaceFileInfo info = RouterTestInfo(1001);
        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 1, info, mapped) == NI_OK);
        uint8_t byte = 0x41;
        NetInterfaceFileData data = {};
        data.bytes = &byte;
        data.size = 1;
        data.offset = mode == 4 ? info.total : 0;
        std::memcpy(data.token, info.token, 32);
        if (mode == 2) { data.token[0] ^= 1; }
        uint8_t payload[128] = {};
        uint32_t size = 0;
        R_CHECK(NetInterfaceFile::encodeData(data, payload, sizeof(payload), size) == NI_OK);
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_DATA;
        frame.m_productId = mode == 1 ? 102 : 101;
        frame.m_messageType = NI_FILE_DATA_TYPE;
        frame.m_requestId = mode == 3 ? info.stream : mapped.stream;
        frame.m_payload = payload;
        frame.m_payloadSize = size;
        const uint32_t sent = services.m_sent;
        const int32_t received = mode == 3 ? router.receiveService(service, frame) : router.receiveClient(mode == 0 ? intruder : client, frame);
        R_CHECK(received == NI_PROTOCOL_ERROR && services.m_sent == sent);
        R_CHECK(mode == 3 ? services.m_closed == 1 : clients.m_closed == 1);
        router.poll();
        if (mode == 0)
        {
            R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 2, info, mapped) == NI_OK && mapped.stream == 1);
        }
    }
    // 真实单调时间推进：128条同时到期，每轮至多回收64条，随后允许重新占用额度。
    {
        NetEndpointProbeSink services;
        NetEndpointProbeSink clients;
        NetEndpointRouter router(&services, &clients);
        uint64_t service = 0;
        uint64_t client[4] = {};
        R_CHECK(router.addHandler(101, 128, service) == NI_OK);
        for (uint32_t index = 0; index < 4; ++index) { R_CHECK(RouterTestClient(router, 1001 + index, client[index]) == NI_OK); }
        const uint64_t expires = static_cast<uint64_t>(std::time(nullptr)) + 2;
        NetInterfaceFileInfo mapped = {};
        for (uint32_t index = 0; index < 128; ++index)
        {
            NetInterfaceFileInfo info = RouterTestInfo(1000 + index);
            info.expiresUtc = expires;
            R_CHECK(RouterTestOpen(router, services, clients, client[index / 32], service, 101, index % 32 + 1, info, mapped) == NI_OK);
        }
        // 独立回归只等待一次真实2秒截止，不向生产状态机增加时钟后门。
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2010);
        while (std::chrono::steady_clock::now() < deadline)
        {
#ifdef _WIN32
            Sleep(1);
#endif
        }
        const uint32_t sent = clients.m_sent;
        R_CHECK(router.poll() && clients.m_sent == sent + 64 && services.m_closed == 0);
        R_CHECK(router.poll() && clients.m_sent == sent + 128);
        R_CHECK(router.poll() && clients.m_sent == sent + 128);
        R_CHECK(RouterTestOpen(router, services, clients, client[0], service, 101, 33, RouterTestInfo(1000), mapped) == NI_OK && mapped.stream == 129);
    }

#undef R_CHECK
}