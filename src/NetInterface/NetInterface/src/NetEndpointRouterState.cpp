#include "NetEndpointRouterState.h"
#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceRequests.h"
#include "NetInterfaceRpc.h"
#include "NetInterfaceFile.h"
#include "NetInterfaceEvent.h"
#include "NetInterfaceTransport.h"
#include "NetEndpointRouter.h"
#include <chrono>
#include <cstring>
#include <vector>
#include <utility>
#include <algorithm>
#include <ctime>

void NetEndpointRouterState::exchangeFile(uint32_t left, uint32_t right)
{
    std::swap(m_fileExpiry[left], m_fileExpiry[right]);
    m_fileExpiry[left]->m_expiryIndex = left;
    m_fileExpiry[right]->m_expiryIndex = right;
}

void NetEndpointRouterState::riseFile(uint32_t index)
{
    while (index != 0)
    {
        const uint32_t parent = (index - 1) / 2;
        if (m_fileExpiry[parent]->m_deadline <= m_fileExpiry[index]->m_deadline)
        {
            break;
        }
        exchangeFile(parent, index);
        index = parent;
    }
}

void NetEndpointRouterState::sinkFile(uint32_t index)
{
    while (index * 2 + 1 < m_fileExpiryCount)
    {
        uint32_t child = index * 2 + 1;
        if (child + 1 < m_fileExpiryCount && m_fileExpiry[child + 1]->m_deadline < m_fileExpiry[child]->m_deadline)
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

void NetEndpointRouterState::attachFile(NetEndpointFileStream* stream)
{
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
        ++endpoint->m_fileCount;
    }
    stream->m_expiryIndex = m_fileExpiryCount;
    m_fileExpiry[m_fileExpiryCount++] = stream;
    riseFile(stream->m_expiryIndex);
}

std::unique_ptr<NetEndpointFileStream> NetEndpointRouterState::takeFile(uint64_t id)
{
    const auto found = m_streams.find(id);
    if (found == m_streams.end())
    {
        return std::unique_ptr<NetEndpointFileStream>();
    }
    std::unique_ptr<NetEndpointFileStream> stream = std::move(found->second);
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
        --endpoint->m_fileCount;
    }
    const uint32_t index = stream->m_expiryIndex;
    --m_fileExpiryCount;
    if (index != m_fileExpiryCount)
    {
        NetEndpointFileStream* replacement = m_fileExpiry[m_fileExpiryCount];
        m_fileExpiry[index] = replacement;
        replacement->m_expiryIndex = index;
        riseFile(index);
        sinkFile(replacement->m_expiryIndex);
    }
    m_fileExpiry[m_fileExpiryCount] = nullptr;
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
m_fileCount(0),
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

NetEndpointRouterState::NetEndpointRouterState(NetEndpointServiceSink* services, NetEndpointServiceSink* clients) :
m_transport(nullptr),
m_entropy(nullptr),
m_entropyContext(nullptr),
m_listening(false),
m_requests(4096),
m_services(services),
m_clients(clients),
m_closingFirst(nullptr),
m_closingLast(nullptr),
m_polling(false),
m_dispatching(false),
m_stopRequested(false),
m_fileExpiryCount(0),
m_lastEndpoint(1),
m_internal(0),
m_generic(0),
m_lastPing(0),
m_lastStream(0),
m_lastEvent(0),
m_status(NI_OK),
m_stopped(false)
{
    std::memset(m_fileExpiry, 0, sizeof(m_fileExpiry));
    m_status = m_requests.addEndpoint(1, true, 0, 128);
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
    if (m_payload.capacity() > 4096)
    {
        std::vector<uint8_t>().swap(m_payload);
    }
}

uint64_t NetEndpointRouterState::now()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}