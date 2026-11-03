#include "LibuvChannel.h"
#include "LibuvChannelState.h"
#include <memory>

LibuvChannelListener::~LibuvChannelListener()
{

}

LibuvChannel::LibuvChannel() :
m_state(new LibuvChannelState)
{

}

LibuvChannel::~LibuvChannel()
{
    delete m_state;
}

int32_t LibuvChannel::listen(const char* address, uint32_t port,
    LibuvChannelListener* listener, uint32_t maxPeers)
{
    if (m_state->m_loopStatus != 0)
    {
        return m_state->m_loopStatus;
    }
    if (m_state->m_started || m_state->m_stopping || listener == nullptr || maxPeers == 0 || maxPeers > 256)
    {
        return UV_EINVAL;
    }
    sockaddr_storage target;
    int result = m_state->parseAddress(address, port, target);
    if (result != 0)
    {
        return result;
    }
    m_state->m_callback = listener;
    m_state->m_started = true;
    m_state->m_maxPeers = maxPeers;
    result = uv_tcp_init(&m_state->m_loop, &m_state->m_listener);
    if (result != 0)
    {
        return result;
    }
    m_state->m_listenerInitialized = true;
    m_state->m_listener.data = m_state;
    result = uv_tcp_bind(&m_state->m_listener, reinterpret_cast<const sockaddr*>(&target), 0);
    if (result == 0)
    {
        result = uv_listen(reinterpret_cast<uv_stream_t*>(&m_state->m_listener), 128, &LibuvChannelState::acceptPeer);
    }
    if (result != 0)
    {
        m_state->shutdown();
        return result;
    }
    int length = sizeof(target);
    result = uv_tcp_getsockname(&m_state->m_listener, reinterpret_cast<sockaddr*>(&target), &length);
    if (result != 0)
    {
        m_state->shutdown();
        return result;
    }
    m_state->m_port = target.ss_family == AF_INET ?
        ntohs(reinterpret_cast<sockaddr_in*>(&target)->sin_port) :
        ntohs(reinterpret_cast<sockaddr_in6*>(&target)->sin6_port);
    return 0;
}

int32_t LibuvChannel::connect(const char* address, uint32_t port,
    LibuvChannelListener* listener, uint64_t& id)
{
    id = 0;
    if (m_state->m_loopStatus != 0)
    {
        return m_state->m_loopStatus;
    }
    if (m_state->m_started || m_state->m_stopping || listener == nullptr || port == 0)
    {
        return UV_EINVAL;
    }
    sockaddr_storage target;
    int result = m_state->parseAddress(address, port, target);
    if (result != 0)
    {
        return result;
    }
    m_state->m_started = true;
    m_state->m_callback = listener;
    m_state->m_maxPeers = 1;
    LibuvChannelPeer* peer = m_state->createPeer(false);
    if (peer == nullptr)
    {
        return UV_ENOMEM;
    }
    result = uv_tcp_connect(&peer->m_connect, &peer->m_tcp, reinterpret_cast<const sockaddr*>(&target),
        &LibuvChannelState::connected);
    if (result != 0)
    {
        peer->m_reportClose = false;
        m_state->closePeer(peer, result);
        return result;
    }
    id = peer->m_id;
    return 0;
}

bool LibuvChannel::poll()
{
    if (m_state->m_loopStatus != 0)
    {
        return false;
    }
    if (m_state->m_polling)
    {
        return uv_loop_alive(&m_state->m_loop) != 0;
    }
    m_state->m_polling = true;
    // 接受回调中一次分配失败不能吞掉libuv已就绪的连接。每轮最多主动重试一次，
    // 持续低内存不在本轮自旋；关闭中的拒绝句柄由closed回调继续原有收敛。
    if (m_state->m_acceptPending && !m_state->m_rejectPending &&
        !m_state->m_stopping && m_state->m_listenerInitialized)
    {
        LibuvChannelState::acceptPeer(reinterpret_cast<uv_stream_t*>(&m_state->m_listener), 0);
    }
    const bool alive = uv_run(&m_state->m_loop, UV_RUN_NOWAIT) != 0;
    m_state->m_polling = false;
    return alive;
}

int32_t LibuvChannel::write(uint64_t id, const uint8_t* data, uint32_t size)
{
    if ((data == nullptr && size != 0) || size > 32768)
    {
        return UV_EINVAL;
    }
    auto entry = m_state->m_peers.find(id);
    if (entry == m_state->m_peers.end() || !entry->second->m_connected || entry->second->m_closing)
    {
        return UV_ENOTCONN;
    }
    if (size == 0)
    {
        return 0;
    }
    LibuvChannelPeer* peer = entry->second;
    if (peer->m_queuedBytes > 65536 - size || peer->m_queuedWrites >= 64)
    {
        return UV_ENOBUFS;
    }
    LibuvChannelWrite* request = nullptr;
    try
    {
        request = new LibuvChannelWrite(peer, data, size);
    }
    catch (...)
    {
        return UV_ENOMEM;
    }
    uv_buf_t buffer = uv_buf_init(reinterpret_cast<char*>(&request->m_bytes[0]), size);
    const int result = uv_write(&request->m_request, reinterpret_cast<uv_stream_t*>(&peer->m_tcp),
        &buffer, 1, &LibuvChannelState::written);
    if (result != 0)
    {
        delete request;
        return result;
    }
    peer->m_queuedBytes += size;
    ++peer->m_queuedWrites;
    return 0;
}

int32_t LibuvChannel::pauseRead(uint64_t id, bool paused)
{
    auto entry = m_state->m_peers.find(id);
    if (entry == m_state->m_peers.end() || !entry->second->m_connected || entry->second->m_closing)
    {
        return UV_ENOTCONN;
    }
    LibuvChannelPeer* peer = entry->second;
    if (peer->m_paused == paused)
    {
        return 0;
    }
    const int result = paused ? uv_read_stop(reinterpret_cast<uv_stream_t*>(&peer->m_tcp)) :
        m_state->startRead(peer);
    if (result == 0)
    {
        peer->m_paused = paused;
    }
    return result;
}

void LibuvChannel::close(uint64_t peer, int32_t reason)
{
    auto entry = m_state->m_peers.find(peer);
    if (entry != m_state->m_peers.end())
    {
        m_state->closePeer(entry->second, reason);
    }
}

void LibuvChannel::shutdown()
{
    m_state->shutdown();
    if (m_state->m_loopStatus == 0 && !m_state->m_polling)
    {
        m_state->m_polling = true;
        uv_run(&m_state->m_loop, UV_RUN_DEFAULT);
        m_state->m_polling = false;
    }
}

uint32_t LibuvChannel::localPort() const
{
    return m_state->m_port;
}

uint32_t LibuvChannel::peerCount() const
{
    return static_cast<uint32_t>(m_state->m_peers.size());
}

bool LibuvChannel::isBackpressure(int32_t result)
{
    return result == UV_ENOBUFS;
}