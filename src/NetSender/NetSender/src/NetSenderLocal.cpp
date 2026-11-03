#include "NetSenderLocal.h"
#include "NetSenderMacro.h"
#include "NetSenderLocalState.h"
#include <algorithm>
#include <cstring>
#include <utility>
#include "CSystem/CSystemAPI.h"
#include <chrono>

NetSenderLocalListener::~NetSenderLocalListener()
{

}

NetSenderLocal::NetSenderLocal(NetSenderLocalInbox& inbox) : m_state(new NetSenderLocalState(inbox))
{

}

NetSenderLocal::~NetSenderLocal()
{
    shutdown();
    delete m_state;
}

int32_t NetSenderLocal::listen(const NetInterfaceTlsOptions& tls, NetSenderLocalListener* listener, uint32_t maximum)
{
    return m_state->configure(tls, listener, true, maximum);
}

int32_t NetSenderLocal::connect(int32_t pid, const NetInterfaceTlsOptions& tls,
    NetSenderLocalListener* listener, uint64_t& peer)
{
    peer = 0;
    if (pid <= 0 || pid == m_state->m_pid)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t configured = m_state->configure(tls, listener, false, 1);
    return configured != NI_OK ? configured : m_state->createPeer(pid, nullptr, peer);
}

bool NetSenderLocal::poll()
{
    return m_state->poll();
}

int32_t NetSenderLocal::send(uint64_t peer, const NetInterfaceFrame& frame)
{
    const auto found = m_state->m_peers.find(peer);
    if (m_state->m_rawListener != nullptr || m_state->m_stopping || found == m_state->m_peers.end() ||
        !found->second->m_ready || found->second->m_phase != NET_LOCAL_ACTIVE)
    {
        return NI_INVALID_STATE;
    }
    return found->second->m_connection->send(frame);
}

void NetSenderLocal::close(uint64_t peer, int32_t reason)
{
    m_state->close(peer, reason);
}

void NetSenderLocal::shutdown()
{
    m_state->shutdown();
}

uint32_t NetSenderLocal::peerCount() const
{
    return static_cast<uint32_t>(m_state->m_peers.size());
}

int32_t NetSenderLocal::outputState(uint64_t peer) const
{
    const auto found = m_state->m_peers.find(peer);
    if (found == m_state->m_peers.end() || found->second->m_phase != NET_LOCAL_ACTIVE) { return NI_INVALID_STATE; }
    if (found->second->m_connection) { return found->second->m_connection->outputState(); }
    return found->second->m_rawOutput.empty() ? NI_OK : NI_READY;
}

// NET_SENDER_LOCAL_PHASE_UNIT_BEGIN
//#include <cstdio>
//
//int main()
//{
//    uint32_t checks = 0;
//#define LOCAL_PHASE_CHECK(value) do { ++checks; if (!(value)) { std::printf("Local phase FAIL line %d\n", __LINE__); return 1; } } while (false)
//    NetSenderLocalPeer lost;
//    LOCAL_PHASE_CHECK(lost.m_phase == NET_LOCAL_ACTIVE && lost.m_drainSince == 0);
//    lost.drain(0, false);
//    LOCAL_PHASE_CHECK(lost.m_phase == NET_LOCAL_DRAINING && lost.m_drainSince == 0);
//    lost.drain(4999, false);
//    LOCAL_PHASE_CHECK(lost.m_phase == NET_LOCAL_DRAINING && lost.m_drainSince == 0);
//    lost.drain(4999, true);
//    LOCAL_PHASE_CHECK(lost.m_phase == NET_LOCAL_ENDED && lost.m_drainSince == 0);
//    lost.drain(5000, false);
//    lost.drain(5001, true);
//    LOCAL_PHASE_CHECK(lost.m_phase == NET_LOCAL_ENDED && lost.m_drainSince == 0);
//    lost.m_phase = NET_LOCAL_CLOSED;
//    lost.drain(6000, true);
//    LOCAL_PHASE_CHECK(lost.m_phase == NET_LOCAL_CLOSED && lost.m_drainSince == 0);
//    NetSenderLocalPeer ended;
//    ended.drain(17, true);
//    LOCAL_PHASE_CHECK(ended.m_phase == NET_LOCAL_ENDED && ended.m_drainSince == 17);
//    ended.drain(9000, true);
//    ended.drain(9001, false);
//    LOCAL_PHASE_CHECK(ended.m_phase == NET_LOCAL_ENDED && ended.m_drainSince == 17);
//    std::printf("Local phase PASS: %u assertions\n", checks);
//    return 0;
//}
//
//#undef LOCAL_PHASE_CHECK
// NET_SENDER_LOCAL_PHASE_UNIT_END

int32_t NetSenderLocal::listenRaw(NetInterfaceEntropyFunction entropy, void* context,
    NetSenderLocalRawListener* listener, uint32_t maximum)
{
    return m_state->configureRaw(entropy, context, listener, true, maximum);
}

int32_t NetSenderLocal::connectRaw(int32_t pid, NetInterfaceEntropyFunction entropy, void* context,
    NetSenderLocalRawListener* listener, uint64_t& peer)
{
    peer = 0;
    if (pid <= 0 || pid == m_state->m_pid)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t configured = m_state->configureRaw(entropy, context, listener, false, 1);
    return configured == NI_OK ? m_state->createPeer(pid, nullptr, peer) : configured;
}

int32_t NetSenderLocal::sendRecord(uint64_t id, const NetInterfaceTunnelRecord& record)
{
    auto found = m_state->m_peers.find(id);
    if (m_state->m_rawListener == nullptr || m_state->m_stopping || found == m_state->m_peers.end() ||
        !found->second->m_ready || found->second->m_phase != NET_LOCAL_ACTIVE)
    {
        return NI_INVALID_STATE;
    }
    uint32_t required = 0;
    const int32_t checked = NetInterfaceTunnel::encode(record, nullptr, 0, required);
    if (checked != NI_BUFFER_TOO_SMALL)
    {
        return checked;
    }
    NetSenderLocalPeer& peer = *found->second;
    const uint32_t limit = 4 * (NET_INTERFACE_TUNNEL_PAYLOAD_LIMIT + NET_INTERFACE_TUNNEL_HEADER_SIZE);
    if (required > limit - peer.m_rawOutputBytes || peer.m_rawOutput.size() >= 32)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        NetSenderLocalChunk output;
        output.m_packet = 0;
        output.m_offset = 0;
        output.m_bytes.resize(required);
        const int32_t encoded = NetInterfaceTunnel::encode(record, output.m_bytes.data(), required, required);
        if (encoded != NI_OK)
        {
            return encoded;
        }
        peer.m_rawOutput.push_back(std::move(output));
        peer.m_rawOutputBytes += required;
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

NetSenderLocalInbox::NetSenderLocalInbox() :
m_state(new NetSenderLocalInboxState)
{

}

NetSenderLocalInbox::~NetSenderLocalInbox()
{
    stop();
    delete m_state;
}

void NetSenderLocalInbox::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
    if (static_cast<int32_t>(type) != NET_SENDER_LOCAL_MESSAGE || sendPid <= 0 ||
        buffer == nullptr || length < 56 || length > 16440)
    {
        return;
    }
    NetSenderLocalPacketView packet = {};
    if (NetSenderLocalPacket::parse(reinterpret_cast<const uint8_t*>(buffer), static_cast<uint32_t>(length), packet) != NI_OK)
    {
        return;
    }
    WriteLock lock(m_state->m_mutex);
    if (!m_state->m_active)
    {
        return;
    }
    NetSenderLocalInboxLane* lane = nullptr;
    if (packet.m_kind == NET_LOCAL_OPEN)
    {
        if (!m_state->m_acceptOpen)
        {
            return;
        }
        // 已登记远端的后续OPEN与其DATA共用FIFO，不能在握手换目标号时发生超车。
        for (const auto& entry : m_state->m_generations)
        {
            if (entry.second->m_pid == sendPid && NetSenderLocalPacket::sameGeneration(entry.second->m_remote.data(), packet.m_source))
            {
                lane = entry.second.get();
                break;
            }
        }
        if (lane == nullptr)
        {
            // 未识别入口满时丢弃本次握手；已建立通道保留自己的全部槽位。
            if (m_state->m_open.m_input.size() >= 64)
            {
                return;
            }
            lane = &m_state->m_open;
        }
    }
    else
    {
        std::array<uint8_t, 16> target = {};
        std::copy(packet.m_target, packet.m_target + 16, target.begin());
        const auto found = m_state->m_generations.find(target);
        if (found == m_state->m_generations.end() || found->second->m_pid != sendPid)
        {
            return;
        }
        lane = found->second.get();
        const std::array<uint8_t, 16> zero = {};
        if (lane->m_remote != zero && !NetSenderLocalPacket::sameGeneration(lane->m_remote.data(), packet.m_source))
        {
            return;
        }
    }
    if (lane != &m_state->m_open)
    {
        if (lane->m_reason < 0)
        {
            return;
        }
        const bool data = packet.m_kind == NET_LOCAL_OPEN || packet.m_kind == NET_LOCAL_DATA;
        if ((data && lane->m_dataCount >= 4) || (!data && lane->m_controlCount >= 16))
        {
            m_state->fail(*lane, NI_LIMIT_EXCEEDED);
            return;
        }
    }
    try
    {
        NetSenderLocalInput input;
        input.m_pid = sendPid;
        input.m_peer = lane->m_peer;
        input.m_kind = packet.m_kind;
        input.m_bytes.assign(reinterpret_cast<const uint8_t*>(buffer), reinterpret_cast<const uint8_t*>(buffer) + length);
        lane->m_input.push_back(std::move(input));
        if (packet.m_kind == NET_LOCAL_ACK || packet.m_kind == NET_LOCAL_END)
        {
            ++lane->m_controlCount;
        }
        else
        {
            ++lane->m_dataCount;
        }
        m_state->queue(*lane);
    }
    catch (...)
    {
        if (lane != &m_state->m_open)
        {
            m_state->fail(*lane, NI_NO_MEMORY);
        }
    }
}

void NetSenderLocalInbox::start(bool server)
{
    WriteLock lock(m_state->m_mutex);
    m_state->clear();
    m_state->m_active = true;
    m_state->m_acceptOpen = server;
}

void NetSenderLocalInbox::stop()
{
    WriteLock lock(m_state->m_mutex);
    m_state->m_active = false;
    m_state->m_acceptOpen = false;
    m_state->clear();
}

int32_t NetSenderLocalInbox::take(NetSenderLocalInput& input)
{
    WriteLock lock(m_state->m_mutex);
    input.m_pid = 0;
    input.m_peer = 0;
    input.m_reason = NI_OK;
    input.m_kind = 0;
    input.m_bytes.clear();
    NetSenderLocalInboxLane* lane = m_state->m_first;
    if (lane == nullptr)
    {
        return NI_NEED_MORE;
    }
    m_state->unqueue(*lane);
    if (lane->m_reason < 0)
    {
        input.m_pid = lane->m_pid;
        input.m_peer = lane->m_peer;
        input.m_reason = lane->m_reason;
        return NI_READY;
    }
    input = std::move(lane->m_input.front());
    lane->m_input.pop_front();
    if (input.m_kind == NET_LOCAL_ACK || input.m_kind == NET_LOCAL_END)
    {
        --lane->m_controlCount;
    }
    else
    {
        --lane->m_dataCount;
    }
    if (!lane->m_input.empty())
    {
        m_state->queue(*lane);
    }
    return NI_READY;
}

int32_t NetSenderLocalInbox::allowGeneration(const uint8_t* generation, int32_t pid, uint64_t peer, const uint8_t* remote)
{
    if (generation == nullptr || pid <= 0 || peer == 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    std::array<uint8_t, 16> value = {};
    std::copy(generation, generation + 16, value.begin());
    const std::array<uint8_t, 16> zero = {};
    if (value == zero)
    {
        return NI_INVALID_ARGUMENT;
    }
    WriteLock lock(m_state->m_mutex);
    if (!m_state->m_active || m_state->m_generations.size() >= 64 || m_state->m_generations.find(value) != m_state->m_generations.end())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetSenderLocalInboxLane> lane(new NetSenderLocalInboxLane(pid, peer, remote));
        const auto inserted = m_state->m_generations.insert(std::make_pair(value, std::move(lane)));
        if (remote != nullptr)
        {
            m_state->adoptOpen(*inserted.first->second);
        }
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetSenderLocalInbox::removeGeneration(const uint8_t* generation)
{
    if (generation == nullptr)
    {
        return;
    }
    std::array<uint8_t, 16> value = {};
    std::copy(generation, generation + 16, value.begin());
    WriteLock lock(m_state->m_mutex);
    const auto found = m_state->m_generations.find(value);
    if (found != m_state->m_generations.end())
    {
        m_state->unqueue(*found->second);
        m_state->m_generations.erase(found);
    }
}