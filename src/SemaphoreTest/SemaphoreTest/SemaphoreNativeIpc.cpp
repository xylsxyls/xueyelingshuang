#include "SemaphoreNativeIpc.h"
#include <cstdio>
#include <system_error>

std::atomic<uint32_t> SemaphoreNativeIpc::s_nativeCleanupFailures(0);

SemaphoreNativeIpc::SemaphoreNativeIpc() :
m_nativeHandle(nullptr),
m_nativeWait(nullptr),
m_nativeRelease(nullptr)
{
#ifdef SEMAPHORE_TEST_NATIVE_NT
    HMODULE module = ::GetModuleHandleW(L"ntdll.dll");
    if (module == nullptr)
    {
        throw std::system_error(static_cast<int32_t>(::GetLastError()), std::system_category());
    }
    m_nativeWait = reinterpret_cast<void*>(::GetProcAddress(module, "NtWaitForSingleObject"));
    m_nativeRelease = reinterpret_cast<void*>(::GetProcAddress(module, "NtReleaseSemaphore"));
    if (m_nativeWait == nullptr || m_nativeRelease == nullptr)
    {
        throw std::system_error(std::make_error_code(std::errc::not_supported));
    }
#endif
}

SemaphoreNativeIpc::~SemaphoreNativeIpc()
{
    if (m_nativeHandle != nullptr && !::CloseHandle(m_nativeHandle))
    {
        s_nativeCleanupFailures.fetch_add(1);
        std::fprintf(stderr, "NATIVE_CLEANUP_FAILURE error=%lu\n", ::GetLastError());
    }
}

uint32_t SemaphoreNativeIpc::nativeCleanupFailures()
{
    return s_nativeCleanupFailures.load();
}

SemaphoreSignalWaitResult SemaphoreNativeIpc::processSignalAndWaitResult(SemaphoreNativeIpc& waitOn, int32_t timeout)
{
    const DWORD result = ::SignalObjectAndWait(
        m_nativeHandle, waitOn.m_nativeHandle, timeout < 0 ? INFINITE : static_cast<DWORD>(timeout), FALSE);
    if (result == WAIT_OBJECT_0)
    {
        return SemaphoreSignalWaitResult(SEMAPHORE_OK, SEMAPHORE_COMMIT_DONE, SEMAPHORE_COMMIT_DONE);
    }
    if (result == WAIT_TIMEOUT)
    {
        return SemaphoreSignalWaitResult(SEMAPHORE_TIMEOUT, SEMAPHORE_COMMIT_DONE, SEMAPHORE_COMMIT_NONE);
    }
    return SemaphoreSignalWaitResult(SEMAPHORE_SYSTEM_ERROR, SEMAPHORE_COMMIT_UNKNOWN, SEMAPHORE_COMMIT_UNKNOWN,
        static_cast<int32_t>(::GetLastError()));
}

void SemaphoreNativeIpc::createProcessSemaphore(const std::string& name, int32_t maximum)
{
    if (m_nativeHandle != nullptr)
    {
        throw std::system_error(std::make_error_code(std::errc::invalid_argument));
    }
    m_nativeHandle = ::CreateSemaphoreA(nullptr, 0, maximum, name.c_str());
    if (m_nativeHandle == nullptr)
    {
        throw std::system_error(static_cast<int32_t>(::GetLastError()), std::system_category());
    }
}

bool SemaphoreNativeIpc::tryOpenProcessSemaphore(const std::string& name)
{
    if (m_nativeHandle != nullptr)
    {
        return true;
    }
    m_nativeHandle = ::OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, name.c_str());
    return m_nativeHandle != nullptr;
}

int32_t SemaphoreNativeIpc::tryProcessSignal()
{
    if (m_nativeRelease != nullptr)
    {
        typedef LONG(NTAPI * SemaphoreNativeReleaseFunction)(HANDLE, LONG, LONG*);
        const LONG status = reinterpret_cast<SemaphoreNativeReleaseFunction>(m_nativeRelease)(m_nativeHandle, 1, nullptr);
        return status == 0 ? 0 : (static_cast<uint32_t>(status) == 0xc0000047 ? 1 : -1);
    }
    if (::ReleaseSemaphore(m_nativeHandle, 1, nullptr))
    {
        return 0;
    }
    return ::GetLastError() == ERROR_TOO_MANY_POSTS ? 1 : -1;
}

bool SemaphoreNativeIpc::processWait(int32_t timeout)
{
    if (m_nativeWait != nullptr)
    {
        typedef LONG(NTAPI * SemaphoreNativeWaitFunction)(HANDLE, BOOLEAN, LARGE_INTEGER*);
        LARGE_INTEGER limit;
        limit.QuadPart = -static_cast<int64_t>(timeout) * 10000;
        const LONG status = reinterpret_cast<SemaphoreNativeWaitFunction>(m_nativeWait)(
            m_nativeHandle, FALSE, timeout == -1 ? nullptr : &limit);
        if (status != 0 && status != 0x102)
        {
            throw std::system_error(status, std::system_category());
        }
        return status == 0;
    }
    const DWORD result = ::WaitForSingleObject(m_nativeHandle, timeout == -1 ? INFINITE : static_cast<DWORD>(timeout));
    if (result == WAIT_OBJECT_0)
    {
        return true;
    }
    if (result == WAIT_TIMEOUT)
    {
        return false;
    }
    throw std::system_error(static_cast<int32_t>(::GetLastError()), std::system_category());
}