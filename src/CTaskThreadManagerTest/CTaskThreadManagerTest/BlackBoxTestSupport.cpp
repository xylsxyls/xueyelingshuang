#include "BlackBoxTestSupport.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <thread>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

static std::atomic<int> cleanupErrors(0);
void bbCleanupFailure(const char* message)
{
    ++cleanupErrors;
    std::fprintf(stderr, "cleanup failed: %s\n", message);
}

void bbCheckCleanup()
{
    bbCheck(cleanupErrors == 0, "fixture cleanup reported an error");
}

void bbCheck(bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

uint64_t bbNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

void bbSleep(int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void bbWait(const std::function<bool()>& predicate, const char* description, int milliseconds)
{
    const uint64_t deadline = bbNow() + milliseconds;
    while (!predicate())
    {
        bbCheck(bbNow() < deadline, std::string("timeout: ") + description);
        bbSleep(1);
    }
}

std::string bbNumber(int64_t number)
{
    std::ostringstream stream;
    stream << number;
    return stream.str();
}

int bbPid()
{
#ifdef _WIN32
    return static_cast<int>(GetCurrentProcessId());
#else
    return static_cast<int>(getpid());
#endif
}

std::string bbExecutable()
{
    char buffer[32768] = {};
#ifdef _WIN32
    DWORD size = GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
    bbCheck(size > 0 && size < sizeof(buffer), "GetModuleFileName failed");
#elif defined(__APPLE__)
    uint32_t size = sizeof(buffer);
    bbCheck(_NSGetExecutablePath(buffer, &size) == 0, "executable path too long");
#else
    ssize_t size = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    bbCheck(size > 0, "read executable path failed");
    buffer[size] = 0;
#endif
    return buffer;
}

std::string bbTempPath(const std::string& suffix)
{
    static std::atomic<unsigned int> serial(0);
    std::string directory;
#ifdef _WIN32
    char buffer[32768] = {};
    DWORD size = GetTempPathA(sizeof(buffer), buffer);
    bbCheck(size > 0 && size < sizeof(buffer), "GetTempPath failed");
    directory = buffer;
#else
    const char* path = std::getenv("TMPDIR");
    directory = path ? path : "/tmp";
    directory += "/";
#endif
    return directory + "base-blackbox-" + bbNumber(bbPid()) + "-" + bbNumber(bbNow()) + "-" +
        bbNumber(serial.fetch_add(1)) + suffix;
}

std::string bbRead(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

void bbWrite(const std::string& path, const std::string& contents)
{
    std::ofstream file(path.c_str(), std::ios::binary | std::ios::trunc);
    bbCheck(file.good(), "cannot write " + path);
    file << contents;
    file.close();
    bbCheck(!file.fail(), "write failed " + path);
}

BlackBoxChild::BlackBoxChild() :
m_process(nullptr),
m_job(nullptr),
m_pid(0),
m_result(-1),
m_processGroup(false)
{

}

BlackBoxChild::~BlackBoxChild()
{
#ifdef _WIN32
    if (m_process)
    {
        if (WaitForSingleObject(m_process, 0) == WAIT_TIMEOUT)
        {
            TerminateProcess(m_process, 124);
            WaitForSingleObject(m_process, 5000);
        }
        CloseHandle(m_process);
    }
    // Job同时拥有节点再派生的进程；用例崩溃/超时也不会遗留IPC节点。
    if (m_job)
    {
        CloseHandle(m_job);
    }
#else
    if (m_pid && m_result < 0)
    {
        kill(m_processGroup ? -m_pid : m_pid, SIGKILL);
        int status = 0;
        while (waitpid(m_pid, &status, 0) < 0 && errno == EINTR)
        {

        }
    }
#endif
}

void BlackBoxChild::start(const std::vector<std::string>& arguments, const std::string& logPath)
{
    startProgram(bbExecutable(), arguments, logPath);
}

void BlackBoxChild::startProgram(const std::string& executable, const std::vector<std::string>& arguments,
    const std::string& logPath)
{
    bbCheck(!m_pid, "child already started");
#ifdef _WIN32
    // 仅通过CreateProcess参数传递，不交给命令解释器；按CRT规则转义引号和反斜线。
    std::vector<std::string> all(1, executable);
    all.insert(all.end(), arguments.begin(), arguments.end());
    std::string command;
    for (size_t i = 0; i < all.size(); ++i)
    {
        command += "\"";
        size_t slashes = 0;
        for (size_t j = 0; j < all[i].size(); ++j)
        {
            const char c = all[i][j];
            if (c == '\\')
            {
                ++slashes;
                continue;
            }
            command.append(c == '"' ? slashes * 2 + 1 : slashes, '\\');
            command += c;
            slashes = 0;
        }
        command.append(slashes * 2, '\\');
        command += "\" ";
    }
    SECURITY_ATTRIBUTES security = {sizeof(security), nullptr, TRUE};
    HANDLE log = CreateFileA(logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        &security, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    bbCheck(log != INVALID_HANDLE_VALUE, "create child log failed");
    HANDLE input = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (input == INVALID_HANDLE_VALUE)
    {
        CloseHandle(log);
        bbCheck(false, "open NUL failed");
    }
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = log;
    startup.hStdError = log;
    startup.hStdInput = input;
    PROCESS_INFORMATION process = {};
    std::vector<char> writable(command.begin(), command.end());
    writable.push_back(0);
    HANDLE job = CreateJobObjectA(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!job || !SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
    {
        if (job)
        {
            CloseHandle(job);
        }
        CloseHandle(input);
        CloseHandle(log);
        bbCheck(false, "create child cleanup job failed");
    }
    BOOL ok = CreateProcessA(executable.c_str(), &writable[0], nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, nullptr, &startup, &process);
    const DWORD error = GetLastError();
    CloseHandle(input);
    CloseHandle(log);
    if (!ok)
    {
        CloseHandle(job);
        bbCheck(false, "CreateProcess failed " + bbNumber(error));
    }
    if (!AssignProcessToJobObject(job, process.hProcess))
    {
        const DWORD jobError = GetLastError();
        TerminateProcess(process.hProcess, 126);
        WaitForSingleObject(process.hProcess, 5000);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        CloseHandle(job);
        bbCheck(false, "assign child cleanup job failed " + bbNumber(jobError));
    }
    m_job = job;
    m_process = process.hProcess;
    m_pid = static_cast<int>(process.dwProcessId);
    const DWORD resumed = ResumeThread(process.hThread);
    CloseHandle(process.hThread);
    bbCheck(resumed != static_cast<DWORD>(-1), "resume child failed");
#else
    std::vector<char*> args;
    args.push_back(const_cast<char*>(executable.c_str()));
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        args.push_back(const_cast<char*>(arguments[i].c_str()));
    }
    args.push_back(nullptr);
    int log = open(logPath.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600);
    bbCheck(log >= 0, "create child log failed");
    m_processGroup = !arguments.empty() && arguments[0] == "--case";
    pid_t child = fork();
    if (child == 0)
    {
        // 顶层用例拥有自己的组；node等后代继承此组。fork后仅调用异步信号安全函数。
        if (m_processGroup && setpgid(0, 0) != 0)
        {
            _exit(126);
        }
        if (dup2(log, STDOUT_FILENO) < 0 || dup2(log, STDERR_FILENO) < 0)
        {
            _exit(126);
        }
        close(log);
        execv(executable.c_str(), &args[0]);
        _exit(127);
    }
    close(log);
    bbCheck(child > 0, "fork failed");
    m_pid = static_cast<int>(child);
#endif
}

int BlackBoxChild::pid() const
{
    return m_pid;
}

void BlackBoxChild::stop(int32_t exitCode)
{
    if (m_pid == 0 || poll() >= 0)
    {
        return;
    }
#ifdef _WIN32
    bbCheck(TerminateProcess(m_process, static_cast<UINT>(exitCode)) != FALSE || poll() >= 0,
        "terminate owned child failed");
#else
    (void)exitCode;
    bbCheck(kill(m_processGroup ? -m_pid : m_pid, SIGKILL) == 0 || errno == ESRCH,
        "terminate owned child failed");
#endif
    wait(5000);
}

int BlackBoxChild::poll()
{
    bbCheck(m_pid != 0, "child not started");
    if (m_result >= 0)
    {
        return m_result;
    }
#ifdef _WIN32
    DWORD state = WaitForSingleObject(m_process, 0);
    bbCheck(state != WAIT_FAILED, "wait child failed");
    if (state == WAIT_TIMEOUT)
    {
        return -1;
    }
    DWORD code = 0;
    bbCheck(GetExitCodeProcess(m_process, &code) != FALSE, "get child exit failed");
    m_result = code == 0 ? 0 : (code < 256 ? static_cast<int>(code) : 125);
#else
    int status = 0;
    pid_t result = waitpid(m_pid, &status, WNOHANG);
    if (result == 0 || (result < 0 && errno == EINTR))
    {
        return -1;
    }
    bbCheck(result == m_pid, "waitpid failed");
    m_result = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    if (m_processGroup && m_result != 0)
    {
        kill(-m_pid, SIGKILL);
    }
#endif
    return m_result;
}

int BlackBoxChild::wait(int milliseconds)
{
    bbWait([this]()
    {
        return poll() >= 0;
    }, "child process", milliseconds);
    return m_result;
}

int bbRun(int argc, char** argv, const char* suite, const std::vector<BlackBoxCase>& cases)
{
    try
    {
#ifdef _WIN32
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
#endif
        bool list = false, stress = false, all = false;
        int childCase = 0;
        std::set<int> selected;
        std::string report = std::string(suite) + "-results.tsv";
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];
            if (arg == "--list")
            {
                list = true;
            }
            else if (arg == "--all")
            {
                all = true;
            }
            else if (arg == "--stress")
            {
                stress = true;
            }
            else if (arg == "--instant")
            {

            }
            else if (arg == "--report" && i + 1 < argc)
            {
                report = argv[++i];
            }
            else if (arg == "--case" && i + 1 < argc)
            {
                char extra = 0;
                bbCheck(std::sscanf(argv[++i], "%d%c", &childCase, &extra) == 1 && childCase > 0, "invalid child case ID");
            }
            else if (arg == "--tests" && i + 1 < argc)
            {
                std::string selection = argv[++i];
                bbCheck(!selection.empty() && selection[0] != ',' && selection[selection.size() - 1] != ',' &&
                    selection.find(",,") == std::string::npos && selection.find_first_not_of("0123456789,-") == std::string::npos,
                    "invalid test selection separators");
                std::replace(selection.begin(), selection.end(), ',', ' ');
                std::istringstream input(selection);
                std::string part;
                while (input >> part)
                {
                    int first = 0, last = 0;
                    char extra = 0;
                    if (std::sscanf(part.c_str(), "%d-%d%c", &first, &last, &extra) == 2)
                    {

                    }
                    else
                    {
                        bbCheck(std::sscanf(part.c_str(), "%d%c", &first, &extra) == 1, "invalid test selection");
                        last = first;
                    }
                    bbCheck(first > 0 && last >= first && last <= static_cast<int>(cases.size()), "test ID out of range");
                    for (int id = first; id <= last; ++id)
                    {
                        selected.insert(id);
                    }
                }
                bbCheck(!selected.empty(), "empty test selection");
            }
            else
            {
                throw std::runtime_error("usage: --list | --instant | --stress | --all | --tests 1,3-5 [--report file]");
            }
        }
        for (size_t i = 0; i < cases.size(); ++i)
        {
            bbCheck(cases[i].id == static_cast<int>(i + 1), "registry IDs must be contiguous");
        }
        if (childCase)
        {
            bbCheck(childCase > 0 && childCase <= static_cast<int>(cases.size()), "unknown case");
            if (cases[childCase - 1].unavailableReason != nullptr)
            {
                std::cout << "NOT_RUN " << cases[childCase - 1].unavailableReason << "\n";
                return 77;
            }
            cases[childCase - 1].run();
            bbCheckCleanup();
            std::cout << "case completed\n";
            return 0;
        }
        std::ofstream output;
        if (!list)
        {
            output.open(report.c_str(), std::ios::binary | std::ios::trunc);
            bbCheck(output.good(), "cannot create report " + report);
            output << "suite\tid\tname\tgroup\tresult\telapsed_ms\texpectation\tlog\tdetail\n";
        }
        int passed = 0, failed = 0, notRun = 0, instantCount = 0, stressCount = 0;
        std::vector<size_t> order;
        for (size_t i = 0; i < cases.size(); ++i)
        {
            order.push_back(i);
        }
        if (all && !list && selected.empty())
        {
            std::stable_sort(order.begin(), order.end(), [&cases](size_t left, size_t right)
            {
                return !cases[left].stress && cases[right].stress;
            });
        }
        for (size_t i = 0; i < cases.size(); ++i)
        {
            const BlackBoxCase& c = cases[order[i]];
            if (list)
            {
                std::cout << c.id << "\t" << c.name << "\t" << (c.stress ? "stress" : "instant") << "\t" << c.expectation << "\n";
                continue;
            }
            if (!selected.empty() ? !selected.count(c.id) : (!all && c.stress != stress))
            {
                continue;
            }
            if (c.unavailableReason != nullptr)
            {
                ++notRun;
                std::cout << "NOT_RUN " << c.id << " " << c.unavailableReason << "\n";
                output << suite << '\t' << c.id << '\t' << c.name << '\t' << (c.stress ? "stress" : "instant")
                       << "\tNOT_RUN\t0\t" << c.expectation << "\t\t" << c.unavailableReason << "\n";
                continue;
            }
            const std::string log = bbTempPath("-" + std::string(suite) + "-" + bbNumber(c.id) + ".log");
            c.stress ? ++stressCount : ++instantCount;
            const uint64_t start = bbNow();
            bool ok = false;
            std::string detail;
            try
            {
                BlackBoxChild child;
                std::vector<std::string> args;
                args.push_back("--case");
                args.push_back(bbNumber(c.id));
                child.start(args, log);
                const int result = child.wait(c.timeoutSeconds * 1000);
                ok = result == 0;
                if (!ok)
                {
                    detail = "exit=" + bbNumber(result);
                }
            }
            catch (const std::exception& error)
            {
                detail = error.what();
            }
            catch (...)
            {
                detail = "unknown runner exception";
            }
            const uint64_t elapsed = bbNow() - start;
            if (ok)
            {
                ++passed;
            }
            else
            {
                ++failed;
            }
            std::cout << (ok ? "PASS " : "FAIL ") << c.id << " " << c.name << " " << elapsed << "ms " << detail << "\n";
            std::cout.flush();
            if (!ok)
            {
                std::cout << bbRead(log) << "log=" << log << "\n";
            }
            output << suite << '\t' << c.id << '\t' << c.name << '\t' << (c.stress ? "stress" : "instant")
                   << '\t' << (ok ? "PASS" : "FAIL") << '\t' << elapsed << '\t' << c.expectation << '\t' << log << '\t' << detail << '\n';
            output.flush();
            bbCheck(output.good(), "cannot flush report");
        }
        if (!list)
        {
            std::cout << "executed=" << passed + failed << " instant=" << instantCount << " stress=" << stressCount
                      << " passed=" << passed << " failed=" << failed << " not_run=" << notRun << " report=" << report << "\n";
        }
        return failed ? 1 : (!list && passed == 0 ? 77 : 0);
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "FAIL: unknown exception\n";
        return 1;
    }
}