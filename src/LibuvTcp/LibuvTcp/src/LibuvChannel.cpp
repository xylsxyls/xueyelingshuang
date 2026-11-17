#include "LibuvChannelState.h"
#include "LibuvChannel.h"
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

int32_t LibuvChannel::configureWritePool(const LibuvChannelWritePoolConfig& config)
{
    if (m_state->m_started || m_state->m_stopping)
    {
        return UV_EBUSY;
    }
    if (!config.valid())
    {
        return UV_EINVAL;
    }
    try
    {
        std::unique_ptr<LibuvChannelWritePoolConfig> copy(new LibuvChannelWritePoolConfig(config));
        std::unique_ptr<LockFreeHashMapPool<LibuvChannelWrite>> pool(new LockFreeHashMapPool<LibuvChannelWrite>);
        const LockFreeHashMapStatus result = pool->initialize(config.m_records);
        if (result != LockFreeHashMapOk)
        {
            return result == LockFreeHashMapNoMemory ? UV_ENOMEM : UV_EINVAL;
        }
        std::unique_ptr<LockFreeQueue<LibuvChannelWrite*>> incoming(new LockFreeQueue<LibuvChannelWrite*>(false));
        std::unique_ptr<LockFreeHashMap<uint64_t, uint32_t>> connections(
            new LockFreeHashMap<uint64_t, uint32_t>(256, config.m_connections));
        if (incoming->configurePool(config.m_submission) != LockFreeQueueSuccess ||
            incoming->tryInit() != LockFreeQueueSuccess || connections->status() != LockFreeHashMapOk)
        {
            return UV_ENOMEM;
        }
        m_state->releaseLanes();
        m_state->m_incoming.swap(incoming);
        m_state->m_connectedPeers.swap(connections);
        m_state->m_writeConfig.swap(copy);
        m_state->m_writePool.swap(pool);
        m_state->m_writePoolStatus = 0;
        return 0;
    }
    catch (...)
    {
        return UV_ENOMEM;
    }
}

int32_t LibuvChannel::configurePeerPool(const LockFreeHashMapPoolConfig& config)
{
    if (m_state->m_started || m_state->m_stopping)
    {
        return UV_EBUSY;
    }
    try
    {
        std::unique_ptr<LockFreeHashMapPool<LibuvChannelPeer>> pool(new LockFreeHashMapPool<LibuvChannelPeer>);
        const LockFreeHashMapStatus result = pool->initialize(config);
        if (result != LockFreeHashMapOk)
        {
            return result == LockFreeHashMapNoMemory ? UV_ENOMEM : UV_EINVAL;
        }
        m_state->m_peerPool.swap(pool);
        m_state->m_peerPoolStatus = 0;
        return 0;
    }
    catch (...)
    {
        return UV_ENOMEM;
    }
}

int32_t LibuvChannel::configureReadBuffer(uint32_t bufferBytes)
{
    if (m_state->m_started || m_state->m_stopping)
    {
        return UV_EBUSY;
    }
    if (bufferBytes == 0 || bufferBytes > 16777216U)
    {
        return UV_EINVAL;
    }
    m_state->m_readBufferBytes = bufferBytes;
    return 0;
}

int32_t LibuvChannel::listen(const char* address, uint32_t port,
    LibuvChannelListener* listener, uint32_t maxPeers)
{
    if (m_state->m_started || m_state->m_stopping || listener == nullptr || maxPeers == 0)
    {
        return UV_EINVAL;
    }
    sockaddr_storage target;
    int result = m_state->parseAddress(address, port, target);
    if (result != 0)
    {
        return result;
    }
    result = m_state->initialize();
    if (result != 0)
    {
        return result;
    }
    try
    {
        // 在监听前准备桶表，连接接纳期间不因桶扩容产生额外抖动。
        m_state->m_peers.reserve(maxPeers);
    }
    catch (...)
    {
        return UV_ENOMEM;
    }
    m_state->m_callback = listener;
    m_state->m_started = true;
    (void)maxPeers;
    result = uv_tcp_init(m_state->loop(), &m_state->m_listener);
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
    result = m_state->initialize();
    if (result != 0)
    {
        return result;
    }
    try
    {
        m_state->m_peers.reserve(1);
    }
    catch (...)
    {
        return UV_ENOMEM;
    }
    m_state->m_started = true;
    m_state->m_callback = listener;
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
    if (!m_state->m_loopInitialized)
    {
        return false;
    }
    if (m_state->m_polling)
    {
        return m_state->m_listenerInitialized || m_state->m_rejectPending || m_state->m_overflowClosing || !m_state->m_peers.empty();
    }
    m_state->m_polling = true;
    if (m_state->m_submissionError.load(std::memory_order_acquire) != 0)
    {
        m_state->shutdown();
    }
    // 接受回调中一次分配失败不能吞掉libuv已就绪的连接。每轮最多主动重试一次，
    // 持续低内存不在本轮自旋；关闭中的拒绝句柄由closed回调继续原有收敛。
    if (m_state->m_acceptPending && !m_state->m_rejectPending && !m_state->m_overflowClosing &&
        !m_state->m_stopping && m_state->m_listenerInitialized)
    {
        LibuvChannelState::acceptPeer(reinterpret_cast<uv_stream_t*>(&m_state->m_listener), 0);
    }
    const bool alive = m_state->m_group == nullptr ? uv_run(m_state->loop(), UV_RUN_NOWAIT) != 0 :
        (m_state->m_group->run(UV_RUN_NOWAIT), m_state->m_listenerInitialized ||
            m_state->m_rejectPending || m_state->m_overflowClosing || !m_state->m_peers.empty());
    m_state->m_polling = false;
    return alive;
}

int32_t LibuvChannel::post(uint64_t id, const uint8_t* data, uint32_t size)
{
    LibuvCall call(m_state->m_submissionGate);
    if (!call.accepted())
    {
        return call.status();
    }
    if ((data == nullptr && size != 0) || size > m_state->m_writeConfig->m_maximumWriteBytes)
    {
        return UV_EINVAL;
    }
    const int32_t failed = m_state->m_submissionError.load(std::memory_order_acquire);
    if (failed != 0) { return failed; }
    LibuvChannelLane* lane = m_state->producerLane();
    if (lane != nullptr)
    {
        const int32_t result = m_state->postLane(*lane, id, data, size);
        if (result == UV_ENOBUFS)
        {
            m_state->m_notifyCapacity.store(true, std::memory_order_release);
        }
        if (result == 0 && size != 0 && !lane->m_notified.load(std::memory_order_acquire) &&
            !lane->m_notified.exchange(true, std::memory_order_acq_rel))
        {
            const int32_t notified = uv_async_send(&m_state->m_postAsync);
            if (notified != 0)
            {
                m_state->m_submissionError.store(notified, std::memory_order_release);
            }
        }
        return result;
    }
    const bool owner = m_state->m_ownerToken != UINT32_MAX ?
        LibuvBase::producerToken() == m_state->m_ownerToken :
        std::this_thread::get_id() == m_state->m_ownerThread;
    LibuvChannelPeer* local = nullptr;
    if (owner)
    {
        // 所属线程独占std索引，无需再穿过并发索引和MPSC队列。
        const auto found = m_state->m_peers.find(id);
        if (found == m_state->m_peers.end() || !found->second->m_connected || found->second->m_closing)
        {
            return UV_ENOTCONN;
        }
        local = found->second;
    }
    else
    {
        const int32_t found = m_state->connectedStatus(id);
        if (found != 0)
        {
            return found;
        }
    }
    if (size == 0)
    {
        return 0;
    }
    int32_t result = 0;
    LibuvChannelWrite* request = m_state->prepareWrite(id, data, size, result);
    if (request == nullptr)
    {
        if (result == UV_ENOBUFS) { m_state->m_notifyCapacity.store(true, std::memory_order_release); }
        // 分配正文失败会归还刚取得的共享槽，也可能解除其他生产者的满池等待。
        uv_async_send(&m_state->m_postAsync);
        return result;
    }
    if (local != nullptr)
    {
        m_state->appendWrite(local, request);
    }
    else
    {
        const LockFreeQueueResult queued = m_state->m_incoming->tryPushAndFence(request, true, false);
        if (queued != LockFreeQueueSuccess)
        {
            m_state->releaseWrite(request);
            m_state->m_notifyCapacity.store(true, std::memory_order_release);
            // 提交失败也归还了共享写槽，唤醒因满池暂停消费的专用通道。
            uv_async_send(&m_state->m_postAsync);
            return queued == LockFreeQueueAllocationFailure ? UV_ENOMEM : UV_ENOBUFS;
        }
    }
    // 先发布完整描述符再唤醒；成功后不能再读request，消费者可能已经回收。
    const int32_t notified = uv_async_send(&m_state->m_postAsync);
    if (notified != 0)
    {
        m_state->m_submissionError.store(notified, std::memory_order_release);
    }
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
    m_state->drain();
}

bool LibuvChannel::canDestroy() const
{
    return !m_state->m_polling && (m_state->m_group == nullptr || !m_state->m_group->m_running);
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