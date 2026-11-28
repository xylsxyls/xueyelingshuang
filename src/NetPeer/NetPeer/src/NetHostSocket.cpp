#include "NetHostSocket.h"
#include "NetPeerMacro.h"
#include "NetHostSocketState.h"
#include "NetInterface/NetInterfaceAPI.h"

NetHostSocketListener::~NetHostSocketListener()
{

}

NetHostSocket::NetHostSocket(const NetHostFunctions* functions) : NetHostSocket(functions, NetHostPoolConfig())
{
}

NetHostSocket::NetHostSocket(const NetHostFunctions* functions, const NetHostPoolConfig& pools) :
m_state(new NetHostSocketState(functions == nullptr ? NetHostSocketState::m_default :
    (NetHostSocketState::valid(*functions) ? *functions : NetHostFunctions()), pools))
{

}

NetHostSocket::~NetHostSocket()
{
    const int32_t result = uninit();
    m_state->m_listener = nullptr;
    if (result == NI_OK)
    {
        delete m_state;
    }
    // 违约宿主拒绝释放时保留已停止/无监听者的有界状态，避免迟到回调使用已释放内存
}

int32_t NetHostSocket::configureDefault(const NetHostFunctions& functions)
{
    if (!NetHostSocketState::valid(functions))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (NetHostSocketState::m_configured)
    {
        return NI_INVALID_STATE;
    }
    const int32_t execution = NetExecution::configure(functions.execution);
    if (execution != NI_OK) { return execution; }
    NetHostSocketState::m_default = functions;
    NetHostSocketState::m_configured = true;
    return NI_OK;
}

bool NetHostSocket::defaultSupports(bool server)
{
    return NetHostSocketState::m_configured &&
        (server ? NetHostSocketState::m_default.listen != nullptr : NetHostSocketState::m_default.connect != nullptr);
}

int32_t NetHostSocket::configureInputPool(const NetBufferPoolConfig& config)
{
    if (m_state->m_phase != NET_HOST_IDLE)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetBufferPool> pool(new NetBufferPool(config));
        if (pool->status() != NI_OK)
        {
            return pool->status();
        }
        m_state->m_bodies.swap(pool);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetHostSocket::configurePeerPool(const LockFreeHashMapPoolConfig& config)
{
    if (m_state->m_phase != NET_HOST_IDLE)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetObjectPool<NetHostSocketPeer>> pool(new NetObjectPool<NetHostSocketPeer>(config));
        if (pool->status() != NI_OK)
        {
            return pool->status();
        }
        m_state->m_peerPool.swap(pool);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetHostSocket::listen(const char* address, uint32_t port, NetHostSocketListener* listener, uint32_t maxPeers)
{
    if (listener == nullptr || !NetHostSocketState::validListen(address, port, maxPeers))
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t started = m_state->start(listener, true);
    if (started != NI_OK)
    {
        return started;
    }
    int32_t result = -1;
    try
    {
        result = m_state->m_functions.listen(m_state->m_instance, address, port, maxPeers);
    }
    catch (...)
    {
        m_state->fail(NI_INVALID_STATE);
    }
    if (result != 0 || m_state->m_fault != NI_OK)
    {
        const int32_t error = m_state->m_fault < 0 ? m_state->m_fault : NI_INVALID_STATE;
        m_state->m_listener = nullptr;
        m_state->stop();
        return error;
    }
    m_state->m_phase = NET_HOST_ACTIVE;
    return NI_OK;
}

int32_t NetHostSocket::connect(const char* address, uint32_t port, NetHostSocketListener* listener, uint64_t& peer)
{
    peer = 0;
    if (listener == nullptr || !NetHostSocketState::validConnect(address, port))
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t started = m_state->start(listener, false);
    if (started != NI_OK)
    {
        return started;
    }
    int32_t result = -1;
    try
    {
        result = m_state->m_functions.connect(m_state->m_instance, address, port, &peer);
        if (result == 0 && peer != 0 && m_state->m_peers.find(peer) == m_state->m_peers.end())
        {
            if (!m_state->m_peers.empty() || peer <= m_state->m_lastPeer)
            {
                m_state->fail(NI_PROTOCOL_ERROR);
            }
            else
            {
                const int32_t added = m_state->addPeer(peer);
                if (added != NI_OK)
                {
                    m_state->fail(added);
                }
            }
        }
    }
    catch (...)
    {
        m_state->fail(NI_INVALID_STATE);
    }
    if (result != 0 || peer == 0 || m_state->m_fault != NI_OK)
    {
        peer = 0;
        const int32_t error = m_state->m_fault < 0 ? m_state->m_fault : NI_INVALID_STATE;
        m_state->m_listener = nullptr;
        m_state->stop();
        return error;
    }
    m_state->m_phase = NET_HOST_ACTIVE;
    // 直连和中继都从这里接纳拨号；下一轮 poll 将推进宿主并登记启动期限。
    NetExecution::progress();
    return NI_OK;
}

bool NetHostSocket::poll()
{
    return m_state->poll();
}

int32_t NetHostSocket::write(uint64_t peer, const uint8_t* data, uint32_t size)
{
    if ((data == nullptr && size != 0) || size > 32768)
    {
        return NI_INVALID_ARGUMENT;
    }
    auto found = m_state->m_peers.find(peer);
    if (m_state->m_phase != NET_HOST_ACTIVE || found == m_state->m_peers.end() || !found->second->m_announced ||
        found->second->m_closing || found->second->m_closed)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        const int32_t result = m_state->m_functions.write(m_state->m_instance, peer, data, size);
        if (result == 0)
        {
            return NI_OK;
        }
        if (result == 1)
        {
            return NI_LIMIT_EXCEEDED;
        }
        if (result > 1)
        {
            m_state->fail(NI_PROTOCOL_ERROR);
        }
        return NI_INVALID_STATE;
    }
    catch (...)
    {
        m_state->fail(NI_INVALID_STATE);
        return NI_INVALID_STATE;
    }
}

int32_t NetHostSocket::pauseRead(uint64_t peer, bool paused)
{
    auto found = m_state->m_peers.find(peer);
    if (m_state->m_phase != NET_HOST_ACTIVE || found == m_state->m_peers.end() || !found->second->m_connected ||
        found->second->m_closing || (!paused && found->second->m_inputPending))
    {
        return NI_INVALID_STATE;
    }
    found->second->m_paused = paused;
    // 已观察EOF但终态尚在排队：仅控制最后输入的本地交付，不再操作底层socket。
    if (found->second->m_closed) { return NI_OK; }
    try
    {
        if (m_state->m_functions.pauseRead(m_state->m_instance, peer, paused ? 1 : 0) != 0)
        {
            m_state->fail(NI_INVALID_STATE);
            return NI_INVALID_STATE;
        }
        return NI_OK;
    }
    catch (...)
    {
        m_state->fail(NI_INVALID_STATE);
        return NI_INVALID_STATE;
    }
}

void NetHostSocket::close(uint64_t peer, int32_t reason)
{
    m_state->close(peer, reason);
}

void NetHostSocket::shutdown()
{
    if (m_state->m_phase != NET_HOST_STOPPED)
    {
        m_state->m_phase = NET_HOST_STOPPING;
    }
    if (!m_state->m_polling && !m_state->m_callback)
    {
        m_state->stop();
    }
}

int32_t NetHostSocket::uninit()
{
    if (m_state->m_polling || m_state->m_callback)
    {
        return NI_INVALID_STATE;
    }
    shutdown();
    if (m_state->m_instance != nullptr)
    {
        try
        {
            if (m_state->m_functions.destroy(m_state->m_instance) != 0)
            {
                m_state->fail(NI_INVALID_STATE);
                return NI_INVALID_STATE;
            }
        }
        catch (...)
        {
            m_state->fail(NI_INVALID_STATE);
            return NI_INVALID_STATE;
        }
        m_state->m_instance = nullptr;
    }
    return NI_OK;
}

int32_t NetHostSocket::status() const
{
    return m_state->m_fault;
}

uint32_t NetHostSocket::localPort() const
{
    if (m_state->m_instance == nullptr || m_state->m_phase != NET_HOST_ACTIVE)
    {
        return 0;
    }
    try
    {
        return m_state->m_functions.localPort(m_state->m_instance);
    }
    catch (...)
    {
        return 0;
    }
}

uint32_t NetHostSocket::peerCount() const
{
    return static_cast<uint32_t>(m_state->m_peers.size());
}

bool NetHostSocket::isBackpressure(int32_t result)
{
    return result == NI_LIMIT_EXCEEDED;
}

int32_t NetHostSocket::randomBytes(uint8_t* output, uint32_t size)
{
    if ((output == nullptr && size != 0) || size > 8192)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (size == 0)
    {
        return NI_OK;
    }
    try
    {
        if (NetHostSocketState::m_configured && NetHostSocketState::m_default.entropy != nullptr &&
            NetHostSocketState::m_default.entropy(NetHostSocketState::m_default.context, output, size) == 0)
        {
            return NI_OK;
        }
    }
    catch (...)
    {
        // 宿主异常按熵失败处理，不把未完成的输出用于TLS或会话代次
    }
    NetInterface::wipe(output, size);
    return NI_INVALID_STATE;
}