#include "NetInterfaceMacro.h"
#include "NetInterfaceRequests.h"
#include "NetInterfaceRequestsState.h"
#include <algorithm>
#include <cstring>

NetInterfaceRequestEndpoint::NetInterfaceRequestEndpoint() :
m_limit(0),
m_product(0),
m_service(false),
m_internalOnly(false),
m_highestOriginal(0)
{
    for (uint32_t role = 0; role < NI_REQUEST_ROLE_COUNT; ++role)
    {
        m_count[role] = 0;
        m_first[role] = nullptr;
        m_last[role] = nullptr;
    }
}

NetInterfaceRequestEntry::NetInterfaceRequestEntry(const NetInterfaceRequest& request) :
m_request(request),
m_previousChild(nullptr),
m_nextChild(nullptr),
m_heapIndex(0)
{
    for (uint32_t role = 0; role < NI_REQUEST_ROLE_COUNT; ++role)
    {
        m_endpoint[role] = nullptr;
        m_previous[role] = nullptr;
        m_next[role] = nullptr;
    }
}

NetInterfaceRequestChildren::NetInterfaceRequestChildren() :
m_first(nullptr),
m_last(nullptr)
{

}

NetInterfaceRequestsState::NetInterfaceRequestsState(uint32_t capacity) :
m_lastRoute(0),
m_lastEndpoint(0),
m_lastMs(0),
m_capacity(capacity),
m_futureCount(0),
m_status(capacity > 0 && capacity <= 4096 ? NI_OK : NI_INVALID_ARGUMENT)
{

}

bool NetInterfaceRequestsState::earlier(const NetInterfaceRequestEntry* left, const NetInterfaceRequestEntry* right)
{
    return left->m_request.m_deadline < right->m_request.m_deadline ||
        (left->m_request.m_deadline == right->m_request.m_deadline && left->m_request.m_route < right->m_request.m_route);
}

void NetInterfaceRequestsState::rise(size_t index)
{
    NetInterfaceRequestEntry* value = m_expiry[index];
    while (index != 0)
    {
        const size_t parent = (index - 1) / 2;
        if (!earlier(value, m_expiry[parent]))
        {
            break;
        }
        m_expiry[index] = m_expiry[parent];
        m_expiry[index]->m_heapIndex = index;
        index = parent;
    }
    m_expiry[index] = value;
    value->m_heapIndex = index;
}

void NetInterfaceRequestsState::sink(size_t index)
{
    NetInterfaceRequestEntry* value = m_expiry[index];
    const size_t count = m_expiry.size();
    while (index * 2 + 1 < count)
    {
        size_t child = index * 2 + 1;
        if (child + 1 < count && earlier(m_expiry[child + 1], m_expiry[child]))
        {
            ++child;
        }
        if (!earlier(m_expiry[child], value))
        {
            break;
        }
        m_expiry[index] = m_expiry[child];
        m_expiry[index]->m_heapIndex = index;
        index = child;
    }
    m_expiry[index] = value;
    value->m_heapIndex = index;
}

int32_t NetInterfaceRequestsState::insert(const NetInterfaceRequest& request, NetInterfaceRequestEndpoint* source,
    NetInterfaceRequestEndpoint* target, NetInterfaceRequestEndpoint* origin)
{
    bool newChildren = false;
    NetInterfaceRequestChildren* children = nullptr;
    NetInterfaceRequestEntry* entry = nullptr;
    const std::pair<uint64_t, uint64_t> key(request.m_source, request.m_original);
    try
    {
        // 所有可能分配的位置在提交额度之前完成；堆只存指针，最多32KiB（64位）。
        if (m_expiry.capacity() < m_capacity)
        {
            m_expiry.reserve(m_capacity);
        }
        if (request.m_parent != 0)
        {
            const auto inserted = m_children.insert(std::make_pair(request.m_parent, NetInterfaceRequestChildren()));
            children = &inserted.first->second;
            newChildren = inserted.second;
        }
        const auto inserted = m_requests.insert(std::make_pair(request.m_route, NetInterfaceRequestEntry(request)));
        if (!inserted.second)
        {
            if (newChildren)
            {
                m_children.erase(request.m_parent);
            }
            return NI_INVALID_STATE;
        }
        entry = &inserted.first->second;
        if (!m_sources.insert(std::make_pair(key, entry)).second)
        {
            m_requests.erase(request.m_route);
            if (newChildren)
            {
                m_children.erase(request.m_parent);
            }
            return NI_INVALID_STATE;
        }
    }
    catch (...)
    {
        // 还未连入任何非拥有索引，不需要回调或回滚端点计数。
        if (entry != nullptr)
        {
            m_requests.erase(request.m_route);
        }
        if (newChildren)
        {
            m_children.erase(request.m_parent);
        }
        return NI_NO_MEMORY;
    }
    entry->m_endpoint[NI_REQUEST_SOURCE] = source;
    entry->m_endpoint[NI_REQUEST_TARGET] = target;
    entry->m_endpoint[NI_REQUEST_ORIGIN] = origin;
    for (uint32_t role = 0; role < NI_REQUEST_ROLE_COUNT; ++role)
    {
        NetInterfaceRequestEndpoint* endpoint = entry->m_endpoint[role];
        entry->m_previous[role] = endpoint->m_last[role];
        if (endpoint->m_last[role] == nullptr)
        {
            endpoint->m_first[role] = entry;
        }
        else
        {
            endpoint->m_last[role]->m_next[role] = entry;
        }
        endpoint->m_last[role] = entry;
        ++endpoint->m_count[role];
    }
    if (children != nullptr)
    {
        entry->m_previousChild = children->m_last;
        if (children->m_last == nullptr)
        {
            children->m_first = entry;
        }
        else
        {
            children->m_last->m_nextChild = entry;
        }
        children->m_last = entry;
    }
    entry->m_heapIndex = m_expiry.size();
    // 容量和指针元素均已准备，追加不会再分配，也没有用户代码可抛异常。
    m_expiry.push_back(entry);
    rise(entry->m_heapIndex);
    return NI_OK;
}

int32_t NetInterfaceRequestsState::take(NetInterfaceRequestEntry* entry, NetInterfaceRequest& request)
{
    request = entry->m_request;
    const size_t index = entry->m_heapIndex;
    NetInterfaceRequestEntry* replacement = m_expiry.back();
    m_expiry.pop_back();
    if (index < m_expiry.size())
    {
        m_expiry[index] = replacement;
        replacement->m_heapIndex = index;
        if (index != 0 && earlier(replacement, m_expiry[(index - 1) / 2]))
        {
            rise(index);
        }
        else
        {
            sink(index);
        }
    }
    for (uint32_t role = 0; role < NI_REQUEST_ROLE_COUNT; ++role)
    {
        NetInterfaceRequestEndpoint* endpoint = entry->m_endpoint[role];
        if (entry->m_previous[role] == nullptr)
        {
            endpoint->m_first[role] = entry->m_next[role];
        }
        else
        {
            entry->m_previous[role]->m_next[role] = entry->m_next[role];
        }
        if (entry->m_next[role] == nullptr)
        {
            endpoint->m_last[role] = entry->m_previous[role];
        }
        else
        {
            entry->m_next[role]->m_previous[role] = entry->m_previous[role];
        }
        --endpoint->m_count[role];
    }
    if (request.m_parent != 0)
    {
        const auto found = m_children.find(request.m_parent);
        if (entry->m_previousChild == nullptr)
        {
            found->second.m_first = entry->m_nextChild;
        }
        else
        {
            entry->m_previousChild->m_nextChild = entry->m_nextChild;
        }
        if (entry->m_nextChild == nullptr)
        {
            found->second.m_last = entry->m_previousChild;
        }
        else
        {
            entry->m_nextChild->m_previousChild = entry->m_previousChild;
        }
        if (found->second.m_first == nullptr)
        {
            m_children.erase(found);
        }
    }
    m_sources.erase(std::make_pair(request.m_source, request.m_original));
    m_requests.erase(request.m_route);
    return NI_READY;
}

NetInterfaceRequests::NetInterfaceRequests(uint32_t capacity) :
m_state(new NetInterfaceRequestsState(capacity))
{

}

NetInterfaceRequests::~NetInterfaceRequests()
{
    delete m_state;
}

int32_t NetInterfaceRequests::addEndpoint(uint64_t endpoint, bool service, uint32_t product, uint32_t limit, bool internalOnly)
{
    if (m_state->m_status < 0)
    {
        return m_state->m_status;
    }
    if (endpoint == 0 || endpoint <= m_state->m_lastEndpoint || limit == 0 || limit > 128 || (!service && (product != 0 || internalOnly)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_state->m_endpoints.size() >= 1024)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        NetInterfaceRequestEndpoint entry = {};
        entry.m_limit = limit;
        entry.m_service = service;
        entry.m_internalOnly = internalOnly;
        entry.m_product = product;
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
        if (found->second.m_first[role] != nullptr)
        {
            return NI_INVALID_STATE;
        }
    }
    m_state->m_futureCount -= static_cast<uint32_t>(found->second.m_futureCancelled.size());
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
    if (from == m_state->m_endpoints.end() || to == m_state->m_endpoints.end() || !to->second.m_service ||
        (to->second.m_product != 0 && to->second.m_product != product) || (to->second.m_internalOnly && parent == 0) ||
        original <= from->second.m_highestOriginal)
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
        if (!from->second.m_service || ancestor == m_state->m_requests.end() || ancestor->second.m_request.m_target != source ||
            ancestor->second.m_request.m_parent != 0 || ancestor->second.m_request.m_product != product)
        {
            return NI_PROTOCOL_ERROR;
        }
        if (ancestor->second.m_request.m_deadline <= nowMs)
        {
            return NI_TIMEOUT;
        }
        value.m_origin = ancestor->second.m_request.m_origin;
        value.m_deadline = (std::min)(value.m_deadline, ancestor->second.m_request.m_deadline);
    }
    from->second.m_highestOriginal = original;
    const bool cancelled = from->second.m_futureCancelled.find(original) != from->second.m_futureCancelled.end();
    while (!from->second.m_futureCancelled.empty() && *from->second.m_futureCancelled.begin() <= original)
    {
        from->second.m_futureCancelled.erase(from->second.m_futureCancelled.begin());
        --m_state->m_futureCount;
    }
    if (cancelled)
    {
        return NI_CANCELLED;
    }
    if (m_state->m_requests.size() >= m_state->m_capacity || from->second.m_count[NI_REQUEST_SOURCE] >= from->second.m_limit ||
        to->second.m_count[NI_REQUEST_TARGET] >= to->second.m_limit || m_state->m_lastRoute == UINT64_MAX)
    {
        return NI_LIMIT_EXCEEDED;
    }
    value.m_route = ++m_state->m_lastRoute;
    NetInterfaceRequestEndpoint* origin = &m_state->m_endpoints.find(value.m_origin)->second;
    const int32_t inserted = m_state->insert(value, &from->second, &to->second, origin);
    if (inserted == NI_OK)
    {
        request = value;
    }
    return inserted;
}

int32_t NetInterfaceRequests::parentContext(uint64_t target, uint64_t route, uint32_t product, NetInterfaceRequest& request) const
{
    std::memset(&request, 0, sizeof(request));
    const auto endpoint = m_state->m_endpoints.find(target);
    const auto parent = m_state->m_requests.find(route);
    if (endpoint == m_state->m_endpoints.end() || !endpoint->second.m_service || parent == m_state->m_requests.end() ||
        parent->second.m_request.m_target != target || parent->second.m_request.m_parent != 0 || parent->second.m_request.m_product != product)
    {
        return NI_NOT_FOUND;
    }
    request = parent->second.m_request;
    return NI_READY;
}

int32_t NetInterfaceRequests::setEndpointLimit(uint64_t endpoint, uint32_t limit)
{
    const auto found = m_state->m_endpoints.find(endpoint);
    if (found == m_state->m_endpoints.end())
    {
        return NI_NOT_FOUND;
    }
    if (limit == 0 || limit > 128 || found->second.m_count[NI_REQUEST_TARGET] > limit || found->second.m_count[NI_REQUEST_SOURCE] > limit ||
        found->second.m_futureCancelled.size() > limit)
    {
        return NI_INVALID_ARGUMENT;
    }
    found->second.m_limit = limit;
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
    if (endpoint->second.m_highestOriginal == UINT64_MAX)
    {
        return NI_LIMIT_EXCEEDED;
    }
    return begin(source, endpoint->second.m_highestOriginal + 1, target, product, type, parent, nowMs, timeoutMs, request);
}

int32_t NetInterfaceRequests::take(uint64_t route, NetInterfaceRequest& request)
{
    std::memset(&request, 0, sizeof(request));
    const auto found = m_state->m_requests.find(route);
    if (found == m_state->m_requests.end())
    {
        return NI_NOT_FOUND;
    }
    return m_state->take(&found->second, request);
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
    if (found->second.m_request.m_target != target || found->second.m_request.m_product != product || found->second.m_request.m_type != type)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (nowMs >= found->second.m_request.m_deadline)
    {
        take(route, request);
        return NI_TIMEOUT;
    }
    if (found->second.m_request.m_parent != 0 && m_state->m_requests.find(found->second.m_request.m_parent) == m_state->m_requests.end())
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
    const auto found = m_state->m_sources.find(std::make_pair(source, original));
    return found == m_state->m_sources.end() ? NI_NOT_FOUND : complete(target, found->second->m_request.m_route, product, type, nowMs, request);
}

int32_t NetInterfaceRequests::cancel(uint64_t source, uint64_t original, NetInterfaceRequest& request)
{
    std::memset(&request, 0, sizeof(request));
    const auto endpoint = m_state->m_endpoints.find(source);
    if (original == 0 || endpoint == m_state->m_endpoints.end())
    {
        return NI_INVALID_ARGUMENT;
    }
    const auto found = m_state->m_sources.find(std::make_pair(source, original));
    if (found != m_state->m_sources.end())
    {
        return m_state->take(found->second, request);
    }
    if (original <= endpoint->second.m_highestOriginal)
    {
        return NI_NOT_FOUND;
    }
    if (endpoint->second.m_futureCancelled.find(original) != endpoint->second.m_futureCancelled.end())
    {
        return NI_OK;
    }
    if (endpoint->second.m_futureCancelled.size() >= endpoint->second.m_limit || m_state->m_futureCount >= m_state->m_capacity)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        endpoint->second.m_futureCancelled.insert(original);
        ++m_state->m_futureCount;
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
        NetInterfaceRequestEntry* entry = found->second.m_first[role];
        if (entry != nullptr && (first == nullptr || entry->m_request.m_route < first->m_request.m_route))
        {
            first = entry;
        }
    }
    return first == nullptr ? NI_NOT_FOUND : m_state->take(first, request);
}

uint32_t NetInterfaceRequests::count() const
{
    return static_cast<uint32_t>(m_state->m_requests.size());
}

// NET_INTERFACE_REQUESTS_UNIT_BEGIN
// #include <chrono>
// #include <cstdio>
// #include <cstring>
// #include <vector>
// #if defined(_MSC_VER) && defined(_DEBUG)
// #include <crtdbg.h>
// #endif
//
// int main(int argc, const char* argv[])
// {
//     uint32_t checks = 0;
// #define REQUEST_CHECK(value) { ++checks; if (!(value)) { std::printf("REQUEST_FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #value); return 1; } }
//     NetInterfaceRequests requests(4096);
//     NetInterfaceRequest actual = {};
//     std::vector<NetInterfaceRequest> reference;
//     const bool benchmark = argc == 2 && std::strcmp(argv[1], "--benchmark") == 0;
//     for (uint64_t endpoint = 1; endpoint <= 64; ++endpoint)
//     {
//         REQUEST_CHECK(requests.addEndpoint(endpoint, endpoint > 32, endpoint > 32 ? 4 : 0, 128) == NI_OK);
//     }
//     for (uint32_t index = 0; index < 4096; ++index)
//     {
//         const uint64_t source = index / 128 + 1;
//         const uint32_t deadline = benchmark ? 60000 : (index * 31337U) % 60000 + 1;
//         REQUEST_CHECK(requests.begin(source, index % 128 + 1, source + 32, 4, index, 0, 1000, deadline, actual) == NI_OK);
//         REQUEST_CHECK(actual.m_route != 0 && actual.m_original == index % 128 + 1 && actual.m_deadline == 1000 + deadline);
//         reference.push_back(actual);
//     }
//     REQUEST_CHECK(requests.count() == 4096);
//     if (benchmark)
//     {
//         const auto started = std::chrono::steady_clock::now();
//         for (uint32_t iteration = 0; iteration < 10000; ++iteration)
//         {
//             REQUEST_CHECK(requests.takeExpired(1000, actual) == NI_NOT_FOUND && actual.m_route == 0);
//             REQUEST_CHECK(requests.takeChild(UINT64_MAX, actual) == NI_NOT_FOUND && actual.m_route == 0);
//             REQUEST_CHECK(requests.takeEndpoint(1025, actual) == NI_NOT_FOUND && actual.m_route == 0);
//         }
//         const int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started).count();
//         REQUEST_CHECK(requests.count() == 4096);
//         uint32_t removed = 0;
//         for (uint64_t endpoint = 1; endpoint <= 32; ++endpoint)
//         {
//             while (requests.takeEndpoint(endpoint, actual) == NI_READY)
//             {
//                 REQUEST_CHECK(actual.m_source == endpoint && actual.m_origin == endpoint && actual.m_target == endpoint + 32);
//                 ++removed;
//             }
//         }
//         REQUEST_CHECK(removed == 4096 && requests.count() == 0);
//         std::printf("REQUEST_INDEX_BENCHMARK live=4096 lookups=30000 microseconds=%lld PASS\n", static_cast<long long>(elapsed));
//         return 0;
//     }
//     uint32_t seed = 0x71a359d3;
//     uint32_t remaining = 4096;
//     // 任意位置删除后用独立线性模型找最早截止，覆盖堆上浮、下沉和末项删除。
//     for (uint32_t iteration = 0; iteration < 2048; ++iteration)
//     {
//         seed = seed * 1664525U + 1013904223U;
//         const size_t index = seed % reference.size();
//         if (reference[index].m_route == 0)
//         {
//             continue;
//         }
//         const NetInterfaceRequest expected = reference[index];
//         if (iteration % 2 == 0)
//         {
//             REQUEST_CHECK(requests.cancel(expected.m_source, expected.m_original, actual) == NI_READY);
//         }
//         else
//         {
//             REQUEST_CHECK(requests.completeOriginal(expected.m_source, expected.m_original, expected.m_target,
//                 expected.m_product, expected.m_type, 1000, actual) == NI_READY);
//         }
//         REQUEST_CHECK(actual.m_route == expected.m_route && actual.m_source == expected.m_source && actual.m_type == expected.m_type);
//         reference[index].m_route = 0;
//         REQUEST_CHECK(requests.count() == --remaining);
//     }
//     while (remaining != 0)
//     {
//         size_t selected = reference.size();
//         for (size_t index = 0; index < reference.size(); ++index)
//         {
//             const NetInterfaceRequest& candidate = reference[index];
//             if (candidate.m_route != 0 && (selected == reference.size() ||
//                 candidate.m_deadline < reference[selected].m_deadline ||
//                 (candidate.m_deadline == reference[selected].m_deadline && candidate.m_route < reference[selected].m_route)))
//             {
//                 selected = index;
//             }
//         }
//         REQUEST_CHECK(selected != reference.size());
//         const NetInterfaceRequest expected = reference[selected];
//         REQUEST_CHECK(requests.takeExpired(expected.m_deadline - 1, actual) == NI_NOT_FOUND);
//         REQUEST_CHECK(requests.takeExpired(expected.m_deadline, actual) == NI_READY && actual.m_route == expected.m_route);
//         reference[selected].m_route = 0;
//         REQUEST_CHECK(requests.count() == --remaining);
//     }
//     REQUEST_CHECK(requests.takeExpired(61000, actual) == NI_NOT_FOUND && requests.count() == 0);
//     for (uint64_t endpoint = 1; endpoint <= 64; ++endpoint)
//     {
//         REQUEST_CHECK(requests.removeEndpoint(endpoint) == NI_OK);
//     }
//     REQUEST_CHECK(requests.addEndpoint(64, true, 0, 128) == NI_INVALID_ARGUMENT);
//     REQUEST_CHECK(requests.addEndpoint(65, false, 0, 128) == NI_OK && requests.addEndpoint(66, true, 4, 128) == NI_OK);
//     REQUEST_CHECK(requests.begin(65, 1, 66, 4, 0, 0, 61000, 1, actual) == NI_OK && actual.m_route > 4096);
//     REQUEST_CHECK(requests.takeExpired(61001, actual) == NI_READY && requests.count() == 0);
//
//     NetInterfaceRequests nested(256);
//     REQUEST_CHECK(nested.addEndpoint(1, false, 0, 128) == NI_OK);
//     REQUEST_CHECK(nested.addEndpoint(2, true, 4, 128) == NI_OK);
//     REQUEST_CHECK(nested.addEndpoint(3, true, 0, 128, true) == NI_OK);
//     REQUEST_CHECK(nested.addEndpoint(4, false, 0, 128) == NI_OK);
//     NetInterfaceRequest parents[16] = {};
//     NetInterfaceRequest children[64] = {};
//     for (uint32_t index = 0; index < 16; ++index)
//     {
//         REQUEST_CHECK(nested.begin(index % 2 == 0 ? 1 : 4, index / 2 + 1, 2, 4, index, 0, 1000, 500, parents[index]) == NI_OK);
//         for (uint32_t child = 0; child < 4; ++child)
//         {
//             const uint32_t position = index * 4 + child;
//             REQUEST_CHECK(nested.begin(2, position + 1, 3, 4, position, parents[index].m_route, 1000, 1000, children[position]) == NI_OK);
//             REQUEST_CHECK(children[position].m_origin == parents[index].m_source && children[position].m_deadline == 1500);
//         }
//     }
//     // 父先结束，子链仍能逐个定位；先完成部分子，剩余索引仍保持接纳顺序。
//     for (uint32_t index = 0; index < 16; index += 2)
//     {
//         REQUEST_CHECK(nested.cancel(1, index / 2 + 1, actual) == NI_READY && actual.m_route == parents[index].m_route);
//         REQUEST_CHECK(nested.removeEndpoint(1) == NI_INVALID_STATE);
//         REQUEST_CHECK(nested.complete(3, children[index * 4 + 1].m_route, 4, index * 4 + 1, 1000, actual) == NI_CANCELLED);
//         for (uint32_t child = 0; child < 4; ++child)
//         {
//             if (child == 1)
//             {
//                 continue;
//             }
//             REQUEST_CHECK(nested.takeChild(parents[index].m_route, actual) == NI_READY && actual.m_route == children[index * 4 + child].m_route);
//         }
//         REQUEST_CHECK(nested.takeChild(parents[index].m_route, actual) == NI_NOT_FOUND);
//     }
//     REQUEST_CHECK(nested.removeEndpoint(1) == NI_OK && nested.count() == 40);
//     uint32_t removed = 0;
//     uint64_t previous = 0;
//     while (nested.takeEndpoint(4, actual) == NI_READY)
//     {
//         REQUEST_CHECK(actual.m_origin == 4 && actual.m_route > previous);
//         previous = actual.m_route;
//         ++removed;
//     }
//     REQUEST_CHECK(removed == 40 && nested.count() == 0 && nested.removeEndpoint(4) == NI_OK);
//     REQUEST_CHECK(nested.removeEndpoint(2) == NI_OK && nested.removeEndpoint(3) == NI_OK);
//
//     NetInterfaceRequests future(2);
//     REQUEST_CHECK(future.addEndpoint(1, false, 0, 2) == NI_OK && future.addEndpoint(2, true, 4, 2) == NI_OK);
//     REQUEST_CHECK(future.cancel(1, 1, actual) == NI_OK && future.cancel(1, 1, actual) == NI_OK);
//     REQUEST_CHECK(future.cancel(1, 3, actual) == NI_OK && future.cancel(1, 4, actual) == NI_LIMIT_EXCEEDED);
//     REQUEST_CHECK(future.begin(1, 1, 2, 4, 0, 0, 1000, 1, actual) == NI_CANCELLED && future.count() == 0);
//     REQUEST_CHECK(future.begin(1, 2, 2, 4, 0, 0, 1000, 1, actual) == NI_OK);
//     REQUEST_CHECK(future.begin(1, 3, 2, 4, 0, 0, 1000, 1, actual) == NI_CANCELLED);
//     REQUEST_CHECK(future.cancel(1, 2, actual) == NI_READY && future.count() == 0);
//     REQUEST_CHECK(future.cancel(1, 5, actual) == NI_OK && future.removeEndpoint(1) == NI_OK);
//     REQUEST_CHECK(future.addEndpoint(3, false, 0, 2) == NI_OK && future.cancel(3, 1, actual) == NI_OK && future.cancel(3, 2, actual) == NI_OK);
//     REQUEST_CHECK(future.takeExpired(999, actual) == NI_INVALID_ARGUMENT);
//     REQUEST_CHECK(future.begin(3, 3, 2, 4, 0, 0, 1000, 1, actual) == NI_INVALID_ARGUMENT);
//     REQUEST_CHECK(future.removeEndpoint(3) == NI_OK && future.removeEndpoint(2) == NI_OK);
// #if defined(_MSC_VER) && defined(_DEBUG)
//     // CRT仅在这次同步调用内拒绝第N次分配；各索引提交失败后仍能接纳、取消和移除端点。
//     static int32_t s_failure = -1;
//     /** 仅测试普通分配，不拒绝CRT内部块；返回0模拟本次分配失败
//     @param [in] operation CRT分配操作
//     @param [in] block CRT块类别
//     @return 是否允许分配
//     */
//     const _CRT_ALLOC_HOOK hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
//     {
//         if (operation == _HOOK_ALLOC && block != _CRT_BLOCK && s_failure >= 0)
//         {
//             return s_failure-- == 0 ? 0 : 1;
//         }
//         return 1;
//     };
//     _CrtMemState before = {};
//     _CrtMemState after = {};
//     _CrtMemCheckpoint(&before);
//     for (uint32_t childMode = 0; childMode != 2; ++childMode)
//     {
//         uint32_t failures = 0;
//         bool succeeded = false;
//         for (int32_t failure = 0; failure < 8 && !succeeded; ++failure)
//         {
//             NetInterfaceRequests allocation(16);
//             REQUEST_CHECK(allocation.addEndpoint(1, false, 0, 16) == NI_OK &&
//                 allocation.addEndpoint(2, true, 4, 16) == NI_OK && allocation.addEndpoint(3, true, 0, 16, true) == NI_OK);
//             NetInterfaceRequest parent = {};
//             if (childMode != 0)
//             {
//                 REQUEST_CHECK(allocation.beginNext(1, 2, 4, 7, 0, 1000, 1000, parent) == NI_OK);
//             }
//             const _CRT_ALLOC_HOOK previousHook = _CrtSetAllocHook(hook);
//             s_failure = failure;
//             const int32_t accepted = allocation.beginNext(childMode != 0 ? 2 : 1, childMode != 0 ? 3 : 2,
//                 4, 7, parent.m_route, 1000, 1000, actual);
//             s_failure = -1;
//             _CrtSetAllocHook(previousHook);
//             if (accepted == NI_NO_MEMORY)
//             {
//                 ++failures;
//                 REQUEST_CHECK(actual.m_route == 0 && allocation.count() == childMode);
//                 REQUEST_CHECK(allocation.beginNext(childMode != 0 ? 2 : 1, childMode != 0 ? 3 : 2,
//                     4, 7, parent.m_route, 1000, 1000, actual) == NI_OK && actual.m_original == 2);
//             }
//             else
//             {
//                 REQUEST_CHECK(accepted == NI_OK && actual.m_original == 1);
//                 succeeded = true;
//             }
//             const uint64_t original = actual.m_original;
//             REQUEST_CHECK(allocation.cancel(childMode != 0 ? 2 : 1, original, actual) == NI_READY);
//             if (childMode != 0)
//             {
//                 REQUEST_CHECK(allocation.takeChild(parent.m_route, actual) == NI_NOT_FOUND &&
//                     allocation.cancel(1, 1, actual) == NI_READY);
//             }
//             REQUEST_CHECK(allocation.count() == 0 && allocation.removeEndpoint(1) == NI_OK &&
//                 allocation.removeEndpoint(2) == NI_OK && allocation.removeEndpoint(3) == NI_OK);
//         }
//         REQUEST_CHECK(succeeded && failures >= 3 && failures <= 4);
//     }
//     // 直接销毁仍有父子、期限及取消索引的实例，不先清空来掩盖析构泄漏。
//     for (uint32_t round = 0; round < 512; ++round)
//     {
//         NetInterfaceRequests abandoned(16);
//         REQUEST_CHECK(abandoned.addEndpoint(1, false, 0, 16) == NI_OK &&
//             abandoned.addEndpoint(2, true, 4, 16) == NI_OK && abandoned.addEndpoint(3, true, 0, 16, true) == NI_OK);
//         NetInterfaceRequest parent = {};
//         REQUEST_CHECK(abandoned.beginNext(1, 2, 4, 7, 0, 1000, 1000, parent) == NI_OK &&
//             abandoned.beginNext(2, 3, 4, 7, parent.m_route, 1000, 1000, actual) == NI_OK &&
//             abandoned.cancel(1, 99, actual) == NI_OK);
//     }
//     _CrtMemCheckpoint(&after);
//     REQUEST_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK]);
//     REQUEST_CHECK(before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
// #endif
//     std::printf("NetInterfaceRequests indexed lifetime checks=%u PASS\n", checks);
//     return 0;
// #undef REQUEST_CHECK
// }
// NET_INTERFACE_REQUESTS_UNIT_END