#include "NetInterfaceRequestsState.h"
#include "NetInterfaceMacro.h"
#include "NetInterfaceRequests.h"
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