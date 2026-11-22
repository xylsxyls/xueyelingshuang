#include "CDumpTest.h"
#include "CDump/CDumpAPI.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <sstream>
#ifdef _WIN32
#include <thread>
#include <windows.h>
#include <dbghelp.h>
#elif defined(__linux__) && defined(__GLIBC__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <dirent.h>
#include <signal.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define CHECK(test) do { if (!(test)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #test); return 1; } } while (false)

/** 验证当前进程的一次性注册契约
@return 成功返回 0
*/
static int RegisterOnce()
{
#if defined(_WIN32) || (defined(__linux__) && defined(__GLIBC__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__))
    CHECK(!CDump::declareDumpFile(std::string(".\0ignored", 9)));
    CHECK(!CDump::declareDumpFile(std::string(512, 'x')));
#ifdef _WIN32
    char image[MAX_PATH] = {};
    const DWORD count = GetModuleFileNameA(nullptr, image, MAX_PATH);
    CHECK(count && count < MAX_PATH && !CDump::declareDumpFile(image));
#else
    CHECK(!CDump::declareDumpFile("/proc/self/exe"));
#endif
    CHECK(CDump::declareDumpFile() && !CDump::declareDumpFile("."));
#if defined(__linux__)
    struct sigaction installed = {};
    CHECK(sigaction(SIGABRT, nullptr, &installed) == 0 && installed.sa_handler != SIG_DFL && installed.sa_handler != SIG_IGN);
    CHECK(sigaction(SIGSEGV, nullptr, &installed) == 0 && installed.sa_handler != SIG_DFL && installed.sa_handler != SIG_IGN);
#endif
#else
    CHECK(!CDump::declareDumpFile() && !CDump::declareDumpFile("."));
#endif
    return 0;
}

#ifdef _WIN32
// 独占子进程用于检查转储身份的应用异常码
static const DWORD kTestException = 0xe0424242u;

/** 向系统过滤器交付一次受控异常，本地 SEH 帧负责正常收尾
@return 系统过滤器成功完成转储时返回 0
*/
static int ExerciseFilter()
{
    const ULONG_PTR threadId = GetCurrentThreadId();
    LONG disposition = EXCEPTION_CONTINUE_SEARCH;
    __try
    {
        RaiseException(kTestException, 0, 1, &threadId);
    }
    __except ((disposition = UnhandledExceptionFilter(GetExceptionInformation())), EXCEPTION_EXECUTE_HANDLER)
    {
        return disposition == EXCEPTION_EXECUTE_HANDLER ? 0 : 2;
    }
    return 2;
}

/** 在转储目录表中核对实际异常流
@param [in] path 本次子进程输出的文件
@param [in] threadId 创建子进程时由系统报告的主线程 ID
@param [in] worker 是否预期来自工作线程
@param [out] programCounter 校验成功时取得转储中故障线程的指令地址
@return 签名、版本、线程、异常码及上下文均有效时返回 true
*/
static bool CheckDump(const std::wstring& path, DWORD threadId, bool worker, DWORD64& programCounter)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    MINIDUMP_HEADER header = {};
    if (!input.read(reinterpret_cast<char*>(&header), sizeof(header)) ||
        header.Signature != MINIDUMP_SIGNATURE || LOWORD(header.Version) != MINIDUMP_VERSION)
    {
        return false;
    }
    input.seekg(header.StreamDirectoryRva);
    for (ULONG i = 0; i < header.NumberOfStreams; ++i)
    {
        MINIDUMP_DIRECTORY entry = {};
        if (!input.read(reinterpret_cast<char*>(&entry), sizeof(entry)))
        {
            return false;
        }
        if (entry.StreamType == ExceptionStream && entry.Location.DataSize >= sizeof(MINIDUMP_EXCEPTION_STREAM))
        {
            MINIDUMP_EXCEPTION_STREAM info = {};
            input.seekg(entry.Location.Rva);
            if (!input.read(reinterpret_cast<char*>(&info), sizeof(info)) ||
                (worker ? info.ThreadId == threadId : info.ThreadId != threadId) ||
                info.ExceptionRecord.ExceptionCode != kTestException || info.ExceptionRecord.NumberParameters != 1 ||
                info.ThreadId != info.ExceptionRecord.ExceptionInformation[0])
            {
                return false;
            }
            CONTEXT context = {};
#ifdef _WIN64
            const size_t required = offsetof(CONTEXT, Rip) + sizeof(context.Rip);
#else
            const size_t required = offsetof(CONTEXT, Eip) + sizeof(context.Eip);
#endif
            input.seekg(info.ThreadContext.Rva);
            if (info.ThreadContext.DataSize < required || !input.read(reinterpret_cast<char*>(&context), required))
            {
                return false;
            }
#ifdef _WIN64
            programCounter = context.Rip;
#else
            programCounter = context.Eip;
#endif
            return programCounter != 0;
        }
    }
    return false;
}

/** 启动本程序的独立转储子进程并在超时后回收
@param [in] directory 已存在的独占目录，无末尾分隔符
@param [in] worker 是否启动工作线程场景
@param [out] threadId 成功创建的子进程主线程 ID
@return 子进程在规定时间内完成过滤器调用时返回 true
*/
static bool RunDumpChild(const std::wstring& directory, bool worker, DWORD& threadId)
{
    wchar_t executable[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, executable, MAX_PATH);
    if (!length || length >= MAX_PATH)
    {
        return false;
    }
    std::wstring command = L"\"";
    command += executable;
    command += worker ? L"\" --dump-worker" : L"\" --dump-main";
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION child = {};
    if (!CreateProcessW(executable, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
        directory.c_str(), &startup, &child))
    {
        return false;
    }
    threadId = child.dwThreadId;
    CloseHandle(child.hThread);
    const bool completed = WaitForSingleObject(child.hProcess, 20000) == WAIT_OBJECT_0;
    if (!completed)
    {
        TerminateProcess(child.hProcess, 2);
        WaitForSingleObject(child.hProcess, 5000);
    }
    DWORD code = 0;
    const bool success = completed && GetExitCodeProcess(child.hProcess, &code) && code == 0;
    CloseHandle(child.hProcess);
    return success;
}

/** 核对指定线程生成的转储、文本和输出目录并回收子进程文件
@param [in] worker 是否从 Unicode 工作目录启动工作线程场景
@return 两个同名文件的内容均匹配本次异常且完成清理时返回 0
*/
static int CheckDumpFiles(bool worker)
{
    wchar_t temp[MAX_PATH] = {};
    const DWORD length = GetTempPathW(MAX_PATH, temp);
    CHECK(length && length < MAX_PATH);
    const std::wstring dir = std::wstring(temp) + (worker ? L"CDumpTest \xD83D\xDE80-" : L"CDumpTest-") +
        std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
    CHECK(CreateDirectoryW(dir.c_str(), nullptr));
    const std::wstring output = dir + L"\\output";
    if (!CreateDirectoryW(output.c_str(), nullptr))
    {
        RemoveDirectoryW(dir.c_str());
        return 1;
    }
    const std::wstring alternate = dir + L"\\alternate";
    if (!CreateDirectoryW(alternate.c_str(), nullptr))
    {
        RemoveDirectoryW(output.c_str());
        RemoveDirectoryW(dir.c_str());
        return 1;
    }
    DWORD threadId = 0;
    const bool completed = RunDumpChild(dir, worker, threadId);
    WIN32_FIND_DATAW data = {};
    HANDLE search = FindFirstFileW((output + L"\\*").c_str(), &data);
    unsigned dumps = 0, logs = 0;
    std::wstring dumpStem, logStem;
    std::string logText;
    DWORD64 programCounter = 0;
    bool validDump = true, alternateEmpty = true, clean = true;
    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                const std::wstring name = data.cFileName, file = output + L"\\" + name;
                if (name.size() > 4 && name.substr(name.size() - 4) == L".dmp")
                {
                    ++dumps;
                    dumpStem = name.substr(0, name.size() - 4);
                    validDump = CheckDump(file, threadId, worker, programCounter) && validDump;
                }
                if (name.size() > 4 && name.substr(name.size() - 4) == L".log")
                {
                    std::ifstream input(file.c_str());
                    logText.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
                    ++logs;
                    logStem = name.substr(0, name.size() - 4);
                }
                clean = DeleteFileW(file.c_str()) && clean;
            }
        } while (FindNextFileW(search, &data));
        clean = GetLastError() == ERROR_NO_MORE_FILES && clean;
        FindClose(search);
    }
    std::ostringstream firstFrame;
    firstFrame << "Call Stack:\n#0 0x" << std::hex << programCounter << '\n';

    // 子进程已经结束，此时新工作目录内出现的文件就是写错位置的证据
    search = FindFirstFileW((alternate + L"\\*").c_str(), &data);
    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                alternateEmpty = false;
                clean = DeleteFileW((alternate + L"\\" + data.cFileName).c_str()) && clean;
            }
        } while (FindNextFileW(search, &data));
        FindClose(search);
    }
    clean = RemoveDirectoryW(alternate.c_str()) && clean;
    clean = RemoveDirectoryW(output.c_str()) && clean;
    clean = RemoveDirectoryW(dir.c_str()) && clean;
    CHECK(clean);
    CHECK(completed);
    CHECK(alternateEmpty);
    CHECK(dumps == 1 && logs == 1 && dumpStem == logStem);
    CHECK(validDump);
    CHECK(logText.find(firstFrame.str()) != std::string::npos);
    CHECK(logText.find("Code: 0xe0424242\n") != std::string::npos);
    CHECK(logText.find("MiniDumpWriteDump: OK\n") != std::string::npos);
    return 0;
}

/** 验证主线程转储
@return 文件和线程身份匹配时返回 0
*/
static int DumpFiles()
{
    return CheckDumpFiles(false);
}

/** 验证工作线程转储和相对目录的固定时机
@return 文件和线程身份匹配时返回 0
*/
static int WorkerDump()
{
    return CheckDumpFiles(true);
}
#elif defined(__linux__) && defined(__GLIBC__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
/** 核对连续帧号、非零地址、声明的帧数以及首帧所属的可执行映射
@param [in] file 本次子进程生成的原生日志路径
@return 信号、回溯和映射互相匹配时返回 true
*/
static bool CheckSignalLog(const std::string& file)
{
    std::ifstream input(file.c_str());
    std::string line;
    if (!std::getline(input, line) || line != "signalNum = " + std::to_string(SIGABRT) ||
        !std::getline(input, line))
    {
        return false;
    }
    const std::string prefix = "backtrace() returned ";
    int count = 0;
    if (line.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }
    std::istringstream header(line.substr(prefix.size()));
    if (!(header >> count) || count <= 0 || count > 128 || line != prefix + std::to_string(count) + " addresses")
    {
        return false;
    }
    uintptr_t first = 0;
    for (int i = 0; i < count; ++i)
    {
        const std::string label = "#" + std::to_string(i) + " 0x";
        if (!std::getline(input, line) || line.compare(0, label.size(), label) != 0)
        {
            return false;
        }
        std::istringstream frame(line.substr(label.size()));
        uintptr_t pc = 0;
        if (!(frame >> std::hex >> pc) || !frame.eof() || pc == 0)
        {
            return false;
        }
        if (i == 0)
        {
            first = pc;
        }
    }
    while (std::getline(input, line) && line != "/proc/self/maps")
    {
        // 中间允许 glibc 输出平台相关的符号行
    }
    while (std::getline(input, line))
    {
        std::istringstream mapping(line);
        uintptr_t start = 0, end = 0;
        char dash = 0;
        std::string permissions;
        if (mapping >> std::hex >> start >> dash >> end >> permissions && dash == '-' &&
            permissions.size() >= 3 && permissions[2] == 'x' && start <= first && first < end)
        {
            return true;
        }
    }
    return false;
}

/** 在独立进程发出 SIGABRT，验证处理器生成的堆栈文本
@return 确认信号及堆栈帧且清理成功时返回 0
*/
static int DumpFiles()
{
    const char* base = std::getenv("TMPDIR");
    std::string dir = std::string(base && *base ? base : "/tmp") + "/CDumpTest-XXXXXX";
    CHECK(mkdtemp(&dir[0]));
    const std::string output = dir + "/";
    const pid_t child = fork();
    if (child == 0)
    {
        execl("/proc/self/exe", "CDumpTest", "--dump-child", dir.c_str(), static_cast<char*>(nullptr));
        _exit(2);
    }
    int status = 0;
    bool completed = false;
    if (child > 0)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
        pid_t waited;
        do
        {
            waited = waitpid(child, &status, WNOHANG);
            if (waited < 0 && errno != EINTR)
            {
                break;
            }
            completed = waited == child;
            if (!completed)
            {
                usleep(20000);
            }
        } while (!completed && std::chrono::steady_clock::now() < deadline);
        if (!completed && (waited == 0 || errno == EINTR))
        {
            kill(child, SIGKILL);
            while (waitpid(child, &status, 0) < 0 && errno == EINTR)
            {
                // 信号中断等待时继续回收本用例的子进程
            }
        }
    }
    bool valid = true;
    size_t logs = 0;
    DIR* entries = opendir(dir.c_str());
    if (entries)
    {
        while (dirent* entry = readdir(entries))
        {
            const std::string name = entry->d_name;
            if (name.compare(0, 10, "core-file_") == 0)
            {
                const std::string file = output + name;
                ++logs;
                valid = CheckSignalLog(file) && valid;
                valid = std::remove(file.c_str()) == 0 && valid;
            }
        }
        closedir(entries);
    }
    CHECK(rmdir(dir.c_str()) == 0 && completed && WIFEXITED(status) && WEXITSTATUS(status) == 0 && logs == 1 && valid);
    return 0;
}
#endif

/** 按稳定编号运行；转储用例在独立子进程触发异常或信号
@param [in] argc 命令行参数个数
@param [in] argv 用例选项及对应参数
@return 全部选定用例通过时返回 0，失败返回非零
*/
int main(int argc, char** argv)
{
#ifdef _WIN32
    if (argc == 2 && (std::strcmp(argv[1], "--dump-main") == 0 || std::strcmp(argv[1], "--dump-worker") == 0))
    {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        const bool workerCase = std::strcmp(argv[1], "--dump-worker") == 0;
        if (!CDump::declareDumpFile(workerCase ? "output\\" : "output") || !SetCurrentDirectoryW(L"alternate") ||
            CDump::declareDumpFile("."))
        {
            return 2;
        }
        if (!workerCase)
        {
            return ExerciseFilter();
        }
        int result = 2;
        // 由真实工作线程交付异常，join 后读取该次调用的返回码
        std::thread worker([&result]() { result = ExerciseFilter(); });
        worker.join();
        return result;
    }
#elif defined(__linux__) && defined(__GLIBC__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    if (argc == 3 && std::strcmp(argv[1], "--dump-child") == 0)
    {
        // 只向已安装的非默认处理器投递一次，处理器返回后正常退出
        struct sigaction installed = {};
        if (!CDump::declareDumpFile(argv[2]) || sigaction(SIGABRT, nullptr, &installed) != 0 ||
            installed.sa_handler == SIG_DFL || installed.sa_handler == SIG_IGN)
        {
            return 2;
        }
        return raise(SIGABRT) == 0 ? 0 : 2;
    }
#endif
    const LibraryTestCase cases[] = {{kRegisterOnce, false, RegisterOnce}
#if defined(_WIN32) || (defined(__linux__) && defined(__GLIBC__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__))
        , {kDumpFiles, true, DumpFiles}
#endif
#ifdef _WIN32
        , {kWorkerDump, true, WorkerDump}
#endif
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}