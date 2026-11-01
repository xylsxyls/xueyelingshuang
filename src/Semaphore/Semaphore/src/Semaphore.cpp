#include "Semaphore.h"
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IPHONE) || defined(PROCESS_WORK_PORTABLE_NOTIFY)
#define SEMAPHORE_SHARED_COUNTER 1
#endif
#include <chrono>
#include <algorithm>
#include <limits.h>
#include <stdexcept>
#ifdef _WIN32
#include <Windows.h>
#else
#include <semaphore.h>
#include <fcntl.h>
#include <cerrno>
#include <time.h>
#ifdef SEMAPHORE_SHARED_COUNTER
#include <atomic>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#endif

// 私有后备仅使用系统接口。移动端共享文件为两个对齐32位字段：计数、上限。
struct Semaphore::ProcessSemaphoreMemory
{
#ifdef SEMAPHORE_SHARED_COUNTER
    int32_t* words;
#ifdef _WIN32
    HANDLE handle;
    ProcessSemaphoreMemory(): words(nullptr), handle(nullptr) {}
    ~ProcessSemaphoreMemory()
    {
        if (words != nullptr) ::UnmapViewOfFile(words);
        if (handle != nullptr) ::CloseHandle(handle);
    }
#else
    int directoryDescriptor;
    int descriptor;
    bool created;
    std::string name;
    ProcessSemaphoreMemory(): words(nullptr), directoryDescriptor(-1), descriptor(-1), created(false) {}
    ~ProcessSemaphoreMemory()
    {
        if (created) unlinkName();
        if (words != nullptr) ::munmap(words, 2 * sizeof(int32_t));
        if (descriptor >= 0) ::close(descriptor);
        if (directoryDescriptor >= 0) ::close(directoryDescriptor);
    }

    static std::string& directory()
    {
        static std::string value;
        return value;
    }

    static std::atomic<bool>& directorySealed()
    {
        static std::atomic<bool> value(false);
        return value;
    }

    static int openDirectory(const std::string& path)
    {
        if (path.empty() || path[0] != '/' || path.find('\0') != std::string::npos) return -1;
        const int value = ::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
        if (value < 0) return -1;
        struct stat info = {};
        if (::fstat(value, &info) != 0 || !S_ISDIR(info.st_mode) || info.st_uid != ::geteuid())
        {
            ::close(value);
            return -1;
        }
        return value;
    }
#endif

    static int32_t load(int32_t* value)
    {
#ifdef _WIN32
        return static_cast<int32_t>(::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(value), 0, 0));
#else
        // 必须使用真正无锁的32位指令，不能回退到仅进程内有效的libatomic锁。
        static_assert(__atomic_always_lock_free(sizeof(int32_t), 0), "Semaphore requires lock-free 32-bit atomics");
        return __atomic_load_n(value, __ATOMIC_SEQ_CST);
#endif
    }

    static bool compareExchange(int32_t* value, int32_t expected, int32_t desired)
    {
#ifdef _WIN32
        return ::InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(value), desired, expected) == expected;
#else
        return __atomic_compare_exchange_n(value, &expected, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#endif
    }

    bool open(const std::string& logicalName, int32_t limit)
    {
        if (logicalName.size() > 30) return false;
#ifdef _WIN32
        // 桌面测试开关仅用系统匿名页映射验证移动端计数算法；不代替移动端文件/沙箱验收。
        const std::string nativeName = logicalName + ".s";
        handle = limit > 0 ? ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
            2 * sizeof(int32_t), nativeName.c_str()) : ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, nativeName.c_str());
        if (handle == nullptr) return false;
        words = static_cast<int32_t*>(::MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 2 * sizeof(int32_t)));
#else
        if (directory().empty()) return false;
        // 保留原有通知文件名及两个int32_t布局，后备的管理不再经由其他库。
        name = "m_" + logicalName + ".s";
        directorySealed().store(true);
        directoryDescriptor = openDirectory(directory());
        if (directoryDescriptor < 0) return false;
        if (limit > 0)
        {
            descriptor = ::openat(directoryDescriptor, name.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
            created = descriptor >= 0;
            if (descriptor < 0 && errno == EEXIST)
                descriptor = ::openat(directoryDescriptor, name.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
        }
        else
        {
            descriptor = ::openat(directoryDescriptor, name.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
        }
        if (descriptor < 0) return false;
        const size_t bytes = 2 * sizeof(int32_t);
        if (created)
        {
            // 发布前预留实际存储，分配失败不留下可打开的半成品。
#ifdef __APPLE__
            fstore_t allocation = {};
            allocation.fst_flags = F_ALLOCATEALL;
            allocation.fst_posmode = F_PEOFPOSMODE;
            allocation.fst_length = static_cast<off_t>(bytes);
            if (::fcntl(descriptor, F_PREALLOCATE, &allocation) != 0) return false;
#else
            if (::posix_fallocate(descriptor, 0, static_cast<off_t>(bytes)) != 0) return false;
#endif
            if (::ftruncate(descriptor, static_cast<off_t>(bytes)) != 0) return false;
        }
        struct stat info = {};
        if (::fstat(descriptor, &info) != 0 || !S_ISREG(info.st_mode) || info.st_uid != ::geteuid() ||
            info.st_size != static_cast<off_t>(bytes)) return false;
        void* address = ::mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
        if (address == MAP_FAILED) return false;
        words = static_cast<int32_t*>(address);
#endif
        if (words == nullptr) return false;
        // CAS发布上限，重复create不清空计数，也不覆盖已有对象的上限。
        if (limit > 0) compareExchange(words + 1, 0, limit);
        const int32_t currentLimit = load(words + 1);
        const int32_t count = load(words);
        return currentLimit > 0 && count >= 0 && count <= currentLimit;
    }

    bool unlinkName()
    {
#ifdef _WIN32
        return false;
#else
        if (directoryDescriptor < 0 || name.empty()) return false;
        if (::unlinkat(directoryDescriptor, name.c_str(), 0) == 0 || errno == ENOENT)
        {
            created = false;
            return true;
        }
        return false;
#endif
    }
#endif
};

bool Semaphore::setProcessDirectory(const std::string& directory)
{
#if defined(SEMAPHORE_SHARED_COUNTER) && !defined(_WIN32)
    if (ProcessSemaphoreMemory::directorySealed().load())
        return directory == ProcessSemaphoreMemory::directory();
    const int descriptor = ProcessSemaphoreMemory::openDirectory(directory);
    if (descriptor < 0) return false;
    ::close(descriptor);
    ProcessSemaphoreMemory::directory() = directory;
    return true;
#else
    return directory.empty();
#endif
}

Semaphore::Semaphore(): m_count(0), m_processSemaphore(nullptr), m_processCreated(false)
{
}

Semaphore::~Semaphore()
{
    closeProcessSemaphore();
}

void Semaphore::reset()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_count = 0;
}

void Semaphore::signal()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    if (m_count == UINT64_MAX)
    {
        // 拒绝溢出，不回绕或悄悄吞掉一次通知；异常退出仍由原有RAII释放锁。
        throw std::overflow_error("Semaphore local notification count exhausted");
    }
    ++m_count;
    m_cv.notify_one();
}

void Semaphore::wait()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    while (m_count == 0)
    {
        m_cv.wait(lock);
    }
    --m_count;
}

bool Semaphore::wait(uint32_t timeout)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    // 固定截止时间；虚假唤醒不能让总等待时间不断延长。
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
    while (m_count == 0)
    {
        if (m_cv.wait_until(lock, deadline) == std::cv_status::timeout && m_count == 0)
        {
            return false;
        }
    }
    --m_count;
    return true;
}

void Semaphore::createProcessSemaphore(const std::string& name, int32_t signalCount)
{
    if (m_processSemaphore != nullptr || name.empty() || name.find('\0') != std::string::npos ||
        name.find('/') != std::string::npos || signalCount <= 0)
    {
        return;
    }
#ifdef SEMAPHORE_SHARED_COUNTER
    m_processMemory.reset(new ProcessSemaphoreMemory);
    if (m_processMemory->open(name, signalCount)) m_processSemaphore = m_processMemory->words;
    else m_processMemory.reset();
#elif defined(_WIN32)
    m_processSemaphore = ::CreateSemaphoreA(nullptr, 0, signalCount, name.c_str());
#elif !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    m_processName = "/" + name;
    sem_t* value = ::sem_open(m_processName.c_str(), O_CREAT | O_EXCL, 0600, 0);
    m_processCreated = value != SEM_FAILED;
    if (value == SEM_FAILED && errno == EEXIST)
    {
        value = ::sem_open(m_processName.c_str(), 0);
    }
    if (value != SEM_FAILED)
    {
        m_processSemaphore = value;
    }
#endif
}

void Semaphore::openProcessSemaphore(const std::string& name)
{
    tryOpenProcessSemaphore(name);
}

bool Semaphore::tryOpenProcessSemaphore(const std::string& name)
{
    if (m_processSemaphore != nullptr)
    {
        return true;
    }
    if (name.empty() || name.find('\0') != std::string::npos || name.find('/') != std::string::npos)
    {
        return false;
    }
#ifdef SEMAPHORE_SHARED_COUNTER
    m_processMemory.reset(new ProcessSemaphoreMemory);
    if (m_processMemory->open(name, 0)) m_processSemaphore = m_processMemory->words;
    else m_processMemory.reset();
#elif defined(_WIN32)
    m_processSemaphore = ::OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, name.c_str());
#elif !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    m_processName = "/" + name;
    sem_t* value = ::sem_open(m_processName.c_str(), 0);
    if (value != SEM_FAILED)
    {
        m_processSemaphore = value;
    }
#endif
    return m_processSemaphore != nullptr;
}

int32_t Semaphore::tryProcessSignal()
{
    if (m_processSemaphore == nullptr)
    {
        return -1;
    }
#ifdef SEMAPHORE_SHARED_COUNTER
    int32_t* words = static_cast<int32_t*>(m_processSemaphore);
    const int32_t limit = ProcessSemaphoreMemory::load(words + 1);
    int32_t count = ProcessSemaphoreMemory::load(words);
    while (count >= 0 && count < limit)
    {
        if (ProcessSemaphoreMemory::compareExchange(words, count, count + 1))
        {
            return 0;
        }
        count = ProcessSemaphoreMemory::load(words);
    }
    return count == limit ? 1 : -1;
#elif defined(_WIN32)
    if (::ReleaseSemaphore(m_processSemaphore, 1, nullptr))
    {
        return 0;
    }
    return ::GetLastError() == ERROR_TOO_MANY_POSTS ? 1 : -1;
#elif defined(__EMSCRIPTEN__)
    // Wasm不提供跨进程命名信号量，不引入仅原生平台存在的符号。
    return -1;
#else
    if (::sem_post(static_cast<sem_t*>(m_processSemaphore)) == 0)
    {
        return 0;
    }
    return errno == EOVERFLOW ? 1 : -1;
#endif
}

void Semaphore::processSignal()
{
    tryProcessSignal();
}

void Semaphore::closeProcessSemaphore()
{
    if (m_processSemaphore == nullptr)
    {
        return;
    }
#ifdef SEMAPHORE_SHARED_COUNTER
    m_processMemory.reset();
#elif defined(_WIN32)
    ::CloseHandle(m_processSemaphore);
#elif !defined(__EMSCRIPTEN__)
    ::sem_close(static_cast<sem_t*>(m_processSemaphore));
#if !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    if (m_processCreated)
    {
        ::sem_unlink(m_processName.c_str());
    }
#endif
#endif
    m_processSemaphore = nullptr;
    m_processCreated = false;
    m_processName.clear();
}

void Semaphore::processWait()
{
    processWait(-1);
}

bool Semaphore::unlinkProcessName()
{
#ifdef SEMAPHORE_SHARED_COUNTER
    return m_processMemory && m_processMemory->unlinkName();
#elif defined(_WIN32) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
    return false;
#else
    if (m_processName.empty())
    {
        return false;
    }
    if (::sem_unlink(m_processName.c_str()) == 0 || errno == ENOENT)
    {
        m_processCreated = false;
        return true;
    }
    return false;
#endif
}

bool Semaphore::processWait(int32_t timeout)
{
    if (m_processSemaphore == nullptr || timeout < -1)
    {
        return false;
    }
#ifdef SEMAPHORE_SHARED_COUNTER
    int32_t* count = static_cast<int32_t*>(m_processSemaphore);
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout < 0 ? 0 : timeout);
    for (;;)
    {
        int32_t current = ProcessSemaphoreMemory::load(count);
        while (current > 0)
        {
            if (ProcessSemaphoreMemory::compareExchange(count, current, current - 1))
            {
                return true;
            }
            current = ProcessSemaphoreMemory::load(count);
        }
        if (current < 0 || (timeout >= 0 && std::chrono::steady_clock::now() >= deadline))
        {
            return false;
        }
        // 通知只作提示；相对睡眠兼容Android/iOS，不依赖跨进程pthread锁或墙上时钟。
#ifdef _WIN32
        ::Sleep(1);
#else
        struct timespec pause = {};
        pause.tv_nsec = 1000000;
        ::nanosleep(&pause, nullptr);
#endif
    }
#elif defined(_WIN32)
    return ::WaitForSingleObject(m_processSemaphore, timeout < 0 ? INFINITE : static_cast<DWORD>(timeout)) == WAIT_OBJECT_0;
#elif defined(__EMSCRIPTEN__)
    return false;
#else
    sem_t* value = static_cast<sem_t*>(m_processSemaphore);
    if (timeout < 0)
    {
        int result;
        do { result = ::sem_wait(value); } while (result != 0 && errno == EINTR);
        return result == 0;
    }
    if (::sem_trywait(value) == 0)
    {
        return true;
    }
    if (timeout == 0 || (errno != EAGAIN && errno != EINTR))
    {
        return false;
    }
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
    while (std::chrono::steady_clock::now() < deadline)
    {
        // 有限等待使用相对睡眠，避免sem_timedwait的墙上时钟回拨延长退出时间。
        // Darwin同样支持此路径；无限等待仍使用原生sem_wait。
        const int64_t remaining = std::chrono::duration_cast<std::chrono::nanoseconds>(deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0)
        {
            break;
        }
        struct timespec pause = {};
        pause.tv_nsec = static_cast<long>((std::min)(remaining, static_cast<int64_t>(2000000)));
        ::nanosleep(&pause, nullptr);
        if (::sem_trywait(value) == 0)
        {
            return true;
        }
        if (errno != EAGAIN && errno != EINTR)
        {
            return false;
        }
    }
    return false;
#endif
}

void Semaphore::event()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_cv.notify_one();
}

void Semaphore::eventAll()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_cv.notify_all();
}

void Semaphore::eventWait()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_cv.wait(lock);
}

bool Semaphore::eventWait(int32_t timeout)
{
    std::unique_lock<std::mutex> lock(m_mtx);
    if (m_cv.wait_for(lock, std::chrono::milliseconds(timeout)) == std::cv_status::timeout)
    {
        return false;
    }
    return true;
}

// 以下历史文件后端测试保留为注释；新原生后端测试随ProcessWork.cpp主入口统一执行。
//#include <chrono>
//#include <cstdio>
//#include <cstdlib>
//#include <fstream>
//#include <iterator>
//#include <limits.h>
//#include <stdexcept>
//#include <string>
//#ifdef _MSC_VER
//#include <windows.h>
//#else
//#include <cerrno>
//#include <csignal>
//#include <sys/wait.h>
//#include <unistd.h>
//#if defined(__ANDROID__) || defined(__APPLE__)
//#include <sys/stat.h>
//#endif
//#ifdef __APPLE__
//#include <TargetConditionals.h>
//#endif
//#endif
//
//int main(int argc, char* argv[])
//{
//    /** 判定一条独立预期并累计实际检查数
//    @param [in] condition 实际条件
//    @param [in] message 检查说明
//    @param [in,out] checks 本项已执行的检查数
//    */
//    auto check = [](bool condition, const char* message, int32_t& checks) -> void
//    {
//        ++checks;
//        if (!condition)
//        {
//            throw std::runtime_error(message);
//        }
//    };
//
//    /** 在独立单线程测试进程中设置启动配置，产品运行时不得这样动态变更环境
//    @param [in] scope Android/Apple共享目录，Windows/Linux库应忽略此环境变量
//    */
//    auto configureScope = [](const std::string& scope) -> void
//    {
//    #ifdef _MSC_VER
//        const int32_t result = ::_putenv_s("SEMAPHORE_PROCESS_SCOPE", scope.c_str());
//    #else
//        const int32_t result = ::setenv("SEMAPHORE_PROCESS_SCOPE", scope.c_str(), 1);
//    #endif
//        if (result != 0)
//        {
//            throw std::runtime_error("Failed to set owned test process environment");
//        }
//    };
//
//    /** 生成测试专属的短名称，避免不同进程和用例共享状态
//    @param [in] id 稳定用例ID
//    @return 含当前PID的ASCII逻辑名称
//    */
//    auto testName = [](int32_t id) -> std::string
//    {
//        char name[64];
//    #ifdef _MSC_VER
//        const uint32_t pid = ::GetCurrentProcessId();
//        ::sprintf_s(name, sizeof(name), "SP%u_%d", pid, id);
//    #else
//        const uint32_t pid = static_cast<uint32_t>(::getpid());
//        std::snprintf(name, sizeof(name), "SP%u_%d", pid, id);
//    #endif
//        return name;
//    };
//
//    /** 在独立对端进程打开两个方向的对象并完成一次握手
//    @param [in] name 父进程创建的对象前缀
//    @param [in] scope 通信范围
//    @param [in] infinite true检查无参数无限等待，false检查有限等待
//    @return 成功0，异常或超时1
//    */
//    auto runPeer = [&configureScope](const std::string& name, const std::string& scope, bool infinite) -> int32_t
//    {
//        try
//        {
//            Semaphore inbound;
//            Semaphore outbound;
//            configureScope(scope);
//            inbound.openProcessSemaphore(name + "a");
//            outbound.openProcessSemaphore(name + "b");
//            outbound.processSignal();
//            if (infinite)
//            {
//                inbound.processWait();
//            }
//            else if (!inbound.processWait(2000))
//            {
//                return 1;
//            }
//            outbound.processSignal();
//            return 0;
//        }
//        catch (const std::exception& error)
//        {
//            std::fprintf(stderr, "Peer failed: %s\n", error.what());
//            return 1;
//        }
//    };
//
//    /** 启动真实对端，检查双向通知及有限时间内的进程退出
//    @param [in] name 本项唯一名称前缀
//    @param [in] scope 通信范围
//    @param [in] infinite 是否检查无限等待；父进程看门狗仍有上限
//    @param [in,out] checks 实际检查数
//    @return 可在当前宿主执行时true，iOS CLI不能启动任意对端时false
//    */
//    auto testTwoProcesses = [&check, &configureScope, &runPeer](const std::string& name, const std::string& scope, int32_t& checks, bool infinite) -> bool
//    {
//    #if defined(__APPLE__) && TARGET_OS_IPHONE
//        (void)name;
//        (void)scope;
//        (void)checks;
//        (void)infinite;
//        return false;
//    #else
//        Semaphore outbound;
//        Semaphore inbound;
//        configureScope(scope);
//        outbound.createProcessSemaphore(name + "a");
//        inbound.createProcessSemaphore(name + "b");
//    #ifdef _MSC_VER
//        wchar_t executable[32768];
//        const DWORD length = ::GetModuleFileNameW(nullptr, executable, 32768);
//        check(length != 0 && length < 32768, "Resolve test executable", checks);
//        std::wstring command = L"\"" + std::wstring(executable, length) +
//            (infinite ? L"\" --peer-infinite " : L"\" --peer ");
//        command.append(name.begin(), name.end());
//        STARTUPINFOW startup = {};
//        startup.cb = sizeof(startup);
//        PROCESS_INFORMATION process = {};
//        check(::CreateProcessW(nullptr, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
//            nullptr, nullptr, &startup, &process) != FALSE, "Create peer process", checks);
//        ::CloseHandle(process.hThread);
//        bool response = false;
//        try
//        {
//            check(inbound.processWait(2500), "Peer reports ready before notification", checks);
//            check(!inbound.processWait(20), "Peer cannot acknowledge before receiving notification", checks);
//            outbound.processSignal();
//            response = inbound.processWait(2500);
//        }
//        catch (...)
//        {
//            ::TerminateProcess(process.hProcess, 1);
//            ::WaitForSingleObject(process.hProcess, 3000);
//            ::CloseHandle(process.hProcess);
//            throw;
//        }
//        const DWORD exited = ::WaitForSingleObject(process.hProcess, 3000);
//        DWORD exitCode = 1;
//        if (exited == WAIT_OBJECT_0)
//        {
//            ::GetExitCodeProcess(process.hProcess, &exitCode);
//        }
//        else
//        {
//            ::TerminateProcess(process.hProcess, 1);
//            ::WaitForSingleObject(process.hProcess, 3000);
//        }
//        ::CloseHandle(process.hProcess);
//        check(response, "Peer must acknowledge a different-direction semaphore", checks);
//        check(exited == WAIT_OBJECT_0 && exitCode == 0, "Peer must exit successfully", checks);
//    #else
//        const pid_t child = ::fork();
//        check(child >= 0, "Fork peer process", checks);
//        if (child == 0)
//        {
//            ::_exit(runPeer(name, scope, infinite));
//        }
//        bool response = false;
//        try
//        {
//            check(inbound.processWait(2500), "Peer reports ready before notification", checks);
//            check(!inbound.processWait(20), "Peer cannot acknowledge before receiving notification", checks);
//            outbound.processSignal();
//            response = inbound.processWait(2500);
//        }
//        catch (...)
//        {
//            ::kill(child, SIGKILL);
//            while (::waitpid(child, nullptr, 0) < 0 && errno == EINTR)
//            {
//
//            }
//            throw;
//        }
//        int32_t status = 0;
//        pid_t result = 0;
//        const std::chrono::steady_clock::time_point deadline =
//            std::chrono::steady_clock::now() + std::chrono::seconds(3);
//        do
//        {
//            result = ::waitpid(child, &status, WNOHANG);
//            if (result == 0)
//            {
//                ::usleep(1000);
//            }
//        }
//        while ((result == 0 || (result < 0 && errno == EINTR)) &&
//            std::chrono::steady_clock::now() < deadline);
//        if (result != child && !(result < 0 && errno == ECHILD))
//        {
//            ::kill(child, SIGKILL);
//            while (::waitpid(child, nullptr, 0) < 0 && errno == EINTR)
//            {
//
//            }
//        }
//        check(response, "Peer must acknowledge a different-direction semaphore", checks);
//        check(result == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
//            "Peer must exit successfully", checks);
//    #endif
//        return true;
//    #endif
//    };
//
//    /** 创建一个通知后直接退出，不执行Semaphore析构
//    @param [in] name 本测试拥有的逻辑名称
//    @param [in] scope 启动配置
//    @return 不返回；成功退出0，创建未生效退出1
//    */
//    auto runAbandonedPeer = [&configureScope](const std::string& name, const std::string& scope) -> void
//    {
//        configureScope(scope);
//        Semaphore owner;
//        owner.createProcessSemaphore(name);
//        owner.processSignal();
//        const bool ready = owner.processWait(0);
//        owner.processSignal();
//    #ifdef _MSC_VER
//        ::ExitProcess(ready ? 0 : 1);
//    #else
//        ::_exit(ready ? 0 : 1);
//    #endif
//    };
//
//    /** 验证进程退出释放存活租约，残留通知不能污染重新创建的对象
//    @param [in] name 本测试专属名称
//    @param [in] scope 同一共享目录
//    @param [in,out] checks 实际检查数
//    @return true执行，false为旧Linux语义或iOS CLI不适用
//    */
//    auto testAbandonedProcess = [&check, &runAbandonedPeer](const std::string& name, const std::string& scope, int32_t& checks) -> bool
//    {
//    #if (defined(__APPLE__) && TARGET_OS_IPHONE) || (defined(__linux__) && !defined(__ANDROID__))
//        (void)name;
//        (void)scope;
//        (void)checks;
//        return false;
//    #else
//    #ifdef _MSC_VER
//        (void)scope;
//        wchar_t executable[32768];
//        const DWORD length = ::GetModuleFileNameW(nullptr, executable, 32768);
//        check(length != 0 && length < 32768, "Resolve abandonment test executable", checks);
//        std::wstring command = L"\"" + std::wstring(executable, length) + L"\" --abandon ";
//        command.append(name.begin(), name.end());
//        STARTUPINFOW startup = {};
//        startup.cb = sizeof(startup);
//        PROCESS_INFORMATION process = {};
//        check(::CreateProcessW(nullptr, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
//            nullptr, nullptr, &startup, &process) != FALSE, "Start owned abandoning process", checks);
//        ::CloseHandle(process.hThread);
//        const DWORD result = ::WaitForSingleObject(process.hProcess, 3000);
//        DWORD exitCode = 1;
//        if (result == WAIT_OBJECT_0)
//        {
//            ::GetExitCodeProcess(process.hProcess, &exitCode);
//        }
//        else
//        {
//            ::TerminateProcess(process.hProcess, 1);
//            ::WaitForSingleObject(process.hProcess, 3000);
//        }
//        ::CloseHandle(process.hProcess);
//        check(result == WAIT_OBJECT_0 && exitCode == 0, "Peer exited without library destruction", checks);
//    #else
//        const pid_t child = ::fork();
//        check(child >= 0, "Fork abandoning process", checks);
//        if (child == 0)
//        {
//            runAbandonedPeer(name, scope);
//        }
//        int32_t status = 0;
//        pid_t result = 0;
//        const std::chrono::steady_clock::time_point deadline =
//            std::chrono::steady_clock::now() + std::chrono::seconds(3);
//        do
//        {
//            result = ::waitpid(child, &status, WNOHANG);
//            if (result == 0)
//            {
//                ::usleep(1000);
//            }
//        }
//        while ((result == 0 || (result < 0 && errno == EINTR)) &&
//            std::chrono::steady_clock::now() < deadline);
//        if (result != child && !(result < 0 && errno == ECHILD))
//        {
//            ::kill(child, SIGKILL);
//            while (::waitpid(child, nullptr, 0) < 0 && errno == EINTR)
//            {
//
//            }
//        }
//        check(result == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
//            "Peer exited without library destruction", checks);
//    #endif
//        Semaphore owner;
//        owner.openProcessSemaphore(name);
//        owner.processSignal();
//        check(!owner.processWait(0), "Opening abandoned object must not resurrect old notification", checks);
//        owner.createProcessSemaphore(name);
//        check(!owner.processWait(0), "Recreated object must start empty", checks);
//        owner.processSignal();
//        check(owner.processWait(0), "Recreated object accepts new notifications", checks);
//        return true;
//    #endif
//    };
//
//    /** 执行一个稳定ID的瞬时用例，不支持的目标明确跳过
//    @param [in] id 稳定ID
//    @param [in] scope 显式测试范围
//    @param [in,out] checks 实际检查数
//    @return 执行时true，不适用时false
//    */
//    auto runCase = [&check, &configureScope, &testName, &testTwoProcesses, &testAbandonedProcess](int32_t id, const std::string& scope, int32_t& checks) -> bool
//    {
//        Semaphore owner;
//        if (id == 14 || id == 15 || id == 17)
//        {
//            return false;
//        }
//        if (id <= 3)
//        {
//            if (id == 1)
//            {
//                owner.signal();
//                owner.signal();
//                check(owner.wait(0), "Consume first notification", checks);
//                check(owner.wait(0), "Consume second notification", checks);
//                check(!owner.wait(0), "No extra notification", checks);
//            }
//            else if (id == 2)
//            {
//                const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
//                check(!owner.wait(20), "Unsignaled thread wait times out", checks);
//                const int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
//                    std::chrono::steady_clock::now() - start).count();
//                check(elapsed >= 10 && elapsed < 1000, "Thread timeout duration", checks);
//                owner.signal();
//                check(owner.wait(0), "Thread wait remains reusable", checks);
//            }
//            else
//            {
//                owner.event();
//                owner.eventAll();
//                check(!owner.eventWait(0), "Events do not accumulate without a waiter", checks);
//            }
//            return true;
//        }
//        if (id == 4)
//        {
//            owner.processSignal();
//            owner.processWait();
//            check(!owner.processWait(0), "Uninitialized process wait fails", checks);
//            owner.closeProcessSemaphore();
//            owner.closeProcessSemaphore();
//            check(!owner.processWait(0), "Repeated close is harmless", checks);
//            return true;
//        }
//    #if !defined(__ANDROID__) && !defined(__APPLE__)
//        if ((id >= 7 && id <= 9) || id == 12 || id == 13 || id >= 22)
//        {
//            return false;
//        }
//    #endif
//    #if defined(__linux__) && !defined(__ANDROID__)
//        if (id == 11 || id == 18)
//        {
//            return false;
//        }
//    #endif
//    #if defined(__ANDROID__) || defined(__APPLE__)
//        if ((id == 12 || id == 13 || id == 22) && scope.empty())
//        {
//            return false;
//        }
//    #endif
//        configureScope(scope);
//        const std::string name = testName(id);
//        if (id == 10 || id == 21)
//        {
//            return testTwoProcesses(name, scope, checks, id == 21);
//        }
//        if (id == 20)
//        {
//            return testAbandonedProcess(name, scope, checks);
//        }
//        if (id == 5)
//        {
//            owner.createProcessSemaphore(name);
//            Semaphore peer;
//            peer.openProcessSemaphore(name);
//            owner.processSignal();
//            owner.processSignal();
//            owner.processSignal();
//            check(peer.processWait(0) && peer.processWait(0) && peer.processWait(0),
//                "Peer consumes three notifications", checks);
//            check(!peer.processWait(0), "Peer cannot consume a fourth", checks);
//            peer.closeProcessSemaphore();
//            Semaphore next;
//            next.openProcessSemaphore(name);
//            owner.processSignal();
//            check(next.processWait(0), "Closing peer does not delete owner resource", checks);
//        }
//        else if (id == 6)
//        {
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            owner.createProcessSemaphore(name);
//            owner.openProcessSemaphore(name);
//            check(owner.processWait(0), "Repeated create/open preserves count", checks);
//            check(!owner.processWait(0), "Repeated calls do not add notifications", checks);
//            owner.closeProcessSemaphore();
//            owner.closeProcessSemaphore();
//            owner.createProcessSemaphore(name);
//            check(!owner.processWait(0), "Recreated object starts empty", checks);
//        }
//    #if defined(__ANDROID__) || defined(__APPLE__)
//        else if (id == 7)
//        {
//            owner.openProcessSemaphore(name);
//            owner.processSignal();
//            check(!owner.processWait(0), "Opening missing object stays closed, without exception", checks);
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            check(owner.processWait(0), "Retry after failed open", checks);
//        }
//        else if (id == 8)
//        {
//            const std::string invalid[] = { "", ".", "..", "../outside", "a/b", "a\\b", std::string("a\0b", 3) };
//            for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index)
//            {
//                owner.createProcessSemaphore(invalid[index]);
//                owner.processSignal();
//                check(!owner.processWait(0), "Reject invalid logical name without exception", checks);
//            }
//            configureScope("relative");
//            owner.createProcessSemaphore(name);
//            check(!owner.processWait(0), "Reject relative directory", checks);
//            configureScope(scope);
//            owner.createProcessSemaphore(name);
//            configureScope("invalid/group");
//            owner.createProcessSemaphore(name);
//            owner.openProcessSemaphore(name);
//            owner.processSignal();
//            check(owner.processWait(0), "Live resource uses captured scope instead of environment", checks);
//            configureScope(scope);
//        }
//        else if (id == 9)
//        {
//            owner.createProcessSemaphore(name);
//            check(!owner.processWait(0), "Zero timeout never blocks", checks);
//            const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
//            check(!owner.processWait(20), "Process timeout reports false", checks);
//            const int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
//                std::chrono::steady_clock::now() - start).count();
//            check(elapsed >= 10 && elapsed < 1000, "Process timeout duration", checks);
//            owner.processSignal();
//            check(!owner.processWait(-2), "Unsupported negative timeout fails without consuming", checks);
//            check(owner.processWait(-1), "Negative-one wait consumes preserved notification", checks);
//        }
//    #endif
//        else if (id == 11)
//        {
//            owner.createProcessSemaphore(name);
//            Semaphore peer;
//            peer.openProcessSemaphore(name);
//            owner.closeProcessSemaphore();
//            peer.processSignal();
//            check(peer.processWait(0), "Opened peer survives creator close", checks);
//            check(!peer.processWait(0), "Peer count remains correct", checks);
//        }
//    #if defined(__ANDROID__) || defined(__APPLE__)
//        else if (id == 12 || id == 13)
//        {
//            const std::string file = scope + "/Semaphore." + name;
//            const std::string target = scope + "/" + name + "_target";
//            const std::string fixture = id == 12 ? file : target;
//            {
//                std::ofstream output(fixture.c_str(), std::ios::binary | std::ios::trunc);
//                output << "broken";
//                output.close();
//                check(!output.fail(), "Write owned fault fixture", checks);
//                check(::chmod(fixture.c_str(), 0600) == 0, "Restrict owned fault fixture permissions", checks);
//            }
//            if (id == 13)
//            {
//                check(::symlink(target.c_str(), file.c_str()) == 0, "Create owned symlink", checks);
//            }
//            try
//            {
//                owner.createProcessSemaphore(name);
//                owner.processSignal();
//                check(!owner.processWait(0), "Create rejects incompatible file or symlink", checks);
//                owner.openProcessSemaphore(name);
//                check(!owner.processWait(0), "Open rejects incompatible file or symlink", checks);
//                std::ifstream input(fixture.c_str(), std::ios::binary);
//                std::string actual;
//                std::getline(input, actual);
//                input.close();
//                check(actual == "broken", "Failures must not overwrite fixture or link target", checks);
//            }
//            catch (...)
//            {
//                owner.closeProcessSemaphore();
//                ::unlink(file.c_str());
//                if (id == 13)
//                {
//                    ::unlink(target.c_str());
//                }
//                throw;
//            }
//            ::unlink(file.c_str());
//            if (id == 13)
//            {
//                ::unlink(target.c_str());
//            }
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            check(owner.processWait(0), "Retry after removing owned fault fixture", checks);
//        }
//    #endif
//        else if (id == 16)
//        {
//    #if defined(__ANDROID__) || defined(__APPLE__)
//            configureScope("invalid/group");
//            owner.createProcessSemaphore(name);
//            check(!owner.processWait(0), "Private initialization rejects invalid scope without exception", checks);
//            configureScope(scope);
//    #else
//            configureScope("ignored-on-desktop");
//    #endif
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            check(owner.processWait(0), "Original public API works without any setter", checks);
//            owner.closeProcessSemaphore();
//            configureScope(scope);
//            owner.createProcessSemaphore(name);
//            check(!owner.processWait(0), "Private initialization remains reusable after close", checks);
//        }
//        else if (id == 18)
//        {
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            Semaphore peer;
//            peer.createProcessSemaphore(name);
//            check(peer.processWait(0), "Another creator joins without resetting existing count", checks);
//            owner.closeProcessSemaphore();
//            Semaphore later;
//            later.openProcessSemaphore(name);
//            peer.processSignal();
//            check(later.processWait(0), "New opener still sees name after initial creator closes", checks);
//            later.processSignal();
//            check(peer.processWait(0), "Both remaining peers share exactly the same object", checks);
//            peer.closeProcessSemaphore();
//            later.processSignal();
//            check(later.processWait(0), "Last peer remains usable", checks);
//            later.closeProcessSemaphore();
//            owner.openProcessSemaphore(name);
//            owner.processSignal();
//            check(!owner.processWait(0), "Last close removes the live name", checks);
//            owner.createProcessSemaphore(name);
//            check(!owner.processWait(0), "New generation has no old notifications", checks);
//        }
//        else if (id == 19)
//        {
//            owner.createProcessSemaphore(name + "a");
//            Semaphore other;
//            other.createProcessSemaphore(name + "b");
//            Semaphore peer;
//            peer.openProcessSemaphore(name + "a");
//            owner.processSignal();
//            check(!other.processWait(0), "Different names do not share a notification", checks);
//            check(peer.processWait(0), "Exact same name shares a notification", checks);
//            other.processSignal();
//            check(!owner.processWait(0), "Isolation holds in the other direction", checks);
//            check(other.processWait(0), "Other object retains its own notification", checks);
//        }
//    #if defined(__ANDROID__) || defined(__APPLE__)
//        else if (id == 22)
//        {
//            const uint32_t headers[][5] =
//            {
//                { 0x53454D31, 2, 20, 0, ~uint32_t(0) },
//                { 0x53454D32, 1, 20, 0, ~uint32_t(0) },
//                { 0x53454D32, 2, 24, 0, ~uint32_t(0) },
//                { 0x53454D32, 2, 20, 1, 0 },
//                { 0x53454D32, 2, 20, 0x80000000, 0x7FFFFFFF },
//                { 0, 2, 20, 0, ~uint32_t(0) },
//                { 0x53454D32, 2, 20, 3, ~uint32_t(3) }
//            };
//            const std::string file = scope + "/Semaphore." + name;
//            for (size_t index = 0; index < sizeof(headers) / sizeof(headers[0]); ++index)
//            {
//                const std::string fixture(reinterpret_cast<const char*>(headers[index]), sizeof(headers[index]));
//                std::ofstream output(file.c_str(), std::ios::binary | std::ios::trunc);
//                output.write(fixture.data(), static_cast<std::streamsize>(fixture.size()));
//                output.close();
//                check(!output.fail(), "Write owned v2 record fixture", checks);
//                check(::chmod(file.c_str(), 0600) == 0, "Restrict owned record fixture permissions", checks);
//                try
//                {
//                    if (index < 5)
//                    {
//                        owner.openProcessSemaphore(name);
//                        owner.createProcessSemaphore(name);
//                        owner.processSignal();
//                        check(!owner.processWait(0), "Incompatible or torn state must not become usable", checks);
//                        std::ifstream input(file.c_str(), std::ios::binary);
//                        const std::string actual((std::istreambuf_iterator<char>(input)),
//                            std::istreambuf_iterator<char>());
//                        check(!input.bad() && actual == fixture, "Rejected record is byte-for-byte unchanged", checks);
//                    }
//                    else
//                    {
//                        owner.createProcessSemaphore(name);
//                        check(!owner.processWait(0), "Inactive valid or unpublished generation is reset to empty", checks);
//                        owner.processSignal();
//                        Semaphore peer;
//                        peer.createProcessSemaphore(name);
//                        check(peer.processWait(0), "Live v2 generation is not reset", checks);
//                        check(!peer.processWait(0), "No duplicate notification in v2 generation", checks);
//                        owner.closeProcessSemaphore();
//                        peer.processSignal();
//                        check(peer.processWait(0), "Remaining peer keeps the generation alive", checks);
//                    }
//                    owner.closeProcessSemaphore();
//                    ::unlink(file.c_str());
//                }
//                catch (...)
//                {
//                    owner.closeProcessSemaphore();
//                    ::unlink(file.c_str());
//                    throw;
//                }
//            }
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            check(owner.processWait(0), "Same instance retries after invalid fixture removal", checks);
//        }
//        else if (id == 23)
//        {
//            owner.createProcessSemaphore(std::string(4096, 'x'));
//            owner.processSignal();
//            check(!owner.processWait(0), "Oversized name is rejected, never truncated", checks);
//            owner.createProcessSemaphore(name);
//            owner.processSignal();
//            check(owner.processWait(0), "Short-name retry works", checks);
//        }
//        else if (id == 24)
//        {
//            if (scope.empty())
//            {
//                return false;
//            }
//            const char* previous = std::getenv("TMPDIR");
//            const bool hadPrevious = previous != nullptr;
//            const std::string saved = hadPrevious ? previous : "";
//            check(::setenv("TMPDIR", scope.c_str(), 1) == 0, "Set owned temporary-directory configuration", checks);
//            configureScope("");
//            try
//            {
//                owner.createProcessSemaphore(name);
//                Semaphore peer;
//                peer.openProcessSemaphore(name);
//                owner.processSignal();
//                check(peer.processWait(0), "Optional scope uses the same default directory for original API", checks);
//                owner.closeProcessSemaphore();
//                peer.closeProcessSemaphore();
//            }
//            catch (...)
//            {
//                owner.closeProcessSemaphore();
//                if (hadPrevious)
//                {
//                    ::setenv("TMPDIR", saved.c_str(), 1);
//                }
//                else
//                {
//                    ::unsetenv("TMPDIR");
//                }
//                configureScope(scope);
//                throw;
//            }
//            check((hadPrevious ? ::setenv("TMPDIR", saved.c_str(), 1) : ::unsetenv("TMPDIR")) == 0,
//                "Restore only the owned test process environment", checks);
//            configureScope(scope);
//        }
//    #endif
//        return true;
//    };
//
//    int32_t selected = 0;
//    std::string scope;
//    std::string peer;
//    bool infinitePeer = false;
//    std::string abandoningPeer;
//    for (int32_t index = 1; index < argc; ++index)
//    {
//        const std::string argument = argv[index];
//        if (index + 1 >= argc)
//        {
//            return 2;
//        }
//        if (argument == "--case")
//        {
//            char* end = nullptr;
//            const long value = std::strtol(argv[++index], &end, 10);
//            if (end == argv[index] || *end != '\0' || value < 1 || value > 24)
//            {
//                return 2;
//            }
//            selected = static_cast<int32_t>(value);
//        }
//        else if (argument == "--scope")
//        {
//            scope = argv[++index];
//        }
//        else if (argument == "--peer" || argument == "--peer-infinite")
//        {
//            infinitePeer = argument == "--peer-infinite";
//            peer = argv[++index];
//        }
//        else if (argument == "--abandon")
//        {
//            abandoningPeer = argv[++index];
//        }
//        else
//        {
//            return 2;
//        }
//    }
//    if (!peer.empty())
//    {
//        return runPeer(peer, scope, infinitePeer);
//    }
//    if (!abandoningPeer.empty())
//    {
//        runAbandonedPeer(abandoningPeer, scope);
//    }
//    int32_t failed = 0;
//    int32_t passed = 0;
//    int32_t skipped = 0;
//    int32_t totalChecks = 0;
//    for (int32_t id = 1; id <= 24; ++id)
//    {
//        if (selected != 0 && selected != id)
//        {
//            continue;
//        }
//        int32_t checks = 0;
//        try
//        {
//            if (runCase(id, scope, checks))
//            {
//                ++passed;
//                std::printf("PASS id=%d checks=%d\n", id, checks);
//            }
//            else
//            {
//                ++skipped;
//                std::printf("SKIP id=%d %s\n", id,
//                    (id == 14 || id == 15 || id == 17) ? "retired test" : "platform/host not applicable");
//            }
//        }
//        catch (const std::exception& error)
//        {
//            ++failed;
//            std::fprintf(stderr, "FAIL id=%d checks=%d: %s\n", id, checks, error.what());
//        }
//        totalChecks += checks;
//    }
//    std::printf("RESULT pass=%d fail=%d skip=%d checks=%d\n", passed, failed, skipped, totalChecks);
//    return failed == 0 ? 0 : 1;
//}