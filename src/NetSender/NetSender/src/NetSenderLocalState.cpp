#include "NetSenderLocalState.h"
#include "NetSenderMacro.h"
#include "CSystem/CSystemAPI.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <utility>

int32_t NetSenderLocalState::configureRaw(NetInterfaceEntropyFunction entropy, void* context,
    NetSenderLocalRawListener* listener, bool server, uint32_t maximum)
{
    if (m_started || m_stopping || listener == nullptr || entropy == nullptr || maximum == 0 || maximum > 64 || m_pid <= 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t capacity = ProcessWork::instance().receivePayloadCapacity();
    if (ProcessWork::instance().receiveBufferCount() == 0)
    {
        return NI_INVALID_STATE;
    }
    if (capacity < kNetSenderLocalReceiveBytes - 12)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    m_started = true;
    m_rawListener = listener;
    m_options = NetInterfaceTlsOptions();
    m_options.server = server ? 1 : 0;
    m_options.entropy = entropy;
    m_options.entropyContext = context;
    m_maximum = maximum;
    m_now = m_acceptWindow = now();
    m_inbox->start(server);
    return NI_OK;
}

void NetSenderLocalState::pumpRaw(uint64_t id, NetSenderLocalPeer& peer)
{
    if (!peer.m_ready)
    {
        // 这里不认证PID。业务/管理端点将通过远端CA及本次挑战认证。
        peer.m_ready = true;
        m_rawListener->onRawLocalReady(id, peer.m_pid);
    }
    for (uint32_t step = 0; step < 64 && peer.m_phase != NET_LOCAL_CLOSED && !m_stopping; ++step)
    {
        NetInterfaceTunnelRecord record = {};
        const int32_t ready = peer.m_rawDecoder->peek(record);
        if (ready == NI_READY)
        {
            if (!m_rawListener->onRawLocalRecord(id, record))
            {
                if (peer.m_rawBlockedSince == 0)
                {
                    peer.m_rawBlockedSince = m_now;
                }
                else if (m_now - peer.m_rawBlockedSince >= 5000)
                {
                    close(id, NI_TIMEOUT);
                }
                break;
            }
            peer.m_rawBlockedSince = 0;
            peer.m_rawDecoder->consume();
            peer.m_activity = m_now;
        }
        else if (ready < 0)
        {
            close(id, ready);
            break;
        }
        else if (!peer.m_input.empty())
        {
            NetSenderLocalChunk& chunk = peer.m_input.front();
            uint32_t used = 0;
            const int32_t fed = peer.m_rawDecoder->feed(chunk.m_bytes.data() + chunk.m_offset,
                static_cast<uint32_t>(chunk.m_bytes.size()) - chunk.m_offset, used);
            if (fed < 0)
            {
                close(id, fed);
                break;
            }
            chunk.m_offset += used;
            if (chunk.m_offset == chunk.m_bytes.size())
            {
                if (peer.m_window.received(chunk.m_packet) != NI_OK)
                {
                    close(id, NI_PROTOCOL_ERROR);
                    break;
                }
                peer.m_ackPending = peer.m_window.receipt();
                peer.m_input.pop_front();
            }
        }
        else
        {
            if (peer.m_phase == NET_LOCAL_ENDED)
            {
                const int32_t finished = peer.m_rawDecoder->finish();
                close(id, finished < 0 ? finished : NI_OK);
            }
            break;
        }
    }
    if (peer.m_phase == NET_LOCAL_CLOSED || m_stopping)
    {
        return;
    }
    if (peer.m_ackPending != 0 && peer.m_remoteKnown && peer.m_phase == NET_LOCAL_ACTIVE)
    {
        const int32_t sent = transmit(peer, NET_LOCAL_ACK, peer.m_ackPending, nullptr, 0);
        if (sent == NET_SENDER_IPC_ACCEPTED)
        {
            peer.m_ackPending = 0;
        }
        else if (sent != NET_SENDER_IPC_BUSY)
        {
            sendFailure(id, peer, sent);
            return;
        }
    }
    uint32_t budget = 65536;
    for (uint32_t step = 0; step < 64 && !peer.m_rawOutput.empty() && peer.m_phase == NET_LOCAL_ACTIVE && !m_stopping; ++step)
    {
        const uint64_t next = peer.m_window.next();
        if (next == 0 || budget == 0)
        {
            break;
        }
        NetSenderLocalChunk& front = peer.m_rawOutput.front();
        const uint32_t size = (std::min)((std::min)(static_cast<uint32_t>(front.m_bytes.size()) - front.m_offset, 16384U), budget);
        const int32_t sent = transmit(peer, peer.m_remoteKnown ? NET_LOCAL_DATA : NET_LOCAL_OPEN,
            next, front.m_bytes.data() + front.m_offset, size);
        if (sent == NET_SENDER_IPC_BUSY)
        {
            break;
        }
        if (sent != NET_SENDER_IPC_ACCEPTED)
        {
            sendFailure(id, peer, sent);
            break;
        }
        if (peer.m_window.outstanding() == 0)
        {
            peer.m_ackTime = m_now;
        }
        if (peer.m_window.published(next) != NI_OK)
        {
            close(id, NI_INVALID_STATE);
            break;
        }
        front.m_offset += size;
        peer.m_rawOutputBytes -= size;
        budget -= size;
        if (front.m_offset == front.m_bytes.size())
        {
            peer.m_rawOutput.pop_front();
        }
    }
}

NetSenderLocalInput::NetSenderLocalInput() :
m_pid(0),
m_peer(0),
m_reason(NI_OK),
m_kind(0)
{

}

NetSenderLocalInput::NetSenderLocalInput(NetSenderLocalInput&& other) :
m_pid(other.m_pid),
m_peer(other.m_peer),
m_reason(other.m_reason),
m_kind(other.m_kind),
m_bytes(std::move(other.m_bytes))
{
    other.m_pid = 0;
    other.m_peer = 0;
    other.m_reason = NI_OK;
    other.m_kind = 0;
}

NetSenderLocalInput& NetSenderLocalInput::operator=(NetSenderLocalInput&& other)
{
    if (this != &other)
    {
        m_pid = other.m_pid;
        m_peer = other.m_peer;
        m_reason = other.m_reason;
        m_kind = other.m_kind;
        m_bytes = std::move(other.m_bytes);
        other.m_pid = 0;
        other.m_peer = 0;
        other.m_reason = NI_OK;
        other.m_kind = 0;
    }
    return *this;
}

NetSenderLocalInboxLane::NetSenderLocalInboxLane(int32_t pid, uint64_t peer, const uint8_t* remote) :
m_pid(pid),
m_peer(peer),
m_remote(),
m_dataCount(0),
m_controlCount(0),
m_reason(NI_OK),
m_queued(false),
m_previous(nullptr),
m_next(nullptr)
{
    if (remote != nullptr)
    {
        std::copy(remote, remote + 16, m_remote.begin());
    }
}

NetSenderLocalInboxState::NetSenderLocalInboxState() :
m_open(0, 0),
m_first(nullptr),
m_last(nullptr),
m_active(false),
m_acceptOpen(false)
{

}

void NetSenderLocalInboxState::queue(NetSenderLocalInboxLane& lane)
{
    if (lane.m_queued)
    {
        return;
    }
    lane.m_previous = m_last;
    lane.m_next = nullptr;
    if (m_last == nullptr)
    {
        m_first = &lane;
    }
    else
    {
        m_last->m_next = &lane;
    }
    m_last = &lane;
    lane.m_queued = true;
}

void NetSenderLocalInboxState::unqueue(NetSenderLocalInboxLane& lane)
{
    if (!lane.m_queued)
    {
        return;
    }
    if (lane.m_previous == nullptr)
    {
        m_first = lane.m_next;
    }
    else
    {
        lane.m_previous->m_next = lane.m_next;
    }
    if (lane.m_next == nullptr)
    {
        m_last = lane.m_previous;
    }
    else
    {
        lane.m_next->m_previous = lane.m_previous;
    }
    lane.m_previous = nullptr;
    lane.m_next = nullptr;
    lane.m_queued = false;
}

void NetSenderLocalInboxState::fail(NetSenderLocalInboxLane& lane, int32_t reason)
{
    if (lane.m_reason < 0)
    {
        return;
    }
    lane.m_reason = reason;
    lane.m_input.clear();
    lane.m_dataCount = 0;
    lane.m_controlCount = 0;
    queue(lane);
}

void NetSenderLocalInboxState::adoptOpen(NetSenderLocalInboxLane& lane)
{
    auto input = m_open.m_input.begin();
    while (input != m_open.m_input.end())
    {
        // 收件箱中只有验证完整的NIP2包；源代次位于固定头16处。
        if (input->m_pid != lane.m_pid || !NetSenderLocalPacket::sameGeneration(input->m_bytes.data() + 16, lane.m_remote.data()))
        {
            ++input;
            continue;
        }
        if (lane.m_reason == NI_OK)
        {
            if (lane.m_dataCount >= 4)
            {
                fail(lane, NI_LIMIT_EXCEEDED);
            }
            else
            {
                try
                {
                    input->m_peer = lane.m_peer;
                    lane.m_input.push_back(std::move(*input));
                    ++lane.m_dataCount;
                }
                catch (...)
                {
                    fail(lane, NI_NO_MEMORY);
                }
            }
        }
        input = m_open.m_input.erase(input);
        --m_open.m_dataCount;
    }
    if (m_open.m_input.empty())
    {
        unqueue(m_open);
    }
    if (!lane.m_input.empty() || lane.m_reason < 0)
    {
        queue(lane);
    }
}

void NetSenderLocalInboxState::clear()
{
    m_first = nullptr;
    m_last = nullptr;
    m_generations.clear();
    m_open.m_input.clear();
    m_open.m_dataCount = 0;
    m_open.m_controlCount = 0;
    m_open.m_reason = NI_OK;
    m_open.m_queued = false;
    m_open.m_previous = nullptr;
    m_open.m_next = nullptr;
}

NetSenderLocalChunk::NetSenderLocalChunk() : m_packet(0), m_offset(0)
{

}

NetSenderLocalChunk::NetSenderLocalChunk(NetSenderLocalChunk&& other) :
m_packet(other.m_packet), m_offset(other.m_offset), m_bytes(std::move(other.m_bytes))
{
    other.m_packet = 0;
    other.m_offset = 0;
}

NetSenderLocalChunk& NetSenderLocalChunk::operator=(NetSenderLocalChunk&& other)
{
    if (this != &other)
    {
        m_packet = other.m_packet;
        m_offset = other.m_offset;
        m_bytes = std::move(other.m_bytes);
        other.m_packet = 0;
        other.m_offset = 0;
    }
    return *this;
}

NetSenderLocalPeer::NetSenderLocalPeer() :
m_pid(0),
m_reason(0),
m_ready(false),
m_phase(NET_LOCAL_ACTIVE),
m_remoteKnown(false),
m_ackPending(0),
m_ackTime(0),
m_activity(0),
m_drainSince(0),
m_pingTime(0),
m_pingId(0),
m_rawOutputBytes(0),
m_rawBlockedSince(0)
{
    std::memset(m_local, 0, sizeof(m_local));
    std::memset(m_remote, 0, sizeof(m_remote));
}

void NetSenderLocalPeer::drain(uint64_t now, bool end)
{
    if (m_phase == NET_LOCAL_CLOSED || m_phase == NET_LOCAL_ENDED)
    {
        return;
    }
    if (m_phase == NET_LOCAL_ACTIVE)
    {
        m_drainSince = now;
    }
    m_phase = end ? NET_LOCAL_ENDED : NET_LOCAL_DRAINING;
}

NetSenderLocalState::NetSenderLocalState(NetSenderLocalInbox& inbox) : m_inbox(&inbox), m_listener(nullptr), m_rawListener(nullptr),
    m_options(), m_pid(static_cast<int32_t>(CSystem::currentProcessPid())), m_maximum(0), m_acceptCount(0),
    m_acceptWindow(0), m_lastPeer(0), m_lastPing(0), m_now(0), m_processCheckAt(0), m_started(false), m_stopping(false), m_polling(false)
{

}

NetSenderLocalState::~NetSenderLocalState()
{
    if (!m_privateKey.empty())
    {
        volatile uint8_t* bytes = m_privateKey.data();
        for (size_t index = 0; index < m_privateKey.size(); ++index)
        {
            bytes[index] = 0;
        }
    }
}

uint64_t NetSenderLocalState::now()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

int32_t NetSenderLocalState::configure(const NetInterfaceTlsOptions& tls,
    NetSenderLocalListener* listener, bool server, uint32_t maximum)
{
    if (m_started || m_stopping || listener == nullptr || tls.server != (server ? 1 : 0) ||
        tls.entropy == nullptr || maximum == 0 || maximum > 64 || m_pid <= 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t capacity = ProcessWork::instance().receivePayloadCapacity();
    if (ProcessWork::instance().receiveBufferCount() == 0)
    {
        return NI_INVALID_STATE;
    }
    if (capacity < kNetSenderLocalReceiveBytes - 12)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    m_started = true;
    m_options = tls;
    try
    {
        NetInterfaceConnection validation(tls);
        if (validation.status() < 0)
        {
            m_stopping = true;
            return validation.status();
        }
        if (server)
        {
            m_certificate.assign(tls.certificate, tls.certificate + tls.certificateSize);
            m_privateKey.assign(tls.privateKey, tls.privateKey + tls.privateKeySize);
            m_options.certificate = m_certificate.data();
            m_options.privateKey = m_privateKey.data();
            m_options.authority = nullptr;
            m_options.authoritySize = 0;
            m_options.hostname = nullptr;
        }
        else
        {
            m_authority.assign(tls.authority, tls.authority + tls.authoritySize);
            m_hostname = tls.hostname;
            m_options.authority = m_authority.data();
            m_options.hostname = m_hostname.c_str();
            m_options.certificate = m_options.privateKey = nullptr;
            m_options.certificateSize = m_options.privateKeySize = 0;
        }
        m_listener = listener;
        m_maximum = maximum;
        m_now = m_acceptWindow = now();
        m_inbox->start(server);
        return NI_OK;
    }
    catch (...)
    {
        m_stopping = true;
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderLocalState::createPeer(int32_t pid, const uint8_t* remote, uint64_t& id)
{
    id = 0;
    if (pid <= 0 || pid == m_pid || m_stopping || !m_started || (m_listener == nullptr && m_rawListener == nullptr) ||
        m_peers.size() >= m_maximum || m_lastPeer == UINT64_MAX)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::unique_ptr<NetSenderLocalPeer> peer(new NetSenderLocalPeer);
        peer->m_pid = pid;
        peer->m_activity = peer->m_ackTime = peer->m_pingTime = m_now;
        if (m_options.entropy(m_options.entropyContext, peer->m_local, 16) != 0)
        {
            return NI_INVALID_STATE;
        }
        uint8_t nonzero = 0;
        for (uint32_t index = 0; index < 16; ++index)
        {
            nonzero |= peer->m_local[index];
        }
        if (nonzero == 0)
        {
            return NI_INVALID_STATE;
        }
        if (remote != nullptr)
        {
            std::memcpy(peer->m_remote, remote, 16);
            peer->m_remoteKnown = true;
        }
        const int32_t opened = peer->m_sender.open(pid, m_pid, 16440);
        if (opened != NET_SENDER_IPC_ACCEPTED)
        {
            return opened == NET_SENDER_IPC_TOO_LARGE ? NI_BUFFER_TOO_SMALL : NI_NOT_FOUND;
        }
        if (m_rawListener != nullptr)
        {
            peer->m_rawDecoder.reset(new NetInterfaceTunnel());
        }
        else
        {
            peer->m_connection.reset(new NetInterfaceConnection(m_options, NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT + 32));
            if (peer->m_connection->status() < 0)
            {
                return peer->m_connection->status();
            }
        }
        const uint64_t next = ++m_lastPeer;
        m_peers.insert(std::make_pair(next, std::move(peer)));
        const int32_t allowed = m_inbox->allowGeneration(m_peers.find(next)->second->m_local, pid, next, remote);
        if (allowed != NI_OK) { m_peers.erase(next); return allowed; }
        id = next;
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetSenderLocalState::accept(NetSenderLocalInput& input)
{
    // 收件箱失败是本地可信通知，不伪造END，也不把一个通道的过载扩大到全箱。
    if (input.m_reason < 0)
    {
        const auto found = m_peers.find(input.m_peer);
        if (found != m_peers.end() && found->second->m_pid == input.m_pid)
        {
            close(input.m_peer, input.m_reason);
        }
        return;
    }
    NetSenderLocalPacketView packet = {};
    if (NetSenderLocalPacket::parse(input.m_bytes.data(), static_cast<uint32_t>(input.m_bytes.size()), packet) != NI_OK)
    {
        return;
    }
    uint64_t id = 0;
    if (input.m_peer != 0)
    {
        const auto found = m_peers.find(input.m_peer);
        if (found == m_peers.end() || found->second->m_pid != input.m_pid || found->second->m_phase == NET_LOCAL_CLOSED ||
            (packet.m_kind == NET_LOCAL_OPEN ? !found->second->m_remoteKnown ||
            !NetSenderLocalPacket::sameGeneration(found->second->m_remote, packet.m_source) :
            !NetSenderLocalPacket::sameGeneration(found->second->m_local, packet.m_target)))
        {
            return;
        }
        id = found->first;
    }
    else
    {
        // OPEN尚无本地目标号，只有这条握手路径需要寻找既有远端代次。
        for (const auto& entry : m_peers)
        {
            const NetSenderLocalPeer& peer = *entry.second;
            if (peer.m_pid == input.m_pid && peer.m_phase != NET_LOCAL_CLOSED && peer.m_remoteKnown &&
                NetSenderLocalPacket::sameGeneration(peer.m_remote, packet.m_source))
            {
                id = entry.first;
                break;
            }
        }
    }
    if (id == 0)
    {
        if (m_options.server != 1 || packet.m_kind != NET_LOCAL_OPEN || packet.m_packet != 1 ||
            m_peers.size() >= m_maximum)
        {
            return;
        }
        if (m_now - m_acceptWindow >= 1000)
        {
            m_acceptWindow = m_now;
            m_acceptCount = 0;
        }
        if (m_acceptCount >= 16)
        {
            return;
        }
        ++m_acceptCount;
        if (createPeer(input.m_pid, packet.m_source, id) != NI_OK)
        {
            return;
        }
    }
    NetSenderLocalPeer& peer = *m_peers.find(id)->second;
    if (!peer.m_remoteKnown)
    {
        std::memcpy(peer.m_remote, packet.m_source, 16);
        peer.m_remoteKnown = true;
    }
    else if (!NetSenderLocalPacket::sameGeneration(peer.m_remote, packet.m_source))
    {
        return;
    }
    if (packet.m_kind == NET_LOCAL_ACK)
    {
        const uint32_t before = peer.m_window.outstanding();
        if (peer.m_window.acknowledge(packet.m_packet) != NI_OK)
        {
            close(id, NI_PROTOCOL_ERROR);
        }
        else if (peer.m_window.outstanding() < before)
        {
            peer.m_ackTime = m_now;
        }
        return;
    }
    if (packet.m_kind == NET_LOCAL_END)
    {
        peer.drain(m_now, true);
        return;
    }
    if (peer.m_phase == NET_LOCAL_ENDED)
    {
        close(id, NI_TRUNCATED);
        return;
    }
    if (packet.m_packet <= peer.m_window.receipt())
    {
        peer.m_ackPending = peer.m_window.receipt();
        return;
    }
    if (peer.m_input.size() >= 4 || peer.m_window.receipt() > UINT64_MAX - peer.m_input.size() - 1 ||
        packet.m_packet != peer.m_window.receipt() + peer.m_input.size() + 1)
    {
        close(id, NI_PROTOCOL_ERROR);
        return;
    }
    NetSenderLocalChunk chunk;
    chunk.m_packet = packet.m_packet;
    // 输入已从收件箱转移到owner，连同信封一起转交通道，保留正文起点。
    // 避免每个IPC片再次分配和复制密文；后续解码仍只消费正文，不借用临时input。
    chunk.m_offset = static_cast<uint32_t>(packet.m_data - input.m_bytes.data());
    chunk.m_bytes.swap(input.m_bytes);
    try
    {
        peer.m_input.push_back(std::move(chunk));
    }
    catch (...)
    {
        close(id, NI_NO_MEMORY);
    }
}

int32_t NetSenderLocalState::transmit(NetSenderLocalPeer& peer, uint32_t kind,
    uint64_t number, const uint8_t* data, uint32_t size)
{
    NetSenderLocalPacketView packet = {};
    packet.m_kind = kind;
    packet.m_packet = number;
    std::memcpy(packet.m_source, peer.m_local, 16);
    if (peer.m_remoteKnown)
    {
        std::memcpy(packet.m_target, peer.m_remote, 16);
    }
    packet.m_data = data;
    packet.m_size = size;
    uint8_t bytes[16440];
    uint32_t required = 0;
    if (NetSenderLocalPacket::encode(packet, bytes, sizeof(bytes), required) != NI_OK)
    {
        return NET_SENDER_IPC_INVALID;
    }
    return peer.m_sender.trySend(bytes, required, NET_SENDER_LOCAL_MESSAGE);
}

void NetSenderLocalState::sendFailure(uint64_t id, NetSenderLocalPeer& peer, int32_t result)
{
    if (result == NET_SENDER_IPC_UNAVAILABLE)
    {
        // 这只证明反向接收入口关闭；本方向先前提交的END仍可能等待原IPC回调。
        peer.drain(m_now, false);
    }
    else if (result != NET_SENDER_IPC_ACCEPTED && result != NET_SENDER_IPC_BUSY)
    {
        close(id, NI_INVALID_STATE);
    }
}

void NetSenderLocalState::pump(uint64_t id, NetSenderLocalPeer& peer)
{
    if (peer.m_phase == NET_LOCAL_CLOSED)
    {
        return;
    }
    if (peer.m_phase == NET_LOCAL_ACTIVE && peer.m_sender.flushNotifications() < 0)
    {
        peer.drain(m_now, false);
    }
    if (m_rawListener != nullptr)
    {
        pumpRaw(id, peer);
    }
    else
    {
        pumpTls(id, peer);
    }
    if (peer.m_phase == NET_LOCAL_CLOSED || m_stopping)
    {
        return;
    }
    // 先交付已进入本层的完整输入，再检查期限；END不能被反向失败或未回ACK覆盖。
    if (peer.m_phase != NET_LOCAL_ACTIVE)
    {
        if (m_now - peer.m_drainSince >= 5000)
        {
            close(id, peer.m_phase == NET_LOCAL_ENDED ? NI_TIMEOUT : NI_TRUNCATED);
        }
    }
    else if (peer.m_window.outstanding() != 0 && m_now - peer.m_ackTime >= 5000)
    {
        close(id, NI_TIMEOUT);
    }
}

void NetSenderLocalState::pumpTls(uint64_t id, NetSenderLocalPeer& peer)
{
    uint32_t outputBudget = 65536;
    for (uint32_t step = 0; step < 64 && peer.m_phase != NET_LOCAL_CLOSED && !m_stopping; ++step)
    {
        bool moved = false;
        bool blocked = false;
        if (!peer.m_input.empty())
        {
            NetSenderLocalChunk& chunk = peer.m_input.front();
            uint32_t used = 0;
            const int32_t fed = peer.m_connection->feed(chunk.m_bytes.data() + chunk.m_offset,
                static_cast<uint32_t>(chunk.m_bytes.size()) - chunk.m_offset, used);
            if (fed < 0)
            {
                close(id, fed);
                break;
            }
            chunk.m_offset += used;
            moved = used != 0;
            if (chunk.m_offset == chunk.m_bytes.size())
            {
                if (peer.m_window.received(chunk.m_packet) != NI_OK)
                {
                    close(id, NI_PROTOCOL_ERROR);
                    break;
                }
                peer.m_ackPending = peer.m_window.receipt();
                peer.m_input.pop_front();
            }
        }
        if (peer.m_phase == NET_LOCAL_ENDED && peer.m_input.empty())
        {
            peer.m_connection->finishInput();
        }
        const int32_t state = peer.m_connection->progress(m_now);
        if (state < 0 || state == NET_INTERFACE_TLS_CLOSED)
        {
            close(id, state < 0 ? state : NI_OK);
            break;
        }
        if (state == NET_INTERFACE_TLS_READY)
        {
            if (!peer.m_ready)
            {
                peer.m_ready = true;
                peer.m_activity = peer.m_pingTime = m_now;
                m_listener->onLocalReady(id, peer.m_pid);
                moved = true;
            }
            if (peer.m_phase == NET_LOCAL_CLOSED || m_stopping)
            {
                break;
            }
            NetInterfaceFrame frame = {};
            if (peer.m_connection->peek(frame) == NI_READY)
            {
                bool consume = true;
                if (frame.m_kind == NI_PING || frame.m_kind == NI_PONG)
                {
                    NetInterfaceFrame reply = {};
                    const int32_t control = NetInterfaceRpc::control(frame, reply);
                    if (control < 0 || (frame.m_kind == NI_PONG && frame.m_requestId != peer.m_pingId))
                    {
                        close(id, NI_PROTOCOL_ERROR);
                        break;
                    }
                    if (control == NI_READY && peer.m_phase == NET_LOCAL_ACTIVE && peer.m_connection->send(reply) != NI_OK)
                    {
                        close(id, NI_LIMIT_EXCEEDED);
                        break;
                    }
                    if (frame.m_kind == NI_PONG)
                    {
                        peer.m_pingId = 0;
                    }
                }
                else
                {
                    consume = m_listener->onLocalFrame(id, frame);
                }
                if (consume)
                {
                    peer.m_connection->consume();
                    peer.m_activity = m_now;
                    moved = true;
                }
                else
                {
                    blocked = true;
                }
            }
        }
        if (peer.m_phase == NET_LOCAL_CLOSED || m_stopping)
        {
            break;
        }
        if (peer.m_ackPending != 0 && peer.m_remoteKnown && peer.m_phase == NET_LOCAL_ACTIVE)
        {
            const int32_t sent = transmit(peer, NET_LOCAL_ACK, peer.m_ackPending, nullptr, 0);
            if (sent == NET_SENDER_IPC_ACCEPTED)
            {
                peer.m_ackPending = 0;
                moved = true;
            }
            else if (sent != NET_SENDER_IPC_BUSY)
            {
                sendFailure(id, peer, sent);
                break;
            }
        }
        const uint64_t next = peer.m_window.next();
        const uint8_t* output = nullptr;
        uint32_t size = 0;
        peer.m_connection->peekOutput(output, size);
        size = (std::min)((std::min)(size, 16384U), outputBudget);
        if (next != 0 && size != 0 && peer.m_phase == NET_LOCAL_ACTIVE)
        {
            const int32_t sent = transmit(peer, peer.m_remoteKnown ? NET_LOCAL_DATA : NET_LOCAL_OPEN, next, output, size);
            if (sent == NET_SENDER_IPC_ACCEPTED)
            {
                if (peer.m_window.outstanding() == 0)
                {
                    peer.m_ackTime = m_now;
                }
                peer.m_window.published(next);
                peer.m_connection->consumeOutput(size);
                outputBudget -= size;
                moved = true;
            }
            else if (sent != NET_SENDER_IPC_BUSY)
            {
                sendFailure(id, peer, sent);
                break;
            }
        }
        if (blocked || !moved || outputBudget == 0)
        {
            break;
        }
    }
    if (peer.m_phase == NET_LOCAL_ACTIVE && peer.m_ready && !m_stopping)
    {
        if (m_now - peer.m_activity >= 15000 || (peer.m_pingId != 0 && m_now - peer.m_pingTime >= 10000))
        {
            close(id, NI_TIMEOUT);
        }
        else if (peer.m_pingId == 0 && m_now - peer.m_pingTime >= 5000)
        {
            if (m_lastPing == UINT64_MAX)
            {
                close(id, NI_LIMIT_EXCEEDED);
                return;
            }
            NetInterfaceFrame ping = {};
            ping.m_kind = NI_PING;
            ping.m_requestId = ++m_lastPing;
            if (peer.m_connection->send(ping) != NI_OK)
            {
                close(id, NI_LIMIT_EXCEEDED);
                return;
            }
            peer.m_pingTime = m_now;
            peer.m_pingId = ping.m_requestId;
        }
    }
}

void NetSenderLocalState::close(uint64_t id, int32_t reason)
{
    const auto found = m_peers.find(id);
    if (found == m_peers.end() || found->second->m_phase == NET_LOCAL_CLOSED)
    {
        return;
    }
    NetSenderLocalPeer& peer = *found->second;
    const bool notify = peer.m_phase == NET_LOCAL_ACTIVE && peer.m_remoteKnown;
    peer.m_phase = NET_LOCAL_CLOSED;
    peer.m_reason = reason;
    if (notify)
    {
        transmit(peer, NET_LOCAL_END, 0, nullptr, 0);
    }
}

void NetSenderLocalState::collect()
{
    // 回调可能shutdown并关闭先前仍活动的通道，须在返回false前收敛这些终态。
    // 每次先移除再通知；本实例最多64条，运行中不允许重新configure，整批有界。
    for (uint32_t index = 0; index < 64; ++index)
    {
        auto entry = m_peers.begin();
        while (entry != m_peers.end() && entry->second->m_phase != NET_LOCAL_CLOSED)
        {
            ++entry;
        }
        if (entry == m_peers.end())
        {
            break;
        }
        const uint64_t id = entry->first;
        const int32_t reason = entry->second->m_reason;
        m_inbox->removeGeneration(entry->second->m_local);
        m_peers.erase(entry);
        try
        {
            if (m_rawListener != nullptr)
            {
                m_rawListener->onRawLocalClosed(id, reason);
            }
            else
            {
                m_listener->onLocalClosed(id, reason);
            }
        }
        catch (...)
        {
            // 已经移除本代资源；业务回调异常不得触发重复终态。
        }
    }
}

bool NetSenderLocalState::poll()
{
    if (m_polling || !m_started || m_stopping || (m_listener == nullptr && m_rawListener == nullptr))
    {
        return m_started && !m_stopping && (m_listener != nullptr || m_rawListener != nullptr);
    }
    m_polling = true;
    m_now = now();
    try
    {
        for (uint32_t index = 0; index < 64; ++index)
        {
            NetSenderLocalInput input;
            const int32_t received = m_inbox->take(input);
            if (received < 0)
            {
                shutdown();
                break;
            }
            if (received != NI_READY)
            {
                break;
            }
            accept(input);
        }
        if (m_now - m_processCheckAt >= 500)
        {
            m_processCheckAt = m_now;
            for (const auto& entry : m_peers)
            {
                if (entry.second->m_phase == NET_LOCAL_ACTIVE &&
                    CSystem::processDefinitelyExited(static_cast<uint32_t>(entry.second->m_pid)))
                {
                    entry.second->drain(m_now, false);
                }
            }
        }
        for (const auto& entry : m_peers)
        {
            try
            {
                pump(entry.first, *entry.second);
            }
            catch (...)
            {
                close(entry.first, NI_INVALID_STATE);
            }
        }
    }
    catch (...)
    {
        shutdown();
    }
    collect();
    m_polling = false;
    return m_started && !m_stopping;
}

void NetSenderLocalState::shutdown()
{
    m_stopping = true;
    m_inbox->stop();
    for (const auto& entry : m_peers)
    {
        close(entry.first, NI_CANCELLED);
    }
    if (!m_polling)
    {
        m_polling = true;
        collect();
        m_polling = false;
    }
}