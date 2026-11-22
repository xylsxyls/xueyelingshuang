#ifndef _WIN32
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#endif
#include "SharedMemory.h"
#include <cstring>
#ifdef _WIN32
#include <Windows.h>
#else
#include <cstdlib>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#endif

#ifdef _WIN32
void* SharedMemory::openMappingHandle(bool readOnly) const
{
    HANDLE handle = ::OpenFileMappingA(readOnly ? FILE_MAP_READ : FILE_MAP_READ | FILE_MAP_WRITE, FALSE, m_memoryName.c_str());
    if (handle == nullptr && ::GetLastError() == ERROR_ACCESS_DENIED)
    {
        // FILE_MAP_WRITE 本身允许读写映射，访问控制表可以只授予这一项权限。
        handle = ::OpenFileMappingA(FILE_MAP_WRITE, FALSE, m_memoryName.c_str());
    }
    return handle;
}

void* SharedMemory::mapReadableView() const
{
    void* view = ::MapViewOfFile(m_memoryHandle, FILE_MAP_READ, 0, 0, 0);
    if (view == nullptr && ::GetLastError() == ERROR_ACCESS_DENIED)
    {
        view = ::MapViewOfFile(m_memoryHandle, FILE_MAP_WRITE, 0, 0, 0);
    }
    return view;
}

uint64_t SharedMemory::windowsViewBytes(void* view, uint64_t required, bool writableOnly)
{
    uintptr_t address = reinterpret_cast<uintptr_t>(view);
    uint64_t bytes = 0;
    while (bytes < required)
    {
        MEMORY_BASIC_INFORMATION info = {};
        if (::VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) != sizeof(info) ||
            info.AllocationBase != view || info.RegionSize == 0)
        {
            break;
        }
        if (writableOnly)
        {
            const DWORD protection = info.Protect & 0xff;
            if (info.State != MEM_COMMIT || (info.Protect & PAGE_GUARD) != 0 ||
                (protection != PAGE_READWRITE && protection != PAGE_WRITECOPY &&
                 protection != PAGE_EXECUTE_READWRITE && protection != PAGE_EXECUTE_WRITECOPY))
            {
                break;
            }
        }
        bytes += info.RegionSize;
        if (info.RegionSize > UINTPTR_MAX - address)
        {
            break;
        }
        address += info.RegionSize;
    }
    return bytes;
}
#else
int SharedMemory::m_processDirectoryDescriptor = -1;
std::atomic<bool> SharedMemory::m_processDirectorySealed(false);

void SharedMemory::closeProcessDirectory()
{
    if (m_processDirectoryDescriptor >= 0)
    {
        ::close(m_processDirectoryDescriptor);
        m_processDirectoryDescriptor = -1;
    }
}

#if !defined(__EMSCRIPTEN__)
int SharedMemory::openSharedObject(int flags) const
{
#ifndef SHARED_MEMORY_MOBILE
    if (m_directoryDescriptor < 0)
    {
        return ::shm_open(m_nativeName.c_str(), flags, 0600);
    }
#endif
    return ::openat(m_directoryDescriptor, m_nativeName.c_str(), flags | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0600);
}

bool SharedMemory::initializeStorage() const
{
    // Linux 的 shm_open 和目录文件都需预留空间，其他 POSIX 平台只预留目录文件。
#ifndef __linux__
    if (m_directoryDescriptor >= 0)
#endif
    {
#ifdef __APPLE__
        fstore_t allocation = {};
        allocation.fst_flags = F_ALLOCATEALL;
        allocation.fst_posmode = F_PEOFPOSMODE;
        allocation.fst_length = static_cast<off_t>(m_size);
        if (::fcntl(m_descriptor, F_PREALLOCATE, &allocation) < 0)
        {
            return false;
        }
#else
        // posix_fallocate 直接返回错误码，EINTR 时在同一描述符上重试。
        int result;
        do
        {
            result = ::posix_fallocate(m_descriptor, 0, static_cast<off_t>(m_size));
        } while (result == EINTR);
        if (result != 0)
        {
            return false;
        }
#endif
    }
    int result;
    do
    {
        result = ::ftruncate(m_descriptor, static_cast<off_t>(m_size));
    } while (result < 0 && errno == EINTR);
    return result == 0;
}

uint32_t SharedMemory::sharedObjectCapacity() const
{
    struct stat info = {};
    if (::fstat(m_descriptor, &info) != 0 || (m_directoryDescriptor >= 0 && !S_ISREG(info.st_mode)) ||
        info.st_size <= 0 || static_cast<uint64_t>(info.st_size) > UINT32_MAX)
    {
        return 0;
    }
    return static_cast<uint32_t>(info.st_size);
}
#endif
#endif

bool SharedMemory::setProcessDirectory(const std::string& directory)
{
#ifdef _WIN32
    return directory.empty();
#else
    static std::string s_path;
    if (m_processDirectorySealed.load())
    {
        return directory == s_path;
    }
    if (directory.empty())
    {
#ifdef SHARED_MEMORY_MOBILE
        return false;
#else
        closeProcessDirectory();
        s_path.clear();
        return true;
#endif
    }
    if (directory[0] != '/' || directory.find('\0') != std::string::npos)
    {
        return false;
    }
    std::string selectedPath(directory);
    const int descriptor = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (descriptor < 0)
    {
        return false;
    }
    struct stat info = {};
    if (::fstat(descriptor, &info) != 0 || !S_ISDIR(info.st_mode) || info.st_uid != ::geteuid())
    {
        ::close(descriptor);
        return false;
    }
    // 启动线程只在首次成功取得目录时登记一次清理，切换目录沿用同一个退出回调。
    static bool s_cleanupRegistered = false;
    if (!s_cleanupRegistered)
    {
        if (std::atexit(closeProcessDirectory) != 0)
        {
            ::close(descriptor);
            return false;
        }
        s_cleanupRegistered = true;
    }
    closeProcessDirectory();
    s_path.swap(selectedPath);
    m_processDirectoryDescriptor = descriptor;
    return true;
#endif
}

SharedMemory::SharedMemory(const std::string& name, uint32_t size, bool isAllowExist) :
m_memoryName(name), m_size(size), m_memoryPtr(nullptr),
#ifdef _WIN32
m_memoryHandle(nullptr), m_readMemoryPtr(nullptr), m_writeMemoryPtr(nullptr)
#else
m_descriptor(-1), m_readOnly(false), m_capacity(0), m_created(false), m_nameUnlinked(false), m_directoryDescriptor(-1)
#endif
{
    if (name.find('\0') != std::string::npos || (size == 0 && name.empty()))
    {
        return;
    }
#ifdef _WIN32
    if (size != 0)
    {
        m_memoryHandle = ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size,
            name.empty() ? nullptr : name.c_str());
        const DWORD status = ::GetLastError();
        bool joined = m_memoryHandle != nullptr && !name.empty() && status == ERROR_ALREADY_EXISTS;
        if (m_memoryHandle == nullptr && isAllowExist && !name.empty() && status == ERROR_ACCESS_DENIED)
        {
            // 既有对象可能只允许映射读写，创建 API 所需的其他对象权限不应妨碍加入。
            m_memoryHandle = openMappingHandle(false);
            joined = m_memoryHandle != nullptr;
        }
        if (joined)
        {
            // 已存在时 CreateFileMapping 忽略 size；验证请求的范围确实可写。
            void* checked = isAllowExist ? ::MapViewOfFile(m_memoryHandle, FILE_MAP_WRITE, 0, 0, size) : nullptr;
            if (checked != nullptr)
            {
                ::UnmapViewOfFile(checked);
            }
            else
            {
                ::CloseHandle(m_memoryHandle);
                m_memoryHandle = nullptr;
            }
        }
    }
#else
    if (name.find('/') != std::string::npos || name == "." || name == "..")
    {
        return;
    }
    if (!name.empty())
    {
        const int directory = m_processDirectoryDescriptor;
        if (name.size() > (directory < 0 ? 30 : 128))
        {
            return;
        }
#ifdef SHARED_MEMORY_MOBILE
        if (directory < 0)
        {
            return;
        }
#endif
        m_processDirectorySealed.store(true);
        if (directory >= 0)
        {
            // 所有可能分配字符串的操作放在取得描述符之前，构造抛出时不能泄漏目录引用。
            m_nativeName = "m_" + name;
            m_directoryDescriptor = ::fcntl(directory, F_DUPFD_CLOEXEC, 0);
            if (m_directoryDescriptor < 0)
            {
                m_nativeName.clear();
                return;
            }
        }
#ifndef SHARED_MEMORY_MOBILE
        else
        {
            m_nativeName = "/" + name;
        }
#endif
    }
    if (size != 0 && name.empty())
    {
        m_capacity = size;
        return;
    }
#if !defined(__EMSCRIPTEN__)
    if (size != 0)
    {
        m_descriptor = openSharedObject(O_RDWR | O_CREAT | O_EXCL);
        m_created = m_descriptor >= 0;
        if (!m_created && isAllowExist && errno == EEXIST)
        {
            m_descriptor = openSharedObject(O_RDWR);
        }
        if (m_descriptor >= 0)
        {
            if (!m_created || initializeStorage())
            {
                m_capacity = sharedObjectCapacity();
            }
            if (m_capacity < size)
            {
                ::close(m_descriptor);
                m_descriptor = -1;
                m_capacity = 0;
                if (m_created)
                {
                    unlinkName();
                    m_created = false;
                }
            }
        }
        if (m_descriptor < 0)
        {
            // 创建或加入失败的实例没有取得该名称，不能撤销其他实例的映射。
            m_nativeName.clear();
        }
    }
#endif
#endif
}

SharedMemory::~SharedMemory()
{
    close();
#ifdef _WIN32
    if (m_memoryHandle != nullptr)
    {
        ::CloseHandle(m_memoryHandle);
    }
#else
    if (m_descriptor >= 0)
    {
        ::close(m_descriptor);
    }
    if (m_created)
    {
        unlinkName();
    }
    if (m_directoryDescriptor >= 0)
    {
        ::close(m_directoryDescriptor);
    }
#endif
}

void SharedMemory::open(bool readOnly)
{
    if (m_size != 0 || m_memoryName.empty())
    {
        return;
    }
#ifdef _WIN32
    if (m_memoryHandle == nullptr && m_memoryName.find('\0') == std::string::npos)
    {
        m_memoryHandle = openMappingHandle(readOnly);
    }
#elif !defined(__EMSCRIPTEN__)
    if (m_descriptor < 0 && !m_nativeName.empty())
    {
        // 优先取得可读写描述符，允许同一实例在读后继续写；权限不足时读操作可降为只读。
        for (int attempt = 0; attempt < (readOnly ? 2 : 1); ++attempt)
        {
            const int access = attempt == 0 ? O_RDWR : O_RDONLY;
            m_descriptor = openSharedObject(access);
            if (m_descriptor >= 0)
            {
                m_readOnly = (access == O_RDONLY);
                break;
            }
            if (errno != EACCES && errno != EPERM && errno != EROFS)
            {
                break;
            }
        }
        if (m_descriptor >= 0)
        {
            m_capacity = sharedObjectCapacity();
            if (m_capacity != 0)
            {
                return;
            }
            ::close(m_descriptor);
            m_descriptor = -1;
            m_readOnly = false;
        }
    }
#endif
}

bool SharedMemory::isFailed()
{
#ifdef _WIN32
    return m_memoryHandle == nullptr;
#else
    return m_descriptor < 0 && !(m_memoryName.empty() && m_capacity != 0);
#endif
}

uint32_t SharedMemory::size()
{
    return m_size;
}

std::string SharedMemory::mapName()
{
    return m_memoryName;
}

void* SharedMemory::memory()
{
    return m_memoryPtr;
}

uint32_t SharedMemory::realSize()
{
#ifdef _WIN32
    open(true);
    if (m_memoryHandle == nullptr)
    {
        return 0;
    }
    void* view = m_writeMemoryPtr != nullptr ? m_writeMemoryPtr : m_readMemoryPtr;
    const bool temporary = view == nullptr;
    if (temporary)
    {
        view = mapReadableView();
    }
    if (view == nullptr)
    {
        return 0;
    }
    const uint64_t capacity = windowsViewBytes(view, static_cast<uint64_t>(UINT32_MAX) + 1, false);
    if (temporary)
    {
        ::UnmapViewOfFile(view);
    }
    return capacity <= UINT32_MAX ? static_cast<uint32_t>(capacity) : 0;
#else
    open(true);
    return m_capacity;
#endif
}

void* SharedMemory::readWithoutLock()
{
#ifdef _WIN32
    if (m_readMemoryPtr == nullptr)
    {
        open(true);
        if (m_memoryHandle == nullptr)
        {
            return nullptr;
        }
        m_readMemoryPtr = mapReadableView();
        if (m_readMemoryPtr == nullptr)
        {
            return nullptr;
        }
    }
    return m_memoryPtr = m_readMemoryPtr;
#else
    open(true);
    return mapPosixView(m_readOnly ? PROT_READ : PROT_READ | PROT_WRITE);
#endif
}

void* SharedMemory::writeWithoutLock()
{
#ifdef _WIN32
    if (m_writeMemoryPtr == nullptr)
    {
        open(false);
        if (m_memoryHandle == nullptr)
        {
            return nullptr;
        }
        m_writeMemoryPtr = ::MapViewOfFile(m_memoryHandle, FILE_MAP_WRITE, 0, 0, 0);
        if (m_writeMemoryPtr == nullptr && m_size == 0)
        {
            // 先取得替代引用，成功映射后才关闭旧句柄，避免关闭最后引用导致对象消失。
            HANDLE upgraded = openMappingHandle(false);
            if (upgraded != nullptr)
            {
                void* view = ::MapViewOfFile(upgraded, FILE_MAP_WRITE, 0, 0, 0);
                if (view != nullptr)
                {
                    ::CloseHandle(m_memoryHandle);
                    m_memoryHandle = upgraded;
                    m_writeMemoryPtr = view;
                }
                else
                {
                    ::CloseHandle(upgraded);
                }
            }
        }
        if (m_writeMemoryPtr == nullptr)
        {
            return nullptr;
        }
    }
    return m_memoryPtr = m_writeMemoryPtr;
#else
    if (m_capacity == 0)
    {
        open(false);
    }
    return m_readOnly ? nullptr : mapPosixView(PROT_READ | PROT_WRITE);
#endif
}

#ifndef _WIN32
void* SharedMemory::mapPosixView(int protection)
{
    if (m_memoryPtr != nullptr || m_capacity == 0)
    {
        return m_memoryPtr;
    }
    const int flags = MAP_SHARED | (m_memoryName.empty() ? MAP_ANON : 0);
    void* view = ::mmap(nullptr, m_capacity, protection, flags, m_descriptor, 0);
    if (view == MAP_FAILED)
    {
        return nullptr;
    }
    return m_memoryPtr = view;
}
#endif

void SharedMemory::close()
{
#ifdef _WIN32
    if (m_readMemoryPtr != nullptr)
    {
        ::UnmapViewOfFile(m_readMemoryPtr);
        m_readMemoryPtr = nullptr;
    }
    if (m_writeMemoryPtr != nullptr)
    {
        ::UnmapViewOfFile(m_writeMemoryPtr);
        m_writeMemoryPtr = nullptr;
    }
#else
    if (m_memoryPtr != nullptr)
    {
        ::munmap(m_memoryPtr, m_capacity);
    }
#endif
    m_memoryPtr = nullptr;
}

void SharedMemory::clear()
{
    if (m_size == 0)
    {
        return;
    }
    void* view = writeWithoutLock();
    if (view == nullptr)
    {
        return;
    }
#ifdef _WIN32
    if (windowsViewBytes(view, m_size, true) < m_size)
    {
        return;
    }
#endif
    std::memset(view, 0, m_size);
}

void SharedMemory::preserveName()
{
#ifndef _WIN32
    m_created = false;
#endif
}

bool SharedMemory::unlinkName()
{
#if defined(_WIN32) || defined(__EMSCRIPTEN__)
    return false;
#else
    if (m_nativeName.empty())
    {
        return false;
    }
    if (m_nameUnlinked)
    {
        return true;
    }
    int result = -1;
    if (m_directoryDescriptor >= 0)
    {
        result = ::unlinkat(m_directoryDescriptor, m_nativeName.c_str(), 0);
    }
#ifndef SHARED_MEMORY_MOBILE
    else
    {
        result = ::shm_unlink(m_nativeName.c_str());
    }
#endif
    if (result == 0 || errno == ENOENT)
    {
        // 当前对象已撤名；重复显式调用和析构均不会再触碰后来创建的同名对象。
        m_created = false;
        m_nameUnlinked = true;
        return true;
    }
    return false;
#endif
}

// 使用示例：在本翻译单元末尾启用以下 main，单独构建控制台程序
// #include <chrono>
// #include <sstream>
//
// int main(int argc, char** argv)
// {
// #ifdef SHARED_MEMORY_MOBILE
//     // 由应用传入具有共享权限的私有目录或 App Group 目录。
//     if (argc != 2 || !SharedMemory::setProcessDirectory(argv[1]))
//     {
//         return 1;
//     }
// #else
//     (void)argc;
//     (void)argv;
//     if (!SharedMemory::setProcessDirectory(""))
//     {
//         return 1;
//     }
// #endif
//     std::ostringstream key;
//     key << "smd" << std::hex << std::chrono::steady_clock::now().time_since_epoch().count();
//     SharedMemory owner(key.str(), 4096, false);
//     if (owner.isFailed())
//     {
//         return 2;
//     }
//     void* writable = owner.writeWithoutLock();
//     if (writable == nullptr || owner.realSize() < owner.size())
//     {
//         return 3;
//     }
//     const char message[] = "ready";
//     std::memcpy(writable, message, sizeof(message));
//     SharedMemory peer(owner.mapName());
//     const void* readable = peer.readWithoutLock();
//     if (readable == nullptr || readable != peer.memory() || peer.realSize() < sizeof(message) ||
//         std::memcmp(readable, message, sizeof(message)) != 0)
//     {
//         return 4;
//     }
//     owner.clear();
//     const char zero[sizeof(message)] = {};
//     const bool cleared = std::memcmp(readable, zero, sizeof(zero)) == 0;
//     peer.close();
// #ifndef _WIN32
//     const bool removed = owner.unlinkName();
//     if (!removed)
//     {
//         return 5;
//     }
// #endif
//     return cleared ? 0 : 6;
// }