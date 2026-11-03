#include "NetSenderLinkState.h"
#include "NetSenderMacro.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>

/** 空随机回调仅使用系统强随机，不退回普通伪随机
@param [in] context 忽略
@param [out] output 输出
@param [in] size 字节数
@return 0成功，否则失败
*/
static int32_t SenderLinkEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return CSystem::secureRandomBytes(output, size) ? 0 : -1;
}

/** 管理IPC统一单调时间
@return 单调毫秒
*/
static uint64_t SenderLinkNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetSenderLinkPeer::NetSenderLinkPeer(const NetInterfaceLinkOptions& options) : m_link(options), m_connected(false),
    m_announced(false), m_closing(false), m_closed(false), m_blockedSince(0), m_reason(0)
{

}

NetSenderLinkState::NetSenderLinkState(NetSenderLocalInbox& inbox) : m_raw(inbox), m_listener(nullptr), m_options(),
    m_started(false), m_stopping(false), m_polling(false), m_acceptWindow(0), m_acceptCount(0)
{

}

NetSenderLinkState::~NetSenderLinkState()
{
    if (!m_privateKey.empty()) { NetInterface::wipe(m_privateKey.data(), static_cast<uint32_t>(m_privateKey.size())); }
}

int32_t NetSenderLinkState::configure(const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener)
{
    if (m_started || m_stopping || listener == nullptr) { return NI_INVALID_ARGUMENT; }
    m_started = true;
    m_options = options;
    if (m_options.m_tls.entropy == nullptr)
    {
        m_options.m_tls.entropy = SenderLinkEntropy;
        m_options.m_tls.entropyContext = nullptr;
    }
    try
    {
        NetInterfaceLink validation(m_options);
        if (validation.status() < 0) { return validation.status(); }
        if (options.m_name != nullptr) { m_name = options.m_name; m_options.m_name = m_name.c_str(); }
        if (options.m_role == NI_LINK_NODE && options.m_tls.certificateSize != 0)
        {
            m_certificate.assign(options.m_tls.certificate, options.m_tls.certificate + options.m_tls.certificateSize);
            m_options.m_tls.certificate = m_certificate.data();
        }
        else { m_options.m_tls.certificate = nullptr; m_options.m_tls.certificateSize = 0; }
        if (options.m_role != NI_LINK_CLIENT && options.m_tls.privateKeySize != 0)
        {
            m_privateKey.assign(options.m_tls.privateKey, options.m_tls.privateKey + options.m_tls.privateKeySize);
            m_options.m_tls.privateKey = m_privateKey.data();
        }
        if (options.m_claimsSize != 0)
        {
            m_claims.assign(options.m_claims, options.m_claims + options.m_claimsSize);
            m_options.m_claims = m_claims.data();
        }
        m_listener = listener;
        return NI_OK;
    }
    catch (...) { return NI_NO_MEMORY; }
}

void NetSenderLinkState::close(uint64_t id, int32_t reason)
{
    auto found = m_peers.find(id);
    if (found == m_peers.end() || found->second->m_closing) { return; }
    found->second->m_closing = true;
    if (!found->second->m_closed || found->second->m_reason == NI_OK)
    {
        found->second->m_reason = reason;
    }
    m_raw.close(id, reason);
}

void NetSenderLinkState::onRawLocalReady(uint64_t id, int32_t pid)
{
    (void)pid;
    if (m_stopping) { m_raw.close(id); return; }
    try
    {
        auto found = m_peers.find(id);
        if (found == m_peers.end())
        {
            const uint64_t now = SenderLinkNow();
            if (now - m_acceptWindow >= 1000) { m_acceptWindow = now; m_acceptCount = 0; }
            if (m_acceptCount >= 16) { m_raw.close(id, NI_LIMIT_EXCEEDED); return; }
            ++m_acceptCount;
            std::unique_ptr<NetSenderLinkPeer> peer(new NetSenderLinkPeer(m_options));
            found = m_peers.insert(std::make_pair(id, std::move(peer))).first;
        }
        found->second->m_connected = true;
        if (found->second->m_link.status() < 0) { close(id, found->second->m_link.status()); }
    }
    catch (...) { m_raw.close(id, NI_NO_MEMORY); }
}

bool NetSenderLinkState::onRawLocalRecord(uint64_t id, const NetInterfaceTunnelRecord& record)
{
    auto found = m_peers.find(id);
    if (m_stopping || found == m_peers.end() || found->second->m_closing) { return true; }
    const int32_t accepted = found->second->m_link.accept(record);
    if (accepted == NI_LIMIT_EXCEEDED) { return false; }
    if (accepted != NI_OK) { close(id, accepted); }
    return true;
}

void NetSenderLinkState::onRawLocalClosed(uint64_t id, int32_t reason)
{
    auto found = m_peers.find(id);
    if (found == m_peers.end() || found->second->m_closed)
    {
        return;
    }
    found->second->m_closed = true;
    if (!found->second->m_closing)
    {
        found->second->m_reason = reason;
    }
}

void NetSenderLinkState::collectClosed()
{
    for (auto entry = m_peers.begin(); entry != m_peers.end();)
    {
        if (!entry->second->m_closed || !entry->second->m_closing)
        {
            ++entry;
            continue;
        }
        const uint64_t id = entry->first;
        const int32_t reason = entry->second->m_reason;
        entry = m_peers.erase(entry);
        try
        {
            m_listener->onLinkClosed(id, reason);
        }
        catch (...)
        {
            // 已回收的代次只报告一次；继续收敛其他连接。
        }
    }
}

void NetSenderLinkState::pump(uint64_t id, NetSenderLinkPeer& peer)
{
    if (peer.m_closing || !peer.m_connected || m_stopping) { return; }
    const uint64_t now = SenderLinkNow();
    const int32_t progressed = peer.m_closed ? peer.m_link.drainInput(now) : peer.m_link.progress(now);
    if (progressed < 0) { close(id, progressed); return; }
    if (progressed == NI_READY && !peer.m_announced)
    {
        NetInterfaceLinkPeer identity = {};
        if (peer.m_link.peerInfo(identity) != NI_READY) { close(id, NI_INVALID_STATE); return; }
        peer.m_announced = true;
        m_listener->onLinkReady(id, identity);
    }
    bool blocked = false;
    for (uint32_t step = 0; step < 64 && !peer.m_closing && !m_stopping; ++step)
    {
        NetInterfaceFrame frame = {};
        const int32_t ready = peer.m_link.peekControl(frame);
        if (ready < 0) { close(id, ready); return; }
        if (ready != NI_READY) { break; }
        if (!m_listener->onLinkControl(id, frame)) { blocked = true; break; }
        if (peer.m_link.consumeControl() != NI_OK) { close(id, NI_INVALID_STATE); return; }
        // consume only releases the decoded frame; advance TLS again to expose
        // the next coalesced control before deciding that the control queue is empty.
        const int32_t advanced = peer.m_closed ? peer.m_link.drainInput(now) : peer.m_link.progress(now);
        if (advanced < 0) { close(id, advanced); return; }
    }
    NetInterfaceFrame deferred = {};
    const int32_t controlPending = peer.m_link.peekControl(deferred);
    if (controlPending < 0) { close(id, controlPending); return; }
    // OPEN/CLOSE establishes the channel ownership needed to dispatch records.
    // Never let data overtake an unconsumed control, including budget exhaustion.
    for (uint32_t step = 0; step < 64 && controlPending != NI_READY && !peer.m_closing && !m_stopping; ++step)
    {
        NetInterfaceTunnelRecord record = {};
        const int32_t ready = peer.m_link.peekRecord(record);
        if (ready < 0) { close(id, ready); return; }
        if (ready != NI_READY) { break; }
        if (!m_listener->onLinkRecord(id, record)) { blocked = true; break; }
        if (peer.m_link.consumeRecord() != NI_OK) { close(id, NI_INVALID_STATE); return; }
    }
    if (blocked)
    {
        if (peer.m_blockedSince == 0) { peer.m_blockedSince = now; }
        else if (now - peer.m_blockedSince >= 5000) { close(id, NI_TIMEOUT); return; }
    }
    else { peer.m_blockedSince = 0; }
    for (uint32_t step = 0; step < 64 && !peer.m_closed && !peer.m_closing && !m_stopping; ++step)
    {
        NetInterfaceTunnelRecord record = {};
        const int32_t ready = peer.m_link.peekOutput(record);
        if (ready < 0) { close(id, ready); return; }
        if (ready != NI_READY) { break; }
        const int32_t sent = m_raw.sendRecord(id, record);
        if (sent == NI_LIMIT_EXCEEDED) { break; }
        if (sent != NI_OK) { close(id, sent); return; }
        if (peer.m_link.consumeOutput() != NI_OK) { close(id, NI_INVALID_STATE); return; }
    }
}