#include "NetExecution.h"
#include "NetInterface.h"
#include <chrono>
#include <algorithm>

// 仅启动期configure写入，创建执行者后不再修改。
static NetHostExecutionFunctions s_functions = {};
#ifdef _MSC_VER
static __declspec(thread) NetExecution* s_execution = nullptr;
#else
static thread_local NetExecution* s_execution = nullptr;
#endif

/** 事件等待使用本机单调时钟，业务可传入其他起点的单调时间
@return 本机单调毫秒
*/
static uint64_t ExecutionNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetExecution::NetExecution() :
m_functions(s_functions), m_instance(nullptr), m_observed(0), m_waitUntil(UINT64_MAX), m_status(NI_OK), m_progressed(false)
{
    try
    {
        if (m_functions.create && (m_functions.create(&m_instance) != 0 || !m_instance)) { m_status = NI_INVALID_STATE; }
    }
    catch (...) { m_status = NI_INVALID_STATE; }
}

NetExecution::~NetExecution()
{
    if (m_instance != nullptr)
    {
        // 宿主拒绝释放时宁可保留循环；不能释放仍被连接或通知者借用的状态。
        try { m_functions.destroy(m_instance); }
        catch (...) { /* 宿主继续管理未确认释放的资源，析构不能重复释放。 */ }
    }
}

int32_t NetExecution::configure(const NetHostExecutionFunctions& functions)
{
    const bool any = functions.create || functions.exchange || functions.begin || functions.wait || functions.wake || functions.destroy;
    const bool all = functions.create && functions.exchange && functions.begin && functions.wait && functions.wake && functions.destroy;
    if (any && !all)
    {
        return NI_INVALID_ARGUMENT;
    }
    s_functions = functions;
    return NI_OK;
}

int32_t NetExecution::status() const
{
    return m_status.load(std::memory_order_relaxed);
}

void NetExecution::begin()
{
    m_waitUntil = UINT64_MAX;
    m_progressed = false;
    if (m_instance && status() == NI_OK)
    {
        try { m_observed = m_functions.begin(m_instance); }
        catch (...) { m_status = NI_INVALID_STATE; }
    }
}

int32_t NetExecution::wait(uint32_t timeoutMs)
{
    if (status() != NI_OK) { return NI_INVALID_STATE; }
    if (m_instance == nullptr) { return NI_NEED_MORE; }
    const uint64_t now = ExecutionNow();
    const uint64_t remaining = m_waitUntil > now ? m_waitUntil - now : 0;
    timeoutMs = static_cast<uint32_t>((std::min)(remaining, static_cast<uint64_t>(timeoutMs)));
    try { return m_functions.wait(m_instance, m_observed, m_progressed ? 0 : timeoutMs) == 0 ? NI_OK : NI_INVALID_STATE; }
    catch (...) { m_status = NI_INVALID_STATE; return NI_INVALID_STATE; }
}

void NetExecution::wake()
{
    if (m_instance && status() == NI_OK)
    {
        try { m_functions.wake(m_instance); }
        catch (...) { m_status = NI_INVALID_STATE; }
    }
}

NetExecution* NetExecution::current()
{
    return s_execution;
}

void NetExecution::progress()
{
    if (s_execution != nullptr) { s_execution->m_progressed = true; }
}

void NetExecution::deadline(uint64_t expiresMs, uint64_t nowMs)
{
    if (s_execution == nullptr)
    {
        return;
    }
    const uint64_t remaining = expiresMs > nowMs ? expiresMs - nowMs : 0;
    const uint64_t now = ExecutionNow();
    const uint64_t until = remaining > UINT64_MAX - now ? UINT64_MAX : now + remaining;
    s_execution->m_waitUntil = (std::min)(s_execution->m_waitUntil, until);
}

bool NetExecution::bind(NetExecution* execution, void* host, void*& previousHost)
{
    previousHost = nullptr;
    try
    {
        if (s_functions.exchange) { previousHost = s_functions.exchange(host); }
    }
    catch (...)
    {
        if (execution) { execution->m_status = NI_INVALID_STATE; }
        if (s_execution) { s_execution->m_status = NI_INVALID_STATE; }
        return false;
    }
    s_execution = execution;
    return true;
}

NetExecution* NetExecution::exchange(NetExecution* execution)
{
    void* previousHost = nullptr;
    return exchange(execution, previousHost);
}

NetExecution* NetExecution::exchange(NetExecution* execution, void*& previousHost)
{
    NetExecution* previous = s_execution;
    previousHost = nullptr;
    if (!execution || execution->status() == NI_OK)
    {
        bind(execution, execution ? execution->m_instance : nullptr, previousHost);
    }
    return previous;
}

NetExecutionScope::NetExecutionScope(NetExecution& execution) :
m_previousHost(nullptr), m_previous(NetExecution::current()),
m_bound(execution.status() == NI_OK && NetExecution::bind(&execution, execution.m_instance, m_previousHost))
{

}

NetExecutionScope::~NetExecutionScope()
{
    if (m_bound)
    {
        void* discarded = nullptr;
        if (!NetExecution::bind(m_previous, m_previousHost, discarded))
        {
            // 宿主切换失败已记录。当前作用域即将退出，不再暴露即将失效的协议执行者。
            s_execution = m_previous;
        }
    }
}