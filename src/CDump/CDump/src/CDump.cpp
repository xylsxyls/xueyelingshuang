#include "CDump.h"
#ifdef _WIN32
#include "mdump.h"
#elif defined(__linux__) && defined(__GLIBC__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
#define CDUMP_LINUX_BACKEND
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#endif

#if defined(_WIN32) || defined(CDUMP_LINUX_BACKEND)
#include <mutex>
// 串行提交注册，失败不消耗首次成功的机会
static std::mutex s_registrationMutex;
static bool s_registered = false;
#endif

#ifdef CDUMP_LINUX_BACKEND
// 安装前填好绝对路径前缀，信号处理时只读
static char s_prefix[PATH_MAX] = {};

/** 向文件写完一个连续字节段，中断后继续
@param [in] fd 本次日志的文件描述符
@param [in] data 输入字节
@param [in] size 剩余字节数
@return 是否完整写入
*/
static bool WriteBytes(int fd, const char* data, size_t size)
{
    while (size != 0)
    {
        const ssize_t count = write(fd, data, size);
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        if (count <= 0)
        {
            return false;
        }
        data += count;
        size -= static_cast<size_t>(count);
    }
    return true;
}

/** 将非负整数追加到调用者的缓冲区，不依赖区域设置或堆分配
@param [in,out] out 至少还有 32 字节的写入位置
@param [in] value 无符号数值
@param [in] base 十进制或十六进制
@return 下一可写位置
*/
static char* AppendNumber(char* out, uint64_t value, unsigned base)
{
    char reverse[32];
    size_t size = 0;
    do
    {
        reverse[size++] = "0123456789abcdef"[value % base];
        value /= base;
    } while (value != 0);
    while (size != 0)
    {
        *out++ = reverse[--size];
    }
    return out;
}

/** 写信号、回溯地址和当时的模块映射，符号解析留给离线诊断
@param [in] number 本次收到的 SIGSEGV 或 SIGABRT
*/
static void DumpSignal(int number)
{
    const int savedError = errno;
    char path[PATH_MAX];
    const size_t prefixSize = strlen(s_prefix);
    memcpy(path, s_prefix, prefixSize);
    char* tail = AppendNumber(path + prefixSize, static_cast<uint64_t>(getpid()), 10);
    *tail++ = '_';
    tail = AppendNumber(tail, static_cast<uint64_t>(time(nullptr)), 10);
    *tail++ = '_';
    tail = AppendNumber(tail, static_cast<unsigned>(number), 10);
    memcpy(tail, ".log", 5);
    const int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (fd >= 0)
    {
        char line[96] = "signalNum = ";
        char* end = AppendNumber(line + 12, static_cast<unsigned>(number), 10);
        *end++ = '\n';
        WriteBytes(fd, line, end - line);
        void* frames[128];
        const int count = backtrace(frames, 128);
        memcpy(line, "backtrace() returned ", 21);
        end = AppendNumber(line + 21, static_cast<unsigned>(count), 10);
        memcpy(end, " addresses\n", 11);
        WriteBytes(fd, line, end - line + 11);
        for (int i = 0; i < count; ++i)
        {
            line[0] = '#';
            end = AppendNumber(line + 1, static_cast<unsigned>(i), 10);
            memcpy(end, " 0x", 3);
            end = AppendNumber(end + 3, reinterpret_cast<uintptr_t>(frames[i]), 16);
            *end++ = '\n';
            WriteBytes(fd, line, end - line);
        }
        // 保留系统返回的模块名与符号偏移；文件及行号由匹配的符号离线解释
        if (count > 0)
        {
            backtrace_symbols_fd(frames, count, fd);
        }
        const int maps = open("/proc/self/maps", O_RDONLY | O_CLOEXEC);
        if (maps >= 0)
        {
            WriteBytes(fd, "/proc/self/maps\n", 16);
            char chunk[1024];
            ssize_t size;
            while ((size = read(maps, chunk, sizeof(chunk))) != 0)
            {
                if (size < 0 && errno == EINTR)
                {
                    continue;
                }
                if (size < 0 || !WriteBytes(fd, chunk, static_cast<size_t>(size)))
                {
                    break;
                }
            }
            close(maps);
        }
        close(fd);
    }
    errno = savedError;
}

/** 冻结输出目录、预热回溯并安装两个一次性信号处理器
@param [in] directory 原生路径字节，空串选择可执行文件目录
@return 两个处理器均安装成功时返回 true
*/
static bool InstallLinuxDump(const std::string& directory)
{
    char image[PATH_MAX];
    const ssize_t size = readlink("/proc/self/exe", image, sizeof(image));
    if (size <= 0 || static_cast<size_t>(size) >= sizeof(image))
    {
        return false;
    }
    const std::string exe(image, static_cast<size_t>(size));
    const size_t slash = exe.rfind('/');
    if (slash == std::string::npos)
    {
        return false;
    }
    const std::string dir = directory.empty() ? exe.substr(0, slash + 1) : directory;
    char canonical[PATH_MAX];
    struct stat info = {};
    if (!realpath(dir.c_str(), canonical) || stat(canonical, &info) != 0 ||
        !S_ISDIR(info.st_mode))
    {
        return false;
    }
    const std::string prefix = std::string(canonical) + "/core-file_" + exe.substr(slash + 1) + "_";
    // PID、时间、信号号、两个下划线及扩展名均须能完整追加
    if (prefix.size() + 64 > sizeof(s_prefix) || exe.size() - slash - 1 + 74 > NAME_MAX)
    {
        return false;
    }
    memcpy(s_prefix, prefix.c_str(), prefix.size() + 1);
    void* frame;
    backtrace(&frame, 1);
    struct sigaction action = {};
    struct sigaction previous = {};
    action.sa_handler = DumpSignal;
    action.sa_flags = SA_RESETHAND;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGABRT);
    sigaddset(&action.sa_mask, SIGSEGV);
    if (sigaction(SIGSEGV, &action, &previous) != 0)
    {
        return false;
    }
    if (sigaction(SIGABRT, &action, nullptr) != 0)
    {
        sigaction(SIGSEGV, &previous, nullptr);
        return false;
    }
    return true;
}
#endif

bool CDump::declareDumpFile(const std::string& dumpFileDir)
{
#if defined(_WIN32) || defined(CDUMP_LINUX_BACKEND)
    std::lock_guard<std::mutex> lock(s_registrationMutex);
    if (s_registered || dumpFileDir.find('\0') != std::string::npos)
    {
        return false;
    }
#ifdef _WIN32
    s_registered = InstallWindowsDump(dumpFileDir);
#else
    s_registered = InstallLinuxDump(dumpFileDir);
#endif
    return s_registered;
#else
    (void)dumpFileDir;
    return false;
#endif
}

// 用法示例：宿主启动时注册一次，记录诊断是否可用，然后继续启动业务
// #include <cstdio>
// int main()
// {
//     const bool installed = CDump::declareDumpFile(".");
//     std::puts(installed ? "CDump registered" : "CDump registration unavailable");
//     return 0;
// }