#include "LibuvBase.h"
#include "uv.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "LibuvTcp.h"
#include "LibuvClient.h"
#include "LibuvServer.h"
#include <memory>
#include <new>
#include <cstring>
#include <climits>
#include <algorithm>

std::atomic<uint32_t> LibuvBase::m_nextProducerToken(1);
#ifdef _MSC_VER
__declspec(thread) uint32_t LibuvBase::m_producerToken = 0;
#else
thread_local uint32_t LibuvBase::m_producerToken = 0;
#endif

LibuvSendLane::LibuvSendLane(uint32_t capacity) :
m_token(0),
m_published(0),
m_write(0),
m_seenRead(0),
m_consumed(0),
m_read(0),
m_items(capacity == 0 ? nullptr : new (std::nothrow) LibuvPendingSend[capacity]),
m_capacity(capacity)
{

}

LibuvSendLane::~LibuvSendLane()
{
    delete[] m_items;
}

size_t LibuvSendLane::pop(LibuvPendingSend* output, size_t maximum)
{
    const uint32_t available = m_published.load(std::memory_order_acquire) - m_read;
    const size_t taken = (std::min)(maximum, static_cast<size_t>(available));
    for (size_t index = 0; index < taken; ++index)
    {
        output[index] = m_items[(m_read + static_cast<uint32_t>(index)) & (m_capacity - 1)];
    }
    if (taken != 0)
    {
        m_read += static_cast<uint32_t>(taken);
        m_consumed.store(m_read, std::memory_order_release);
    }
    return taken;
}

bool LibuvBase::popSendPrefix(LibuvPendingSend* output, size_t maximum, size_t& taken)
{
    const bool lane = m_laneFirst;
    m_laneFirst = !m_laneFirst;
    taken = 0;
    if (lane)
    {
        taken = m_sendLane.pop(output, maximum);
        return taken != 0 || popParallelPrefix(output, maximum, taken);
    }
    if (popParallelPrefix(output, maximum, taken))
    {
        return true;
    }
    taken = m_sendLane.pop(output, maximum);
    return taken != 0;
}

bool LibuvBase::popParallelPrefix(LibuvPendingSend* output, size_t maximum, size_t& taken)
{
    const uint32_t lanes = m_sendProducers == nullptr ? 0 : m_settings.m_send.m_producerCacheCapacity;
    taken = 0;
    for (uint32_t scanned = 0; scanned <= lanes; ++scanned)
    {
        const uint32_t current = m_parallelCursor;
        m_parallelCursor = current == lanes ? 0 : current + 1;
        if (current == lanes)
        {
            if (m_queue->popReadyPrefix(output, maximum, taken))
            {
                return true;
            }
        }
        else
        {
            taken = m_sendProducers[current].m_lane.pop(output, maximum);
            if (taken != 0)
            {
                return true;
            }
        }
    }
    return false;
}

bool LibuvBase::sendsReady() const
{
    if (m_sendLane.m_published.load(std::memory_order_acquire) != m_sendLane.m_read || m_queue->possiblyReady())
    {
        return true;
    }
    for (uint32_t index = 0; m_sendProducers != nullptr && index < m_settings.m_send.m_producerCacheCapacity; ++index)
    {
        const LibuvSendLane& lane = m_sendProducers[index].m_lane;
        if (lane.m_published.load(std::memory_order_acquire) != lane.m_read)
        {
            return true;
        }
    }
    return false;
}

LibuvSendProducer::LibuvSendProducer() :
m_token(0),
m_busy(false),
m_lane(0)
{

}

uint32_t LibuvBase::allocateProducerToken()
{
    if (m_producerToken == 0)
    {
        uint32_t next = m_nextProducerToken.load(std::memory_order_relaxed);
        uint32_t attempts = 0;
        while (next != UINT32_MAX)
        {
            if (m_nextProducerToken.compare_exchange_weak(next, next + 1, std::memory_order_relaxed))
            {
                m_producerToken = next;
                return next;
            }
            WaitForContention(attempts);
        }
        m_producerToken = UINT32_MAX;
    }
    return m_producerToken;
}

LibuvLifecycleSlot::LibuvLifecycleSlot() :
m_token(0),
m_calls(0)
{

}

LibuvLifecycleGate::LibuvLifecycleGate() :
m_calls(0x80000000U)
{

}

void LibuvLifecycleGate::open()
{
    // 不重置线程槽：关闭后的失败进入者可能尚在退出，槽身份和计数覆盖整个外层对象生命期。
    m_calls.store(0, std::memory_order_release);
}

void LibuvLifecycleGate::close()
{
    m_calls.fetch_or(0x80000000U, std::memory_order_seq_cst);
    for (;;)
    {
        bool active = m_calls.load(std::memory_order_acquire) != 0x80000000U;
        for (uint32_t i = 0; i < 32; ++i)
        {
            active = m_slots[i].m_calls.load(std::memory_order_seq_cst) != 0 || active;
        }
        if (!active)
        {
            return;
        }
        uv_sleep(1);
    }
}

LibuvPendingWrite::LibuvPendingWrite() :
m_owner(nullptr),
m_buffer(nullptr),
m_charge(0),
m_next(nullptr)
{
    m_request.data = this;
}

LibuvReadCall::LibuvReadCall(LibuvBase& owner, char* bytes) :
m_owner(owner),
m_bytes(bytes)
{
}

LibuvReadCall::~LibuvReadCall()
{
    m_owner.releaseReadBytes(m_bytes);
}

/** 把公开数值配置转换为依赖库配置，不在此处创建池
@param [in] input 已校验的连接池参数
@return 独立配置副本，沿用依赖默认扩缩容策略
*/
static LockFreeHashMapPoolConfig LibuvConnectionIndexConfig(const LibuvConnectionPoolConfig& input)
{
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = input.m_initialCapacity;
    config.maximumCapacity = input.m_maximumCapacity;
    config.growFreeThreshold = input.m_growFreeThreshold;
    config.directoryCapacity = input.m_directoryCapacity;
    config.protectionRecordCount = input.m_protectionRecordCount;
    config.lookupCacheCapacity = input.m_lookupCacheCapacity;
    return config;
}

/** 把公开数值配置转换为依赖队列配置，不分配节点
@param [in] input 已校验的队列池参数
@return 供configurePool复制的配置
*/
static LockFreeQueuePoolConfig LibuvQueueConfig(const LibuvQueuePoolConfig& input)
{
    LockFreeQueuePoolConfig config;
    config.initialCapacity = input.m_initialCapacity;
    config.maximumCapacity = input.m_maximumCapacity;
    config.growThreshold = input.m_growThreshold;
    config.maximumProducers = input.m_maximumProducers;
    return config;
}

LibuvBase::LibuvBase(const LibuvPoolSettings& settings) :
m_settings(settings),
m_sendProducers(nullptr),
m_producerMask(settings.m_send.m_producerCacheCapacity != 0 &&
    (settings.m_send.m_producerCacheCapacity & (settings.m_send.m_producerCacheCapacity - 1)) == 0 ?
    settings.m_send.m_producerCacheCapacity - 1 : UINT32_MAX),
m_libuvTcp(nullptr),
m_loop(nullptr),
m_asyncHandle(nullptr),
m_queueReady(false),
m_queue(nullptr),
m_closeQueue(nullptr),
m_asyncCloseHandle(nullptr),
m_isClient(false),
m_connections(settings.m_connections.m_bucketCount, LibuvConnectionIndexConfig(settings.m_connections)),
m_stopping(false),
m_nextConnection(1),
m_loopInitialized(false),
m_closing(false),
m_readBuffers(nullptr),
m_cachedReads(0),
m_pendingBytes(0),
m_freeWrites(nullptr),
m_cachedWrites(0),
m_activeWrites(0),
m_ownerReady(false),
m_sendLane(settings.m_send.m_laneCapacity),
m_laneFirst(true),
m_parallelCursor(0)
{
    std::unique_ptr<uv_loop_t> loop(new uv_loop_t);
    std::unique_ptr<uv_async_t> send(new uv_async_t);
    std::unique_ptr<uv_async_t> stop(new uv_async_t);
    std::unique_ptr<LockFreeQueue<LibuvPendingSend>> queue(new LockFreeQueue<LibuvPendingSend>(false));
    std::unique_ptr<LockFreeQueue<LibuvPendingSend>> closes(new LockFreeQueue<LibuvPendingSend>(false));
    std::unique_ptr<char*[]> reads(settings.m_read.m_maximumCachedBuffers == 0 ? nullptr :
        new (std::nothrow) char*[settings.m_read.m_maximumCachedBuffers]);
    std::unique_ptr<LibuvSendProducer[]> producers(settings.m_send.m_producerCacheCapacity == 0 ? nullptr :
        new (std::nothrow) LibuvSendProducer[settings.m_send.m_producerCacheCapacity]);
    // 附加描述符总槽数不超过发送队列配置上限；每槽通道不大于主通道容量。
    // 保留足够突发容量，避免批量操作因过小通道持续等待；不运行时增长或换路。
    const uint32_t share = settings.m_send.m_producerCacheCapacity == 0 ? 0 :
        (std::min)(settings.m_send.m_laneCapacity,
            settings.m_sendQueue.m_maximumCapacity / settings.m_send.m_producerCacheCapacity);
    uint32_t laneCapacity = share == 0 ? 0 : 1;
    while (laneCapacity != 0 && laneCapacity <= share / 2)
    {
        laneCapacity *= 2;
    }
    bool lanesReady = true;
    for (uint32_t index = 0; producers.get() != nullptr && index < settings.m_send.m_producerCacheCapacity; ++index)
    {
        LibuvSendLane& lane = producers[index].m_lane;
        lane.m_capacity = laneCapacity;
        lane.m_items = laneCapacity == 0 ? nullptr : new (std::nothrow) LibuvPendingSend[laneCapacity];
        lanesReady = lanesReady && (laneCapacity == 0 || lane.m_items != nullptr);
    }
    LockFreeQueuePoolConfig queueConfig = LibuvQueueConfig(settings.m_sendQueue);
    // 每生产者顺序由发布协议保证；重叠调用不要求跨线程全局次序，允许取消未完成预约。
    m_queueReady = queue->configurePool(queueConfig) == LockFreeQueueSuccess &&
        queue->configureProducerCounters(true) == LockFreeQueueSuccess &&
        queue->configureCancelableReservations(true) == LockFreeQueueSuccess &&
        queue->tryInit() == LockFreeQueueSuccess;
    queueConfig = LibuvQueueConfig(settings.m_closeQueue);
    m_queueReady = closes->configurePool(queueConfig) == LockFreeQueueSuccess &&
        closes->tryInit() == LockFreeQueueSuccess && m_queueReady;
    m_queueReady = m_queueReady && lanesReady && settings.valid() &&
        (settings.m_send.m_laneCapacity == 0 || m_sendLane.m_items != nullptr) &&
        (settings.m_read.m_maximumCachedBuffers == 0 || reads.get() != nullptr) &&
        (settings.m_send.m_producerCacheCapacity == 0 || producers.get() != nullptr);
    m_readBuffers = reads.release();
    m_sendProducers = producers.release();
    m_loop = loop.release();
    m_asyncHandle = send.release();
    m_asyncCloseHandle = stop.release();
    m_queue = queue.release();
    m_closeQueue = closes.release();
}

LibuvBase::~LibuvBase()
{
    // 外层已经封闭借用并结束loop；所有上下文先归还，不能在队列销毁之后由成员析构触碰旧地址。
    for (size_t index = 0; m_sendProducers != nullptr && index < m_settings.m_send.m_producerCacheCapacity; ++index)
    {
        m_sendProducers[index].m_producer.release();
    }
    LibuvPendingSend pending;
    size_t taken = 0;
    while (popSendPrefix(&pending, 1, taken))
    {
        delete[] pending.m_buffer;
        releaseSend(pending.m_charge);
    }
    delete[] m_sendProducers;
    delete m_queue;
    delete m_closeQueue;
    delete m_asyncCloseHandle;
    delete m_asyncHandle;
    delete m_loop;
    while (m_cachedReads != 0)
    {
        delete[] m_readBuffers[--m_cachedReads];
    }
    delete[] m_readBuffers;
    while (m_freeWrites != nullptr)
    {
        LibuvPendingWrite* next = m_freeWrites->m_next;
        delete m_freeWrites;
        m_freeWrites = next;
    }
}

uv_tcp_t *LibuvBase::mainTcp()
{
    return m_isClient ? static_cast<LibuvClient *>(this)->m_dest : static_cast<LibuvServer *>(this)->m_server;
}

int32_t LibuvBase::initialize(LibuvTcp *owner)
{
    if (!m_queueReady || !m_pendingBytes.is_lock_free() || m_connections.status() != LockFreeHashMapOk)
    {
        return UV_ENOMEM;
    }
    m_libuvTcp = owner;
    int32_t result = uv_loop_init(m_loop);
    if (result != 0)
    {
        return result;
    }
    m_loopInitialized = true;
    result = uv_async_init(m_loop, m_asyncHandle, &SendPending);
    if (result != 0)
    {
        return result;
    }
    m_asyncHandle->data = this;
    result = uv_async_init(m_loop, m_asyncCloseHandle, &StopPending);
    if (result != 0)
    {
        return result;
    }
    m_asyncCloseHandle->data = this;
    result = uv_tcp_init(m_loop, mainTcp());
    if (result != 0)
    {
        return result;
    }
    mainTcp()->data = this;
    return 0;
}

bool LibuvBase::registerTcp(uv_tcp_t *tcp)
{
    if (m_stopping.load() || m_nextConnection == 0)
    {
        return false;
    }
    const uint64_t generation = m_nextConnection++;
    try
    {
        // 先准备 owner 索引，再向并发发送者发布；两侧成功后才通知业务连接可用。
        m_ownerConnections[tcp] = generation;
        if (m_connections.trySet(reinterpret_cast<uintptr_t>(tcp), generation, nullptr, true, false).m_committed)
        {
            return true;
        }
        m_ownerConnections.erase(tcp);
    }
    catch (...)
    {
        m_ownerConnections.erase(tcp);
    }
    m_stopping.store(true);
    LibuvBase::Wake(m_asyncCloseHandle);
    return false;
}

bool LibuvBase::connectionGeneration(uv_tcp_t* tcp, uint64_t& generation)
{
    if (m_connections.get(reinterpret_cast<uintptr_t>(tcp), generation))
    {
        return true;
    }
    decltype(m_connections)::Snapshot snapshot;
    const LockFreeHashMapResult found = m_connections.find(reinterpret_cast<uintptr_t>(tcp), snapshot);
    if (found.m_status == LockFreeHashMapOk)
    {
        generation = *snapshot.m_value;
        return true;
    }
    if (found.m_status != LockFreeHashMapNotFound)
    {
        m_stopping.store(true);
        LibuvBase::Wake(m_asyncCloseHandle);
    }
    return false;
}

void LibuvBase::closeTcp(uv_tcp_t *tcp)
{
    // 阻止 owner 继续使用旧代；生产者此前已排队的旧代操作随后直接回收。
    m_ownerConnections.erase(tcp);
    const LockFreeHashMapResult removed = m_connections.tryErase(reinterpret_cast<uintptr_t>(tcp), true, false);
    if (!removed.m_committed && removed.m_status != LockFreeHashMapNotFound)
    {
        // 索引撤销失败时全局拒绝发送，不能让残留地址指向已释放的TCP。
        m_stopping.store(true);
        if (!m_closing)
        {
            LibuvBase::Wake(m_asyncCloseHandle);
        }
    }
    m_connections.maintain(16);
    if (uv_is_closing(reinterpret_cast<uv_handle_t *>(tcp)))
    {
        return;
    }
    uv_read_stop(reinterpret_cast<uv_stream_t *>(tcp));
    uv_close(reinterpret_cast<uv_handle_t *>(tcp), &TcpClosed);
}

void LibuvBase::WalkClose(uv_handle_t *handle, void *context)
{
    LibuvBase *state = static_cast<LibuvBase *>(context);
    if (handle->data != state || uv_is_closing(handle))
    {
        return;
    }
    if (handle->type == UV_TCP)
    {
        state->closeTcp(reinterpret_cast<uv_tcp_t *>(handle));
    }
    else if (handle == reinterpret_cast<uv_handle_t *>(state->m_asyncHandle) ||
             handle == reinterpret_cast<uv_handle_t *>(state->m_asyncCloseHandle))
    {
        uv_close(handle, nullptr);
    }
}

void LibuvBase::beginClose()
{
    if (m_closing || !m_loopInitialized)
    {
        return;
    }
    m_closing = true;
    m_stopping.store(true);
    if (m_libuvTcp != nullptr)
    {
        // 外层借用涵盖队列发布和最后一次uv_async_send；归还后才能关闭async句柄。
        m_libuvTcp->closeSubmissions();
    }
    uv_walk(m_loop, &WalkClose, this);
}

void LibuvBase::drain()
{
    if (!m_loopInitialized)
    {
        return;
    }
    beginClose();
    uv_run(m_loop, UV_RUN_DEFAULT);
    if (uv_loop_close(m_loop) == 0)
    {
        m_loopInitialized = false;
    }
}

void LibuvBase::TcpClosed(uv_handle_t *handle)
{
    LibuvBase *state = static_cast<LibuvBase *>(handle->data);
    uv_tcp_t *tcp = reinterpret_cast<uv_tcp_t *>(handle);
    if (tcp != state->mainTcp())
    {
        delete tcp;
    }
}

void LibuvBase::AllocateRead(uv_handle_t *handle, size_t suggestedSize, uv_buf_t *buffer)
{
    LibuvBase* state = static_cast<LibuvBase*>(handle->data);
    const size_t size = (std::min)(suggestedSize, static_cast<size_t>(state->m_settings.m_read.m_bufferBytes));
    char *memory = size == 0 ? nullptr : state->acquireReadBytes();
    *buffer = uv_buf_init(memory, memory == nullptr ? 0 : static_cast<unsigned int>(size));
}

char* LibuvBase::acquireReadBytes()
{
    return m_cachedReads == 0 ? new (std::nothrow) char[m_settings.m_read.m_bufferBytes] : m_readBuffers[--m_cachedReads];
}

void LibuvBase::releaseReadBytes(char* bytes)
{
    if (bytes == nullptr)
    {
        return;
    }
    if (m_closing || m_cachedReads == m_settings.m_read.m_maximumCachedBuffers)
    {
        delete[] bytes;
        return;
    }
    m_readBuffers[m_cachedReads++] = bytes;
}

int32_t LibuvBase::startRead(uv_tcp_t *tcp)
{
    return uv_read_start(reinterpret_cast<uv_stream_t *>(tcp), &AllocateRead, &Read);
}

void LibuvBase::Read(uv_stream_t *sender, ssize_t size, const uv_buf_t *buffer)
{
    LibuvBase *state = static_cast<LibuvBase *>(sender->data);
    LibuvReadCall bytes(*state, buffer->base);
    LibuvTcp *owner = state->m_libuvTcp;
    if (owner == nullptr || state->m_stopping.load() || size == 0)
    {
        return;
    }
    uv_tcp_t *tcp = reinterpret_cast<uv_tcp_t *>(sender);
    if (uv_is_closing(reinterpret_cast<uv_handle_t*>(tcp)))
    {
        return;
    }
    try
    {
        if (size > 0)
        {
            owner->receive(tcp, buffer->base, static_cast<int32_t>(size));
            return;
        }
        state->failTcp(tcp);
    }
    catch (...)
    {
        state->closeTcp(tcp);
        owner->stop();
    }
}

void LibuvBase::Connected(uv_connect_t *request, int status)
{
    LibuvBase *state = static_cast<LibuvBase *>(request->data);
    LibuvTcp *owner = state->m_libuvTcp;
    if (owner == nullptr || state->m_stopping.load())
    {
        return;
    }
    try
    {
        if (status != 0)
        {
            owner->uvServerNotFindClear();
            owner->stop();
            owner->uvServerNotFind();
            return;
        }
        uv_tcp_t *tcp = state->mainTcp();
        uv_tcp_keepalive(tcp, 1, 5);
        if (!state->registerTcp(tcp) || state->startRead(tcp) != 0)
        {
            state->closeTcp(tcp);
            owner->stop();
            return;
        }
        owner->uvServerConnected(tcp);
    }
    catch (...)
    {
        owner->stop();
    }
}

void LibuvBase::Accepted(uv_stream_t *listener, int status)
{
    LibuvBase *state = static_cast<LibuvBase *>(listener->data);
    if (status < 0 || state->m_stopping.load() || state->m_libuvTcp == nullptr)
    {
        return;
    }
    uv_tcp_t *tcp = new (std::nothrow) uv_tcp_t;
    if (tcp == nullptr)
    {
        state->m_libuvTcp->stop();
        return;
    }
    if (uv_tcp_init(state->m_loop, tcp) != 0)
    {
        delete tcp;
        return;
    }
    tcp->data = state;
    if (uv_accept(listener, reinterpret_cast<uv_stream_t *>(tcp)) != 0 || !state->registerTcp(tcp) ||
        state->startRead(tcp) != 0)
    {
        state->closeTcp(tcp);
        return;
    }
    uv_tcp_keepalive(tcp, 1, 5);
    try
    {
        state->m_libuvTcp->uvClientConnected(tcp);
    }
    catch (...)
    {
        state->closeTcp(tcp);
        state->m_libuvTcp->stop();
    }
}

void LibuvBase::SendPending(uv_async_t *handle)
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
    LibuvBase *state = static_cast<LibuvBase *>(handle->data);
    LibuvPendingSend close;
    size_t closed = 0;
    while (closed < 128 && state->m_closeQueue->popReady(&close))
    {
        ++closed;
        state->processPending(close);
    }
    state->m_closeQueue->maintain(16);
    LibuvPendingSend* pending = state->m_pendingScratch;
    size_t count = 0;
    size_t consumed = 0;
    uint64_t bytes = 0;
    while (count < 1024 && (bytes < 524288U || state->m_stopping.load()))
    {
        const size_t available = state->m_stopping.load() ? 256 : state->m_settings.m_send.m_maximumActiveWrites - state->m_activeWrites;
        const size_t maximum = (std::min)(available, (std::min)(size_t(256), 1024 - count));
        consumed = 0;
        while (consumed < maximum)
        {
            size_t taken = 0;
            if (!state->popSendPrefix(pending + consumed, maximum - consumed, taken))
            {
                break;
            }
            bool small = true;
            for (size_t index = consumed; index < consumed + taken; ++index)
            {
                small = pending[index].m_charge <= 1028 + sizeof(uv_tcp_t*) + sizeof(LibuvPendingWrite) && small;
            }
            consumed += taken;
            // 队列一次仅保证有限前缀；小帧可继续拼接已有前缀，大帧立即处理以限制单轮字节开销。
            if (!small)
            {
                break;
            }
        }
        if (consumed == 0)
        {
            break;
        }
        count += consumed;
        // 前缀已全部取出；即便中途发生停止，剩余项也必须逐项释放，不能直接跳出。
        for (size_t index = 0; index < consumed; ++index)
        {
            bytes += pending[index].m_charge;
        }
        state->processPrefix(pending, consumed);
    }
    state->m_queue->maintain(16);
    state->m_connections.maintain(16);
    // 同时限制条数和字节，避免大包发送长期压住读事件及写完成回调。
    // 一个已取出的前缀必须完整处理；预算最多额外跨过一个 256 项前缀。
    const bool moreCloses = closed == 128 && state->m_closeQueue->possiblyReady();
    const bool moreData = (count >= 1024 || bytes >= 524288U) &&
        state->m_activeWrites < state->m_settings.m_send.m_maximumActiveWrites && state->sendsReady();
    if (!state->m_stopping.load() && (moreCloses || moreData))
    {
        LibuvBase::Wake(handle);
    }
}

bool LibuvBase::isLoopOwner() const
{
    if (!m_ownerReady.load(std::memory_order_acquire))
    {
        return false;
    }
    const uv_thread_t current = uv_thread_self();
    return uv_thread_equal(&current, &m_ownerThread) != 0;
}

void LibuvBase::WaitForContention(uint32_t& attempts)
{
    ++attempts;
    if ((attempts & 4095U) == 0)
    {
        uv_sleep(1);
    }
    else if ((attempts & 63U) == 0)
    {
        uv_sleep(0);
    }
}

void LibuvBase::WaitForSpace(uint32_t& attempts)
{
    if (attempts < 4096)
    {
        ++attempts;
    }
    if (attempts == 4096)
    {
        uv_sleep(1);
    }
    else
    {
        // 满队列时立即让出当前时间片，使唯一消费者能腾出空间。
        // 零时长只请求调度让步；达到4096次后仍按原契约每轮请求休眠1毫秒。
        uv_sleep(0);
    }
}

LibuvPendingWrite* LibuvBase::acquireWrite()
{
    LibuvPendingWrite* request = m_freeWrites;
    if (request == nullptr)
    {
        return new (std::nothrow) LibuvPendingWrite;
    }
    m_freeWrites = request->m_next;
    request->m_next = nullptr;
    --m_cachedWrites;
    return request;
}

void LibuvBase::releaseWrite(LibuvPendingWrite* request)
{
    delete[] request->m_buffer;
    releaseSend(request->m_charge);
    request->m_buffer = nullptr;
    request->m_charge = 0;
    request->m_owner = nullptr;
    if (m_closing || m_cachedWrites == m_settings.m_send.m_maximumCachedWrites)
    {
        delete request;
        return;
    }
    request->m_next = m_freeWrites;
    m_freeWrites = request;
    ++m_cachedWrites;
}

void LibuvBase::processPending(const LibuvPendingSend& pending, uint32_t written)
{
    std::unique_ptr<char[]> bytes(pending.m_buffer);
    const auto connection = m_ownerConnections.find(pending.m_target);
    const bool valid = !m_stopping.load() && connection != m_ownerConnections.end() &&
        connection->second == pending.m_generation;
    const bool inlineFrame = bytes.get() == nullptr && pending.m_wireSize >= 4 &&
        pending.m_wireSize <= sizeof(pending.m_inlineBytes);
    if (!valid || pending.m_close || (bytes.get() == nullptr && !inlineFrame))
    {
        bytes.reset();
        releaseSend(pending.m_charge);
        if (valid && pending.m_close)
        {
            closeTcp(pending.m_target);
        }
        return;
    }
    int32_t length = 0;
    std::memcpy(&length, inlineFrame ? pending.m_inlineBytes : bytes.get() + sizeof(uv_tcp_t*), sizeof(length));
    LibuvPendingWrite* request = length >= 0 && length <= INT_MAX - 4 ? acquireWrite() : nullptr;
    if (request == nullptr)
    {
        bytes.reset();
        releaseSend(pending.m_charge);
        m_libuvTcp->stop();
        return;
    }
    request->m_owner = this;
    request->m_buffer = bytes.release();
    request->m_charge = pending.m_charge;
    const uint32_t wireSize = pending.m_wireSize != 0 ? pending.m_wireSize : static_cast<uint32_t>(length + 4);
    if (inlineFrame)
    {
        std::memcpy(request->m_inlineBytes, pending.m_inlineBytes, wireSize);
    }
    uv_buf_t output = uv_buf_init(inlineFrame ? request->m_inlineBytes : request->m_buffer + sizeof(uv_tcp_t*), wireSize);
    // 能立即交给系统的部分不创建异步写积压；写队列非空时 libuv 会拒绝 try，保持原有顺序。
    // EAGAIN 只属于底层 try，普通提交继续交给 uv_write，不能作为 Busy 向上返回。
    // 小帧实测受益于同步快写；大帧直接异步写，避免额外系统调用及快写路径的尾延迟。
    const int32_t sent = written != 0 ? static_cast<int32_t>(written) : wireSize > 16388 ? UV_EAGAIN :
        uv_try_write(reinterpret_cast<uv_stream_t*>(pending.m_target), &output, 1);
    if (sent == static_cast<int32_t>(wireSize))
    {
        releaseWrite(request);
        return;
    }
    if (sent < 0 && sent != UV_EAGAIN && sent != UV_ENOSYS)
    {
        releaseWrite(request);
        failTcp(pending.m_target);
        return;
    }
    if (sent > 0)
    {
        output.base += sent;
        output.len -= static_cast<unsigned int>(sent);
    }
    const int32_t result = uv_write(&request->m_request,
        reinterpret_cast<uv_stream_t*>(pending.m_target), &output, 1, &Written);
    if (result != 0)
    {
        releaseWrite(request);
        failTcp(pending.m_target);
    }
    else
    {
        ++m_activeWrites;
    }
}

void LibuvBase::processPrefix(const LibuvPendingSend* pending, size_t count)
{
    for (size_t begin = 0; begin < count;)
    {
        size_t end = begin;
        uint32_t wireSize = 0;
        uint32_t charge = 0;
        while (end < count && pending[end].m_target == pending[begin].m_target &&
            pending[end].m_generation == pending[begin].m_generation && !pending[end].m_close &&
            (pending[end].m_buffer != nullptr || (pending[end].m_wireSize >= 4 && pending[end].m_wireSize <= 16)))
        {
            int32_t length = 0;
            const char* wire = pending[end].m_buffer != nullptr ?
                pending[end].m_buffer + sizeof(uv_tcp_t*) : pending[end].m_inlineBytes;
            std::memcpy(&length, wire, 4);
            const uint32_t size = pending[end].m_wireSize != 0 ? pending[end].m_wireSize :
                static_cast<uint32_t>(length) + 4;
            if (length < 0 || size > 1028 || size > 65536 - wireSize)
            {
                break;
            }
            wireSize += size;
            charge += pending[end].m_charge;
            ++end;
        }
        // 不为孤立消息增加分配；停止时逐项回收，避免无意义的复制和分配。
        if (end - begin < 2 || m_stopping.load())
        {
            processPending(pending[begin++]);
            continue;
        }
        if (wireSize <= 16388 && processDirectPrefix(pending + begin, end - begin, wireSize, charge))
        {
            begin = end;
            continue;
        }
        std::unique_ptr<char[]> merged(new (std::nothrow) char[sizeof(uv_tcp_t*) + wireSize]);
        if (!merged)
        {
            // 合批只是优化，辅助分配失败仍走原有逐帧发送，不丢消息或增加失败语义。
            processPending(pending[begin++]);
            continue;
        }
        uint32_t offset = static_cast<uint32_t>(sizeof(uv_tcp_t*));
        std::memcpy(merged.get(), &pending[begin].m_target, sizeof(uv_tcp_t*));
        for (size_t index = begin; index < end; ++index)
        {
            int32_t length = 0;
            const char* wire = pending[index].m_buffer != nullptr ?
                pending[index].m_buffer + sizeof(uv_tcp_t*) : pending[index].m_inlineBytes;
            std::memcpy(&length, wire, 4);
            const uint32_t size = pending[index].m_wireSize != 0 ? pending[index].m_wireSize :
                static_cast<uint32_t>(length) + 4;
            if (size <= 16)
            {
                copySmall(merged.get() + offset, wire, size);
            }
            else
            {
                std::memcpy(merged.get() + offset, wire, size);
            }
            offset += size;
            delete[] pending[index].m_buffer;
        }
        LibuvPendingSend group = pending[begin];
        group.m_buffer = merged.release();
        group.m_charge = charge;
        group.m_wireSize = wireSize;
        processPending(group);
        begin = end;
    }
}

bool LibuvBase::processDirectPrefix(const LibuvPendingSend* pending, size_t count, uint32_t wireSize, uint32_t charge)
{
    if (wireSize > 16388 || count < 2 || m_stopping.load())
    {
        return false;
    }
    const auto connection = m_ownerConnections.find(pending[0].m_target);
    if (connection == m_ownerConnections.end() || connection->second != pending[0].m_generation ||
        reinterpret_cast<uv_stream_t*>(pending[0].m_target)->write_queue_size != 0)
    {
        return false;
    }
    // 缓冲只借给同步系统调用；任何未写出的部分都先复制到有所有权的数组。
    char bytes[16388];
    uint32_t offset = 0;
    for (size_t index = 0; index < count; ++index)
    {
        const char* source = pending[index].m_buffer != nullptr ?
            pending[index].m_buffer + sizeof(uv_tcp_t*) : pending[index].m_inlineBytes;
        int32_t length = 0;
        std::memcpy(&length, source, 4);
        const uint32_t size = pending[index].m_wireSize != 0 ? pending[index].m_wireSize :
            static_cast<uint32_t>(length) + 4;
        if (size <= 16)
        {
            copySmall(bytes + offset, source, size);
        }
        else
        {
            std::memcpy(bytes + offset, source, size);
        }
        offset += size;
    }
    uv_buf_t output = uv_buf_init(bytes, wireSize);
    const int32_t sent = uv_try_write(reinterpret_cast<uv_stream_t*>(pending[0].m_target), &output, 1);
    if (sent == UV_EAGAIN || sent == UV_ENOSYS || sent == 0)
    {
        return false;
    }
    char* retained = nullptr;
    if (sent > 0 && static_cast<uint32_t>(sent) < wireSize)
    {
        retained = new (std::nothrow) char[sizeof(uv_tcp_t*) + wireSize];
        if (retained != nullptr)
        {
            std::memcpy(retained, &pending[0].m_target, sizeof(uv_tcp_t*));
            std::memcpy(retained + sizeof(uv_tcp_t*), bytes, wireSize);
        }
    }
    for (size_t index = 0; index < count; ++index)
    {
        delete[] pending[index].m_buffer;
    }
    if (retained != nullptr)
    {
        LibuvPendingSend group = pending[0];
        group.m_buffer = retained;
        group.m_charge = charge;
        group.m_wireSize = wireSize;
        processPending(group, static_cast<uint32_t>(sent));
        return true;
    }
    releaseSend(charge);
    if (sent != static_cast<int32_t>(wireSize))
    {
        // 部分成功后无法保留尾部时必须关闭，不能回退整帧重发造成字节重复。
        failTcp(pending[0].m_target);
    }
    return true;
}

void LibuvBase::failTcp(uv_tcp_t* tcp)
{
    if (uv_is_closing(reinterpret_cast<uv_handle_t*>(tcp)))
    {
        return;
    }
    closeTcp(tcp);
    if (m_libuvTcp == nullptr || m_stopping.load())
    {
        return;
    }
    try
    {
        m_libuvTcp->uvDisconnectedClear(tcp);
        if (m_isClient)
        {
            m_libuvTcp->stop();
            m_libuvTcp->uvServerDisconnected(tcp);
        }
        else
        {
            m_libuvTcp->uvClientDisconnected(tcp);
        }
    }
    catch (...)
    {
        m_libuvTcp->stop();
    }
}

void LibuvBase::StopPending(uv_async_t *handle)
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
    static_cast<LibuvBase *>(handle->data)->beginClose();
}

void LibuvBase::Written(uv_write_t *request, int status)
{
    LibuvPendingWrite* write = static_cast<LibuvPendingWrite*>(request->data);
    LibuvBase* owner = write->m_owner;
    uv_tcp_t* tcp = reinterpret_cast<uv_tcp_t*>(request->handle);
    const bool reopen = owner->m_activeWrites == owner->m_settings.m_send.m_maximumActiveWrites;
    --owner->m_activeWrites;
    owner->releaseWrite(write);
    if (status != 0)
    {
        owner->failTcp(tcp);
    }
    if (reopen && !owner->m_stopping.load() && owner->sendsReady())
    {
        Wake(owner->m_asyncHandle);
    }
}

int32_t LibuvBase::Wake(uv_async_t* handle)
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return uv_async_send(handle);
}