#include "NetSession.h"
#include "NetInterfaceMacro.h"
#include "NetSessionState.h"
#include "NetInterface.h"
#include "NetClientDiscovery.h"
#include "NetInterfaceRequests.h"
#include "NetExecution.h"
#include "NetInterfaceRpc.h"
#include "NetInterfaceTransport.h"
#include <chrono>
#include <cstring>
#include <new>
#include <vector>

/** 原生会话使用与传输一致的单调时钟
@return 毫秒
*/
static uint64_t SessionNow()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetSessionListener::~NetSessionListener()
{

}

NetSession::NetSession() :
m_state(new NetSessionState())
{

}

NetSession::~NetSession()
{
    shutdown();
    delete m_state;
}

int32_t NetSession::configureHeartbeat(const NetHeartbeatConfig& config)
{
    if (m_state->m_generation != 0 || m_state->m_callback)
    {
        return NI_INVALID_STATE;
    }
    if (!config.valid()) { return NI_INVALID_ARGUMENT; }
    m_state->m_heartbeat = config;
    return NI_OK;
}

int32_t NetSession::configureExpiryBudget(uint32_t count)
{
    if (m_state->m_generation != 0 || m_state->m_callback)
    {
        return NI_INVALID_STATE;
    }
    if (count == 0) { return NI_INVALID_ARGUMENT; }
    m_state->m_requestExpiryBudget = count;
    return NI_OK;
}

int32_t NetSession::configurePayloadRetention(int64_t bytes)
{
    if (m_state->m_generation != 0 || m_state->m_callback)
    {
        return NI_INVALID_STATE;
    }
    if (bytes < -1) { return NI_INVALID_ARGUMENT; }
    m_state->m_payloadRetentionBytes = bytes;
    return NI_OK;
}

uint64_t NetSession::payloadCapacity() const
{
    return m_state->m_payload.capacity();
}

int32_t NetSession::configureRequestPool(const LockFreeHashMapPoolConfig& config)
{
    if (m_state->m_generation != 0 || m_state->m_active || m_state->m_polling || m_state->m_callback || !m_state->m_pending.empty())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetObjectPool<NetSessionPending>> replacement(new NetObjectPool<NetSessionPending>(config));
        const int32_t result = replacement->status();
        if (result != NI_OK)
        {
            return result;
        }
        m_state->m_records.swap(replacement);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSession::bindChannel(NetClientChannel* channel)
{
    if (m_state->m_active || m_state->m_callback || m_state->m_polling)
    {
        return NI_INVALID_STATE;
    }
    shutdown();
    m_state->m_external = channel;
    return NI_OK;
}

int32_t NetSession::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls,
                            const NetInterfaceHandshakeOptions& handshake, NetSessionListener* listener)
{
    return connectWithPools(address, port, tls, handshake, listener, nullptr);
}

int32_t NetSession::connectWithPools(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls,
    const NetInterfaceHandshakeOptions& handshake, NetSessionListener* listener, const NetConnectionPoolConfig* pools)
{
    if (m_state->m_active || m_state->m_polling || m_state->m_callback || m_state->m_generation == UINT64_MAX)
    {
        return NI_INVALID_STATE;
    }
    if (listener == nullptr || tls.server != 0 || handshake.m_server != 0 || address == nullptr || *address == 0 ||
        port == 0 || port > 65535 ||
        tls.effectiveHandshakeTimeout() < 100 || tls.handshakeTimeoutMs > 60000 || (pools != nullptr && !pools->validCodecSettings()))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_state->m_external == nullptr)
    {
        return NI_NOT_SUPPORTED;
    }
    NetSessionCallbackScope callback(m_state->m_callback);
    NetInterfaceHandshakeOptions options = {};
    const int32_t validated = NetSessionState::prepareHandshake(handshake, tls, options);
    if (validated != NI_OK) { return validated; }
    if (pools != nullptr)
    {
        int32_t configured;
        try
        {
            configured = m_state->m_external->configureConnectionPools(*pools);
        }
        catch (const std::bad_alloc&)
        {
            configured = NI_NO_MEMORY;
        }
        catch (...)
        {
            configured = NI_INVALID_STATE;
        }
        if (configured != NI_OK)
        {
            return configured;
        }
    }
    m_state->m_channel = m_state->m_external;
    m_state->resetRequests(options.m_maxRequests);
    m_state->m_handshake.reset();
    m_state->m_options = options;
    m_state->m_listener = listener;
    m_state->m_ready = false;
    m_state->m_peer = 0;
    m_state->m_ping = 0;
    m_state->m_pingSent = 0;
    m_state->m_maxChunk = 0;
    m_state->m_capabilities = 0;
    m_state->m_localClose = false;
    m_state->m_closeReason = 0;
    int32_t result = NI_NO_MEMORY;
    try
    {
        result = m_state->m_channel->connect(address, port, tls, this, m_state->m_peer);
        if (result == NI_OK && m_state->m_peer == 0)
        {
            result = NI_PROTOCOL_ERROR;
        }
    }
    catch (const std::bad_alloc&)
    {
        result = NI_NO_MEMORY;
    }
    catch (...)
    {
        result = NI_INVALID_STATE;
    }
    if (result == NI_OK)
    {
        ++m_state->m_generation;
        m_state->m_active = true;
    }
    else
    {
        // 同步拒绝不占用代次，停止适配器可能已分配的部分资源。
        m_state->m_listener = nullptr;
        stopChannel();
    }
    return result;
}

void NetSession::onReady(uint64_t peer)
{
    if (!m_state->m_active || m_state->m_localClose)
    {
        return;
    }
    if (peer != m_state->m_peer || m_state->m_handshake)
    {
        close(NI_PROTOCOL_ERROR);
        return;
    }
    const uint64_t now = SessionNow();
    try
    {
        m_state->m_handshake.reset(new NetInterfaceHandshake(m_state->m_options, now));
    }
    catch (...)
    {
        close(NI_NO_MEMORY);
        return;
    }
    m_state->m_lastReceive = now;
    progressHandshake(now);
}

void NetSession::progressHandshake(uint64_t nowMs)
{
    if (!m_state->m_handshake || !m_state->m_active || m_state->m_localClose)
    {
        return;
    }
    const int32_t status = m_state->m_handshake->progress(nowMs);
    if (status < 0)
    {
        close(status);
        return;
    }
    NetInterfaceFrame frame = {};
    if (m_state->m_handshake->peekOutput(frame) == NI_READY)
    {
        const int32_t result = sendFrame(frame);
        if (result == NI_OK)
        {
            m_state->m_handshake->consumeOutput();
        }
        else if (result != NI_LIMIT_EXCEEDED)
        {
            close(result);
        }
    }
}

bool NetSession::onFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != m_state->m_peer || !m_state->m_active || m_state->m_localClose || !m_state->m_handshake)
    {
        return true;
    }
    const uint64_t now = SessionNow();
    if (m_state->m_ready && m_state->m_heartbeat.expired(now - m_state->m_lastReceive,
            m_state->m_pingSent == 0 ? 0 : now - m_state->m_pingSent))
    {
        close(NI_TIMEOUT);
        return true;
    }
    const int32_t accepted = m_state->m_handshake->accept(frame, now);
    if (accepted < 0)
    {
        close(accepted);
        return true;
    }
    if (!m_state->m_ready)
    {
        m_state->m_lastReceive = now;
        if (accepted != NI_READY)
        {
            return true;
        }
        uint32_t fields[4] = {};
        uint8_t nonce[16] = {};
        uint8_t device[16] = {};
        if (m_state->m_handshake->peerInfo(fields, nonce, device) != NI_READY ||
            fields[2] == 0 || fields[2] > m_state->m_options.m_maxRequests)
        {
            close(NI_PROTOCOL_ERROR);
            return true;
        }
        if ((fields[1] & 8) != 0)
        {
            const int32_t configured = m_state->m_channel->configureCompression(m_state->m_peer,
                m_state->m_compressionTypes.empty() ? nullptr : m_state->m_compressionTypes.data(),
                static_cast<uint32_t>(m_state->m_compressionTypes.size()), true);
            if (configured != NI_OK)
            {
                close(configured);
                return true;
            }
        }
        const int32_t compact = m_state->m_channel->configureCompact(m_state->m_peer, (fields[1] & 16) != 0);
        if (compact != NI_OK)
        {
            close(compact);
            return true;
        }
        m_state->m_maxRequests = fields[2];
        m_state->m_ready = true;
        m_state->m_maxChunk = fields[3];
        m_state->m_capabilities = fields[1];
        notifyReady(fields[2], fields[3]);
        return true;
    }
    if (frame.m_kind == NI_PING || frame.m_kind == NI_PONG)
    {
        NetInterfaceFrame reply = {};
        const int32_t status = NetInterfaceRpc::control(frame, reply);
        if (status == NI_READY)
        {
            const int32_t sent = sendFrame(reply);
            if (sent == NI_LIMIT_EXCEEDED)
            {
                return false;
            }
            if (sent != NI_OK)
            {
                close(sent);
            }
        }
        else if (status != NI_OK || m_state->m_pingSent == 0 || frame.m_requestId != m_state->m_ping)
        {
            close(NI_PROTOCOL_ERROR);
        }
        else
        {
            m_state->m_pingSent = 0;
        }
        m_state->m_lastReceive = now;
        return true;
    }
    if (frame.m_kind == NI_RESPONSE)
    {
        m_state->m_lastReceive = now;
        NetInterfaceRpcView view = {};
        if (NetInterfaceRpc::parse(frame, view) != NI_OK)
        {
            close(NI_PROTOCOL_ERROR);
            return true;
        }
        uint64_t original = frame.m_requestId;
        if (m_state->m_remapped)
        {
            const auto mapped = m_state->m_remapped->find(frame.m_requestId);
            if (mapped != m_state->m_remapped->end()) { original = mapped->second; }
        }
        const auto found = m_state->m_pending.find(original);
        if (found == m_state->m_pending.end())
        {
            return true;
        }
        if (found->second->m_request.m_route != frame.m_requestId)
        {
            // 已退役的线路号可能仍是一条业务预留的原号；迟到或重复的响应不结算它。
            if (frame.m_requestId > m_state->m_highestWire) { close(NI_PROTOCOL_ERROR); }
            return true;
        }
        const NetInterfaceRequest request = found->second->m_request;
        if (frame.m_productId != request.m_product || frame.m_messageType != request.m_type)
        {
            close(NI_PROTOCOL_ERROR);
            return true;
        }
        NetSessionPending completed = {};
        m_state->take(original, completed);
        const bool expired = now >= request.m_deadline;
        notifyResult(request, expired ? NI_RPC_TIMEOUT : view.m_value,
            expired ? nullptr : view.m_data, expired ? 0 : view.m_size);
        return true;
    }
    if ((frame.m_kind == NI_EVENT || frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK) &&
        (frame.m_kind == NI_EVENT || (frame.m_productId != 0 && (m_state->m_capabilities & 2) != 0)) && frame.m_requestId != 0 &&
        (frame.m_kind == NI_EVENT || frame.m_payloadSize <= m_state->m_maxChunk + 128))
    {
        const bool delivered = notifyFrame(frame);
        if (delivered) { m_state->m_lastReceive = now; }
        return delivered;
    }
    close(NI_PROTOCOL_ERROR);
    return true;
}

int32_t NetSession::beginRequest(uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId)
{
    return reserveRequest(0, product, type, SessionNow(), timeoutMs, requestId);
}

int32_t NetSession::beginRequestAt(uint64_t requestId, uint32_t product, uint32_t type, uint64_t startedMs, uint32_t timeoutMs)
{
    if (requestId == 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    uint64_t reserved = 0;
    return reserveRequest(requestId, product, type, startedMs, timeoutMs, reserved);
}

int32_t NetSession::reserveRequest(uint64_t original, uint32_t product, uint32_t type, uint64_t startedMs, uint32_t timeoutMs, uint64_t& requestId)
{
    requestId = 0;
    if (!ready())
    {
        return NI_INVALID_STATE;
    }
    const uint64_t now = SessionNow();
    if (product == 0 || timeoutMs == 0 || timeoutMs > 60000 || startedMs > now || startedMs > UINT64_MAX - timeoutMs)
    {
        return NI_INVALID_ARGUMENT;
    }
    const uint64_t deadline = startedMs + timeoutMs;
    if (now >= deadline) { return NI_TIMEOUT; }
    const int32_t result = m_state->reserve(original, product, type, deadline, requestId);
    if (result == NI_OK && NetExecution::current() != nullptr)
    {
        // 预留可发生在本轮 poll 之后，先登记真实截止时间再交还业务编号。
        NetExecution::deadline(deadline, SessionNow());
    }
    return result;
}

int32_t NetSession::sendFrame(const NetInterfaceFrame& frame)
{
    try
    {
        return m_state->m_channel->send(m_state->m_peer, frame);
    }
    catch (...)
    {
        // 接纳情况不可猜测；返回明确失败，绝不由本层重放请求。
        return NI_INVALID_STATE;
    }
}

int32_t NetSession::sendRequest(uint64_t requestId, const uint8_t* body, uint32_t size)
{
    if (!ready())
    {
        return NI_INVALID_STATE;
    }
    const auto found = m_state->m_pending.find(requestId);
    if (found == m_state->m_pending.end())
    {
        return NI_NOT_FOUND;
    }
    if (found->second->m_request.m_route != 0)
    {
        return NI_INVALID_STATE;
    }
    if ((size != 0 && body == nullptr) || size > 1048560)
    {
        return NI_INVALID_ARGUMENT;
    }
    const uint64_t now = SessionNow();
    if (now >= found->second->m_request.m_deadline)
    {
        return NI_TIMEOUT;
    }
    if (m_state->m_highestWire == UINT64_MAX)
    {
        return NI_LIMIT_EXCEEDED;
    }
    int32_t result = NI_NO_MEMORY;
    try
    {
        m_state->m_payload.resize(size + 16);
        uint32_t required = 0;
        result = NetInterfaceRpc::encode(NI_REQUEST, static_cast<uint32_t>(found->second->m_request.m_deadline - now), 0,
            body, size, m_state->m_payload.data(), static_cast<uint32_t>(m_state->m_payload.size()), required);
        if (result == NI_OK)
        {
            NetInterfaceFrame frame = {};
            frame.m_kind = NI_REQUEST;
            frame.m_productId = found->second->m_request.m_product;
            frame.m_messageType = found->second->m_request.m_type;
            frame.m_requestId = requestId > m_state->m_highestWire ? requestId : m_state->m_highestWire + 1;
            frame.m_payload = m_state->m_payload.data();
            frame.m_payloadSize = required;
            if (frame.m_requestId != requestId)
            {
                // 运输接纳前完成可能失败的索引分配，接纳后只发布线路号。
                if (!m_state->m_remapped) { m_state->m_remapped.reset(new std::unordered_map<uint64_t, uint64_t>()); }
                m_state->m_remapped->emplace(frame.m_requestId, requestId);
            }
            result = sendFrame(frame);
            if (result == NI_OK)
            {
                found->second->m_request.m_route = frame.m_requestId;
                m_state->m_highestWire = frame.m_requestId;
            }
            else if (frame.m_requestId != requestId)
            {
                m_state->m_remapped->erase(frame.m_requestId);
            }
        }
    }
    catch (...)
    {
        result = NI_NO_MEMORY;
    }
    // 异常和编码失败同样擦除，不能只在正常send返回后擦除。
    m_state->clearPayload();
    return result;
}

int32_t NetSession::configureCompression(const uint32_t* types, uint32_t count)
{
    if ((count != 0 && types == nullptr))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_state->m_active || m_state->m_polling || m_state->m_callback)
    {
        return NI_INVALID_STATE;
    }
    for (uint32_t index = 0; index < count; ++index)
    {
        if (types[index] == 0)
        {
            return NI_INVALID_ARGUMENT;
        }
    }
    try
    {
        std::vector<uint32_t> copied;
        if (count != 0)
        {
            copied.assign(types, types + count);
        }
        m_state->m_compressionTypes.swap(copied);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSession::sendCancel(const NetInterfaceRequest& request)
{
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_CANCEL;
    frame.m_requestId = request.m_route;
    frame.m_productId = request.m_product;
    frame.m_messageType = request.m_type;
    return sendFrame(frame);
}

int32_t NetSession::cancel(uint64_t requestId)
{
    NetSessionPending pending = {};
    if (!m_state->take(requestId, pending))
    {
        return NI_NOT_FOUND;
    }
    const int32_t sent = pending.m_request.m_route != 0 && ready() ? sendCancel(pending.m_request) : NI_OK;
    // 已摘除请求的终态必须先交付；控制失败后再关闭，不能丢失本请求。
    notifyResult(pending.m_request, NI_RPC_CANCELLED, nullptr, 0);
    if (sent != NI_OK)
    {
        close(sent);
    }
    return NI_OK;
}

int32_t NetSession::sendData(const NetInterfaceFrame& frame)
{
    if (!ready())
    {
        return NI_INVALID_STATE;
    }
    if ((m_state->m_capabilities & 2) == 0)
    {
        return NI_NOT_SUPPORTED;
    }
    if (frame.m_kind == NI_CANCEL && frame.m_messageType == NI_FILE_DATA_TYPE)
    {
        NetInterfaceFrame unused = {};
        const int32_t validated = NetInterfaceRpc::control(frame, unused);
        return validated == NI_OK ? sendFrame(frame) : validated;
    }
    if ((frame.m_kind != NI_DATA && frame.m_kind != NI_DATA_ACK) || frame.m_productId == 0 || frame.m_requestId == 0 ||
        frame.m_payloadSize > m_state->m_maxChunk + 128 || (frame.m_payloadSize != 0 && frame.m_payload == nullptr))
    {
        return NI_INVALID_ARGUMENT;
    }
    return sendFrame(frame);
}

bool NetSession::poll()
{
    if ((!m_state->m_active && !m_state->m_closeFailed) || m_state->m_polling || m_state->m_callback)
    {
        return false;
    }
    m_state->m_polling = true;
    int32_t failure = NI_OK;
    try
    {
        // poll 中的回调也可能请求关闭并遇到异常；这种情况统一走下方停止分支。
        const bool transportActive = m_state->m_closeFailed || m_state->m_channel->poll();
        if (!transportActive && m_state->m_active && !m_state->m_closeFailed)
        {
            // 运输已没有资源，随后不会再有关闭回调；保留本地主动关闭的原因。
            onClosed(m_state->m_peer, m_state->m_localClose ? m_state->m_closeReason : NI_TRUNCATED, 0);
        }
        progressHandshake(SessionNow());
        for (uint32_t index = 0; index < m_state->m_requestExpiryBudget && ready(); ++index)
        {
            // 回调可提交新请求；每次重新取单调时间，不延长任何原始截止。
            const uint64_t original = m_state->expired(SessionNow());
            if (original == 0)
            {
                break;
            }
            NetSessionPending pending = {};
            m_state->take(original, pending);
            const int32_t sent = pending.m_request.m_route != 0 ? sendCancel(pending.m_request) : NI_OK;
            notifyResult(pending.m_request, NI_RPC_TIMEOUT, nullptr, 0);
            if (sent != NI_OK)
            {
                close(sent);
            }
        }
        if (ready())
        {
            const uint64_t now = SessionNow();
            const NetHeartbeatConfig& heartbeat = m_state->m_heartbeat;
            const uint64_t idle = now - m_state->m_lastReceive;
            if (heartbeat.expired(idle, m_state->m_pingSent == 0 ? 0 : now - m_state->m_pingSent))
            {
                close(NI_TIMEOUT);
            }
            else if (m_state->m_pingSent == 0 && idle >= heartbeat.m_pingIntervalMs)
            {
                if (m_state->m_ping == UINT64_MAX)
                {
                    close(NI_LIMIT_EXCEEDED);
                }
                else
                {
                    NetInterfaceFrame frame = {};
                    frame.m_kind = NI_PING;
                    frame.m_requestId = ++m_state->m_ping;
                    const int32_t result = sendFrame(frame);
                    if (result == NI_OK)
                    {
                        m_state->m_pingSent = now;
                    }
                    else
                    {
                        close(result);
                    }
                }
            }
            if (ready())
            {
                NetExecution::deadline(heartbeat.m_receiveTimeoutMs, idle);
                if (m_state->m_pingSent == 0)
                {
                    NetExecution::deadline(heartbeat.m_pingIntervalMs, idle);
                }
                else
                {
                    NetExecution::deadline(heartbeat.m_pongTimeoutMs, now - m_state->m_pingSent);
                }
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        failure = NI_NO_MEMORY;
    }
    catch (...)
    {
        failure = NI_INVALID_STATE;
    }
    if (m_state->m_closeFailed || (failure != NI_OK && m_state->m_active))
    {
        // 运输调用已经返回，此处可停止通道并结算待办，保留首次关闭原因。
        if (!m_state->m_localClose)
        {
            m_state->m_localClose = true;
            m_state->m_closeReason = failure;
        }
        m_state->m_ready = false;
        stopChannel();
        onClosed(m_state->m_peer, m_state->m_closeReason, 0);
    }
    m_state->m_polling = false;
    return m_state->m_active;
}

bool NetSession::closedRequestStatus(uint32_t& status) const
{
    status = m_state->m_localClose && (m_state->m_closeReason == NI_OK || m_state->m_closeReason == NI_CANCELLED) ?
        NI_RPC_CANCELLED : NI_RPC_UNAVAILABLE;
    return m_state->m_notifyingClose && m_state->m_callback;
}

void NetSession::onClosed(uint64_t peer, int32_t reason, int32_t detail)
{
    if (peer != m_state->m_peer || !m_state->m_active)
    {
        return;
    }
    m_state->m_ready = false;
    m_state->m_active = false;
    NetSessionCallbackScope closing(m_state->m_notifyingClose);
    if (m_state->m_localClose)
    {
        reason = m_state->m_closeReason;
        detail = 0;
    }
    uint32_t status = NI_RPC_UNAVAILABLE;
    closedRequestStatus(status);
    while (!m_state->m_pending.empty())
    {
        NetSessionPending pending = {};
        m_state->take(m_state->m_pending.begin()->first, pending);
        notifyResult(pending.m_request, status, nullptr, 0);
    }
    notifyClosed(reason, detail);
}

void NetSession::close(int32_t reason)
{
    if (!m_state->m_active || m_state->m_localClose)
    {
        return;
    }
    m_state->m_localClose = true;
    m_state->m_closeReason = reason;
    m_state->m_ready = false;
    try
    {
        m_state->m_channel->close(m_state->m_peer, reason);
    }
    catch (...)
    {
        // close 可由运输的帧回调触发，先退回执行者再解绑监听和交付终态。
        m_state->m_closeFailed = true;
        NetExecution::progress();
    }
}

void NetSession::stopChannel()
{
    NetClientChannel* channel = m_state->m_channel;
    m_state->m_channel = nullptr;
    m_state->m_closeFailed = false;
    if (channel == nullptr) { return; }
    NetSessionCallbackScope callback(m_state->m_callback);
    try
    {
        channel->shutdown();
    }
    catch (...)
    {
        // 调用方在运输返回后统一处理尚未交付的终态，包括主动正常关闭。
    }
}

void NetSession::shutdown()
{
    if (m_state->m_callback || m_state->m_polling)
    {
        close();
        return;
    }
    if (m_state->m_active && !m_state->m_localClose)
    {
        m_state->m_localClose = true;
        m_state->m_closeReason = NI_OK;
    }
    m_state->m_ready = false;
    stopChannel();
    if (m_state->m_active)
    {
        // 即使适配器停止时没有额外事件，共用会话也必须结算已接纳请求
        onClosed(m_state->m_peer, m_state->m_closeReason, 0);
    }
    m_state->m_handshake.reset();
    m_state->resetRequests(0);
    std::vector<uint8_t>().swap(m_state->m_payload);
    m_state->m_external = nullptr;
    m_state->m_listener = nullptr;
}

bool NetSession::ready() const
{
    return m_state->m_active && m_state->m_ready;
}

uint64_t NetSession::generation() const
{
    return m_state->m_generation;
}

int32_t NetSession::outputState() const
{
    if (!ready() || m_state->m_channel == nullptr)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        return m_state->m_channel->outputState(m_state->m_peer);
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

void NetSession::notifyReady(uint32_t requests, uint32_t chunk)
{
    try
    {
        NetSessionCallbackScope callback(m_state->m_callback);
        m_state->m_listener->onSessionReady(m_state->m_generation, requests, chunk);
    }
    catch (...)
    {
        close(NI_INVALID_STATE);
    }
}

void NetSession::notifyResult(const NetInterfaceRequest& request, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    try
    {
        NetSessionCallbackScope callback(m_state->m_callback);
        m_state->m_listener->onSessionResult(m_state->m_generation, request.m_original, request.m_product,
            request.m_type, status, bytes, size);
    }
    catch (...)
    {
        close(NI_INVALID_STATE);
    }
}

bool NetSession::notifyFrame(const NetInterfaceFrame& frame)
{
    try
    {
        NetSessionCallbackScope callback(m_state->m_callback);
        return m_state->m_listener->onSessionFrame(m_state->m_generation, frame);
    }
    catch (...)
    {
        close(NI_INVALID_STATE);
        return true;
    }
}

void NetSession::notifyClosed(int32_t reason, int32_t detail)
{
    try
    {
        NetSessionCallbackScope callback(m_state->m_callback);
        m_state->m_listener->onSessionClosed(m_state->m_generation, reason, detail);
    }
    catch (...)
    {
        // 关闭通知只交付一次，观察者异常不会延长该代次。
    }
}