#include "SemaphoreTestPosixName.h"
#if defined(_WIN32) && defined(SEMAPHORE_TEST_POSIX_NAME)
#include <Windows.h>
#include <mutex>
#include <cstring>
#include <cstdarg>
#include <climits>

// 此锁只保护测试夹具的命名表，不进入生产库或性能计时配置
static std::mutex s_nameMutex;
static SemaphoreTestPosixSlot s_names[64] = {};

uint32_t SemaphoreTestPosixNameHandles()
{
    try
    {
        std::lock_guard<std::mutex> lock(s_nameMutex);
        uint32_t count = 0;
        for (size_t i = 0; i < 64; ++i)
        {
            if (s_names[i].m_handle != nullptr)
            {
                ++count;
            }
        }
        return count;
    }
    catch (...)
    {
        return UINT32_MAX;
    }
}

sem_t* sem_open(const char* name, int flags, ...)
{
    if (name == nullptr || name[0] != '/' || std::strlen(name) >= sizeof(s_names[0].m_name))
    {
        errno = ENAMETOOLONG;
        return SEM_FAILED;
    }
    unsigned initial = 0;
    if ((flags & O_CREAT) != 0)
    {
        va_list arguments;
        va_start(arguments, flags);
        (void)va_arg(arguments, int);
        initial = va_arg(arguments, unsigned);
        va_end(arguments);
    }
    try
    {
        std::lock_guard<std::mutex> lock(s_nameMutex);
        SemaphoreTestPosixSlot* empty = nullptr;
        for (size_t i = 0; i < 64; ++i)
        {
            SemaphoreTestPosixSlot& entry = s_names[i];
            if (entry.m_named && std::strcmp(entry.m_name, name) == 0)
            {
                if ((flags & O_CREAT) != 0 && (flags & O_EXCL) != 0)
                {
                    errno = EEXIST;
                    return SEM_FAILED;
                }
                if (entry.m_references == UINT32_MAX)
                {
                    errno = EMFILE;
                    return SEM_FAILED;
                }
                ++entry.m_references;
                return &entry;
            }
            if (entry.m_handle == nullptr && empty == nullptr)
            {
                empty = &entry;
            }
        }
        if ((flags & O_CREAT) == 0 || empty == nullptr || initial > LONG_MAX)
        {
            errno = (flags & O_CREAT) == 0 ? ENOENT : EINVAL;
            return SEM_FAILED;
        }
        empty->m_handle = ::CreateSemaphoreW(nullptr, static_cast<LONG>(initial), LONG_MAX, nullptr);
        if (empty->m_handle == nullptr)
        {
            errno = ENOMEM;
            return SEM_FAILED;
        }
        std::strcpy(empty->m_name, name);
        empty->m_named = true;
        empty->m_references = 1;
        return empty;
    }
    catch (...)
    {
        errno = EIO;
        return SEM_FAILED;
    }
}

int sem_unlink(const char* name)
{
    try
    {
        std::lock_guard<std::mutex> lock(s_nameMutex);
        for (size_t i = 0; i < 64; ++i)
        {
            SemaphoreTestPosixSlot& entry = s_names[i];
            if (entry.m_named && std::strcmp(entry.m_name, name) == 0)
            {
                if (entry.m_references == 0)
                {
                    if (!::CloseHandle(entry.m_handle))
                    {
                        errno = EIO;
                        return -1;
                    }
                    entry.m_handle = nullptr;
                }
                entry.m_named = false;
                return 0;
            }
        }
        errno = ENOENT;
    }
    catch (...)
    {
        errno = EIO;
    }
    return -1;
}

int sem_close(sem_t* value)
{
    try
    {
        std::lock_guard<std::mutex> lock(s_nameMutex);
        for (size_t i = 0; i < 64; ++i)
        {
            SemaphoreTestPosixSlot& entry = s_names[i];
            if (&entry == value && entry.m_handle != nullptr && entry.m_references != 0)
            {
                if (entry.m_references == 1 && !entry.m_named)
                {
                    if (!::CloseHandle(entry.m_handle))
                    {
                        errno = EIO;
                        return -1;
                    }
                    entry.m_handle = nullptr;
                }
                --entry.m_references;
                return 0;
            }
        }
        errno = EINVAL;
    }
    catch (...)
    {
        errno = EIO;
    }
    return -1;
}

int sem_post(sem_t* value)
{
    if (value == nullptr || value == SEM_FAILED)
    {
        errno = EINVAL;
        return -1;
    }
    if (::ReleaseSemaphore(value->m_handle, 1, nullptr))
    {
        return 0;
    }
    errno = ::GetLastError() == ERROR_TOO_MANY_POSTS ? EOVERFLOW : EINVAL;
    return -1;
}

int sem_wait(sem_t* value)
{
    if (value != nullptr && value != SEM_FAILED && ::WaitForSingleObject(value->m_handle, INFINITE) == WAIT_OBJECT_0)
    {
        return 0;
    }
    errno = EINVAL;
    return -1;
}

int sem_trywait(sem_t* value)
{
    if (value == nullptr || value == SEM_FAILED)
    {
        errno = EINVAL;
        return -1;
    }
    const DWORD waited = ::WaitForSingleObject(value->m_handle, 0);
    if (waited == WAIT_OBJECT_0)
    {
        return 0;
    }
    errno = waited == WAIT_TIMEOUT ? EAGAIN : EINVAL;
    return -1;
}

int nanosleep(const timespec* requested, timespec* remaining)
{
    if (requested == nullptr || requested->tv_sec != 0 || requested->tv_nsec < 0 || requested->tv_nsec >= 1000000000)
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
#endif