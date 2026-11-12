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