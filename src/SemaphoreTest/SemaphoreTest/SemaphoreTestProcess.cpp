#include "SemaphoreTestProcess.h"
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#include <wincrypt.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <spawn.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
extern char** environ;
#endif

/** 为Windows命令行保留参数中的反斜杠和引号
@param [in] argument 单个参数
@return 带引号的参数文本
*/
static std::string QuoteArgument(const std::string& argument)
{
    std::string quoted = "\"";
    size_t slashes = 0;
    for (size_t i = 0; i < argument.size(); ++i)
    {
        const char value = argument[i];
        if (value == '\0')
        {
            throw std::invalid_argument("NUL in process argument");
        }
        if (value == '\\')
        {
            ++slashes;
            continue;
        }
        quoted.append(value == '"' ? slashes * 2 + 1 : slashes, '\\');
        quoted += value;
        slashes = 0;
    }
    quoted.append(slashes * 2, '\\');
    quoted += '"';
    return quoted;
}

double SemaphoreTestProcess::now()
{
#ifdef _WIN32
    LARGE_INTEGER value, frequency;
    if (!::QueryPerformanceCounter(&value) || !::QueryPerformanceFrequency(&frequency))
    {
        throw std::runtime_error("QPC unavailable");
    }
    return static_cast<double>(value.QuadPart) * 1000 / frequency.QuadPart;
#else
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

std::string SemaphoreTestProcess::executable()
{
    char path[32768] = {};
#ifdef _WIN32
    const DWORD length = ::GetModuleFileNameA(nullptr, path, sizeof(path));
    if (length == 0 || length >= sizeof(path))
    {
        throw std::runtime_error("Executable path unavailable");
    }
#elif defined(__APPLE__)
    uint32_t length = sizeof(path);
    if (::_NSGetExecutablePath(path, &length) != 0)
    {
        throw std::runtime_error("Executable path unavailable");
    }
#else
    const ssize_t length = ::readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (length <= 0 || static_cast<size_t>(length) >= sizeof(path) - 1)
    {
        throw std::runtime_error("Executable path unavailable");
    }
#endif
    return path;
}

bool SemaphoreTestProcess::makeDirectory(const std::string& path)
{
#ifdef _WIN32
    if (::CreateDirectoryA(path.c_str(), nullptr))
    {
        return true;
    }
    const DWORD attributes = ::GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    if (::mkdir(path.c_str(), 0700) == 0)
    {
        return true;
    }
    struct stat info = {};
    return errno == EEXIST && ::stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
#endif
}

int32_t SemaphoreTestProcess::run(
    const std::string& executable, const std::vector<std::string>& arguments, const std::string& log, uint32_t timeout)
{
#ifdef _WIN32
    std::string command = QuoteArgument(executable);
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        command += " " + QuoteArgument(arguments[i]);
    }
    std::vector<char> buffer(command.begin(), command.end());
    buffer.push_back(0);
    SECURITY_ATTRIBUTES security = {sizeof(security), nullptr, TRUE};
    HANDLE output = ::CreateFileA(
        log.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (output == INVALID_HANDLE_VALUE)
    {
        return 125;
    }
    HANDLE job = ::CreateJobObjectW(nullptr, nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (job == nullptr || !::SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)))
    {
        if (job != nullptr)
        {
            ::CloseHandle(job);
        }
        ::CloseHandle(output);
        return 125;
    }
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = output;
    startup.hStdError = output;
    PROCESS_INFORMATION process = {};
    int32_t result = 125;
    if (::CreateProcessA(executable.c_str(), &buffer[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_SUSPENDED,
            nullptr, nullptr, &startup, &process))
    {
        if (::AssignProcessToJobObject(job, process.hProcess) && ::ResumeThread(process.hThread) != DWORD(-1))
        {
            const DWORD waited = ::WaitForSingleObject(process.hProcess, timeout);
            DWORD code = 125;
            if (waited == WAIT_OBJECT_0 && ::GetExitCodeProcess(process.hProcess, &code))
            {
                result = static_cast<int32_t>(code);
            }
            else
            {
                result = waited == WAIT_TIMEOUT ? 124 : 125;
            }
        }
        if (::WaitForSingleObject(process.hProcess, 0) != WAIT_OBJECT_0)
        {
            // 仅处理本执行器创建的进程；整个作业包括它创建的IPC对端
            if (!::TerminateProcess(process.hProcess, result) ||
                ::WaitForSingleObject(process.hProcess, 5000) != WAIT_OBJECT_0)
            {
                result = 125;
            }
        }
        if (!::CloseHandle(process.hThread))
        {
            result = 125;
        }
        if (!::CloseHandle(process.hProcess))
        {
            result = 125;
        }
    }
    if (!::CloseHandle(job))
    {
        result = 125;
    }
    if (!::CloseHandle(output))
    {
        result = 125;
    }
    return result;
#else
    std::vector<char*> values;
    values.push_back(const_cast<char*>(executable.c_str()));
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        values.push_back(const_cast<char*>(arguments[i].c_str()));
    }
    values.push_back(nullptr);
    const double started = now();
    posix_spawn_file_actions_t actions;
    if (::posix_spawn_file_actions_init(&actions) != 0)
    {
        return 125;
    }
    posix_spawnattr_t attributes;
    if (::posix_spawnattr_init(&attributes) != 0)
    {
        ::posix_spawn_file_actions_destroy(&actions);
        return 125;
    }
    int32_t error =
        ::posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, log.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (error == 0)
    {
        error = ::posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);
    }
    if (error == 0)
    {
        error = ::posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP);
    }
    if (error == 0)
    {
        error = ::posix_spawnattr_setpgroup(&attributes, 0);
    }
    pid_t child = -1;
    if (error == 0)
    {
        error = ::posix_spawn(&child, executable.c_str(), &actions, &attributes, &values[0], environ);
    }
    const int32_t cleanup = ::posix_spawn_file_actions_destroy(&actions);
    const int32_t attributesCleanup = ::posix_spawnattr_destroy(&attributes);
    if (error != 0)
    {
        return 125;
    }
    for (;;)
    {
        // 先观察但不回收，保留子进程身份，清理本次进程组后才允许PID复用
        siginfo_t information = {};
        const int32_t waited = ::waitid(P_PID, child, &information, WEXITED | WNOHANG | WNOWAIT);
        if (waited < 0 && errno != EINTR)
        {
            // 观察失败也不能遗留仍归本执行器所有的子进程；ECHILD时不再触碰可能复用的PID
            if (errno != ECHILD && (::kill(-child, SIGKILL) == 0 || errno == ESRCH))
            {
                int32_t status = 0;
                while (::waitpid(child, &status, 0) < 0 && errno == EINTR)
                {

                }
            }
            return 125;
        }
        const bool ended = information.si_pid == child;
        if (ended || now() - started >= timeout)
        {
            const bool stopped = ::kill(-child, SIGKILL) == 0 || errno == ESRCH;
            int32_t status = 0;
            pid_t reaped;
            do
            {
                reaped = ::waitpid(child, &status, 0);
            } while (reaped < 0 && errno == EINTR);
            if (!stopped || reaped != child || cleanup != 0 || attributesCleanup != 0)
            {
                return 125;
            }
            return !ended ? 124 : (WIFEXITED(status) ? WEXITSTATUS(status) : 125);
        }
        struct timespec delay = {0, 10000000};
        ::nanosleep(&delay, nullptr);
    }
#endif
}

std::string SemaphoreTestProcess::hash(const std::string& path)
{
#ifdef _WIN32
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    HANDLE input = INVALID_HANDLE_VALUE;
    bool success = false;
    BYTE digest[32] = {};
    if (::CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) &&
        ::CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash))
    {
        input = ::CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (input != INVALID_HANDLE_VALUE)
        {
            BYTE buffer[16384];
            DWORD count = 0;
            while (::ReadFile(input, buffer, sizeof(buffer), &count, nullptr))
            {
                if (count == 0)
                {
                    DWORD length = sizeof(digest);
                    success =
                        ::CryptGetHashParam(hash, HP_HASHVAL, digest, &length, 0) != FALSE && length == sizeof(digest);
                    break;
                }
                if (!::CryptHashData(hash, buffer, count, 0))
                {
                    success = false;
                    break;
                }
            }
            if (count != 0)
            {
                success = false;
            }
        }
    }
    if (input != INVALID_HANDLE_VALUE && !::CloseHandle(input))
    {
        success = false;
    }
    if (hash != 0 && !::CryptDestroyHash(hash))
    {
        success = false;
    }
    if (provider != 0 && !::CryptReleaseContext(provider, 0))
    {
        success = false;
    }
    if (!success)
    {
        throw std::runtime_error("SHA256 read or cleanup failed");
    }
    const char* digits = "0123456789abcdef";
    std::string result(64, '0');
    for (size_t i = 0; i < sizeof(digest); ++i)
    {
        result[i * 2] = digits[digest[i] >> 4];
        result[i * 2 + 1] = digits[digest[i] & 15];
    }
    return result;
#else
    (void)path;
    return "NOT_AVAILABLE";
#endif
}