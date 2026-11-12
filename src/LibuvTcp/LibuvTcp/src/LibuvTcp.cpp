#include "LibuvHostState.h"
#include "LibuvTcp.h"
#include "LibuvPoolSettings.h"
#include "LibuvBase.h"
#include "LibuvClient.h"
#include "LibuvServer.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <memory>
#include <cstring>
#include <climits>
#include <cstdio>
#include <new>

#ifdef _WIN32
#pragma comment(lib, "Dbghelp.lib")
#endif

LibuvTcp::LibuvTcp() :
m_libuv(nullptr),
m_lifecycle(nullptr),
m_settings(nullptr),
m_running(false)
{
    std::unique_ptr<LibuvLifecycleGate> lifecycle(new LibuvLifecycleGate());
    m_settings = new LibuvPoolSettings();
    m_lifecycle = lifecycle.release();
}

LibuvTcp::~LibuvTcp()
{
    stop();
    // 宿主仍须先停止/等待派生对象；严禁回调中delete this或并发开始init/loop。
    while (m_running.load(std::memory_order_acquire))
    {
        uv_sleep(1);
    }
    closeSubmissions();
    LibuvBase *state = m_libuv;
    m_libuv = nullptr;
    if (state != nullptr)
    {
        state->m_libuvTcp = nullptr;
    }
    DestroyState(state);
    delete m_lifecycle;
    delete m_settings;
}

void LibuvTcp::closeSubmissions()
{
    m_lifecycle->close();
}

void LibuvTcp::DestroyState(LibuvBase *state)
{
    if (state == nullptr)
    {
        return;
    }
    state->drain();
    if (state->m_loopInitialized)
    {
        // 外部通过loopPtr添加的句柄仍须由外部先关闭，不能释放仍忙碌的循环内存。
        std::fputs("LibuvTcp loop still busy: external handles must be closed before teardown\n", stderr);
        return;
    }
    if (state->m_isClient)
    {
        delete static_cast<LibuvClient *>(state);
    }
    else
    {
        delete static_cast<LibuvServer *>(state);
    }
}

int32_t LibuvTcp::configureSendQueue(const LibuvQueuePoolConfig& config)
{
    if (m_libuv != nullptr || m_running.load(std::memory_order_acquire))
    {
        return UV_EBUSY;
    }
    if (!config.valid())
    {
        return UV_EINVAL;
    }
    m_settings->m_sendQueue = config;
    return 0;
}

int32_t LibuvTcp::configureCloseQueue(const LibuvQueuePoolConfig& config)
{
    if (m_libuv != nullptr || m_running.load(std::memory_order_acquire))
    {
        return UV_EBUSY;
    }
    if (!config.valid())
    {
        return UV_EINVAL;
    }
    m_settings->m_closeQueue = config;
    return 0;
}

int32_t LibuvTcp::configureConnectionPool(const LibuvConnectionPoolConfig& config)
{
    if (m_libuv != nullptr || m_running.load(std::memory_order_acquire))
    {
        return UV_EBUSY;
    }
    if (!config.valid())
    {
        return UV_EINVAL;
    }
    m_settings->m_connections = config;
    return 0;
}

int32_t LibuvTcp::configureSendPool(const LibuvSendPoolConfig& config)
{
    if (m_libuv != nullptr || m_running.load(std::memory_order_acquire))
    {
        return UV_EBUSY;
    }
    if (!config.valid())
    {
        return UV_EINVAL;
    }
    m_settings->m_send = config;
    return 0;
}

int32_t LibuvTcp::configureReadPool(const LibuvReadPoolConfig& config)
{
    if (m_libuv != nullptr || m_running.load(std::memory_order_acquire))
    {
        return UV_EBUSY;
    }
    if (!config.valid())
    {
        return UV_EINVAL;
    }
    m_settings->m_read = config;
    return 0;
}

bool LibuvTcp::initClient(const char *ip, int32_t port)
{
    if (!m_lifecycle->valid() || !m_settings->valid())
    {
        return false;
    }
    sockaddr_in address = {};
    if (ip == nullptr || port < 1 || port > 65535 || uv_ip4_addr(ip, port, &address) != 0)
    {
        return false;
    }
    // init由同一生命周期owner串行调用；运行期间不能重新初始化。
    if (m_running.load(std::memory_order_acquire) || m_libuv != nullptr)
    {
        return false;
    }
    std::unique_ptr<LibuvClient> state;
    try
    {
        state.reset(new LibuvClient(*m_settings));
    }
    catch (...)
    {
        return false;
    }
    int32_t result = state->initialize(this);
    if (result == 0)
    {
        state->m_connect->data = state.get();
        result = uv_tcp_connect(
            state->m_connect, state->m_dest, reinterpret_cast<const sockaddr *>(&address), &LibuvBase::Connected);
    }
    if (result != 0)
    {
        state->m_libuvTcp = nullptr;
        DestroyState(state.release());
        return false;
    }
    m_libuv = state.release();
    m_lifecycle->open();
    return true;
}

bool LibuvTcp::initServer(int32_t port, int32_t backlog)
{
    return initServer("0.0.0.0", port, backlog);
}

bool LibuvTcp::initServer(const char* ip, int32_t port, int32_t backlog)
{
    if (!m_lifecycle->valid() || !m_settings->valid())
    {
        return false;
    }
    sockaddr_in address = {};
    if (ip == nullptr || port < 0 || port > 65535 || backlog < 1 || uv_ip4_addr(ip, port, &address) != 0)
    {
        return false;
    }
    // init由同一生命周期owner串行调用；运行期间不能重新初始化。
    if (m_running.load(std::memory_order_acquire) || m_libuv != nullptr)
    {
        return false;
    }
    std::unique_ptr<LibuvServer> state;
    try
    {
        state.reset(new LibuvServer(*m_settings));
    }
    catch (...)
    {
        return false;
    }
    int32_t result = state->initialize(this);
    if (result == 0)
    {
        result = uv_tcp_bind(state->m_server, reinterpret_cast<const sockaddr *>(&address), 0);
    }
    if (result == 0)
    {
        result = uv_listen(reinterpret_cast<uv_stream_t *>(state->m_server), backlog, &LibuvBase::Accepted);
    }
    if (result != 0)
    {
        state->m_libuvTcp = nullptr;
        DestroyState(state.release());
        return false;
    }
    m_libuv = state.release();
    m_lifecycle->open();
    return true;
}

void LibuvTcp::loop()
{
    if (!m_lifecycle->valid())
    {
        return;
    }
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true))
    {
        return;
    }
    LibuvBase *state = m_libuv;
    if (state == nullptr)
    {
        m_running.store(false, std::memory_order_release);
        return;
    }
    state->m_ownerThread = uv_thread_self();
    state->m_ownerReady.store(true, std::memory_order_release);
    uv_run(state->m_loop, UV_RUN_DEFAULT);
    state->drain();
    closeSubmissions();
    m_libuv = nullptr;
    DestroyState(state);
    // 最后访问外层对象；析构等待者观察到false后才允许销毁计数器。
    m_running.store(false, std::memory_order_release);
}

void LibuvTcp::stop()
{
    LibuvCall call(*m_lifecycle);
    if (!call.accepted())
    {
        return;
    }
    LibuvBase* state = m_libuv;
    if (!state->m_stopping.exchange(true))
    {
        LibuvBase::Wake(state->m_asyncCloseHandle);
    }
}

char *LibuvTcp::memoryForSend(int32_t length)
{
    return length <= 0 ? nullptr : new char[static_cast<size_t>(length)];
}

void LibuvTcp::releaseSendMemory(const char* buffer)
{
    delete[] buffer;
}

bool LibuvTcp::send(const char *buffer)
{
    std::unique_ptr<char[]> bytes(const_cast<char *>(buffer));
    if (trySend(buffer, true, true) != 0)
    {
        return false;
    }
    bytes.release();
    return true;
}

int32_t LibuvTcp::trySend(const char* buffer, bool retryContention, bool waitForSpace)
{
    return trySubmitBuffer(buffer, retryContention, waitForSpace);
}

int32_t LibuvTcp::trySubmitBuffer(const char* buffer, bool retryContention, bool waitForSpace)
{
    if (buffer == nullptr || !m_lifecycle->valid())
    {
        return UV_EINVAL;
    }
    LibuvPendingSend pending;
    std::memcpy(&pending.m_target, buffer, sizeof(pending.m_target));
    int32_t length = 0;
    std::memcpy(&length, buffer + sizeof(uv_tcp_t *), sizeof(length));
    if (length < 0 || length > INT_MAX - 4 || pending.m_target == nullptr)
    {
        return UV_EINVAL;
    }
    pending.m_buffer = const_cast<char*>(buffer);
    pending.m_charge = static_cast<uint32_t>(length) + static_cast<uint32_t>(sizeof(uv_tcp_t*) + 4 + sizeof(LibuvPendingWrite));
    if (length <= 12)
    {
        pending.m_buffer = nullptr;
        pending.m_wireSize = static_cast<uint32_t>(length + 4);
        std::memcpy(pending.m_inlineBytes, buffer + sizeof(uv_tcp_t*), length + 4);
    }
    const int32_t result = trySubmit(pending, retryContention, waitForSpace);
    if (result == 0 && pending.m_buffer == nullptr)
    {
        // 副本已经发布；原数组在分配线程释放，减少跨线程堆同步。
        delete[] buffer;
    }
    return result;
}

bool LibuvTcp::send(uv_tcp_t* target, const char* data, uint32_t size)
{
    return trySend(target, data, size, true, true) == 0;
}

int32_t LibuvTcp::trySend(uv_tcp_t* target, const char* data, uint32_t size,
    bool retryContention, bool waitForSpace)
{
    return trySubmitData(target, data, size, retryContention, waitForSpace);
}

int32_t LibuvTcp::trySubmitData(uv_tcp_t* target, const char* data, uint32_t size,
    bool retryContention, bool waitForSpace)
{
    const uint32_t overhead = static_cast<uint32_t>(sizeof(target) + 4 + sizeof(LibuvPendingWrite));
    if (target == nullptr || (data == nullptr && size != 0) || size > 0x7fffffffU - overhead || !m_lifecycle->valid())
    {
        return UV_EINVAL;
    }
    LibuvPendingSend pending;
    pending.m_target = target;
    pending.m_charge = size + overhead;
    pending.m_wireSize = size + 4;
    if (pending.m_wireSize <= sizeof(pending.m_inlineBytes))
    {
        std::memcpy(pending.m_inlineBytes, &size, 4);
        LibuvBase::copySmall(pending.m_inlineBytes + 4, data, size);
        return trySubmit(pending, retryContention, waitForSpace);
    }
    if (pending.m_charge > m_settings->m_send.m_maximumPendingBytes)
    {
        return UV_EINVAL;
    }
    std::unique_ptr<char[]> bytes(new (std::nothrow) char[sizeof(target) + pending.m_wireSize]);
    if (!bytes)
    {
        return UV_ENOMEM;
    }
    pending.m_buffer = bytes.get();
    char* wire = bytes.get() + sizeof(target);
    std::memcpy(wire, &size, 4);
    if (size != 0)
    {
        std::memcpy(wire + 4, data, size);
    }
    const int32_t result = trySubmit(pending, retryContention, waitForSpace);
    if (result == 0)
    {
        bytes.release();
    }
    return result;
}

bool LibuvTcp::sendBatch(uv_tcp_t* target, const LibuvSendBuffer* buffers, uint32_t count)
{
    return trySendBatch(target, buffers, count, true, true) == 0;
}

int32_t LibuvTcp::trySendBatch(uv_tcp_t* target, const LibuvSendBuffer* buffers, uint32_t count,
    bool retryContention, bool waitForSpace)
{
    return trySubmitBatch(target, buffers, count, retryContention, waitForSpace);
}

int32_t LibuvTcp::trySubmitBatch(uv_tcp_t* target, const LibuvSendBuffer* buffers, uint32_t count,
    bool retryContention, bool waitForSpace)
{
    if (target == nullptr || buffers == nullptr || count == 0 || count > 4096 || !m_lifecycle->valid())
    {
        return UV_EINVAL;
    }
    uint32_t wireSize = 0;
    for (uint32_t index = 0; index < count; ++index)
    {
        const uint32_t size = buffers[index].m_size;
        if ((size != 0 && buffers[index].m_data == nullptr) || size > 1048576U - 4 ||
            wireSize > 1048576U - 4 - size)
        {
            return UV_EINVAL;
        }
        wireSize += size + 4;
    }
    if (wireSize + sizeof(target) + sizeof(LibuvPendingWrite) > m_settings->m_send.m_maximumPendingBytes)
    {
        return UV_EINVAL;
    }
    std::unique_ptr<char[]> bytes(new (std::nothrow) char[sizeof(target) + wireSize]);
    if (!bytes)
    {
        return UV_ENOMEM;
    }
    std::memcpy(bytes.get(), &target, sizeof(target));
    size_t offset = sizeof(target);
    for (uint32_t index = 0; index < count; ++index)
    {
        const uint32_t size = buffers[index].m_size;
        std::memcpy(bytes.get() + offset, &size, 4);
        offset += 4;
        if (size != 0)
        {
            std::memcpy(bytes.get() + offset, buffers[index].m_data, size);
            offset += size;
        }
    }
    LibuvPendingSend pending;
    pending.m_target = target;
    pending.m_buffer = bytes.get();
    pending.m_wireSize = wireSize;
    pending.m_charge = wireSize + static_cast<uint32_t>(sizeof(target) + sizeof(LibuvPendingWrite));
    const int32_t result = trySubmit(pending, retryContention, waitForSpace);
    if (result == 0)
    {
        bytes.release();
    }
    return result;
}

bool LibuvTcp::post(const char* buffer)
{
    std::unique_ptr<char[]> bytes(const_cast<char*>(buffer));
    if (tryPost(buffer, true, true) != 0)
    {
        return false;
    }
    bytes.release();
    return true;
}

int32_t LibuvTcp::tryPost(const char* buffer, bool retryContention, bool waitForSpace)
{
    return trySubmitBuffer(buffer, retryContention, waitForSpace);
}

bool LibuvTcp::post(uv_tcp_t* target, const char* data, uint32_t size)
{
    return tryPost(target, data, size, true, true) == 0;
}

int32_t LibuvTcp::tryPost(uv_tcp_t* target, const char* data, uint32_t size,
    bool retryContention, bool waitForSpace)
{
    return trySubmitData(target, data, size, retryContention, waitForSpace);
}

bool LibuvTcp::postBatch(uv_tcp_t* target, const LibuvSendBuffer* buffers, uint32_t count)
{
    return tryPostBatch(target, buffers, count, true, true) == 0;
}

int32_t LibuvTcp::tryPostBatch(uv_tcp_t* target, const LibuvSendBuffer* buffers, uint32_t count,
    bool retryContention, bool waitForSpace)
{
    return trySubmitBatch(target, buffers, count, retryContention, waitForSpace);
}

int32_t LibuvTcp::trySubmit(LibuvPendingSend& pending, bool retryContention, bool waitForSpace)
{
    LibuvCall call(*m_lifecycle, retryContention);
    if (!call.accepted())
    {
        return call.status();
    }
    LibuvBase* state = m_libuv;
    if (state->m_stopping.load())
    {
        return UV_ECANCELED;
    }
    if (!state->connectionGeneration(pending.m_target, pending.m_generation))
    {
        return UV_ENOTCONN;
    }
    uint32_t spaceAttempts = 0;
    uint32_t contentionAttempts = 0;
    for (;;)
    {
        if (state->m_stopping.load())
        {
            return UV_ECANCELED;
        }
        if (spaceAttempts != 0 || contentionAttempts != 0)
        {
            uint64_t generation = 0;
            if (!state->connectionGeneration(pending.m_target, generation) || generation != pending.m_generation)
            {
                return UV_ENOTCONN;
            }
        }
        int32_t result = state->tryReserveSend(pending.m_charge, retryContention);
        if (result == 0)
        {
            // 容量等待由本层执行，才能每轮检查停止和连接代次，不能被下层无限等待困住。
            // 发布屏障由队列接口完成；唤醒前不再叠加相同屏障，其他平台仍由该接口提供完整排序。
            const LockFreeQueueResult queued = state->tryQueueSend(pending, retryContention);
            if (queued == LockFreeQueueSuccess)
            {
                // 接纳后所有权已转移；即便唤醒系统异常，也不能返回“未接纳”让调用方重复释放。
                if (uv_async_send(state->m_asyncHandle) != 0)
                {
                    state->m_stopping.store(true);
                    LibuvBase::Wake(state->m_asyncCloseHandle);
                }
                return 0;
            }
            state->releaseSend(pending.m_charge);
            if (queued == LockFreeQueueBusy)
            {
                result = UV_EAGAIN;
            }
            else if (queued == LockFreeQueueCapacityLimit || queued == LockFreeQueueResourceUnavailable)
            {
                result = UV_ENOBUFS;
            }
            else
            {
                return queued == LockFreeQueueAllocationFailure ? UV_ENOMEM : UV_EIO;
            }
        }
        if (result == UV_EAGAIN && retryContention)
        {
            if ((++contentionAttempts & 63U) == 0)
            {
                uv_sleep(0);
            }
            continue;
        }
        if (result != UV_ENOBUFS || !waitForSpace)
        {
            return result;
        }
        if (!state->m_ownerReady.load(std::memory_order_acquire) || state->isLoopOwner())
        {
            // 等待尚未运行或当前栈上的循环会阻塞释放容量的唯一执行者，属于非法调用上下文。
            return UV_EPERM;
        }
        LibuvBase::WaitForSpace(spaceAttempts);
    }
}

uv_loop_t *LibuvTcp::loopPtr()
{
    LibuvCall call(*m_lifecycle);
    return !call.accepted() ? nullptr : m_libuv->m_loop;
}

bool LibuvTcp::isClient()
{
    LibuvCall call(*m_lifecycle);
    return call.accepted() && m_libuv->m_isClient;
}

void LibuvTcp::close(uv_tcp_t *tcp)
{
    LibuvCall call(*m_lifecycle);
    if (!call.accepted() || tcp == nullptr || m_libuv->m_stopping.load())
    {
        return;
    }
    LibuvPendingSend pending;
    if (!m_libuv->connectionGeneration(tcp, pending.m_generation))
    {
        return;
    }
    pending.m_target = tcp;
    pending.m_close = true;
    if (m_libuv->isLoopOwner())
    {
        m_libuv->closeTcp(tcp);
        return;
    }
    uint32_t spaceAttempts = 0;
    for (;;)
    {
        if (m_libuv->m_stopping.load())
        {
            return;
        }
        uint64_t generation = 0;
        if (!m_libuv->connectionGeneration(tcp, generation) || generation != pending.m_generation)
        {
            return;
        }
        const LockFreeQueueResult result = m_libuv->m_closeQueue->tryPush(pending, true, false);
        if (result == LockFreeQueueSuccess)
        {
            if (LibuvBase::Wake(m_libuv->m_asyncHandle) == 0)
            {
                return;
            }
            break;
        }
        if ((result == LockFreeQueueCapacityLimit || result == LockFreeQueueResourceUnavailable) &&
            m_libuv->m_ownerReady.load(std::memory_order_acquire))
        {
            LibuvBase::WaitForSpace(spaceAttempts);
            continue;
        }
        break;
    }
    // 不把运行中的瞬时容量不足变成全局停止；仅不可恢复资源/非法生命周期走独立停止兜底。
    m_libuv->m_stopping.store(true);
    LibuvBase::Wake(m_libuv->m_asyncCloseHandle);
}

void LibuvTcp::receive(uv_tcp_t* sender, const char* buffer, int32_t length)
{
	printf("sender = %p, buffer = %p, length = %d\n", sender, buffer, length);
}

void LibuvTcp::uvClientConnected(uv_tcp_t* client)
{
	printf("client = %p\n", client);
}

void LibuvTcp::uvClientDisconnected(uv_tcp_t* client)
{
	printf("client disconnect = %p\n", client);
}

void LibuvTcp::uvServerConnected(uv_tcp_t* server)
{
	printf("server = %p\n", server);
}

void LibuvTcp::uvServerNotFind()
{
	printf("server not find\n");
}

void LibuvTcp::uvServerNotFindClear()
{

}

void LibuvTcp::uvServerDisconnected(uv_tcp_t* server)
{
	printf("server disconnect = %p\n", server);
}

void LibuvTcp::uvDisconnectedClear(uv_tcp_t* tcp)
{

}

int32_t LibuvTcp::hostCreate(void* providerContext, void* observerContext, LibuvHostConnected connected,
    LibuvHostData data, LibuvHostWritable writable, LibuvHostClosed closed, void** instance)
{
    (void)providerContext;
    if (instance == nullptr)
    {
        return -1;
    }
    *instance = nullptr;
    if (connected == nullptr || data == nullptr || writable == nullptr || closed == nullptr)
    {
        return -1;
    }
    try
    {
        *instance = new LibuvHostState(observerContext, connected, data, writable, closed);
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}

int32_t LibuvTcp::hostConfigureWritePool(void* instance, const LibuvChannelWritePoolConfig& config)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr || state->m_stopping || state->m_depth != 0 || state->m_polling)
    {
        return UV_EINVAL;
    }
    return state->m_channel.configureWritePool(config);
}

int32_t LibuvTcp::hostConfigureReadBuffer(void* instance, uint32_t bufferBytes)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr || state->m_stopping || state->m_depth != 0 || state->m_polling)
    {
        return UV_EINVAL;
    }
    return state->m_channel.configureReadBuffer(bufferBytes);
}

int32_t LibuvTcp::hostListen(void* instance, const char* address, uint32_t port, uint32_t maxPeers)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr || state->m_stopping || state->m_depth != 0 || state->m_polling)
    {
        return -1;
    }
    try
    {
        return state->listen(address, port, maxPeers);
    }
    catch (...)
    {
        return -1;
    }
}

int32_t LibuvTcp::hostConnect(void* instance, const char* address, uint32_t port, uint64_t* peer)
{
    if (peer == nullptr)
    {
        return -1;
    }
    *peer = 0;
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr || state->m_stopping || state->m_depth != 0 || state->m_polling)
    {
        return -1;
    }
    try
    {
        return state->connect(address, port, *peer);
    }
    catch (...)
    {
        *peer = 0;
        return -1;
    }
}

int32_t LibuvTcp::hostPoll(void* instance)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr || state->m_depth != 0 || state->m_polling)
    {
        return -1;
    }
    state->m_polling = true;
    int32_t result = -1;
    try
    {
        const bool active = state->m_channel.poll();
        // uv_run在全部读取暂停时可返回0，但暂停的TCP资源仍由本实例拥有
        result = active || state->m_channel.peerCount() != 0 ? 1 : 0;
    }
    catch (...)
    {
        state->m_stopping = true;
        // 保留实例，由调用者在外层收敛，不在可能的回调栈上释放
    }
    state->m_polling = false;
    return result;
}

int32_t LibuvTcp::hostWrite(void* instance, uint64_t peer, const uint8_t* data, uint32_t size)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr || state->m_stopping)
    {
        return -1;
    }
    try
    {
        const int32_t result = state->m_channel.write(peer, data, size);
        return LibuvChannel::isBackpressure(result) ? 1 : result;
    }
    catch (...)
    {
        return -1;
    }
}

int32_t LibuvTcp::hostPauseRead(void* instance, uint64_t peer, int32_t paused)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr || state->m_stopping || (paused != 0 && paused != 1))
    {
        return -1;
    }
    try
    {
        return state->m_channel.pauseRead(peer, paused != 0);
    }
    catch (...)
    {
        return -1;
    }
}

int32_t LibuvTcp::hostClose(void* instance, uint64_t peer, int32_t reason)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr)
    {
        return -1;
    }
    try
    {
        state->m_channel.close(peer, reason);
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}

int32_t LibuvTcp::hostShutdown(void* instance)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr)
    {
        return -1;
    }
    state->m_stopping = true;
    try
    {
        state->m_channel.shutdown();
        return 0;
    }
    catch (...)
    {
        return -1;
    }
}

int32_t LibuvTcp::hostDestroy(void* instance)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    if (state == nullptr)
    {
        return 0;
    }
    if (state->m_depth != 0 || state->m_polling)
    {
        return -1;
    }
    // shutdown期间的终态也不得重入销毁；只有完全收敛后才释放监听者
    state->m_polling = true;
    const int32_t result = LibuvTcp::hostShutdown(instance);
    state->m_polling = false;
    if (result != 0)
    {
        return result;
    }
    delete state;
    return 0;
}

uint32_t LibuvTcp::hostLocalPort(void* instance)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    return state == nullptr ? 0 : state->m_channel.localPort();
}

uint32_t LibuvTcp::hostPeerCount(void* instance)
{
    LibuvHostState* state = static_cast<LibuvHostState*>(instance);
    return state == nullptr ? 0 : state->m_channel.peerCount();
}

// LIBUV_TCP_USAGE_DEMO
// // 使用示例：取消本段逐行注释后编译此源文件为演示进程；正式库保持注释
// // 外部应用只包含 LibuvTcp/LibuvTcpAPI.h，本文件已有内部主头文件声明
// // 默认执行原始字节自连接；另开两个进程运行：--server 28745、--client 28745
// // 本示例局部类型只为把完整用法保留在一个main中，实际项目按.h/.cpp拆分类
// #include <string>
// #include <cstdlib>
// #include <cerrno>
//
// int main(int argc, char** argv)
// {
//     try
//     {
//         if (argc == 1)
//         {
//             // 两个宿主完全由当前执行者驱动，回调上下文存活到destroy成功。
//             struct Host
//             {
//                 // 原始运输句柄，销毁成功后失效
//                 void* instance;
//                 // 当前有效的本实例连接号
//                 uint64_t peer;
//                 // 已收到的正文字节数
//                 uint32_t received;
//                 // 已确认连接建立
//                 bool connected;
//                 // 已收到终态通知
//                 bool closed;
//                 // 首个应用或运输错误
//                 int32_t error;
//             };
//             Host server = {}, client = {};
//             const LibuvHostConnected connected = [](void* context, uint64_t peer)
//             {
//                 Host* host = static_cast<Host*>(context);
//                 host->peer = peer;
//                 host->connected = true;
//             };
//             const LibuvHostData data = [](void* context, uint64_t peer, const uint8_t* bytes, uint32_t size)
//             {
//                 (void)peer;
//                 Host* host = static_cast<Host*>(context);
//                 // 此协议是一条6字节正文；真实协议必须自行重组完整帧。
//                 const char* text = "123456";
//                 for (uint32_t index = 0; index < size; ++index)
//                 {
//                     if (host->received >= 6 || bytes[index] != static_cast<uint8_t>(text[host->received]))
//                     {
//                         host->error = UV_EPROTO;
//                         return;
//                     }
//                     ++host->received;
//                 }
//             };
//             const LibuvHostWritable writable = [](void* context, uint64_t peer)
//             {
//                 (void)context;
//                 (void)peer;
//                 // 可在所属执行者重新尝试被背压拒绝的请求；这不是对端ACK。
//             };
//             const LibuvHostClosed closed = [](void* context, uint64_t peer, int32_t reason)
//             {
//                 (void)peer;
//                 Host* host = static_cast<Host*>(context);
//                 host->connected = false;
//                 host->closed = true;
//                 if (reason != 0)
//                 {
//                     host->error = reason;
//                 }
//             };
//             int32_t result = LibuvTcp::hostCreate(nullptr, &server, connected, data, writable, closed, &server.instance);
//             if (result == 0)
//             {
//                 result = LibuvTcp::hostCreate(nullptr, &client, connected, data, writable, closed, &client.instance);
//             }
//             LibuvChannelWritePoolConfig writes;
//             writes.m_maximumWriteBytes = 4096;
//             writes.m_maximumPendingBytesPerPeer = 16384;
//             for (uint32_t index = 0; index < 2 && result == 0; ++index)
//             {
//                 Host& host = index == 0 ? server : client;
//                 result = LibuvTcp::hostConfigureWritePool(host.instance, writes);
//                 if (result == 0)
//                 {
//                     result = LibuvTcp::hostConfigureReadBuffer(host.instance, 4096);
//                 }
//             }
//             if (result == 0)
//             {
//                 result = LibuvTcp::hostListen(server.instance, "127.0.0.1", 0, 1);
//             }
//             if (result == 0)
//             {
//                 result = LibuvTcp::hostConnect(client.instance, "127.0.0.1", LibuvTcp::hostLocalPort(server.instance), &client.peer);
//             }
//             bool sent = false;
//             const uint64_t deadline = uv_hrtime() + 5000000000ULL;
//             while (result == 0 && server.received != 6 && uv_hrtime() < deadline)
//             {
//                 const int32_t left = LibuvTcp::hostPoll(server.instance);
//                 const int32_t right = LibuvTcp::hostPoll(client.instance);
//                 if (left < 0 || right < 0 || server.error != 0 || client.error != 0 || server.closed || client.closed)
//                 {
//                     result = UV_ECONNRESET;
//                     break;
//                 }
//                 if (server.connected && client.connected && !sent)
//                 {
//                     result = LibuvTcp::hostPauseRead(server.instance, server.peer, 1);
//                     if (result == 0)
//                     {
//                         result = LibuvTcp::hostPauseRead(server.instance, server.peer, 0);
//                     }
//                     if (result == 0)
//                     {
//                         const int32_t accepted = LibuvTcp::hostWrite(client.instance, client.peer,
//                             reinterpret_cast<const uint8_t*>("123456"), 6);
//                         sent = accepted == 0;
//                         result = accepted == 1 ? 0 : accepted;
//                     }
//                 }
//                 uv_sleep(1);
//             }
//             if (result == 0 && server.received != 6)
//             {
//                 result = UV_ETIMEDOUT;
//             }
//             std::printf("host received=%u peers=%u result=%d\n", server.received,
//                 LibuvTcp::hostPeerCount(server.instance), result);
//             if (client.connected)
//             {
//                 LibuvTcp::hostClose(client.instance, client.peer, 0);
//             }
//             for (uint32_t index = 0; index < 2; ++index)
//             {
//                 Host& host = index == 0 ? client : server;
//                 if (host.instance != nullptr)
//                 {
//                     LibuvTcp::hostShutdown(host.instance);
//                     // destroy在回调外同步收敛自有句柄，失败时不能释放回调上下文。
//                     const int32_t released = LibuvTcp::hostDestroy(host.instance);
//                     if (released != 0)
//                     {
//                         return 1;
//                     }
//                     host.instance = nullptr;
//                 }
//             }
//             return result == 0 ? 0 : 1;
//         }
//         if (argc != 3 || (std::strcmp(argv[1], "--server") != 0 && std::strcmp(argv[1], "--client") != 0))
//         {
//             std::fputs("usage: demo | demo --server PORT | demo --client PORT\n", stderr);
//             return 2;
//         }
//         char* end = nullptr;
//         errno = 0;
//         const long port = std::strtol(argv[2], &end, 10);
//         if (errno != 0 || end == argv[2] || *end != 0 || port < 1 || port > 65535)
//         {
//             return 2;
//         }
//         // 单连接回显应用；回调在loop执行者运行。跨线程业务应由CTaskThreadManager托管loop。
//         class Demo : public LibuvTcp
//         {
//         public:
//             // 应用自建截止定时器，stop前自行关闭
//             uv_timer_t timer;
//             // 当前连接未组装完的TCP字节
//             std::string input;
//             // 完整回显帧计数
//             uint32_t received;
//             // 本次演示退出状态
//             int32_t result;
//             // 只有uv_timer_init成功后才允许关闭句柄
//             bool timerReady;
//
//             // 构造空的业务状态
//             Demo() : timer(), received(0), result(1), timerReady(false) {}
//
//             // 先关闭业务句柄，再请求库停止
//             void finish(int32_t status)
//             {
//                 result = status;
//                 if (timerReady && !uv_is_closing(reinterpret_cast<uv_handle_t*>(&timer)))
//                 {
//                     uv_timer_stop(&timer);
//                     uv_close(reinterpret_cast<uv_handle_t*>(&timer), nullptr);
//                 }
//                 stop();
//             }
//
//             // 分配并填好一条数组形式的长度帧，交由同一库释放
//             char* packet(uv_tcp_t* peer)
//             {
//                 const int32_t length = 6;
//                 char* bytes = memoryForSend(static_cast<int32_t>(sizeof(peer)) + 4 + length);
//                 std::memcpy(bytes, &peer, sizeof(peer));
//                 std::memcpy(bytes + sizeof(peer), &length, 4);
//                 std::memcpy(bytes + sizeof(peer) + 4, "123456", 6);
//                 return bytes;
//             }
//
//             // 客户端已连通，演示全部十二种提交入口
//             void uvServerConnected(uv_tcp_t* peer) override
//             {
//                 try
//                 {
//                     // 本例只提交16个小帧且已配置足够容量；一般回调必须用不等空间的try接口。
//                     bool ok = send(peer, "123456", 6) && post(peer, "123456", 6) &&
//                         trySend(peer, "123456", 6, true, false) == 0 && tryPost(peer, "123456", 6, true, false) == 0;
//                     const LibuvSendBuffer messages[] = { { "123456", 6 }, { "123456", 6 } };
//                     ok = sendBatch(peer, messages, 2) && ok;
//                     ok = postBatch(peer, messages, 2) && ok;
//                     ok = trySendBatch(peer, messages, 2, true, false) == 0 && ok;
//                     ok = tryPostBatch(peer, messages, 2, true, false) == 0 && ok;
//                     ok = send(packet(peer)) && ok;
//                     ok = post(packet(peer)) && ok;
//                     char* bytes = packet(peer);
//                     if (trySend(bytes, true, false) != 0)
//                     {
//                         releaseSendMemory(bytes);
//                         ok = false;
//                     }
//                     bytes = packet(peer);
//                     if (tryPost(bytes, true, false) != 0)
//                     {
//                         releaseSendMemory(bytes);
//                         ok = false;
//                     }
//                     // 数组普通send/post即使失败也接管；try只有成功接管，不能重复释放。
//                     if (!ok)
//                     {
//                         finish(1);
//                     }
//                 }
//                 catch (...)
//                 {
//                     finish(1);
//                 }
//             }
//
//             // 将TCP片段重组为受限长度帧，再处理业务
//             void receive(uv_tcp_t* peer, const char* bytes, int32_t size) override
//             {
//                 try
//                 {
//                     input.append(bytes, static_cast<size_t>(size));
//                     while (input.size() >= 4)
//                     {
//                         int32_t length = 0;
//                         std::memcpy(&length, input.data(), 4);
//                         if (length < 0 || length > 4096)
//                         {
//                             finish(1);
//                             return;
//                         }
//                         if (input.size() < static_cast<size_t>(length) + 4)
//                         {
//                             break;
//                         }
//                         ++received;
//                         if (!isClient() && trySend(peer, input.data() + 4, static_cast<uint32_t>(length), true, false) != 0)
//                         {
//                             finish(1);
//                             return;
//                         }
//                         input.erase(0, static_cast<size_t>(length) + 4);
//                         if (isClient() && received == 16)
//                         {
//                             // 已收到应用回显后才取消连接；close本身不是flush或送达确认。
//                             close(peer);
//                             finish(0);
//                             return;
//                         }
//                     }
//                 }
//                 catch (...)
//                 {
//                     finish(1);
//                 }
//             }
//
//             // 服务端连接建立，本例只等待客户端消息
//             void uvClientConnected(uv_tcp_t* peer) override { (void)peer; }
//
//             // 服务端观察本次客户端结束
//             void uvClientDisconnected(uv_tcp_t* peer) override { (void)peer; finish(received == 16 ? 0 : 1); }
//
//             // 客户端观察服务端结束
//             void uvServerDisconnected(uv_tcp_t* peer) override { (void)peer; finish(received == 16 ? 0 : 1); }
//
//             // 连接失败时收敛演示，避免永久等候
//             void uvServerNotFind() override { finish(1); }
//
//             // 连接失败后无其他业务资源需要清理
//             void uvServerNotFindClear() override {}
//
//             // 本例没有每连接的额外清理对象
//             void uvDisconnectedClear(uv_tcp_t* peer) override { (void)peer; }
//         } tcp;
//         LibuvQueuePoolConfig sendQueue;
//         LibuvQueuePoolConfig closeQueue;
//         closeQueue.m_initialCapacity = 32;
//         closeQueue.m_maximumCapacity = 256;
//         LibuvConnectionPoolConfig connections;
//         LibuvSendPoolConfig sendPool;
//         LibuvReadPoolConfig reads;
//         if (tcp.configureSendQueue(sendQueue) != 0 || tcp.configureCloseQueue(closeQueue) != 0 ||
//             tcp.configureConnectionPool(connections) != 0 || tcp.configureSendPool(sendPool) != 0 ||
//             tcp.configureReadPool(reads) != 0)
//         {
//             return 1;
//         }
//         const bool server = std::strcmp(argv[1], "--server") == 0;
//         const bool initialized = server ? tcp.initServer("127.0.0.1", static_cast<int32_t>(port), 16) :
//             tcp.initClient("127.0.0.1", static_cast<int32_t>(port));
//         // initServer(port)也可使用，但默认监听所有网卡；对外监听需由业务明确选择。
//         if (!initialized)
//         {
//             return 1;
//         }
//         if (uv_timer_init(tcp.loopPtr(), &tcp.timer) != 0)
//         {
//             tcp.stop();
//             tcp.loop();
//             return 1;
//         }
//         tcp.timerReady = true;
//         tcp.timer.data = &tcp;
//         if (uv_timer_start(&tcp.timer, [](uv_timer_t* timer)
//             { static_cast<Demo*>(timer->data)->finish(1); }, 15000, 0) != 0)
//         {
//             tcp.finish(1);
//         }
//         std::printf("%s ready\n", server ? "server" : "client");
//         std::fflush(stdout);
//         tcp.loop();
//         // loop返回后已无本实例回调，随后才销毁派生状态和对象。
//         std::printf("frames=%u result=%d\n", tcp.received, tcp.result);
//         return tcp.result;
//     }
//     catch (...)
//     {
//         std::fputs("demo resource failure\n", stderr);
//         return 1;
//     }
// }