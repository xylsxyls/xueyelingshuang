#include "NetExecution.h"
#include "NetInterfaceRequests.h"
#include "NetInterfaceMacro.h"
#include "NetInterfaceRequestsState.h"
#include <algorithm>
#include <cstring>

NetInterfaceRequests::NetInterfaceRequests() :
NetInterfaceRequests(NetRequestTablePools())
{

}

NetInterfaceRequests::NetInterfaceRequests(const NetRequestTablePools& pools) :
m_state(new NetInterfaceRequestsState(pools))
{

}

NetInterfaceRequests::NetInterfaceRequests(uint32_t capacity) :
m_state(nullptr)
{
    NetRequestTablePools pools;
    pools.m_requests.initialCapacity = (std::min)(capacity, 128U);
    pools.m_requests.maximumCapacity = capacity;
    pools.m_cancellations.initialCapacity = (std::min)(capacity, 128U);
    pools.m_cancellations.maximumCapacity = capacity;
    m_state = new NetInterfaceRequestsState(pools);
    if (capacity == 0)
    {
        m_state->m_status = NI_INVALID_ARGUMENT;
    }
}

NetInterfaceRequests::~NetInterfaceRequests()
{
    delete m_state;
}

int32_t NetInterfaceRequests::status() const
{
    return m_state->m_status;
}

int32_t NetInterfaceRequests::addEndpoint(uint64_t endpoint, bool service, uint32_t product, uint32_t limit, bool internalOnly)
{
    if (m_state->m_status < 0)
    {
        return m_state->m_status;
    }
    if (endpoint == 0 || endpoint <= m_state->m_lastEndpoint || limit == 0 || (!service && (product != 0 || internalOnly)))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetObjectReference<NetInterfaceRequestEndpoint> entry;
    const int32_t allocated = m_state->m_endpointPool.create(entry);
    if (allocated != NI_OK)
    {
        return allocated;
    }
    try
    {
        entry->m_limit = limit;
        entry->m_service = service;
        entry->m_internalOnly = internalOnly;
        entry->m_product = product;
        m_state->m_endpoints.insert(std::make_pair(endpoint, entry));
        m_state->m_lastEndpoint = endpoint;
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetInterfaceRequests::removeEndpoint(uint64_t endpoint)
{
    const auto found = m_state->m_endpoints.find(endpoint);
    if (found == m_state->m_endpoints.end())
    {
        return NI_NOT_FOUND;
    }
    for (uint32_t role = 0; role < NI_REQUEST_ROLE_COUNT; ++role)
    {
        if (found->second->m_first[role] != nullptr)
        {
            return NI_INVALID_STATE;
        }
    }
    m_state->m_endpoints.erase(found);
    return NI_OK;
}

int32_t NetInterfaceRequests::advance(uint64_t nowMs)
{
    if (m_state->m_status >= 0 && nowMs < m_state->m_lastMs)
    {
        m_state->m_status = NI_INVALID_ARGUMENT;
    }
    m_state->m_lastMs = nowMs;
    return m_state->m_status;
}

int32_t NetInterfaceRequests::begin(uint64_t source, uint64_t original, uint64_t target, uint32_t product, uint32_t type, uint64_t parent,
    uint64_t nowMs, uint32_t timeoutMs, NetInterfaceRequest& request)
{
    std::memset(&request, 0, sizeof(request));
    if (advance(nowMs) < 0)
    {
        return m_state->m_status;
    }
    if (source == target || original == 0 || product == 0 || timeoutMs == 0 || timeoutMs > 60000 || nowMs > UINT64_MAX - timeoutMs)
    {
        return NI_INVALID_ARGUMENT;
    }
    const auto from = m_state->m_endpoints.find(source);
    const auto to = m_state->m_endpoints.find(target);
    if (from == m_state->m_endpoints.end() || to == m_state->m_endpoints.end() || !to->second->m_service ||
        (to->second->m_product != 0 && to->second->m_product != product) || (to->second->m_internalOnly && parent == 0) ||
        original <= from->second->m_highestOriginal)
    {
        return NI_PROTOCOL_ERROR;
    }
    NetInterfaceRequest value = {};
    value.m_source = source;
    value.m_original = original;
    value.m_target = target;
    value.m_origin = source;
    value.m_parent = parent;
    value.m_deadline = nowMs + timeoutMs;
    value.m_product = product;
    value.m_type = type;
    if (parent != 0)
    {
        const auto ancestor = m_state->m_requests.find(parent);
        if (!from->second->m_service || ancestor == m_state->m_requests.end() || ancestor->second->m_request.m_target != source ||
            ancestor->second->m_request.m_parent != 0 || ancestor->second->m_request.m_product != product)
        {
            return NI_PROTOCOL_ERROR;
        }
        if (ancestor->second->m_request.m_deadline <= nowMs)
        {
            return NI_TIMEOUT;
        }
        value.m_origin = ancestor->second->m_request.m_origin;
        value.m_deadline = (std::min)(value.m_deadline, ancestor->second->m_request.m_deadline);
    }
    from->second->m_highestOriginal = original;
    const bool cancelled = from->second->m_futureCancelled.find(original) != from->second->m_futureCancelled.end();
    while (!from->second->m_futureCancelled.empty() && from->second->m_futureCancelled.begin()->first <= original)
    {
        from->second->m_futureCancelled.erase(from->second->m_futureCancelled.begin());
    }
    if (cancelled)
    {
        return NI_CANCELLED;
    }
    if (m_state->m_lastRoute == UINT64_MAX)
    {
        return NI_LIMIT_EXCEEDED;
    }
    value.m_route = m_state->m_lastRoute + 1;
    NetInterfaceRequestEndpoint* origin = m_state->m_endpoints.find(value.m_origin)->second.get();
    const int32_t inserted = m_state->insert(value, from->second.get(), to->second.get(), origin);
    if (inserted == NI_OK)
    {
        m_state->m_lastRoute = value.m_route;
        request = value;
    }
    return inserted;
}

int32_t NetInterfaceRequests::parentContext(uint64_t target, uint64_t route, uint32_t product, NetInterfaceRequest& request) const
{
    std::memset(&request, 0, sizeof(request));
    const auto endpoint = m_state->m_endpoints.find(target);
    const auto parent = m_state->m_requests.find(route);
    if (endpoint == m_state->m_endpoints.end() || !endpoint->second->m_service || parent == m_state->m_requests.end() ||
        parent->second->m_request.m_target != target || parent->second->m_request.m_parent != 0 || parent->second->m_request.m_product != product)
    {
        return NI_NOT_FOUND;
    }
    request = parent->second->m_request;
    return NI_READY;
}

int32_t NetInterfaceRequests::setEndpointLimit(uint64_t endpoint, uint32_t limit)
{
    const auto found = m_state->m_endpoints.find(endpoint);
    if (found == m_state->m_endpoints.end())
    {
        return NI_NOT_FOUND;
    }
    if (limit == 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    found->second->m_limit = limit;
    return NI_OK;
}

int32_t NetInterfaceRequests::beginNext(uint64_t source, uint64_t target, uint32_t product, uint32_t type, uint64_t parent, uint64_t nowMs,
    uint32_t timeoutMs, NetInterfaceRequest& request)
{
    std::memset(&request, 0, sizeof(request));
    const auto endpoint = m_state->m_endpoints.find(source);
    if (endpoint == m_state->m_endpoints.end())
    {
        return NI_INVALID_ARGUMENT;
    }
    if (endpoint->second->m_highestOriginal == UINT64_MAX)
    {
        return NI_LIMIT_EXCEEDED;
    }
    return begin(source, endpoint->second->m_highestOriginal + 1, target, product, type, parent, nowMs, timeoutMs, request);
}

int32_t NetInterfaceRequests::take(uint64_t route, NetInterfaceRequest& request)
{
    std::memset(&request, 0, sizeof(request));
    const auto found = m_state->m_requests.find(route);
    if (found == m_state->m_requests.end())
    {
        return NI_NOT_FOUND;
    }
    return m_state->take(found->second.get(), request);
}

int32_t NetInterfaceRequests::complete(uint64_t target, uint64_t route, uint32_t product, uint32_t type, uint64_t nowMs,
    NetInterfaceRequest& request)
{
    std::memset(&request, 0, sizeof(request));
    if (advance(nowMs) < 0)
    {
        return m_state->m_status;
    }
    const auto found = m_state->m_requests.find(route);
    if (found == m_state->m_requests.end())
    {
        return NI_NOT_FOUND;
    }
    if (found->second->m_request.m_target != target || found->second->m_request.m_product != product || found->second->m_request.m_type != type)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (nowMs >= found->second->m_request.m_deadline)
    {
        take(route, request);
        return NI_TIMEOUT;
    }
    if (found->second->m_request.m_parent != 0 && m_state->m_requests.find(found->second->m_request.m_parent) == m_state->m_requests.end())
    {
        take(route, request);
        return NI_CANCELLED;
    }
    return take(route, request);
}

int32_t NetInterfaceRequests::completeOriginal(uint64_t source, uint64_t original, uint64_t target, uint32_t product, uint32_t type,
    uint64_t nowMs, NetInterfaceRequest& request)
{
    std::memset(&request, 0, sizeof(request));
    if (advance(nowMs) < 0)
    {
        return m_state->m_status;
    }
    const auto endpoint = m_state->m_endpoints.find(source);
    if (endpoint == m_state->m_endpoints.end())
    {
        return NI_NOT_FOUND;
    }
    const auto found = endpoint->second->m_originals.find(original);
    return found == endpoint->second->m_originals.end() ? NI_NOT_FOUND :
        complete(target, found->second->m_request.m_route, product, type, nowMs, request);
}

int32_t NetInterfaceRequests::cancel(uint64_t source, uint64_t original, NetInterfaceRequest& request)
{
    std::memset(&request, 0, sizeof(request));
    const auto endpoint = m_state->m_endpoints.find(source);
    if (original == 0 || endpoint == m_state->m_endpoints.end())
    {
        return NI_INVALID_ARGUMENT;
    }
    const auto found = endpoint->second->m_originals.find(original);
    if (found != endpoint->second->m_originals.end())
    {
        return m_state->take(found->second, request);
    }
    if (original <= endpoint->second->m_highestOriginal)
    {
        return NI_NOT_FOUND;
    }
    if (endpoint->second->m_futureCancelled.find(original) != endpoint->second->m_futureCancelled.end())
    {
        return NI_OK;
    }
    NetObjectReference<uint64_t> cancellation;
    const int32_t allocated = m_state->m_cancelPool.create(cancellation, original);
    if (allocated != NI_OK)
    {
        return allocated;
    }
    try
    {
        endpoint->second->m_futureCancelled.insert(std::make_pair(original, cancellation));
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetInterfaceRequests::takeExpired(uint64_t nowMs, NetInterfaceRequest& request)
{
    std::memset(&request, 0, sizeof(request));
    if (advance(nowMs) < 0)
    {
        return m_state->m_status;
    }
    if (!m_state->m_expiry.empty())
    {
        NetExecution::deadline(m_state->m_expiry.front()->m_request.m_deadline, nowMs);
    }
    if (m_state->m_expiry.empty() || m_state->m_expiry.front()->m_request.m_deadline > nowMs)
    {
        return NI_NOT_FOUND;
    }
    return m_state->take(m_state->m_expiry.front(), request);
}

int32_t NetInterfaceRequests::takeChild(uint64_t parent, NetInterfaceRequest& request)
{
    std::memset(&request, 0, sizeof(request));
    const auto found = m_state->m_children.find(parent);
    return found == m_state->m_children.end() ? NI_NOT_FOUND : m_state->take(found->second.m_first, request);
}

int32_t NetInterfaceRequests::takeEndpoint(uint64_t endpoint, NetInterfaceRequest& request)
{
    std::memset(&request, 0, sizeof(request));
    const auto found = m_state->m_endpoints.find(endpoint);
    if (found == m_state->m_endpoints.end())
    {
        return NI_NOT_FOUND;
    }
    NetInterfaceRequestEntry* first = nullptr;
    for (uint32_t role = 0; role < NI_REQUEST_ROLE_COUNT; ++role)
    {
        NetInterfaceRequestEntry* entry = found->second->m_first[role];
        if (entry != nullptr && (first == nullptr || entry->m_request.m_route < first->m_request.m_route))
        {
            first = entry;
        }
    }
    return first == nullptr ? NI_NOT_FOUND : m_state->take(first, request);
}

uint32_t NetInterfaceRequests::count() const
{
    return static_cast<uint32_t>((std::min)(m_state->m_requests.size(), static_cast<size_t>(UINT32_MAX)));
}