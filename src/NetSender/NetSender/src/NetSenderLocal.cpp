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

int32_t NetSenderLocal::configureOutputPool(const NetBufferPoolConfig& config)
{
    if (m_state->m_started || m_state->m_stopping)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetBufferPool> pool(new NetBufferPool(config));
        if (pool->status() != NI_OK)
        {
            return pool->status();
        }
        m_state->m_outputPool.swap(pool);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderLocal::configurePeerPool(const LockFreeHashMapPoolConfig& config)
{
    if (m_state->m_started || m_state->m_stopping) { return NI_INVALID_STATE; }
    try
    {
        NetObjectPool<NetSenderLocalPeer> probe(config);
        if (probe.status() != NI_OK) { return probe.status(); }
        std::unique_ptr<LockFreeHashMapPoolConfig> copy(new LockFreeHashMapPoolConfig(config));
        m_state->m_peerConfig.swap(copy);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderLocal::configureConnectionPools(const NetConnectionPoolConfig& config)
{
    if (m_state->m_started)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetConnectionPoolConfig> copy(new NetConnectionPoolConfig(config));
        m_state->m_connectionPools.swap(copy);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
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
    try
    {
        NetSenderLocalChunk output;
        output.m_packet = 0;
        output.m_offset = 0;
        const int32_t allocated = output.m_bytes.allocate(*m_state->m_outputPool, required);
        if (allocated != NI_OK)
        {
            return allocated;
        }
        const int32_t encoded = NetInterfaceTunnel::encode(record, output.m_bytes.data(), required, required);
        if (encoded != NI_OK)
        {
            return encoded;
        }
        peer.m_rawOutput.push_back(std::move(output));
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

int32_t NetSenderLocalInbox::configurePools(const NetSenderLocalInboxPools& pools)
{
    if (m_state->m_started)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetSenderLocalInboxState> state(new NetSenderLocalInboxState(pools));
        if (state->m_initializationResult != NI_OK)
        {
            return state->m_initializationResult;
        }
        delete m_state;
        m_state = state.release();
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
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
    NetSenderLocalInboxCall call(*m_state);
    if (!call.accepted())
    {
        return;
    }
    const bool open = packet.m_kind == NET_LOCAL_OPEN;
    if (open && !m_state->m_acceptOpen)
    {
        return;
    }
    std::shared_ptr<NetSenderLocalInboxLane> lane;
    const NetSenderLocalRouteKey key(open ? packet.m_source : packet.m_target, sendPid, open);
    if (!m_state->m_routes.get(key, lane))
    {
        // 快速查询的false也可能表示保护记录不足，不能把资源失败当成未知包静默丢弃。
        decltype(m_state->m_routes)::Snapshot snapshot;
        const LockFreeHashMapResult found = m_state->m_routes.find(key, snapshot);
        if (found.m_status == LockFreeHashMapOk)
        {
            lane = *snapshot.m_value;
        }
        else if (found.m_status != LockFreeHashMapNotFound)
        {
            m_state->m_routeFailed.store(true);
            return;
        }
        else if (open)
        {
            lane = m_state->m_open;
        }
        else
        {
            return;
        }
    }
    if (lane->m_retired.load() || lane->m_reason.load() < 0)
    {
        return;
    }
    const std::array<uint8_t, 16> zero = {};
    if (!open && lane->m_remote != zero && !NetSenderLocalPacket::sameGeneration(lane->m_remote.data(), packet.m_source))
    {
        return;
    }
    int32_t failure = NI_NO_MEMORY;
    try
    {
        std::unique_ptr<NetSenderLocalInput> input(new NetSenderLocalInput);
        input->m_lane = lane;
        input->m_pid = sendPid;
        input->m_peer = lane->m_peer;
        input->m_kind = packet.m_kind;
        NetBufferPool& pool = lane == m_state->m_open ? m_state->m_openPool :
            (packet.m_kind == NET_LOCAL_ACK || packet.m_kind == NET_LOCAL_END) ? m_state->m_controlPool : m_state->m_dataPool;
        failure = input->m_bytes.allocate(pool, static_cast<uint32_t>(length));
        if (failure != NI_OK)
        {
            if (lane != m_state->m_open)
            {
                m_state->fail(*lane, failure);
            }
            return;
        }
        std::memcpy(input->m_bytes.data(), buffer, static_cast<uint32_t>(length));
        const LockFreeQueueResult queued = m_state->m_incoming.tryPush(input.get(), true, false);
        if (queued == LockFreeQueueSuccess)
        {
            // 队列接纳后由owner独占，回调不能继续访问已可能被消费的对象。
            input.release();
            return;
        }
        failure = queued == LockFreeQueueCapacityLimit || queued == LockFreeQueueResourceUnavailable ||
            queued == LockFreeQueueGrowthDeclined || queued == LockFreeQueueSequenceExhausted ? NI_LIMIT_EXCEEDED :
            queued == LockFreeQueueAllocationFailure ? NI_NO_MEMORY : NI_INVALID_STATE;
    }
    catch (...)
    {
        failure = NI_NO_MEMORY;
    }
    if (lane != m_state->m_open)
    {
        m_state->fail(*lane, failure);
    }
}

void NetSenderLocalInbox::start(bool server)
{
    m_state->m_started = true;
    m_state->quiesce();
    m_state->clear();
    m_state->m_active = m_state->m_initializationResult == NI_OK && !m_state->m_routeFailed && m_state->m_routes.status() == LockFreeHashMapOk;
    if (!m_state->m_active)
    {
        return;
    }
    m_state->m_execution = NetExecution::current();
    m_state->m_acceptOpen = server;
    m_state->m_admission.store(0, std::memory_order_release);
}

void NetSenderLocalInbox::stop()
{
    m_state->quiesce();
    m_state->m_execution = nullptr;
    m_state->m_active = false;
    m_state->m_acceptOpen = false;
    m_state->clear();
}

int32_t NetSenderLocalInbox::take(NetSenderLocalInput& input)
{
    m_state->pump();
    input.m_lane.reset();
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
    NetExecution::progress();
    m_state->unqueue(*lane);
    if (lane->m_reason < 0)
    {
        input.m_pid = lane->m_pid;
        input.m_peer = lane->m_peer;
        input.m_reason = lane->m_reason.load();
        lane->m_failureTaken = true;
        m_state->discard(*lane);
        return NI_READY;
    }
    input = std::move(lane->m_input.front());
    lane->m_input.pop_front();
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
    if (!m_state->m_active || m_state->m_routeFailed || m_state->m_generations.find(value) != m_state->m_generations.end())
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::shared_ptr<NetSenderLocalInboxLane> lane(new NetSenderLocalInboxLane(pid, peer, remote));
        m_state->m_generations.insert(std::make_pair(value, lane));
        const NetSenderLocalRouteKey localKey(generation, pid, false);
        decltype(m_state->m_routes)::Snapshot snapshot;
        int32_t published = NetSenderLocalInboxState::routeResult(
            m_state->m_routes.tryInsertIfAbsent(localKey, lane, snapshot, true, false));
        if (published == NI_OK && remote != nullptr)
        {
            published = NetSenderLocalInboxState::routeResult(
                m_state->m_routes.tryInsertIfAbsent(NetSenderLocalRouteKey(remote, pid, true), lane, snapshot, true, false));
        }
        if (published != NI_OK)
        {
            removeGeneration(generation);
            return published;
        }
        if (remote != nullptr)
        {
            m_state->adoptOpen(*lane);
        }
        return NI_OK;
    }
    catch (...)
    {
        removeGeneration(generation);
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
    const auto found = m_state->m_generations.find(value);
    if (found != m_state->m_generations.end())
    {
        std::shared_ptr<NetSenderLocalInboxLane> lane = found->second;
        lane->m_retired.store(true);
        m_state->eraseRoute(NetSenderLocalRouteKey(value.data(), lane->m_pid, false), lane);
        const std::array<uint8_t, 16> zero = {};
        if (lane->m_remote != zero)
        {
            m_state->eraseRoute(NetSenderLocalRouteKey(lane->m_remote.data(), lane->m_pid, true), lane);
        }
        m_state->unqueue(*lane);
        m_state->discard(*lane);
        m_state->m_generations.erase(found);
    }
}