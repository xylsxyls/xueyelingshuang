#include "NetInterfaceMacro.h"
#include "NetSessionState.h"
#include "NetInterface.h"
#include "NetClientDiscovery.h"
#include "NetInterfaceRequests.h"
#include "NetInterfaceRpc.h"
#include "NetInterfaceTransport.h"
#include "NetSession.h"
#include <chrono>
#include <cstring>
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

NetSessionState::NetSessionState() :
m_channel(nullptr),
m_external(nullptr),
m_expiryCount(0),
m_highestOriginal(0),
m_maxRequests(0),
m_listener(nullptr),
m_peer(0),
m_generation(0),
m_lastReceive(0),
m_ping(0),
m_pingSent(0),
m_maxChunk(0),
m_capabilities(0),
m_active(false),
m_ready(false),
m_polling(false),
m_callback(false),
m_localClose(false),
m_closeReason(0)
{
    std::memset(&m_options, 0, sizeof(m_options));
    std::memset(m_expiry, 0, sizeof(m_expiry));
}

NetSessionState::~NetSessionState()
{
    clearPayload();
}

void NetSessionState::resetRequests(uint32_t limit)
{
    m_expiryCount = 0;
    std::memset(m_expiry, 0, sizeof(m_expiry));
    m_pending.clear();
    m_highestOriginal = 0;
    m_maxRequests = limit;
    clearPayload();
}

bool NetSessionState::earlier(const NetSessionPending* left, const NetSessionPending* right)
{
    return left->m_request.m_deadline < right->m_request.m_deadline ||
        (left->m_request.m_deadline == right->m_request.m_deadline && left->m_request.m_original < right->m_request.m_original);
}

void NetSessionState::exchange(size_t left, size_t right)
{
    NetSessionPending* saved = m_expiry[left];
    m_expiry[left] = m_expiry[right];
    m_expiry[right] = saved;
    m_expiry[left]->m_heapIndex = left;
    m_expiry[right]->m_heapIndex = right;
}

void NetSessionState::rise(size_t index)
{
    while (index != 0)
    {
        const size_t parent = (index - 1) / 2;
        if (!earlier(m_expiry[index], m_expiry[parent]))
        {
            break;
        }
        exchange(index, parent);
        index = parent;
    }
}

void NetSessionState::sink(size_t index)
{
    while (index * 2 + 1 < m_expiryCount)
    {
        size_t child = index * 2 + 1;
        if (child + 1 < m_expiryCount && earlier(m_expiry[child + 1], m_expiry[child]))
        {
            ++child;
        }
        if (!earlier(m_expiry[child], m_expiry[index]))
        {
            break;
        }
        exchange(index, child);
        index = child;
    }
}

int32_t NetSessionState::reserve(uint64_t original, uint32_t product, uint32_t type, uint64_t deadline, uint64_t& requestId)
{
    requestId = 0;
    if (product == 0 || deadline == 0 || m_maxRequests == 0 || m_maxRequests > 128)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (original == 0)
    {
        if (m_highestOriginal == UINT64_MAX)
        {
            return NI_LIMIT_EXCEEDED;
        }
        original = m_highestOriginal + 1;
    }
    if (original <= m_highestOriginal)
    {
        return NI_PROTOCOL_ERROR;
    }
    m_highestOriginal = original;
    if (m_expiryCount >= m_maxRequests)
    {
        return NI_LIMIT_EXCEEDED;
    }
    NetSessionPending pending = {};
    pending.m_request.m_source = 1;
    pending.m_request.m_target = 2;
    pending.m_request.m_origin = 1;
    pending.m_request.m_route = original;
    pending.m_request.m_original = original;
    pending.m_request.m_product = product;
    pending.m_request.m_type = type;
    pending.m_request.m_deadline = deadline;
    pending.m_heapIndex = m_expiryCount;
    try
    {
        const auto added = m_pending.insert(std::make_pair(original, pending));
        if (!added.second)
        {
            return NI_PROTOCOL_ERROR;
        }
        m_expiry[m_expiryCount++] = &added.first->second;
        rise(added.first->second.m_heapIndex);
    }
    catch (...)
    {
        // map分配是唯一可失败步骤；堆尚未接纳，不需要第二份等待表回滚。
        return NI_NO_MEMORY;
    }
    requestId = original;
    return NI_OK;
}

bool NetSessionState::take(uint64_t original, NetSessionPending& pending)
{
    const auto found = m_pending.find(original);
    if (found == m_pending.end())
    {
        return false;
    }
    pending = found->second;
    const size_t index = pending.m_heapIndex;
    --m_expiryCount;
    if (index != m_expiryCount)
    {
        m_expiry[index] = m_expiry[m_expiryCount];
        m_expiry[index]->m_heapIndex = index;
        if (index != 0 && earlier(m_expiry[index], m_expiry[(index - 1) / 2]))
        {
            rise(index);
        }
        else
        {
            sink(index);
        }
    }
    m_expiry[m_expiryCount] = nullptr;
    m_pending.erase(found);
    return true;
}

uint64_t NetSessionState::expired(uint64_t nowMs) const
{
    return m_expiryCount != 0 && m_expiry[0]->m_request.m_deadline <= nowMs ? m_expiry[0]->m_request.m_original : 0;
}

void NetSessionState::clearPayload()
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

NetSession::NetSession() :
m_state(new NetSessionState())
{

}

NetSession::~NetSession()
{
    shutdown();
    delete m_state;
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
    if (m_state->m_active || m_state->m_polling || m_state->m_callback || m_state->m_generation == UINT64_MAX)
    {
        return NI_INVALID_STATE;
    }
    if (listener == nullptr || tls.server != 0 || handshake.m_server != 0 || address == nullptr || port == 0 || port > 65535)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_state->m_external == nullptr)
    {
        return NI_NOT_SUPPORTED;
    }
    NetInterfaceHandshakeOptions options = handshake;
    if (NetInterface::entropyBytes(tls.entropy, tls.entropyContext, options.m_nonce, sizeof(options.m_nonce)) != NI_OK)
    {
        return NI_INVALID_STATE;
    }
    const uint64_t now = SessionNow();
    NetInterfaceHandshake validation(options, now);
    if (validation.progress(now) < 0)
    {
        return NI_INVALID_ARGUMENT;
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
    ++m_state->m_generation;
    m_state->m_localClose = false;
    m_state->m_closeReason = 0;
    m_state->m_active = true;
    int32_t result = NI_NO_MEMORY;
    try
    {
        result = m_state->m_channel->connect(address, port, tls, this, m_state->m_peer);
        if (result == NI_OK && m_state->m_peer == 0)
        {
            result = NI_PROTOCOL_ERROR;
        }
    }
    catch (...)
    {
        result = NI_NO_MEMORY;
    }
    if (result != NI_OK)
    {
        // 未接纳连接不交付业务终态；适配器部分启动的资源仍须停止。
        m_state->m_active = false;
        m_state->m_listener = nullptr;
        try
        {
            m_state->m_channel->shutdown();
        }
        catch (...)
        {
            // 适配器必须先收敛借用；异常不能把会话重新变成active。
        }
    }
    return result;
}

void NetSession::onReady(uint64_t peer)
{
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
    if (!m_state->m_handshake || !m_state->m_active)
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
    const uint64_t now = SessionNow();
    if (peer != m_state->m_peer || !m_state->m_active || !m_state->m_handshake)
    {
        return true;
    }
    const int32_t accepted = m_state->m_handshake->accept(frame, now);
    if (accepted < 0)
    {
        close(accepted);
        return true;
    }
    m_state->m_lastReceive = now;
    if (!m_state->m_ready)
    {
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
        return true;
    }
    if (frame.m_kind == NI_RESPONSE)
    {
        NetInterfaceRpcView view = {};
        if (NetInterfaceRpc::parse(frame, view) != NI_OK)
        {
            close(NI_PROTOCOL_ERROR);
            return true;
        }
        const auto found = m_state->m_pending.find(frame.m_requestId);
        if (found == m_state->m_pending.end())
        {
            return true;
        }
        if (!found->second.m_sent)
        {
            close(NI_PROTOCOL_ERROR);
            return true;
        }
        const NetInterfaceRequest request = found->second.m_request;
        if (frame.m_productId != request.m_product || frame.m_messageType != request.m_type)
        {
            close(NI_PROTOCOL_ERROR);
            return true;
        }
        NetSessionPending completed = {};
        m_state->take(frame.m_requestId, completed);
        const bool expired = now >= request.m_deadline;
        notifyResult(request, expired ? NI_RPC_TIMEOUT : view.m_value,
            expired ? nullptr : view.m_data, expired ? 0 : view.m_size);
        return true;
    }
    if ((frame.m_kind == NI_EVENT || frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK) &&
        (frame.m_kind == NI_EVENT || (frame.m_productId != 0 && (m_state->m_capabilities & 2) != 0)) && frame.m_requestId != 0 &&
        (frame.m_kind == NI_EVENT || frame.m_payloadSize <= m_state->m_maxChunk + 128))
    {
        return notifyFrame(frame);
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
    return now >= deadline ? NI_TIMEOUT : m_state->reserve(original, product, type, deadline, requestId);
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
    if (found->second.m_sent)
    {
        return NI_INVALID_STATE;
    }
    if ((size != 0 && body == nullptr) || size > 1048560)
    {
        return NI_INVALID_ARGUMENT;
    }
    const uint64_t now = SessionNow();
    if (now >= found->second.m_request.m_deadline)
    {
        return NI_TIMEOUT;
    }
    int32_t result = NI_NO_MEMORY;
    try
    {
        m_state->m_payload.resize(size + 16);
        uint32_t required = 0;
        result = NetInterfaceRpc::encode(NI_REQUEST, static_cast<uint32_t>(found->second.m_request.m_deadline - now), 0,
            body, size, m_state->m_payload.data(), static_cast<uint32_t>(m_state->m_payload.size()), required);
        if (result == NI_OK)
        {
            NetInterfaceFrame frame = {};
            frame.m_kind = NI_REQUEST;
            frame.m_productId = found->second.m_request.m_product;
            frame.m_messageType = found->second.m_request.m_type;
            frame.m_requestId = requestId;
            frame.m_payload = m_state->m_payload.data();
            frame.m_payloadSize = required;
            result = sendFrame(frame);
            if (result == NI_OK)
            {
                found->second.m_sent = true;
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
    if (count > 64 || (count != 0 && types == nullptr))
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
    frame.m_requestId = request.m_original;
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
    const int32_t sent = pending.m_sent && ready() ? sendCancel(pending.m_request) : NI_OK;
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
    if (!m_state->m_active || m_state->m_polling || m_state->m_callback)
    {
        return false;
    }
    m_state->m_polling = true;
    try
    {
        m_state->m_channel->poll();
        const uint64_t now = SessionNow();
        progressHandshake(now);
        if (ready())
        {
            for (uint32_t index = 0; index < 128 && ready(); ++index)
            {
                // 回调可提交新请求；每次重新取单调时间，不延长任何原始截止。
                const uint64_t original = m_state->expired(SessionNow());
                if (original == 0)
                {
                    break;
                }
                NetSessionPending pending = {};
                m_state->take(original, pending);
                const int32_t sent = pending.m_sent ? sendCancel(pending.m_request) : NI_OK;
                notifyResult(pending.m_request, NI_RPC_TIMEOUT, nullptr, 0);
                if (sent != NI_OK)
                {
                    close(sent);
                }
            }
            if (!ready())
            {
                m_state->m_polling = false;
                return m_state->m_active;
            }
            if ((m_state->m_pingSent != 0 && now - m_state->m_pingSent >= 60000) || now - m_state->m_lastReceive >= 90000)
            {
                close(NI_TIMEOUT);
            }
            else if (m_state->m_pingSent == 0 && now - m_state->m_lastReceive >= 30000)
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
        }
    }
    catch (...)
    {
        // 任一外部适配器或容器异常均不得遗留polling，按同一关闭路径结算。
        close(NI_INVALID_STATE);
    }
    m_state->m_polling = false;
    return m_state->m_active;
}

void NetSession::onClosed(uint64_t peer, int32_t reason, int32_t detail)
{
    if (peer != m_state->m_peer || !m_state->m_active)
    {
        return;
    }
    m_state->m_ready = false;
    m_state->m_active = false;
    const uint32_t status = m_state->m_localClose && (m_state->m_closeReason == NI_OK || m_state->m_closeReason == NI_CANCELLED) ?
        NI_RPC_CANCELLED : NI_RPC_UNAVAILABLE;
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
    if (m_state->m_active && !m_state->m_localClose)
    {
        m_state->m_localClose = true;
        m_state->m_closeReason = reason;
    }
    m_state->m_ready = false;
    if (m_state->m_active && m_state->m_channel)
    {
        try
        {
            m_state->m_channel->close(m_state->m_peer, reason);
        }
        catch (...)
        {
            // 借用适配器违反异常约定时也结算本地请求，不保留永久active状态。
            onClosed(m_state->m_peer, NI_INVALID_STATE, 0);
        }
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
    if (m_state->m_channel)
    {
        try
        {
            m_state->m_channel->shutdown();
        }
        catch (...)
        {
            onClosed(m_state->m_peer, NI_INVALID_STATE, 0);
        }
    }
    if (m_state->m_active)
    {
        // 即使适配器停止时没有额外事件，共用会话也必须结算已接纳请求
        onClosed(m_state->m_peer, 0, 0);
    }
    m_state->m_handshake.reset();
    m_state->resetRequests(0);
    m_state->m_channel = nullptr;
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
    const bool callback = m_state->m_callback;
    m_state->m_callback = true;
    bool failed = false;
    try
    {
        m_state->m_listener->onSessionReady(m_state->m_generation, requests, chunk);
    }
    catch (...)
    {
        failed = true;
    }
    m_state->m_callback = callback;
    if (failed)
    {
        close(NI_INVALID_STATE);
    }
}

void NetSession::notifyResult(const NetInterfaceRequest& request, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    const bool callback = m_state->m_callback;
    m_state->m_callback = true;
    bool failed = false;
    try
    {
        m_state->m_listener->onSessionResult(m_state->m_generation, request.m_original, request.m_product,
            request.m_type, status, bytes, size);
    }
    catch (...)
    {
        failed = true;
    }
    m_state->m_callback = callback;
    if (failed)
    {
        close(NI_INVALID_STATE);
    }
}

bool NetSession::notifyFrame(const NetInterfaceFrame& frame)
{
    const bool callback = m_state->m_callback;
    m_state->m_callback = true;
    bool accepted = true;
    bool failed = false;
    try
    {
        accepted = m_state->m_listener->onSessionFrame(m_state->m_generation, frame);
    }
    catch (...)
    {
        failed = true;
    }
    m_state->m_callback = callback;
    if (failed)
    {
        close(NI_INVALID_STATE);
    }
    return accepted;
}

void NetSession::notifyClosed(int32_t reason, int32_t detail)
{
    const bool callback = m_state->m_callback;
    m_state->m_callback = true;
    try
    {
        m_state->m_listener->onSessionClosed(m_state->m_generation, reason, detail);
    }
    catch (...)
    {
        // 终态已产生，不重发通知，也不让观察者异常阻止本对象回收。
    }
    m_state->m_callback = callback;
}

// NET_PROTOCOL_SESSION_UNIT_BEGIN
//#include "NetInterface.h"
//#include "NetInterface/NetClientSessionTest.h"
//#ifdef STATIC_LIB
//#include "NetInterface/NetSessionState.h"
//#endif
//#include <algorithm>
//#include <chrono>
//#include <cstdio>
//#include <cstring>
//#include <iterator>
//#include <map>
//#if defined(_WIN32) && defined(_DEBUG)
//#include <crtdbg.h>
//#endif
//NetClientSessionTestTransport::NetClientSessionTestTransport(NetClientSessionTestCounters& counters) :
//m_counters(&counters)
//{
//
//}
//
//NetClientSessionTestTransport::~NetClientSessionTestTransport()
//{
//    ++m_counters->m_destroyed;
//}
//
//int32_t NetClientSessionTestTransport::connect(const char* address, uint32_t port,
//    const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener, uint64_t& peer)
//{
//    (void)address;
//    (void)port;
//    (void)options;
//    (void)listener;
//    ++m_counters->m_connected;
//    peer = 0;
//    return m_counters->m_connectResult;
//}
//
//bool NetClientSessionTestTransport::poll()
//{
//    return false;
//}
//
//int32_t NetClientSessionTestTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
//{
//    (void)peer;
//    (void)frame;
//    return NI_INVALID_STATE;
//}
//
//int32_t NetClientSessionTestTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
//{
//    (void)peer;
//    (void)record;
//    return NI_INVALID_STATE;
//}
//
//void NetClientSessionTestTransport::shutdown()
//{
//    ++m_counters->m_stopped;
//}
//
//NetClientSessionTestFactory::NetClientSessionTestFactory(NetClientSessionTestCounters& counters) :
//m_counters(&counters)
//{
//
//}
//
//NetGatewayTransport* NetClientSessionTestFactory::create()
//{
//    ++m_counters->m_created;
//    if (m_counters->m_factoryMode == 1)
//    {
//        return nullptr;
//    }
//    if (m_counters->m_factoryMode == 2)
//    {
//        throw 1;
//    }
//    return new NetClientSessionTestTransport(*m_counters);
//}
//
//NetClientSessionTestListener::NetClientSessionTestListener() :
//m_other(0),
//m_nestedSession(nullptr),
//m_nestedRequest(0),
//m_nestedExecuting(false),
//m_nestedCancelOk(false),
//m_reenterClosed(false),
//m_throwClosed(false),
//m_throwResult(false)
//{
//
//}
//
//void NetClientSessionTestListener::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
//{
//    (void)generation;
//    (void)maxRequests;
//    (void)maxChunk;
//    ++m_other;
//}
//
//void NetClientSessionTestListener::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product,
//    uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
//{
//    (void)generation;
//    (void)requestId;
//    (void)product;
//    (void)type;
//    (void)status;
//    (void)bytes;
//    (void)size;
//    ++m_other;
//    m_resultIds.push_back(requestId);
//    m_resultStatus.push_back(status);
//    m_events.push_back(requestId);
//    if (m_throwResult)
//    {
//        throw 1;
//    }
//    if (m_nestedSession != nullptr && m_nestedRequest != 0)
//    {
//        const uint64_t nested = m_nestedRequest;
//        m_nestedRequest = 0;
//        m_nestedCancelOk = m_nestedSession->cancel(nested) == NI_OK;
//        m_nestedExecuting = m_nestedSession->executing();
//    }
//}
//
//bool NetClientSessionTestListener::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
//{
//    (void)generation;
//    (void)frame;
//    ++m_other;
//    return false;
//}
//
//void NetClientSessionTestListener::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
//{
//    (void)detail;
//    m_events.push_back(UINT64_MAX);
//    m_generations.push_back(generation);
//    m_reasons.push_back(reason);
//    if (m_reenterClosed && m_nestedSession != nullptr)
//    {
//        m_nestedExecuting = m_nestedSession->executing();
//        m_nestedSession->close(NI_TIMEOUT);
//        m_nestedSession->shutdown();
//        m_nestedSession->poll();
//        m_nestedCancelOk = m_nestedSession->bindGateway(nullptr) == NI_INVALID_STATE;
//    }
//    if (m_throwClosed)
//    {
//        throw 1;
//    }
//}
//
//int32_t NetClientSessionTestListener::entropy(void* context, uint8_t* bytes, uint32_t size)
//{
//    (void)context;
//    std::memset(bytes, 0x41, size);
//    return 0;
//}
//
//NetClientSessionTestDiscovery::NetClientSessionTestDiscovery() :
//m_available(true),
//m_invalidPoint(0),
//m_throw(false),
//m_drain(false),
//m_shutdownStarted(false),
//m_remaining(0),
//m_fault(0)
//{
//
//}
//
//int32_t NetClientSessionTestDiscovery::start(const std::vector<NetGatewayAddress>& seeds, const std::string& cachePath,
//    const NetInterfaceTlsOptions& tls)
//{
//    (void)seeds;
//    (void)cachePath;
//    (void)tls;
//    m_shutdownStarted = false;
//    m_remaining = m_drain ? 2 : 0;
//    if ((m_fault & 1) != 0)
//    {
//        m_fault &= ~1U;
//        throw 1;
//    }
//    return (m_fault & 32) != 0 ? NI_INVALID_ARGUMENT : NI_OK;
//}
//
//bool NetClientSessionTestDiscovery::poll()
//{
//    if ((m_fault & 4) != 0)
//    {
//        m_fault &= ~4U;
//        throw 1;
//    }
//    if (m_shutdownStarted && m_remaining != 0)
//    {
//        --m_remaining;
//    }
//    return m_remaining != 0;
//}
//
//void NetClientSessionTestDiscovery::shutdown()
//{
//    m_shutdownStarted = true;
//    if ((m_fault & 2) != 0)
//    {
//        m_fault &= ~2U;
//        throw 1;
//    }
//}
//
//bool NetClientSessionTestDiscovery::stopped() const
//{
//    if ((m_fault & 16) != 0)
//    {
//        throw 1;
//    }
//    return m_remaining == 0;
//}
//
//bool NetClientSessionTestDiscovery::choose(NetClientDiscoveryPoint& point)
//{
//    if ((m_fault & 8) != 0)
//    {
//        m_fault &= ~8U;
//        throw 1;
//    }
//    if (m_invalidPoint == 1)
//    {
//        point.m_structSize = 0;
//    }
//    else if (m_invalidPoint == 2)
//    {
//        point.m_version = 2;
//    }
//    else if (m_invalidPoint == 3)
//    {
//        std::memset(point.m_address, '1', sizeof(point.m_address));
//    }
//    else if (m_invalidPoint == 4)
//    {
//        point.m_port = 0;
//    }
//    return m_available;
//}
//
///** 与真实会话相同的单调时间，仅供已验证帧的握手Mock
//@return 单调毫秒
//*/
//static uint64_t RecoveryTestNow()
//{
//    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
//        std::chrono::steady_clock::now().time_since_epoch()).count());
//}
//
//NetClientSessionTestChannel::NetClientSessionTestChannel(const NetInterfaceHandshakeOptions& options) :
//m_fault(0),
//m_sendCount(0),
//m_observed(nullptr),
//m_observedSize(0),
//m_handshake(new NetInterfaceHandshake(options, RecoveryTestNow())),
//m_listener(nullptr),
//m_announced(false),
//m_closing(false),
//m_reason(0),
//m_queued(false),
//m_queuedFrame()
//{
//
//}
//
//NetClientSessionTestChannel::~NetClientSessionTestChannel()
//{
//    shutdown();
//}
//
//int32_t NetClientSessionTestChannel::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
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
//    if ((m_fault & 256) != 0)
//    {
//        throw 1;
//    }
//    peer = (m_fault & 128) != 0 ? 0 : 1;
//    return (m_fault & 512) != 0 ? NI_NOT_FOUND : NI_OK;
//}
//
//bool NetClientSessionTestChannel::poll()
//{
//    if ((m_fault & 64) != 0)
//    {
//        m_fault &= ~64U;
//        throw 1;
//    }
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
//    if (m_queued && !m_closing && m_listener->onFrame(1, m_queuedFrame))
//    {
//        m_queued = false;
//        m_queuedBody.clear();
//    }
//    return true;
//}
//
//int32_t NetClientSessionTestChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
//{
//    if (peer != 1 || m_listener == nullptr || m_closing)
//    {
//        return NI_INVALID_STATE;
//    }
//    ++m_sendCount;
//    if (frame.m_kind == NI_REQUEST)
//    {
//        m_observed = frame.m_payload;
//        m_observedSize = frame.m_payloadSize;
//        if ((m_fault & 1) != 0)
//        {
//            throw 1;
//        }
//        return (m_fault & 1024) != 0 ? NI_LIMIT_EXCEEDED : NI_OK;
//    }
//    if (frame.m_kind == NI_CANCEL || frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK)
//    {
//        if ((m_fault & (frame.m_kind == NI_CANCEL ? 2U : 4U)) != 0)
//        {
//            throw 1;
//        }
//        return NI_OK;
//    }
//    const int32_t accepted = m_handshake->accept(frame, RecoveryTestNow());
//    return accepted < 0 ? accepted : NI_OK;
//}
//
//void NetClientSessionTestChannel::close(uint64_t peer, int32_t reason)
//{
//    if (peer == 1 && !m_closing)
//    {
//        m_closing = true;
//        m_reason = reason;
//    }
//    if ((m_fault & 32) != 0)
//    {
//        throw 1;
//    }
//}
//
//void NetClientSessionTestChannel::shutdown()
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
//int32_t NetClientSessionTestChannel::configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated)
//{
//    (void)peer;
//    (void)types;
//    (void)count;
//    (void)negotiated;
//    if ((m_fault & 8) != 0)
//    {
//        throw 1;
//    }
//    return NI_OK;
//}
//
//int32_t NetClientSessionTestChannel::outputState(uint64_t peer) const
//{
//    (void)peer;
//    if ((m_fault & 16) != 0)
//    {
//        throw 1;
//    }
//    return NI_OK;
//}
//
//void NetClientSessionTestChannel::queueFrame(const NetInterfaceFrame& frame)
//{
//    m_queuedBody.clear();
//    if (frame.m_payloadSize != 0)
//    {
//        m_queuedBody.assign(frame.m_payload, frame.m_payload + frame.m_payloadSize);
//    }
//    m_queuedFrame = frame;
//    m_queuedFrame.m_payload = m_queuedBody.empty() ? nullptr : m_queuedBody.data();
//    m_queued = true;
//}
//
//void NetClientSessionTestDiscovery::failed(const NetClientDiscoveryPoint& point)
//{
//    (void)point;
//    if (m_throw)
//    {
//        throw 1;
//    }
//}
//
//#if defined(_WIN32) && defined(_DEBUG)
//// 只在独立单元期间启用，释放钩子在实际内存释放前观察明文擦除。
//static NetClientSessionTestChannel* g_protocolObserved = nullptr;
//static uint32_t g_protocolWiped = 0;
//static uint32_t g_protocolDirty = 0;
//static bool g_protocolFailAllocation = false;
//
///** 检查请求明文释放，或者只拒绝下一次普通块分配
//@param [in] operation CRT操作
//@param [in] data 释放时仍有效的原始块
//@param [in] size 未使用
//@param [in] block CRT块类型
//@param [in] request 未使用
//@param [in] file 未使用
//@param [in] line 未使用
//@return 1允许，受控的一次分配为0
//*/
//static int ProtocolMemoryHook(int operation, void* data, size_t size, int block, long request,
//    const unsigned char* file, int line)
//{
//    (void)size;
//    (void)request;
//    (void)file;
//    (void)line;
//    if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK && g_protocolFailAllocation)
//    {
//        g_protocolFailAllocation = false;
//        return 0;
//    }
//    if (operation == _HOOK_FREE && g_protocolObserved != nullptr && data == g_protocolObserved->m_observed)
//    {
//        const uint8_t* bytes = static_cast<const uint8_t*>(data);
//        for (uint32_t index = 0; index < g_protocolObserved->m_observedSize; ++index)
//        {
//            if (bytes[index] != 0)
//            {
//                ++g_protocolDirty;
//            }
//        }
//        ++g_protocolWiped;
//        g_protocolObserved->m_observed = nullptr;
//    }
//    return 1;
//}
//#endif
//
//int main()
//{
//    uint32_t checks = 0;
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState before;
//    _CrtMemCheckpoint(&before);
//#endif
//#define P_CHECK(condition) { ++checks; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
//    {
//        NetInterfaceTlsOptions tls = {};
//        tls.handshakeTimeoutMs = 1000;
//        tls.entropy = NetClientSessionTestListener::entropy;
//        NetInterfaceHandshakeOptions hello = {};
//        hello.m_platform = 1;
//        hello.m_capabilities = 7;
//        hello.m_maxRequests = 128;
//        hello.m_maxChunk = 4096;
//        hello.m_timeoutMs = 1000;
//        std::memset(hello.m_device, 0x31, sizeof(hello.m_device));
//        NetInterfaceHandshakeOptions server = hello;
//        server.m_server = 1;
//        server.m_platform = 0;
//        std::memset(server.m_device, 0, sizeof(server.m_device));
//        std::memset(server.m_nonce, 0x73, sizeof(server.m_nonce));
//        for (uint32_t mode = 0; mode < 8; ++mode)
//        {
//            NetClientSessionTestChannel channel(server);
//            NetClientSessionTestListener listener;
//            NetSession session;
//            P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, hello, &listener) == NI_OK);
//            for (uint32_t index = 0; index < 8 && !session.ready(); ++index)
//            {
//                session.poll();
//            }
//            P_CHECK(session.ready() && listener.m_other == 1);
//            uint64_t ids[3] = {};
//            for (uint32_t index = 0; index < 3; ++index)
//            {
//                P_CHECK(session.beginRequest(101, 11 + index, 1000, ids[index]) == NI_OK && ids[index] == index + 1);
//            }
//            channel.m_fault = mode == 0 ? 1 : mode == 1 ? 2 : mode == 2 ? 4 : mode == 3 ? 16 :
//                mode == 4 ? 32 : mode == 5 ? 64 : mode == 6 ? 1024 : 0;
//            if (mode == 0)
//            {
//                const uint8_t body[] = { 0x71, 0x72, 0x73 };
//                P_CHECK(session.sendRequest(ids[0], body, sizeof(body)) == NI_INVALID_STATE);
//                P_CHECK(session.cancel(ids[0]) == NI_OK);
//            }
//            else if (mode == 1)
//            {
//                P_CHECK(session.sendRequest(ids[0], nullptr, 0) == NI_OK);
//                P_CHECK(session.cancel(ids[0]) == NI_OK && !session.ready());
//                P_CHECK(listener.m_resultIds.size() == 1 && listener.m_resultStatus[0] == NI_RPC_CANCELLED);
//            }
//            else if (mode == 2)
//            {
//                NetInterfaceFrame frame = {};
//                frame.m_kind = NI_DATA;
//                frame.m_productId = 101;
//                frame.m_requestId = 1;
//                P_CHECK(session.sendData(frame) == NI_INVALID_STATE);
//            }
//            else if (mode == 3)
//            {
//                P_CHECK(session.outputState() == NI_INVALID_STATE);
//            }
//            else if (mode == 4)
//            {
//                session.close(NI_CANCELLED);
//                P_CHECK(!session.ready() && listener.m_generations.size() == 1);
//            }
//            else if (mode == 5)
//            {
//                session.poll();
//                P_CHECK(!session.ready());
//            }
//            else if (mode == 6)
//            {
//                for (uint32_t index = 0; index < 128; ++index)
//                {
//                    P_CHECK(session.sendRequest(ids[0], nullptr, 0) == NI_LIMIT_EXCEEDED && listener.m_resultIds.empty());
//                }
//                channel.m_fault = 0;
//                P_CHECK(session.sendRequest(ids[0], nullptr, 0) == NI_OK);
//                P_CHECK(session.sendRequest(ids[0], nullptr, 0) == NI_INVALID_STATE);
//                P_CHECK(session.cancel(ids[0]) == NI_OK);
//            }
//            else
//            {
//                listener.m_throwResult = true;
//                P_CHECK(session.cancel(ids[0]) == NI_OK && !session.ready());
//            }
//            session.shutdown();
//            P_CHECK(listener.m_resultIds.size() == 3 && listener.m_resultStatus.size() == 3 &&
//                listener.m_generations.size() == 1 && listener.m_events.size() == 4 && listener.m_events.back() == UINT64_MAX);
//            std::sort(listener.m_resultIds.begin(), listener.m_resultIds.end());
//            P_CHECK(listener.m_resultIds[0] == 1 && listener.m_resultIds[1] == 2 && listener.m_resultIds[2] == 3);
//            for (uint32_t index = 0; index < 3; ++index)
//            {
//                P_CHECK(session.cancel(ids[index]) == NI_NOT_FOUND);
//            }
//            P_CHECK(!session.poll());
//            session.shutdown();
//            P_CHECK(listener.m_generations.size() == 1 && listener.m_events.size() == 4);
//        }
//        const uint32_t connectFaults[] = { 128, 256, 512 };
//        for (uint32_t index = 0; index < 3; ++index)
//        {
//            NetClientSessionTestChannel channel(server);
//            NetClientSessionTestListener listener;
//            NetSession session;
//            channel.m_fault = connectFaults[index];
//            P_CHECK(session.bindChannel(&channel) == NI_OK);
//            P_CHECK(session.connect("mock", 1, tls, hello, &listener) ==
//                (index == 0 ? NI_PROTOCOL_ERROR : index == 1 ? NI_NO_MEMORY : NI_NOT_FOUND));
//            session.shutdown();
//            P_CHECK(!session.poll() && listener.m_generations.empty() && listener.m_resultIds.empty() && listener.m_other == 0);
//        }
//        for (uint32_t mode = 0; mode < 4; ++mode)
//        {
//            NetClientSessionTestChannel channel(server);
//            NetClientSessionTestListener listener;
//            NetSession session;
//            P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, hello, &listener) == NI_OK);
//            for (uint32_t index = 0; index < 8 && !session.ready(); ++index)
//            {
//                session.poll();
//            }
//            P_CHECK(session.ready());
//            uint64_t id = 0;
//            P_CHECK(session.beginRequest(101, 11, 1000, id) == NI_OK);
//            if (mode != 3)
//            {
//                P_CHECK(session.sendRequest(id, nullptr, 0) == NI_OK);
//            }
//            uint8_t payload[18] = { 'N', 'R', 'P', '2' };
//            payload[16] = 0x5a;
//            payload[17] = 0xa5;
//            NetInterfaceFrame frame = {};
//            frame.m_kind = NI_RESPONSE;
//            frame.m_productId = mode == 1 ? 102 : 101;
//            frame.m_messageType = mode == 2 ? 12 : 11;
//            frame.m_requestId = id;
//            frame.m_payload = payload;
//            frame.m_payloadSize = sizeof(payload);
//            channel.queueFrame(frame);
//            session.poll();
//            if (mode == 0)
//            {
//                P_CHECK(listener.m_resultIds.size() == 1 && listener.m_resultStatus[0] == NI_RPC_SUCCESS && session.ready());
//                channel.queueFrame(frame);
//                session.poll();
//                P_CHECK(listener.m_resultIds.size() == 1 && session.ready());
//            }
//            session.shutdown();
//            P_CHECK(listener.m_resultIds.size() == 1 && listener.m_resultIds[0] == id &&
//                listener.m_resultStatus[0] == (mode == 0 ? NI_RPC_SUCCESS : NI_RPC_UNAVAILABLE));
//            P_CHECK(listener.m_generations.size() == 1 && listener.m_events.back() == UINT64_MAX);
//        }
//        {
//            NetClientSessionTestChannel channel(server);
//            NetClientSessionTestListener listener;
//            NetSession session;
//            P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, hello, &listener) == NI_OK);
//            for (uint32_t index = 0; index < 8 && !session.ready(); ++index)
//            {
//                session.poll();
//            }
//            for (uint32_t index = 0; index < 128; ++index)
//            {
//                uint64_t id = 0;
//                P_CHECK(session.beginRequest(101, 11, 60000, id) == NI_OK && id == index + 1);
//            }
//            uint64_t id = 99;
//            P_CHECK(session.beginRequest(101, 11, 60000, id) == NI_LIMIT_EXCEEDED && id == 0);
//            P_CHECK(session.cancel(1) == NI_OK);
//            P_CHECK(session.beginRequestAt(129, 101, 11, RecoveryTestNow(), 60000) == NI_PROTOCOL_ERROR);
//            P_CHECK(session.beginRequest(101, 11, 60000, id) == NI_OK && id == 130);
//            session.shutdown();
//            P_CHECK(listener.m_resultIds.size() == 129 && listener.m_generations.size() == 1);
//            for (uint32_t index = 0; index < listener.m_resultStatus.size(); ++index)
//            {
//                P_CHECK(listener.m_resultStatus[index] == NI_RPC_CANCELLED);
//            }
//        }
//
//        for (uint32_t mode = 0; mode < 2; ++mode)
//        {
//            NetClientSessionTestChannel channel(server);
//            NetClientSessionTestListener listener;
//            NetSession session;
//            P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, hello, &listener) == NI_OK);
//            for (uint32_t index = 0; index < 8 && !session.ready(); ++index)
//            {
//                session.poll();
//            }
//            uint64_t id = 0;
//            P_CHECK(session.beginRequest(101, 11, 10, id) == NI_OK && session.sendRequest(id, nullptr, 0) == NI_OK);
//            uint8_t payload[16] = { 'N', 'R', 'P', '2' };
//            NetInterfaceFrame response = {};
//            response.m_kind = NI_RESPONSE;
//            response.m_productId = 101;
//            response.m_messageType = 11;
//            response.m_requestId = id;
//            response.m_payload = payload;
//            response.m_payloadSize = sizeof(payload);
//            if (mode == 0)
//            {
//                channel.queueFrame(response);
//            }
//            else
//            {
//                channel.m_fault = 2;
//            }
//            const uint64_t afterDeadline = RecoveryTestNow() + 11;
//            while (RecoveryTestNow() < afterDeadline)
//            {
//                // 测试固定11毫秒，不创建生产线程，不替代真实30秒恢复期限用例。
//            }
//            session.poll();
//            P_CHECK(listener.m_resultIds.size() == 1 && listener.m_resultIds[0] == id &&
//                listener.m_resultStatus[0] == NI_RPC_TIMEOUT && session.ready() == (mode == 0));
//            session.shutdown();
//            P_CHECK(listener.m_events.size() == 2 && listener.m_events.back() == UINT64_MAX);
//        }
//
//
//        {
//            NetInterfaceHandshakeOptions compressedHello = hello;
//            NetInterfaceHandshakeOptions compressedServer = server;
//            compressedHello.m_capabilities = compressedServer.m_capabilities = 15;
//            NetClientSessionTestChannel channel(compressedServer);
//            NetClientSessionTestListener listener;
//            NetSession session;
//            channel.m_fault = 8;
//            P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, compressedHello, &listener) == NI_OK);
//            for (uint32_t index = 0; index < 8; ++index)
//            {
//                session.poll();
//            }
//            session.shutdown();
//            P_CHECK(!session.ready() && listener.m_generations.size() == 1 && listener.m_other == 0);
//        }
//        {
//            NetClientSessionTestChannel channel(server);
//            NetClientSessionTestListener listener;
//            NetSession session;
//            P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, hello, &listener) == NI_OK);
//            for (uint32_t index = 0; index < 8 && !session.ready(); ++index)
//            {
//                session.poll();
//            }
//            const uint32_t count = channel.m_sendCount;
//            NetInterfaceFrame frame = {};
//            frame.m_kind = NI_DATA;
//            frame.m_productId = 101;
//            frame.m_requestId = 1;
//            frame.m_payloadSize = 1;
//            P_CHECK(session.sendData(frame) == NI_INVALID_ARGUMENT && channel.m_sendCount == count);
//            session.shutdown();
//        }
//#ifdef STATIC_LIB
//        {
//            NetSessionState state;
//            state.resetRequests(128);
//            std::map<uint64_t, uint64_t> reference;
//            uint32_t random = 0x89173;
//            uint64_t highest = 0;
//            for (uint32_t step = 0; step < 10000; ++step)
//            {
//                random = random * 1664525U + 1013904223U;
//                if ((random & 3) != 0 || reference.empty())
//                {
//                    const uint64_t deadline = 1000 + (random >> 8) % 5000;
//                    uint64_t id = 77;
//                    const int32_t result = state.reserve(0, 101, step, deadline, id);
//                    ++highest;
//                    if (reference.size() == 128)
//                    {
//                        P_CHECK(result == NI_LIMIT_EXCEEDED && id == 0);
//                    }
//                    else
//                    {
//                        P_CHECK(result == NI_OK && id == highest);
//                        reference[id] = deadline;
//                    }
//                }
//                else
//                {
//                    auto found = reference.begin();
//                    std::advance(found, (random >> 8) % reference.size());
//                    NetSessionPending pending = {};
//                    P_CHECK(state.take(found->first, pending) && pending.m_request.m_original == found->first &&
//                        pending.m_request.m_deadline == found->second && !pending.m_sent);
//                    reference.erase(found);
//                }
//                P_CHECK(state.m_pending.size() == reference.size() && state.m_expiryCount == reference.size());
//                uint64_t earliest = 0;
//                uint64_t deadline = UINT64_MAX;
//                for (auto found = reference.begin(); found != reference.end(); ++found)
//                {
//                    if (found->second < deadline)
//                    {
//                        earliest = found->first;
//                        deadline = found->second;
//                    }
//                }
//                P_CHECK(state.expired(UINT64_MAX) == earliest);
//                if (earliest != 0)
//                {
//                    P_CHECK(state.expired(deadline - 1) == 0 && state.expired(deadline) == earliest);
//                }
//                for (size_t index = 0; index < state.m_expiryCount; ++index)
//                {
//                    P_CHECK(state.m_expiry[index]->m_heapIndex == index &&
//                        state.m_pending.find(state.m_expiry[index]->m_request.m_original) != state.m_pending.end());
//                }
//            }
//            while (!reference.empty())
//            {
//                const uint64_t id = state.expired(UINT64_MAX);
//                P_CHECK(id != 0 && reference.find(id) != reference.end());
//                NetSessionPending pending = {};
//                P_CHECK(state.take(id, pending));
//                reference.erase(id);
//            }
//            P_CHECK(state.expired(UINT64_MAX) == 0 && state.m_pending.empty());
//            state.resetRequests(1);
//            uint64_t id = 1;
//            P_CHECK(state.reserve(0, 0, 0, 1, id) == NI_INVALID_ARGUMENT && id == 0 && state.m_highestOriginal == 0);
//            P_CHECK(state.reserve(UINT64_MAX, 101, 0, 1, id) == NI_OK && id == UINT64_MAX);
//            NetSessionPending pending = {};
//            P_CHECK(state.take(id, pending) && state.reserve(0, 101, 0, 1, id) == NI_LIMIT_EXCEEDED && id == 0);
//            P_CHECK(state.reserve(UINT64_MAX, 101, 0, 1, id) == NI_PROTOCOL_ERROR && id == 0);
//            state.resetRequests(0);
//            P_CHECK(state.reserve(0, 101, 0, 1, id) == NI_INVALID_ARGUMENT && state.m_expiryCount == 0);
//        }
//#endif
//
//
//#if defined(_WIN32) && defined(_DEBUG)
//        for (uint32_t mode = 0; mode < 6; ++mode)
//        {
//            NetClientSessionTestChannel channel(server);
//            NetClientSessionTestListener listener;
//            const uint32_t wiped = g_protocolWiped;
//            const uint32_t dirty = g_protocolDirty;
//            std::vector<uint8_t> body(mode < 3 ? 80 : 8193, 0x5a);
//            g_protocolObserved = &channel;
//            const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(ProtocolMemoryHook);
//            {
//                NetSession session;
//                P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, hello, &listener) == NI_OK);
//                for (uint32_t index = 0; index < 8 && !session.ready(); ++index)
//                {
//                    session.poll();
//                }
//                uint64_t id = 0;
//                P_CHECK(session.beginRequest(101, 11, 1000, id) == NI_OK);
//                channel.m_fault = mode % 3 == 0 ? 0 : mode % 3 == 1 ? 1 : 1024;
//                const int32_t sent = session.sendRequest(id, body.data(), static_cast<uint32_t>(body.size()));
//                P_CHECK(sent == (mode % 3 == 0 ? NI_OK : mode % 3 == 1 ? NI_INVALID_STATE : NI_LIMIT_EXCEEDED));
//                session.shutdown();
//            }
//            _CrtSetAllocHook(previous);
//            g_protocolObserved = nullptr;
//            P_CHECK(g_protocolWiped == wiped + 1 && g_protocolDirty == dirty && listener.m_resultIds.size() == 1);
//        }
//        {
//            NetClientSessionTestChannel channel(server);
//            NetClientSessionTestListener listener;
//            NetSession session;
//            P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, hello, &listener) == NI_OK);
//            for (uint32_t index = 0; index < 8 && !session.ready(); ++index)
//            {
//                session.poll();
//            }
//            uint64_t id = 99;
//            g_protocolFailAllocation = true;
//            _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(ProtocolMemoryHook);
//            const int32_t failed = session.beginRequest(101, 11, 1000, id);
//            _CrtSetAllocHook(previous);
//            P_CHECK(failed == NI_NO_MEMORY && id == 0 && !g_protocolFailAllocation && listener.m_resultIds.empty());
//            P_CHECK(session.beginRequest(101, 11, 1000, id) == NI_OK && id == 2);
//            std::vector<uint8_t> body(32768, 0x6a);
//            g_protocolFailAllocation = true;
//            previous = _CrtSetAllocHook(ProtocolMemoryHook);
//            const int32_t rejected = session.sendRequest(id, body.data(), static_cast<uint32_t>(body.size()));
//            _CrtSetAllocHook(previous);
//            P_CHECK(rejected == NI_NO_MEMORY && !g_protocolFailAllocation && listener.m_resultIds.empty());
//            P_CHECK(session.cancel(id) == NI_OK && listener.m_resultIds.size() == 1 && listener.m_resultIds[0] == id);
//            session.shutdown();
//        }
//#endif
//
//    }
//#if defined(_WIN32) && defined(_DEBUG)
//    _CrtMemState after;
//    _CrtMemCheckpoint(&after);
//    P_CHECK(after.lCounts[_NORMAL_BLOCK] == before.lCounts[_NORMAL_BLOCK] &&
//        after.lSizes[_NORMAL_BLOCK] == before.lSizes[_NORMAL_BLOCK]);
//#endif
//    std::printf("Protocol session checks=%u PASS\n", checks);
//    return 0;
//#undef P_CHECK
//}
// NET_PROTOCOL_SESSION_UNIT_END