#include "LibuvChannelState.h"
#include <cstring>
#include <new>
#include <algorithm>

LibuvChannelState::LibuvChannelState() :
m_loopInitialized(false),
m_ownerThread(),
m_connectedPeers(new LockFreeHashMap<uint64_t, uint32_t>),
m_incoming(new LockFreeQueue<LibuvChannelWrite*>(false)),
m_notifyCapacity(false),
m_submissionError(0),
m_connectionVersion(1),
m_laneCount(0),
m_laneCursor(0),
m_writeConfig(new LibuvChannelWritePoolConfig),
m_peerPool(new LockFreeHashMapPool<LibuvChannelPeer>),
m_writePool(new LockFreeHashMapPool<LibuvChannelWrite>),
m_peerPoolStatus(0),
m_writePoolStatus(0),
m_readBufferBytes(16384),
m_loopStatus(0),
m_group(nullptr),
m_overflowClosing(false),
m_callback(nullptr),
m_postInitialized(false),
m_readyFirst(nullptr),
m_readyLast(nullptr),
m_nextPeer(1),
m_port(0),
m_started(false),
m_listenerInitialized(false),
m_stopping(false),
m_polling(false),
m_rejectPending(false),
m_acceptPending(false)
{
    LockFreeHashMapPoolConfig peers;
    peers.initialCapacity = 2;
    peers.maximumCapacity = 8388608 / (sizeof(LockFreeHashMapPoolSlot<LibuvChannelPeer>) + m_readBufferBytes);
    peers.directoryCapacity = 128;
    peers.lookupCacheCapacity = 0;
    m_peerPoolStatus = m_peerPool->initialize(peers) == LockFreeHashMapOk ? 0 : UV_ENOMEM;
    m_writePoolStatus = m_writePool->initialize(m_writeConfig->m_records) == LockFreeHashMapOk ? 0 : UV_ENOMEM;
    if (m_incoming->configurePool(m_writeConfig->m_submission) != LockFreeQueueSuccess ||
        m_incoming->tryInit() != LockFreeQueueSuccess || m_connectedPeers->status() != LockFreeHashMapOk ||
        !m_submissionGate.valid())
    {
        m_writePoolStatus = UV_ENOMEM;
    }
}

LibuvChannelState::~LibuvChannelState()
{
    if (!m_loopInitialized)
    {
        return;
    }
    shutdown();
    drain();
    releaseLanes();
    if (m_group != nullptr)
    {
        --m_group->m_channels;
    }
    else
    {
        uv_loop_close(&m_loop);
    }
}

int32_t LibuvChannelState::initialize()
{
    if (m_peerPoolStatus != 0 || m_writePoolStatus != 0)
    {
        return m_peerPoolStatus != 0 ? m_peerPoolStatus : m_writePoolStatus;
    }
    if (!m_loopInitialized)
    {
        m_group = LibuvHostLoop::current();
        if (m_group != nullptr)
        {
            ++m_group->m_channels;
            m_loopStatus = 0;
        }
        else
        {
            m_loopStatus = uv_loop_init(&m_loop);
        }
        m_loopInitialized = m_loopStatus == 0;
    }
    if (m_loopInitialized && !m_postInitialized)
    {
        m_loopStatus = prepareLanes();
        if (m_loopStatus != 0)
        {
            return m_loopStatus;
        }
        m_loopStatus = uv_async_init(loop(), &m_postAsync, posted);
        if (m_loopStatus == 0)
        {
            m_postInitialized = true;
            m_postAsync.data = this;
            uv_unref(reinterpret_cast<uv_handle_t*>(&m_postAsync));
            m_ownerThread = std::this_thread::get_id();
            m_submissionGate.open();
        }
    }
    return m_loopStatus;
}

uv_loop_t* LibuvChannelState::loop()
{
    return m_group == nullptr ? &m_loop : &m_group->m_loop;
}

void LibuvChannelState::activity()
{
    if (m_group != nullptr)
    {
        m_group->m_generation.fetch_add(1, std::memory_order_release);
    }
}

void LibuvChannelState::drain()
{
    if (!m_loopInitialized || m_polling || (m_group != nullptr && m_group->m_running))
    {
        return;
    }
    m_polling = true;
    if (m_group == nullptr)
    {
        uv_run(&m_loop, UV_RUN_DEFAULT);
    }
    else
    {
        // shutdown已关闭本通道所有句柄；只等这些关闭回调，不等其他通道结束。
        while (m_postInitialized || m_listenerInitialized || m_rejectPending || m_overflowClosing || !m_peers.empty())
        {
            m_group->run(UV_RUN_NOWAIT);
        }
    }
    m_polling = false;
}

void LibuvChannelState::listenerClosed(uv_handle_t* handle)
{
    LibuvChannelState* owner = static_cast<LibuvChannelState*>(handle->data);
    owner->m_listenerInitialized = false;
    owner->activity();
}

void LibuvChannelState::overflowClosed(uv_handle_t* handle)
{
    LibuvChannelState* owner = static_cast<LibuvChannelState*>(handle->data);
    owner->m_overflowClosing = false;
    owner->activity();
}

int32_t LibuvChannelState::parseAddress(const char* address, uint32_t port, sockaddr_storage& result)
{
    if (address == nullptr || port > 65535)
    {
        return UV_EINVAL;
    }
    std::memset(&result, 0, sizeof(result));
    int resultCode = uv_ip4_addr(address, static_cast<int>(port), reinterpret_cast<sockaddr_in*>(&result));
    if (resultCode != 0)
    {
        resultCode = uv_ip6_addr(address, static_cast<int>(port), reinterpret_cast<sockaddr_in6*>(&result));
    }
    return resultCode;
}

LibuvChannelPeer* LibuvChannelState::createPeer(bool rejectOnly)
{
    if (!m_loopInitialized || m_loopStatus != 0 || m_nextPeer == 0)
    {
        return nullptr;
    }
    LockFreeHashMapEpochGuard guard(m_peerPool->epoch());
    if (guard.record() == nullptr)
    {
        return nullptr;
    }
    LockFreeHashMapStatus allocated = LockFreeHashMapOk;
    LockFreeHashMapPoolSlot<LibuvChannelPeer>* slot = m_peerPool->acquire(*guard.record(), allocated);
    if (slot == nullptr)
    {
        return nullptr;
    }
    LibuvChannelPeer* peer = new (slot->payload()) LibuvChannelPeer(this, rejectOnly ? 0 : m_nextPeer++);
    slot->m_kind.store(1, std::memory_order_release);
    peer->m_storage = slot;
    if (peer->m_input == nullptr)
    {
        releasePeer(peer);
        return nullptr;
    }
    if (uv_tcp_init(loop(), &peer->m_tcp) != 0)
    {
        releasePeer(peer);
        return nullptr;
    }
    peer->m_tcp.data = peer;
    peer->m_connect.data = peer;
    if (rejectOnly)
    {
        m_rejectPending = true;
    }
    if (!rejectOnly)
    {
        try
        {
            m_peers.insert(std::make_pair(peer->m_id, peer));
        }
        catch (...)
        {
            peer->m_id = 0;
            peer->m_reportClose = false;
            m_rejectPending = true;
            closePeer(peer, UV_ENOMEM);
            return nullptr;
        }
    }
    return peer;
}

void LibuvChannelState::releasePeer(LibuvChannelPeer* peer)
{
    if (peer != nullptr)
    {
        m_peerPool->release(static_cast<LockFreeHashMapPoolSlot<LibuvChannelPeer>*>(peer->m_storage));
    }
}

LibuvChannelWrite* LibuvChannelState::prepareWrite(uint64_t peer, const uint8_t* data, uint32_t size, int32_t& result)
{
    result = 0;
    LockFreeHashMapEpochGuard guard(m_writePool->epoch());
    if (guard.record() == nullptr)
    {
        result = UV_ENOBUFS;
        return nullptr;
    }
    LockFreeHashMapStatus allocated = LockFreeHashMapOk;
    LockFreeHashMapPoolSlot<LibuvChannelWrite>* slot = m_writePool->acquire(*guard.record(), allocated);
    if (slot == nullptr)
    {
        result = allocated == LockFreeHashMapNoMemory ? UV_ENOMEM : UV_ENOBUFS;
        return nullptr;
    }
    try
    {
        LibuvChannelWrite* request = new (slot->payload()) LibuvChannelWrite();
        slot->m_kind.store(1, std::memory_order_release);
        request->m_storage = slot;
        request->m_peerId = peer;
        request->m_request.data = request;
        request->m_size = size;
        if (size <= sizeof(request->m_inline)) { std::memcpy(request->m_inline, data, size); }
        else { request->m_bytes.assign(data, data + size); }
        return request;
    }
    catch (...)
    {
        m_writePool->release(slot);
        result = UV_ENOMEM;
        return nullptr;
    }
}

void LibuvChannelState::releaseWrite(LibuvChannelWrite* request)
{
    if (request != nullptr)
    {
        m_writePool->release(static_cast<LockFreeHashMapPoolSlot<LibuvChannelWrite>*>(request->m_storage));
    }
}

void LibuvChannelState::appendWrite(LibuvChannelPeer* peer, LibuvChannelWrite* request)
{
    request->m_peer = peer;
    if (peer->m_postLast != nullptr) { peer->m_postLast->m_next = request; }
    else { peer->m_postFirst = request; }
    peer->m_postLast = request;
    peer->m_queuedBytes += request->m_size;
    ++peer->m_queuedWrites;
    queueReady(peer);
}

void LibuvChannelState::notifyCapacity(uint64_t exclude)
{
    if (m_stopping || !m_notifyCapacity.exchange(false, std::memory_order_acq_rel)) { return; }
    std::vector<uint64_t> ids;
    try
    {
        ids.reserve(m_peers.size());
        for (auto item = m_peers.begin(); item != m_peers.end(); ++item)
        {
            if (!item->second->m_closing && item->first != exclude) { ids.push_back(item->first); }
        }
    }
    catch (...)
    {
        m_notifyCapacity.store(true, std::memory_order_release);
        return;
    }
    for (size_t index = 0; index < ids.size(); ++index)
    {
        const auto found = m_peers.find(ids[index]);
        if (found == m_peers.end() || found->second->m_closing) { continue; }
        LibuvChannelPeer* peer = found->second;
        try { m_callback->onWritable(ids[index]); }
        catch (...) { closePeer(peer, UV_EPROTO); }
    }
}

void LibuvChannelState::queueReady(LibuvChannelPeer* peer)
{
    if (peer->m_readyQueued) { return; }
    peer->m_readyQueued = true;
    peer->m_readyPrevious = m_readyLast;
    peer->m_readyNext = nullptr;
    if (m_readyLast != nullptr) { m_readyLast->m_readyNext = peer; }
    else { m_readyFirst = peer; }
    m_readyLast = peer;
}

void LibuvChannelState::removeReady(LibuvChannelPeer* peer)
{
    if (!peer->m_readyQueued) { return; }
    if (peer->m_readyPrevious != nullptr) { peer->m_readyPrevious->m_readyNext = peer->m_readyNext; }
    else { m_readyFirst = peer->m_readyNext; }
    if (peer->m_readyNext != nullptr) { peer->m_readyNext->m_readyPrevious = peer->m_readyPrevious; }
    else { m_readyLast = peer->m_readyPrevious; }
    peer->m_readyPrevious = peer->m_readyNext = nullptr;
    peer->m_readyQueued = false;
}

void LibuvChannelState::releaseBatch(LibuvChannelWrite* first)
{
    while (first != nullptr)
    {
        LibuvChannelWrite* next = first->m_next;
        first->m_peer->m_queuedBytes -= first->m_size;
        --first->m_peer->m_queuedWrites;
        releaseWrite(first);
        first = next;
    }
}

void LibuvChannelState::posted(uv_async_t* handle)
{
    LibuvChannelState* owner = static_cast<LibuvChannelState*>(handle->data);
    for (uint32_t turn = 0; turn < 4096; ++turn)
    {
        LibuvChannelWrite* request = nullptr;
        if (!owner->m_incoming->popReady(&request))
        {
            break;
        }
        const auto found = owner->m_peers.find(request->m_peerId);
        if (owner->m_stopping || found == owner->m_peers.end() || found->second->m_closing)
        {
            owner->releaseWrite(request);
            continue;
        }
        owner->appendWrite(found->second, request);
    }
    const bool lanePending = owner->drainLanes();
    owner->m_incoming->maintain(16);
    owner->m_connectedPeers->maintain(16);
    // 每轮最多处理64个连接批次，随后把执行权交还loop；这不是接纳容量上限。
    for (uint32_t turn = 0; turn < 64 && !owner->m_stopping && owner->m_readyFirst != nullptr; ++turn)
    {
        LibuvChannelPeer* peer = owner->m_readyFirst;
        owner->removeReady(peer);
        LibuvChannelWrite* first = peer->m_postFirst;
        LibuvChannelWrite* last = nullptr;
        LibuvChannelWrite* next = first;
        uv_buf_t buffers[64];
        uint32_t count = 0;
        // libuv复制uv_buf_t描述，正文则由整条请求链保持到唯一完成回调。
        while (next != nullptr && count < 64)
        {
            buffers[count++] = uv_buf_init(reinterpret_cast<char*>(next->data()),
                static_cast<unsigned int>(next->m_size));
            last = next;
            next = next->m_next;
        }
        peer->m_postFirst = next;
        if (next == nullptr) { peer->m_postLast = nullptr; }
        else { owner->queueReady(peer); }
        last->m_next = nullptr;
        const int result = uv_write(&first->m_request, reinterpret_cast<uv_stream_t*>(&peer->m_tcp),
            buffers, count, written);
        owner->activity();
        if (result != 0)
        {
            // uv_write立即失败时不会再调用written，当前批次必须在此回收。
            owner->releaseBatch(first);
            owner->closePeer(peer, result);
        }
    }
    if (!owner->m_stopping && (lanePending || owner->m_readyFirst != nullptr || owner->m_incoming->possiblyReady()))
    {
        uv_async_send(&owner->m_postAsync);
    }
}

void LibuvChannelState::postClosed(uv_handle_t* handle)
{
    LibuvChannelState* owner = static_cast<LibuvChannelState*>(handle->data);
    owner->m_postInitialized = false;
    owner->activity();
}

int32_t LibuvChannelState::startRead(LibuvChannelPeer* peer)
{
    return uv_read_start(reinterpret_cast<uv_stream_t*>(&peer->m_tcp), &allocateRead, &read);
}

void LibuvChannelState::closePeer(LibuvChannelPeer* peer, int32_t reason)
{
    if (peer->m_closing)
    {
        return;
    }
    const LockFreeHashMapResult erased = m_connectedPeers->tryErase(peer->m_id, true, false);
    m_connectionVersion.fetch_add(1, std::memory_order_release);
    if (!erased.m_committed && erased.m_status != LockFreeHashMapNotFound)
    {
        m_submissionError.store(UV_EIO, std::memory_order_release);
    }
    peer->m_closing = true;
    peer->m_reason = reason;
    removeReady(peer);
    LibuvChannelWrite* pending = peer->m_postFirst;
    peer->m_postFirst = peer->m_postLast = nullptr;
    releaseBatch(pending);
    if (m_laneCount != 0 && !m_stopping)
    {
        // 取消释放的共享槽可能让其他连接的通道继续推进。
        uv_async_send(&m_postAsync);
    }
    uv_read_stop(reinterpret_cast<uv_stream_t*>(&peer->m_tcp));
    uv_close(reinterpret_cast<uv_handle_t*>(&peer->m_tcp), &closed);
}

void LibuvChannelState::shutdown()
{
    if (m_stopping)
    {
        return;
    }
    m_stopping = true;
    m_submissionGate.close();
    LibuvChannelWrite* submitted = nullptr;
    while (m_incoming->pop(&submitted))
    {
        releaseWrite(submitted);
    }
    m_connectedPeers->clear();
    if (m_postInitialized)
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_postAsync), postClosed);
    }
    if (m_listenerInitialized && !uv_is_closing(reinterpret_cast<uv_handle_t*>(&m_listener)))
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_listener), listenerClosed);
    }
    for (auto peer = m_peers.begin(); peer != m_peers.end(); ++peer)
    {
        closePeer(peer->second, UV_ECANCELED);
    }
}

void LibuvChannelState::acceptPeer(uv_stream_t* server, int status)
{
    LibuvChannelState* owner = static_cast<LibuvChannelState*>(server->data);
    owner->activity();
    if (status < 0 || owner->m_stopping)
    {
        return;
    }
    if (owner->m_rejectPending || owner->m_overflowClosing)
    {
        owner->m_acceptPending = true;
        return;
    }
    owner->m_acceptPending = false;
    LibuvChannelPeer* peer = owner->createPeer(false);
    if (peer == nullptr)
    {
        // 实际池耗尽时消费并拒绝这次连接，避免保留就绪反复唤醒造成空转。
        // 拒绝句柄嵌在实例中，不需要再向已耗尽的池申请槽位。
        if (uv_tcp_init(owner->loop(), &owner->m_overflow) == 0)
        {
            owner->m_overflow.data = owner;
            owner->m_overflowClosing = true;
            uv_accept(server, reinterpret_cast<uv_stream_t*>(&owner->m_overflow));
            uv_close(reinterpret_cast<uv_handle_t*>(&owner->m_overflow), &overflowClosed);
        }
        else
        {
            owner->shutdown();
        }
        return;
    }
    // 入站连接尚未交给上层，接纳或启动读取失败只回收资源，不能上报未知连接的终态。
    peer->m_reportClose = false;
    const int result = uv_accept(server, reinterpret_cast<uv_stream_t*>(&peer->m_tcp));
    if (result != 0)
    {
        owner->closePeer(peer, result);
        return;
    }
    if (!owner->m_connectedPeers->tryInsert(peer->m_id, 1U, true, false).m_committed)
    {
        owner->closePeer(peer, UV_ENOMEM);
        return;
    }
    owner->m_connectionVersion.fetch_add(1, std::memory_order_release);
    peer->m_connected = true;
    uv_tcp_nodelay(&peer->m_tcp, 1);
    const int readResult = owner->startRead(peer);
    if (readResult != 0)
    {
        owner->closePeer(peer, readResult);
        return;
    }
    // 从发布连接事件起才承担配对终态责任；回调抛异常也必须随后关闭。
    peer->m_reportClose = true;
    try
    {
        owner->m_callback->onConnected(peer->m_id);
    }
    catch (...)
    {
        owner->closePeer(peer, UV_EPROTO);
    }
}

void LibuvChannelState::connected(uv_connect_t* request, int status)
{
    LibuvChannelPeer* peer = static_cast<LibuvChannelPeer*>(request->data);
    peer->m_owner->activity();
    LibuvChannelState* owner = peer->m_owner;
    if (peer->m_closing)
    {
        return;
    }
    if (status != 0 || owner->m_stopping)
    {
        owner->closePeer(peer, status != 0 ? status : UV_ECANCELED);
        return;
    }
    if (!owner->m_connectedPeers->tryInsert(peer->m_id, 1U, true, false).m_committed)
    {
        owner->closePeer(peer, UV_ENOMEM);
        return;
    }
    owner->m_connectionVersion.fetch_add(1, std::memory_order_release);
    peer->m_connected = true;
    uv_tcp_nodelay(&peer->m_tcp, 1);
    const int result = owner->startRead(peer);
    if (result != 0)
    {
        owner->closePeer(peer, result);
        return;
    }
    try
    {
        owner->m_callback->onConnected(peer->m_id);
    }
    catch (...)
    {
        owner->closePeer(peer, UV_EPROTO);
    }
}

void LibuvChannelState::allocateRead(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buffer)
{
    (void)suggestedSize;
    LibuvChannelPeer* peer = static_cast<LibuvChannelPeer*>(handle->data);
    peer->m_owner->activity();
    *buffer = uv_buf_init(reinterpret_cast<char*>(peer->m_input), peer->m_owner->m_readBufferBytes);
}

void LibuvChannelState::read(uv_stream_t* stream, ssize_t size, const uv_buf_t* buffer)
{
    LibuvChannelPeer* peer = static_cast<LibuvChannelPeer*>(stream->data);
    peer->m_owner->activity();
    LibuvChannelState* owner = peer->m_owner;
    if (peer->m_closing || size == 0)
    {
        return;
    }
    if (size < 0)
    {
        owner->closePeer(peer, static_cast<int32_t>(size));
        return;
    }
    try
    {
        owner->m_callback->onData(peer->m_id, reinterpret_cast<const uint8_t*>(buffer->base),
            static_cast<uint32_t>(size));
    }
    catch (...)
    {
        owner->closePeer(peer, UV_EPROTO);
    }
}

void LibuvChannelState::written(uv_write_t* request, int status)
{
    LibuvChannelWrite* write = static_cast<LibuvChannelWrite*>(request->data);
    LibuvChannelPeer* peer = write->m_peer;
    LibuvChannelState* owner = peer->m_owner;
    owner->activity();
    owner->releaseBatch(write);
    if (owner->m_laneCount != 0 && !owner->m_stopping)
    {
        // 满池时消费通道不自旋，实际写完成归还槽后再唤醒。
        uv_async_send(&owner->m_postAsync);
    }
    // 先唤醒没有自身完成回调的等待者，避免共享池由原连接反复独占。
    owner->notifyCapacity(peer->m_id);
    if (peer->m_closing)
    {
        return;
    }
    if (status != 0)
    {
        owner->closePeer(peer, status);
        return;
    }
    try
    {
        owner->m_callback->onWritable(peer->m_id);
    }
    catch (...)
    {
        owner->closePeer(peer, UV_EPROTO);
    }
}

void LibuvChannelState::closed(uv_handle_t* handle)
{
    LibuvChannelPeer* peer = static_cast<LibuvChannelPeer*>(handle->data);
    peer->m_owner->activity();
    LibuvChannelState* owner = peer->m_owner;
    const uint64_t id = peer->m_id;
    const int32_t reason = peer->m_reason;
    const bool report = peer->m_reportClose;
    owner->m_peers.erase(id);
    owner->releasePeer(peer);
    if (id == 0)
    {
        owner->m_rejectPending = false;
        // 不在关闭回调里再次接纳。持续分配失败时，立即重试会再次生成关闭句柄，
        // 使libuv同一轮关闭回调始终排不空；保留m_acceptPending交下一次poll重试。
    }
    owner->notifyCapacity();
    if (report)
    {
        try
        {
            owner->m_callback->onClosed(id, reason);
        }
        catch (...)
        {
            // Closing already completed; no exception may cross a libuv callback.
        }
    }
}

int32_t LibuvChannelState::prepareLanes()
{
    if (m_lanes || m_writeConfig->m_records.initialCapacity < 64)
    {
        return 0;
    }
    const size_t budget = m_writeConfig->m_records.initialCapacity / 2;
    m_laneCount = static_cast<uint32_t>((std::min)(size_t(16), m_writeConfig->m_submission.maximumProducers));
    if (m_laneCount == 0)
    {
        return 0;
    }
    try
    {
        m_lanes.reset(new LibuvChannelLane[m_laneCount]);
        for (uint32_t index = 0; index < m_laneCount; ++index)
        {
            const size_t available = m_laneCount == 1 ? budget :
                (index == 0 ? budget / 2 : budget / 2 / (m_laneCount - 1));
            uint32_t capacity = 1;
            while (capacity <= available / 2 && capacity < 1048576U)
            {
                capacity *= 2;
            }
            LibuvChannelLane& lane = m_lanes[index];
            lane.m_items.reset(new LibuvChannelWrite*[capacity]());
            lane.m_capacity = capacity;
            for (uint32_t item = 0; item < capacity; ++item)
            {
                int32_t result = 0;
                lane.m_items[item] = prepareWrite(0, reinterpret_cast<const uint8_t*>(""), 0, result);
                if (lane.m_items[item] == nullptr)
                {
                    releaseLanes();
                    return result;
                }
            }
        }
    }
    catch (...)
    {
        releaseLanes();
        return UV_ENOMEM;
    }
    return 0;
}

void LibuvChannelState::releaseLanes()
{
    if (m_lanes)
    {
        for (uint32_t index = 0; index < m_laneCount; ++index)
        {
            LibuvChannelLane& lane = m_lanes[index];
            for (uint32_t item = 0; item < lane.m_capacity; ++item)
            {
                releaseWrite(lane.m_items[item]);
            }
        }
    }
    m_lanes.reset();
    m_laneCount = 0;
}

LibuvChannelLane* LibuvChannelState::producerLane()
{
    if (m_laneCount == 0)
    {
        return nullptr;
    }
    const uint32_t token = LibuvBase::producerToken();
    if (token == UINT32_MAX)
    {
        return nullptr;
    }
    LibuvChannelLane& primary = m_lanes[0];
    uint32_t owner = primary.m_token.load(std::memory_order_acquire);
    if (owner == 0 && primary.m_token.compare_exchange_strong(owner, token,
        std::memory_order_acq_rel, std::memory_order_acquire))
    {
        owner = token;
    }
    if (owner == token)
    {
        return &primary;
    }
    if (m_laneCount == 1)
    {
        return nullptr;
    }
    LibuvChannelLane& lane = m_lanes[1 + token % (m_laneCount - 1)];
    owner = lane.m_token.load(std::memory_order_acquire);
    if (owner == 0 && lane.m_token.compare_exchange_strong(owner, token,
        std::memory_order_acq_rel, std::memory_order_acquire))
    {
        owner = token;
    }
    return owner == token ? &lane : nullptr;
}

int32_t LibuvChannelState::postLane(LibuvChannelLane& lane, uint64_t peer, const uint8_t* data, uint32_t size)
{
    const uint64_t version = m_connectionVersion.load(std::memory_order_acquire);
    if (lane.m_peer != peer || lane.m_version != version)
    {
        LockFreeHashMap<uint64_t, uint32_t>::Snapshot connected;
        const LockFreeHashMapResult found = m_connectedPeers->find(peer, connected);
        if (found.m_status != LockFreeHashMapOk)
        {
            return found.m_status == LockFreeHashMapNotFound ? UV_ENOTCONN : UV_ENOMEM;
        }
        lane.m_peer = peer;
        lane.m_version = version;
    }
    if (size == 0)
    {
        return 0;
    }
    if (lane.m_write - lane.m_seenRead >= lane.m_capacity)
    {
        lane.m_seenRead = lane.m_consumed.load(std::memory_order_acquire);
        if (lane.m_write - lane.m_seenRead >= lane.m_capacity)
        {
            return UV_ENOBUFS;
        }
    }
    LibuvChannelWrite* request = lane.m_items[lane.m_write & (lane.m_capacity - 1)];
    try
    {
        if (size <= sizeof(request->m_inline))
        {
            if (size <= 16)
            {
                LibuvBase::copySmall(reinterpret_cast<char*>(request->m_inline), reinterpret_cast<const char*>(data), size);
            }
            else
            {
                std::memcpy(request->m_inline, data, size);
            }
        }
        else
        {
            request->m_bytes.assign(data, data + size);
        }
    }
    catch (...)
    {
        return UV_ENOMEM;
    }
    request->m_peerId = peer;
    request->m_size = size;
    ++lane.m_write;
    lane.m_published.store(lane.m_write, std::memory_order_release);
    // 正文及描述符先发布，再通知libuv；不得仅依赖某个CPU的强内存序。
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return 0;
}

bool LibuvChannelState::drainLanes()
{
    uint8_t bytes[65536];
    uint32_t budget = 16384;
    const uint32_t maximum = (std::min)(static_cast<uint32_t>(sizeof(bytes)), m_writeConfig->m_maximumWriteBytes);
    for (uint32_t visited = 0; visited < m_laneCount && budget != 0; ++visited)
    {
        LibuvChannelLane& lane = m_lanes[m_laneCursor];
        m_laneCursor = (m_laneCursor + 1) % m_laneCount;
        // 与生产者发布后的SC屏障配对：要么本轮看到正文，要么生产者看到撤销并重新唤醒。
        lane.m_notified.store(false, std::memory_order_release);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        const uint64_t published = lane.m_published.load(std::memory_order_acquire);
        if (lane.m_read == published)
        {
            continue;
        }
        LibuvChannelWrite* first = lane.m_items[lane.m_read & (lane.m_capacity - 1)];
        const uint64_t id = first->m_peerId;
        const auto found = m_peers.find(id);
        const bool active = !m_stopping && found != m_peers.end() && !found->second->m_closing;
        if (active && first->m_size > sizeof(first->m_inline))
        {
            // 大正文已经由生产者复制到独占vector；只转移所有权，不再次经过栈合批缓冲。
            // 每次最多64条交回轮转，槽在所有权交接后才能被生产者复用。
            uint32_t moved = 0;
            while (lane.m_read != published && moved < 64 && budget != 0)
            {
                LibuvChannelWrite* item = lane.m_items[lane.m_read & (lane.m_capacity - 1)];
                if (item->m_peerId != id || item->m_size <= sizeof(item->m_inline))
                {
                    break;
                }
                int32_t result = 0;
                LibuvChannelWrite* write = prepareWrite(id, reinterpret_cast<const uint8_t*>(""), 0, result);
                if (write == nullptr)
                {
                    if (result != UV_ENOBUFS)
                    {
                        closePeer(found->second, result);
                    }
                    break;
                }
                write->m_size = item->m_size;
                write->m_bytes.swap(item->m_bytes);
                appendWrite(found->second, write);
                ++lane.m_read;
                ++moved;
                --budget;
            }
            lane.m_consumed.store(lane.m_read, std::memory_order_release);
            continue;
        }
        uint32_t count = 0, size = 0;
        const uint32_t limit = (std::min)(budget, 4096U);
        while (lane.m_read + count != published && count < limit)
        {
            LibuvChannelWrite* item = lane.m_items[(lane.m_read + count) & (lane.m_capacity - 1)];
            if (item->m_peerId != id)
            {
                break;
            }
            if (active)
            {
                if (item->m_size > sizeof(item->m_inline) || item->m_size > maximum - size)
                {
                    break;
                }
                if (item->m_size <= 16)
                {
                    LibuvBase::copySmall(reinterpret_cast<char*>(bytes + size), reinterpret_cast<const char*>(item->data()), item->m_size);
                }
                else
                {
                    std::memcpy(bytes + size, item->data(), item->m_size);
                }
                size += item->m_size;
            }
            ++count;
        }
        if (active)
        {
            // 首条已通过单次正文上限检查且属于内联正文，至少一条能够放入合批缓冲。
            int32_t result = 0;
            LibuvChannelWrite* write = prepareWrite(id, bytes, size, result);
            if (write == nullptr)
            {
                if (result != UV_ENOBUFS)
                {
                    closePeer(found->second, result);
                }
                continue;
            }
            appendWrite(found->second, write);
        }
        lane.m_read += count;
        budget -= count;
        lane.m_consumed.store(lane.m_read, std::memory_order_release);
    }
    for (uint32_t index = 0; index < m_laneCount; ++index)
    {
        if (m_lanes[index].m_read != m_lanes[index].m_published.load(std::memory_order_acquire))
        {
            // 无进展说明写池暂时不可用；等待写完成、取消或新的生产者提交。
            return budget != 16384;
        }
    }
    return false;
}