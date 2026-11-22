#include "NetSenderMacro.h"
#include "NetSenderServiceManaged.h"
#include "CSystem/CSystemAPI.h"
#include <algorithm>
#include <chrono>
#include <cstring>

/** 同一执行者的单调毫秒，不使用墙上时钟计算网络期限
@return 单调毫秒
*/
static uint64_t ManagedServiceNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

/** 服务端业务握手使用系统强随机
@param [in] context 忽略
@param [out] output 随机字节
@param [in] size 字节数
@return 0成功
*/
static int32_t ManagedServiceEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return CSystem::secureRandomBytes(output, size) ? 0 : -1;
}

NetSenderServiceManagedPeer::NetSenderServiceManagedPeer() : m_origin(), m_channel(0), m_role(0)
{

}

NetSenderServiceManagedSink::NetSenderServiceManagedSink(NetSenderServiceManaged& owner, bool clients) :
    m_owner(&owner), m_clients(clients)
{

}

int32_t NetSenderServiceManagedSink::send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin)
{
    return m_owner->enqueue(m_clients, endpoint, frame, origin);
}

void NetSenderServiceManagedSink::close(uint64_t endpoint)
{
    m_owner->closeEndpoint(m_clients, endpoint);
}

NetSenderServiceManaged::NetSenderServiceManaged(NetSenderLocalInbox& inbox) : m_services(*this, false), m_clients(*this, true),
    m_router(&m_services, &m_clients), m_gateway(inbox), m_listener(nullptr), m_handler(0), m_internalEndpoint(0),
    m_generation(0), m_foreignParent(0), m_retryAt(0), m_internalReceive(0), m_pingAt(0), m_ping(0), m_retryDelay(250),
    m_product(0), m_limit(0), m_reason(0), m_started(false), m_stopping(false), m_closed(false), m_polling(false), m_internalClosing(false)
{

}

NetSenderServiceManaged::~NetSenderServiceManaged()
{
    shutdown();
    // 即使只完成部分登记，也必须在网关成员析构前停止路由；路由析构可能回调本对象。
    m_stopping = true;
    m_router.shutdown();
}

int32_t NetSenderServiceManaged::configureQueuePools(const NetBufferPoolConfig& bodies, const LockFreeHashMapPoolConfig& records)
{
    if (m_started || m_closed || m_stopping)
    {
        return NI_INVALID_STATE;
    }
    return m_queue.configurePools(bodies, records);
}

int32_t NetSenderServiceManaged::connect(int32_t pid, uint32_t product, NetInterfaceLinkServiceProof proof, void* context,
    NetSenderServiceListener* listener, uint32_t limit, uint32_t weight, uint32_t channels)
{
    if (m_started || m_closed || listener == nullptr || limit == 0) { return NI_INVALID_ARGUMENT; }
    if (m_queue.status() != NI_OK) { return m_queue.status(); }
    const int32_t registered = m_router.addHandler(product, limit, m_handler);
    if (registered != NI_OK) { return registered; }
    m_product = product;
    m_limit = limit;
    m_listener = listener;
    m_started = true;
    const int32_t result = m_gateway.connect(pid, product, proof, context, this, limit, weight, channels);
    if (result != NI_OK) { m_started = false; m_closed = true; m_router.shutdown(); m_listener = nullptr; }
    return result;
}

void NetSenderServiceManaged::onGatewayServiceReady()
{
    if (m_generation != 0 || !CSystem::secureRandomBytes(reinterpret_cast<uint8_t*>(&m_generation), sizeof(m_generation)) ||
        m_generation == 0) { fail(NI_INVALID_STATE); return; }
    m_listener->onServiceReady(m_generation);
}

int32_t NetSenderServiceManaged::attach(uint64_t channel, uint32_t role, const NetInterfaceOrigin& origin, uint64_t& endpoint)
{
    endpoint = 0;
    const auto prior = m_roots.find(origin.m_endpoint);
    if (prior != m_roots.end())
    {
        const auto existing = m_peers.find(prior->second);
        if (existing == m_peers.end() || existing->second.m_channel != channel || existing->second.m_role != role ||
            existing->second.m_origin.m_platform != origin.m_platform ||
            std::memcmp(existing->second.m_origin.m_device, origin.m_device, sizeof(origin.m_device)) != 0)
        {
            return NI_PROTOCOL_ERROR;
        }
        endpoint = prior->second;
        return NI_OK;
    }
    uint32_t fields[4] = {};
    if (m_gateway.clientInfo(channel, fields) != NI_READY) { return NI_INVALID_STATE; }
    if (m_product == 0) { fields[0] = origin.m_platform; }
    return addPeer(channel, role, origin, fields, endpoint);
}

int32_t NetSenderServiceManaged::addPeer(uint64_t channel, uint32_t role, const NetInterfaceOrigin& origin,
    const uint32_t fields[4], uint64_t& endpoint)
{
    endpoint = 0;
    uint64_t created = 0;
    int32_t result = m_router.addClient(origin, fields, created);
    if (result != NI_OK)
    {
        return result;
    }
    NetSenderServiceManagedPeer peer;
    peer.m_channel = channel;
    peer.m_role = role;
    peer.m_origin = origin;
    bool insertedPeer = false;
    try
    {
        insertedPeer = m_peers.insert(std::make_pair(created, peer)).second;
        if (!insertedPeer || !m_roots.insert(std::make_pair(origin.m_endpoint, created)).second)
        {
            result = NI_INVALID_STATE;
        }
    }
    catch (...)
    {
        result = NI_NO_MEMORY;
    }
    if (result != NI_OK)
    {
        if (insertedPeer)
        {
            m_peers.erase(created);
        }
        // 尚未公开的端点不再接收业务；关闭队列无分配，正常poll回收其路由资源。
        m_router.closeEndpoint(created, result);
        return result;
    }
    endpoint = created;
    return result;
}

void NetSenderServiceManaged::onGatewayClientReady(uint64_t channel, uint32_t role, const NetInterfaceOrigin& origin)
{
    if (m_product == 0)
    {
        if (role != NI_LINK_SERVICE) { m_gateway.close(channel, NI_PROTOCOL_ERROR); }
        return;
    }
    uint64_t endpoint = 0;
    const int32_t result = attach(channel, role, origin, endpoint);
    if (result != NI_OK) { m_gateway.close(channel, result); }
}

bool NetSenderServiceManaged::onGatewayClientFrame(uint64_t channel, uint32_t role, const NetInterfaceFrame& frame,
    const NetInterfaceOrigin& direct)
{
    m_foreignParent = 0;
    NetInterfaceOrigin origin = direct;
    NetInterfaceFrame business = frame;
    NetSenderServiceQueueScratch storage;
    std::vector<uint8_t>& localRequest = storage.m_request;
    uint64_t endpoint = 0;
    if (m_product == 0)
    {
        if (role != NI_LINK_SERVICE || NetInterfaceService::unwrap(frame, origin, business) != NI_OK)
        {
            m_gateway.close(channel, NI_PROTOCOL_ERROR);
            return true;
        }
        if (business.m_kind == NI_EVENT && business.m_messageType == NET_INTERFACE_SERVICE_ROOT_CLOSED)
        {
            const auto root = m_roots.find(origin.m_endpoint);
            if (business.m_payloadSize != 0 || business.m_requestId != 0) { m_gateway.close(channel, NI_PROTOCOL_ERROR); return true; }
            if (root == m_roots.end()) { return true; }
            const uint64_t closed = root->second;
            const auto peer = m_peers.find(closed);
            if (peer == m_peers.end() || peer->second.m_channel != channel) { m_gateway.close(channel, NI_PROTOCOL_ERROR); return true; }
            m_roots.erase(root);
            m_peers.erase(peer);
            m_router.closeEndpoint(closed, NI_TRUNCATED);
            return true;
        }
        if (business.m_kind == NI_REQUEST)
        {
            NetInterfaceRpcView rpc = {};
            if (NetInterfaceRpc::parse(business, rpc) != NI_OK || rpc.m_parent == 0)
            {
                m_gateway.close(channel, NI_PROTOCOL_ERROR);
                return true;
            }
            m_foreignParent = rpc.m_parent;
            localRequest.resize(business.m_payloadSize);
            uint32_t required = 0;
            if (NetInterfaceRpc::encode(NI_REQUEST, rpc.m_value, 0, rpc.m_data, rpc.m_size, localRequest.data(),
                static_cast<uint32_t>(localRequest.size()), required) != NI_OK) { fail(NI_PROTOCOL_ERROR); return true; }
            business.m_payload = localRequest.data();
        }
        else if (m_roots.count(origin.m_endpoint) == 0) { return true; }
    }
    const int32_t attached = attach(channel, role, origin, endpoint);
    if (attached != NI_OK) { m_foreignParent = 0; m_gateway.close(channel, attached); return true; }
    const int32_t accepted = m_router.receiveClient(endpoint, business);
    m_foreignParent = 0;
    return accepted != NI_LIMIT_EXCEEDED || (business.m_kind != NI_DATA && business.m_kind != NI_DATA_ACK && business.m_kind != NI_EVENT);
}

void NetSenderServiceManaged::onGatewayClientClosed(uint64_t channel, int32_t reason)
{
    // 路由器最多1023个端点；关闭快照使用固定栈空间，不在内存紧张时再次分配。
    uint64_t closed[1023] = {};
    uint32_t count = 0;
    for (const auto& peer : m_peers)
    {
        if (peer.second.m_channel == channel && count < 1023)
        {
            closed[count++] = peer.first;
        }
    }
    for (uint32_t index = 0; index < count; ++index)
    {
        const uint64_t id = closed[index];
        const auto found = m_peers.find(id);
        if (found == m_peers.end()) { continue; }
        if (m_product != 0 && m_internalEndpoint != 0 && !m_internalClosing && !m_stopping)
        {
            NetInterfaceFrame close = {};
            close.m_kind = NI_EVENT;
            close.m_productId = m_product;
            close.m_messageType = NET_INTERFACE_SERVICE_ROOT_CLOSED;
            if (push(2, m_internalEndpoint, close, found->second.m_origin) != NI_OK) { stopInternal(NI_LIMIT_EXCEEDED); }
        }
        m_roots.erase(found->second.m_origin.m_endpoint);
        m_peers.erase(found);
        m_router.closeEndpoint(id, reason);
    }
}

void NetSenderServiceManaged::onGatewayServiceClosed(int32_t reason)
{
    if (m_closed) { return; }
    if (m_reason != 0) { reason = m_reason; }
    m_stopping = true;
    m_reason = reason;
    stopInternal(reason);
    m_router.shutdown();
    m_queue.clear();
    m_generation = 0;
    m_closed = true;
    if (m_listener != nullptr) { m_listener->onServiceClosed(reason); }
}

int32_t NetSenderServiceManaged::room(uint32_t count, uint64_t bytes)
{
    return m_queue.room(count, bytes);
}

int32_t NetSenderServiceManaged::push(uint32_t lane, uint64_t endpoint,
    const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin)
{
    return m_queue.push(lane, endpoint, frame, origin, ManagedServiceNow());
}

int32_t NetSenderServiceManaged::enqueue(bool clients, uint64_t endpoint, const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin)
{
    if (m_stopping || m_closed) { return NI_INVALID_STATE; }
    if (clients) { return push(1, endpoint, frame, origin); }
    if (endpoint == m_internalEndpoint && endpoint != 0) { return push(2, endpoint, frame, origin); }
    if (endpoint != m_handler) { return NI_NOT_FOUND; }
    if (m_product == 0 && frame.m_kind == NI_REQUEST && m_foreignParent != 0)
    {
        NetInterfaceRpcView rpc = {};
        if (NetInterfaceRpc::parse(frame, rpc) != NI_OK)
        {
            return NI_PROTOCOL_ERROR;
        }
        NetSenderServiceQueueScratch storage;
        std::vector<uint8_t>& bytes = storage.m_request;
        bytes.resize(frame.m_payloadSize);
        uint32_t required = 0;
        if (NetInterfaceRpc::encode(NI_REQUEST, rpc.m_value, m_foreignParent, rpc.m_data, rpc.m_size, bytes.data(),
            static_cast<uint32_t>(bytes.size()), required) != NI_OK) { return NI_PROTOCOL_ERROR; }
        NetInterfaceFrame adjusted = frame;
        adjusted.m_payload = bytes.data();
        return push(0, endpoint, adjusted, origin);
    }
    return push(0, endpoint, frame, origin);
}

void NetSenderServiceManaged::closeEndpoint(bool clients, uint64_t endpoint)
{
    if (m_stopping || m_closed) { return; }
    if (clients)
    {
        const auto found = m_peers.find(endpoint);
        if (found != m_peers.end()) { m_gateway.close(found->second.m_channel, NI_TRUNCATED); }
    }
    else if (endpoint == m_internalEndpoint) { stopInternal(NI_TRUNCATED); }
    else if (endpoint == m_handler) { fail(NI_PROTOCOL_ERROR); }
}

void NetSenderServiceManaged::onReady(uint64_t peer)
{
    if (peer != 1 || m_internalHandshake) { stopInternal(NI_PROTOCOL_ERROR); return; }
    NetInterfaceHandshakeOptions options = {};
    options.m_platform = 1;
    options.m_capabilities = 7;
    options.m_maxRequests = m_limit;
    options.m_maxChunk = 65536;
    options.m_timeoutMs = 5000;
    if (ManagedServiceEntropy(nullptr, options.m_nonce, sizeof(options.m_nonce)) != 0 ||
        ManagedServiceEntropy(nullptr, options.m_device, sizeof(options.m_device)) != 0) { stopInternal(NI_INVALID_STATE); return; }
    m_internalReceive = ManagedServiceNow();
    m_pingAt = m_internalReceive;
    m_internalHandshake.reset(new NetInterfaceHandshake(options, m_internalReceive));
}

bool NetSenderServiceManaged::onFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != 1 || m_internalClosing || !m_internalHandshake) { return true; }
    const uint64_t now = ManagedServiceNow();
    const int32_t accepted = m_internalHandshake->accept(frame, now);
    if (accepted < 0) { stopInternal(accepted); return true; }
    m_internalReceive = now;
    if (accepted != NI_OK) { return true; }
    if (frame.m_kind == NI_PING || frame.m_kind == NI_PONG)
    {
        NetInterfaceFrame pong = {};
        const int32_t result = NetInterfaceRpc::control(frame, pong);
        if (result < 0 || (frame.m_kind == NI_PONG && frame.m_requestId != m_ping)) { stopInternal(NI_PROTOCOL_ERROR); return true; }
        if (frame.m_kind == NI_PONG) { m_pingAt = now; return true; }
        const int32_t sent = m_internalChannel->send(1, pong);
        if (sent == NI_LIMIT_EXCEEDED) { return false; }
        if (sent != NI_OK) { stopInternal(sent); }
        return true;
    }
    if (m_internalEndpoint == 0) { stopInternal(NI_PROTOCOL_ERROR); return true; }
    NetInterfaceOrigin origin = {};
    NetInterfaceFrame business = {};
    if (NetInterfaceService::unwrap(frame, origin, business) != NI_OK)
    {
        stopInternal(NI_PROTOCOL_ERROR); return true;
    }
    if (business.m_kind == NI_EVENT && !(business.m_messageType == NI_FILE_CANCEL && business.m_payloadSize >= 4 &&
        std::memcmp(business.m_payload, "NFN2", 4) == 0))
    {
        NetInterfaceEventView view = {};
        if (NetInterfaceEvent::parse(business, view) != NI_OK ||
            (view.m_flags == 0 && (origin.m_endpoint == 0 || view.m_target != origin.m_endpoint)))
        {
            stopInternal(NI_PROTOCOL_ERROR);
            return true;
        }
    }
    const int32_t result = m_router.receiveService(m_internalEndpoint, business);
    if (result == NI_LIMIT_EXCEEDED && (business.m_kind == NI_EVENT || business.m_kind == NI_DATA || business.m_kind == NI_DATA_ACK)) { return false; }
    if (result < 0) { stopInternal(result); }
    return true;
}

void NetSenderServiceManaged::onClosed(uint64_t peer, int32_t reason, int32_t detail)
{
    (void)peer;
    (void)detail;
    stopInternal(reason);
}

void NetSenderServiceManaged::stopInternal(int32_t reason)
{
    if (m_internalClosing) { return; }
    m_internalClosing = true;
    const uint64_t endpoint = m_internalEndpoint;
    m_internalEndpoint = 0;
    if (endpoint != 0) { m_router.closeEndpoint(endpoint, reason); }
    m_queue.clear(2);
    if (m_internalChannel) { m_internalChannel->close(1, reason); }
}

void NetSenderServiceManaged::pumpInternal(uint64_t now)
{
    if (m_product == 0 || m_stopping || m_generation == 0) { return; }
    if (m_internalChannel && !m_internalClosing) { m_internalChannel->poll(); }
    now = ManagedServiceNow();
    if (m_internalClosing)
    {
        if (m_internalChannel) { m_internalChannel->shutdown(); }
        m_internalHandshake.reset();
        m_internalChannel.reset();
        m_internalTransport.reset();
        m_internalClosing = false;
        m_retryAt = now + m_retryDelay;
        m_retryDelay = (std::min)(m_retryDelay * 2, 5000U);
    }
    if (!m_internalChannel && now >= m_retryAt)
    {
        m_internalTransport.reset(new NetSenderServiceOutgoingTransport(m_gateway));
        m_internalChannel.reset(new NetGatewayClientChannel(0, m_internalTransport.get()));
        NetInterfaceTlsOptions tls = {};
        tls.entropy = CSystem::secureRandomBytesCallback;
        tls.handshakeTimeoutMs = 5000;
        uint64_t peer = 0;
        if (m_internalChannel->connect("local", 1, tls, this, peer) != NI_OK) { stopInternal(NI_TRUNCATED); }
    }
    if (!m_internalHandshake || m_internalClosing) { return; }
    const int32_t status = m_internalHandshake->progress(now);
    if (status < 0 || now - m_internalReceive >= 45000) { stopInternal(status < 0 ? status : NI_TIMEOUT); return; }
    NetInterfaceFrame hello = {};
    if (m_internalHandshake->peekOutput(hello) == NI_READY)
    {
        const int32_t sent = m_internalChannel->send(1, hello);
        if (sent == NI_OK) { m_internalHandshake->consumeOutput(); }
        else if (sent != NI_LIMIT_EXCEEDED) { stopInternal(sent); return; }
    }
    if (m_internalHandshake->progress(now) == NI_READY && m_internalEndpoint == 0)
    {
        if (m_router.addService(0, true, m_limit, m_internalEndpoint) != NI_OK) { stopInternal(NI_LIMIT_EXCEEDED); return; }
        m_retryDelay = 250;
    }
    if (m_internalEndpoint != 0 && now - m_pingAt >= 15000)
    {
        NetInterfaceFrame ping = {};
        ping.m_kind = NI_PING;
        ping.m_requestId = m_ping + 1;
        if (ping.m_requestId == 0) { stopInternal(NI_LIMIT_EXCEEDED); return; }
        const int32_t sent = m_internalChannel->send(1, ping);
        if (sent == NI_OK) { ++m_ping; m_pingAt = now; }
        else if (sent != NI_LIMIT_EXCEEDED) { stopInternal(sent); }
    }
}

uint64_t NetSenderServiceManaged::queueTarget(const NetSenderServiceManagedFrame& frame, uint32_t kind) const
{
    if (kind == 0) { return frame.m_origin.m_endpoint; }
    if (kind != 1) { return frame.m_endpoint; }
    if (frame.m_channel != 0) { return frame.m_channel; }
    const auto found = m_peers.find(frame.m_endpoint);
    return found == m_peers.end() ? 0 : found->second.m_channel;
}

void NetSenderServiceManaged::pumpQueue(uint32_t kind, uint64_t now)
{
    uint32_t budget = (std::min)(m_queue.size(kind), 32U);
    uint64_t blocked[32] = {};
    uint32_t blockedCount = 0;
    while (budget-- != 0 && m_queue.size(kind) != 0 && !m_stopping)
    {
        uint32_t selected = m_queue.first(kind);
        while (selected != UINT32_MAX)
        {
            const NetSenderServiceManagedFrame* candidate = m_queue.get(selected);
            if (candidate == nullptr)
            {
                fail(NI_INVALID_STATE);
                return;
            }
            const uint64_t target = queueTarget(*candidate, kind);
            bool skip = false;
            for (uint32_t index = 0; index < blockedCount; ++index)
            {
                skip = skip || blocked[index] == target;
            }
            if (!skip)
            {
                break;
            }
            selected = candidate->m_next;
        }
        if (selected == UINT32_MAX)
        {
            break;
        }
        now = ManagedServiceNow();
        NetSenderServiceManagedFrame* item = m_queue.borrow(selected);
        if (item == nullptr)
        {
            fail(NI_INVALID_STATE);
            return;
        }
        // 借用仍占用共享额度；回调重入填满、清理、失败或提前返回均不能释放其正文。
        NetSenderServiceQueueLoan loan(m_queue, selected);
        NetSenderServiceQueueScratch storage;
        NetInterfaceFrame frame = {};
        const int32_t available = item->view(now, frame, storage.m_request);
        if (available == NI_TIMEOUT) { continue; }
        if (available != NI_READY) { fail(available); return; }
        int32_t result = NI_OK;
        if (kind == 0)
        {
            result = m_listener->onServiceFrame(frame, item->m_origin) ? NI_OK : NI_LIMIT_EXCEEDED;
            if (result == NI_LIMIT_EXCEEDED) { blocked[blockedCount++] = item->m_origin.m_endpoint; }
        }
        else if (kind == 1)
        {
            const auto found = m_peers.find(item->m_endpoint);
            if (item->m_channel == 0 && found == m_peers.end()) { continue; }
            const uint64_t channel = item->m_channel != 0 ? item->m_channel : found->second.m_channel;
            std::vector<uint8_t>& event = storage.m_event;
            if (m_product == 0 && item->m_channel == 0 && frame.m_kind == NI_EVENT &&
                !(frame.m_messageType == NI_FILE_CANCEL && frame.m_payloadSize >= 4 && std::memcmp(frame.m_payload, "NFN2", 4) == 0))
            {
                uint32_t required = 0;
                if (NetInterfaceEvent::encode(item->m_origin.m_endpoint, 0, frame.m_payload, frame.m_payloadSize,
                    nullptr, 0, required) != NI_BUFFER_TOO_SMALL) { fail(NI_PROTOCOL_ERROR); return; }
                event.resize(required);
                if (NetInterfaceEvent::encode(item->m_origin.m_endpoint, 0, frame.m_payload, frame.m_payloadSize,
                    event.data(), required, required) != NI_OK) { fail(NI_PROTOCOL_ERROR); return; }
                frame.m_payload = event.data();
                frame.m_payloadSize = required;
            }
            if (m_product != 0 && frame.m_productId == 0 && frame.m_kind == NI_EVENT) { frame.m_productId = m_product; }
            std::vector<uint8_t>& wrapped = storage.m_wrapped;
            if (m_product == 0)
            {
                uint32_t required = 0;
                wrapped.resize(frame.m_payloadSize + 32);
                result = NetInterfaceService::wrap(item->m_origin, frame.m_payload, frame.m_payloadSize, wrapped.data(),
                    static_cast<uint32_t>(wrapped.size()), required);
                if (result != NI_OK) { fail(result); return; }
                frame.m_payload = wrapped.data();
                frame.m_payloadSize = required;
            }
            result = m_gateway.send(channel, frame);
            if (result == NI_LIMIT_EXCEEDED) { blocked[blockedCount++] = channel; }
            if (result != NI_OK && result != NI_LIMIT_EXCEEDED) { m_gateway.close(channel, result); }
        }
        else
        {
            if (m_internalEndpoint == 0 || item->m_endpoint != m_internalEndpoint || !m_internalChannel) { continue; }
            std::vector<uint8_t>& wrapped = storage.m_wrapped;
            wrapped.resize(frame.m_payloadSize + 32);
            uint32_t required = 0;
            result = NetInterfaceService::wrap(item->m_origin, frame.m_payload, frame.m_payloadSize, wrapped.data(),
                static_cast<uint32_t>(wrapped.size()), required);
            if (result != NI_OK) { stopInternal(result); return; }
            frame.m_payload = wrapped.data();
            frame.m_payloadSize = required;
            result = m_internalChannel->send(1, frame);
            if (result != NI_OK && result != NI_LIMIT_EXCEEDED) { stopInternal(result); }
        }
        if (m_stopping || m_closed) { return; }
        if (result == NI_LIMIT_EXCEEDED)
        {
            if (item->m_blocked == 0) { item->m_blocked = now; }
            if (now - item->m_blocked >= 5000)
            {
                const uint64_t endpoint = item->m_endpoint;
                const uint64_t channel = item->m_channel;
                // 回调借用已结束；先归还原项额度，再产生它的BUSY或关闭结果。
                loan.release();
                if (kind == 2) { stopInternal(NI_TIMEOUT); }
                else if (kind == 1 && channel == 0) { m_router.closeEndpoint(endpoint, NI_TIMEOUT); }
                else if (kind == 1) { m_gateway.close(channel, NI_TIMEOUT); }
                else if (frame.m_kind == NI_REQUEST)
                {
                    // No callback accepted this request, so report BUSY for it
                    // instead of tearing down every client of the service.
                    uint8_t bytes[16] = {};
                    uint32_t required = 0;
                    if (NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_BUSY, 0, nullptr, 0, bytes, sizeof(bytes), required) != NI_OK)
                    {
                        fail(NI_PROTOCOL_ERROR); return;
                    }
                    NetInterfaceFrame response = frame;
                    response.m_kind = NI_RESPONSE;
                    response.m_payload = bytes;
                    response.m_payloadSize = required;
                    const int32_t rejected = m_router.receiveService(m_handler, response);
                    if (rejected != NI_OK) { fail(rejected); return; }
                }
                else { fail(NI_TIMEOUT); }
            }
            else
            {
                if (!loan.restore())
                {
                    fail(NI_INVALID_STATE);
                    return;
                }
                if (kind == 2) { break; }
            }
        }
    }
}

int32_t NetSenderServiceManaged::configureCompression(const uint32_t* types, uint32_t count)
{
    return m_gateway.configureCompression(types, count);
}

int32_t NetSenderServiceManaged::send(const NetInterfaceFrame& frame)
{
    if (frame.m_kind == NI_EVENT)
    {
        NetInterfaceEventView event = {};
        if (NetInterfaceEvent::parse(frame, event) == NI_OK && event.m_flags == 0 && m_roots.count(event.m_target) == 0)
        {
            // Public sends can report a missing target. Incoming delayed events
            // still use the router's harmless stale-event discard semantics.
            return NI_NOT_FOUND;
        }
    }
    if (endpoint() == 0) { return NI_INVALID_STATE; }
    if (m_product == 0 && frame.m_kind == NI_EVENT)
    {
        NetInterfaceEventView event = {};
        if (NetInterfaceEvent::parse(frame, event) == NI_OK && event.m_flags == 1)
        {
            std::set<uint64_t> targets;
            for (const auto& peer : m_peers)
            {
                if (event.m_target == 0 || event.m_target == peer.second.m_origin.m_endpoint) { targets.insert(peer.second.m_channel); }
            }
            const int32_t storage = room(static_cast<uint32_t>(targets.size()), static_cast<uint64_t>(targets.size()) * frame.m_payloadSize);
            if (storage != NI_OK) { return storage; }
            const int32_t routed = m_router.receiveService(m_handler, frame);
            if (routed != NI_OK) { return routed; }
            for (uint64_t channel : targets)
            {
                const int32_t pushed = push(1, 0, frame, NetInterfaceOrigin());
                if (pushed != NI_OK) { fail(pushed); return pushed; }
                m_queue.get(m_queue.last(1))->m_channel = channel;
            }
            return NI_OK;
        }
    }
    return m_router.receiveService(m_handler, frame);
}

void NetSenderServiceManaged::fail(int32_t reason)
{
    if (m_stopping || m_closed) { return; }
    m_stopping = true;
    m_reason = reason;
    m_gateway.shutdown();
}

bool NetSenderServiceManaged::poll()
{
    if (!m_started || m_closed) { return false; }
    if (m_polling) { return !m_stopping; }
    m_polling = true;
    try
    {
        m_gateway.poll();
        const uint64_t now = ManagedServiceNow();
        if (!m_stopping && !m_closed)
        {
            pumpInternal(now);
            m_router.poll();
            pumpQueue(0, now);
            pumpQueue(2, now);
            pumpQueue(1, now);
        }
    }
    catch (...) { fail(NI_NO_MEMORY); }
    m_polling = false;
    return !m_closed;
}

void NetSenderServiceManaged::shutdown()
{
    if (!m_started || m_closed) { return; }
    fail(0);
    if (!m_polling) { poll(); }
    if (m_internalChannel) { m_internalChannel->shutdown(); }
    m_internalHandshake.reset();
    m_internalChannel.reset();
    m_internalTransport.reset();
    m_router.shutdown();
}

uint64_t NetSenderServiceManaged::endpoint() const
{
    return !m_stopping && !m_closed ? m_generation : 0;
}