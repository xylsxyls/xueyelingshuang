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

#ifdef _WIN32
/** 打开命名映射，优先保留独立只读视图的能力
@param [in] name 已验证的映射名称
@param [in] readOnly 是否只请求读取现有数据
@return 调用者负责关闭的句柄，权限不足或对象不存在时返回空
*/
static HANDLE OpenMappingHandle(const std::string& name, bool readOnly)
{
    HANDLE handle = ::OpenFileMappingA(readOnly ? FILE_MAP_READ : FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name.c_str());
    if (handle == nullptr && ::GetLastError() == ERROR_ACCESS_DENIED)
    {
        // FILE_MAP_WRITE 本身允许读写映射，访问控制表可以只授予这一项权限。
        handle = ::OpenFileMappingA(FILE_MAP_WRITE, FALSE, name.c_str());
    }
    return handle;
}

/** 从现有句柄映射可读数据，优先取得只读视图
@param [in] handle 已持有的文件映射句柄
@return 须由调用者解除的视图，无法映射时返回空
*/
static void* MapReadableView(HANDLE handle)
{
    void* view = ::MapViewOfFile(handle, FILE_MAP_READ, 0, 0, 0);
    if (view == nullptr && ::GetLastError() == ERROR_ACCESS_DENIED)
    {
        view = ::MapViewOfFile(handle, FILE_MAP_WRITE, 0, 0, 0);
    }
    return view;
}

/** 累计同一视图中连续的区域，达到所需长度即结束。
@param [in] view 从偏移零开始映射的视图基址
@param [in] required 至少需要核对的字节数
@param [in] writableOnly 只累计已提交且可写的连续区域
@return 通过核对的区域总字节数，最后一个区域可以超过所需长度
*/
static uint64_t WindowsViewBytes(void* view, uint64_t required, bool writableOnly)
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
/** 持有宿主选定的目录对象，首次命名映射后只读
*/
struct SharedDirectoryConfig
{
public:
    // 用户传入的路径，供重复配置检查
    std::string m_path;
    // 对已验证目录的引用，负值表示使用系统默认后端
    int m_descriptor;

public:
    /** 默认使用系统后端
    */
    SharedDirectoryConfig() :
        m_descriptor(-1)
    {

    }

    /** 释放配置对目录持有的引用
    */
    ~SharedDirectoryConfig()
    {
        if (m_descriptor >= 0)
        {
            ::close(m_descriptor);
        }
    }

private:
    /** 目录引用由本进程的配置对象独占
    @param [in] other 其他配置对象
    */
    SharedDirectoryConfig(const SharedDirectoryConfig& other) = delete;

    /** 禁止用另一个配置覆盖已持有的目录引用
    @param [in] other 其他配置对象
    @return 当前配置的引用
    */
    SharedDirectoryConfig& operator=(const SharedDirectoryConfig& other) = delete;
};

/** 获取由宿主在启动期设置的共享目录。
@return 进程内唯一配置存储
*/
static SharedDirectoryConfig& SharedDirectory()
{
    static SharedDirectoryConfig s_directory;
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
    SharedDirectoryConfig& config = SharedDirectory();
    if (SharedDirectorySealed().load())
    {
        return directory == config.m_path;
    }
    if (directory.empty())
    {
#ifdef SHARED_MEMORY_MOBILE
        return false;
#else
        if (config.m_descriptor >= 0)
        {
            ::close(config.m_descriptor);
            config.m_descriptor = -1;
        }
        config.m_path.clear();
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
    if (config.m_descriptor >= 0)
    {
        ::close(config.m_descriptor);
    }
    config.m_path.swap(selectedPath);
    config.m_descriptor = descriptor;
    return true;
#endif
}

SharedMemory::SharedMemory(const std::string& name, uint32_t size, bool isAllowExist):
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
            m_memoryHandle = OpenMappingHandle(name, false);
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
        const int directory = SharedDirectory().m_descriptor;
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
        SharedDirectorySealed().store(true);
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
                m_descriptor = ::openat(m_directoryDescriptor, m_nativeName.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
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
        m_memoryHandle = OpenMappingHandle(m_memoryName, readOnly);
    }
#elif !defined(__EMSCRIPTEN__)
    if (m_descriptor < 0 && !m_nativeName.empty())
    {
        // 优先取得可读写描述符，允许同一实例在读后继续写；权限不足时读操作可降为只读。
        for (int attempt = 0; attempt < (readOnly ? 2 : 1); ++attempt)
        {
            const int access = attempt == 0 ? O_RDWR : O_RDONLY;
            if (m_directoryDescriptor >= 0)
            {
                // 在核实是普通文件前，不等待其他类型的目录项就绪。
                m_descriptor = ::openat(m_directoryDescriptor, m_nativeName.c_str(), access | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
            }
#ifndef SHARED_MEMORY_MOBILE
            else
            {
                m_descriptor = ::shm_open(m_nativeName.c_str(), access, 0600);
            }
#endif
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
        view = MapReadableView(m_memoryHandle);
    }
    if (view == nullptr)
    {
        return 0;
    }
    const uint64_t capacity = WindowsViewBytes(view, static_cast<uint64_t>(UINT32_MAX) + 1, false);
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
        m_readMemoryPtr = MapReadableView(m_memoryHandle);
        if (m_readMemoryPtr == nullptr)
        {
            return nullptr;
        }
    }
    return m_memoryPtr = m_readMemoryPtr;
#else
    open(true);
    if (!m_readOnly)
    {
        return writeWithoutLock();
    }
    if (m_memoryPtr == nullptr && m_descriptor >= 0 && m_capacity != 0)
    {
        void* view = ::mmap(nullptr, m_capacity, PROT_READ, MAP_SHARED, m_descriptor, 0);
        if (view != MAP_FAILED)
        {
            m_memoryPtr = view;
        }
    }
    return m_memoryPtr;
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
            HANDLE upgraded = OpenMappingHandle(m_memoryName, false);
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
    if (m_readOnly)
    {
        return nullptr;
    }
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
    if (WindowsViewBytes(view, m_size, true) < m_size)
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
// #include <cstring>
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
// #endif
//     std::ostringstream key;
//     key << "smd" << std::hex << std::chrono::steady_clock::now().time_since_epoch().count();
//     SharedMemory owner(key.str(), 4096, false);
//     if (owner.isFailed() || owner.realSize() < owner.size())
//     {
//         return 2;
//     }
//     void* writable = owner.writeWithoutLock();
//     if (writable == nullptr)
//     {
//         return 3;
//     }
//     std::memcpy(writable, "ready", 6);
//     SharedMemory peer(owner.mapName());
//     const void* readable = peer.readWithoutLock();
//     if (readable == nullptr || readable != peer.memory() || std::memcmp(readable, "ready", 6) != 0)
//     {
//         return 4;
//     }
//     owner.clear();
//     const bool cleared = static_cast<const char*>(readable)[0] == 0;
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