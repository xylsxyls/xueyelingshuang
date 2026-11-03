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
m_detail(0)
{

}

NetHostSocketPeer::~NetHostSocketPeer()
{
    if (!m_input.empty())
    {
        NetInterface::wipe(m_input.data(), static_cast<uint32_t>(m_input.size()));
    }
}

NetHostSocketState::NetHostSocketState(const NetHostFunctions& functions) :
m_functions(functions),
m_instance(nullptr),
m_listener(nullptr),
m_eventCapacity(0),
m_eventHead(0),
m_eventCount(0),
m_lastPeer(0),
m_queuedBytes(0),
m_maxPeers(0),
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

int32_t NetHostSocketState::start(NetHostSocketListener* listener, uint32_t maxPeers, bool server)
{
    if (m_phase != NET_HOST_IDLE || m_polling || m_callback)
    {
        return NI_INVALID_STATE;
    }
    if (!valid(m_functions) || (server ? m_functions.listen == nullptr : m_functions.connect == nullptr))
    {
        return NI_NOT_SUPPORTED;
    }
    m_phase = NET_HOST_CREATING;
    m_maxPeers = maxPeers;
    m_server = server;
    m_listener = listener;
    int32_t result = -1;
    try
    {
        m_eventCapacity = maxPeers * 4;
        m_events.reset(new NetHostSocketEvent[m_eventCapacity]);
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
    if (m_phase != NET_HOST_STOPPED)
    {
        m_phase = NET_HOST_STOPPING;
    }
}

bool NetHostSocketState::queue(NetHostEventKind kind, uint64_t peer, int32_t reason, int32_t detail)
{
    if (m_eventCount == m_eventCapacity)
    {
        fail(NI_LIMIT_EXCEEDED);
        return false;
    }
    NetHostSocketEvent& event = m_events[(m_eventHead + m_eventCount) % m_eventCapacity];
    event.m_kind = kind;
    event.m_peer = peer;
    event.m_reason = reason;
    event.m_detail = detail;
    ++m_eventCount;
    return true;
}

void NetHostSocketState::clearInput()
{
    if (!m_input.empty())
    {
        NetInterface::wipe(m_input.data(), static_cast<uint32_t>(m_input.size()));
        m_input.clear();
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
            ((!state->m_server && state->m_phase != NET_HOST_STARTING) || peer <= state->m_lastPeer || state->m_peers.size() >= state->m_maxPeers)))
        {
            state->fail(NI_PROTOCOL_ERROR);
            return;
        }
        if (found == state->m_peers.end())
        {
            found = state->m_peers.insert(std::make_pair(peer, NetHostSocketPeer())).first;
            state->m_lastPeer = peer;
        }
        if (found->second.m_connected || found->second.m_closed)
        {
            state->fail(NI_PROTOCOL_ERROR);
            return;
        }
        found->second.m_connected = true;
        found->second.m_paused = true;
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
    NetHostSocketPeer& current = found->second;
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
        current.m_input.assign(bytes, bytes + size);
        if (!state->queue(NET_HOST_DATA, peer, 0))
        {
            return;
        }
        state->m_queuedBytes += size;
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
    if (found == state->m_peers.end() || !found->second.m_connected || found->second.m_closing ||
        found->second.m_closed || found->second.m_writablePending)
    {
        return;
    }
    found->second.m_writablePending = true;
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
            if (peer != 0 && peer <= state->m_lastPeer)
            {
                return;
            }
            if (state->m_phase != NET_HOST_STARTING || state->m_server || peer == 0 || state->m_peers.size() >= state->m_maxPeers)
            {
                state->fail(NI_PROTOCOL_ERROR);
                return;
            }
            found = state->m_peers.insert(std::make_pair(peer, NetHostSocketPeer())).first;
            state->m_lastPeer = peer;
        }
        if (found->second.m_closed)
        {
            return;
        }
        found->second.m_closed = true;
        if (!found->second.m_closing)
        {
            found->second.m_reason = reason == 0 ? NI_OK : NI_INVALID_STATE;
            found->second.m_detail = reason;
        }
        state->queue(NET_HOST_CLOSED, peer, found->second.m_reason, found->second.m_detail);
    }
    catch (...)
    {
        state->fail(NI_NO_MEMORY);
    }
}

void NetHostSocketState::close(uint64_t peer, int32_t reason)
{
    auto found = m_peers.find(peer);
    if (found == m_peers.end() || found->second.m_closing)
    {
        return;
    }
    found->second.m_closing = true;
    found->second.m_reason = reason;
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
    for (uint32_t count = 0; m_eventCount != 0 && count < 2048; ++count)
    {
        const NetHostSocketEvent event = m_events[m_eventHead];
        m_eventHead = (m_eventHead + 1) % m_eventCapacity;
        --m_eventCount;
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
                found->second.m_inputPending = false;
                m_queuedBytes -= static_cast<uint32_t>(found->second.m_input.size());
                m_input.swap(found->second.m_input);
            }
            if (event.m_kind == NET_HOST_WRITABLE)
            {
                found->second.m_writablePending = false;
            }
            if (found->second.m_closing || m_phase == NET_HOST_STOPPING)
            {
                clearInput();
                continue;
            }
            if (event.m_kind == NET_HOST_CONNECTED)
            {
                found->second.m_announced = true;
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
                m_listener->onData(event.m_peer, m_input.data(), static_cast<uint32_t>(m_input.size()));
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
    m_eventCount = 0;
    m_eventHead = 0;
    m_queuedBytes = 0;
    clearInput();
    while (!m_peers.empty())
    {
        const uint64_t peer = m_peers.begin()->first;
        const int32_t reason = m_fault != NI_OK ? m_fault : m_peers.begin()->second.m_reason;
        const int32_t detail = m_peers.begin()->second.m_detail;
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
        return !m_peers.empty() || m_eventCount != 0;
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
        if (m_phase == NET_HOST_ACTIVE && active == 0 && (m_server || !m_peers.empty()))
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
    return active > 0 || !m_peers.empty() || m_eventCount != 0;
}