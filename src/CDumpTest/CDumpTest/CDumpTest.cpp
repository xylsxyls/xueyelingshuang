#include "CDumpTest.h"
#include "CDump/CDumpAPI.h"
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__) && !defined(__EMSCRIPTEN__)
#include <chrono>
#include <cstdlib>
#include <dirent.h>
#include <iterator>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>

#define CHECK(test) do { if (!(test)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #test); return 1; } } while (false)

/** 验证当前进程的一次性注册契约
@return 成功返回 0
*/
static int RegisterOnce()
{
#if defined(_WIN32) || (defined(__linux__) && !defined(__EMSCRIPTEN__))
#if defined(__linux__)
    CHECK(!CDump::declareDumpFile(std::string(512, 'x')));
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
/** 启动本程序的独立转储子进程并在超时后回收
@param [in] directory 已存在的独占目录，无末尾分隔符
@return 子进程在规定时间内异常结束时返回 true
*/
static bool RunDumpChild(const std::string& directory)
{
    char executable[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(nullptr, executable, MAX_PATH);
    if (!length || length >= MAX_PATH)
    {
        return false;
    }
    std::string command = std::string("\"") + executable + "\" --dump-child \"" + directory + "\"";
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION child = {};
    if (!CreateProcessA(executable, &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child))
    {
        return false;
    }
    CloseHandle(child.hThread);
    const bool completed = WaitForSingleObject(child.hProcess, 20000) == WAIT_OBJECT_0;
    if (!completed)
    {
        TerminateProcess(child.hProcess, 2);
        WaitForSingleObject(child.hProcess, 5000);
    }
    DWORD code = 0;
    const bool crashed = completed && GetExitCodeProcess(child.hProcess, &code) && code != 0 && code != 2;
    CloseHandle(child.hProcess);
    return crashed;
}

/** 仅检查并回收独占目录内的转储和日志
@return 已生成可识别的转储与非空日志时返回 0
*/
static int DumpFiles()
{
    char temp[MAX_PATH] = {};
    const DWORD length = GetTempPathA(MAX_PATH, temp);
    CHECK(length && length < MAX_PATH);
    const std::string dir = std::string(temp) + "CDumpTest-" + std::to_string(GetCurrentProcessId()) +
        "-" + std::to_string(GetTickCount64());
    CHECK(CreateDirectoryA(dir.c_str(), nullptr));
    const bool crashed = RunDumpChild(dir);
    WIN32_FIND_DATAA data = {};
    HANDLE search = FindFirstFileA((dir + "\\*").c_str(), &data);
    bool dump = false, log = false, clean = true;
    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                const std::string name = data.cFileName, file = dir + "\\" + name;
                if (name.size() > 4 && name.substr(name.size() - 4) == ".dmp")
                {
                    char magic[4] = {};
                    std::ifstream input(file.c_str(), std::ios::binary);
                    dump = input.read(magic, sizeof(magic)) && std::memcmp(magic, "MDMP", 4) == 0;
                }
                if (name.size() > 4 && name.substr(name.size() - 4) == ".log")
                {
                    log = data.nFileSizeLow != 0 || data.nFileSizeHigh != 0;
                }
                clean = DeleteFileA(file.c_str()) && clean;
            }
        } while (FindNextFileA(search, &data));
        FindClose(search);
    }
    CHECK(RemoveDirectoryA(dir.c_str()) && clean && crashed && dump && log);
    return 0;
}
#elif defined(__linux__) && !defined(__EMSCRIPTEN__)
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
        execl("/proc/self/exe", "CDumpTest", "--dump-child", output.c_str(), static_cast<char*>(nullptr));
        _exit(2);
    }
    int status = 0;
    bool completed = false;
    if (child > 0)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
        do
        {
            completed = waitpid(child, &status, WNOHANG) == child;
            if (!completed)
            {
                usleep(20000);
            }
        } while (!completed && std::chrono::steady_clock::now() < deadline);
        if (!completed)
        {
            kill(child, SIGKILL);
            waitpid(child, &status, 0);
        }
    }
    bool log = false;
    DIR* entries = opendir(dir.c_str());
    if (entries)
    {
        while (dirent* entry = readdir(entries))
        {
            const std::string name = entry->d_name;
            if (name.compare(0, 10, "core-file_") == 0)
            {
                const std::string file = output + name;
                std::ifstream stream(file.c_str());
                const std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
                log = text.find("signalNum = " + std::to_string(SIGABRT)) != std::string::npos &&
                    text.find("backtrace() returned ") != std::string::npos && text.find("#0 0x") != std::string::npos;
                std::remove(file.c_str());
            }
        }
        closedir(entries);
    }
    CHECK(rmdir(dir.c_str()) == 0 && completed && WIFEXITED(status) && WEXITSTATUS(status) == 0 && log);
    return 0;
}
#endif

/** 按稳定编号运行；子进程模式只由用例 2 调用
@param [in] argc 命令行参数个数
@param [in] argv 用例选项及对应参数
@return 全部选定用例通过时返回 0，失败返回非零
*/
int main(int argc, char** argv)
{
#ifdef _WIN32
    if (argc == 3 && std::strcmp(argv[1], "--dump-child") == 0)
    {
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        if (!CDump::declareDumpFile(argv[2]))
        {
            return 2;
        }
        RaiseException(0xe0424242u, EXCEPTION_NONCONTINUABLE, 0, nullptr);
        return 2;
    }
#elif defined(__linux__) && !defined(__EMSCRIPTEN__)
    if (argc == 3 && std::strcmp(argv[1], "--dump-child") == 0)
    {
        // 单次 raise 在处理器返回后继续执行，避免为测试生成内核 core 文件。
        return CDump::declareDumpFile(argv[2]) && raise(SIGABRT) == 0 ? 0 : 2;
    }
#endif
    const LibraryTestCase cases[] = {{1, false, RegisterOnce}
#if defined(_WIN32) || (defined(__linux__) && !defined(__EMSCRIPTEN__))
        , {2, true, DumpFiles}
#endif
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}