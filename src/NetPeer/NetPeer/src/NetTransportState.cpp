#include "NetPeerMacro.h"
#include "NetTransportState.h"
#include "NetHostSocketState.h"
#include "NetTransportEntropy.h"
#include <algorithm>

NetTransportPeer::NetTransportPeer() :
m_pendingOffset(0), m_started(0), m_connected(false), m_ready(false),
m_closing(false), m_tcpClosed(false), m_reason(0), m_detail(0)
{

}

NetDirectTransportConfig::NetDirectTransportConfig()
{
    LockFreeHashMapPoolConfig& config = m_peers;
    config.initialCapacity = 128;
    config.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetTransportPeer>>);
    config.directoryCapacity = 128;
    config.lookupCacheCapacity = 0;
}

NetTransportState::NetTransportState(const NetDirectTransportConfig& config) :
m_connectionPools(new NetConnectionPoolConfig(config.m_connection)), m_ciphertext(config.m_ciphertext),
m_payloadLimit(config.m_payloadLimit), m_channel(nullptr, config.m_host),
m_peerPool(new NetObjectPool<NetTransportPeer>(config.m_peers)), m_listener(nullptr), m_options(), m_started(false), m_stopping(false),
m_polling(false), m_now(now()), m_frameBudget(config.m_frameBudget), m_outputBudget(config.m_outputBudget), m_drainTimeoutMs(config.m_drainTimeoutMs)
{

}

NetTransportState::~NetTransportState()
{
    shutdown();
    if (!m_privateKey.empty())
    {
        NetInterface::wipe(m_privateKey.data(), static_cast<uint32_t>(m_privateKey.size()));
    }
}

uint64_t NetTransportState::now()
{
    return NetInterface::monotonicMilliseconds();
}

int32_t NetTransportState::configure(const NetInterfaceTlsOptions& options,
    NetTransportListener* listener, bool server)
{
    if (m_payloadLimit == 0 || m_payloadLimit > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT || m_outputBudget == 0 || m_frameBudget == 0 || m_drainTimeoutMs == 0 || m_started || m_stopping || listener == nullptr || options.server != (server ? 1 : 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    m_options = options;
    if (m_options.entropy == nullptr)
    {
        m_options.entropy = &NetTransportEntropy;
        m_options.entropyContext = nullptr;
    }
    try
    {
        // 先通过同源TLS验证指针/长度/证书，不能先按不可信长度分配。
        NetInterfaceConnection validation(m_options, m_payloadLimit, *m_connectionPools, m_ciphertext);
        if (validation.status() < 0)
        {
            return validation.status();
        }
        if (server)
        {
            m_certificate.assign(options.certificate, options.certificate + options.certificateSize);
            m_privateKey.assign(options.privateKey, options.privateKey + options.privateKeySize);
            m_options.certificate = &m_certificate[0];
            m_options.privateKey = &m_privateKey[0];
            m_options.authority = nullptr;
            m_options.authoritySize = 0;
            m_options.hostname = nullptr;
        }
        else
        {
            m_authority.assign(options.authority, options.authority + options.authoritySize);
            m_hostname = options.hostname;
            m_options.authority = &m_authority[0];
            m_options.hostname = m_hostname.c_str();
            m_options.certificate = nullptr;
            m_options.certificateSize = 0;
            m_options.privateKey = nullptr;
            m_options.privateKeySize = 0;
        }
        m_started = true;
        m_listener = listener;
        return 0;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

NetInterfaceConnection* NetTransportState::writableConnection(uint64_t id) const
{
    const auto found = m_peers.find(id);
    if (m_stopping || found == m_peers.end() || found->second->m_closing || found->second->m_tcpClosed ||
        NetHostSocketState::pending(m_channel, id, NET_HOST_CLOSED))
    {
        return nullptr;
    }
    return found->second->m_connection.get();
}

void NetTransportState::onConnected(uint64_t id)
{
    if (m_stopping)
    {
        m_channel.close(id);
        return;
    }
    auto entry = m_peers.find(id);
    if (entry == m_peers.end())
    {
        if (m_options.server == 0)
        {
            m_channel.close(id, NI_INVALID_STATE);
            return;
        }
        NetObjectReference<NetTransportPeer> peer;
        const int32_t allocated = m_peerPool->create(peer);
        if (allocated != NI_OK) { m_channel.close(id, allocated); return; }
        entry = m_peers.insert(std::make_pair(id, std::move(peer))).first;
    }
    entry->second->m_connected = true;
    entry->second->m_started = m_now;
    // 暂停读取，交给外层 poll 分配 TLS，限制一次宿主事件分发触发的握手工作。
    const int32_t result = m_channel.pauseRead(id, true);
    if (result < 0)
    {
        close(id, result);
    }
}

void NetTransportState::onData(uint64_t id, const uint8_t* data, uint32_t size)
{
    auto entry = m_peers.find(id);
    if (entry == m_peers.end() || entry->second->m_closing)
    {
        return;
    }
    NetTransportPeer& peer = *entry->second;
    if (!peer.m_connection || !peer.m_pending.empty() || size > 16384)
    {
        close(id, NI_INVALID_STATE);
        return;
    }
    uint32_t used = 0;
    const int32_t result = peer.m_connection->feed(data, size, used);
    if (result < 0)
    {
        close(id, result);
        return;
    }
    if (used != size)
    {
        peer.m_pending.assign(data + used, data + size);
        peer.m_pendingOffset = 0;
        const int32_t paused = m_channel.pauseRead(id, true);
        if (paused < 0)
        {
            close(id, paused);
        }
    }
}

void NetTransportState::onWritable(uint64_t peer)
{
    (void)peer;
}

void NetTransportState::onClosed(uint64_t id, int32_t reason, int32_t detail)
{
    auto entry = m_peers.find(id);
    if (entry == m_peers.end())
    {
        return;
    }
    NetTransportPeer& peer = *entry->second;
    peer.m_tcpClosed = true;
    peer.m_started = m_now;
    if (!peer.m_connection && !peer.m_closing)
    {
        // TLS 尚未启动，宿主返回的 EOF 无法证明对端正常关闭。
        peer.m_closing = true;
        peer.m_reason = reason == NI_OK ? NET_INTERFACE_TLS_TRUNCATED : reason;
        peer.m_detail = detail;
    }
    // 已建连接在pump中处理密文尾部和认证关闭，不能把裸TCP EOF算成功。
}

void NetTransportState::close(uint64_t id, int32_t reason)
{
    auto entry = m_peers.find(id);
    if (entry == m_peers.end() || entry->second->m_closing)
    {
        return;
    }
    NetTransportPeer& peer = *entry->second;
    peer.m_closing = true;
    peer.m_reason = reason;
    peer.m_detail = peer.m_connection ? peer.m_connection->detail() : 0;
    m_channel.close(id, reason);
}

void NetTransportState::pump(uint64_t id, NetTransportPeer& peer)
{
    // 终态已经入队时先按宿主顺序派发，不再新建 TLS 或尝试写入关闭的连接。
    if (peer.m_closing || NetHostSocketState::pending(m_channel, id, NET_HOST_CLOSED))
    {
        return;
    }
    if (!peer.m_connected)
    {
        const uint32_t timeout = m_options.effectiveHandshakeTimeout();
        NetExecution::deadline(timeout, m_now - peer.m_started);
        if (m_now - peer.m_started >= timeout)
        {
            close(id, NET_INTERFACE_TLS_TIMEOUT);
        }
        return;
    }
    if (peer.m_tcpClosed)
    {
        NetExecution::deadline(m_drainTimeoutMs, m_now - peer.m_started);
        if (m_now - peer.m_started >= m_drainTimeoutMs)
        {
            close(id, NET_INTERFACE_TLS_TRUNCATED);
            return;
        }
    }
    if (!peer.m_connection)
    {
        peer.m_connection.reset(new NetInterfaceConnection(m_options, m_payloadLimit, *m_connectionPools, m_ciphertext));
        if (peer.m_connection->status() < 0) { close(id, peer.m_connection->status()); return; }
    }
    bool applicationBlocked = false;
    uint32_t outputBudget = m_outputBudget;
    for (uint32_t step = 0; step < m_frameBudget && !peer.m_closing && !m_stopping; ++step)
    {
        bool moved = false;
        if (!peer.m_pending.empty())
        {
            uint32_t used = 0;
            const int32_t fed = peer.m_connection->feed(&peer.m_pending[peer.m_pendingOffset],
                static_cast<uint32_t>(peer.m_pending.size()) - peer.m_pendingOffset, used);
            if (fed < 0)
            {
                close(id, fed);
                break;
            }
            peer.m_pendingOffset += used;
            moved = used != 0;
            if (peer.m_pendingOffset == peer.m_pending.size())
            {
                peer.m_pending.clear();
                peer.m_pendingOffset = 0;
            }
        }
        if (peer.m_tcpClosed && peer.m_pending.empty())
        {
            peer.m_connection->finishInput();
        }
        const int32_t result = peer.m_connection->progress(m_now);
        if (result < 0 || result == NET_INTERFACE_TLS_CLOSED)
        {
            close(id, result < 0 ? result : 0);
            break;
        }
        if (result == NET_INTERFACE_TLS_READY)
        {
            if (!peer.m_ready)
            {
                peer.m_ready = true;
                m_listener->onReady(id);
                moved = true;
            }
            if (peer.m_closing || m_stopping)
            {
                break;
            }
            NetInterfaceFrame frame = {};
            if (peer.m_connection->peek(frame) == NI_READY)
            {
                if (m_listener->onFrame(id, frame))
                {
                    peer.m_connection->consume();
                    moved = true;
                }
                else
                {
                    applicationBlocked = true;
                }
            }
        }
        if (peer.m_closing || m_stopping)
        {
            break;
        }
        const uint8_t* output = nullptr;
        uint32_t size = 0;
        peer.m_connection->peekOutput(output, size);
        size = (std::min)(size, outputBudget);
        if (size != 0 && !peer.m_tcpClosed)
        {
            const int32_t written = m_channel.write(id, output, size);
            if (written == 0)
            {
                peer.m_connection->consumeOutput(size);
                outputBudget -= size;
                moved = true;
            }
            else if (!NetHostSocket::isBackpressure(written) && !NetHostSocketState::pending(m_channel, id, NET_HOST_CLOSED))
            {
                close(id, written);
                break;
            }
        }
        if (applicationBlocked || !moved || outputBudget == 0)
        {
            break;
        }
    }
    // 已接纳的输入自带暂停，交付 onData 后才能按 TLS 状态恢复读取。
    if (!peer.m_closing && !peer.m_tcpClosed && !NetHostSocketState::pending(m_channel, id, NET_HOST_DATA))
    {
        const int32_t paused = m_channel.pauseRead(id, applicationBlocked || !peer.m_pending.empty());
        if (paused < 0)
        {
            close(id, paused);
        }
    }
}

bool NetTransportState::poll()
{
    if (m_polling)
    {
        return !m_peers.empty() || (m_started && !m_stopping);
    }
    m_polling = true;
    m_now = now();
    bool alive = m_channel.poll();
    for (auto entry = m_peers.begin(); entry != m_peers.end(); ++entry)
    {
        try
        {
            pump(entry->first, *entry->second);
        }
        catch (...)
        {
            close(entry->first, NI_INVALID_STATE);
        }
    }
    if (m_stopping)
    {
        m_channel.shutdown();
        alive = false;
    }
    collectClosed();
    m_polling = false;
    return alive || !m_peers.empty();
}

void NetTransportState::collectClosed()
{
    for (auto entry = m_peers.begin(); entry != m_peers.end(); )
    {
        if (!entry->second->m_closing || !entry->second->m_tcpClosed)
        {
            ++entry;
            continue;
        }
        const uint64_t id = entry->first;
        const int32_t reason = entry->second->m_reason;
        const int32_t detail = entry->second->m_detail;
        entry = m_peers.erase(entry);
        try
        {
            m_listener->onClosed(id, reason, detail);
        }
        catch (...)
        {
            // 连接已回收，终态回调异常只影响本次通知。
        }
    }
}

void NetTransportState::shutdown()
{
    m_stopping = true;
    for (auto entry = m_peers.begin(); entry != m_peers.end(); ++entry)
    {
        close(entry->first, 0);
    }
    if (!m_polling)
    {
        m_polling = true;
        m_channel.shutdown();
        collectClosed();
        m_polling = false;
    }
}