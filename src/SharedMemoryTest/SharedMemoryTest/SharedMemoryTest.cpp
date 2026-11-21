#include "PlatformTest/PlatformTestAPI.h"
#include "SharedMemoryTest.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include <chrono>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <memory>
// Android 的 adb 测试程序可重新启动自身，iOS 应用入口由宿主启动。
#if !defined(SHARED_MEMORY_MOBILE) || defined(__ANDROID__)
#define SHARED_MEMORY_TEST_CAN_EXEC 1
#endif
#ifdef _WIN32
#include <windows.h>
#include <sddl.h>
#pragma comment(lib, "advapi32.lib")
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef SHARED_MEMORY_TEST_CAN_EXEC
#include <sys/wait.h>
#include <cerrno>
#include <csignal>
#include <thread>
#include <vector>
#endif
#endif

/** 运行本工程的接口性能用例
@return 输入和结果满足各用例约定时返回0
*/
static int RunBenchmarks()
{
    SharedMemory mapping("", 4096, false);
    uint8_t* data = static_cast<uint8_t*>(mapping.writeWithoutLock());
    const uint32_t capacity = mapping.realSize();
    if (data == nullptr || capacity < 4096)
    {
        return 1;
    }
    uint32_t invalid = 0;
    LibraryBenchmark::run("SharedMemory.writeWithoutLock.warm", [&](uint32_t count) -> uint64_t {
        uint64_t successes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            successes += mapping.writeWithoutLock() == data;
        }
        invalid |= successes != count;
        return successes;
    });
    LibraryBenchmark::run("SharedMemory.realSize.mapped", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            sum += mapping.realSize();
        }
        invalid |= sum != static_cast<uint64_t>(count) * capacity;
        return sum;
    });
    SharedMemory unmapped("", 4096, false);
    if (unmapped.isFailed() || unmapped.realSize() != capacity || unmapped.memory() != nullptr)
    {
        return 1;
    }
    LibraryBenchmark::run("SharedMemory.realSize.unmapped", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            sum += unmapped.realSize();
        }
        invalid |= sum != static_cast<uint64_t>(count) * capacity || unmapped.memory() != nullptr;
        return sum;
    });
    LibraryBenchmark::run("SharedMemory.clear.4KiB", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            const size_t offset = i & 4095;
            data[offset] = 0x5a;
            mapping.clear();
            sum += data[offset];
        }
        invalid |= sum != 0;
        return sum;
    });
    if (invalid != 0)
    {
        return 1;
    }
    return 0;
}

#define CHECK(expression) do { if (!(expression)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expression); return 1; } } while (false)

/** 设置平台共享目录并验证非法路径，成功返回 0。 */
static int ConfigureDirectory()
{
#ifdef SHARED_MEMORY_MOBILE
    const char* directory = std::getenv("SHARED_MEMORY_TEST_DIR");
    if (directory == nullptr)
    {
        directory = std::getenv("TMPDIR");
    }
    CHECK(directory != nullptr && SharedMemory::setProcessDirectory(directory));
#endif
#ifndef SHARED_MEMORY_MOBILE
    CHECK(!SharedMemory::setProcessDirectory("relative-path"));
    CHECK(SharedMemory::setProcessDirectory(""));
#endif
    return 0;
}

/** 生成当前场景独占的名称。 */
static std::string TestName()
{
    std::ostringstream key;
    key << "smt" << std::hex;
#ifdef _WIN32
    key << ::GetCurrentProcessId();
#else
    key << ::getpid();
#endif
    static std::atomic<uint32_t> sequence(0);
    key << '_' << static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count())
        << '_' << sequence.fetch_add(1);
    return key.str();
}

#ifdef _WIN32
/** 核对同一映射内的完整容量和可安全清零的前缀。
@return 容量、清零范围和保护页均符合预期时返回 0
*/
static int TestMappingExtent()
{
    SYSTEM_INFO system = {};
    ::GetSystemInfo(&system);
    const DWORD page = system.dwPageSize;
    SharedMemory owner(TestName(), 3 * page, false);
    uint8_t* view = static_cast<uint8_t*>(owner.writeWithoutLock());
    CHECK(view != nullptr);
    view[0] = 0x39;
    view[2 * page] = 0x71;
    DWORD oldProtection = 0;
    CHECK(::VirtualProtect(view + page, page, PAGE_READONLY, &oldProtection));
    CHECK(owner.realSize() == 3 * page);
    owner.clear();
    CHECK(view[0] == 0x39 && view[2 * page] == 0x71);
    DWORD ignored = 0;
    CHECK(::VirtualProtect(view + page, page, oldProtection, &ignored));
    owner.clear();
    CHECK(view[0] == 0 && view[2 * page] == 0);
#ifdef _WIN64
    const std::string name = TestName();
    std::unique_ptr<void, decltype(&::CloseHandle)> section(
        ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE | SEC_RESERVE, 1, 0, name.c_str()), ::CloseHandle);
    CHECK(section.get() != nullptr);
    SharedMemory prefix(name, 128, true);
    uint8_t* reserved = static_cast<uint8_t*>(prefix.writeWithoutLock());
    CHECK(reserved != nullptr && ::VirtualAlloc(reserved, page, MEM_COMMIT, PAGE_READWRITE) == reserved);
    reserved[127] = 0x31;
    reserved[128] = 0x42;
    CHECK(prefix.realSize() == 0);
    prefix.clear();
    CHECK(reserved[127] == 0 && reserved[128] == 0x42);
#endif
    return 0;
}
#endif

/** 两实例检验碰撞、别名、双向读写与清零，成功返回 0。 */
static int TestViews()
{
    CHECK(ConfigureDirectory() == 0);
    const std::string name = TestName();
    SharedMemory owner(name, 128, false);
    CHECK(!owner.isFailed() && owner.size() == 128 && owner.realSize() >= 128 && owner.mapName() == name);
#ifdef SHARED_MEMORY_MOBILE
    CHECK(ConfigureDirectory() == 0 && !SharedMemory::setProcessDirectory(""));
#else
    CHECK(SharedMemory::setProcessDirectory("") && !SharedMemory::setProcessDirectory("relative-path"));
#endif
    SharedMemory truncated(name + std::string("\0invalid", 8));
    CHECK(truncated.readWithoutLock() == nullptr && truncated.writeWithoutLock() == nullptr && truncated.realSize() == 0);
    SharedMemory unopened(name);
    unopened.clear();
    CHECK(unopened.memory() == nullptr);
    SharedMemory collision(name, 128, false);
    CHECK(collision.isFailed());
    CHECK(!collision.unlinkName());
    SharedMemory joined(name, 128, true);
    CHECK(!joined.isFailed() && joined.writeWithoutLock() != nullptr);
    SharedMemory oversize(name, 1024 * 1024, true);
    CHECK(oversize.isFailed() && oversize.writeWithoutLock() == nullptr);
    CHECK(!oversize.unlinkName());
    SharedMemory withinPage(name, 129, true);
#ifdef _WIN32
    SYSTEM_INFO information = {};
    ::GetSystemInfo(&information);
    CHECK(information.dwPageSize >= 129 && !withinPage.isFailed());
    SharedMemory nextPage(name, information.dwPageSize + 1, true);
    CHECK(nextPage.isFailed());
#else
    CHECK(withinPage.isFailed());
#endif
    SharedMemory reader(name);
    const uint8_t* view = static_cast<const uint8_t*>(reader.readWithoutLock());
    uint8_t* contents = static_cast<uint8_t*>(owner.writeWithoutLock());
    CHECK(view && contents);
    uint8_t expected[128];
    for (size_t i = 0; i < sizeof(expected); ++i)
    {
        expected[i] = static_cast<uint8_t>(i + 1);
    }
    std::memcpy(contents, expected, sizeof(expected));
    CHECK(std::memcmp(view, expected, sizeof(expected)) == 0);
    CHECK(reader.size() == 0 && reader.memory() == view);
    reader.clear();
    CHECK(std::memcmp(view, expected, sizeof(expected)) == 0);
    uint8_t* writeView = static_cast<uint8_t*>(reader.writeWithoutLock());
    CHECK(writeView != nullptr);
    writeView[1] = 0xfe;
    CHECK(contents[1] == 0xfe);
    reader.close();
    CHECK(reader.memory() == nullptr);
    view = static_cast<const uint8_t*>(reader.readWithoutLock());
    CHECK(view != nullptr && view[1] == 0xfe);
#ifdef _WIN32
    CHECK(!owner.unlinkName());
#else
    owner.preserveName();
    CHECK(owner.unlinkName());
    SharedMemory unlinked(name);
    CHECK(unlinked.readWithoutLock() == nullptr && view[1] == 0xfe);
    SharedMemory replacement(name, 128, false);
    CHECK(!replacement.isFailed() && owner.unlinkName());
    SharedMemory replacementPeer(name);
    CHECK(replacementPeer.readWithoutLock() != nullptr);
#endif
    owner.clear();
    for (int i = 0; i < 128; ++i)
    {
        CHECK(view[i] == 0);
    }
    reader.close(); reader.close();
    owner.close();
    return 0;
}

/** 以较短长度加入已有映射，逐字节核对 clear 的边界。
@return 原视图的请求前缀全零且尾部完整时返回 0
*/
static int TestPartialClear()
{
    CHECK(ConfigureDirectory() == 0);
    const std::string name = TestName();
    uint8_t expected[73];
    const uint32_t prefixSize = 17;
    SharedMemory owner(name, sizeof(expected), false);
    uint8_t* contents = static_cast<uint8_t*>(owner.writeWithoutLock());
    CHECK(contents != nullptr);
    for (size_t i = 0; i < sizeof(expected); ++i)
    {
        expected[i] = static_cast<uint8_t>(i + 1);
    }
    std::memcpy(contents, expected, sizeof(expected));
    SharedMemory prefix(name, prefixSize, true);
    CHECK(!prefix.isFailed() && prefix.writeWithoutLock() != nullptr && prefix.size() == prefixSize);
    prefix.clear();
    std::memset(expected, 0, prefixSize);
    CHECK(std::memcmp(contents, expected, sizeof(expected)) == 0);
    return 0;
}

/** 验证缺失名称与匿名映射的生命周期，成功返回 0。 */
static int TestAnonymous()
{
    CHECK(ConfigureDirectory() == 0);
    const std::string name = TestName();
    SharedMemory absent(name);
    CHECK(absent.readWithoutLock() == nullptr && absent.writeWithoutLock() == nullptr && absent.realSize() == 0);
    SharedMemory anonymous("", 16, false);
    CHECK(!anonymous.isFailed() && anonymous.writeWithoutLock() != nullptr && !anonymous.unlinkName());
    uint8_t* privateData = static_cast<uint8_t*>(anonymous.writeWithoutLock());
    std::memset(privateData, 0x9a, 16);
    anonymous.clear();
    for (int i = 0; i < 16; ++i)
    {
        CHECK(privateData[i] == 0);
    }
    privateData[0] = 0x49;
    anonymous.close();
    CHECK(anonymous.memory() == nullptr);
    privateData = static_cast<uint8_t*>(anonymous.writeWithoutLock());
    CHECK(privateData != nullptr);
#ifdef _WIN32
    CHECK(privateData[0] == 0x49);
#else
    CHECK(privateData[0] == 0);
#endif
    return 0;
}

/** 先观察缺名，再验证创建者退出和最后一个引用释放后的名称及数据。
@return 名称和已有视图的寿命符合当前平台约定时返回 0
*/
static int TestNameLifetime()
{
    CHECK(ConfigureDirectory() == 0);
    const std::string name = TestName();
    bool keptView = false;
    {
        SharedMemory early(name);
        CHECK(early.readWithoutLock() == nullptr && early.realSize() == 0);
        const uint8_t* view = nullptr;
        {
            SharedMemory owner(name, 16, false);
            uint8_t* data = static_cast<uint8_t*>(owner.writeWithoutLock());
            CHECK(data != nullptr);
            data[15] = 0x79;
            view = static_cast<const uint8_t*>(early.readWithoutLock());
            CHECK(view != nullptr && view[15] == 0x79);
            owner.preserveName();
        }
        keptView = early.readWithoutLock() == view && view[15] == 0x79;
    }
    SharedMemory reopened(name);
#ifdef _WIN32
    CHECK(keptView && reopened.readWithoutLock() == nullptr && reopened.isFailed());
#else
    const uint8_t* data = static_cast<const uint8_t*>(reopened.readWithoutLock());
    const bool survived = data != nullptr && data[15] == 0x79;
    const bool removed = reopened.unlinkName();
    SharedMemory absent(name);
    CHECK(keptView && survived && removed && absent.readWithoutLock() == nullptr);
#endif
    return 0;
}

#ifdef _WIN32
/** 使用系统创建的只读映射，验证写升级失败后原读视图及 memory() 仍有效。 */
static int TestReadAccess()
{
    const std::string name = TestName();
    const std::unique_ptr<void, decltype(&::CloseHandle)> object(
        ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READONLY, 0, 64, name.c_str()), &::CloseHandle);
    CHECK(object.get() != nullptr && ::GetLastError() != ERROR_ALREADY_EXISTS);
    SharedMemory peer(name);
    const uint8_t* view = static_cast<const uint8_t*>(peer.readWithoutLock());
    CHECK(view != nullptr && peer.memory() == view && view[0] == 0);
    CHECK(peer.writeWithoutLock() == nullptr && peer.memory() == view);
    CHECK(peer.realSize() >= 64 && peer.readWithoutLock() == view);
    peer.close();
    CHECK(peer.memory() == nullptr);
    // 对现有映射仅授予 SECTION_MAP_READ | SECTION_MAP_WRITE，独立系统打开用于核对测试前提。
    const std::string limitedName = TestName();
    const std::unique_ptr<void, decltype(&::CloseHandle)> limited(
        ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 64, limitedName.c_str()), &::CloseHandle);
    CHECK(limited.get() != nullptr && ::GetLastError() != ERROR_ALREADY_EXISTS);
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    CHECK(::ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(A;;0x0006;;;WD)",
        SDDL_REVISION_1, &descriptor, nullptr) != FALSE);
    const std::unique_ptr<void, decltype(&::LocalFree)> security(descriptor, &::LocalFree);
    CHECK(::SetKernelObjectSecurity(limited.get(), DACL_SECURITY_INFORMATION, descriptor) != FALSE);
    const std::unique_ptr<void, decltype(&::CloseHandle)> allowed(
        ::OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, limitedName.c_str()), &::CloseHandle);
    CHECK(allowed.get() != nullptr);
    SharedMemory joined(limitedName, 64, true);
    CHECK(!joined.isFailed() && joined.writeWithoutLock() != nullptr);
    SharedMemory direct(limitedName);
    uint8_t* writable = static_cast<uint8_t*>(direct.writeWithoutLock());
    CHECK(writable != nullptr);
    writable[7] = 0x5b;
    SharedMemory upgraded(limitedName);
    const uint8_t* original = static_cast<const uint8_t*>(upgraded.readWithoutLock());
    CHECK(original && original[7] == 0x5b);
    uint8_t* upgradedWrite = static_cast<uint8_t*>(upgraded.writeWithoutLock());
    CHECK(upgradedWrite != nullptr);
    upgradedWrite[7] = 0x26;
    CHECK(writable[7] == 0x26 && original[7] == 0x26);
    return 0;
}
/** 以操作系统准许的 FILE_MAP_WRITE 视图作为容量和字节可见性的独立预期
@return 按名加入、首次读写、查询容量和重新映射都成功时返回 0
*/
static int TestWriteMappingRight()
{
    const std::string name = TestName();
    const std::unique_ptr<void, decltype(&::CloseHandle)> owner(
        ::CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, 73, name.c_str()), &::CloseHandle);
    CHECK(owner.get() != nullptr && ::GetLastError() != ERROR_ALREADY_EXISTS);
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    CHECK(::ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(A;;0x0002;;;OW)",
        SDDL_REVISION_1, &descriptor, nullptr));
    const std::unique_ptr<void, decltype(&::LocalFree)> security(descriptor, &::LocalFree);
    CHECK(::SetKernelObjectSecurity(owner.get(), DACL_SECURITY_INFORMATION, descriptor));
    const std::unique_ptr<void, decltype(&::CloseHandle)> both(
        ::OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, name.c_str()), &::CloseHandle);
    CHECK(!both && ::GetLastError() == ERROR_ACCESS_DENIED);
    const std::unique_ptr<void, decltype(&::CloseHandle)> readOnly(
        ::OpenFileMappingA(FILE_MAP_READ, FALSE, name.c_str()), &::CloseHandle);
    CHECK(!readOnly && ::GetLastError() == ERROR_ACCESS_DENIED);
    const std::unique_ptr<void, decltype(&::CloseHandle)> writable(
        ::OpenFileMappingA(FILE_MAP_WRITE, FALSE, name.c_str()), &::CloseHandle);
    CHECK(writable.get() != nullptr);
    const std::unique_ptr<void, decltype(&::UnmapViewOfFile)> original(
        ::MapViewOfFile(writable.get(), FILE_MAP_WRITE, 0, 0, 0), &::UnmapViewOfFile);
    CHECK(original.get() != nullptr);
    MEMORY_BASIC_INFORMATION region = {};
    CHECK(::VirtualQuery(original.get(), &region, sizeof(region)) == sizeof(region));
    uint8_t* reference = static_cast<uint8_t*>(original.get());
    reference[72] = 0x65;

    SharedMemory joined(name, 73, true);
    SharedMemory sized(name);
    SharedMemory reader(name);
    SharedMemory writer(name);
    CHECK(!joined.isFailed() && joined.realSize() == region.RegionSize);
    CHECK(sized.realSize() == region.RegionSize && sized.memory() == nullptr);
    const uint8_t* read = static_cast<const uint8_t*>(reader.readWithoutLock());
    CHECK(read && read[72] == 0x65 && reader.memory() == read);
    uint8_t* write = static_cast<uint8_t*>(writer.writeWithoutLock());
    CHECK(write && write[72] == 0x65);
    write[72] = 0x49;
    CHECK(read[72] == 0x49 && reference[72] == 0x49);
    joined.clear();
    CHECK(read[72] == 0 && reference[72] == 0);
    reader.close();
    const uint8_t* reopened = static_cast<const uint8_t*>(reader.readWithoutLock());
    CHECK(reopened && reopened[72] == 0 && reader.realSize() == region.RegionSize);
    return 0;
}
#else
/** 校验只读权限，系统原生的读写打开结果用作独立预期。 */
static int TestReadAccess()
{
    CHECK(ConfigureDirectory() == 0);
    const std::string name = TestName();
    SharedMemory owner(name, 64, false);
    uint8_t* original = static_cast<uint8_t*>(owner.writeWithoutLock());
    CHECK(original != nullptr);
    original[17] = 0x73;
#ifdef SHARED_MEMORY_MOBILE
    const char* directory = std::getenv("SHARED_MEMORY_TEST_DIR");
    if (directory == nullptr)
    {
        directory = std::getenv("TMPDIR");
    }
    const std::string path = std::string(directory) + "/m_" + name;
    const int descriptor = ::open(path.c_str(), O_RDWR);
#else
    const std::string path = "/" + name;
    const int descriptor = ::shm_open(path.c_str(), O_RDWR, 0);
#endif
    CHECK(descriptor >= 0);
    const bool restricted = ::fchmod(descriptor, 0400) == 0;
    ::close(descriptor);
    CHECK(restricted);
#ifdef SHARED_MEMORY_MOBILE
    const int writable = ::open(path.c_str(), O_RDWR);
#else
    const int writable = ::shm_open(path.c_str(), O_RDWR, 0);
#endif
    if (writable >= 0)
    {
        ::close(writable);
    }
    SharedMemory peer(name);
    CHECK(peer.realSize() == 64);
    const uint8_t* view = static_cast<const uint8_t*>(peer.readWithoutLock());
    CHECK(view != nullptr && view[17] == 0x73);
    CHECK((peer.writeWithoutLock() != nullptr) == (writable >= 0));
    // 模拟另一个创建者已发布名称，稍后才完成扩容和权限设置。
    const std::string pendingName = TestName();
#ifdef SHARED_MEMORY_MOBILE
    const std::string pendingPath = std::string(directory) + "/m_" + pendingName;
    const int pending = ::open(pendingPath.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
#else
    const std::string pendingPath = "/" + pendingName;
    const int pending = ::shm_open(pendingPath.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
#endif
    CHECK(pending >= 0);
    SharedMemory retry(pendingName);
    const bool empty = ::fchmod(pending, 0400) == 0 && retry.readWithoutLock() == nullptr;
    const bool ready = ::ftruncate(pending, 64) == 0 && ::fchmod(pending, 0600) == 0;
    ::close(pending);
    uint8_t* data = static_cast<uint8_t*>(retry.writeWithoutLock());
    bool shared = false;
    if (data != nullptr)
    {
        data[63] = 0x52;
        SharedMemory observer(pendingName);
        const uint8_t* read = static_cast<const uint8_t*>(observer.readWithoutLock());
        shared = read != nullptr && read[63] == 0x52;
    }
    const bool removed = retry.unlinkName();
    CHECK(empty && ready && shared && removed);
    return 0;
}
#endif

#ifdef SHARED_MEMORY_TEST_CAN_EXEC
#ifndef _WIN32
// 启动参数中的测试程序路径，供目录配置用例重新执行未封存配置的进程
static const char* testExecutable = nullptr;

/** 在限定时间内取得测试子进程的退出状态，超时后终止并回收该子进程。
@param [in] child 当前用例启动的子进程 ID
@return 正常退出且状态为零时返回 true
*/
static bool ChildPassed(pid_t child)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    int status = 0;
    while (std::chrono::steady_clock::now() < deadline)
    {
        const pid_t result = ::waitpid(child, &status, WNOHANG);
        if (result == child)
        {
            return WIFEXITED(status) && WEXITSTATUS(status) == 0;
        }
        if (result < 0 && errno != EINTR)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ::kill(child, SIGKILL);
    while (::waitpid(child, &status, 0) < 0 && errno == EINTR) {}
    return false;
}

/** 在独立进程设置实际目录，检查文件落点、容量、配置封存和析构撤名。
@param [in] directory 父进程分配的独占临时目录
@return 全部状态符合约定时返回 0
*/
static int ProbeDirectory(const std::string& directory)
{
    const std::string selected = directory + "/selected";
    const std::string moved = directory + "/moved";
    CHECK(::mkdir(selected.c_str(), 0700) == 0 && SharedMemory::setProcessDirectory(selected));
    CHECK(::rename(selected.c_str(), moved.c_str()) == 0 && ::mkdir(selected.c_str(), 0700) == 0);
    const std::string other = moved + "/m_reserved";
    CHECK(::mkdir(other.c_str(), 0700) == 0);
    SharedMemory invalid("reserved");
    const bool rejected = invalid.readWithoutLock() == nullptr && invalid.writeWithoutLock() == nullptr &&
        invalid.realSize() == 0;
    CHECK(::rmdir(other.c_str()) == 0 && rejected);
    const std::string path = moved + "/m_check";
    {
        SharedMemory owner("check", 97, false);
        struct stat info = {};
        CHECK(!owner.isFailed() && ::stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode) && info.st_size == 97);
        CHECK(::access((selected + "/m_check").c_str(), F_OK) < 0 && errno == ENOENT);
        CHECK(SharedMemory::setProcessDirectory(selected) && !SharedMemory::setProcessDirectory(""));
        SharedMemory peer("check");
        uint8_t* write = static_cast<uint8_t*>(owner.writeWithoutLock());
        const uint8_t* read = static_cast<const uint8_t*>(peer.readWithoutLock());
        CHECK(write != nullptr && read != nullptr);
        write[96] = 0x3a;
        CHECK(read[96] == 0x3a);
    }
    CHECK(::access(path.c_str(), F_OK) < 0 && errno == ENOENT);
    return 0;
}

/** 用新建的临时目录验证命名映射的配置，不依赖其他用例的进程全局设置。 */
static int TestDirectory()
{
    const char* temp = std::getenv("TMPDIR");
    std::string pattern = std::string(temp && *temp ? temp : "/tmp") + "/SharedMemoryTest-XXXXXX";
    std::vector<char> directory(pattern.begin(), pattern.end());
    directory.push_back('\0');
    CHECK(::mkdtemp(directory.data()) != nullptr);
    const pid_t child = ::fork();
    if (child == 0)
    {
        ::execlp(testExecutable, testExecutable, "--shared-memory-directory", directory.data(), static_cast<char*>(nullptr));
        ::_exit(127);
    }
    const bool passed = child > 0 && ChildPassed(child);
    const char* const subdirectories[] = {"/selected", "/moved"};
    for (const char* subdirectory : subdirectories)
    {
        const std::string path = std::string(directory.data()) + subdirectory;
        ::unlink((path + "/m_check").c_str());
        ::rmdir((path + "/m_reserved").c_str());
        ::rmdir(path.c_str());
    }
    const int removed = ::rmdir(directory.data());
    CHECK(passed && removed == 0);
    return 0;
}
#endif

/** 子进程独立打开指定名称，核对标记并通过自己的写视图返回字节。
@param [in] name 父进程创建的映射名称
@return 读写完成时返回 0
*/
static int ProbePeer(const std::string& name)
{
    CHECK(ConfigureDirectory() == 0);
    SharedMemory peer(name);
    const uint8_t* read = static_cast<const uint8_t*>(peer.readWithoutLock());
    CHECK(read != nullptr && read[0] == 0x36);
    uint8_t* write = static_cast<uint8_t*>(peer.writeWithoutLock());
    CHECK(write != nullptr);
    write[1] = 0x91;
    return 0;
}

/** 父子进程独立打开同一个命名映射，父进程在子进程退出后核对回写值。 */
static int TestProcesses()
{
    CHECK(ConfigureDirectory() == 0);
    const std::string name = TestName();
    SharedMemory owner(name, 64, false);
    uint8_t* view = static_cast<uint8_t*>(owner.writeWithoutLock());
    CHECK(view != nullptr);
    view[0] = 0x36;
    view[1] = 0;
#ifdef _WIN32
    wchar_t path[32768];
    const DWORD length = ::GetModuleFileNameW(nullptr, path, sizeof(path) / sizeof(path[0]));
    CHECK(length > 0 && length < sizeof(path) / sizeof(path[0]));
    std::wstring command = L"\"" + std::wstring(path, length) + L"\" --shared-memory-peer " +
        std::wstring(name.begin(), name.end());
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    CHECK(::CreateProcessW(path, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
        nullptr, nullptr, &startup, &process) != FALSE);
    ::CloseHandle(process.hThread);
    const DWORD waited = ::WaitForSingleObject(process.hProcess, 15000);
    if (waited != WAIT_OBJECT_0)
    {
        ::TerminateProcess(process.hProcess, 1);
        ::WaitForSingleObject(process.hProcess, INFINITE);
    }
    DWORD code = 1;
    const BOOL queried = ::GetExitCodeProcess(process.hProcess, &code);
    ::CloseHandle(process.hProcess);
    CHECK(waited == WAIT_OBJECT_0 && queried && code == 0);
#else
    const pid_t child = ::fork();
    CHECK(child >= 0);
    if (child == 0)
    {
        ::execlp(testExecutable, testExecutable, "--shared-memory-peer", name.c_str(), static_cast<char*>(nullptr));
        ::_exit(127);
    }
    CHECK(ChildPassed(child));
#endif
    CHECK(view[1] == 0x91);
    return 0;
}
#endif

int main(int argc, char** argv)
{
#ifdef SHARED_MEMORY_TEST_CAN_EXEC
    if (argc == 3 && std::strcmp(argv[1], "--shared-memory-peer") == 0)
    {
        return ProbePeer(argv[2]);
    }
#ifndef _WIN32
    testExecutable = argv[0];
    if (argc == 3 && std::strcmp(argv[1], "--shared-memory-directory") == 0)
    {
        return ProbeDirectory(argv[2]);
    }
#endif
#endif
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return RunBenchmarks();
    }
    const LibraryTestCase cases[] = {
        {1, false, TestViews},
        {2, false, TestAnonymous},
        {3, false, TestReadAccess},
#ifdef SHARED_MEMORY_TEST_CAN_EXEC
        {4, false, TestProcesses},
#ifndef _WIN32
        {5, false, TestDirectory},
#else
        {6, false, TestMappingExtent},
#endif
#endif
        {7, false, TestNameLifetime},
        {8, false, TestPartialClear},
#ifdef _WIN32
        {9, false, TestWriteMappingRight},
#endif
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}