#include "PlatformTest/PlatformTestAPI.h"
#include "CSystemTest.h"
#include "CSystem/CSystemAPI.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <memory>
#if CSYSTEM_HAS_NATIVE_API
#include <codecvt>
#include <locale>
#endif

/** 运行本工程的接口性能用例
@return 输入和结果满足各用例约定时返回0
*/
static int RunBenchmarks()
{
    const std::string dirs[] = {"project/media/recordings/session-alpha", "project/media/recordings/session-bravo/"};
    const std::string file = "session-recording.2026.final.flac";
    const std::string paths[] = {dirs[0] + "/" + file, dirs[1] + file};
    if (CSystem::GetName(paths[0], 2) != "flac" || CSystem::GetName(paths[1], 1) != file ||
        CSystem::joinPath(dirs[0], file) != paths[0] || CSystem::joinPath(dirs[1], file) != paths[1])
    {
        return 1;
    }
    for (int32_t mode = 1; mode <= 2; ++mode)
    {
        LibraryBenchmark::run(mode == 1 ? "CSystem.GetName.filename" : "CSystem.GetName.extension", [&](uint32_t count) -> uint64_t {
            uint64_t sum = 0;
            for (uint32_t i = 0; i < count; ++i)
            {
                sum += CSystem::GetName(paths[i & 1], mode).size();
            }
            return sum;
        });
    }
    LibraryBenchmark::run("CSystem.joinPath", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            sum += CSystem::joinPath(dirs[i & 1], file).size();
        }
        return sum;
    });
    return 0;
}

#define CHECK(expression) do { if (!(expression)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expression); return 1; } } while (false)
#if CSYSTEM_HAS_NATIVE_API
CSystemTestFile::CSystemTestFile(const std::string& path) :
    m_path(path), m_handle(CSystem::openFileHandle(path.c_str(), 2)), m_duplicate(-1)
{
    if (m_handle == -1)
    {
        m_path.clear();
    }
}

CSystemTestFile::~CSystemTestFile()
{
    CSystem::closeFileHandle(&m_handle);
    CSystem::closeFileHandle(&m_duplicate);
    if (!m_path.empty())
    {
        CSystem::deleteFile(m_path.c_str());
    }
}
#endif

int32_t CSystemTestQtRect::left() const
{
    return m_edges[0];
}

int32_t CSystemTestQtRect::top() const
{
    return m_edges[1];
}

int32_t CSystemTestQtRect::right() const
{
    return m_edges[2];
}

int32_t CSystemTestQtRect::bottom() const
{
    return m_edges[3];
}

void CSystemTestQtRect::setLeft(int32_t value)
{
    m_edges[0] = value;
}

void CSystemTestQtRect::setTop(int32_t value)
{
    m_edges[1] = value;
}

void CSystemTestQtRect::setRight(int32_t value)
{
    m_edges[2] = value;
}

void CSystemTestQtRect::setBottom(int32_t value)
{
    m_edges[3] = value;
}

int CSystemTestElement::live = 0;

CSystemTestElement::CSystemTestElement()
{
    if (live == 3)
    {
        throw 3;
    }
    ++live;
}

CSystemTestElement::~CSystemTestElement()
{
    --live;
}

#ifdef _WIN32
#ifdef _MSC_VER
// VS2013 对显式模板实例化仍做私有访问检查。测试链接到该编译器的真实静态成员符号。
extern "C" void __cdecl CSystemFallbackWait(uint32_t milliseconds);
#ifdef _M_IX86
#pragma comment(linker, "/alternatename:_CSystemFallbackWait=?sleepWithTimerResolution@CSystem@@CAXI@Z")
#else
#pragma comment(linker, "/alternatename:CSystemFallbackWait=?sleepWithTimerResolution@CSystem@@CAXI@Z")
#endif
#else
namespace
{
/** Windows 回退分支的实际函数指针，只在本用例中使用。 */
struct WindowsFallback
{
    typedef void (*Wait)(uint32_t);
    friend Wait fallbackWait(WindowsFallback);
};

/** 在测试翻译单元显式实例化私有静态成员地址，保持生产类声明一致。 */
template <WindowsFallback::Wait Wait>
struct BindWindowsFallback
{
    friend WindowsFallback::Wait fallbackWait(WindowsFallback)
    {
        return Wait;
    }
};
template struct BindWindowsFallback<&CSystem::sleepWithTimerResolution>;
/** 通过编译器生成的指针调用真实回退函数。 */
void CSystemFallbackWait(uint32_t milliseconds)
{
    fallbackWait(WindowsFallback())(milliseconds);
}
}
#endif
#endif

#if CSYSTEM_HAS_NATIVE_API
/** 检查每次短等待均走到结束；严格模式另验证原有 20 次等待的时间窗口
@param [in] wait 毫秒等待函数
@param [in] strict 是否核验诊断模式的 160 毫秒上界
@return 满足计时条件返回 true
*/
static bool CheckSleep(void (*wait)(uint32_t), bool strict)
{
    wait(1);
    const auto begin = std::chrono::steady_clock::now();
    for (int i = 0; i < 20; ++i)
    {
        wait(2);
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count();
    return elapsed >= 38 && (!strict || elapsed < 160);
}
#endif

/** 核对数组异常清理、时间单位、路径以及参数转换，成功返回 0。 */
static int TestCommon()
{
    bool threw = false;
    try
    {
        CSystemTestElement** values = CSystem::CreateDyadicArray<CSystemTestElement>(3, 2);
        CSystem::DestroyDyadicArray(values, 3);
    }
    catch (int marker)
    {
        threw = marker == 3;
    }
    CHECK(threw && CSystemTestElement::live == 0);
    CHECK(CSystem::CreateDyadicArray<int>(-1, 2) == nullptr);
    int** matrix = CSystem::CreateDyadicArray<int>(2, 3);
    CHECK(matrix != nullptr);
    matrix[0][2] = 19; matrix[1][2] = 23;
    CHECK(matrix[0][2] == 19 && matrix[1][2] == 23);
    CSystem::DestroyDyadicArray(matrix, 2);
    CSystem::DestroyDyadicArray<int>(nullptr, 0);
    CSystemTestRect r = {-3, 2, 7, -1};
    r = CSystem::rectValid(r);
    CHECK(r.left == 0 && r.top == 2 && r.right == 7 && r.bottom == 0);
    const CSystemTestQtRect qtRect = {-3, 2, 7, -1};
    const CSystemTestQtRect clamped = CSystem::qrectValid(qtRect);
    CHECK(clamped.left() == 0 && clamped.top() == 2 && clamped.right() == 7 && clamped.bottom() == 0);
    CHECK(qtRect.left() == -3 && qtRect.bottom() == -1);
    CHECK(CSystem::GetName("dir/abc.txt", 1) == "abc.txt");
    CHECK(CSystem::GetName("dir/abc.txt", 3) == "abc");
    CHECK(CSystem::GetName("dot.dir/trailing", 2).empty());
    CHECK(CSystem::GetName("folder/.config", 3).empty());
    CHECK(CSystem::GetName("folder\\", 4) == "folder\\");
    CHECK(CSystem::GetName("name.", 2).empty() && CSystem::GetName("name", 3) == "name");
    CHECK(CSystem::GetName("no-dir", 4).empty() && CSystem::GetName("no-dir", 0).empty());
    CHECK(CSystem::joinPath("", "name") == "name" && CSystem::joinPath("dir", "") == "dir");
    CHECK(CSystem::joinPath("dir/", "abc.txt") == "dir/abc.txt");
    CHECK(CSystem::ensureFileExtension("abc", ".txt") == "abc.txt");
    CHECK(CSystem::ensureFileExtension("a.TxT", ".txt") == "a.txt" && CSystem::ensureFileExtension("", ".txt").empty());
    CHECK(CSystem::timetToStr(0, false) == "1970-01-01 00:00:00");
#ifndef _WIN32
    if (sizeof(time_t) >= 8)
    {
        // UTC 的 10000 年需要五位年份，固定长度接口应报告格式化失败。
        CHECK(CSystem::timetToStr(static_cast<time_t>(INT64_C(253402300800)), false).empty());
    }
#endif
    std::vector<std::string> arguments;
    char program[] = "CSystemTest";
    char* sample[] = {program};
    CHECK(CSystem::commandArguments(1, sample, arguments));
    CHECK(!CSystem::commandArguments(-1, nullptr, arguments));
    std::chrono::high_resolution_clock::time_point a = CSystem::GetHighTickCount();
    const auto b = a + std::chrono::seconds(2);
    CHECK(CSystem::GetMilliRunTime(b, a) == 2000 && CSystem::GetMicroRunTime(b, a) == 2000000);
    CHECK(CSystem::GetNanoRunTime(b, a) == 2000000000LL);
    return 0;
}

#if CSYSTEM_HAS_NATIVE_API
/** 在本次独占文件上核对系统资源、目录配置和文件操作，成功返回 0。 */
static int TestNative()
{
    CHECK(!CSystem::setProcessDirectory("relative-path"));
#ifdef CSYSTEM_SHARED_IDENTITY
    const char* parent = std::getenv("CSYSTEM_TEST_DIR");
    if (parent == nullptr) { parent = std::getenv("TMPDIR"); }
    CHECK(parent != nullptr);
    uint64_t unavailable = 123;
    CHECK(!CSystem::processIdentity(CSystem::currentProcessPid(), unavailable) && unavailable == 0);
    const std::string identityDirectory = CSystem::joinPath(parent, "identity-" + CSystem::uuid(0));
    CHECK(CSystem::CreateDir(identityDirectory));
    const auto cleanIdentity = [&identityDirectory](const std::string*) {
        CSystem::deleteFile(CSystem::joinPath(identityDirectory, "m_P2I" + std::to_string(CSystem::currentProcessPid())).c_str());
        CSystem::DestroyDir(identityDirectory);
    };
    const std::unique_ptr<const std::string, decltype(cleanIdentity)> ownedDirectory(&identityDirectory, cleanIdentity);
    CHECK(CSystem::setProcessDirectory(identityDirectory));
#else
    CHECK(CSystem::setProcessDirectory(""));
#endif
    uint64_t identity = 0, repeated = 0;
    CHECK(!CSystem::processIdentity(0, identity) && identity == 0);
    CHECK(CSystem::processIdentity(CSystem::currentProcessPid(), identity) && identity != 0);
    CHECK(CSystem::processIdentity(CSystem::currentProcessPid(), repeated) && repeated == identity);
#ifdef CSYSTEM_SHARED_IDENTITY
    CHECK(CSystem::setProcessDirectory(identityDirectory) && !CSystem::setProcessDirectory(""));
    const std::string marker = CSystem::readFile(CSystem::joinPath(identityDirectory, "m_P2I" + std::to_string(CSystem::currentProcessPid())));
    CHECK(marker.size() == sizeof(identity) && std::memcmp(marker.data(), &identity, sizeof(identity)) == 0);
#endif
    CHECK(CSystem::currentProcessPid() != 0 && CSystem::SystemThreadId() != 0 && CSystem::GetCPUCoreCount() > 0);
#if !defined(__APPLE__) || !TARGET_OS_IPHONE
    const uint32_t self = CSystem::currentProcessPid();
    const std::string selfName = CSystem::processName(self);
    CHECK(!selfName.empty() && selfName.find('\0') == std::string::npos);
    const std::vector<uint32_t> matches = CSystem::processPid(selfName);
    CHECK(std::find(matches.begin(), matches.end(), self) != matches.end());
    CHECK(CSystem::processFirstPid(selfName) != 0 && CSystem::processPid("").empty());
    CHECK(CSystem::processPid(selfName + std::string("\0suffix", 7)).empty());
    CHECK(CSystem::processFirstPid("CSystemTest invalid/name") == 0 && CSystem::processName(UINT32_MAX).empty());
#endif
    CHECK(!CSystem::secureRandomBytes(nullptr, 1) && CSystem::secureRandomBytes(nullptr, 0));
    uint8_t random[34] = {};
    random[0] = random[33] = 123;
    CHECK(CSystem::secureRandomBytes(random + 1, 32) && random[0] == 123 && random[33] == 123);
    CHECK(CSystem::secureRandomBytesCallback(nullptr, nullptr, 0) == 0 && CSystem::secureRandomBytesCallback(nullptr, nullptr, 1) != 0);
    CHECK(!CSystem::secureRandomBytes(random + 1, 8193) && random[0] == 123 && random[33] == 123);
    CHECK(!CSystem::uuid().empty());
    const std::string compactId = CSystem::uuid(0);
    const std::string dashedId = CSystem::uuid(1);
    CHECK(compactId.size() == 32 && compactId.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos);
    CHECK(dashedId.size() == 36 && dashedId[8] == '-' && dashedId[13] == '-' && dashedId[18] == '-' && dashedId[23] == '-');
    CHECK(CSystem::GetSystemBits() == 32 || CSystem::GetSystemBits() == 64);
#if CSYSTEM_HAS_DESKTOP_API
#ifdef _WIN32
    const DWORD scheduling = ::GetPriorityClass(::GetCurrentProcess());
    CHECK(CSystem::GetConsoleHwnd() == ::GetConsoleWindow());
#endif
    const double frequency = CSystem::GetCPUSpeedGHz();
    CHECK(frequency >= 0.0 && frequency < 1000.0);
#ifdef _WIN32
    CHECK(scheduling != 0 && ::GetPriorityClass(::GetCurrentProcess()) == scheduling);
    DWORD mhz = 0, valueBytes = sizeof(mhz);
    if (::RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
        L"~MHz", RRF_RT_REG_DWORD, nullptr, &mhz, &valueBytes) == ERROR_SUCCESS)
    {
        CHECK(frequency == mhz / 1000.0);
    }
#endif
#endif
    CHECK(CSystem::GetEnvironment(nullptr).empty());
    CHECK(CSystem::canonicalPathUtf8(std::string("a\0b", 3), false).empty());
    const char* scratch = std::getenv("CSYSTEM_TEST_DIR");
    if (scratch == nullptr) { scratch = std::getenv("TMPDIR"); }
    const std::string directory = scratch == nullptr ? "." : scratch;
    const std::string path = CSystem::joinPath(directory, "csystem-test-" + CSystem::uuid(0) + ".bin");
    CSystemTestFile file(path);
    CHECK(file.m_handle != -1 && CSystem::plainFileUtf8(path));
    CHECK(CSystem::openFileHandle(path.c_str(), 2) == -1);
    const uint8_t content[] = {0x42, 0, 0x80, 0x7f, 0xff};
    CHECK(CSystem::writeFileHandle(file.m_handle, 0, content, sizeof(content)) && CSystem::flushFileHandle(file.m_handle));
    uint64_t length = 0;
    CHECK(CSystem::fileHandleSize(file.m_handle, &length) && length == sizeof(content));
    file.m_duplicate = CSystem::duplicateFileHandle(file.m_handle);
    CHECK(file.m_duplicate != -1);
    uint8_t bytes[9] = {};
    uint32_t received = 99;
    CHECK(CSystem::readFileHandle(file.m_duplicate, 1, bytes, sizeof(bytes), &received) && received == 4 &&
        std::memcmp(bytes, content + 1, 4) == 0);
    CHECK(CSystem::readFileHandle(file.m_handle, sizeof(content), bytes, sizeof(bytes), &received) && received == 0);
    CHECK(CSystem::resizeFileHandle(file.m_handle, 2));
    CHECK(CSystem::fileHandleSize(file.m_duplicate, &length) && length == 2);
    CHECK(CSystem::closeFileHandle(&file.m_duplicate) && file.m_duplicate == -1);
    CHECK(CSystem::closeFileHandle(&file.m_duplicate));
    CHECK(CSystem::closeFileHandle(&file.m_handle));
    const std::wstring widePath = sizeof(wchar_t) == 2 ?
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(path) :
        std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(path);
    CHECK(CSystem::fileExists(widePath));
    const std::wstring invalidWide = widePath + std::wstring(L"\0suffix", 7);
    CHECK(!CSystem::fileExists(invalidWide) && !CSystem::deleteFile(invalidWide));
    CHECK(!CSystem::replaceFile(invalidWide, widePath + L".absent") && !CSystem::replaceFile(widePath, invalidWide));
    std::string writeError;
    CHECK(CSystem::openBinaryOutputFile(invalidWide, &writeError) == nullptr && !writeError.empty());
    CHECK(!CSystem::rename(path + std::string("\0suffix", 7), path + ".absent") &&
        !CSystem::rename(path, path + std::string("\0suffix", 7)));
    std::vector<uint8_t> saved(1, 0xa5);
    CHECK(!CSystem::readBinaryFile(widePath, 1, saved) && saved.size() == 1 && saved[0] == 0xa5);
    CHECK(CSystem::readBinaryFile(widePath, 2, saved) && saved.size() == 2 && saved[0] == 0x42 && saved[1] == 0);
    CHECK(CSystem::readFile(path) == std::string("B\0", 2));
    CHECK(CSystem::readFile(path + std::string("\0suffix", 7)).empty());
    CHECK(CSystem::isPathWithin(path, directory) && !CSystem::isPathWithin(directory, directory));
    uint64_t stamp = 0;
    CHECK(CSystem::fileStampUtf8(path, &length, &stamp) && length == 2);
    std::string blocks(32771, '\0');
    for (size_t i = 0; i < blocks.size(); ++i) { blocks[i] = static_cast<char>(i % 251); }
    CSystem::saveFile(blocks, path);
    CHECK(CSystem::readFile(path) == blocks);
#if CSYSTEM_HAS_DESKTOP_API
    // 无效路径不访问桌面或修改文件。
    CSystem::OpenFolder(std::string("\0directory", 10));
    CSystem::OpenFolderAndSelectFile(std::string("\0selection", 10));
    CSystem::OpenFile(std::string("\0file", 5));
#endif
    CSystem::saveFile("sentinel", path + std::string("\0suffix", 7));
    CHECK(CSystem::readFile(path) == blocks);
    CHECK(CSystem::deleteFile(path.c_str()) && !CSystem::plainFileUtf8(path));
    file.m_path.clear();
#ifdef _WIN32
    uint64_t monitor = CSystem::openProcessMonitor(CSystem::currentProcessPid());
    CHECK(monitor != 0);
    uint64_t commit = 0, resident = 0, cpu = 0;
    uint32_t handles = 0, threads = 0;
    int32_t exitCode = -1;
    const bool sampled = CSystem::queryProcessResources(0, commit, resident, cpu, handles, threads);
    const int32_t status = CSystem::pollProcessMonitor(monitor, exitCode);
    CSystem::closeProcessMonitor(monitor);
    CHECK(sampled && commit > 0 && resident > 0 && handles > 0 && threads > 0);
    CHECK(status == 0 && exitCode == 0 && monitor == 0 && CSystem::pollProcessMonitor(0, exitCode) == -1);
#endif
    return 0;
}

/** 验证公共等待及 Windows 通用计时器分支会等待指定时间，成功返回 0。 */
static int TestSleep(bool strict)
{
    CHECK(CheckSleep(CSystem::sleepPrecise, strict));
#ifdef _WIN32
    CHECK(CheckSleep(CSystemFallbackWait, strict));
#endif
    return 0;
}
#endif

/** 核对线程及等级合法性，并从系统观察实际设置，成功返回 0。 */
static int TestPriority()
{
    int32_t nativeError = -1;
    std::thread absentThread;
    CHECK(CSystem::setThreadPriority(absentThread, 0, nativeError) == CSystemThreadPriorityInvalid && nativeError == 0);
#if CSYSTEM_HAS_NATIVE_API
    std::atomic<bool> done(false);
    std::thread worker([&done]() { while (!done.load()) { std::this_thread::yield(); } });
    const bool rejects = CSystem::setThreadPriority(worker, 3, nativeError) == CSystemThreadPriorityInvalid && nativeError == 0 &&
        CSystem::setThreadPriority(worker, -3, nativeError) == CSystemThreadPriorityInvalid && nativeError == 0;
    bool changed = true;
#ifdef _WIN32
    const int expected[] = {THREAD_PRIORITY_IDLE, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL,
        THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST};
    for (int32_t level = -2; level <= 2; ++level)
    {
        changed = changed && CSystem::setThreadPriority(worker, level, nativeError) == CSystemThreadPriorityApplied &&
            nativeError == 0 && ::GetThreadPriority(worker.native_handle()) == expected[level + 2];
    }
#else
    changed = CSystem::setThreadPriority(worker, -1, nativeError) == CSystemThreadPriorityUnsupported && nativeError == 0;
#endif
    const auto applied = CSystem::setThreadPriority(worker, 0, nativeError);
#ifdef _WIN32
    const bool observed = ::GetThreadPriority(worker.native_handle()) == THREAD_PRIORITY_NORMAL;
#else
    const bool observed = nativeError == 0;
#endif
    done.store(true);
    worker.join();
    CHECK(rejects && changed && applied == CSystemThreadPriorityApplied && observed);
#if defined(__linux__) && !defined(__ANDROID__)
    CHECK(CSystem::readFile("/proc/self/status").find("Name:") != std::string::npos);
#endif
#endif
    return 0;
}

#if CSYSTEM_HAS_NATIVE_API && (!defined(__APPLE__) || !TARGET_OS_IPHONE)
/** 验证短输出在子进程结束后完整交付，并保留真实的非零退出状态。 */
static int TestCommand()
{
    std::string output("unchanged");
    CHECK(CSystem::SystemCommand("", output) == -1);
    CHECK(CSystem::SystemCommand(std::string("unused\0suffix", 13), output) == -1);
#ifdef _WIN32
    CHECK(CSystem::SystemCommand("cmd.exe /d /c exit 7", output, false) == 7 && output.empty());
    CHECK(CSystem::SystemCommand("echo marker", output, true) == 0 && output.find("marker") != std::string::npos);
    output = "cmd.exe /d /c echo alias";
    CHECK(CSystem::SystemCommand(output, output) == 0 && output == "alias\r\n");
    std::string first, second;
    int firstStatus = -1;
    std::thread parallel([&]() { firstStatus = CSystem::SystemCommand("cmd.exe /d /c echo first", first); });
    const int secondStatus = CSystem::SystemCommand("cmd.exe /d /c echo second", second);
    parallel.join();
    CHECK(firstStatus == 0 && secondStatus == 0 && first == "first\r\n" && second == "second\r\n");
#else
    CHECK(CSystem::SystemCommand("printf marker; exit 7", output) == 7 && output == "marker");
    output = "printf 'A\\000B'";
    CHECK(CSystem::SystemCommand(output, output) == 0 && output == std::string("A\0B", 3));
#endif
    return 0;
}
#endif

int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return RunBenchmarks();
    }
#if CSYSTEM_HAS_NATIVE_API
    if (argc == 2 && std::strcmp(argv[1], "--precise-sleep") == 0) { return TestSleep(true); }
#endif
#if CSYSTEM_HAS_DESKTOP_API
    if (argc == 2 && std::strcmp(argv[1], "--console-line") == 0)
    {
        // 此入口由父进程从标准输入提供两行，第二行只有一个大写 X。
        CSystem::ClearScanf();
        if (std::getchar() != 'X') { return 1; }
        CSystem::ClearScanf();
        CSystem::ClearScanf();
        return std::feof(stdin) ? 0 : 1;
    }
#endif
    const LibraryTestCase cases[] = {
        {1, false, TestCommon},
#if CSYSTEM_HAS_NATIVE_API
        {2, false, TestNative},
#endif
        {3, false, RunSystemInterfaceTests},
        {4, false, TestPriority},
#if CSYSTEM_HAS_NATIVE_API
        {5, false, []() -> int { return TestSleep(false); }},
        {6, false, RunSystemEntropyTests},
        {7, true, RunSystemConcurrentTests},
#endif
#if CSYSTEM_HAS_DESKTOP_API
        {8, false, []() -> int { return SystemLeaseAudit() ? 0 : 1; }},
#endif
#if CSYSTEM_HAS_NATIVE_API && (!defined(__APPLE__) || !TARGET_OS_IPHONE)
        {9, false, TestCommand},
#endif
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}