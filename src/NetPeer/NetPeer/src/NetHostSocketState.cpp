#include "NetHostSocketState.h"
#include "NetPeerMacro.h"
#include "NetHostSocket.h"
#include <cstring>
#include <utility>
#include "NetInterface/NetInterfaceAPI.h"

NetHostFunctions NetHostSocketState::m_default = {};
bool NetHostSocketState::m_configured = false;

NetHostSocketPeer::NetHostSocketPeer() :
m_connected(false),
m_announced(false),
m_inputPending(false),
m_writablePending(false),
m_closing(false),
m_closed(false),
m_paused(false),
m_reason(0),
m_detail(0),
m_inputSize(0)
{
    for (uint32_t index = 0; index != 4; ++index)
    {
        m_events[index].m_next = nullptr;
        m_events[index].m_queued = false;
    }
}

NetHostSocketPeer::~NetHostSocketPeer()
{
    if (m_inputSize != 0)
    {
        NetInterface::wipe(m_input.data(), m_inputSize);
    }
}

NetHostPoolConfig::NetHostPoolConfig()
{
    LockFreeHashMapPoolConfig& config = m_peers;
    config.initialCapacity = 128;
    config.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetHostSocketPeer>>);
    config.directoryCapacity = 128;
    config.lookupCacheCapacity = 0;
}

NetHostSocketState::NetHostSocketState(const NetHostFunctions& functions, const NetHostPoolConfig& pools) :
m_functions(functions),
m_instance(nullptr),
m_listener(nullptr),
m_bodies(new NetBufferPool(pools.m_input)),
m_peerPool(new NetObjectPool<NetHostSocketPeer>(pools.m_peers)),
m_eventBudget(pools.m_eventBudget),
m_eventHead(nullptr),
m_eventTail(nullptr),
m_inputSize(0),
m_lastPeer(0),
m_fault(NI_OK),
m_phase(NET_HOST_IDLE),
m_server(false),
m_polling(false),
m_callback(false)
{

}

bool NetHostSocketState::valid(const NetHostFunctions& functions)
{
    return functions.structSize == sizeof(NetHostFunctions) && functions.version == 1 && functions.create != nullptr &&
        (functions.listen != nullptr || functions.connect != nullptr) && functions.poll != nullptr && functions.write != nullptr &&
        functions.pauseRead != nullptr && functions.close != nullptr && functions.shutdown != nullptr &&
        functions.destroy != nullptr && functions.localPort != nullptr && functions.peerCount != nullptr;
}

bool NetHostSocketState::validListen(const char* address, uint32_t port, uint32_t maxPeers)
{
    return address != nullptr && address[0] != 0 && port <= 65535 && maxPeers != 0;
}

bool NetHostSocketState::validConnect(const char* address, uint32_t port)
{
    return address != nullptr && address[0] != 0 && port > 0 && port <= 65535;
}

bool NetHostSocketState::pending(const NetHostSocket& socket, uint64_t peer, NetHostEventKind kind)
{
    const auto found = socket.m_state->m_peers.find(peer);
    return found != socket.m_state->m_peers.end() && found->second->m_events[kind].m_queued;
}

int32_t NetHostSocketState::start(NetHostSocketListener* listener, bool server)
{
    if (m_phase != NET_HOST_IDLE || m_polling || m_callback)
    {
        return NI_INVALID_STATE;
    }
    if (!m_eventBudget) { return NI_INVALID_ARGUMENT; }
    if (!valid(m_functions) || (server ? m_functions.listen == nullptr : m_functions.connect == nullptr))
    {
        return NI_NOT_SUPPORTED;
    }
    m_phase = NET_HOST_CREATING;
    if (m_peerPool->status() != NI_OK || m_bodies->status() != NI_OK)
    {
        m_phase = NET_HOST_IDLE;
        return m_peerPool->status() != NI_OK ? m_peerPool->status() : m_bodies->status();
    }
    m_server = server;
    m_listener = listener;
    int32_t result = -1;
    try
    {
        result = m_functions.create(m_functions.context, this, connected, data, writable, closed, &m_instance);
    }
    catch (...)
    {
        fail(NI_NO_MEMORY);
    }
    if (result != 0 || m_instance == nullptr || m_fault != NI_OK)
    {
        m_listener = nullptr;
        fail(NI_INVALID_STATE);
        stop();
        return m_fault;
    }
    m_phase = NET_HOST_STARTING;
    return NI_OK;
}

void NetHostSocketState::fail(int32_t reason)
{
    if (m_fault == NI_OK)
    {
        m_fault = reason;
    }
    if (m_phase != NET_HOST_STOPPING && m_phase != NET_HOST_STOPPED)
    {
        m_phase = NET_HOST_STOPPING;
        if (!m_polling)
        {
            NetExecution::progress();
        }
    }
}

bool NetHostSocketState::queue(NetHostEventKind kind, uint64_t peer, int32_t reason, int32_t detail)
{
    auto found = m_peers.find(peer);
    if (found == m_peers.end() || found->second->m_events[kind].m_queued)
    {
        fail(NI_PROTOCOL_ERROR);
        return false;
    }
    NetHostSocketEvent& event = found->second->m_events[kind];
    event.m_kind = kind;
    event.m_peer = peer;
    event.m_reason = reason;
    event.m_detail = detail;
    event.m_next = nullptr;
    event.m_queued = true;
    if (m_eventTail != nullptr)
    {
        m_eventTail->m_next = &event;
    }
    else
    {
        m_eventHead = &event;
        // 上一次 poll 返回后，宿主操作仍可同步入队；执行者此时还没有派发这条事件。
        if (!m_polling)
        {
            NetExecution::progress();
        }
    }
    m_eventTail = &event;
    return true;
}

int32_t NetHostSocketState::addPeer(uint64_t peer)
{
    NetObjectReference<NetHostSocketPeer> record;
    const int32_t allocated = m_peerPool->create(record);
    if (allocated != NI_OK)
    {
        return allocated;
    }
    try
    {
        if (!m_peers.insert(std::make_pair(peer, std::move(record))).second)
        {
            return NI_INVALID_STATE;
        }
        m_lastPeer = peer;
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetHostSocketState::clearInput()
{
    if (m_inputSize != 0)
    {
        NetInterface::wipe(m_input.data(), m_inputSize);
        m_inputSize = 0;
        m_input.reset();
    }
}

void NetHostSocketState::connected(void* context, uint64_t peer)
{
    NetHostSocketState* state = static_cast<NetHostSocketState*>(context);
    if (state->m_phase == NET_HOST_STOPPING || state->m_phase == NET_HOST_STOPPED)
    {
        return;
    }
    try
    {
        auto found = state->m_peers.find(peer);
        if (state->m_phase == NET_HOST_CREATING || state->m_instance == nullptr || peer == 0 || (found == state->m_peers.end() &&
            ((!state->m_server && (state->m_phase != NET_HOST_STARTING || state->m_lastPeer != 0)) || peer <= state->m_lastPeer)))
        {
            state->fail(NI_PROTOCOL_ERROR);
            return;
        }
        if (found == state->m_peers.end())
        {
            const int32_t added = state->addPeer(peer);
            if (added != NI_OK)
            {
                // 新连接未向上层发布；仅拒绝此代次，旧连接继续运行。
                state->m_lastPeer = peer;
                if (state->m_functions.close(state->m_instance, peer, added) != 0)
                {
                    state->fail(NI_INVALID_STATE);
                }
                return;
            }
            found = state->m_peers.find(peer);
        }
        if (found->second->m_connected || found->second->m_closed)
        {
            state->fail(NI_PROTOCOL_ERROR);
            return;
        }
        found->second->m_connected = true;
        found->second->m_paused = true;
        if (!state->queue(NET_HOST_CONNECTED, peer, 0))
        {
            return;
        }
        if (state->m_instance != nullptr && state->m_functions.pauseRead(state->m_instance, peer, 1) != 0)
        {
            state->fail(NI_INVALID_STATE);
            return;
        }

    }
    catch (...)
    {
        state->fail(NI_NO_MEMORY);
    }
}

void NetHostSocketState::data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
{
    NetHostSocketState* state = static_cast<NetHostSocketState*>(context);
    if (state->m_phase == NET_HOST_STOPPING || state->m_phase == NET_HOST_STOPPED)
    {
        return;
    }
    auto found = state->m_peers.find(peer);
    if (found == state->m_peers.end())
    {
        if (peer == 0 || peer > state->m_lastPeer)
        {
            state->fail(NI_PROTOCOL_ERROR);
        }
        return;
    }
    NetHostSocketPeer& current = *found->second;
    if (current.m_closing || current.m_closed)
    {
        return;
    }
    if (!current.m_connected || current.m_paused || current.m_inputPending || bytes == nullptr || size == 0 || size > 16384)
    {
        state->fail(NI_PROTOCOL_ERROR);
        return;
    }
    current.m_paused = true;
    current.m_inputPending = true;
    try
    {
        // 先拥有字节并保留原始事件先后，再调用允许同步CLOSED的宿主暂停接口。
        const int32_t allocated = state->m_bodies->allocate(size, current.m_input);
        if (allocated != NI_OK)
        {
            current.m_inputPending = false;
            state->close(peer, allocated);
            return;
        }
        std::memcpy(current.m_input.data(), bytes, size);
        current.m_inputSize = size;
        if (!state->queue(NET_HOST_DATA, peer, 0))
        {
            return;
        }
        if (state->m_functions.pauseRead(state->m_instance, peer, 1) != 0)
        {
            state->fail(NI_INVALID_STATE);
        }
    }
    catch (...)
    {
        state->fail(NI_NO_MEMORY);
    }
}

void NetHostSocketState::writable(void* context, uint64_t peer)
{
    NetHostSocketState* state = static_cast<NetHostSocketState*>(context);
    if (state->m_phase == NET_HOST_STOPPING || state->m_phase == NET_HOST_STOPPED)
    {
        return;
    }
    auto found = state->m_peers.find(peer);
    if (found == state->m_peers.end() || !found->second->m_connected || found->second->m_closing ||
        found->second->m_closed || found->second->m_writablePending)
    {
        return;
    }
    found->second->m_writablePending = true;
    state->queue(NET_HOST_WRITABLE, peer, 0);
}

void NetHostSocketState::closed(void* context, uint64_t peer, int32_t reason)
{
    NetHostSocketState* state = static_cast<NetHostSocketState*>(context);
    if (state->m_phase == NET_HOST_STOPPED)
    {
        return;
    }
    try
    {
        auto found = state->m_peers.find(peer);
        if (found == state->m_peers.end())
        {
            // 主动连接只接纳一个编号；同步 EOF 和同步 connected 必须属于同一次拨号。
            if (!state->m_server && state->m_phase == NET_HOST_STARTING && state->m_lastPeer != 0 && peer != state->m_lastPeer)
            {
                state->fail(NI_PROTOCOL_ERROR);
                return;
            }
            if (peer != 0 && peer <= state->m_lastPeer)
            {
                return;
            }
            if (state->m_phase != NET_HOST_STARTING || state->m_server || peer == 0)
            {
                state->fail(NI_PROTOCOL_ERROR);
                return;
            }
            const int32_t added = state->addPeer(peer);
            if (added != NI_OK)
            {
                state->fail(added);
                return;
            }
            found = state->m_peers.find(peer);
        }
        if (found->second->m_closed)
        {
            return;
        }
        found->second->m_closed = true;
        if (!found->second->m_closing)
        {
            found->second->m_reason = reason == 0 ? NI_OK : NI_INVALID_STATE;
            found->second->m_detail = reason;
        }
        state->queue(NET_HOST_CLOSED, peer, found->second->m_reason, found->second->m_detail);
    }
    catch (...)
    {
        state->fail(NI_NO_MEMORY);
    }
}

void NetHostSocketState::close(uint64_t peer, int32_t reason)
{
    auto found = m_peers.find(peer);
    if (found == m_peers.end() || found->second->m_closing || found->second->m_closed)
    {
        return;
    }
    found->second->m_closing = true;
    found->second->m_reason = reason;
    try
    {
        if (m_instance == nullptr || m_functions.close(m_instance, peer, reason) != 0)
        {
            fail(NI_INVALID_STATE);
        }
    }
    catch (...)
    {
        fail(NI_INVALID_STATE);
    }
}

void NetHostSocketState::dispatch()
{
    if (m_eventHead != nullptr)
    {
        NetExecution::progress();
    }
    for (uint32_t count = 0; m_eventHead != nullptr && count < m_eventBudget; ++count)
    {
        const NetHostSocketEvent event = *m_eventHead;
        m_eventHead->m_queued = false;
        m_eventHead = event.m_next;
        if (m_eventHead == nullptr)
        {
            m_eventTail = nullptr;
        }
        const auto found = m_peers.find(event.m_peer);
        if (found == m_peers.end())
        {
            continue;
        }
        if (event.m_kind == NET_HOST_CLOSED)
        {
            m_peers.erase(found);
        }
        else
        {
            if (event.m_kind == NET_HOST_DATA)
            {
                found->second->m_inputPending = false;
                m_input.swap(found->second->m_input);
                m_inputSize = found->second->m_inputSize;
                found->second->m_inputSize = 0;
            }
            if (event.m_kind == NET_HOST_WRITABLE)
            {
                found->second->m_writablePending = false;
            }
            if (found->second->m_closing || m_phase == NET_HOST_STOPPING)
            {
                clearInput();
                continue;
            }
            if (event.m_kind == NET_HOST_CONNECTED)
            {
                found->second->m_announced = true;
            }
        }
        if (m_listener == nullptr)
        {
            clearInput();
            continue;
        }
        m_callback = true;
        try
        {
            switch (event.m_kind)
            {
            case NET_HOST_CONNECTED:
                m_listener->onConnected(event.m_peer);
                break;
            case NET_HOST_DATA:
                m_listener->onData(event.m_peer, m_input.data(), m_inputSize);
                break;
            case NET_HOST_WRITABLE:
                m_listener->onWritable(event.m_peer);
                break;
            case NET_HOST_CLOSED:
                m_listener->onClosed(event.m_peer, event.m_reason, event.m_detail);
                break;
            }
        }
        catch (...)
        {
            close(event.m_peer, NI_INVALID_STATE);
        }
        m_callback = false;
        clearInput();
    }
}

void NetHostSocketState::stop()
{
    if (m_phase == NET_HOST_STOPPED)
    {
        return;
    }
    m_phase = NET_HOST_STOPPING;
    const bool previous = m_polling;
    m_polling = true;
    if (m_instance != nullptr)
    {
        try
        {
            if (m_functions.shutdown(m_instance) != 0)
            {
                fail(NI_INVALID_STATE);
            }
        }
        catch (...)
        {
            fail(NI_INVALID_STATE);
        }
    }
    m_phase = NET_HOST_STOPPED;
    // 终态直接从既有拥有表取走，不因事件槽耗尽或分配失败遗漏关闭通知。
    m_eventHead = nullptr;
    m_eventTail = nullptr;
    clearInput();
    while (!m_peers.empty())
    {
        const uint64_t peer = m_peers.begin()->first;
        const int32_t reason = m_fault != NI_OK ? m_fault : m_peers.begin()->second->m_reason;
        const int32_t detail = m_peers.begin()->second->m_detail;
        m_peers.erase(m_peers.begin());
        if (m_listener != nullptr)
        {
            m_callback = true;
            try
            {
                m_listener->onClosed(peer, reason, detail);
            }
            catch (...)
            {
                // 资源已经摘除，异常不能制造第二个终态或中断其他端点收尾。
            }
            m_callback = false;
        }
    }
    m_listener = nullptr;
    m_polling = previous;
}

bool NetHostSocketState::poll()
{
    if (m_polling || m_callback)
    {
        return !m_peers.empty() || m_eventHead != nullptr;
    }
    m_polling = true;
    int32_t active = 0;
    try
    {
        if (m_instance != nullptr && m_phase == NET_HOST_ACTIVE)
        {
            active = m_functions.poll(m_instance);
            if (active < 0 || active > 1)
            {
                fail(NI_INVALID_STATE);
            }
        }
        dispatch();
        if (m_phase == NET_HOST_ACTIVE && active == 0 && m_eventHead == nullptr && (m_server || !m_peers.empty()))
        {
            fail(NI_PROTOCOL_ERROR);
        }
    }
    catch (...)
    {
        // 执行标记必须在异常后恢复，不能将后续poll永久误判为重入。
        m_callback = false;
        clearInput();
        fail(NI_INVALID_STATE);
    }
    if (m_phase == NET_HOST_STOPPING)
    {
        stop();
        active = 0;
    }
    m_polling = false;
    return active > 0 || !m_peers.empty() || m_eventHead != nullptr;
}