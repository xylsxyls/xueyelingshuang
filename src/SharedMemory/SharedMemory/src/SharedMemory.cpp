#ifndef _WIN32
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif
#endif
#include "SharedMemory.h"
#include <cstring>
#include <atomic>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#endif

#ifndef _WIN32
/** 保存由宿主在启动期配置的共享目录；配置后不再修改。
@return 进程内唯一配置存储
*/
static std::string& SharedDirectory()
{
    static std::string s_directory;
    return s_directory;
}

/** 首次使用命名映射后封存配置，生命周期owner不得并发配置。
@return 封存标志
*/
static std::atomic<bool>& SharedDirectorySealed()
{
    static std::atomic<bool> s_sealed(false);
    return s_sealed;
}
#endif

bool SharedMemory::setProcessDirectory(const std::string& directory)
{
#ifdef _WIN32
    return directory.empty();
#else
    if (SharedDirectorySealed().load())
    {
        return directory == SharedDirectory();
    }
    if (directory.empty())
    {
#ifdef SHARED_MEMORY_MOBILE
        return false;
#else
        SharedDirectory().clear();
        return true;
#endif
    }
    if (directory[0] != '/' || directory.find('\0') != std::string::npos)
    {
        return false;
    }
    const int descriptor = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (descriptor < 0)
    {
        return false;
    }
    struct stat info = {};
    const bool valid = ::fstat(descriptor, &info) == 0 && S_ISDIR(info.st_mode) && info.st_uid == ::geteuid();
    ::close(descriptor);
    if (valid)
    {
        SharedDirectory() = directory;
    }
    return valid;
#endif
}

SharedMemory::SharedMemory(const std::string& name, uint32_t size, bool isAllowExist):
    m_memoryName(name), m_size(size), m_memoryPtr(nullptr),
#ifdef _WIN32
    m_memoryHandle(nullptr), m_readMemoryPtr(nullptr), m_writeMemoryPtr(nullptr)
#else
    m_descriptor(-1), m_capacity(0), m_created(false), m_directoryDescriptor(-1)
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
        if (m_memoryHandle != nullptr && !isAllowExist && ::GetLastError() == ERROR_ALREADY_EXISTS)
        {
            ::CloseHandle(m_memoryHandle);
            m_memoryHandle = nullptr;
        }
    }
#else
    if (name.find('/') != std::string::npos || name == "." || name == "..")
    {
        return;
    }
    if (!name.empty())
    {
        if (name.size() > (SharedDirectory().empty() ? 30 : 128))
        {
            return;
        }
#ifdef SHARED_MEMORY_MOBILE
        if (SharedDirectory().empty())
        {
            return;
        }
#endif
        SharedDirectorySealed().store(true);
        if (!SharedDirectory().empty())
        {
            // 所有可能分配字符串的操作放在取得描述符之前，构造抛出时不能泄漏目录引用。
            m_nativeName = "m_" + name;
            m_directoryDescriptor = ::open(SharedDirectory().c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
            if (m_directoryDescriptor < 0)
            {
                m_nativeName.clear();
                return;
            }
        }
        else
        {
#ifdef SHARED_MEMORY_MOBILE
            return;
#else
            m_nativeName = "/" + name;
#endif
        }
    }
    if (size != 0 && name.empty())
    {
        m_capacity = size;
        return;
    }
#if !defined(__EMSCRIPTEN__)
    if (size != 0)
    {
        if (m_directoryDescriptor >= 0)
        {
            m_descriptor = ::openat(m_directoryDescriptor, m_nativeName.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        }
#ifndef SHARED_MEMORY_MOBILE
        else
        {
            m_descriptor = ::shm_open(m_nativeName.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
        }
#endif
        m_created = m_descriptor >= 0;
        if (m_created)
        {
            bool allocated = true;
#ifdef __linux__
            // Linux的shm_open同样由tmpfs承载，不能只为普通文件预留空间。
            const bool reserveStorage = true;
#else
            const bool reserveStorage = m_directoryDescriptor >= 0;
#endif
            if (reserveStorage)
            {
                // 先预留空间，避免ftruncate只扩逻辑长度后，首次写页因ENOSPC触发SIGBUS。
#ifdef __APPLE__
                fstore_t allocation = {};
                allocation.fst_flags = F_ALLOCATEALL;
                allocation.fst_posmode = F_PEOFPOSMODE;
                allocation.fst_length = static_cast<off_t>(size);
                allocated = ::fcntl(m_descriptor, F_PREALLOCATE, &allocation) == 0;
#else
                allocated = ::posix_fallocate(m_descriptor, 0, static_cast<off_t>(size)) == 0;
#endif
            }
            if (!allocated || ::ftruncate(m_descriptor, size) != 0)
            {
                ::close(m_descriptor);
                m_descriptor = -1;
                unlinkName();
                m_created = false;
            }
        }
        else if (isAllowExist && errno == EEXIST)
        {
            if (m_directoryDescriptor >= 0)
            {
                m_descriptor = ::openat(m_directoryDescriptor, m_nativeName.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
            }
#ifndef SHARED_MEMORY_MOBILE
            else
            {
                m_descriptor = ::shm_open(m_nativeName.c_str(), O_RDWR, 0600);
            }
#endif
        }
        if (m_descriptor >= 0)
        {
            struct stat info = {};
            // POSIX共享对象的文件类型位未定义；仅普通目录后备要求S_ISREG。
            if (::fstat(m_descriptor, &info) == 0 && (m_directoryDescriptor < 0 || S_ISREG(info.st_mode)) && info.st_size >= size &&
                static_cast<uint64_t>(info.st_size) <= UINT32_MAX)
            {
                m_capacity = static_cast<uint32_t>(info.st_size);
            }
            else
            {
                ::close(m_descriptor);
                m_descriptor = -1;
                if (m_created)
                {
                    unlinkName();
                    m_created = false;
                }
            }
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
    if (m_memoryHandle == nullptr)
    {
        m_memoryHandle = ::OpenFileMappingA(readOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS, FALSE, m_memoryName.c_str());
    }
#elif !defined(__EMSCRIPTEN__)
    if (m_descriptor < 0 && !m_nativeName.empty())
    {
        if (m_directoryDescriptor >= 0)
        {
            m_descriptor = ::openat(m_directoryDescriptor, m_nativeName.c_str(), (readOnly ? O_RDONLY : O_RDWR) | O_CLOEXEC | O_NOFOLLOW);
        }
#ifndef SHARED_MEMORY_MOBILE
        else
        {
            m_descriptor = ::shm_open(m_nativeName.c_str(), readOnly ? O_RDONLY : O_RDWR, 0600);
        }
#endif
        struct stat info = {};
        if (m_descriptor >= 0 && ::fstat(m_descriptor, &info) == 0 && (m_directoryDescriptor < 0 || S_ISREG(info.st_mode)) && info.st_size > 0 &&
            static_cast<uint64_t>(info.st_size) <= UINT32_MAX)
        {
            m_capacity = static_cast<uint32_t>(info.st_size);
        }
        else if (m_descriptor >= 0)
        {
            ::close(m_descriptor);
            m_descriptor = -1;
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
        view = ::MapViewOfFile(m_memoryHandle, FILE_MAP_READ, 0, 0, 0);
    }
    if (view == nullptr)
    {
        return 0;
    }
    MEMORY_BASIC_INFORMATION info = {};
    const SIZE_T queried = ::VirtualQuery(view, &info, sizeof(info));
    if (temporary)
    {
        ::UnmapViewOfFile(view);
    }
    return queried == sizeof(info) && info.RegionSize <= UINT32_MAX ? static_cast<uint32_t>(info.RegionSize) : 0;
#else
    // 查询使用读写打开，之后writeWithoutLock不会被一个只读描述符阻止。
    open(false);
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
        m_readMemoryPtr = ::MapViewOfFile(m_memoryHandle, FILE_MAP_READ, 0, 0, 0);
    }
    return m_memoryPtr = m_readMemoryPtr;
#else
    return writeWithoutLock();
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
        m_writeMemoryPtr = ::MapViewOfFile(m_memoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (m_writeMemoryPtr == nullptr && m_size == 0)
        {
            // 先取得替代引用，成功映射后才关闭旧句柄，避免关闭最后引用导致对象消失。
            HANDLE upgraded = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, m_memoryName.c_str());
            if (upgraded != nullptr)
            {
                void* view = ::MapViewOfFile(upgraded, FILE_MAP_ALL_ACCESS, 0, 0, 0);
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
    }
    return m_memoryPtr = m_writeMemoryPtr;
#else
    if (m_memoryPtr != nullptr)
    {
        return m_memoryPtr;
    }
    open(false);
    if (isFailed() || m_capacity == 0)
    {
        return nullptr;
    }
    const int flags = MAP_SHARED | (m_memoryName.empty() ? MAP_ANON : 0);
    void* view = ::mmap(nullptr, m_capacity, PROT_READ | PROT_WRITE, flags, m_descriptor, 0);
    if (view == MAP_FAILED)
    {
        return nullptr;
    }
    return m_memoryPtr = view;
#endif
}

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
    void* view = writeWithoutLock();
    if (view != nullptr && m_size != 0 && realSize() >= m_size)
    {
        std::memset(view, 0, m_size);
    }
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
        // 显式撤名后不再由析构重复删除，避免删除后来新建的同名对象。
        m_created = false;
        return true;
    }
    return false;
#endif
}

// 以下为保留的历史手工测试，当前验证见ProcessWork.cpp中的关闭测试入口。
//int32_t main()
//{
//	SharedMemory mem("mmsa", 4095);
//	int32_t size = mem.size();
//	{
//		WriteLock writelock(mem);
//		void* ptr = mem.memory();
//		memcpy(ptr, "121212\0", 7);
//	}
//	getchar();
//	return 0;
//}
//
////读
//int32_t main()
//{
//	SharedMemory mem("mmsa");
//	int32_t size = mem.size();
//	{
//		ReadLock readlock(mem);
//		void* ptr = mem.memory();
//		printf("%s\n", ptr);
//	}
//	getchar();
//	return 0;
//}