#include "NetEndpointRouterState.h"
#include "NetInterface.h"
#include <chrono>
#include <cstring>
#include <vector>
#include <utility>

void NetEndpointRouterState::exchangeFile(size_t left, size_t right)
{
    std::swap(m_fileExpiry[left], m_fileExpiry[right]);
    m_fileExpiry[left]->m_expiryIndex = left;
    m_fileExpiry[right]->m_expiryIndex = right;
}

void NetEndpointRouterState::riseFile(size_t index)
{
    while (index != 0)
    {
        const size_t parent = (index - 1) / 2;
        if (m_fileExpiry[parent]->m_deadline <= m_fileExpiry[index]->m_deadline)
        {
            break;
        }
        exchangeFile(parent, index);
        index = parent;
    }
}

void NetEndpointRouterState::sinkFile(size_t index)
{
    const size_t count = m_fileExpiry.size();
    while (index < count / 2)
    {
        size_t child = index * 2 + 1;
        if (child + 1 < count && m_fileExpiry[child + 1]->m_deadline < m_fileExpiry[child]->m_deadline)
        {
            ++child;
        }
        if (m_fileExpiry[index]->m_deadline <= m_fileExpiry[child]->m_deadline)
        {
            break;
        }
        exchangeFile(index, child);
        index = child;
    }
}

int32_t NetEndpointRouterState::attachFile(NetEndpointFileStream* stream)
{
    try
    {
        m_fileExpiry.push_back(stream);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
    const uint64_t ids[2] = {stream->m_client, stream->m_service};
    for (uint32_t role = 0; role < 2; ++role)
    {
        NetEndpointPeer* endpoint = m_endpoints.find(ids[role])->second.get();
        stream->m_next[role] = endpoint->m_firstFile;
        if (endpoint->m_firstFile != nullptr)
        {
            endpoint->m_firstFile->m_previous[role] = stream;
        }
        endpoint->m_firstFile = stream;
    }
    stream->m_expiryIndex = m_fileExpiry.size() - 1;
    riseFile(stream->m_expiryIndex);
    return NI_OK;
}

NetObjectReference<NetEndpointFileStream> NetEndpointRouterState::takeFile(uint64_t id)
{
    const auto found = m_streams.find(id);
    if (found == m_streams.end())
    {
        return NetObjectReference<NetEndpointFileStream>();
    }
    NetObjectReference<NetEndpointFileStream> stream = std::move(found->second);
    m_streams.erase(found);
    m_serviceStreams.erase(std::make_pair(stream->m_service, stream->m_local));
    const uint64_t ids[2] = {stream->m_client, stream->m_service};
    for (uint32_t role = 0; role < 2; ++role)
    {
        NetEndpointPeer* endpoint = m_endpoints.find(ids[role])->second.get();
        if (stream->m_previous[role] != nullptr)
        {
            stream->m_previous[role]->m_next[role] = stream->m_next[role];
        }
        else
        {
            endpoint->m_firstFile = stream->m_next[role];
        }
        if (stream->m_next[role] != nullptr)
        {
            stream->m_next[role]->m_previous[role] = stream->m_previous[role];
        }
    }
    const size_t index = stream->m_expiryIndex;
    NetEndpointFileStream* replacement = m_fileExpiry.back();
    m_fileExpiry.pop_back();
    if (index != m_fileExpiry.size())
    {
        m_fileExpiry[index] = replacement;
        replacement->m_expiryIndex = index;
        riseFile(index);
        sinkFile(replacement->m_expiryIndex);
    }
    return stream;
}

void NetEndpointRouterState::renewFile(NetEndpointFileStream* stream, uint64_t deadline)
{
    stream->m_deadline = deadline;
    riseFile(stream->m_expiryIndex);
    sinkFile(stream->m_expiryIndex);
}

NetEndpointPeer::NetEndpointPeer() :
m_id(0),
m_nextClosing(nullptr),
m_filesCollected(false),
m_firstFile(nullptr),
m_service(false),
m_attached(false),
m_closing(false),
m_peer(0),
m_product(0),
m_activity(0),
m_pingTime(0),
m_pingId(0)
{
    std::memset(&m_origin, 0, sizeof(m_origin));
    std::memset(m_negotiated, 0, sizeof(m_negotiated));
}

NetEndpointRouterState::NetEndpointRouterState(NetEndpointServiceSink* services, NetEndpointServiceSink* clients, const NetEndpointPools& pools) :
m_transport(nullptr),
m_entropy(nullptr),
m_entropyContext(nullptr),
m_handshake(),
m_listening(false),
m_requests(pools.m_requests),
m_services(services),
m_clients(clients),
m_endpointPool(pools.m_endpoints),
m_filePool(pools.m_files),
m_closingFirst(nullptr),
m_closingLast(nullptr),
m_payloadRetentionBytes(pools.m_payloadRetentionBytes),
m_expiredRequestBudget(pools.m_expiredRequestBudget),
m_closingEndpointBudget(pools.m_closingEndpointBudget),
m_expiredFileBudget(pools.m_expiredFileBudget),
m_heartbeat(pools.m_heartbeat),
m_polling(false),
m_dispatching(false),
m_stopRequested(false),
m_lastEndpoint(1),
m_internal(0),
m_generic(0),
m_lastPing(0),
m_lastStream(0),
m_lastEvent(0),
m_status(NI_OK),
m_stopped(false)
{
    m_handshake.m_server = 1;
    m_handshake.m_capabilities = 7;
    m_handshake.m_maxRequests = 32;
    m_handshake.m_maxChunk = 65536;
    m_handshake.m_timeoutMs = 5000;
    m_status = (m_payloadRetentionBytes < -1 || !m_heartbeat.valid() || !m_expiredRequestBudget ||
        !m_closingEndpointBudget || !m_expiredFileBudget) ? NI_INVALID_ARGUMENT : m_endpointPool.status();
    if (m_status == NI_OK) { m_status = m_filePool.status(); }
    if (m_status == NI_OK) { m_status = m_requests.status(); }
    if (m_status == NI_OK) { m_status = m_requests.addEndpoint(1, true, 0, 128); }
}

void NetEndpointRouterState::enqueueClosing(NetEndpointPeer* peer)
{
    peer->m_nextClosing = nullptr;
    if (m_closingLast == nullptr)
    {
        m_closingFirst = peer;
    }
    else
    {
        m_closingLast->m_nextClosing = peer;
    }
    m_closingLast = peer;
}

NetEndpointPeer* NetEndpointRouterState::takeClosing()
{
    NetEndpointPeer* peer = m_closingFirst;
    if (peer != nullptr)
    {
        m_closingFirst = peer->m_nextClosing;
        if (m_closingFirst == nullptr)
        {
            m_closingLast = nullptr;
        }
        peer->m_nextClosing = nullptr;
    }
    return peer;
}

void NetEndpointRouterState::clearPayload()
{
    if (!m_payload.empty())
    {
        NetInterface::wipe(m_payload.data(), static_cast<uint32_t>(m_payload.size()));
        m_payload.clear();
    }
    if (m_payloadRetentionBytes >= 0 && m_payload.capacity() > static_cast<uint64_t>(m_payloadRetentionBytes))
    {
        std::vector<uint8_t>().swap(m_payload);
    }
}

uint64_t NetEndpointRouterState::now()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}