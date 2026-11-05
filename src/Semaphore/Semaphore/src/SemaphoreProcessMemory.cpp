#include "SemaphoreProcessMemory.h"
#include "SemaphoreWake.h"
#ifdef _WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#endif

SemaphoreProcessMemory::SemaphoreProcessMemory() :
m_words(nullptr),
m_handle(nullptr),
m_directoryDescriptor(-1),
m_descriptor(-1),
m_created(false)
{

}

SemaphoreProcessMemory::~SemaphoreProcessMemory()
{
    if (close().status != SEMAPHORE_OK)
    {
        SemaphoreWake::recordCleanupFailure();
#if defined(SEMAPHORE_SHARED_COUNTER) && !defined(_WIN32)
        // 析构后已无法重试撤名；记录错误后仍须释放保留的本地映射和描述符
        m_created = false;
        m_name.clear();
        if (close().status != SEMAPHORE_OK)
        {
            SemaphoreWake::recordCleanupFailure();
        }
#endif
    }
}

std::string& SemaphoreProcessMemory::directory()
{
    static std::string s_directory;
    return s_directory;
}

std::atomic<bool>& SemaphoreProcessMemory::directorySealed()
{
    static std::atomic<bool> s_sealed(false);
    return s_sealed;
}

int32_t SemaphoreProcessMemory::openDirectory(const std::string& path)
{
#if defined(SEMAPHORE_SHARED_COUNTER) && !defined(_WIN32)
    if (path.empty() || path[0] != '/' || path.find('\0') != std::string::npos)
    {
        errno = EINVAL;
        return -1;
    }
    const int32_t descriptor = ::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (descriptor < 0)
    {
        return -1;
    }
    struct stat info = {};
    const int32_t inspected = ::fstat(descriptor, &info);
    if (inspected != 0 || !S_ISDIR(info.st_mode) || info.st_uid != ::geteuid())
    {
        const int32_t error = inspected != 0 ? errno : EACCES;
        if (::close(descriptor) != 0)
        {
            SemaphoreWake::recordCleanupFailure();
        }
        errno = error;
        return -1;
    }
    return descriptor;
#else
    (void)path;
    return -1;
#endif
}

int32_t SemaphoreProcessMemory::load(int32_t* value)
{
#ifdef _WIN32
    return static_cast<int32_t>(::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(value), 0, 0));
#else
    static_assert(__atomic_always_lock_free(sizeof(int32_t), 0), "IPC requires native lock-free 32-bit atomics");
    return __atomic_load_n(value, __ATOMIC_SEQ_CST);
#endif
}

bool SemaphoreProcessMemory::compareExchange(int32_t* value, int32_t expected, int32_t desired)
{
#ifdef _WIN32
    return ::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(value), desired, expected) == expected;
#else
    return __atomic_compare_exchange_n(value, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#endif
}

SemaphoreResult SemaphoreProcessMemory::open(const std::string& logicalName, int32_t limit)
{
#ifndef SEMAPHORE_SHARED_COUNTER
    (void)logicalName;
    (void)limit;
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#elif defined(_WIN32)
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
#else
    m_name = "m_" + logicalName + ".s";
    directorySealed().store(true);
    m_directoryDescriptor = openDirectory(directory());
    if (m_directoryDescriptor < 0)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
    }
    if (limit > 0)
    {
        m_descriptor =
            ::openat(m_directoryDescriptor, m_name.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        m_created = m_descriptor >= 0;
        if (m_descriptor < 0 && errno == EEXIST)
        {
            m_descriptor = ::openat(m_directoryDescriptor, m_name.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
        }
    }
    else
    {
        m_descriptor = ::openat(m_directoryDescriptor, m_name.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
    }
    if (m_descriptor < 0)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
    }
    const size_t bytes = 2 * sizeof(int32_t);
    if (m_created)
    {
#ifdef __APPLE__
        fstore_t allocation = {};
        allocation.fst_flags = F_ALLOCATEALL;
        allocation.fst_posmode = F_PEOFPOSMODE;
        allocation.fst_length = static_cast<off_t>(bytes);
        if (::fcntl(m_descriptor, F_PREALLOCATE, &allocation) != 0)
        {
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
        }
#else
        const int32_t allocated = ::posix_fallocate(m_descriptor, 0, static_cast<off_t>(bytes));
        if (allocated != 0)
        {
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, allocated);
        }
#endif
        if (::ftruncate(m_descriptor, static_cast<off_t>(bytes)) != 0)
        {
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
        }
    }
    struct stat info = {};
    if (::fstat(m_descriptor, &info) != 0)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
    }
    if (!S_ISREG(info.st_mode) || info.st_uid != ::geteuid() || info.st_size != static_cast<off_t>(bytes))
    {
        return SemaphoreResult(SEMAPHORE_INVALID);
    }
    void* address = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, m_descriptor, 0);
    if (address == MAP_FAILED)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
    }
    m_words = static_cast<int32_t*>(address);
    // 加入者不修复上限为0的未知/未完成对象，只有独占创建者可发布布局。
    if (m_created)
    {
        compareExchange(m_words + 1, 0, limit);
    }
    const int32_t maximum = load(m_words + 1);
    const int32_t count = load(m_words);
    return SemaphoreResult(maximum > 0 && count >= 0 && count <= maximum ? SEMAPHORE_OK : SEMAPHORE_INVALID);
#endif
}

SemaphoreResult SemaphoreProcessMemory::unlinkName()
{
#if defined(SEMAPHORE_SHARED_COUNTER) && !defined(_WIN32)
    if (!m_created || m_name.empty() || m_directoryDescriptor < 0)
    {
        return SemaphoreResult(SEMAPHORE_INVALID);
    }
    struct stat opened = {};
    struct stat named = {};
    if (::fstat(m_descriptor, &opened) != 0)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
    }
    if (::fstatat(m_directoryDescriptor, m_name.c_str(), &named, AT_SYMLINK_NOFOLLOW) != 0)
    {
        if (errno != ENOENT)
        {
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
        }
    }
    else
    {
        if (opened.st_dev != named.st_dev || opened.st_ino != named.st_ino)
        {
            m_created = false;
            m_name.clear();
            return SemaphoreResult(SEMAPHORE_INVALID);
        }
        if (::unlinkat(m_directoryDescriptor, m_name.c_str(), 0) != 0 && errno != ENOENT)
        {
            return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
        }
    }
    m_created = false;
    m_name.clear();
    return SemaphoreResult();
#else
    return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
#endif
}

SemaphoreResult SemaphoreProcessMemory::close()
{
    SemaphoreResult result;
#ifdef _WIN32
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
#elif defined(SEMAPHORE_SHARED_COUNTER)
    if (m_created)
    {
        result = unlinkName();
    }
    if (m_words != nullptr)
    {
        if (::munmap(m_words, 2 * sizeof(int32_t)) != 0)
        {
            if (result.status == SEMAPHORE_OK)
            {
                result = SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
            }
        }
        else
        {
            m_words = nullptr;
        }
    }
    if (m_created)
    {
        // 撤名失败保留原名称及两个描述符，重试时仍能核对同一设备/inode
        // 映射已经单独尝试释放；不能丢掉撤名责任后把下一次关闭伪报成功
        return result;
    }
    // close(EINTR)不能跨平台盲目重试同一fd，以免关闭后来复用的描述符。
    if (m_descriptor >= 0)
    {
        const int32_t descriptor = m_descriptor;
        m_descriptor = -1;
        if (::close(descriptor) != 0 && result.status == SEMAPHORE_OK)
        {
            result = SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
        }
    }
    if (m_directoryDescriptor >= 0)
    {
        const int32_t descriptor = m_directoryDescriptor;
        m_directoryDescriptor = -1;
        if (::close(descriptor) != 0 && result.status == SEMAPHORE_OK)
        {
            result = SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, errno);
        }
    }
#endif
    return result;
}