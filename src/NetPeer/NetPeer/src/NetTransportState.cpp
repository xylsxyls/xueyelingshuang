#include "NetPeerMacro.h"
#include "NetTransportState.h"
#include "NetTransportEntropy.h"
#include <algorithm>
#include <chrono>
#include <cstring>

NetTransportPeer::NetTransportPeer() :
m_pendingOffset(0), m_started(0), m_connected(false), m_ready(false),
m_closing(false), m_tcpClosed(false), m_reason(0), m_detail(0)
{

}

NetTransportState::NetTransportState() :
m_listener(nullptr), m_options(), m_started(false), m_stopping(false),
m_polling(false), m_now(now()), m_acceptWindow(m_now), m_acceptCount(0)
{

}

NetTransportState::~NetTransportState()
{
    shutdown();
    if (!m_privateKey.empty())
    {
        volatile uint8_t* key = &m_privateKey[0];
        for (size_t index = 0; index < m_privateKey.size(); ++index)
        {
            key[index] = 0;
        }
    }
}

uint64_t NetTransportState::now()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

int32_t NetTransportState::configure(const NetInterfaceTlsOptions& options,
    NetTransportListener* listener, bool server)
{
    if (m_started || m_stopping || listener == nullptr || options.server != (server ? 1 : 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    m_started = true;
    m_options = options;
    if (m_options.entropy == nullptr)
    {
        m_options.entropy = &NetTransportEntropy;
        m_options.entropyContext = nullptr;
    }
    try
    {
        // 先通过同源TLS验证指针/长度/证书，不能先按不可信长度分配。
        NetInterfaceConnection validation(m_options);
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
        m_listener = listener;
        return 0;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
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
        if (m_now - m_acceptWindow >= 1000)
        {
            m_acceptWindow = m_now;
            m_acceptCount = 0;
        }
        if (m_acceptCount >= 16)
        {
            m_channel.close(id, NI_LIMIT_EXCEEDED);
            return;
        }
        ++m_acceptCount;
        std::unique_ptr<NetTransportPeer> peer(new NetTransportPeer);
        entry = m_peers.insert(std::make_pair(id, std::move(peer))).first;
    }
    entry->second->m_connected = true;
    entry->second->m_started = m_now;
    // 暂停到外层poll分配TLS，禁止在同次uv_run中无限创建握手工作。
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
    if (!peer.m_connected || !peer.m_connection)
    {
        if (!peer.m_closing)
        {
            peer.m_closing = true;
            peer.m_reason = reason;
            peer.m_detail = detail;
        }
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
    if (peer.m_closing)
    {
        return;
    }
    if (!peer.m_connected)
    {
        if (m_now - peer.m_started >= m_options.handshakeTimeoutMs)
        {
            close(id, NET_INTERFACE_TLS_TIMEOUT);
        }
        return;
    }
    if (peer.m_tcpClosed && m_now - peer.m_started >= 5000)
    {
        close(id, NET_INTERFACE_TLS_TRUNCATED);
        return;
    }
    if (!peer.m_connection)
    {
        peer.m_connection.reset(new NetInterfaceConnection(m_options));
    }
    bool applicationBlocked = false;
    uint32_t outputBudget = 65536;
    for (uint32_t step = 0; step < 64 && !peer.m_closing && !m_stopping; ++step)
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
            else if (!NetHostSocket::isBackpressure(written))
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
    if (!peer.m_closing && !peer.m_tcpClosed)
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
            // 状态已经删除；终态回调异常不得跨过libuv或导致重复通知。
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