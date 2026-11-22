#include "NetEndpointRouter.h"
#include "NetInterfaceMacro.h"
#include "NetEndpointRouterState.h"
#include "NetInterface.h"
#include "NetInterfaceRequests.h"
#include "NetInterfaceRpc.h"
#include "NetInterfaceFile.h"
#include "NetInterfaceEvent.h"
#include <algorithm>
#include <cstring>
#include <ctime>
#include <utility>
#include "NetInterfaceTransport.h"
#include <chrono>
#include <vector>

NetEndpointServiceSink::~NetEndpointServiceSink()
{

}

NetEndpointRouter::NetEndpointRouter(NetEndpointServiceSink* services, NetEndpointServiceSink* clients) :
m_state(new NetEndpointRouterState(services, clients, NetEndpointPools()))
{

}

NetEndpointRouter::~NetEndpointRouter()
{
    shutdown();
    delete m_state;
}

int32_t NetEndpointRouter::configurePools(const NetEndpointPools& pools)
{
    if (m_state->m_lastEndpoint != 1 || m_state->m_listening || m_state->m_stopped || m_state->m_polling || m_state->m_dispatching)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetEndpointRouterState> prepared(new NetEndpointRouterState(m_state->m_services, m_state->m_clients, pools));
        if (prepared->m_status != NI_OK) { return prepared->m_status; }
        prepared->m_transport = m_state->m_transport;
        delete m_state;
        m_state = prepared.release();
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
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
    if (address == nullptr || port > 65535 || peers == 0 || tls.entropy == nullptr)
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
        ((internal || generic) != (product == 0)) || limit == 0 ||
        m_state->m_lastEndpoint == UINT64_MAX ||
        (internal ? m_state->m_internal != 0 : generic ? m_state->m_generic != 0 : m_state->m_products.find(product) != m_state->m_products.end()))
    {
        return NI_INVALID_ARGUMENT;
    }
    const uint64_t id = ++m_state->m_lastEndpoint;
    try
    {
        NetObjectReference<NetEndpointPeer> value;
        const int32_t allocated = m_state->m_endpointPool.create(value);
        if (allocated != NI_OK) { return allocated; }
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
        negotiated[0] != context.m_platform || (negotiated[1] & 1) == 0 || (negotiated[1] & ~31U) != 0 ||
        negotiated[2] == 0 || negotiated[3] < 1024 || negotiated[3] > 65536 ||
        m_state->m_lastEndpoint == UINT64_MAX || m_state->m_origins.count(context.m_endpoint) != 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    const uint64_t id = ++m_state->m_lastEndpoint;
    try
    {
        NetObjectReference<NetEndpointPeer> value;
        const int32_t allocated = m_state->m_endpointPool.create(value);
        if (allocated != NI_OK) { return allocated; }
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
        m_state->m_lastEndpoint == UINT64_MAX)
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
        NetObjectReference<NetEndpointPeer> value;
        const int32_t allocated = m_state->m_endpointPool.create(value);
        if (allocated != NI_OK) { closePeer(peer, allocated); return; }
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
    // 深度只有两级；子请求由实际记录池持有，不进行递归。
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
    m_state->m_fileExpiry.clear();
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

int32_t NetEndpointRouter::bindFileReply(const NetInterfaceRequest& request, const NetInterfaceRpcView& rpc, std::vector<uint8_t>& payload)
{
    try
    {
        payload.resize(NET_INTERFACE_FILE_INFO_SIZE + 16);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
    NetInterfaceFileInfo info = {};
    NetEndpointSecretScope infoScope(&info, sizeof(info));
    if (NetInterfaceFile::parseInfo(rpc.m_data, rpc.m_size, info) != NI_OK)
    {
        return NI_PROTOCOL_ERROR;
    }
    const auto client = m_state->m_endpoints.find(request.m_source);
    const auto service = m_state->m_endpoints.find(request.m_target);
    if (client == m_state->m_endpoints.end() || service == m_state->m_endpoints.end() || client->second->m_service ||
        !service->second->m_service ||
        (service->second->m_product != request.m_product && request.m_target != m_state->m_generic) || client->second->m_closing ||
        service->second->m_closing)
    {
        return NI_INVALID_STATE;
    }
    uint32_t negotiated[4] = {};
    uint8_t nonce[16] = {};
    uint8_t device[16] = {};
    if (client->second->m_attached)
    {
        std::memcpy(negotiated, client->second->m_negotiated, sizeof(negotiated));
    }
    else if (client->second->m_handshake->peerInfo(negotiated, nonce, device) != NI_READY)
    {
        return NI_NOT_SUPPORTED;
    }
    if ((negotiated[1] & 2) == 0 ||
        info.chunk > negotiated[3])
    {
        return NI_NOT_SUPPORTED;
    }
    if (info.state == NI_FILE_TRANSFERRING)
    {
        const int64_t utc = static_cast<int64_t>(std::time(nullptr));
        if (utc <= 0 || info.expiresUtc <= static_cast<uint64_t>(utc))
        {
            return NI_TIMEOUT;
        }
        const uint64_t deadline =
            NetEndpointRouterState::now() +
            (std::min<uint64_t>)(UINT64_C(1800000),
                (std::min<uint64_t>)(UINT64_C(1800), info.expiresUtc - static_cast<uint64_t>(utc)) * 1000);
        const auto key = std::make_pair(request.m_target, info.stream);
        const auto existing = m_state->m_serviceStreams.find(key);
        uint64_t publicId = 0;
        if (existing != m_state->m_serviceStreams.end())
        {
            const auto stream = m_state->m_streams.find(existing->second);
            if (stream == m_state->m_streams.end() || stream->second->m_client != request.m_source ||
                stream->second->m_product != request.m_product || stream->second->m_info.direction != info.direction ||
                stream->second->m_info.total != info.total || stream->second->m_info.chunk != info.chunk ||
                stream->second->m_info.window != info.window || !NetInterfaceFile::equal(stream->second->m_info.hash, info.hash, 32) ||
                !NetInterfaceFile::equal(stream->second->m_info.scope, info.scope, 16) ||
                !NetInterfaceFile::equal(stream->second->m_info.token, info.token, 32) ||
                !NetInterfaceFile::equal(stream->second->m_info.transfer, info.transfer, 16))
            {
                return NI_PROTOCOL_ERROR;
            }
            m_state->renewFile(stream->second.get(), deadline);
            publicId = existing->second;
        }
        else
        {
            if (m_state->m_lastStream == UINT64_MAX)
            {
                return NI_LIMIT_EXCEEDED;
            }
            try
            {
                NetObjectReference<NetEndpointFileStream> stream;
                const int32_t allocated = m_state->m_filePool.create(stream,
                    request.m_source, request.m_target, request.m_product, info, deadline);
                if (allocated != NI_OK) { return allocated; }
                publicId = ++m_state->m_lastStream;
                stream->m_id = publicId;
                m_state->m_streams.insert(std::make_pair(publicId, std::move(stream)));
                m_state->m_serviceStreams.insert(std::make_pair(key, publicId));
                // 期限数组准备失败时撤销双索引，实际流槽及秘密由引用自动回收。
                const int32_t attached = m_state->attachFile(m_state->m_streams.find(publicId)->second.get());
                if (attached != NI_OK)
                {
                    m_state->m_streams.erase(publicId);
                    m_state->m_serviceStreams.erase(key);
                    return attached;
                }
            }
            catch (...)
            {
                m_state->m_streams.erase(publicId);
                m_state->m_serviceStreams.erase(key);
                return NI_NO_MEMORY;
            }
        }
        info.stream = publicId;
    }
    else
    {
        NetEndpointFileStream* stream = client->second->m_firstFile;
        while (stream != nullptr)
        {
            NetEndpointFileStream* next = stream->m_next[0];
            if (stream->m_service == request.m_target && stream->m_product == request.m_product &&
                NetInterfaceFile::equal(stream->m_info.transfer, info.transfer, 16))
            {
                dropFile(stream->m_id, info.state == NI_FILE_COMPLETED ? NI_NOT_FOUND : NI_CANCELLED);
            }
            stream = next;
        }
    }
    uint8_t data[NET_INTERFACE_FILE_INFO_SIZE] = {};
    NetEndpointSecretScope dataScope(data, sizeof(data));
    const int32_t encoded = NetInterfaceFile::encodeInfo(info, data, sizeof(data));
    if (encoded != NI_OK)
    {
        return encoded;
    }
    uint32_t required = 0;
    return NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, data, sizeof(data), payload.data(),
        static_cast<uint32_t>(payload.size()), required);
}

int32_t NetEndpointRouter::routeFile(uint64_t endpoint, const NetInterfaceFrame& frame)
{
    const auto source = m_state->m_endpoints.find(endpoint);
    if (source == m_state->m_endpoints.end() || source->second->m_closing)
    {
        return NI_NOT_FOUND;
    }
    uint64_t id = frame.m_requestId;
    if (source->second->m_service)
    {
        const auto mapped = m_state->m_serviceStreams.find(std::make_pair(endpoint, id));
        if (mapped == m_state->m_serviceStreams.end())
        {
            return NI_OK;
        }
        id = mapped->second;
    }
    const auto found = m_state->m_streams.find(id);
    if (found == m_state->m_streams.end())
    {
        return NI_OK;
    }
    NetEndpointFileStream* stream = found->second.get();
    if ((source->second->m_service ? endpoint != stream->m_service : endpoint != stream->m_client) ||
        frame.m_productId != stream->m_product)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (NetEndpointRouterState::now() >= stream->m_deadline)
    {
        dropFile(id, NI_TIMEOUT);
        return NI_OK;
    }
    if (frame.m_kind == NI_CANCEL)
    {
        dropFile(id, NI_CANCELLED, endpoint);
        return NI_OK;
    }
    bool terminal = false;
    const bool sendsData = stream->m_info.direction == 1 ? !source->second->m_service : source->second->m_service;
    if (frame.m_kind == NI_DATA)
    {
        NetInterfaceFileData data = {};
        NetEndpointSecretScope dataScope(&data, sizeof(data));
        if (!sendsData || NetInterfaceFile::parseData(frame, data) != NI_OK ||
            !NetInterfaceFile::equal(data.token, stream->m_info.token, 32) || data.offset > stream->m_info.total ||
            data.size > stream->m_info.total - data.offset || data.size > stream->m_info.chunk)
        {
            return NI_PROTOCOL_ERROR;
        }
    }
    else if (frame.m_kind == NI_DATA_ACK)
    {
        NetInterfaceFileAck ack = {};
        NetEndpointSecretScope ackScope(&ack, sizeof(ack));
        if (sendsData || NetInterfaceFile::parseAck(frame, ack) != NI_OK || !NetInterfaceFile::equal(ack.token, stream->m_info.token, 32) ||
            ack.offset > stream->m_info.total)
        {
            return NI_PROTOCOL_ERROR;
        }
        terminal = ack.flags != 0;
    }
    else
    {
        NetInterfaceFileNotice notice = {};
        NetEndpointSecretScope noticeScope(&notice, sizeof(notice));
        if (!source->second->m_service || NetInterfaceFile::parseNotice(frame, notice) != NI_OK ||
            !NetInterfaceFile::equal(notice.token, stream->m_info.token, 32) ||
            !NetInterfaceFile::equal(notice.transfer, stream->m_info.transfer, 16))
        {
            return NI_PROTOCOL_ERROR;
        }
        terminal = true;
    }
    NetInterfaceFrame forwarded = frame;
    forwarded.m_requestId = source->second->m_service ? id : stream->m_local;
    const uint64_t target = source->second->m_service ? stream->m_client : stream->m_service;
    const int32_t delivered = deliver(target, forwarded, origin(stream->m_client));
    if (delivered == NI_OK && terminal)
    {
        m_state->takeFile(id);
    }
    return delivered;
}

void NetEndpointRouter::dropFile(uint64_t id, int32_t reason, uint64_t skip)
{
    NetObjectReference<NetEndpointFileStream> stream = m_state->takeFile(id);
    if (!stream)
    {
        return;
    }
    NetInterfaceFileNotice notice = {};
    NetEndpointSecretScope noticeScope(&notice, sizeof(notice));
    notice.result = reason < 0 ? reason : NI_CANCELLED;
    std::memcpy(notice.token, stream->m_info.token, 32);
    std::memcpy(notice.transfer, stream->m_info.transfer, 16);
    uint8_t payload[NET_INTERFACE_FILE_NOTICE_SIZE] = {};
    NetEndpointSecretScope payloadScope(payload, sizeof(payload));
    if (NetInterfaceFile::encodeNotice(notice, payload, sizeof(payload)) != NI_OK)
    {
        return;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_EVENT;
    frame.m_productId = stream->m_product;
    frame.m_messageType = NI_FILE_CANCEL;
    frame.m_payload = payload;
    frame.m_payloadSize = sizeof(payload);
    const auto context = origin(stream->m_client);
    frame.m_requestId = id;
    if (skip != stream->m_client && deliver(stream->m_client, frame, context) != NI_OK)
    {
        closeEndpoint(stream->m_client, NI_LIMIT_EXCEEDED);
    }
    frame.m_requestId = stream->m_local;
    if (skip != stream->m_service && deliver(stream->m_service, frame, context) != NI_OK)
    {
        closeEndpoint(stream->m_service, NI_LIMIT_EXCEEDED);
    }
}

void NetEndpointRouter::collectFiles(uint64_t endpoint)
{
    if (endpoint != 0)
    {
        const auto found = m_state->m_endpoints.find(endpoint);
        if (found == m_state->m_endpoints.end())
        {
            return;
        }
        while (found->second->m_firstFile != nullptr)
        {
            dropFile(found->second->m_firstFile->m_id, NI_INVALID_STATE, endpoint);
        }
        return;
    }
    const uint64_t now = NetEndpointRouterState::now();
    for (uint32_t count = 0; count < 64 && m_state->m_fileExpiryCount != 0; ++count)
    {
        NetEndpointFileStream* stream = m_state->m_fileExpiry[0];
        if (now < stream->m_deadline)
        {
            break;
        }
        dropFile(stream->m_id, NI_TIMEOUT);
    }
}

int32_t NetEndpointRouter::routeEvent(uint64_t endpoint, const NetInterfaceFrame& frame)
{
    const auto source = m_state->m_endpoints.find(endpoint);
    if (source == m_state->m_endpoints.end() || !source->second->m_service ||
        (source->second->m_product != 0 && frame.m_productId != source->second->m_product))
    {
        return NI_PROTOCOL_ERROR;
    }
    NetInterfaceEventView event = {};
    if (NetInterfaceEvent::parse(frame, event) != NI_OK)
    {
        return NI_PROTOCOL_ERROR;
    }
    uint64_t targetEndpoint = event.m_target;
    const auto publicTarget = m_state->m_origins.find(event.m_target);
    if (publicTarget != m_state->m_origins.end())
    {
        targetEndpoint = publicTarget->second;
    }
    else if (m_state->m_clients != nullptr && event.m_target != 0)
    {
        return NI_OK;
    }
    if (event.m_flags == 1)
    {
        for (auto item = m_state->m_streams.begin(); item != m_state->m_streams.end();)
        {
            const uint64_t id = item->first;
            const auto current = item++;
            if ((event.m_target == 0 || current->second->m_client == targetEndpoint) &&
                (source->second->m_product == 0 || current->second->m_service == endpoint) &&
                NetInterfaceFile::equal(current->second->m_info.scope, event.m_data, 16))
            {
                dropFile(id, NI_CANCELLED);
            }
        }
        return NI_OK;
    }
    const auto target = m_state->m_endpoints.find(targetEndpoint);
    if (target == m_state->m_endpoints.end() || target->second->m_service || target->second->m_closing)
    {
        return NI_OK;
    }
    if (m_state->m_lastEvent == UINT64_MAX)
    {
        return NI_LIMIT_EXCEEDED;
    }
    NetInterfaceFrame forwarded = frame;
    forwarded.m_requestId = m_state->m_lastEvent + 1;
    forwarded.m_payload = event.m_data;
    forwarded.m_payloadSize = event.m_size;
    const int32_t result = deliver(targetEndpoint, forwarded, origin(targetEndpoint));
    if (result == NI_OK)
    {
        ++m_state->m_lastEvent;
    }
    return result;
}