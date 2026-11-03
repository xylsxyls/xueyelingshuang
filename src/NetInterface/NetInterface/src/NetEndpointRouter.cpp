#include "NetInterfaceMacro.h"
#include "NetEndpointRouterState.h"
#include "NetInterface.h"
#include "NetInterfaceRequests.h"
#include "NetInterfaceRpc.h"
#include "NetInterfaceFile.h"
#include "NetInterfaceEvent.h"
#include "NetInterfaceTransport.h"
#include "NetEndpointRouter.h"
#include <chrono>
#include <cstring>
#include <vector>
#include <utility>

NetEndpointServiceSink::~NetEndpointServiceSink()
{

}

NetEndpointPeer::NetEndpointPeer() :
m_id(0),
m_nextClosing(nullptr),
m_filesCollected(false),
m_firstFile(nullptr),
m_fileCount(0),
m_service(false),
m_attached(false),
m_closing(false),
m_peer(0),
m_product(0),
m_activity(0),
m_pingTime(0),
m_pingId(0)
{
    std::memset(&m_origin, 0, sizeof(m_origin));
    std::memset(m_negotiated, 0, sizeof(m_negotiated));
}

NetEndpointRouterState::NetEndpointRouterState(NetEndpointServiceSink* services, NetEndpointServiceSink* clients) :
m_transport(nullptr),
m_entropy(nullptr),
m_entropyContext(nullptr),
m_listening(false),
m_requests(4096),
m_services(services),
m_clients(clients),
m_closingFirst(nullptr),
m_closingLast(nullptr),
m_polling(false),
m_dispatching(false),
m_stopRequested(false),
m_fileExpiryCount(0),
m_lastEndpoint(1),
m_internal(0),
m_generic(0),
m_lastPing(0),
m_lastStream(0),
m_lastEvent(0),
m_status(NI_OK),
m_stopped(false)
{
    std::memset(m_fileExpiry, 0, sizeof(m_fileExpiry));
    m_status = m_requests.addEndpoint(1, true, 0, 128);
}

void NetEndpointRouterState::enqueueClosing(NetEndpointPeer* peer)
{
    peer->m_nextClosing = nullptr;
    if (m_closingLast == nullptr)
    {
        m_closingFirst = peer;
    }
    else
    {
        m_closingLast->m_nextClosing = peer;
    }
    m_closingLast = peer;
}

NetEndpointPeer* NetEndpointRouterState::takeClosing()
{
    NetEndpointPeer* peer = m_closingFirst;
    if (peer != nullptr)
    {
        m_closingFirst = peer->m_nextClosing;
        if (m_closingFirst == nullptr)
        {
            m_closingLast = nullptr;
        }
        peer->m_nextClosing = nullptr;
    }
    return peer;
}

void NetEndpointRouterState::clearPayload()
{
    if (!m_payload.empty())
    {
        NetInterface::wipe(m_payload.data(), static_cast<uint32_t>(m_payload.size()));
        m_payload.clear();
    }
    if (m_payload.capacity() > 4096)
    {
        std::vector<uint8_t>().swap(m_payload);
    }
}

uint64_t NetEndpointRouterState::now()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetEndpointRouter::NetEndpointRouter(NetEndpointServiceSink* services, NetEndpointServiceSink* clients) :
m_state(new NetEndpointRouterState(services, clients))
{

}

NetEndpointRouter::~NetEndpointRouter()
{
    shutdown();
    delete m_state;
}

int32_t NetEndpointRouter::bindTransport(NetServerChannel* transport)
{
    if ((!m_state->m_stopped && (m_state->m_listening || !m_state->m_endpoints.empty())) ||
        (transport != nullptr && (m_state->m_stopped || m_state->m_clients != nullptr)))
    {
        return NI_INVALID_STATE;
    }
    m_state->m_transport = transport;
    return NI_OK;
}

int32_t NetEndpointRouter::listen(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls, uint32_t peers)
{
    if (m_state->m_clients != nullptr || m_state->m_stopped || m_state->m_stopRequested || m_state->m_listening)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_status < 0)
    {
        return m_state->m_status;
    }
    if (m_state->m_transport == nullptr)
    {
        return NI_NOT_SUPPORTED;
    }
    if (address == nullptr || port > 65535 || peers == 0 || peers > 256 || tls.entropy == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    int32_t result = NI_INVALID_STATE;
    try
    {
        result = m_state->m_transport->listen(address, port, tls, this, peers);
    }
    catch (...)
    {
        result = NI_INVALID_STATE;
    }
    if (result == NI_OK)
    {
        m_state->m_entropy = tls.entropy;
        m_state->m_entropyContext = tls.entropyContext;
        m_state->m_listening = true;
    }
    else
    {
        // listen不交付回调；部分接纳后失败仍必须停止宿主，不能遗留监听者借用。
        try
        {
            m_state->m_transport->shutdown();
        }
        catch (...)
        {
            m_state->m_status = NI_INVALID_STATE;
        }
    }
    return result;
}

uint32_t NetEndpointRouter::localPort() const
{
    if (!m_state->m_listening || m_state->m_stopped || m_state->m_transport == nullptr)
    {
        return 0;
    }
    try
    {
        return m_state->m_transport->localPort();
    }
    catch (...)
    {
        return 0;
    }
}

uint32_t NetEndpointRouter::pendingCount() const
{
    return m_state->m_requests.count();
}

int32_t NetEndpointRouter::addService(uint32_t product, bool internal, uint32_t limit, uint64_t& endpoint)
{
    endpoint = 0;
    if (internal != (product == 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    return addPort(product, internal, false, limit, endpoint);
}

int32_t NetEndpointRouter::addHandler(uint32_t product, uint32_t limit, uint64_t& endpoint)
{
    return addPort(product, false, product == 0, limit, endpoint);
}

int32_t NetEndpointRouter::addPort(uint32_t product, bool internal, bool generic, uint32_t limit, uint64_t& endpoint)
{
    endpoint = 0;
    if (m_state->m_stopped || m_state->m_stopRequested || m_state->m_services == nullptr || (internal && generic) ||
        ((internal || generic) != (product == 0)) || limit == 0 || limit > 128 ||
        m_state->m_endpoints.size() >= 1023 || m_state->m_lastEndpoint == UINT64_MAX ||
        (internal ? m_state->m_internal != 0 : generic ? m_state->m_generic != 0 : m_state->m_products.find(product) != m_state->m_products.end()))
    {
        return NI_INVALID_ARGUMENT;
    }
    const uint64_t id = ++m_state->m_lastEndpoint;
    try
    {
        std::unique_ptr<NetEndpointPeer> value(new NetEndpointPeer());
        value->m_id = id;
        value->m_service = true;
        value->m_product = product;
        m_state->m_endpoints.insert(std::make_pair(id, std::move(value)));
        if (!internal && !generic)
        {
            m_state->m_products.insert(std::make_pair(product, id));
        }
    }
    catch (...)
    {
        m_state->m_endpoints.erase(id);
        m_state->m_products.erase(product);
        return NI_NO_MEMORY;
    }
    const int32_t result = m_state->m_requests.addEndpoint(id, true, product, limit, internal);
    if (result != NI_OK)
    {
        m_state->m_endpoints.erase(id);
        m_state->m_products.erase(product);
        return result;
    }
    if (internal)
    {
        m_state->m_internal = id;
    }
    if (generic)
    {
        m_state->m_generic = id;
    }
    endpoint = id;
    return NI_OK;
}

int32_t NetEndpointRouter::addClient(const NetEndpointOrigin& input, const uint32_t negotiated[4], uint64_t& endpoint)
{
    const NetEndpointOrigin context = input;
    endpoint = 0;
    uint8_t device = 0;
    for (uint32_t index = 0; index < sizeof(context.m_device); ++index) { device |= context.m_device[index]; }
    if (m_state->m_stopped || m_state->m_stopRequested || m_state->m_clients == nullptr || m_state->m_status < 0 || negotiated == nullptr ||
        context.m_endpoint == 0 || device == 0 || context.m_platform == 0 || context.m_platform > 3 ||
        negotiated[0] != context.m_platform || (negotiated[1] & 1) == 0 || (negotiated[1] & ~15U) != 0 ||
        negotiated[2] == 0 || negotiated[2] > 128 || negotiated[3] < 1024 || negotiated[3] > 65536 ||
        m_state->m_endpoints.size() >= 1023 || m_state->m_lastEndpoint == UINT64_MAX || m_state->m_origins.count(context.m_endpoint) != 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    const uint64_t id = ++m_state->m_lastEndpoint;
    try
    {
        std::unique_ptr<NetEndpointPeer> value(new NetEndpointPeer());
        value->m_id = id;
        value->m_attached = true;
        value->m_origin = context;
        std::memcpy(value->m_negotiated, negotiated, sizeof(value->m_negotiated));
        value->m_activity = NetEndpointRouterState::now();
        m_state->m_endpoints.insert(std::make_pair(id, std::move(value)));
        m_state->m_origins.insert(std::make_pair(context.m_endpoint, id));
    }
    catch (...)
    {
        m_state->m_endpoints.erase(id);
        m_state->m_origins.erase(context.m_endpoint);
        return NI_NO_MEMORY;
    }
    const int32_t added = m_state->m_requests.addEndpoint(id, false, 0, negotiated[2]);
    if (added != NI_OK)
    {
        m_state->m_endpoints.erase(id);
        m_state->m_origins.erase(context.m_endpoint);
        return added;
    }
    endpoint = id;
    return NI_OK;
}

int32_t NetEndpointRouter::receiveClient(uint64_t endpoint, const NetInterfaceFrame& frame)
{
    const auto found = m_state->m_endpoints.find(endpoint);
    if (found == m_state->m_endpoints.end() || !found->second->m_attached || found->second->m_closing)
    {
        return NI_NOT_FOUND;
    }
    const int32_t result = dispatch(endpoint, frame);
    if (result == NI_LIMIT_EXCEEDED && (frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK || frame.m_kind == NI_EVENT))
    {
        return result;
    }
    if (result < 0)
    {
        closeEndpoint(endpoint, result);
    }
    else { found->second->m_activity = NetEndpointRouterState::now(); }
    return result;
}

void NetEndpointRouter::onReady(uint64_t peer)
{
    const auto existing = m_state->m_peers.find(peer);
    if (existing != m_state->m_peers.end())
    {
        closeEndpoint(existing->second, NI_PROTOCOL_ERROR);
        return;
    }
    if (m_state->m_stopped || m_state->m_stopRequested || peer == 0 ||
        m_state->m_lastEndpoint == UINT64_MAX || m_state->m_endpoints.size() >= 1023)
    {
        closePeer(peer, peer == 0 ? NI_PROTOCOL_ERROR : NI_LIMIT_EXCEEDED);
        return;
    }
    const uint64_t id = ++m_state->m_lastEndpoint;
    try
    {
        NetInterfaceHandshakeOptions options = {};
        options.m_server = 1;
        options.m_capabilities = 7;
        options.m_maxRequests = 32;
        options.m_maxChunk = 65536;
        options.m_timeoutMs = 5000;
        if (NetInterface::entropyBytes(m_state->m_entropy, m_state->m_entropyContext, options.m_nonce, sizeof(options.m_nonce)) != NI_OK)
        {
            closePeer(peer, NI_INVALID_STATE);
            return;
        }
        std::unique_ptr<NetEndpointPeer> value(new NetEndpointPeer());
        value->m_id = id;
        value->m_peer = peer;
        value->m_activity = value->m_pingTime = NetEndpointRouterState::now();
        value->m_handshake.reset(new NetInterfaceHandshake(options, value->m_activity));
        m_state->m_endpoints.insert(std::make_pair(id, std::move(value)));
        m_state->m_peers.insert(std::make_pair(peer, id));
        if (m_state->m_requests.addEndpoint(id, false, 0, 32) != NI_OK)
        {
            closeEndpoint(id, NI_LIMIT_EXCEEDED);
        }
    }
    catch (...)
    {
        m_state->m_endpoints.erase(id);
        m_state->m_peers.erase(peer);
        closePeer(peer, NI_NO_MEMORY);
    }
}

bool NetEndpointRouter::onFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    const auto found = m_state->m_peers.find(peer);
    if (found == m_state->m_peers.end())
    {
        closePeer(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    const uint64_t id = found->second;
    NetEndpointPeer* endpoint = m_state->m_endpoints.find(id)->second.get();
    if (endpoint->m_closing)
    {
        return true;
    }
    const uint64_t now = NetEndpointRouterState::now();
    const int32_t accepted = endpoint->m_handshake->accept(frame, now);
    const int32_t result = accepted == NI_OK ? dispatch(id, frame) : accepted;
    if (result == NI_LIMIT_EXCEEDED && (frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK || frame.m_kind == NI_EVENT))
    {
        return false;
    }
    if (result < 0)
    {
        closeEndpoint(id, result);
    }
    else
    {
        endpoint->m_activity = now;
    }
    return true;
}

void NetEndpointRouter::onClosed(uint64_t peer, int32_t reason, int32_t detail)
{
    (void)detail;
    const auto found = m_state->m_peers.find(peer);
    if (found != m_state->m_peers.end())
    {
        closeEndpoint(found->second, reason);
    }
}

NetEndpointOrigin NetEndpointRouter::origin(uint64_t endpoint) const
{
    const auto found = m_state->m_endpoints.find(endpoint);
    return found == m_state->m_endpoints.end() ? NetEndpointOrigin() : found->second->m_origin;
}

int32_t NetEndpointRouter::deliver(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& context)
{
    const auto found = m_state->m_endpoints.find(endpoint);
    if (found == m_state->m_endpoints.end() || found->second->m_closing)
    {
        return NI_NOT_FOUND;
    }
    int32_t result = NI_INVALID_STATE;
    try
    {
        result = found->second->m_service ? m_state->m_services->send(endpoint, frame, context) :
            found->second->m_attached ? m_state->m_clients->send(endpoint, frame, context) :
                                      m_state->m_transport->send(found->second->m_peer, frame);
    }
    catch (...)
    {
        result = NI_NO_MEMORY;
    }
    return result;
}

void NetEndpointRouter::reject(uint64_t endpoint, const NetInterfaceFrame& original, uint32_t status, uint64_t root)
{
    if (root == 0)
    {
        NetInterfaceRpcView rpc = {};
        NetInterfaceRequest parent = {};
        if (original.m_kind == NI_REQUEST && NetInterfaceRpc::parse(original, rpc) == NI_OK && rpc.m_parent != 0 &&
            m_state->m_requests.parentContext(endpoint, rpc.m_parent, original.m_productId, parent) == NI_READY)
        {
            root = parent.m_origin;
        }
        else
        {
            root = endpoint;
        }
    }
    uint8_t payload[16];
    uint32_t required = 0;
    NetInterfaceRpc::encode(NI_RESPONSE, status, 0, nullptr, 0, payload, sizeof(payload), required);
    NetInterfaceFrame response = original;
    response.m_kind = NI_RESPONSE;
    response.m_payload = payload;
    response.m_payloadSize = sizeof(payload);
    if (deliver(endpoint, response, origin(root)) != NI_OK)
    {
        closeEndpoint(endpoint, NI_LIMIT_EXCEEDED);
    }
}

void NetEndpointRouter::terminate(const NetInterfaceRequest& request, uint32_t status, bool notifySource)
{
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_CANCEL;
    frame.m_productId = request.m_product;
    frame.m_messageType = request.m_type;
    frame.m_requestId = request.m_route;
    if (request.m_target != 1 && deliver(request.m_target, frame, origin(request.m_origin)) != NI_OK)
    {
        closeEndpoint(request.m_target, NI_LIMIT_EXCEEDED);
    }
    if (notifySource)
    {
        frame.m_requestId = request.m_original;
        reject(request.m_source, frame, status, request.m_origin);
    }
    // 深度只有两级；每个父请求最多128个直接子请求，不进行递归。
    NetInterfaceRequest child = {};
    while (m_state->m_requests.takeChild(request.m_route, child) == NI_READY)
    {
        NetInterfaceFrame cancel = {};
        cancel.m_kind = NI_CANCEL;
        cancel.m_productId = child.m_product;
        cancel.m_messageType = child.m_type;
        cancel.m_requestId = child.m_route;
        if (deliver(child.m_target, cancel, origin(child.m_origin)) != NI_OK)
        {
            closeEndpoint(child.m_target, NI_LIMIT_EXCEEDED);
        }
        cancel.m_requestId = child.m_original;
        reject(child.m_source, cancel, status, child.m_origin);
    }
}

int32_t NetEndpointRouter::dispatch(uint64_t endpoint, const NetInterfaceFrame& frame)
{
    if (m_state->m_stopped || m_state->m_stopRequested || m_state->m_dispatching)
    {
        return NI_INVALID_STATE;
    }
    m_state->m_dispatching = true;
    int32_t result = NI_NO_MEMORY;
    try
    {
        result = routeFrame(endpoint, frame);
    }
    catch (...)
    {
        result = NI_NO_MEMORY;
    }
    m_state->clearPayload();
    m_state->m_dispatching = false;
    return result;
}

void NetEndpointRouter::closePeer(uint64_t peer, int32_t reason)
{
    if (m_state->m_transport == nullptr)
    {
        return;
    }
    try
    {
        m_state->m_transport->close(peer, reason);
    }
    catch (...)
    {
        // 逻辑端点已经失效并排队；宿主停止异常不能跳过本地资源回收。
    }
}

int32_t NetEndpointRouter::routeFrame(uint64_t endpoint, const NetInterfaceFrame& frame)
{
    const auto found = m_state->m_endpoints.find(endpoint);
    if (found == m_state->m_endpoints.end() || found->second->m_closing)
    {
        return NI_NOT_FOUND;
    }
    NetEndpointPeer* source = found->second.get();
    const uint64_t now = NetEndpointRouterState::now();
    if (frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK ||
        (frame.m_kind == NI_EVENT && frame.m_messageType == NI_FILE_CANCEL && frame.m_payloadSize >= 4 && frame.m_payload != nullptr &&
         std::memcmp(frame.m_payload, "NFN2", 4) == 0))
    {
        return routeFile(endpoint, frame);
    }
    if (frame.m_kind == NI_EVENT)
    {
        return routeEvent(endpoint, frame);
    }
    if (frame.m_kind == NI_PING || frame.m_kind == NI_PONG)
    {
        NetInterfaceFrame response = {};
        if (source->m_service || NetInterfaceRpc::control(frame, response) < 0)
        {
            return NI_PROTOCOL_ERROR;
        }
        if (frame.m_kind == NI_PONG)
        {
            if (source->m_pingId != frame.m_requestId)
            {
                return NI_PROTOCOL_ERROR;
            }
            source->m_pingId = 0;
            return NI_OK;
        }
        return deliver(endpoint, response, NetEndpointOrigin());
    }
    if (frame.m_kind == NI_CANCEL)
    {
        NetInterfaceFrame ignored = {};
        if (NetInterfaceRpc::control(frame, ignored) != NI_OK)
        {
            return NI_PROTOCOL_ERROR;
        }
        if (frame.m_messageType == NI_FILE_DATA_TYPE)
        {
            return routeFile(endpoint, frame);
        }
        NetInterfaceRequest request = {};
        const int32_t result = m_state->m_requests.cancel(endpoint, frame.m_requestId, request);
        if (result == NI_READY)
        {
            terminate(request, NI_RPC_CANCELLED, false);
        }
        return result == NI_NOT_FOUND || result >= 0 ? NI_OK : result;
    }
    NetInterfaceRpcView rpc = {};
    if (NetInterfaceRpc::parse(frame, rpc) != NI_OK)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (frame.m_kind == NI_RESPONSE)
    {
        if (!source->m_service)
        {
            return NI_PROTOCOL_ERROR;
        }
        NetInterfaceRequest request = {};
        const int32_t result =
            m_state->m_requests.complete(endpoint, frame.m_requestId, frame.m_productId, frame.m_messageType, now, request);
        if (result == NI_NOT_FOUND)
        {
            return NI_OK;
        }
        if (result == NI_TIMEOUT || result == NI_CANCELLED)
        {
            terminate(request, result == NI_TIMEOUT ? NI_RPC_TIMEOUT : NI_RPC_CANCELLED, true);
            return NI_OK;
        }
        if (result != NI_READY)
        {
            return result;
        }
        // 父请求已取出；任何文件绑定失败也必须同步结束其内部查询。
        NetInterfaceRequest child = {};
        while (m_state->m_requests.takeChild(request.m_route, child) == NI_READY)
        {
            terminate(child, NI_RPC_CANCELLED, true);
        }
        NetInterfaceFrame response = frame;
        response.m_requestId = request.m_original;
        std::vector<uint8_t>& filePayload = m_state->m_payload;
        if (request.m_parent == 0 && NetInterfaceFile::isControl(request.m_type) && rpc.m_value == NI_RPC_SUCCESS)
        {
            const int32_t bound = bindFileReply(request, rpc, filePayload);
            if (bound != NI_OK)
            {
                reject(request.m_source, response, bound == NI_LIMIT_EXCEEDED ? NI_RPC_BUSY : NI_RPC_FAILED, request.m_origin);
                if (bound == NI_PROTOCOL_ERROR)
                {
                    closeEndpoint(endpoint, bound);
                }
                return NI_OK;
            }
            response.m_payload = filePayload.data();
            response.m_payloadSize = static_cast<uint32_t>(filePayload.size());
        }
        if (deliver(request.m_source, response, origin(request.m_origin)) != NI_OK)
        {
            closeEndpoint(request.m_source, NI_LIMIT_EXCEEDED);
        }
        return NI_OK;
    }
    if (frame.m_kind != NI_REQUEST || (source->m_service ? rpc.m_parent == 0 : rpc.m_parent != 0))
    {
        return NI_PROTOCOL_ERROR;
    }
    const auto product = m_state->m_products.find(frame.m_productId);
    uint64_t target = source->m_service ? m_state->m_internal : product == m_state->m_products.end() ? m_state->m_generic : product->second;
    if (target == 0)
    {
        target = 1;
    }
    NetInterfaceRequest request = {};
    const int32_t begun = m_state->m_requests.begin(endpoint, frame.m_requestId, target, frame.m_productId, frame.m_messageType,
                                                    rpc.m_parent, now, rpc.m_value, request);
    if (begun != NI_OK)
    {
        if (begun == NI_CANCELLED || begun == NI_LIMIT_EXCEEDED || begun == NI_TIMEOUT || (source->m_service && begun == NI_PROTOCOL_ERROR))
        {
            reject(endpoint, frame,
                   begun == NI_CANCELLED        ? NI_RPC_CANCELLED
                   : begun == NI_TIMEOUT        ? NI_RPC_TIMEOUT
                   : begun == NI_LIMIT_EXCEEDED ? NI_RPC_BUSY
                                                : NI_RPC_UNAUTHORIZED);
            return NI_OK;
        }
        return begun;
    }
    if (target == 1)
    {
        NetInterfaceRequest retired = {};
        m_state->m_requests.complete(1, request.m_route, request.m_product, request.m_type, now, retired);
        reject(endpoint, frame, NI_RPC_UNAVAILABLE, request.m_origin);
        return NI_OK;
    }
    std::vector<uint8_t>& payload = m_state->m_payload;
    payload.resize(rpc.m_size + 16);
    uint32_t required = 0;
    NetInterfaceRpc::encode(NI_REQUEST, static_cast<uint32_t>(request.m_deadline - now), request.m_parent, rpc.m_data, rpc.m_size,
                            payload.data(), static_cast<uint32_t>(payload.size()), required);
    NetInterfaceFrame forwarded = frame;
    forwarded.m_requestId = request.m_route;
    forwarded.m_payload = payload.data();
    forwarded.m_payloadSize = static_cast<uint32_t>(payload.size());
    const int32_t sent = deliver(target, forwarded, origin(request.m_origin));
    if (sent != NI_OK)
    {
        NetInterfaceRequest retired = {};
        m_state->m_requests.cancel(endpoint, request.m_original, retired);
        reject(endpoint, frame, sent == NI_LIMIT_EXCEEDED ? NI_RPC_BUSY : NI_RPC_UNAVAILABLE, request.m_origin);
        if (sent != NI_LIMIT_EXCEEDED)
        {
            closeEndpoint(target, sent);
        }
    }
    return NI_OK;
}

int32_t NetEndpointRouter::receiveService(uint64_t endpoint, const NetInterfaceFrame& frame)
{
    const auto found = m_state->m_endpoints.find(endpoint);
    if (found == m_state->m_endpoints.end() || !found->second->m_service || found->second->m_closing)
    {
        return NI_NOT_FOUND;
    }
    const int32_t result = dispatch(endpoint, frame);
    if (result < 0 &&
        !(result == NI_LIMIT_EXCEEDED && (frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK || frame.m_kind == NI_EVENT)))
    {
        closeEndpoint(endpoint, result);
    }
    return result;
}

void NetEndpointRouter::closeEndpoint(uint64_t endpoint, int32_t reason)
{
    const auto found = m_state->m_endpoints.find(endpoint);
    if (found == m_state->m_endpoints.end() || found->second->m_closing)
    {
        return;
    }
    found->second->m_closing = true;
    m_state->enqueueClosing(found->second.get());
    if (found->second->m_service)
    {
        if (m_state->m_internal == endpoint)
        {
            m_state->m_internal = 0;
        }
        else if (m_state->m_generic == endpoint)
        {
            m_state->m_generic = 0;
        }
        else
        {
            m_state->m_products.erase(found->second->m_product);
        }
        try
        {
            m_state->m_services->close(endpoint);
        }
        catch (...)
        {
            // 路由已失效，宿主关闭异常不能重新激活此代次或逃出析构。
        }
    }
    else
    {
        if (found->second->m_attached)
        {
            try
            {
                m_state->m_clients->close(endpoint);
            }
            catch (...)
            {
                // 端点已经从接入表失效，关闭队列仍负责回收资源。
            }
        }
        else
        {
            closePeer(found->second->m_peer, reason);
        }
    }
}

bool NetEndpointRouter::poll()
{
    if (m_state->m_stopped)
    {
        return false;
    }
    if (m_state->m_polling || m_state->m_dispatching)
    {
        return true;
    }
    m_state->m_polling = true;
    try
    {
        if (!m_state->m_stopRequested)
        {
            advance();
        }
    }
    catch (...)
    {
        m_state->m_status = NI_INVALID_STATE;
        m_state->m_stopRequested = true;
    }
    m_state->m_polling = false;
    if (m_state->m_stopRequested)
    {
        shutdown();
    }
    return !m_state->m_stopped;
}

bool NetEndpointRouter::advance()
{
    if (m_state->m_stopped)
    {
        return false;
    }
    if (m_state->m_transport != nullptr)
    {
        const bool active = m_state->m_transport->poll();
        if (m_state->m_listening && !active)
        {
            // 已建立的监听不应自行静止；传播终态，防止业务继续向失效宿主提交。
            m_state->m_status = NI_INVALID_STATE;
            m_state->m_stopRequested = true;
        }
    }
    if (m_state->m_stopRequested)
    {
        return false;
    }
    collectFiles(0);
    const uint64_t now = NetEndpointRouterState::now();
    for (const auto& entry : m_state->m_endpoints)
    {
        NetEndpointPeer* endpoint = entry.second.get();
        if (endpoint->m_service || endpoint->m_attached || endpoint->m_closing)
        {
            continue;
        }
        const int32_t state = endpoint->m_handshake->progress(now);
        if (state < 0)
        {
            closeEndpoint(entry.first, state);
            continue;
        }
        NetInterfaceFrame output = {};
        if (endpoint->m_handshake->peekOutput(output) == NI_READY)
        {
            const int32_t sent = deliver(entry.first, output, NetEndpointOrigin());
            if (sent == NI_OK)
            {
                endpoint->m_handshake->consumeOutput();
                uint32_t fields[4] = {};
                uint8_t nonce[16] = {};
                endpoint->m_handshake->peerInfo(fields, nonce, endpoint->m_origin.m_device);
                endpoint->m_origin.m_endpoint = entry.first;
                endpoint->m_origin.m_platform = fields[0];
                std::memcpy(endpoint->m_negotiated, fields, sizeof(fields));
                if (m_state->m_requests.setEndpointLimit(entry.first, fields[2]) != NI_OK)
                {
                    closeEndpoint(entry.first, NI_INVALID_STATE);
                }
            }
            else if (sent != NI_LIMIT_EXCEEDED)
            {
                closeEndpoint(entry.first, sent);
            }
        }
        if (now - endpoint->m_activity >= 90000 || (endpoint->m_pingId != 0 && now - endpoint->m_pingTime >= 60000))
        {
            closeEndpoint(entry.first, NI_TIMEOUT);
        }
        else if (state == NI_READY && endpoint->m_pingId == 0 && now - endpoint->m_pingTime >= 30000)
        {
            if (m_state->m_lastPing == UINT64_MAX)
            {
                closeEndpoint(entry.first, NI_LIMIT_EXCEEDED);
                continue;
            }
            NetInterfaceFrame ping = {};
            ping.m_kind = NI_PING;
            ping.m_requestId = ++m_state->m_lastPing;
            if (deliver(entry.first, ping, NetEndpointOrigin()) != NI_OK)
            {
                closeEndpoint(entry.first, NI_LIMIT_EXCEEDED);
            }
            endpoint->m_pingId = ping.m_requestId;
            endpoint->m_pingTime = now;
        }
    }
    for (uint32_t count = 0; count < 64; ++count)
    {
        NetInterfaceRequest request = {};
        if (m_state->m_requests.takeExpired(now, request) != NI_READY)
        {
            break;
        }
        terminate(request, NI_RPC_TIMEOUT, true);
    }
    for (uint32_t count = 0; count < 64; ++count)
    {
        NetEndpointPeer* endpoint = m_state->takeClosing();
        if (endpoint == nullptr)
        {
            break;
        }
        const uint64_t id = endpoint->m_id;
        if (!endpoint->m_filesCollected)
        {
            endpoint->m_filesCollected = true;
            collectFiles(id);
        }
        NetInterfaceRequest request = {};
        if (m_state->m_requests.takeEndpoint(id, request) == NI_READY)
        {
            terminate(request, NI_RPC_UNAVAILABLE, true);
            m_state->enqueueClosing(endpoint);
        }
        else
        {
            m_state->m_origins.erase(endpoint->m_origin.m_endpoint);
            m_state->m_peers.erase(endpoint->m_peer);
            m_state->m_requests.removeEndpoint(id);
            m_state->m_endpoints.erase(id);
        }
    }
    return true;
}

void NetEndpointRouter::shutdown()
{
    if (m_state->m_polling || m_state->m_dispatching)
    {
        m_state->m_stopRequested = true;
        return;
    }
    if (m_state->m_stopped)
    {
        return;
    }
    m_state->m_stopped = true;
    m_state->m_stopRequested = false;
    for (const auto& endpoint : m_state->m_endpoints)
    {
        closeEndpoint(endpoint.first, NI_CANCELLED);
    }
    if (m_state->m_transport != nullptr)
    {
        try
        {
            m_state->m_transport->shutdown();
        }
        catch (...)
        {
            // 停止异常不能跳过已关闭路由、请求及文件元数据的清理。
        }
    }
    for (const auto& endpoint : m_state->m_endpoints)
    {
        NetInterfaceRequest request = {};
        while (m_state->m_requests.takeEndpoint(endpoint.first, request) == NI_READY)
        {

        }
        m_state->m_requests.removeEndpoint(endpoint.first);
    }
    m_state->m_closingFirst = nullptr;
    m_state->m_closingLast = nullptr;
    m_state->m_fileExpiryCount = 0;
    std::memset(m_state->m_fileExpiry, 0, sizeof(m_state->m_fileExpiry));
    m_state->m_streams.clear();
    m_state->m_serviceStreams.clear();
    m_state->m_endpoints.clear();
    m_state->m_peers.clear();
    m_state->m_products.clear();
    m_state->m_origins.clear();
    m_state->clearPayload();
    m_state->m_listening = false;
    m_state->m_entropy = nullptr;
    m_state->m_entropyContext = nullptr;
}

// NET_ENDPOINT_ROUTER_UNIT_BEGIN
//#include "NetInterface/NetEndpointRouterTest.h"
//#include "NetInterface.h"
//#ifdef STATIC_LIB
//#include "NetInterface/NetEndpointRouterState.h"
//#endif
//#include <algorithm>
//#include <chrono>
//#ifdef _WIN32
//#include <windows.h>
//#endif
//#include <cstdio>
//#include <cstring>
//#include <ctime>
//#include <utility>
//#if defined(_WIN32) && defined(_DEBUG)
//#include <crtdbg.h>
//static int32_t g_routerRejectAfter = -1;
//static uint32_t g_routerAllocations = 0;
//static NetEndpointProbeSink* g_routerObserved = nullptr;
//static uint32_t g_routerWiped = 0;
//static uint32_t g_routerDirty = 0;
//
///** 独立测试中拒绝第N个普通分配，0表示从下一次起全部拒绝
//@param [in] operation CRT操作
//@param [in] data 未使用
//@param [in] size 未使用
//@param [in] block 内存类别
//@param [in] request 未使用
//@param [in] file 未使用
//@param [in] line 未使用
//@return 允许或拒绝
//*/
//static int RouterMemoryHook(int operation, void* data, size_t, int block, long, const unsigned char*, int)
//{
//    if (operation == _HOOK_FREE && block == _NORMAL_BLOCK && g_routerObserved != nullptr &&
//        data == g_routerObserved->m_borrowed && data != nullptr)
//    {
//        ++g_routerWiped;
//        const uint8_t* bytes = static_cast<const uint8_t*>(data);
//        for (uint32_t index = 0; index < g_routerObserved->m_borrowedSize; ++index)
//        {
//            if (bytes[index] != 0) { ++g_routerDirty; break; }
//        }
//        g_routerObserved->m_borrowed = nullptr;
//    }
//    if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK)
//    {
//        ++g_routerAllocations;
//        if (g_routerRejectAfter == 0) { return 0; }
//        if (g_routerRejectAfter > 0) { --g_routerRejectAfter; }
//    }
//    return 1;
//}
//#endif
//NetEndpointProbeSink::NetEndpointProbeSink() :
//m_fault(0), m_sent(0), m_closed(0), m_endpoint(0), m_borrowed(nullptr), m_borrowedSize(0), m_router(nullptr)
//{
//    std::memset(&m_frame, 0, sizeof(m_frame));
//    std::memset(&m_origin, 0, sizeof(m_origin));
//    std::memset(m_bytes, 0, sizeof(m_bytes));
//}
//
//int32_t NetEndpointProbeSink::send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin)
//{
//    m_borrowed = frame.m_payload;
//    m_borrowedSize = frame.m_payloadSize;
//    if ((m_fault & 1) != 0) { throw 1; }
//    if ((m_fault & 4) != 0) { return NI_LIMIT_EXCEEDED; }
//    if ((m_fault & 8) != 0 && m_router != nullptr)
//    {
//        m_router->shutdown();
//        m_router->poll();
//    }
//    ++m_sent;
//    m_endpoint = endpoint;
//    m_origin = origin;
//    m_frame = frame;
//    if (frame.m_payloadSize != 0)
//    {
//        std::memcpy(m_bytes, frame.m_payload, (std::min<uint32_t>)(sizeof(m_bytes), frame.m_payloadSize));
//    }
//    m_frame.m_payload = m_bytes;
//    return NI_OK;
//}
//
//void NetEndpointProbeSink::close(uint64_t endpoint)
//{
//    (void)endpoint;
//    ++m_closed;
//    if ((m_fault & 2) != 0) { throw 1; }
//}
//
//NetEndpointProbeTransport::NetEndpointProbeTransport() :
//m_fault(0), m_stopped(0), m_listened(0), m_listener(nullptr)
//{
//}
//
//int32_t NetEndpointProbeTransport::connect(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener*, uint64_t& peer)
//{
//    peer = 0;
//    return NI_NOT_SUPPORTED;
//}
//
//int32_t NetEndpointProbeTransport::listen(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener* listener, uint32_t)
//{
//    ++m_listened;
//    m_listener = listener;
//    if ((m_fault & 1) != 0) { throw 1; }
//    return (m_fault & 32) != 0 ? NI_NOT_SUPPORTED : NI_OK;
//}
//
//bool NetEndpointProbeTransport::poll()
//{
//    if ((m_fault & 8) != 0) { throw 1; }
//    return (m_fault & 64) == 0;
//}
//
//int32_t NetEndpointProbeTransport::send(uint64_t, const NetInterfaceFrame&)
//{
//    return NI_OK;
//}
//
//void NetEndpointProbeTransport::close(uint64_t, int32_t)
//{
//    if ((m_fault & 16) != 0) { throw 1; }
//}
//
//void NetEndpointProbeTransport::shutdown()
//{
//    ++m_stopped;
//    m_listener = nullptr;
//    if ((m_fault & 2) != 0) { throw 1; }
//}
//
//uint32_t NetEndpointProbeTransport::localPort() const
//{
//    if ((m_fault & 4) != 0) { throw 1; }
//    return 1025;
//}
//
//uint32_t NetEndpointProbeTransport::peerCount() const
//{
//    return 0;
//}
//
///** 构造带固定观察期的已认证端点，只供测试
//@param [in,out] router 被测实例
//@param [in] publicId 宿主来源号
//@param [out] endpoint 私有路由代次
//@return 登记结果
//*/
//static int32_t RouterTestClient(NetEndpointRouter& router, uint64_t publicId, uint64_t& endpoint)
//{
//    NetEndpointOrigin origin = {};
//    origin.m_endpoint = publicId;
//    origin.m_platform = 1;
//    std::memset(origin.m_device, 1, sizeof(origin.m_device));
//    const uint32_t negotiated[] = {1, 7, 128, 65536};
//    return router.addClient(origin, negotiated, endpoint);
//}
//
///** 构造测试RPC，同步调用期间借用输出缓冲区
//@param [in] kind 请求/回复
//@param [in] product 产品
//@param [in] type 类型
//@param [in] id 来源号或路由号
//@param [in] parent 内部父号
//@param [in] bytes 消息体
//@param [in] size 体长度
//@param [out] payload 至少size+16
//@return 完整帧
//*/
//static NetInterfaceFrame RouterTestRpc(uint16_t kind, uint32_t product, uint32_t type, uint64_t id, uint64_t parent,
//    const uint8_t* bytes, uint32_t size, uint8_t* payload)
//{
//    NetInterfaceFrame frame = {};
//    frame.m_kind = kind;
//    frame.m_productId = product;
//    frame.m_messageType = type;
//    frame.m_requestId = id;
//    frame.m_payload = payload;
//    NetInterfaceRpc::encode(kind, kind == NI_REQUEST ? 60000 : NI_RPC_SUCCESS, parent, bytes, size, payload, size + 16, frame.m_payloadSize);
//    return frame;
//}
//
///** 构造合法上传流元数据，不代表实际业务授权
//@param [in] local 服务本地流号
//@return 元数据
//*/
//static NetInterfaceFileInfo RouterTestInfo(uint64_t local)
//{
//    NetInterfaceFileInfo info = {};
//    info.direction = 1;
//    info.state = NI_FILE_TRANSFERRING;
//    info.chunk = 1024;
//    info.window = 2;
//    info.total = 4096;
//    info.stream = local;
//    info.expiresUtc = static_cast<uint64_t>(std::time(nullptr)) + 1200;
//    std::memcpy(info.transfer, &local, sizeof(local));
//    std::memset(info.token, 0x41, sizeof(info.token));
//    std::memset(info.scope, 0x51, sizeof(info.scope));
//    return info;
//}
//
///** 走真实RPC接入并返回网关改写后的文件结果
//@param [in,out] router 被测路由
//@param [in,out] services 观测服务端目标
//@param [in,out] clients 观测客户端回复
//@param [in] client 客户端代次
//@param [in] service 服务端代次
//@param [in] product 产品号
//@param [in] original 客户端严格递增号
//@param [in] info 服务授权的元数据
//@param [out] mapped 公开文件信息
//@return RPC结果或错误
//*/
//static int32_t RouterTestOpen(NetEndpointRouter& router, NetEndpointProbeSink& services, NetEndpointProbeSink& clients,
//    uint64_t client, uint64_t service, uint32_t product, uint64_t original, const NetInterfaceFileInfo& info, NetInterfaceFileInfo& mapped)
//{
//    uint8_t payload[176] = {};
//    NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, product, NI_FILE_OPEN_UPLOAD, original, 0, nullptr, 0, payload);
//    if (router.receiveClient(client, request) != NI_OK) { return NI_INVALID_STATE; }
//    const uint64_t route = services.m_frame.m_requestId;
//    uint8_t data[160] = {};
//    if (NetInterfaceFile::encodeInfo(info, data, sizeof(data)) != NI_OK)
//{
//    return NI_INVALID_ARGUMENT;
//}

//    NetInterfaceFrame reply = RouterTestRpc(NI_RESPONSE, product, NI_FILE_OPEN_UPLOAD, route, 0, data, sizeof(data), payload);
//    if (router.receiveService(service, reply) != NI_OK) { return NI_INVALID_STATE; }
//    NetInterfaceRpcView view = {};
//    if (clients.m_frame.m_requestId != original || NetInterfaceRpc::parse(clients.m_frame, view) != NI_OK)
//{
//    return NI_PROTOCOL_ERROR;
//}

//    if (view.m_value != NI_RPC_SUCCESS) { return static_cast<int32_t>(view.m_value); }
//    return NetInterfaceFile::parseInfo(view.m_data, view.m_size, mapped);
//}
//
///** 测试随机不用于生产安全
//@param [in] context 未使用
//@param [out] bytes 填充区域
//@param [in] size 长度
//@return 成功
//*/
//static int32_t RouterTestEntropy(void*, uint8_t* bytes, uint32_t size)
//{
//    std::memset(bytes, 0x52, size);
//    return NI_OK;
//}
//
//int main()
//{
//    uint32_t checks = 0;
//#define R_CHECK(condition) { ++checks; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState before;
//    _CrtMemCheckpoint(&before);
//#endif
//    {
//    // 监听失败、推进异常、宿主静止及停止异常都要收敛，不保留虚假的监听状态。
//    for (uint32_t mode = 0; mode < 6; ++mode)
//    {
//        NetEndpointProbeTransport transport;
//        NetEndpointProbeSink services;
//        NetEndpointRouter router(&services);
//        NetInterfaceTlsOptions tls = {};
//        tls.entropy = RouterTestEntropy;
//        R_CHECK(router.bindTransport(&transport) == NI_OK && router.localPort() == 0);
//        transport.m_fault = 64;
//        R_CHECK(router.poll()); // 尚未监听时，宿主静止不代表故障。
//        transport.m_fault = mode == 0 ? 1 : mode == 1 ? 32 : mode == 2 ? 3 : 0;
//        const int32_t started = router.listen("127.0.0.1", 0, tls, 8);
//        if (mode < 3)
//        {
//            R_CHECK(started < 0 && router.localPort() == 0 && transport.m_stopped == 1);
//            transport.m_fault = 0;
//            if (mode < 2)
//            {
//                R_CHECK(router.listen("127.0.0.1", 0, tls, 8) == NI_OK && router.localPort() == 1025);
//            }
//            else
//            {
//                R_CHECK(router.listen("127.0.0.1", 0, tls, 8) == NI_INVALID_STATE);
//            }
//        }
//        else
//        {
//            R_CHECK(started == NI_OK && router.listen("127.0.0.1", 0, tls, 8) == NI_INVALID_STATE && transport.m_listened == 1);
//            transport.m_fault = 4;
//            R_CHECK(router.localPort() == 0);
//            transport.m_fault = mode == 3 ? 10 : mode == 5 ? 64 : 2;
//            if (mode == 3 || mode == 5) { R_CHECK(!router.poll() && router.localPort() == 0); }
//        }
//        router.shutdown();
//        R_CHECK(!router.poll() && router.pendingCount() == 0);
//        const uint32_t stopped = transport.m_stopped;
//        router.shutdown();
//        R_CHECK(transport.m_stopped == stopped);
//    }
//    // 来源输出参数与输入来源号重叠，不得破坏原来源校验；关闭回调异常不丢收尾任务。
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        NetEndpointRouter router(&services, &clients);
//        uint64_t handler = 0;
//        R_CHECK(router.addHandler(101, 128, handler) == NI_OK);
//        NetEndpointOrigin origin = {};
//        origin.m_endpoint = 1001;
//        origin.m_platform = 1;
//        std::memset(origin.m_device, 1, sizeof(origin.m_device));
//        const uint32_t negotiated[] = {1, 7, 128, 65536};
//        R_CHECK(router.addClient(origin, negotiated, origin.m_endpoint) == NI_OK && origin.m_endpoint != 1001);
//        uint8_t payload[16] = {};
//        NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, 101, 11, 1, 0, nullptr, 0, payload);
//        R_CHECK(router.receiveClient(origin.m_endpoint, request) == NI_OK && router.pendingCount() == 1 && services.m_origin.m_endpoint == 1001);
//        services.m_fault = 2;
//        router.closeEndpoint(handler, NI_CANCELLED);
//        router.closeEndpoint(handler, NI_CANCELLED);
//        R_CHECK(services.m_closed == 1 && router.poll() && router.pendingCount() == 0);
//        NetInterfaceRpcView failure = {};
//        R_CHECK(NetInterfaceRpc::parse(clients.m_frame, failure) == NI_OK && failure.m_value == NI_RPC_UNAVAILABLE);
//        uint64_t replacement = 0;
//        R_CHECK(router.addHandler(101, 128, replacement) == NI_OK && replacement > handler);
//    }
//    // 混合多个关闭端点，不让一个端点的128条在途独占整轮回收。
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        NetEndpointRouter router(&services, &clients);
//        uint64_t handler[3] = {};
//        uint64_t client[3] = {};
//        uint8_t bytes[16] = {};
//        for (uint32_t group = 0; group < 3; ++group)
//        {
//            R_CHECK(router.addHandler(101 + group, 128, handler[group]) == NI_OK && RouterTestClient(router, 1001 + group, client[group]) == NI_OK);
//            for (uint64_t id = 1; id <= 128; ++id)
//            {
//                const NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, 101 + group, 11, id, 0, nullptr, 0, bytes);
//                R_CHECK(router.receiveClient(client[group], request) == NI_OK);
//            }
//            router.closeEndpoint(handler[group], NI_CANCELLED);
//        }
//        R_CHECK(router.pendingCount() == 384 && router.poll() && router.pendingCount() == 320);
//        R_CHECK(clients.m_endpoint == client[0]);
//        for (uint32_t round = 0; round < 8; ++round) { router.poll(); }
//        R_CHECK(router.pendingCount() == 0 && services.m_closed == 3 && clients.m_sent == 384);
//    }
//    // 回调只请求停止，不能在借用输出帧时释放路由对象或再次推进。
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        NetEndpointRouter router(&services, &clients);
//        uint64_t service = 0;
//        uint64_t client = 0;
//        R_CHECK(router.addHandler(101, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK);
//        services.m_fault = 8;
//        services.m_router = &router;
//        uint8_t bytes[16] = {};
//        const NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, 101, 11, 1, 0, nullptr, 0, bytes);
//        R_CHECK(router.receiveClient(client, request) == NI_OK && !router.poll() && router.pendingCount() == 0 && services.m_closed == 1);
//    }
//    // 文件32条客户端限额；第33条父请求失败也立刻结束已发出的内部查询。
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        NetEndpointRouter router(&services, &clients);
//        uint64_t service = 0;
//        uint64_t dependency = 0;
//        uint64_t client = 0;
//        R_CHECK(router.addHandler(101, 128, service) == NI_OK && router.addService(0, true, 128, dependency) == NI_OK &&
//            RouterTestClient(router, 1001, client) == NI_OK);
//        NetInterfaceFileInfo mapped = {};
//        for (uint64_t id = 1; id <= 32; ++id)
//        {
//            R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, id, RouterTestInfo(id + 1000), mapped) == NI_OK &&
//                mapped.stream == id && router.pendingCount() == 0);
//        }
//        uint8_t payload[176] = {};
//        NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, 101, NI_FILE_OPEN_UPLOAD, 33, 0, nullptr, 0, payload);
//        R_CHECK(router.receiveClient(client, request) == NI_OK);
//        const uint64_t parent = services.m_frame.m_requestId;
//        request = RouterTestRpc(NI_REQUEST, 101, 11, 1, parent, nullptr, 0, payload);
//        R_CHECK(router.receiveService(service, request) == NI_OK && router.pendingCount() == 2 && services.m_endpoint == dependency);
//        uint8_t data[160] = {};
//        const NetInterfaceFileInfo info = RouterTestInfo(1033);
//        R_CHECK(NetInterfaceFile::encodeInfo(info, data, sizeof(data)) == NI_OK);
//        const NetInterfaceFrame reply = RouterTestRpc(NI_RESPONSE, 101, NI_FILE_OPEN_UPLOAD, parent, 0, data, sizeof(data), payload);
//        R_CHECK(router.receiveService(service, reply) == NI_OK && router.pendingCount() == 0 && services.m_closed == 0);
//        NetInterfaceRpcView failure = {};
//        R_CHECK(NetInterfaceRpc::parse(clients.m_frame, failure) == NI_OK && failure.m_value == NI_RPC_BUSY);
//        // 取消流会释放额度，正常绑定能复用额度但永不复用公开流号。
//        NetInterfaceFrame cancel = {};
//        cancel.m_kind = NI_CANCEL;
//        cancel.m_productId = 101;
//        cancel.m_messageType = NI_FILE_DATA_TYPE;
//        cancel.m_requestId = 1;
//        R_CHECK(router.receiveClient(client, cancel) == NI_OK);
//        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 34, RouterTestInfo(1034), mapped) == NI_OK && mapped.stream == 33);
//        const uint64_t sameStream = mapped.stream;
//        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 35, RouterTestInfo(1034), mapped) == NI_OK && mapped.stream == sameStream);
//        NetInterfaceFileInfo forged = RouterTestInfo(1034);
//        forged.token[0] ^= 1;
//        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 36, forged, mapped) == NI_RPC_FAILED && services.m_closed == 1);
//        R_CHECK(router.poll() && router.pendingCount() == 0);
//    }
//    // DATA及最终ACK背压完全无副作用；成功终态才释放流额度。
//    for (uint32_t direction = 1; direction <= 2; ++direction)
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        NetEndpointRouter router(&services, &clients);
//        uint64_t service = 0;
//        uint64_t client = 0;
//        R_CHECK(router.addHandler(101, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK);
//        NetInterfaceFileInfo info = RouterTestInfo(1001);
//        info.direction = direction;
//        NetInterfaceFileInfo mapped = {};
//        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 1, info, mapped) == NI_OK);
//        uint8_t payload[128] = {};
//        uint8_t byte = 0x52;
//        NetInterfaceFileData data = {};
//        data.bytes = &byte;
//        data.size = 1;
//        std::memcpy(data.token, info.token, 32);
//        uint32_t size = 0;
//        R_CHECK(NetInterfaceFile::encodeData(data, payload, sizeof(payload), size) == NI_OK);
//        NetInterfaceFrame frame = {};
//        frame.m_kind = NI_DATA;
//        frame.m_productId = 101;
//        frame.m_messageType = NI_FILE_DATA_TYPE;
//        frame.m_requestId = direction == 1 ? mapped.stream : info.stream;
//        frame.m_payload = payload;
//        frame.m_payloadSize = size;
//        NetEndpointProbeSink& dataSink = direction == 1 ? services : clients;
//        dataSink.m_fault = 4;
//        const uint32_t sent = dataSink.m_sent;
//        R_CHECK((direction == 1 ? router.receiveClient(client, frame) : router.receiveService(service, frame)) == NI_LIMIT_EXCEEDED && dataSink.m_sent == sent);
//        dataSink.m_fault = 0;
//        R_CHECK((direction == 1 ? router.receiveClient(client, frame) : router.receiveService(service, frame)) == NI_OK && dataSink.m_sent == sent + 1 &&
//            dataSink.m_frame.m_requestId == (direction == 1 ? info.stream : mapped.stream));
//        NetInterfaceFileAck ack = {};
//        ack.flags = 1;
//        ack.offset = info.total;
//        std::memcpy(ack.token, info.token, 32);
//        R_CHECK(NetInterfaceFile::encodeAck(ack, payload, sizeof(payload)) == NI_OK);
//        frame.m_kind = NI_DATA_ACK;
//        frame.m_requestId = direction == 1 ? info.stream : mapped.stream;
//        frame.m_payloadSize = NET_INTERFACE_FILE_ACK_SIZE;
//        NetEndpointProbeSink& ackSink = direction == 1 ? clients : services;
//        ackSink.m_fault = 4;
//        R_CHECK((direction == 1 ? router.receiveService(service, frame) : router.receiveClient(client, frame)) == NI_LIMIT_EXCEEDED);
//        ackSink.m_fault = 0;
//        R_CHECK((direction == 1 ? router.receiveService(service, frame) : router.receiveClient(client, frame)) == NI_OK);
//        const uint32_t after = ackSink.m_sent;
//        R_CHECK((direction == 1 ? router.receiveService(service, frame) : router.receiveClient(client, frame)) == NI_OK && ackSink.m_sent == after);
//        // 用新来源请求重建同一服务流号，公开号必须递增。
//        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 2, info, mapped) == NI_OK && mapped.stream == 2);
//        router.closeEndpoint(client, NI_CANCELLED);
//        R_CHECK(router.poll() && services.m_closed == 0);
//    }
//    // 同一服务128条与全局1024条额度，跨服务同号隔离，撤销只影响授权服务的分组。
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        NetEndpointRouter router(&services, &clients);
//        uint64_t service[9] = {};
//        uint64_t client[34] = {};
//        for (uint32_t index = 0; index < 9; ++index)
//        {
//            R_CHECK(router.addHandler(101 + index, 128, service[index]) == NI_OK);
//        }
//        for (uint32_t index = 0; index < 34; ++index)
//        {
//            R_CHECK(RouterTestClient(router, 1001 + index, client[index]) == NI_OK);
//        }
//        NetInterfaceFileInfo mapped = {};
//        for (uint32_t group = 0; group < 8; ++group)
//        {
//            for (uint32_t item = 0; item < 128; ++item)
//            {
//                R_CHECK(RouterTestOpen(router, services, clients, client[group * 4 + item / 32], service[group], 101 + group,
//                    item % 32 + 1, RouterTestInfo(item + 1000), mapped) == NI_OK);
//            }
//            R_CHECK(RouterTestOpen(router, services, clients, client[33], service[group], 101 + group,
//                group + 1, RouterTestInfo(2000), mapped) == NI_RPC_BUSY);
//        }
//        R_CHECK(RouterTestOpen(router, services, clients, client[32], service[8], 109, 1, RouterTestInfo(3000), mapped) == NI_RPC_BUSY);
//        uint8_t eventBytes[40] = {};
//        uint8_t scope[16];
//        std::memset(scope, 0x51, sizeof(scope));
//        uint32_t size = 0;
//        R_CHECK(NetInterfaceEvent::encode(0, 1, scope, sizeof(scope), eventBytes, sizeof(eventBytes), size) == NI_OK);
//        NetInterfaceFrame event = {};
//        event.m_kind = NI_EVENT;
//        event.m_productId = 101;
//        event.m_requestId = 1;
//        event.m_messageType = NI_FILE_REVOKE_SCOPE;
//        event.m_payload = eventBytes;
//        event.m_payloadSize = size;
//        const uint32_t notices = clients.m_sent;
//        R_CHECK(router.receiveService(service[0], event) == NI_OK && clients.m_sent == notices + 128);
//        R_CHECK(RouterTestOpen(router, services, clients, client[32], service[8], 109, 2, RouterTestInfo(3000), mapped) == NI_OK && mapped.stream == 1025);
//        R_CHECK(RouterTestOpen(router, services, clients, client[33], service[1], 102, 9, RouterTestInfo(2000), mapped) == NI_RPC_BUSY);
//        router.shutdown();
//        R_CHECK(!router.poll() && router.pendingCount() == 0 && services.m_closed == 9 && clients.m_closed == 34);
//    }
//
//    // 通用处理器0可以承载已授权产品文件；只按真实请求路由绑定，不能跨产品换流。
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        NetEndpointRouter router(&services, &clients);
//        uint64_t service = 0;
//        uint64_t client = 0;
//        R_CHECK(router.addHandler(0, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK);
//        NetInterfaceFileInfo mapped = {};
//        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 1, RouterTestInfo(1001), mapped) == NI_OK && mapped.stream == 1);
//        R_CHECK(RouterTestOpen(router, services, clients, client, service, 102, 2, RouterTestInfo(1001), mapped) == NI_RPC_FAILED && services.m_closed == 1);
//        R_CHECK(router.poll() && router.pendingCount() == 0);
//    }
//    // 已经过期的授权不能登记流；端点关闭后通知存活的另一端。
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        NetEndpointRouter router(&services, &clients);
//        uint64_t service = 0;
//        uint64_t client = 0;
//        R_CHECK(router.addHandler(101, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK);
//        NetInterfaceFileInfo mapped = {};
//        NetInterfaceFileInfo info = RouterTestInfo(1001);
//        info.expiresUtc = static_cast<uint64_t>(std::time(nullptr)) - 1;
//        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 1, info, mapped) == NI_RPC_FAILED);
//        info = RouterTestInfo(1001);
//        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 2, info, mapped) == NI_OK && mapped.stream == 1);
//        router.closeEndpoint(service, NI_CANCELLED);
//        const uint32_t sent = clients.m_sent;
//        R_CHECK(router.poll() && clients.m_sent == sent + 1);
//        NetInterfaceFileNotice notice = {};
//        R_CHECK(NetInterfaceFile::parseNotice(clients.m_frame, notice) == NI_OK && notice.result == NI_INVALID_STATE && clients.m_frame.m_requestId == mapped.stream);
//    }
//
//    // 已有合法文件流也不能跨来源、跨产品、更换令牌、倒转方向或越过文件边界。
//    for (uint32_t mode = 0; mode < 5; ++mode)
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        NetEndpointRouter router(&services, &clients);
//        uint64_t service = 0;
//        uint64_t client = 0;
//        uint64_t intruder = 0;
//        R_CHECK(router.addHandler(101, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK &&
//            RouterTestClient(router, 1002, intruder) == NI_OK);
//        NetInterfaceFileInfo mapped = {};
//        const NetInterfaceFileInfo info = RouterTestInfo(1001);
//        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 1, info, mapped) == NI_OK);
//        uint8_t byte = 0x41;
//        NetInterfaceFileData data = {};
//        data.bytes = &byte;
//        data.size = 1;
//        data.offset = mode == 4 ? info.total : 0;
//        std::memcpy(data.token, info.token, 32);
//        if (mode == 2) { data.token[0] ^= 1; }
//        uint8_t payload[128] = {};
//        uint32_t size = 0;
//        R_CHECK(NetInterfaceFile::encodeData(data, payload, sizeof(payload), size) == NI_OK);
//        NetInterfaceFrame frame = {};
//        frame.m_kind = NI_DATA;
//        frame.m_productId = mode == 1 ? 102 : 101;
//        frame.m_messageType = NI_FILE_DATA_TYPE;
//        frame.m_requestId = mode == 3 ? info.stream : mapped.stream;
//        frame.m_payload = payload;
//        frame.m_payloadSize = size;
//        const uint32_t sent = services.m_sent;
//        const int32_t received = mode == 3 ? router.receiveService(service, frame) : router.receiveClient(mode == 0 ? intruder : client, frame);
//        R_CHECK(received == NI_PROTOCOL_ERROR && services.m_sent == sent);
//        R_CHECK(mode == 3 ? services.m_closed == 1 : clients.m_closed == 1);
//        router.poll();
//        if (mode == 0)
//        {
//            R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 2, info, mapped) == NI_OK && mapped.stream == 1);
//        }
//    }
//    // 真实单调时间推进：128条同时到期，每轮至多回收64条，随后允许重新占用额度。
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        NetEndpointRouter router(&services, &clients);
//        uint64_t service = 0;
//        uint64_t client[4] = {};
//        R_CHECK(router.addHandler(101, 128, service) == NI_OK);
//        for (uint32_t index = 0; index < 4; ++index) { R_CHECK(RouterTestClient(router, 1001 + index, client[index]) == NI_OK); }
//        const uint64_t expires = static_cast<uint64_t>(std::time(nullptr)) + 2;
//        NetInterfaceFileInfo mapped = {};
//        for (uint32_t index = 0; index < 128; ++index)
//        {
//            NetInterfaceFileInfo info = RouterTestInfo(1000 + index);
//            info.expiresUtc = expires;
//            R_CHECK(RouterTestOpen(router, services, clients, client[index / 32], service, 101, index % 32 + 1, info, mapped) == NI_OK);
//        }
//        // 独立回归只等待一次真实2秒截止，不向生产状态机增加时钟后门。
//        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2010);
//        while (std::chrono::steady_clock::now() < deadline)
//        {
//#ifdef _WIN32
//            Sleep(1);
//#endif
//        }
//        const uint32_t sent = clients.m_sent;
//        R_CHECK(router.poll() && clients.m_sent == sent + 64 && services.m_closed == 0);
//        R_CHECK(router.poll() && clients.m_sent == sent + 128);
//        R_CHECK(router.poll() && clients.m_sent == sent + 128);
//        R_CHECK(RouterTestOpen(router, services, clients, client[0], service, 101, 33, RouterTestInfo(1000), mapped) == NI_OK && mapped.stream == 129);
//    }
//#ifdef STATIC_LIB
//    // 固定期限堆与双端点链的独立参考模型；反向下标、每端点额度及空槽均逐项核对。
//    {
//        NetEndpointRouterState state(nullptr, nullptr);
//        for (uint64_t id = 1; id <= 40; ++id)
//        {
//            std::unique_ptr<NetEndpointPeer> endpoint(new NetEndpointPeer());
//            endpoint->m_id = id;
//            endpoint->m_service = id > 32;
//            state.m_endpoints.insert(std::make_pair(id, std::move(endpoint)));
//        }
//        std::map<uint64_t, uint64_t> expected;
//        uint32_t random = 0x81a94571;
//        for (uint32_t step = 0; step < 4096; ++step)
//        {
//            random = random * 1664525U + 1013904223U;
//            const uint64_t id = step < 1024 ? step + 1 : ((random ^ (random >> 16)) % 1024) + 1;
//            const auto found = state.m_streams.find(id);
//            random = random * 1664525U + 1013904223U;
//            const uint64_t deadline = random % 10000;
//            if (found == state.m_streams.end())
//            {
//                const uint64_t client = (id - 1) / 32 + 1;
//                const uint64_t service = (id - 1) / 128 + 33;
//                NetInterfaceFileInfo info = RouterTestInfo(id);
//                std::unique_ptr<NetEndpointFileStream> stream(new NetEndpointFileStream(client, service, 101, info, deadline));
//                stream->m_id = id;
//                state.m_streams.insert(std::make_pair(id, std::move(stream)));
//                state.m_serviceStreams.insert(std::make_pair(std::make_pair(service, id), id));
//                state.attachFile(state.m_streams.find(id)->second.get());
//                expected[id] = deadline;
//            }
//            else if (step % 3 == 0)
//            {
//                std::unique_ptr<NetEndpointFileStream> retired = state.takeFile(id);
//                R_CHECK(retired.get() != nullptr && retired->m_id == id);
//                expected.erase(id);
//            }
//            else
//            {
//                state.renewFile(found->second.get(), deadline);
//                expected[id] = deadline;
//            }
//            R_CHECK(state.m_fileExpiryCount == expected.size() && state.m_streams.size() == expected.size() &&
//                state.m_serviceStreams.size() == expected.size());
//            uint64_t minimum = UINT64_MAX;
//            for (const auto& item : expected) { minimum = (std::min)(minimum, item.second); }
//            if (!expected.empty()) { R_CHECK(state.m_fileExpiry[0]->m_deadline == minimum); }
//            for (uint32_t index = 0; index < state.m_fileExpiryCount; ++index)
//            {
//                NetEndpointFileStream* stream = state.m_fileExpiry[index];
//                R_CHECK(stream != nullptr && stream->m_expiryIndex == index && expected.find(stream->m_id) != expected.end() &&
//                    expected[stream->m_id] == stream->m_deadline && state.m_streams[stream->m_id].get() == stream);
//                if (index != 0) { R_CHECK(state.m_fileExpiry[(index - 1) / 2]->m_deadline <= stream->m_deadline); }
//            }
//            if (step % 64 == 0)
//            {
//                for (const auto& entry : state.m_endpoints)
//                {
//                    NetEndpointPeer* endpoint = entry.second.get();
//                    const uint32_t role = endpoint->m_service ? 1 : 0;
//                    uint32_t count = 0;
//                    NetEndpointFileStream* previous = nullptr;
//                    for (NetEndpointFileStream* stream = endpoint->m_firstFile; stream != nullptr; stream = stream->m_next[role])
//                    {
//                        R_CHECK(stream->m_previous[role] == previous && (role == 0 ? stream->m_client : stream->m_service) == entry.first && ++count <= 128);
//                        previous = stream;
//                    }
//                    R_CHECK(count == endpoint->m_fileCount && count <= (role == 0 ? 32U : 128U));
//                }
//            }
//        }
//        while (!state.m_streams.empty())
//        {
//            const uint64_t id = state.m_streams.begin()->first;
//            R_CHECK(state.takeFile(id).get() != nullptr);
//        }
//        R_CHECK(state.m_fileExpiryCount == 0 && state.m_serviceStreams.empty());
//        for (uint32_t index = 0; index < 1024; ++index) { R_CHECK(state.m_fileExpiry[index] == nullptr); }
//        for (const auto& entry : state.m_endpoints) { R_CHECK(entry.second->m_fileCount == 0 && entry.second->m_firstFile == nullptr); }
//        uint8_t secret[32];
//        std::memset(secret, 0x42, sizeof(secret));
//        try
//        {
//            NetEndpointSecretScope scope(secret, sizeof(secret));
//            throw 1;
//        }
//        catch (...) { }
//        for (uint32_t index = 0; index < sizeof(secret); ++index) { R_CHECK(secret[index] == 0); }
//    }
//#endif
//
//#if defined(_WIN32) && defined(_DEBUG)
//    // 对真实关闭路径拒绝一切普通块分配：队列、摘除和终态投递均无内部分配。
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        NetEndpointRouter router(&services, &clients);
//        uint64_t service = 0;
//        uint64_t client = 0;
//        R_CHECK(router.addHandler(101, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK);
//        uint8_t payload[16] = {};
//        NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, 101, 11, 1, 0, nullptr, 0, payload);
//        R_CHECK(router.receiveClient(client, request) == NI_OK);
//        g_routerRejectAfter = 0;
//        g_routerAllocations = 0;
//        const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(RouterMemoryHook);
//        router.closeEndpoint(service, NI_CANCELLED);
//        router.poll();
//        _CrtSetAllocHook(previous);
//        R_CHECK(g_routerAllocations == 0 && router.pendingCount() == 0 && services.m_closed == 1 && clients.m_sent == 1);
//        g_routerRejectAfter = -1;
//    }
//    // 文件登记每一处分配失败均回滚两端额度与双索引，正常重试不需要重建路由器。
//    uint32_t fileFailures = 0;
//    for (int32_t fail = 0; fail < 5; ++fail)
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        NetEndpointRouter router(&services, &clients);
//        uint64_t service = 0;
//        uint64_t client = 0;
//        R_CHECK(router.addHandler(101, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK);
//        uint8_t payload[176] = {};
//        NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, 101, NI_FILE_OPEN_UPLOAD, 1, 0, nullptr, 0, payload);
//        R_CHECK(router.receiveClient(client, request) == NI_OK);
//        NetInterfaceFileInfo info = RouterTestInfo(1001);
//        uint8_t data[160] = {};
//        R_CHECK(NetInterfaceFile::encodeInfo(info, data, sizeof(data)) == NI_OK);
//        const NetInterfaceFrame reply = RouterTestRpc(NI_RESPONSE, 101, NI_FILE_OPEN_UPLOAD, services.m_frame.m_requestId, 0, data, sizeof(data), payload);
//        g_routerRejectAfter = fail;
//        g_routerAllocations = 0;
//        const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(RouterMemoryHook);
//        const int32_t result = router.receiveService(service, reply);
//        _CrtSetAllocHook(previous);
//        g_routerRejectAfter = -1;
//        R_CHECK(result == NI_OK && router.pendingCount() == 0 && services.m_closed == 0);
//        NetInterfaceRpcView view = {};
//        R_CHECK(NetInterfaceRpc::parse(clients.m_frame, view) == NI_OK);
//        if (view.m_value != NI_RPC_SUCCESS) { ++fileFailures; R_CHECK(view.m_value == NI_RPC_FAILED); }
//        NetInterfaceFileInfo mapped = {};
//        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 2, info, mapped) == NI_OK);
//        for (uint64_t id = 2; id <= 32; ++id)
//        {
//            R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, id + 1, RouterTestInfo(id + 1000), mapped) == NI_OK);
//        }
//        R_CHECK(RouterTestOpen(router, services, clients, client, service, 101, 34, RouterTestInfo(2000), mapped) == NI_RPC_BUSY);
//    }
//    R_CHECK(fileFailures == 4);
//#endif
//
//#if defined(_WIN32) && defined(_DEBUG)
//    // 在真实释放前检查转发暂存明文；成功、背压、宿主异常的小包及大包都必须擦除。
//    for (uint32_t mode = 0; mode < 6; ++mode)
//    {
//        NetEndpointProbeSink services;
//        NetEndpointProbeSink clients;
//        const uint32_t wiped = g_routerWiped;
//        const uint32_t dirty = g_routerDirty;
//        g_routerObserved = &services;
//        const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(RouterMemoryHook);
//        {
//            NetEndpointRouter router(&services, &clients);
//            uint64_t service = 0;
//            uint64_t client = 0;
//            R_CHECK(router.addHandler(101, 128, service) == NI_OK && RouterTestClient(router, 1001, client) == NI_OK);
//            std::vector<uint8_t> body(mode < 3 ? 32 : 8192, 0x59);
//            std::vector<uint8_t> payload(body.size() + 16);
//            services.m_fault = mode % 3 == 0 ? 0 : mode % 3 == 1 ? 4 : 1;
//            const NetInterfaceFrame request = RouterTestRpc(NI_REQUEST, 101, 11, 1, 0, body.data(), static_cast<uint32_t>(body.size()), payload.data());
//            R_CHECK(router.receiveClient(client, request) == NI_OK);
//            router.shutdown();
//        }
//        _CrtSetAllocHook(previous);
//        g_routerObserved = nullptr;
//        R_CHECK(g_routerWiped == wiped + 1 && g_routerDirty == dirty);
//    }
//#endif
//
//    }
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState after;
//    _CrtMemCheckpoint(&after);
//    R_CHECK(after.lCounts[_NORMAL_BLOCK] == before.lCounts[_NORMAL_BLOCK] && after.lSizes[_NORMAL_BLOCK] == before.lSizes[_NORMAL_BLOCK]);
//#endif
//    std::printf("Endpoint router checks=%u PASS\n", checks);
//    return 0;
//#undef R_CHECK
//}
// NET_ENDPOINT_ROUTER_UNIT_END