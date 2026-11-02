#include "NetClientGateway.h"
#include <chrono>
#include <cstring>
#include <algorithm>

NetClientGatewayPeer::NetClientGatewayPeer(uint64_t now) :
m_deadline(now + 5000),
m_highest(0),
m_limit(0),
m_pending(0),
m_chunk(0),
m_ready(false)
{

}

NetClientGatewayFile::NetClientGatewayFile(uint64_t peer, uint32_t product, const NetInterfaceFileInfo& info) :
m_peer(peer),
m_product(product),
m_info(info)
{

}

NetClientGatewayFile::~NetClientGatewayFile()
{
    netInterfaceWipe(reinterpret_cast<uint8_t*>(&m_info), sizeof(m_info));
}

NetClientGateway::NetClientGateway() :
m_local(m_inbox),
m_retry(0),
m_queuedBytes(0),
m_remoteLimit(0),
m_remoteChunk(0),
m_receiving(false),
m_started(false),
m_stopping(false)
{
    std::memset(&m_options, 0, sizeof(m_options));
}

NetClientGateway::~NetClientGateway()
{
    shutdown();
}

int32_t NetClientGateway::Entropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return NetTransport::randomBytes(output, size) == NI_OK ? 0 : -1;
}

uint64_t NetClientGateway::Now()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

int32_t NetClientGateway::init(const NetInterfaceTlsOptions& localTls, const NetWorkClientOptions& remote)
{
    if (m_started || m_stopping || remote.structSize != sizeof(remote) || remote.version != 1 || remote.platform != 1 ||
        remote.address == nullptr || remote.tls.hostname == nullptr || remote.tls.authority == nullptr || remote.tls.authoritySize == 0 ||
        remote.tls.authoritySize > 131072 || remote.tls.server != 0 || localTls.server != 1)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        m_options = remote;
        m_address = remote.address;
        m_hostname = remote.tls.hostname;
        m_authority.assign(remote.tls.authority, remote.tls.authority + remote.tls.authoritySize);
        m_options.address = m_address.c_str();
        m_options.tls.hostname = m_hostname.c_str();
        m_options.tls.authority = m_authority.data();
        m_options.tls.entropy = &Entropy;
        m_options.tls.entropyContext = nullptr;
        auto tls = localTls;
        tls.entropy = &Entropy;
        tls.entropyContext = nullptr;
        ProcessWork& work = ProcessWork::instance();
        if (work.receiveBufferCount() != 0)
        {
            return NI_INVALID_STATE;
        }
        work.addProcessReceiveCallback(&m_inbox);
        if (!work.initReceive(16452, 64))
        {
            work.uninitReceive();
            m_local.shutdown();
            return NI_INVALID_STATE;
        }
        m_receiving = true;
        // 本地接收器先就绪，失败收敛只作用于刚取得的自有资源。
        const int32_t listening = m_local.listen(tls, this, 32);
        if (listening != NI_OK)
        {
            shutdown();
            return listening;
        }
        m_started = true;
        const int32_t connected = connectRemote();
        if (connected != NI_OK)
        {
            shutdown();
            return connected;
        }
        return NI_OK;
    }
    catch (...)
    {
        shutdown();
        return NI_NO_MEMORY;
    }
}

int32_t NetClientGateway::connectRemote()
{
    NetInterfaceHandshakeOptions options = {};
    options.m_platform = 1;
    options.m_capabilities = m_options.capabilities;
    options.m_maxRequests = m_options.maxRequests;
    options.m_maxChunk = m_options.maxChunk;
    options.m_timeoutMs = m_options.handshakeTimeoutMs;
    std::memcpy(options.m_device, m_options.device, 16);
    const int32_t detached = m_remote.bindChannel(nullptr);
    if (detached != NI_OK)
    {
        return detached;
    }
    try
    {
        m_remoteTransport.reset(new NetTransport());
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
    const int32_t bound = m_remote.bindChannel(m_remoteTransport.get());
    return bound == NI_OK ? m_remote.connect(m_options.address, m_options.port, m_options.tls, options, this) : bound;
}

bool NetClientGateway::poll()
{
    if (!m_started || m_stopping)
    {
        return false;
    }
    const bool running = m_remote.poll();
    const uint64_t now = Now();
    if (!running && now >= m_retry)
    {
        connectRemote();
        m_retry = now + 1000;
    }
    m_local.poll();
    for (uint32_t index = 0; index < 16 && !m_fileCancels.empty() && m_remote.ready(); ++index)
    {
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_CANCEL;
        frame.m_messageType = NI_FILE_DATA_TYPE;
        frame.m_productId = m_fileCancels.front().first;
        frame.m_requestId = m_fileCancels.front().second;
        const int32_t result = m_remote.sendData(frame);
        if (result == NI_LIMIT_EXCEEDED)
        {
            break;
        }
        m_fileCancels.pop_front();
        if (result != NI_OK)
        {
            m_remote.close(result);
        }
    }
    for (const auto& peer : m_peers)
    {
        if (!peer.second->m_ready && now >= peer.second->m_deadline)
        {
            m_local.close(peer.first, NI_TIMEOUT);
        }
    }
    uint32_t attempts = 0;
    for (auto entry = m_requests.begin(); entry != m_requests.end() && attempts < 64;)
    {
        const uint64_t id = entry->first;
        auto current = entry++;
        if (current->second.m_sent)
        {
            continue;
        }
        ++attempts;
        const auto& body = current->second.m_body;
        const int32_t sent = m_remote.sendRequest(id, body.empty() ? nullptr : body.data(), static_cast<uint32_t>(body.size()));
        if (sent == NI_LIMIT_EXCEEDED)
        {
            break;
        }
        if (sent == NI_OK)
        {
            current->second.m_sent = true;
            m_queuedBytes -= static_cast<uint32_t>(body.size());
            netInterfaceWipe(current->second.m_body.empty() ? nullptr : current->second.m_body.data(),
                             static_cast<uint32_t>(current->second.m_body.size()));
            current->second.m_body.clear();
        }
        else if (sent != NI_TIMEOUT)
        {
            m_local.close(current->second.m_peer, sent);
            m_remote.cancel(id);
        }
    }
    return !m_stopping;
}

void NetClientGateway::onLocalReady(uint64_t peer, int32_t pid)
{
    (void)pid;
    if (!m_remote.ready() || m_stopping)
    {
        m_local.close(peer, NI_INVALID_STATE);
        return;
    }
    try
    {
        m_peers.insert(std::make_pair(peer, std::shared_ptr<NetClientGatewayPeer>(new NetClientGatewayPeer(Now()))));
    }
    catch (...)
    {
        m_local.close(peer, NI_NO_MEMORY);
    }
}

bool NetClientGateway::onLocalFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    const auto found = m_peers.find(peer);
    if (found == m_peers.end() || !m_remote.ready() || m_stopping)
    {
        m_local.close(peer, NI_INVALID_STATE);
        return true;
    }
    auto& client = *found->second;
    const uint64_t now = Now();
    if (!client.m_handshake)
    {
        NetInterfaceHandshakeOptions options = {};
        options.m_server = 1;
        options.m_capabilities = m_options.capabilities;
        options.m_maxRequests = (std::min)(m_remoteLimit, 16U);
        options.m_maxChunk = m_remoteChunk;
        options.m_timeoutMs = 5000;
        if (NetTransport::randomBytes(options.m_nonce, 16) != NI_OK)
        {
            m_local.close(peer, NI_INVALID_STATE);
            return true;
        }
        client.m_handshake.reset(new NetInterfaceHandshake(options, now));
    }
    const int32_t accepted = client.m_handshake->accept(frame, now);
    if (accepted < 0 || (!client.m_ready && now >= client.m_deadline))
    {
        m_local.close(peer, accepted < 0 ? accepted : NI_TIMEOUT);
        return true;
    }
    if (!client.m_ready)
    {
        NetInterfaceFrame reply = {};
        if (client.m_handshake->peekOutput(reply) == NI_READY)
        {
            const int32_t result = m_local.send(peer, reply);
            if (result != NI_OK)
            {
                m_local.close(peer, result);
                return true;
            }
            client.m_handshake->consumeOutput();
        }
        uint32_t fields[4] = {};
        uint8_t nonce[16] = {};
        uint8_t device[16] = {};
        if (client.m_handshake->peerInfo(fields, nonce, device) != NI_READY || fields[0] != 1)
        {
            m_local.close(peer, NI_PROTOCOL_ERROR);
            return true;
        }
        client.m_limit = fields[2];
        client.m_chunk = fields[3];
        client.m_ready = true;
        return true;
    }
    if (frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK || (frame.m_kind == NI_CANCEL && frame.m_messageType == NI_FILE_DATA_TYPE))
    {
        const auto file = m_files.find(frame.m_requestId);
        if (file == m_files.end())
        {
            return true;
        }
        if (file->second->m_peer != peer || file->second->m_product != frame.m_productId)
        {
            m_local.close(peer, NI_PROTOCOL_ERROR);
            return true;
        }
        bool valid = true;
        if (frame.m_kind == NI_DATA)
        {
            NetInterfaceFileData body = {};
            valid = file->second->m_info.direction == 1 && NetInterfaceFile::parseData(frame, body) == NI_OK &&
                    NetInterfaceFile::equal(body.token, file->second->m_info.token, 32) && body.size <= file->second->m_info.chunk;
        }
        else if (frame.m_kind == NI_DATA_ACK)
        {
            NetInterfaceFileAck ack = {};
            valid = file->second->m_info.direction == 2 && NetInterfaceFile::parseAck(frame, ack) == NI_OK &&
                    NetInterfaceFile::equal(ack.token, file->second->m_info.token, 32);
        }
        else
        {
            NetInterfaceFrame unused = {};
            valid = NetInterfaceRpc::control(frame, unused) == NI_OK;
        }
        if (!valid)
        {
            m_local.close(peer, NI_PROTOCOL_ERROR);
            return true;
        }
        const int32_t sent = m_remote.sendData(frame);
        if (sent == NI_LIMIT_EXCEEDED)
        {
            return false;
        }
        if (sent != NI_OK)
        {
            m_local.close(peer, sent);
        }
        if (frame.m_kind == NI_CANCEL || (frame.m_kind == NI_DATA_ACK && frame.m_payloadSize >= 8 && frame.m_payload[7] != 0))
        {
            m_files.erase(file);
        }
        return true;
    }
    if (frame.m_kind == NI_PING || frame.m_kind == NI_PONG)
    {
        NetInterfaceFrame reply = {};
        const int32_t status = NetInterfaceRpc::control(frame, reply);
        if (status == NI_READY)
        {
            const int32_t sent = m_local.send(peer, reply);
            if (sent == NI_LIMIT_EXCEEDED)
            {
                return false;
            }
            if (sent != NI_OK)
            {
                m_local.close(peer, sent);
            }
        }
        else if (status != NI_OK)
        {
            m_local.close(peer, status);
        }
        return true;
    }
    if (frame.m_kind == NI_CANCEL)
    {
        NetInterfaceFrame unused = {};
        if (NetInterfaceRpc::control(frame, unused) != NI_OK)
        {
            m_local.close(peer, NI_PROTOCOL_ERROR);
            return true;
        }
        for (const auto& entry : m_requests)
        {
            if (entry.second.m_peer == peer && entry.second.m_local.m_requestId == frame.m_requestId)
            {
                if (entry.second.m_local.m_productId != frame.m_productId || entry.second.m_local.m_messageType != frame.m_messageType)
                {
                    m_local.close(peer, NI_PROTOCOL_ERROR);
                }
                else
                {
                    m_remote.cancel(entry.first);
                }
                return true;
            }
        }
        if (frame.m_requestId > client.m_highest)
        {
            if (client.m_cancelled.size() >= client.m_limit)
            {
                m_local.close(peer, NI_LIMIT_EXCEEDED);
            }
            else
            {
                client.m_cancelled[frame.m_requestId] = std::make_pair(frame.m_productId, frame.m_messageType);
            }
        }
        return true;
    }
    NetInterfaceRpcView rpc = {};
    if (frame.m_kind != NI_REQUEST || NetInterfaceRpc::parse(frame, rpc) != NI_OK || rpc.m_parent != 0 ||
        frame.m_requestId <= client.m_highest)
    {
        m_local.close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    client.m_highest = frame.m_requestId;
    if (client.m_products.size() >= 32 && client.m_products.find(frame.m_productId) == client.m_products.end())
    {
        respond(peer, frame, NI_RPC_BUSY);
        return true;
    }
    client.m_products.insert(frame.m_productId);
    const auto cancelled = client.m_cancelled.find(frame.m_requestId);
    const bool skip = cancelled != client.m_cancelled.end();
    if (skip && cancelled->second != std::make_pair(frame.m_productId, frame.m_messageType))
    {
        m_local.close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    while (!client.m_cancelled.empty() && client.m_cancelled.begin()->first <= frame.m_requestId)
    {
        client.m_cancelled.erase(client.m_cancelled.begin());
    }
    if (skip)
    {
        respond(peer, frame, NI_RPC_CANCELLED);
        return true;
    }
    if (client.m_pending >= client.m_limit || m_requests.size() >= 128 || rpc.m_size > 2097152U - m_queuedBytes)
    {
        respond(peer, frame, NI_RPC_BUSY);
        return true;
    }
    uint64_t remoteId = 0;
    const int32_t begin = m_remote.beginRequest(frame.m_productId, frame.m_messageType, rpc.m_value, remoteId);
    if (begin != NI_OK)
    {
        respond(peer, frame, begin == NI_LIMIT_EXCEEDED ? NI_RPC_BUSY : NI_RPC_UNAVAILABLE);
        return true;
    }
    try
    {
        NetClientGatewayRequest request = {};
        request.m_local = frame;
        request.m_local.m_payload = nullptr;
        request.m_local.m_payloadSize = 0;
        request.m_peer = peer;
        request.m_sent = false;
        if (rpc.m_size != 0)
        {
            request.m_body.assign(rpc.m_data, rpc.m_data + rpc.m_size);
        }
        m_requests.insert(std::make_pair(remoteId, std::move(request)));
        m_queuedBytes += rpc.m_size;
        ++client.m_pending;
    }
    catch (...)
    {
        m_remote.cancel(remoteId);
        respond(peer, frame, NI_RPC_BUSY);
    }
    return true;
}

void NetClientGateway::respond(uint64_t peer, const NetInterfaceFrame& original, uint32_t status, const uint8_t* body, uint32_t size)
{
    try
    {
        std::vector<uint8_t> payload(size + 16);
        uint32_t required = 0;
        const int32_t result =
            NetInterfaceRpc::encode(NI_RESPONSE, status, 0, body, size, payload.data(), static_cast<uint32_t>(payload.size()), required);
        NetInterfaceFrame frame = original;
        frame.m_kind = NI_RESPONSE;
        frame.m_payload = payload.data();
        frame.m_payloadSize = required;
        const int32_t sent = result == NI_OK ? m_local.send(peer, frame) : result;
        netInterfaceWipe(payload.data(), static_cast<uint32_t>(payload.size()));
        if (sent != NI_OK)
        {
            m_local.close(peer, sent);
        }
    }
    catch (...)
    {
        m_local.close(peer, NI_NO_MEMORY);
    }
}

void NetClientGateway::onLocalClosed(uint64_t peer, int32_t reason)
{
    (void)reason;
    int32_t cancelError = NI_OK;
    for (auto item = m_files.begin(); item != m_files.end();)
    {
        if (item->second->m_peer != peer)
        {
            ++item;
            continue;
        }
        if (m_fileCancels.size() < 32 && cancelError == NI_OK)
        {
            try
            {
                m_fileCancels.push_back(std::make_pair(item->second->m_product, item->first));
            }
            catch (...)
            {
                // 通知入队失败不能中断本地清理；全部摘除后再关闭远端会话。
                cancelError = NI_NO_MEMORY;
            }
        }
        else if (cancelError == NI_OK)
        {
            cancelError = NI_LIMIT_EXCEEDED;
        }
        item = m_files.erase(item);
    }
    for (auto entry = m_requests.begin(); entry != m_requests.end();)
    {
        if (entry->second.m_peer != peer)
        {
            ++entry;
            continue;
        }
        const uint64_t id = entry->first;
        m_queuedBytes -= static_cast<uint32_t>(entry->second.m_body.size());
        netInterfaceWipe(entry->second.m_body.empty() ? nullptr : entry->second.m_body.data(),
                         static_cast<uint32_t>(entry->second.m_body.size()));
        entry = m_requests.erase(entry);
        m_remote.cancel(id);
    }
    m_peers.erase(peer);
    if (cancelError != NI_OK)
    {
        // 关闭可同步清空文件表，必须在遍历和请求结算结束后执行。
        m_remote.close(cancelError);
    }
}

void NetClientGateway::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    (void)generation;
    m_remoteLimit = maxRequests;
    m_remoteChunk = maxChunk;
}

void NetClientGateway::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status,
                                       const uint8_t* data, uint32_t size)
{
    if (generation != m_remote.generation())
    {
        return;
    }
    const auto found = m_requests.find(requestId);
    if (found == m_requests.end())
    {
        return;
    }
    const auto request = found->second.m_local;
    const uint64_t peer = found->second.m_peer;
    m_queuedBytes -= static_cast<uint32_t>(found->second.m_body.size());
    netInterfaceWipe(found->second.m_body.empty() ? nullptr : found->second.m_body.data(),
                     static_cast<uint32_t>(found->second.m_body.size()));
    m_requests.erase(found);
    const auto client = m_peers.find(peer);
    if (client != m_peers.end() && client->second->m_pending != 0)
    {
        --client->second->m_pending;
    }
    if (product != request.m_productId || type != request.m_messageType)
    {
        m_local.close(peer, NI_PROTOCOL_ERROR);
    }
    else
    {
        if (status == NI_RPC_SUCCESS && NetInterfaceFile::isControl(type))
        {
            NetInterfaceFileInfo info = {};
            if (NetInterfaceFile::parseInfo(data, size, info) != NI_OK || client == m_peers.end() || info.chunk > client->second->m_chunk)
            {
                m_local.close(peer, NI_PROTOCOL_ERROR);
                return;
            }
            if (info.state == NI_FILE_TRANSFERRING)
            {
                const auto current = m_files.find(info.stream);
                if (current != m_files.end() && (current->second->m_peer != peer || current->second->m_product != product ||
                                                 !NetInterfaceFile::equal(current->second->m_info.token, info.token, 32)))
                {
                    m_remote.close(NI_PROTOCOL_ERROR);
                    return;
                }
                if (current == m_files.end())
                {
                    if (m_files.size() >= 32)
                    {
                        m_local.close(peer, NI_LIMIT_EXCEEDED);
                        return;
                    }
                    try
                    {
                        m_files.insert(std::make_pair(
                            info.stream, std::shared_ptr<NetClientGatewayFile>(new NetClientGatewayFile(peer, product, info))));
                    }
                    catch (...)
                    {
                        m_local.close(peer, NI_NO_MEMORY);
                        return;
                    }
                }
            }
        }
        respond(peer, request, status, data, size);
    }
}

bool NetClientGateway::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    if (generation != m_remote.generation())
    {
        return true;
    }
    if (frame.m_kind == NI_DATA || frame.m_kind == NI_DATA_ACK || (frame.m_kind == NI_EVENT && frame.m_messageType == NI_FILE_CANCEL))
    {
        const auto file = m_files.find(frame.m_requestId);
        if (file == m_files.end())
        {
            return true;
        }
        if (file->second->m_product != frame.m_productId)
        {
            m_remote.close(NI_PROTOCOL_ERROR);
            return true;
        }
        const int32_t sent = m_local.send(file->second->m_peer, frame);
        if (sent == NI_LIMIT_EXCEEDED)
        {
            return false;
        }
        if (sent != NI_OK)
        {
            m_local.close(file->second->m_peer, sent);
        }
        if (frame.m_kind == NI_EVENT || (frame.m_kind == NI_DATA_ACK && frame.m_payloadSize >= 8 && frame.m_payload[7] != 0))
        {
            m_files.erase(file);
        }
        return true;
    }
    if (frame.m_kind != NI_EVENT)
    {
        m_remote.close(NI_PROTOCOL_ERROR);
        return true;
    }
    for (const auto& peer : m_peers)
    {
        if (peer.second->m_ready &&
            (frame.m_productId == 0 || peer.second->m_products.find(frame.m_productId) != peer.second->m_products.end()))
        {
            const int32_t sent = m_local.send(peer.first, frame);
            if (sent != NI_OK)
            {
                m_local.close(peer.first, sent);
            }
        }
    }
    return true;
}

void NetClientGateway::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)generation;
    (void)detail;
    m_remoteLimit = 0;
    m_remoteChunk = 0;
    m_files.clear();
    m_fileCancels.clear();
    for (const auto& peer : m_peers)
    {
        m_local.close(peer.first, reason == 0 ? NI_INVALID_STATE : reason);
    }
    m_retry = Now() + 1000;
}

void NetClientGateway::shutdown()
{
    if (m_stopping)
    {
        return;
    }
    m_stopping = true;
    m_remote.shutdown();
    m_local.shutdown();
    if (m_receiving)
    {
        ProcessWork::instance().uninitReceive();
        m_receiving = false;
    }
    m_started = false;
}

bool NetClientGateway::ready() const
{
    return !m_stopping && m_remote.ready();
}