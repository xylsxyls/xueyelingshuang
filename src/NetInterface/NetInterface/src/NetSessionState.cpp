#include "NetSessionState.h"
#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetClientDiscovery.h"
#include "NetInterfaceRequests.h"
#include "NetInterfaceRpc.h"
#include "NetInterfaceTransport.h"
#include "NetSession.h"
#include <chrono>
#include <cstring>
#include <vector>

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