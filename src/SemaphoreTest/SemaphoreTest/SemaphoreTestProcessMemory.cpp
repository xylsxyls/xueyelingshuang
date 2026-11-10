#include "SemaphoreTestProcessMemory.h"
#include "SemaphoreTestHooks.h"
#include "SemaphoreWake.h"
#include <Windows.h>

int nanosleep(const timespec* requested, timespec* remaining)
{
    if (requested == nullptr || requested->tv_sec != 0 || requested->tv_nsec < 0 ||
        requested->tv_nsec >= 1000000000)
    {
        errno = EINVAL;
        return -1;
    }
    ::Sleep(static_cast<DWORD>((requested->tv_nsec + 999999) / 1000000));
    if (remaining != nullptr)
    {
        remaining->tv_sec = 0;
        remaining->tv_nsec = 0;
    }
    return 0;
}

SemaphoreProcessMemory::SemaphoreProcessMemory() :
m_words(nullptr),
m_handle(nullptr)
{

}

SemaphoreProcessMemory::~SemaphoreProcessMemory()
{
    if (close().status != SEMAPHORE_OK)
    {
        SemaphoreWake::recordCleanupFailure();
    }
}

int32_t SemaphoreProcessMemory::load(int32_t* value)
{
    return static_cast<int32_t>(::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(value), 0, 0));
}

bool SemaphoreProcessMemory::compareExchange(int32_t* value, int32_t expected, int32_t desired)
{
    return ::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(value), desired, expected) == expected;
}

SemaphoreResult SemaphoreProcessMemory::open(const std::string& logicalName, int32_t limit)
{
    // 仅桌面算法测试，不代表手机文件权限和生命周期已验证。
    const std::string nativeName = logicalName + ".s";
    m_handle = limit > 0 ? ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 2 * sizeof(int32_t),
                               nativeName.c_str())
                         : ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, nativeName.c_str());
    if (m_handle == nullptr)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, static_cast<int32_t>(::GetLastError()));
    }
    const bool created = limit > 0 && ::GetLastError() != ERROR_ALREADY_EXISTS;
    m_words = static_cast<int32_t*>(::MapViewOfFile(m_handle, FILE_MAP_ALL_ACCESS, 0, 0, 2 * sizeof(int32_t)));
    if (m_words == nullptr)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, static_cast<int32_t>(::GetLastError()));
    }
    if (created)
    {
        compareExchange(m_words + 1, 0, limit);
    }
    const int32_t maximum = load(m_words + 1);
    const int32_t count = load(m_words);
    return SemaphoreResult(maximum > 0 && count >= 0 && count <= maximum ? SEMAPHORE_OK : SEMAPHORE_INVALID);
}

SemaphoreResult SemaphoreProcessMemory::unlinkName()
{
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
}

SemaphoreResult SemaphoreProcessMemory::close()
{
    SemaphoreResult result;
    if (m_words != nullptr)
    {
        if (!::UnmapViewOfFile(m_words))
        {
            result = SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, static_cast<int32_t>(::GetLastError()));
        }
        else
        {
            m_words = nullptr;
        }
    }
    if (m_handle != nullptr)
    {
        const int32_t injected = SemaphoreTestFault(10);
        if (injected != 0)
        {
            return result.status == SEMAPHORE_OK ? SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, injected) : result;
        }
        if (!::CloseHandle(m_handle))
        {
            if (result.status == SEMAPHORE_OK)
            {
                result = SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, static_cast<int32_t>(::GetLastError()));
            }
        }
        else
        {
            m_handle = nullptr;
        }
    }
    return result;
}