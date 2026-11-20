#include "SemaphoreTestProcess.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <cstdlib>
#include <limits.h>
#include <dlfcn.h>
#include "Semaphore.h"
#ifndef SEMAPHORE_TEST_EXTERNAL_LAUNCH
#include <spawn.h>
#endif
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#ifndef SEMAPHORE_TEST_EXTERNAL_LAUNCH
extern char** environ;
#endif
#endif

std::string SemaphoreTestProcess::library()
{
#if defined(_WIN32) && !defined(STATIC_LIB)
    // 直接询问加载器，不能用输出目录里碰巧存在的同名文件代替实际模块。
#ifdef _DEBUG
    HMODULE module = ::GetModuleHandleA("Semaphored.dll");
#else
    HMODULE module = ::GetModuleHandleA("Semaphore.dll");
#endif
    char path[32768] = {};
    const DWORD length = module == nullptr ? 0 : ::GetModuleFileNameA(module, path, sizeof(path));
    if (length == 0 || length >= sizeof(path))
    {
        throw std::runtime_error("Cannot identify loaded Semaphore DLL");
    }
    return std::string(path, length);
#elif defined(_WIN32)
    return executable();
#else
    // 静态链接时地址归属当前程序，动态链接时归属实际加载的库。
    Dl_info info = {};
    if (::dladdr(reinterpret_cast<const void*>(&Semaphore::cleanupFailures), &info) == 0 || info.dli_fname == nullptr)
    {
        throw std::runtime_error("Cannot identify loaded Semaphore image");
    }
    // 父进程可能经相对路径启动，子进程使用绝对路径。两端归一为同一实际镜像。
    char canonical[PATH_MAX];
    if (::realpath(info.dli_fname, canonical) == nullptr)
    {
        throw std::runtime_error("Cannot resolve loaded Semaphore image path");
    }
    return canonical;
#endif
}

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
    if (!::QueryPerformanceCounter(&value) || !::QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0)
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

std::string SemaphoreTestProcess::reportPath(const std::string& path)
{
#ifdef _WIN32
    if (path.empty())
    {
        return path;
    }
    // 文件调用与argv使用系统ANSI代码页，报告统一使用UTF-8。
    const int32_t wideSize = ::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, path.c_str(), -1, nullptr, 0);
    if (wideSize <= 0)
    {
        throw std::runtime_error("Cannot decode native report path");
    }
    std::vector<wchar_t> wide(wideSize);
    if (::MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, path.c_str(), -1, &wide[0], wideSize) != wideSize)
    {
        throw std::runtime_error("Cannot decode native report path");
    }
    const int32_t bytes = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &wide[0], wideSize - 1,
        nullptr, 0, nullptr, nullptr);
    if (bytes <= 0)
    {
        throw std::runtime_error("Cannot encode UTF-8 report path");
    }
    std::string result(bytes, '\0');
    if (::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &wide[0], wideSize - 1, &result[0], bytes,
        nullptr, nullptr) != bytes)
    {
        throw std::runtime_error("Cannot encode UTF-8 report path");
    }
    return result;
#else
    return path;
#endif
}

bool SemaphoreTestProcess::reserveReport(const std::string& path)
{
#ifdef _WIN32
    HANDLE file = ::CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    return file != INVALID_HANDLE_VALUE && ::CloseHandle(file) != FALSE;
#else
    const int32_t descriptor = ::open(path.c_str(), O_CREAT | O_EXCL | O_WRONLY | O_CLOEXEC, 0600);
    return descriptor >= 0 && ::close(descriptor) == 0;
#endif
}

int32_t SemaphoreTestProcess::run(
    const std::string& executable, const std::vector<std::string>& arguments, const std::string& log,
    uint32_t timeout, bool* started)
{
    if (started != nullptr)
    {
        *started = false;
    }
    // 两端都拒绝嵌入NUL，避免原生API悄悄截断实际执行的路径或参数。
    if (executable.find('\0') != std::string::npos || log.find('\0') != std::string::npos)
    {
        throw std::invalid_argument("NUL in process path");
    }
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        if (arguments[i].find('\0') != std::string::npos)
        {
            throw std::invalid_argument("NUL in process argument");
        }
    }
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
            if (started != nullptr)
            {
                *started = true;
            }
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
#elif defined(SEMAPHORE_TEST_EXTERNAL_LAUNCH)
    (void)timeout;
    return 77;
#else
    std::vector<char*> values;
    values.push_back(const_cast<char*>(executable.c_str()));
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        values.push_back(const_cast<char*>(arguments[i].c_str()));
    }
    values.push_back(nullptr);
    const double startTime = now();
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
    if (started != nullptr)
    {
        *started = true;
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
        if (ended || now() - startTime >= timeout)
        {
            const bool stopped = ::kill(-child, SIGKILL) == 0 || errno == ESRCH;
            if (!stopped && !ended)
            {
                // 终止失败时不能再无期限阻塞回收；明确报告未完成清理。
                std::fprintf(stderr, "Cannot terminate owned test process group errno=%d\n", errno);
                return 125;
            }
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

/** SHA256 对 32 位字循环右移，调用处的位数限定为 1 至 31。
@param [in] value 数据字
@param [in] count 位数
@return 右移后的字
*/
static uint32_t RotateRight(uint32_t value, uint32_t count)
{
    return (value >> count) | (value << (32 - count));
}

/** 将 64 字节消息块按大端序压入 SHA256 状态。
@param [in,out] state 八个摘要字
@param [in] block 完整消息块
*/
static void HashBlock(uint32_t* state, const uint8_t* block)
{
    static const uint32_t kRound[] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    uint32_t words[64];
    for (size_t i = 0; i < 16; ++i)
    {
        const uint8_t* p = block + i * 4;
        words[i] = (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
    }
    for (size_t i = 16; i < 64; ++i)
    {
        const uint32_t x = words[i - 15], y = words[i - 2];
        words[i] = words[i - 16] + (RotateRight(x, 7) ^ RotateRight(x, 18) ^ (x >> 3)) +
            words[i - 7] + (RotateRight(y, 17) ^ RotateRight(y, 19) ^ (y >> 10));
    }
    uint32_t w[8];
    std::memcpy(w, state, sizeof(w));
    for (size_t i = 0; i < 64; ++i)
    {
        const uint32_t e = w[4], a = w[0];
        const uint32_t first = w[7] + (RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25)) +
            ((e & w[5]) ^ (~e & w[6])) + kRound[i] + words[i];
        const uint32_t second = (RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22)) +
            ((a & w[1]) ^ (a & w[2]) ^ (w[1] & w[2]));
        for (size_t j = 7; j != 0; --j)
        {
            w[j] = w[j - 1];
        }
        w[0] = first + second;
        w[4] += first;
    }
    for (size_t i = 0; i < 8; ++i)
    {
        state[i] += w[i];
    }
}

std::string SemaphoreTestProcess::hash(const std::string& path)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input)
    {
        throw std::runtime_error("Cannot open image for SHA256");
    }
    uint32_t state[] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    uint8_t block[64];
    uint64_t bytes = 0;
    while (input.read(reinterpret_cast<char*>(block), sizeof(block)))
    {
        HashBlock(state, block);
        bytes += sizeof(block);
    }
    if (!input.eof() || input.bad())
    {
        throw std::runtime_error("SHA256 read failed");
    }
    size_t tail = static_cast<size_t>(input.gcount());
    const uint64_t bits = (bytes + tail) * 8;
    block[tail++] = 0x80;
    if (tail > 56)
    {
        std::memset(block + tail, 0, sizeof(block) - tail);
        HashBlock(state, block);
        tail = 0;
    }
    std::memset(block + tail, 0, 56 - tail);
    for (size_t i = 0; i < 8; ++i)
    {
        block[63 - i] = static_cast<uint8_t>(bits >> (8 * i));
    }
    HashBlock(state, block);
    const char* digits = "0123456789abcdef";
    std::string result(64, '0');
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = digits[(state[i / 8] >> (28 - 4 * (i % 8))) & 15];
    }
    return result;
}