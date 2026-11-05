#include "SemaphoreWake.h"
#include <system_error>
#include <climits>
#ifdef _WIN32
#include <Windows.h>
#elif defined(__EMSCRIPTEN__)
#include <emscripten/threading.h>
#include <cerrno>
#elif defined(__linux__) || defined(__ANDROID__)
#include <linux/futex.h>
#include <linux/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <time.h>
#include <cerrno>
#elif defined(__APPLE__)
#include <dispatch/dispatch.h>
#else
#error Semaphore needs a supported native waiting backend
#endif

std::atomic<uint32_t> SemaphoreWake::s_cleanupFailures(0);
std::atomic<void*> SemaphoreWake::s_cachedWaitAddress(nullptr);
std::atomic<void*> SemaphoreWake::s_cachedWakeAddress(nullptr);

SemaphoreWake::SemaphoreWake() :
m_word(0),
m_handle(nullptr),
m_waitAddress(nullptr),
m_wakeAddress(nullptr),
m_hint(0),
m_sleepers(0)
{
    static_assert(sizeof(std::atomic<uint32_t>) == sizeof(uint32_t), "Native wait word must be 32 bits");
    if (!m_word.is_lock_free() || !m_hint.is_lock_free() || !s_cleanupFailures.is_lock_free() ||
        !s_cachedWaitAddress.is_lock_free())
    {
        throw std::system_error(std::make_error_code(std::errc::not_supported));
    }
#ifdef _WIN32
    // Win8+公开地址等待API由KernelBase导出；Kernel32并不保证转发这些名称。
    // 仅借用进程已加载的系统模块，旧系统/缺失导出仍走事件后备。
    m_waitAddress = s_cachedWaitAddress.load(std::memory_order_acquire);
    if (m_waitAddress != nullptr)
    {
        m_wakeAddress = s_cachedWakeAddress.load(std::memory_order_relaxed);
    }
    else
    {
        HMODULE kernel = ::GetModuleHandleW(L"kernelbase.dll");
        if (kernel == nullptr)
        {
            kernel = ::GetModuleHandleW(L"kernel32.dll");
        }
        if (kernel != nullptr)
        {
            m_waitAddress = reinterpret_cast<void*>(::GetProcAddress(kernel, "WaitOnAddress"));
            m_wakeAddress = reinterpret_cast<void*>(::GetProcAddress(kernel, "WakeByAddressAll"));
        }
        if (m_waitAddress != nullptr && m_wakeAddress != nullptr)
        {
            s_cachedWakeAddress.store(m_wakeAddress, std::memory_order_relaxed);
            s_cachedWaitAddress.store(m_waitAddress, std::memory_order_release);
        }
    }
    if (m_waitAddress == nullptr || m_wakeAddress == nullptr)
    {
        m_waitAddress = nullptr;
        m_wakeAddress = nullptr;
        m_handle = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (m_handle == nullptr)
        {
            throw std::system_error(static_cast<int32_t>(::GetLastError()), std::system_category());
        }
    }
#elif defined(__APPLE__)
    m_handle = dispatch_semaphore_create(0);
    if (m_handle == nullptr)
    {
        throw std::system_error(std::make_error_code(std::errc::not_enough_memory));
    }
#endif
}

SemaphoreWake::~SemaphoreWake()
{
#ifdef _WIN32
    if (m_handle != nullptr && !::CloseHandle(m_handle))
    {
        recordCleanupFailure();
    }
#elif defined(__APPLE__)
    if (m_handle != nullptr)
    {
        dispatch_release(static_cast<dispatch_semaphore_t>(m_handle));
    }
#endif
}

uint32_t SemaphoreWake::snapshot() const
{
    return m_word.load();
}

SemaphoreResult SemaphoreWake::notify(std::atomic<uint32_t>* address)
{
    if (address != nullptr)
    {
        return notifyAddress(*address);
    }
    // 业务发布在前，登记读在后；等待方登记在前、谓词重查在后，全部seq_cst。
    // 因而两者不可能同时漏看对方：无人登记时无需更新提示字。
    if (m_sleepers.load() == 0)
    {
        return SemaphoreResult();
    }
    uint32_t current = m_word.load();
    while (current != UINT32_MAX && !m_word.compare_exchange_weak(current, current + 1))
    {

    }
    if (canWaitAddress())
    {
        return notifyAddress(m_word);
    }
#ifdef _WIN32
    if (!::SetEvent(m_handle))
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, static_cast<int32_t>(::GetLastError()));
    }
#elif defined(__APPLE__)
    if (m_hint.exchange(1) == 0)
    {
        dispatch_semaphore_signal(static_cast<dispatch_semaphore_t>(m_handle));
    }
#endif
    return SemaphoreResult();
}

bool SemaphoreWake::canWaitAddress() const
{
#ifdef _WIN32
    return m_waitAddress != nullptr && m_wakeAddress != nullptr;
#elif defined(__EMSCRIPTEN__)
#ifdef __EMSCRIPTEN_PTHREADS__
    return true;
#else
    return false;
#endif
#elif defined(__linux__) || defined(__ANDROID__)
    return true;
#else
    return false;
#endif
}

SemaphoreResult SemaphoreWake::prepareSignalAndWait()
{
    if (!canBlock())
    {
        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
    }
#ifdef _WIN32
    if (m_handle == nullptr)
    {
        HANDLE prepared = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (prepared == nullptr)
        {
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, static_cast<int32_t>(::GetLastError()));
        }
        m_handle = prepared;
        m_waitAddress = nullptr;
        m_wakeAddress = nullptr;
    }
#endif
    return SemaphoreResult();
}

bool SemaphoreWake::canSignalAndWait(const SemaphoreWake& waitOn) const
{
#ifdef _WIN32
    return m_handle != nullptr && waitOn.m_handle != nullptr && m_waitAddress == nullptr &&
        waitOn.m_waitAddress == nullptr;
#else
    return false;
#endif
}

SemaphoreResult SemaphoreWake::signalAndSleep(SemaphoreWake& waitOn, uint32_t expected, uint32_t milliseconds)
{
#ifdef _WIN32
    // 与兼容sleepNative相同：先复位后检查提示字，期间发布不能落入睡眠缝隙。
    if (!::ResetEvent(waitOn.m_handle))
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, static_cast<int32_t>(::GetLastError()));
    }
    if (waitOn.m_word.load() != expected)
    {
        return notify();
    }
    if (m_sleepers.load() != 0)
    {
        uint32_t current = m_word.load();
        while (current != UINT32_MAX && !m_word.compare_exchange_weak(current, current + 1))
        {

        }
    }
    const DWORD waited = ::SignalObjectAndWait(m_handle, waitOn.m_handle, milliseconds, FALSE);
    if (waited == WAIT_OBJECT_0)
    {
        return SemaphoreResult();
    }
    return SemaphoreResult(waited == WAIT_TIMEOUT ? SEMAPHORE_TIMEOUT : SEMAPHORE_SYSTEM_ERROR, false,
        waited == WAIT_TIMEOUT ? 0
                               : static_cast<int32_t>(waited == WAIT_FAILED ? ::GetLastError() : ERROR_GEN_FAILURE));
#else
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#endif
}

SemaphoreResult SemaphoreWake::enterWait()
{
    if (!canBlock())
    {
        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
    }
    uint32_t sleepers = m_sleepers.load();
    do
    {
        if (sleepers == UINT32_MAX)
        {
            return SemaphoreResult(SEMAPHORE_CAPACITY);
        }
    } while (!m_sleepers.compare_exchange_weak(sleepers, sleepers + 1));
    return SemaphoreResult();
}

void SemaphoreWake::leaveWait()
{
    m_sleepers.fetch_sub(1);
}

SemaphoreResult SemaphoreWake::sleep(
    uint32_t expected, uint32_t milliseconds, std::atomic<uint32_t>* address)
{
    return address != nullptr ? sleepAddress(*address, expected, milliseconds) : sleepNative(expected, milliseconds);
}

bool SemaphoreWake::canBlock()
{
#if defined(__EMSCRIPTEN__) && defined(__EMSCRIPTEN_PTHREADS__)
    return !emscripten_is_main_runtime_thread() && !emscripten_is_main_browser_thread();
#elif defined(__EMSCRIPTEN__)
    return false;
#else
    return true;
#endif
}

uint32_t SemaphoreWake::cleanupFailures()
{
    return s_cleanupFailures.load();
}

void SemaphoreWake::recordCleanupFailure()
{
    uint32_t current = s_cleanupFailures.load();
    while (current != UINT32_MAX && !s_cleanupFailures.compare_exchange_weak(current, current + 1))
    {

    }
}

SemaphoreResult SemaphoreWake::notifyAddress(std::atomic<uint32_t>& word)
{
#ifdef _WIN32
    if (m_wakeAddress == nullptr)
    {
        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
    }
    typedef VOID(WINAPI * SemaphoreWakeAddressFunction)(PVOID);
    reinterpret_cast<SemaphoreWakeAddressFunction>(m_wakeAddress)(&word);
#elif defined(__EMSCRIPTEN__)
#ifdef __EMSCRIPTEN_PTHREADS__
    const int32_t result = emscripten_futex_wake(&word, INT_MAX);
    if (result < 0)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, -result);
    }
#else
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#endif
#elif defined(__linux__) || defined(__ANDROID__)
    if (::syscall(SYS_futex, &word, FUTEX_WAKE_PRIVATE, INT_MAX, nullptr, nullptr, 0) < 0)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
    }
#else
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#endif
    return SemaphoreResult();
}

SemaphoreResult SemaphoreWake::sleepAddress(
    std::atomic<uint32_t>& word, uint32_t expected, uint32_t milliseconds)
{
#ifdef _WIN32
    if (m_waitAddress == nullptr)
    {
        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
    }
    typedef BOOL(WINAPI * SemaphoreWaitAddressFunction)(volatile VOID*, PVOID, SIZE_T, DWORD);
    if (reinterpret_cast<SemaphoreWaitAddressFunction>(m_waitAddress)(
            &word, &expected, sizeof(expected), milliseconds))
    {
        return SemaphoreResult();
    }
    const DWORD error = ::GetLastError();
    return SemaphoreResult(error == ERROR_TIMEOUT ? SEMAPHORE_TIMEOUT : SEMAPHORE_SYSTEM_ERROR, false,
        error == ERROR_TIMEOUT ? 0 : static_cast<int32_t>(error));
#elif defined(__EMSCRIPTEN__)
#ifdef __EMSCRIPTEN_PTHREADS__
    const int32_t result = emscripten_futex_wait(&word, expected, milliseconds);
    if (result == 0 || result == -EWOULDBLOCK || result == -EINTR)
    {
        return SemaphoreResult();
    }
    return SemaphoreResult(
        result == -ETIMEDOUT ? SEMAPHORE_TIMEOUT : SEMAPHORE_SYSTEM_ERROR, false, result == -ETIMEDOUT ? 0 : -result);
#else
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#endif
#elif defined(__linux__) || defined(__ANDROID__)
    // SYS_futex使用内核long布局的相对超时；32位libc的time64 timespec不能直接传入
    const __kernel_long_t duration[2] = {0, static_cast<__kernel_long_t>(milliseconds) * 1000000};
    if (::syscall(SYS_futex, &word, FUTEX_WAIT_PRIVATE, expected, duration, nullptr, 0) == 0)
    {
        return SemaphoreResult();
    }
    const int32_t error = errno;
    if (error == EAGAIN || error == EINTR)
    {
        return SemaphoreResult();
    }
    return SemaphoreResult(
        error == ETIMEDOUT ? SEMAPHORE_TIMEOUT : SEMAPHORE_SYSTEM_ERROR, false, error == ETIMEDOUT ? 0 : error);
#else
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#endif
}

SemaphoreResult SemaphoreWake::sleepNative(uint32_t expected, uint32_t milliseconds)
{
    if (canWaitAddress())
    {
        return sleepAddress(m_word, expected, milliseconds);
    }
#ifdef _WIN32
    // 兼容后端先复位再比较。另一等待者可消除提示，固定32ms补查保证无永久睡眠。
    if (!::ResetEvent(m_handle))
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, static_cast<int32_t>(::GetLastError()));
    }
    if (m_word.load() != expected)
    {
        return SemaphoreResult();
    }
    const DWORD result = ::WaitForSingleObject(m_handle, milliseconds);
    if (result == WAIT_OBJECT_0)
    {
        return SemaphoreResult();
    }
    return SemaphoreResult(result == WAIT_TIMEOUT ? SEMAPHORE_TIMEOUT : SEMAPHORE_SYSTEM_ERROR, false,
        result == WAIT_TIMEOUT ? 0 : static_cast<int32_t>(::GetLastError()));
#elif defined(__APPLE__)
    if (m_word.load() != expected)
    {
        return SemaphoreResult();
    }
    const long result = dispatch_semaphore_wait(static_cast<dispatch_semaphore_t>(m_handle),
        dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(milliseconds) * 1000000));
    if (result == 0)
    {
        m_hint.store(0);
    }
    return SemaphoreResult(result == 0 ? SEMAPHORE_OK : SEMAPHORE_TIMEOUT);
#else
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#endif
}