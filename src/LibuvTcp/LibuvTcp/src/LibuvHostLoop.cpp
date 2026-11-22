#include "LibuvHostLoop.h"

// 当前执行线程选定的共同循环；绑定通道借用它，离开作用域前由调用者恢复。
#ifdef _MSC_VER
static __declspec(thread) LibuvHostLoop* s_hostLoop = nullptr;
#else
static thread_local LibuvHostLoop* s_hostLoop = nullptr;
#endif

LibuvHostLoop::LibuvHostLoop() :
m_generation(0),
m_channels(0),
m_running(false),
m_initialized(false),
m_asyncInitialized(false),
m_timerInitialized(false)
{

}

LibuvHostLoop::~LibuvHostLoop()
{
    if (!m_initialized)
    {
        return;
    }
    if (m_timerInitialized)
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_timer), nullptr);
    }
    if (m_asyncInitialized)
    {
        uv_close(reinterpret_cast<uv_handle_t*>(&m_async), nullptr);
    }
    uv_run(&m_loop, UV_RUN_DEFAULT);
    uv_loop_close(&m_loop);
}

int32_t LibuvHostLoop::init()
{
    int result = uv_loop_init(&m_loop);
    if (result != 0)
    {
        return result;
    }
    m_initialized = true;
    result = uv_async_init(&m_loop, &m_async, notified);
    if (result != 0)
    {
        return result;
    }
    m_asyncInitialized = true;
    result = uv_timer_init(&m_loop, &m_timer);
    m_timerInitialized = result == 0;
    return result;
}

int32_t LibuvHostLoop::run(uv_run_mode mode)
{
    if (m_running)
    {
        return UV_EBUSY;
    }
    m_running = true;
    uv_run(&m_loop, mode);
    m_running = false;
    return 0;
}

void LibuvHostLoop::wake()
{
    m_generation.fetch_add(1, std::memory_order_release);
    if (current() != this)
    {
        uv_async_send(&m_async);
    }
}

int32_t LibuvHostLoop::wait(uint64_t observed, uint32_t timeoutMs)
{
    if (m_running)
    {
        return UV_EBUSY;
    }
    if (timeoutMs == 0 || m_generation.load(std::memory_order_acquire) != observed)
    {
        // 业务已有通知时无需睡眠，仍须处理本轮就绪的网络事件。
        return run(UV_RUN_NOWAIT);
    }
    // 更新libuv缓存时间，避免上层处理较久后把维护期限误算成已过期。
    uv_update_time(&m_loop);
    const int result = uv_timer_start(&m_timer, elapsed, timeoutMs, 0);
    if (result != 0)
    {
        return result;
    }
    const int32_t waited = run(UV_RUN_ONCE);
    uv_timer_stop(&m_timer);
    return waited;
}

LibuvHostLoop* LibuvHostLoop::exchange(LibuvHostLoop* loop)
{
    LibuvHostLoop* previous = s_hostLoop;
    s_hostLoop = loop;
    return previous;
}

LibuvHostLoop* LibuvHostLoop::current()
{
    return s_hostLoop;
}

void LibuvHostLoop::notified(uv_async_t* handle)
{
    (void)handle;
}

void LibuvHostLoop::elapsed(uv_timer_t* handle)
{
    (void)handle;
}