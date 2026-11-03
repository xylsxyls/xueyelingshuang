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

LibuvLifecycleGate::LibuvLifecycleGate() :
m_mutexReady(false),
m_conditionReady(false)
{
#ifdef _WIN32
    InitializeSRWLock(&m_mutex);
    InitializeConditionVariable(&m_changed);
    m_mutexReady = true;
    m_conditionReady = true;
#else
    m_mutexReady = pthread_mutex_init(&m_mutex, nullptr) == 0;
    if (m_mutexReady)
    {
        m_conditionReady = pthread_cond_init(&m_changed, nullptr) == 0;
    }
#endif
}

LibuvLifecycleGate::~LibuvLifecycleGate()
{
#ifndef _WIN32
    if (m_conditionReady)
    {
        pthread_cond_destroy(&m_changed);
    }
    if (m_mutexReady)
    {
        pthread_mutex_destroy(&m_mutex);
    }
#endif
}

bool LibuvLifecycleGate::valid() const
{
    return m_mutexReady && m_conditionReady;
}

void LibuvLifecycleGate::lock()
{
#ifdef _WIN32
    AcquireSRWLockExclusive(&m_mutex);
#else
    pthread_mutex_lock(&m_mutex);
#endif
}

void LibuvLifecycleGate::unlock()
{
#ifdef _WIN32
    ReleaseSRWLockExclusive(&m_mutex);
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}

bool LibuvLifecycleGate::wait()
{
#ifdef _WIN32
    return SleepConditionVariableSRW(&m_changed, &m_mutex, INFINITE, 0) != FALSE;
#else
    return pthread_cond_wait(&m_changed, &m_mutex) == 0;
#endif
}

void LibuvLifecycleGate::notifyAll()
{
#ifdef _WIN32
    WakeAllConditionVariable(&m_changed);
#else
    pthread_cond_broadcast(&m_changed);
#endif
}

LibuvPendingSend::LibuvPendingSend() :
m_buffer(nullptr),
m_target(nullptr),
m_generation(0),
m_close(false)
{

}

LibuvBase::LibuvBase() :
m_libuvTcp(nullptr),
m_loop(nullptr),
m_asyncHandle(nullptr),
m_queue(nullptr),
m_asyncCloseHandle(nullptr),
m_isClient(false),
m_stopping(false),
m_nextConnection(1),
m_loopInitialized(false),
m_closing(false)
{
    std::unique_ptr<uv_loop_t> loop(new uv_loop_t);
    std::unique_ptr<uv_async_t> send(new uv_async_t);
    std::unique_ptr<uv_async_t> stop(new uv_async_t);
    std::unique_ptr<LockFreeQueue<LibuvPendingSend>> queue(new LockFreeQueue<LibuvPendingSend>);
    m_loop = loop.release();
    m_asyncHandle = send.release();
    m_asyncCloseHandle = stop.release();
    m_queue = queue.release();
}

LibuvBase::~LibuvBase()
{
    LibuvPendingSend pending;
    while (m_queue->pop(&pending))
    {
        delete[] pending.m_buffer;
    }
    delete m_queue;
    delete m_asyncCloseHandle;
    delete m_asyncHandle;
    delete m_loop;
}

uv_tcp_t *LibuvBase::mainTcp()
{
    return m_isClient ? static_cast<LibuvClient *>(this)->m_dest : static_cast<LibuvServer *>(this)->m_server;
}

int32_t LibuvBase::initialize(LibuvTcp *owner)
{
    if (!m_mutex.valid())
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
    std::lock_guard<LibuvLifecycleGate> lock(m_mutex);
    if (m_stopping.load() || m_nextConnection == 0)
    {
        return false;
    }
    try
    {
        m_connections.insert(std::make_pair(tcp, m_nextConnection++));
        return true;
    }
    catch (...)
    {
        m_stopping.store(true);
        uv_async_send(m_asyncCloseHandle);
        return false;
    }
}

void LibuvBase::closeTcp(uv_tcp_t *tcp)
{
    {
        std::lock_guard<LibuvLifecycleGate> lock(m_mutex);
        m_connections.erase(tcp);
    }
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
    // 与提交方使用同一短锁，保证最后一次uv_async_send结束后才能关闭唤醒句柄。
    {
        std::lock_guard<LibuvLifecycleGate> lock(m_mutex);
        m_stopping.store(true);
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
    (void)handle;
    const size_t size = (std::min)(suggestedSize, static_cast<size_t>(65536));
    char *memory = size == 0 ? nullptr : new (std::nothrow) char[size];
    *buffer = uv_buf_init(memory, memory == nullptr ? 0 : static_cast<unsigned int>(size));
}

int32_t LibuvBase::startRead(uv_tcp_t *tcp)
{
    return uv_read_start(reinterpret_cast<uv_stream_t *>(tcp), &AllocateRead, &Read);
}

void LibuvBase::Read(uv_stream_t *sender, ssize_t size, const uv_buf_t *buffer)
{
    std::unique_ptr<char[]> bytes(buffer->base);
    LibuvBase *state = static_cast<LibuvBase *>(sender->data);
    LibuvTcp *owner = state->m_libuvTcp;
    if (owner == nullptr || state->m_stopping.load() || size == 0)
    {
        return;
    }
    uv_tcp_t *tcp = reinterpret_cast<uv_tcp_t *>(sender);
    try
    {
        if (size > 0)
        {
            owner->receive(tcp, buffer->base, static_cast<int32_t>(size));
            return;
        }
        state->closeTcp(tcp);
        owner->uvDisconnectedClear(tcp);
        if (state->m_isClient)
        {
            owner->stop();
            owner->uvServerDisconnected(tcp);
        }
        else
        {
            owner->uvClientDisconnected(tcp);
        }
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
    LibuvBase *state = static_cast<LibuvBase *>(handle->data);
    LibuvPendingSend pending;
    uint32_t count = 0;
    while (count < 128 && state->m_queue->pop(&pending))
    {
        ++count;
        std::unique_ptr<char[]> bytes(pending.m_buffer);
        bool valid = false;
        {
            std::lock_guard<LibuvLifecycleGate> lock(state->m_mutex);
            const auto found = state->m_connections.find(pending.m_target);
            valid = !state->m_stopping.load() && found != state->m_connections.end() &&
                    found->second == pending.m_generation;
        }
        if (!valid)
        {
            continue;
        }
        if (pending.m_close)
        {
            state->closeTcp(pending.m_target);
            continue;
        }
        int32_t length = 0;
        std::memcpy(&length, bytes.get() + sizeof(uv_tcp_t *), sizeof(length));
        if (length < 0 || length > INT_MAX - 4)
        {
            continue;
        }
        uv_write_t *request = new (std::nothrow) uv_write_t;
        if (request == nullptr)
        {
            state->m_libuvTcp->stop();
            continue;
        }
        request->data = bytes.get();
        uv_buf_t output = uv_buf_init(bytes.get() + sizeof(uv_tcp_t *), static_cast<unsigned int>(length + 4));
        const int32_t result =
            uv_write(request, reinterpret_cast<uv_stream_t *>(pending.m_target), &output, 1, &Written);
        if (result == 0)
        {
            bytes.release();
        }
        else
        {
            delete request;
        }
    }
    if (count == 128 && !state->m_stopping.load())
    {
        uv_async_send(handle);
    }
}

void LibuvBase::StopPending(uv_async_t *handle)
{
    static_cast<LibuvBase *>(handle->data)->beginClose();
}

void LibuvBase::Written(uv_write_t *request, int status)
{
    (void)status;
    delete[] static_cast<char *>(request->data);
    delete request;
}