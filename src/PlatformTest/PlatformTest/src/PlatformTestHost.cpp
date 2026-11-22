#include "PlatformTestHost.hpp"
#ifndef PLATFORM_TEST_CMAKE
#include "PlatformTest.h"
#endif
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <random>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <errno.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

/** 将参数写作 CMake 字面量，避免引号、分号、美元符号或方括号改变参数边界
@param [in] value 一个原始参数
@return CMake 括号参数
*/
static std::string Literal(const std::string& value)
{
    std::string equals("=");
    while (value.find("]" + equals + "]") != std::string::npos)
    {
        equals += '=';
    }
    // CMake 会跳过左括号后紧邻的第一个换行，故固定提供该换行。
    return "[" + equals + "[\n" + value + "]" + equals + "]";
}

/** 直接启动 CMake，无 shell 展开；CMake 负责子进程的超时与输出重定向
@param [in] script 本次调用独占的脚本绝对路径
@return CMake 成功退出时返回 true
*/
static bool ExecuteScript(const std::string& script)
{
#ifdef PLATFORM_TEST_CMAKE
    const char* cmake = PLATFORM_TEST_CMAKE;
#else
    const char* cmake = std::getenv("PLATFORM_TEST_CMAKE");
    if (!cmake || !*cmake)
    {
        cmake = "cmake";
    }
#endif
#ifdef _WIN32
    std::string command = std::string("\"") + cmake + "\" -P \"" + script + "\"";
    std::vector<char> writable(command.begin(), command.end());
    writable.push_back('\0');
    STARTUPINFOA info = {};
    info.cb = sizeof(info);
    PROCESS_INFORMATION process = {};
    if (!CreateProcessA(nullptr, writable.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &info, &process))
    {
        return false;
    }
    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD status = 1;
    GetExitCodeProcess(process.hProcess, &status);
    CloseHandle(process.hProcess);
    return status == 0;
#else
    char* const arguments[] = {const_cast<char*>(cmake), const_cast<char*>("-P"), const_cast<char*>(script.c_str()), nullptr};
    pid_t child = 0;
    if (posix_spawnp(&child, arguments[0], nullptr, nullptr, arguments, environ) != 0)
    {
        return false;
    }
    int status = 0;
    pid_t waited = 0;
    do
    {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    return waited == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
#endif
}

PlatformTestHost::PlatformTestHost()
{
    std::random_device entropy;
    for (int attempt = 0; attempt < 16; ++attempt)
    {
        std::ostringstream token;
        // iOS 使用 NSUUID 校验标记，并按其小写规范形式发布结果文件。
        token << std::hex << std::setfill('0');
        for (unsigned i = 0; i < 16; ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
            {
                token << '-';
            }
            unsigned byte = entropy() & 255;
            if (i == 6)
            {
                byte = (byte & 15) | 64;
            }
            if (i == 8)
            {
                byte = (byte & 63) | 128;
            }
            token << std::setw(2) << byte;
        }
        m_token = token.str();
#ifdef _WIN32
        char base[MAX_PATH + 1] = {};
        DWORD length = GetTempPathA(MAX_PATH, base);
        if (length && length < MAX_PATH)
        {
            m_directory = std::string(base) + "platform-test-" + m_token;
            if (CreateDirectoryA(m_directory.c_str(), nullptr))
            {
                return;
            }
        }
#else
        const char* temp = std::getenv("TMPDIR");
        m_directory = std::string(temp && *temp ? temp : "/tmp") + "/platform-test-" + m_token;
        if (mkdir(m_directory.c_str(), 0700) == 0)
        {
            return;
        }
#endif
    }
    throw std::runtime_error("Cannot create a private temporary directory");
}

PlatformTestHost::~PlatformTestHost()
{
    std::vector<std::string> items = files(m_directory);
    for (size_t index = 0; index < items.size(); ++index)
    {
        std::remove((m_directory + "/" + items[index]).c_str());
    }
#ifdef _WIN32
    RemoveDirectoryA(m_directory.c_str());
#else
    rmdir(m_directory.c_str());
#endif
}

int PlatformTestHost::command(const std::vector<std::string>& args, int timeoutSeconds, std::string& output, bool required) const
{
    static std::atomic<unsigned> s_sequence(0);
    const std::string base = m_directory + "/process-" + std::to_string(++s_sequence);
    const std::string status = base + ".status";
    const std::string log = base + ".log";
    const std::string script = base + ".cmake";
    std::string request = "cmake_minimum_required(VERSION 3.15)\nexecute_process(COMMAND";
    for (size_t index = 0; index < args.size(); ++index)
    {
        request += " " + Literal(args[index]);
    }
    request += " TIMEOUT " + std::to_string(timeoutSeconds) + " RESULT_VARIABLE status OUTPUT_FILE " + Literal(log) + " ERROR_FILE " + Literal(log) + ")\nfile(WRITE " + Literal(status) + " \"${status}\")";
    write(script, request);
    bool executed = ExecuteScript(script);
    std::string code = read(status);
    output = read(log);
    std::remove(script.c_str());
    std::remove(status.c_str());
    std::remove(log.c_str());
    bool numeric = !code.empty() && code.find_first_not_of("0123456789") == std::string::npos;
    int result = executed && numeric ? std::atoi(code.c_str()) : 1;
    if (required && result != 0)
    {
        throw std::runtime_error(args.front() + " failed (" + code + "): " + output);
    }
    return result;
}

const std::string& PlatformTestHost::directory() const
{
    return m_directory;
}

const std::string& PlatformTestHost::token() const
{
    return m_token;
}

std::string PlatformTestHost::read(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

void PlatformTestHost::write(const std::string& path, const std::string& data)
{
    std::ofstream file(path.c_str(), std::ios::binary);
    file.write(data.data(), data.size());
    file.close();
    if (!file)
    {
        throw std::runtime_error("Cannot write temporary file " + path);
    }
}

bool PlatformTestHost::exists(const std::string& path)
{
#ifdef _WIN32
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
    struct stat attributes;
    return stat(path.c_str(), &attributes) == 0;
#endif
}

std::vector<std::string> PlatformTestHost::files(const std::string& directory)
{
    std::vector<std::string> names;
#ifdef _WIN32
    WIN32_FIND_DATAA item;
    HANDLE search = FindFirstFileA((directory + "/*").c_str(), &item);
    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                names.push_back(item.cFileName);
            }
        } while (FindNextFileA(search, &item));
        FindClose(search);
    }
#else
    DIR* entries = opendir(directory.c_str());
    if (entries)
    {
        while (dirent* item = readdir(entries))
        {
            struct stat attributes;
            if (stat((directory + "/" + item->d_name).c_str(), &attributes) == 0 && S_ISREG(attributes.st_mode))
            {
                names.push_back(item->d_name);
            }
        }
        closedir(entries);
    }
#endif
    return names;
}

#ifndef PLATFORM_TEST_CMAKE
int PlatformTest::runDesktopProcess(const std::vector<std::string>& arguments, int timeoutSeconds, std::string& output)
{
    if (arguments.empty() || arguments.front().empty() || timeoutSeconds <= 0)
    {
        throw std::invalid_argument("Expected executable and positive timeout");
    }
    PlatformTestHost host;
    return host.command(arguments, timeoutSeconds, output, false);
}
#endif