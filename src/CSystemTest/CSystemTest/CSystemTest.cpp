#include "CSystemTest.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <exception>
#include <cstring>
#include <algorithm>
#include <memory>
#if CSYSTEM_HAS_NATIVE_API
#include <codecvt>
#include <locale>
#ifdef _WIN32
#include <shellapi.h>
#else
#include <pthread.h>
#include <sched.h>
#endif
#endif

/** 运行本工程的接口性能用例
@return 输入和结果满足各用例约定时返回0
*/
static int RunBenchmarks()
{
    const std::string dirs[] = {"project/media/recordings/session-alpha", "project/media/recordings/session-bravo/"};
    const std::string file = "session-recording.2026.final.flac";
    const std::string paths[] = {dirs[0] + "/" + file, dirs[1] + file};
    for (size_t index = 0; index < 2; ++index)
    {
        if (CSystem::GetName(paths[index], 1) != file || CSystem::GetName(paths[index], 2) != "flac" ||
            CSystem::joinPath(dirs[index], file) != paths[index])
        {
            return 1;
        }
    }
    bool valid = true;
    for (int32_t mode = 1; mode <= 2; ++mode)
    {
        LibraryBenchmark::run(mode == 1 ? "CSystem.GetName.filename" : "CSystem.GetName.extension", [&](uint32_t count) -> uint64_t {
            uint64_t sum = 0;
            for (uint32_t i = 0; i < count; ++i)
            {
                sum += CSystem::GetName(paths[i & 1], mode).size();
            }
            const uint64_t length = mode == 1 ? file.size() : 4;
            valid = valid && sum == count * length;
            return sum;
        });
    }
    LibraryBenchmark::run("CSystem.joinPath", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            sum += CSystem::joinPath(dirs[i & 1], file).size();
        }
        const uint64_t expected = uint64_t(count / 2) * (paths[0].size() + paths[1].size()) +
            uint64_t(count % 2) * paths[0].size();
        valid = valid && sum == expected;
        return sum;
    });
    return valid ? 0 : 1;
}

#define CHECK(expression) do { if (!(expression)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expression); return 1; } } while (false)
#if CSYSTEM_HAS_NATIVE_API
std::string TestPathUtf8(const std::string& path)
{
#ifdef _WIN32
    const int size = ::MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, nullptr, 0);
    if (size <= 0)
    {
        return "";
    }
    std::vector<wchar_t> wide(size);
    if (!::MultiByteToWideChar(CP_ACP, 0, path.c_str(), -1, wide.data(), size))
    {
        return "";
    }
    return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(wide.data());
#else
    return path;
#endif
}

CSystemTestFile::CSystemTestFile(const std::string& path) :
    m_path(path), m_handle(CSystem::openFileHandle(TestPathUtf8(path).c_str(), 2)), m_duplicate(-1)
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
// GNU Windows 测试翻译单元由 CMake 允许访问真实的私有回退入口。
#define CSystemFallbackWait CSystem::sleepWithTimerResolution
#endif
#endif

#if CSYSTEM_HAS_NATIVE_API
/** 按整组累计时长核对 20 次短等待；严格模式另检查累计时长上界
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
    const bool independentRows = matrix[0][2] == 19 && matrix[1][2] == 23;
    CSystem::DestroyDyadicArray(matrix, 2);
    CHECK(independentRows);
    CSystem::DestroyDyadicArray<int>(nullptr, 0);
    CSystemTestRect r = {-3, 2, 7, -1};
    r = CSystem::rectValid(r);
    CHECK(r.left == 0 && r.top == 2 && r.right == 7 && r.bottom == 0);
    const CSystemTestQtRect qtRect = {-3, 2, 7, -1};
    const CSystemTestQtRect clamped = CSystem::qrectValid(qtRect);
    CHECK(clamped.left() == 0 && clamped.top() == 2 && clamped.right() == 7 && clamped.bottom() == 0);
    CHECK(qtRect.left() == -3 && qtRect.bottom() == -1);
    const CSystemTestRect otherEdges = {5, -7, -2, 9};
    const CSystemTestRect otherResult = CSystem::rectValid(otherEdges);
    CHECK(otherResult.left == 5 && otherResult.top == 0 && otherResult.right == 0 && otherResult.bottom == 9);
    const CSystemTestQtRect otherQt = {5, -7, -2, 9};
    const CSystemTestQtRect otherQtResult = CSystem::qrectValid(otherQt);
    CHECK(otherQtResult.left() == 5 && otherQtResult.top() == 0 && otherQtResult.right() == 0 && otherQtResult.bottom() == 9);
    const std::string leaf = "\xe6\x96\x87.tar.Gz";
    const std::string directory = "top.dot/sub\\";
    CHECK(CSystem::GetName(directory + leaf, 1) == leaf);
    CHECK(CSystem::GetName(directory + leaf, 2) == "Gz");
    CHECK(CSystem::GetName(directory + leaf, 3) == "\xe6\x96\x87.tar");
    CHECK(CSystem::GetName(directory + leaf, 4) == directory);
    CHECK(CSystem::GetName(".config", 2) == "config");
    CHECK(CSystem::GetName("", 1).empty() && CSystem::GetName(leaf, 5).empty());
    CHECK(CSystem::joinPath("top.dot/sub", leaf) == "top.dot/sub/" + leaf);
    CHECK(CSystem::joinPath(directory, leaf) == directory + leaf);
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
#ifdef _WIN32
    const std::vector<std::string> actual = CSystem::exeParam();
    CHECK(!actual.empty() && static_cast<int>(actual.size()) == __argc);
    for (int index = 0; index < __argc; ++index)
    {
        CHECK(actual[index] == __argv[index]);
    }
    int count = 0;
    std::unique_ptr<wchar_t*, decltype(&::LocalFree)> systemArgs(::CommandLineToArgvW(::GetCommandLineW(), &count), ::LocalFree);
    CHECK(systemArgs && count > 0 && arguments.size() == static_cast<size_t>(count - 1));
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utf8;
    for (int index = 1; index < count; ++index)
    {
        CHECK(arguments[index - 1] == utf8.to_bytes(systemArgs.get()[index]));
    }
#else
    CHECK(arguments.empty());
    char empty[] = "";
    char spaced[] = "two words";
    char* supplied[] = {program, empty, spaced};
    CHECK(CSystem::commandArguments(3, supplied, arguments));
    CHECK(arguments.size() == 2 && arguments[0].empty() && arguments[1] == spaced);
    supplied[1] = nullptr;
    CHECK(!CSystem::commandArguments(3, supplied, arguments) && arguments.size() == 2 &&
        arguments[0].empty() && arguments[1] == spaced);
#endif
    const std::vector<std::string> savedArguments = arguments;
    CHECK(!CSystem::commandArguments(-1, nullptr, arguments) && arguments == savedArguments);
    CHECK(!CSystem::commandArguments(4097, sample, arguments) && arguments == savedArguments);
    std::chrono::high_resolution_clock::time_point a = CSystem::GetHighTickCount();
    const auto b = a + std::chrono::seconds(2);
    CHECK(CSystem::GetMilliRunTime(b, a) == 2000 && CSystem::GetMicroRunTime(b, a) == 2000000);
    CHECK(CSystem::GetNanoRunTime(b, a) == 2000000000LL);
    CHECK(CSystem::GetMilliRunTime(a, b) == -2000 && CSystem::GetMicroRunTime(a, b) == -2000000);
    CHECK(CSystem::GetNanoRunTime(a, b) == -2000000000LL && CSystem::GetNanoRunTime(a, a) == 0);
    // 给起点预留一秒，使不同单位可区分；上界取自这组三次读取之后的同一时钟。
    const auto start = std::chrono::high_resolution_clock::now() - std::chrono::seconds(1);
    const int32_t millis = CSystem::GetHighTickCountMilliRunTime(start);
    const int32_t micros = CSystem::GetHighTickCountMicroRunTime(start);
    const int64_t nanos = CSystem::GetHighTickCountNanoRunTime(start);
    const auto elapsed = std::chrono::high_resolution_clock::now() - start;
    CHECK(millis >= 1000 && millis <= std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
    CHECK(micros >= 1000000 && micros <= std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
    CHECK(nanos >= 1000000000 && nanos <= std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());
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
    const std::string identityUtf8 = TestPathUtf8(identityDirectory);
    CHECK(CSystem::setProcessDirectory(identityUtf8));
#else
    CHECK(CSystem::setProcessDirectory(""));
#endif
    uint64_t identity = 0, repeated = 0;
    CHECK(!CSystem::processIdentity(0, identity) && identity == 0);
    CHECK(CSystem::processIdentity(CSystem::currentProcessPid(), identity) && identity != 0);
    CHECK(CSystem::processIdentity(CSystem::currentProcessPid(), repeated) && repeated == identity);
#ifdef CSYSTEM_SHARED_IDENTITY
    CHECK(CSystem::setProcessDirectory(identityUtf8) && !CSystem::setProcessDirectory(""));
    const std::string marker = CSystem::readFile(CSystem::joinPath(identityDirectory, "m_P2I" + std::to_string(CSystem::currentProcessPid())));
    CHECK(marker.size() == sizeof(identity) && std::memcmp(marker.data(), &identity, sizeof(identity)) == 0);
#endif
    CHECK(CSystem::currentProcessPid() != 0 && CSystem::SystemThreadId() != 0);
    CHECK(!CSystem::processDefinitelyExited(CSystem::currentProcessPid()) &&
        !CSystem::processDefinitelyExited(0) && !CSystem::processDefinitelyExited(UINT32_MAX));
#ifdef _WIN32
    SYSTEM_INFO systemInfo = {};
    ::GetSystemInfo(&systemInfo);
    CHECK(CSystem::GetCPUCoreCount() == static_cast<int32_t>(systemInfo.dwNumberOfProcessors));
    typedef BOOL (WINAPI *QueryMachine)(HANDLE, USHORT*, USHORT*);
    const QueryMachine queryMachine = reinterpret_cast<QueryMachine>(
        ::GetProcAddress(::GetModuleHandleW(L"kernel32.dll"), "IsWow64Process2"));
    if (queryMachine != nullptr)
    {
        USHORT process = 0, machine = 0;
        CHECK(queryMachine(::GetCurrentProcess(), &process, &machine));
        // 0xaa64 是旧 SDK 未定义的 IMAGE_FILE_MACHINE_ARM64。
        const bool native64 = machine == IMAGE_FILE_MACHINE_AMD64 || machine == IMAGE_FILE_MACHINE_IA64 || machine == 0xaa64;
        CHECK(CSystem::GetSystemBits() == (native64 ? 64 : 32));
    }
    else
    {
        BOOL emulated = FALSE;
        CHECK(::IsWow64Process(::GetCurrentProcess(), &emulated));
        CHECK(CSystem::GetSystemBits() == (sizeof(void*) == 8 || emulated ? 64 : 32));
    }
#else
    CHECK(CSystem::GetSystemBits() == static_cast<int>(sizeof(void*) * CHAR_BIT));
    CHECK(CSystem::GetCPUCoreCount() == static_cast<int32_t>(std::thread::hardware_concurrency()));
#endif
#if !defined(__APPLE__) || !TARGET_OS_IPHONE
    const uint32_t self = CSystem::currentProcessPid();
    const std::string selfName = CSystem::processName(self);
    CHECK(!selfName.empty() && selfName.find('\0') == std::string::npos);
#if defined(__APPLE__)
    const std::string executablePath = CSystem::currentExecutablePathUtf8();
    CHECK(!executablePath.empty() && selfName == executablePath.substr(executablePath.rfind('/') + 1));
#endif
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
    // 为完整请求和两端哨兵分配空间，库误接收超限长度时也能正常报告失败。
    std::vector<uint8_t> overLimit(8195, 123);
    CHECK(!CSystem::secureRandomBytes(overLimit.data() + 1, 8193));
    CHECK(std::count(overLimit.begin(), overLimit.end(), static_cast<uint8_t>(123)) == 8195);
    CHECK(!CSystem::uuid().empty());
    const std::string compactId = CSystem::uuid(0);
    const std::string dashedId = CSystem::uuid(1);
    CHECK(compactId.size() == 32 && compactId.find_first_not_of("0123456789abcdef") == std::string::npos);
    CHECK(dashedId.size() == 36 && dashedId[8] == '-' && dashedId[13] == '-' && dashedId[18] == '-' && dashedId[23] == '-');
    std::string digits = dashedId;
    digits.erase(std::remove(digits.begin(), digits.end(), '-'), digits.end());
    CHECK(digits.size() == 32 && digits.find_first_not_of("0123456789abcdef") == std::string::npos);
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
    const std::string utf8Path = TestPathUtf8(path);
    CSystemTestFile file(path);
    CHECK(file.m_handle != -1 && CSystem::plainFileUtf8(utf8Path));
    uint32_t error = 0;
    CHECK(CSystem::openFileHandle(utf8Path.c_str(), 2, &error) == -1 && error != 0);
    CHECK(CSystem::openFileHandle(utf8Path.c_str(), 0, &error) == -1 && error != 0);
    CHECK(CSystem::openFileHandle(utf8Path.c_str(), 4, &error) == -1 && error != 0);
    const uint8_t content[] = {0x42, 0, 0x80, 0x7f, 0xff};
    CHECK(CSystem::writeFileHandle(file.m_handle, 0, content, sizeof(content)) && CSystem::flushFileHandle(file.m_handle));
    uint64_t length = 0;
    CHECK(CSystem::fileHandleSize(file.m_handle, &length) && length == sizeof(content));
    file.m_duplicate = CSystem::duplicateFileHandle(file.m_handle);
    CHECK(file.m_duplicate != -1);
    uint8_t bytes[9] = {};
    uint32_t received = 99;
    CHECK(!CSystem::readFileHandle(file.m_handle, UINT64_MAX, bytes, 1, &received, &error) && received == 0 && error != 0);
    CHECK(!CSystem::writeFileHandle(file.m_handle, UINT64_MAX, content, 1, &error) && error != 0);
    CHECK(CSystem::readFileHandle(file.m_handle, 0, nullptr, 0, &received, &error) && received == 0 && error == 0);
    CHECK(CSystem::readFileHandle(file.m_duplicate, 1, bytes, sizeof(bytes), &received) && received == 4 &&
        std::memcmp(bytes, content + 1, 4) == 0);
    CHECK(CSystem::readFileHandle(file.m_handle, sizeof(content), bytes, sizeof(bytes), &received) && received == 0);
    CHECK(CSystem::resizeFileHandle(file.m_handle, 2));
    CHECK(CSystem::fileHandleSize(file.m_duplicate, &length) && length == 2);
    CHECK(CSystem::closeFileHandle(&file.m_handle) && file.m_handle == -1);
    CHECK(CSystem::readFileHandle(file.m_duplicate, 0, bytes, sizeof(bytes), &received, &error) &&
        received == 2 && error == 0 && std::memcmp(bytes, content, 2) == 0);
    CHECK(CSystem::closeFileHandle(&file.m_duplicate) && file.m_duplicate == -1);
    CHECK(CSystem::closeFileHandle(&file.m_duplicate));
    CHECK(CSystem::closeFileHandle(&file.m_handle));
    const std::wstring widePath = sizeof(wchar_t) == 2 ?
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(utf8Path) :
        std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(utf8Path);
    CHECK(CSystem::fileExists(widePath));
    const std::wstring invalidWide = widePath + std::wstring(L"\0suffix", 7);
    CHECK(!CSystem::fileExists(invalidWide) && !CSystem::deleteFile(invalidWide));
    CHECK(!CSystem::replaceFile(invalidWide, widePath + L".absent") && !CSystem::replaceFile(widePath, invalidWide));
    std::string writeError;
    CHECK(CSystem::openBinaryOutputFile(invalidWide, &writeError) == nullptr && !writeError.empty());
    writeError.clear();
    CHECK(CSystem::openPrivateBinaryOutputFile(widePath, &writeError) == nullptr && !writeError.empty());
    writeError.clear();
    CHECK(CSystem::openPrivateBinaryOutputFile(invalidWide, &writeError) == nullptr && !writeError.empty());
    CHECK(!CSystem::rename(path + std::string("\0suffix", 7), path + ".absent") &&
        !CSystem::rename(path, path + std::string("\0suffix", 7)));
    std::vector<uint8_t> saved(1, 0xa5);
    CHECK(!CSystem::readBinaryFile(widePath, 1, saved) && saved.size() == 1 && saved[0] == 0xa5);
    CHECK(CSystem::readBinaryFile(widePath, 2, saved) && saved.size() == 2 && saved[0] == 0x42 && saved[1] == 0);
    CHECK(CSystem::readFile(path) == std::string("B\0", 2));
    CHECK(CSystem::readFile(path + std::string("\0suffix", 7)).empty());
    CHECK(CSystem::isPathWithin(path, directory) && !CSystem::isPathWithin(directory, directory));
    uint64_t stamp = 123;
    length = 123;
    CHECK(!CSystem::fileStampUtf8(utf8Path, &length, nullptr) && length == 0);
    CHECK(!CSystem::fileStampUtf8(utf8Path, nullptr, &stamp) && stamp == 0);
    stamp = 123;
    CHECK(!CSystem::fileStampUtf8(utf8Path, &stamp, &stamp) && stamp == 0);
    length = stamp = 123;
    CHECK(!CSystem::fileStampUtf8(TestPathUtf8(directory), &length, &stamp) && length == 0 && stamp == 0);
    CHECK(!CSystem::fileStampUtf8(utf8Path + std::string("\0tail", 5), &length, &stamp));
    CHECK(CSystem::fileStampUtf8(utf8Path, &length, &stamp) && length == 2);
    std::string blocks(32771, '\0');
    for (size_t i = 0; i < blocks.size(); ++i) { blocks[i] = static_cast<char>(i % 251); }
    CSystem::saveFile(blocks, path);
    CHECK(CSystem::readFile(path) == blocks);
    file.m_handle = CSystem::openFileHandle(utf8Path.c_str(), 1, &error);
    CHECK(file.m_handle != -1 && error == 0);
    const size_t middle = blocks.size() / 2;
    CHECK(!CSystem::writeFileHandle(file.m_handle, middle, content, sizeof(content), &error) && error != 0);
    CHECK(CSystem::closeFileHandle(&file.m_handle) && CSystem::readFile(path) == blocks);
    file.m_handle = CSystem::openFileHandle(utf8Path.c_str(), 3, &error);
    CHECK(file.m_handle != -1 && error == 0);
    CHECK(CSystem::fileHandleSize(file.m_handle, &length) && length == blocks.size());
    CHECK(CSystem::writeFileHandle(file.m_handle, middle, content, sizeof(content), &error) && error == 0);
    CHECK(CSystem::closeFileHandle(&file.m_handle));
    blocks.replace(middle, sizeof(content), reinterpret_cast<const char*>(content), sizeof(content));
    CHECK(CSystem::readFile(path) == blocks);
#if CSYSTEM_HAS_DESKTOP_API
    // 无效路径不访问桌面或修改文件。
    CSystem::OpenFolder(std::string("\0directory", 10));
    CSystem::OpenFolderAndSelectFile(std::string("\0selection", 10));
    CSystem::OpenFile(std::string("\0file", 5));
#endif
    CSystem::saveFile("sentinel", path + std::string("\0suffix", 7));
    CHECK(CSystem::readFile(path) == blocks);
    CHECK(CSystem::deleteFile(path.c_str()) && !CSystem::plainFileUtf8(utf8Path));
    file.m_path.clear();
    file.m_handle = CSystem::createObservedOutputFile(utf8Path.c_str(), &error);
    CHECK(file.m_handle != -1 && error == 0);
    file.m_path = path;
    CHECK(CSystem::writeFileHandle(file.m_handle, 0, content, 2));
    CHECK(CSystem::readBinaryFile(widePath, 2, saved) && saved == std::vector<uint8_t>(content, content + 2));
    CHECK(CSystem::writeFileHandle(file.m_handle, 2, content + 2, 3));
    CHECK(CSystem::readBinaryFile(widePath, 5, saved) && saved == std::vector<uint8_t>(content, content + 5));
    CHECK(CSystem::createObservedOutputFile(utf8Path.c_str(), &error) == -1 && error != 0);
    CHECK(CSystem::closeFileHandle(&file.m_handle));
    CSystemTestFile draft(path + ".draft");
    CHECK(draft.m_handle != -1 && CSystem::writeFileHandle(draft.m_handle, 0, content + 2, 3));
    CHECK(CSystem::flushFileHandle(draft.m_handle) && CSystem::closeFileHandle(&draft.m_handle));
    const std::wstring wideDraft = widePath + L".draft";
    CHECK(!CSystem::installFileIfAbsent(wideDraft, widePath));
    CHECK(CSystem::readBinaryFile(wideDraft, 3, saved) && saved == std::vector<uint8_t>(content + 2, content + 5));
    CHECK(CSystem::readBinaryFile(widePath, 5, saved) && saved == std::vector<uint8_t>(content, content + 5));
    CHECK(CSystem::replaceFile(wideDraft, widePath) && !CSystem::fileExists(wideDraft));
    CHECK(!CSystem::replaceFile(wideDraft, widePath));
    CHECK(CSystem::readBinaryFile(widePath, 3, saved) && saved == std::vector<uint8_t>(content + 2, content + 5));
    CHECK(CSystem::installFileIfAbsent(widePath, wideDraft));
    CHECK(CSystem::readBinaryFile(wideDraft, 3, saved) && saved == std::vector<uint8_t>(content + 2, content + 5));
    CHECK(!CSystem::writeBinaryOutputFile(nullptr, content, 1) && !CSystem::flushBinaryOutputFile(nullptr) &&
        !CSystem::closeBinaryOutputFile(nullptr));
    writeError = "previous error";
    std::unique_ptr<FILE, decltype(&CSystem::closeBinaryOutputFile)> stream(
        CSystem::openBinaryOutputFile(wideDraft, &writeError), &CSystem::closeBinaryOutputFile);
    CHECK(stream && writeError.empty());
    CHECK(!CSystem::writeBinaryOutputFile(stream.get(), nullptr, 1));
    CHECK(CSystem::writeBinaryOutputFile(stream.get(), content, 0));
    CHECK(CSystem::writeBinaryOutputFile(stream.get(), content, 2) && CSystem::flushBinaryOutputFile(stream.get()));
    CHECK(CSystem::closeBinaryOutputFile(stream.release()));
    CHECK(CSystem::readFile(draft.m_path) == std::string("B\0", 2));
    CHECK(CSystem::deleteFile(wideDraft));
    writeError = "previous error";
    stream.reset(CSystem::openPrivateBinaryOutputFile(wideDraft, &writeError));
    CHECK(stream && writeError.empty());
    CHECK(CSystem::writeBinaryOutputFile(stream.get(), content, sizeof(content)) && CSystem::flushBinaryOutputFile(stream.get()));
    CHECK(CSystem::closeBinaryOutputFile(stream.release()));
    CHECK(CSystem::readFile(draft.m_path) == std::string(reinterpret_cast<const char*>(content), sizeof(content)));
#ifdef _WIN32
    // 在专用线程中依次进入 MTA 和 STA。重复初始化的返回值可验证库没有消耗宿主的引用。
    CHECK(RunSystemWorkers(1, [](size_t) -> bool {
        const DWORD models[] = {COINIT_MULTITHREADED, COINIT_APARTMENTTHREADED};
        for (size_t i = 0; i < 2; ++i)
        {
            const HRESULT first = ::CoInitializeEx(nullptr, models[i]);
            if (FAILED(first))
            {
                return false;
            }
            const auto release = [](const DWORD*) { ::CoUninitialize(); };
            std::unique_ptr<const DWORD, decltype(release)> apartment(&models[i], release);
            CSystem::allCameraName();
            const HRESULT second = ::CoInitializeEx(nullptr, models[i]);
            if (SUCCEEDED(second))
            {
                ::CoUninitialize();
            }
            if (first != S_OK || second != S_FALSE)
            {
                return false;
            }
        }
        return true;
    }));
    const auto devices = CSystem::allDeviceInfo();
    for (const auto& group : devices)
    {
        CHECK(!group.first.empty() && !group.second.empty());
        for (const std::string& id : group.second)
        {
            CHECK(id.substr(0, id.find('\\')) == group.first);
        }
    }
    CHECK(CSystem::GetVisibleHeight() == ::GetSystemMetrics(SM_CYFULLSCREEN));
    const int width = ::GetSystemMetrics(SM_CXSCREEN);
    const int height = ::GetSystemMetrics(SM_CYSCREEN);
    const RECT bounds = CSystem::GetWindowResolution();
    const POINT center = CSystem::screenCenterPoint();
    CHECK(bounds.left == 0 && bounds.top == 0 && bounds.right == width && bounds.bottom == height);
    CHECK(center.x == width / 2 && center.y == height / 2);
    std::vector<char> modulePath(32768);
    const DWORD copied = ::GetModuleFileNameA(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
    CHECK(copied > 0 && copied < modulePath.size());
    const std::string executable(modulePath.data(), copied);
    const std::string baseName = executable.substr(executable.find_last_of("\\/") + 1);
    CHECK(CSystem::processName(::GetCurrentProcessId()) == baseName);
    std::vector<wchar_t> wideModule(32768);
    const DWORD wideLength = ::GetModuleFileNameW(nullptr, wideModule.data(), static_cast<DWORD>(wideModule.size()));
    CHECK(wideLength > 0 && wideLength < wideModule.size());
    const std::wstring wideExecutable(wideModule.data(), wideLength);
    const std::wstring wideBase = wideExecutable.substr(wideExecutable.find_last_of(L"\\/") + 1);
    CHECK(CSystem::processNameW(::GetCurrentProcessId()) == wideBase);
    const std::vector<uint32_t> wideMatches = CSystem::processPid(wideBase);
    CHECK(std::find(wideMatches.begin(), wideMatches.end(), ::GetCurrentProcessId()) != wideMatches.end());
    CHECK(CSystem::processFirstPid(wideBase) != 0);
    const std::vector<std::string> modules = CSystem::processMountDll(::GetCurrentProcessId());
    CHECK(std::find(modules.begin(), modules.end(), baseName) != modules.end());
    uint64_t monitor = CSystem::openProcessMonitor(CSystem::currentProcessPid());
    CHECK(monitor != 0);
    uint64_t commit = 0, resident = 0, cpu = 0;
    uint32_t handles = 0, threads = 0;
    int32_t exitCode = -1;
    const bool sampled = CSystem::queryProcessResources(0, commit, resident, cpu, handles, threads);
    const bool sampledSelf = sampled && commit > 0 && resident > 0 && handles > 0 && threads > 0;
    const bool sampledHandle = CSystem::queryProcessResources(monitor, commit, resident, cpu, handles, threads);
    const int32_t status = CSystem::pollProcessMonitor(monitor, exitCode);
    CSystem::closeProcessMonitor(monitor);
    CHECK(sampledSelf && sampledHandle && commit > 0 && resident > 0 && handles > 0 && threads > 0);
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
    const bool matched = applied == CSystemThreadPriorityApplied && nativeError == 0 &&
        ::GetThreadPriority(worker.native_handle()) == THREAD_PRIORITY_NORMAL;
#else
    int observedPolicy = -1;
    struct sched_param observedParameters = {};
    const bool observed = ::pthread_getschedparam(worker.native_handle(), &observedPolicy, &observedParameters) == 0 &&
        observedPolicy == SCHED_OTHER && observedParameters.sched_priority == 0;
    // 先观察库设置后的实际状态，再独立向系统请求同一设置，防止参考调用掩盖库的遗漏。
    struct sched_param requested = {};
    const int reference = ::pthread_setschedparam(worker.native_handle(), SCHED_OTHER, &requested);
    const bool matched = reference == 0 ? applied == CSystemThreadPriorityApplied && nativeError == 0 && observed :
        applied == CSystemThreadPriorityFailed && nativeError == reference;
#endif
    done.store(true);
    worker.join();
    CHECK(rejects && changed && matched);
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
    const bool parallel = RunSystemWorkers(2, [](size_t index) -> bool {
        const char* marker = index == 0 ? "first" : "second";
        std::string output;
        return CSystem::SystemCommand(std::string("cmd.exe /d /c echo ") + marker, output) == 0 &&
            output == std::string(marker) + "\r\n";
    });
    CHECK(parallel);
#else
    CHECK(CSystem::SystemCommand("printf marker; exit 7", output) == 7 && output == "marker");
    output = "printf 'A\\000B'";
    CHECK(CSystem::SystemCommand(output, output) == 0 && output == std::string("A\0B", 3));
#endif
    return 0;
}
#endif

#ifdef _WIN32
/** 等待本用例创建的短命子进程，核对 PID 和退出状态并关闭监视器
@param [in] pid 启动接口交付的子进程号
@param [in,out] monitor 本用例拥有的进程句柄，返回前释放
@param [in] expected 子进程约定退出码
@return 十秒内退出且身份及退出码一致时为 true
*/
static bool CheckChildExit(uint32_t pid, uint64_t& monitor, int32_t expected)
{
    HANDLE process = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(monitor));
    int32_t code = -1;
    const bool correct = monitor != 0 && pid != 0 && ::GetProcessId(process) == pid &&
        ::WaitForSingleObject(process, 10000) == WAIT_OBJECT_0 &&
        CSystem::pollProcessMonitor(monitor, code) == 1 && code == expected && CSystem::processDefinitelyExited(pid);
    CSystem::closeProcessMonitor(monitor);
    return correct && monitor == 0;
}

/** 核对从系统宽字符命令行读出的参数和重定向的两个标准流
@param [in] argc 子进程参数数量
@param [in] argv C 运行库参数
@return 数据完整时返回供父进程判断的 23，失败返回 1
*/
static int CapturedChild(int argc, char** argv)
{
    std::vector<std::string> args;
    CHECK(CSystem::commandArguments(argc, argv, args) && args.size() == 7);
    const bool receivedArguments = args[0] == "--csystem-captured" && args[1].empty() && args[2] == "two words" &&
        args[3] == "\"quoted\"" && args[4] == "trail\\" && args[5] == "a\\\"b" &&
        args[6] == "\xe4\xb8\xad\xe6\x96\x87";
    CHECK(receivedArguments);
    CHECK(CSystem::bindInheritedOutput());
    CHECK(std::fputs("stdout|", stdout) >= 0 && std::fflush(stdout) == 0);
    CHECK(std::fputs("stderr|", stderr) >= 0 && std::fflush(stderr) == 0);

    // 先只改 Win32 标准句柄，再从独立管道观察库自己 CRT 的输出是否真的改向。
    HANDLE reader = nullptr, writer = nullptr;
    CHECK(::CreatePipe(&reader, &writer, nullptr, 0));
    std::unique_ptr<void, decltype(&::CloseHandle)> readEnd(reader, ::CloseHandle), writeEnd(writer, ::CloseHandle);
    CHECK(::SetStdHandle(STD_OUTPUT_HANDLE, writer) && CSystem::bindInheritedOutput());
    CSystem::OutputVector(std::vector<std::string>(1, "library-pipe"));
    // 再次绑定会关闭并刷新此前绑定的库内流。先检查可读量，错误时直接退出。
    CHECK(CSystem::bindInheritedOutput());
    char buffer[32] = {};
    DWORD available = 0, received = 0;
    CHECK(::PeekNamedPipe(reader, nullptr, 0, nullptr, &available, nullptr) && available > 0 && available <= sizeof(buffer));
    CHECK(::ReadFile(reader, buffer, available, &received, nullptr) && received == available);
    const std::string observed(buffer, received);
    CHECK(observed == "library-pipe\n" || observed == "library-pipe\r\n");
    return 23;
}

/** 核对捕获及分离启动、参数传递、日志排他性和进程退出
@return 所有子进程及日志检查通过时返回 0
*/
static int TestChildProcesses()
{
    const wchar_t* scratch = ::_wgetenv(L"CSYSTEM_TEST_DIR");
    if (scratch == nullptr)
    {
        scratch = ::_wgetenv(L"TEMP");
    }
    const std::string unique = CSystem::uuid(0);
    CHECK(!unique.empty());
    const std::wstring root = std::wstring(scratch == nullptr ? L"." : scratch) + L"\\CSystem Child " +
        std::wstring(unique.begin(), unique.end());
    const std::wstring log = root + L"\\\u65e5\u5fd7.bin";
    const std::string output = CSystem::canonicalPathUtf8(std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(log), false);
    CHECK(!output.empty() && ::CreateDirectoryW(root.c_str(), nullptr));
    // 本次排他创建目录成功后，只清理该目录及用例指定的输出文件。
    const auto cleanup = [&root, &log](const std::wstring*) {
        ::DeleteFileW(log.c_str());
        ::RemoveDirectoryW(root.c_str());
    };
    std::unique_ptr<const std::wstring, decltype(cleanup)> owned(&root, cleanup);
    // U+FFFD 是合法文件名字符，损坏的 UTF-8 不得命中同一文件。
    const std::wstring named = root + L"\\\ufffd.bin";
    const std::string utf8Root = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(root);
    const std::string invalidName = utf8Root + "\\\xff.bin";
    const std::string validName = utf8Root + "\\\xef\xbf\xbd.bin";
    FILE* fixture = CSystem::openPrivateBinaryOutputFile(named);
    CHECK(fixture != nullptr);
    const auto removeFixture = [](const std::wstring* path) { ::DeleteFileW(path->c_str()); };
    std::unique_ptr<const std::wstring, decltype(removeFixture)> ownedFixture(&named, removeFixture);
    CHECK(CSystem::closeBinaryOutputFile(fixture) && CSystem::plainFileUtf8(validName));
    uint32_t error = 0;
    intptr_t opened = CSystem::openFileHandle(invalidName.c_str(), 1, &error);
    const bool refused = opened == -1 && error == ERROR_NO_UNICODE_TRANSLATION;
    CSystem::closeFileHandle(&opened);
    CHECK(refused && CSystem::canonicalPathUtf8(invalidName, false).empty() && !CSystem::plainFileUtf8(invalidName));
    CHECK(!CSystem::canonicalPathUtf8(validName).empty() && CSystem::plainFileUtf8(validName));

    const std::string exe = CSystem::currentExecutablePathUtf8();
    uint32_t pid = 1;
    uint64_t monitor = 1;
    CHECK(CSystem::startDetachedProcess("relative.exe", {}, pid, monitor) != 0 && pid == 0 && monitor == 0);
    CHECK(CSystem::startCapturedProcess(exe, std::vector<std::string>(65), output, pid, monitor) != 0 &&
        pid == 0 && monitor == 0 && !CSystem::fileExists(log));
    const std::vector<std::string> args = {"--csystem-captured", "", "two words", "\"quoted\"", "trail\\", "a\\\"b", "\xe4\xb8\xad\xe6\x96\x87"};
    CHECK(CSystem::startCapturedProcess(exe, args, output, pid, monitor) == 0);
    CHECK(CheckChildExit(pid, monitor, 23));
    CHECK(CSystem::startCapturedProcess(exe, args, output, pid, monitor) != 0 && pid == 0 && monitor == 0);
    std::vector<uint8_t> data;
    CHECK(CSystem::readBinaryFile(log, 64, data) && std::string(data.begin(), data.end()) == "stdout|stderr|");
    CHECK(CSystem::startDetachedProcess(exe, {"--csystem-detached"}, pid, monitor) == 0);
    CHECK(CheckChildExit(pid, monitor, 37));
    return 0;
}
#endif

int main(int argc, char** argv) try
{
#ifdef _WIN32
    if (argc >= 2 && std::strcmp(argv[1], "--csystem-captured") == 0)
    {
        return CapturedChild(argc, argv);
    }
    if (argc == 2 && std::strcmp(argv[1], "--csystem-detached") == 0)
    {
        return 37;
    }
#endif
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
        if (CSystem::inputString("") != "X")
        {
            return 1;
        }
        CSystem::ClearScanf();
        return 0;
    }
    if (argc == 2 && std::strcmp(argv[1], "--console-reader") == 0)
    {
        // 先从应用自己的 CRT 预读，再让库通过同一读取函数清行。
        if (std::getchar() == EOF)
        {
            return 1;
        }
        CSystem::ClearScanf(nullptr);
        CSystem::ClearScanf(&std::getchar);
        if (std::getchar() != 'X')
        {
            return 1;
        }
        const int endOfLine = std::getchar();
        if (endOfLine != '\n' && endOfLine != EOF)
        {
            return 1;
        }
        CSystem::ClearScanf(&std::getchar);
        return std::feof(stdin) ? 0 : 1;
    }
#endif
    const CSystemCase cases[] = {
        {CSystemCaseCommon, false, "纯工具", "数组回滚、路径、时间单位和参数符合独立预期", TestCommon},
#if CSYSTEM_HAS_NATIVE_API
        {CSystemCaseNative, false, "原生系统与文件", "进程位数、系统资源、文件内容和句柄状态与独立观察一致", TestNative},
#endif
        {CSystemCaseInterfaces, false, "接口组合", "目录枚举、改名、复制和迁移保留预期字节及属性", RunSystemInterfaceTests},
        {CSystemCasePriority, false, "线程调度", "等级及系统回读结果一致，未生效时返回对应状态", TestPriority},
#if CSYSTEM_HAS_NATIVE_API
        {CSystemCaseSleep, false, "短等待", "预热后的累计等待达到下界", []() -> int { return TestSleep(false); }},
        {CSystemCaseEntropy, false, "系统熵源", "有效请求成功，越界请求保留完整缓冲区", RunSystemEntropyTests},
        {CSystemCaseConcurrent, true, "并发身份", "八个线程的身份稳定且独立，全部 UUID 互异", RunSystemConcurrentTests},
#endif
#if CSYSTEM_HAS_DESKTOP_API
        {CSystemCaseLease, false, "进程租约", "竞争、就绪、释放与重试符合租约状态", []() -> int { return SystemLeaseAudit() ? 0 : 1; }},
#endif
#if CSYSTEM_HAS_NATIVE_API && (!defined(__APPLE__) || !TARGET_OS_IPHONE)
        {CSystemCaseCommand, false, "命令执行", "输出字节和退出码与子进程约定一致", TestCommand},
#endif
#ifdef _WIN32
        {CSystemCaseChild, false, "子进程与日志", "启动参数、排他日志、输出字节和监视退出码一致", TestChildProcesses},
#endif
    };
    unsigned previous = 0;
    for (const auto& item : cases)
    {
        if (item.id <= previous || item.name == nullptr || *item.name == 0 ||
            item.expected == nullptr || *item.expected == 0 || item.run == nullptr)
        {
            std::fputs("Invalid CSystem test registration\n", stderr);
            return 1;
        }
        previous = item.id;
    }
    // 平台调度器选中用例后先输出中文场景和预期，再返回真实入口的退出码。
    return LibraryCaseSelection::run(argc, argv, cases, [](const CSystemCase& item) -> int {
        std::printf("[%u] %s - %s\n", static_cast<unsigned>(item.id), item.name, item.expected);
        return item.run();
    });
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "CSystemTest failed: %s\n", error.what());
    return 1;
}
catch (...)
{
    std::fputs("CSystemTest failed with an unexpected exception\n", stderr);
    return 1;
}