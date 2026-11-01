#include "CloudRebuild.h"
#include <string>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <vector>
#include <sstream>
#include <map>
#ifdef _WIN32
#include <process.h>
#define NOMINMAX
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>
#endif
#if defined(__unix__) || defined(__APPLE__)
#include <dirent.h>
#endif

#define SPACE std::string(" ")
#define MARK(src) ("\"" + src + "\"")
#ifdef _WIN32
#define PATHLEVEL std::string("\\")
#elif defined(__unix__) || defined(__APPLE__)
#define PATHLEVEL std::string("/")
#endif

#if defined(__unix__) || defined(__APPLE__)

/** 获取本机构建并行度，查询失败时至少保留一个工作进程
@return 1至4095之间的可用处理器数量
*/
static int32_t GetCPUCoreCount()
{
#ifdef _WIN32
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwNumberOfProcessors;
#elif defined(__unix__) || defined(__APPLE__)
    const long count = sysconf(_SC_NPROCESSORS_ONLN);
    return count > 0 && count < 4096 ? static_cast<int32_t>(count) : 1;
#endif
}

/** 枚举已有目录的文件名，目录不存在时保持结果不变
@param [in] dirPath 目录路径
@param [out] vec 追加目录内的文件名
@return 无
*/
static void FindDirFile(const char* dirPath,std::vector<std::string>& vec)
{
    struct dirent* pDirent = nullptr;
    DIR* pDir = opendir(dirPath);
    if (pDir == nullptr)
    {
        return;
    }
    while ((pDirent = readdir(pDir)) != NULL)
	{
        vec.push_back(std::string(pDirent->d_name));
    }
    closedir(pDir);
}
#endif

/** Normalize a command-line token for Wasm dispatch
@param [in] value Borrowed argument, or null for an empty token
@return Lowercase token
*/
static std::string WasmLower(const char* value)
{
    std::string text(value == NULL ? "" : value);
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return text;
}

#ifndef _WIN32
/** Execute one CMake step and preserve its failure status
@param [in] command Executable and arguments owned by the caller
@return Child exit status, or 127 if the process could not start
*/
static int BuildCommand(const std::vector<std::string>& command)
{
    std::vector<char*> arguments;
    for (size_t index = 0; index < command.size(); ++index)
    {
        arguments.push_back(const_cast<char*>(command[index].c_str()));
    }
    arguments.push_back(NULL);
    const pid_t process = fork();
    if (process == -1)
    {
        std::perror("Could not fork the CMake process");
        return 127;
    }
    if (process == 0)
    {
        execvp(arguments[0], &arguments[0]);
        std::perror("Could not execute CMake");
        _exit(127);
    }
    int status = 0;
    while (waitpid(process, &status, 0) == -1)
    {
        if (errno != EINTR)
        {
            return 127;
        }
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}

#endif

/** 根据真实原生目标和链接类型确定产物名称
@param [in] library 库名，不含路径
@param [in] linkage lib或dll
@param [in] mode debug或release
@return 已准备SDK中的归档或动态库文件名
*/
static std::string NativeLibraryName(const std::string& library, const std::string& linkage, const std::string& mode)
{
#ifdef _WIN32
    return library + (mode == "debug" ? "d" : "") + ".lib";
#else
    std::string suffix = ".so";
#ifdef __APPLE__
    suffix = ".dylib";
#endif
    const char* targetSystem = std::getenv("CMAKE_SYSTEM_NAME");
    if (targetSystem != nullptr && targetSystem[0] != '\0')
    {
        const std::string target(targetSystem);
        suffix = target == "Darwin" || target == "iOS" ? ".dylib" : ".so";
    }
    return "lib" + library + (mode == "debug" ? "d" : "") + (linkage == "lib" ? ".a" : suffix);
#endif
}

/** 校验项目标识，拒绝路径和命令解释字符
@param [in] value 项目名称
@return 是否为非空ASCII标识
*/
static bool ValidLibrary(const std::string& value)
{
    if (value.empty() || value.size() > 128 || !(std::isalpha(static_cast<unsigned char>(value[0])) || value[0] == '_'))
    {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](unsigned char item) { return std::isalnum(item) || item == '_'; });
}

/** 设置构建子进程环境，空值表示移除
@param [in] name 环境变量名
@param [in] value 环境变量值
@return 是否成功
*/
static bool BuildEnvironment(const char* name, const std::string& value)
{
#ifdef _WIN32
    return _putenv_s(name, value.c_str()) == 0;
#else
    return (value.empty() ? unsetenv(name) : setenv(name, value.c_str(), 1)) == 0;
#endif
}

/** 执行before并通过匿名管道读取stdout，日志转发到stderr；所有退出路径回收句柄和子进程
@param [in] command 可执行文件及独立参数，不交给命令解释器拼接
@param [out] records 仅包含带协议前缀的记录；总量最多4MiB
@return 0成功，非零为子进程退出码或管道错误
*/
static int CaptureBefore(const std::vector<std::string>& command, std::vector<std::string>& records)
{
    records.clear();
#ifdef _WIN32
    std::string line;
    for (const std::string& argument : command)
    {
        line += " \"";
        size_t slashes = 0;
        for (char value : argument)
        {
            if (value == '\\')
            {
                ++slashes;
                continue;
            }
            line.append(value == '"' ? slashes * 2 + 1 : slashes, '\\');
            slashes = 0;
            line += value;
        }
        line.append(slashes * 2, '\\');
        line += '"';
    }
    SECURITY_ATTRIBUTES security = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    HANDLE reader = nullptr;
    HANDLE writer = nullptr;
    if (!CreatePipe(&reader, &writer, &security, 0))
    {
        return 127;
    }
    if (!SetHandleInformation(reader, HANDLE_FLAG_INHERIT, 0))
    {
        CloseHandle(reader);
        CloseHandle(writer);
        return 127;
    }
    HANDLE errorReader = nullptr;
    HANDLE errorWriter = nullptr;
    if (!CreatePipe(&errorReader, &errorWriter, &security, 0) ||
        !SetHandleInformation(errorReader, HANDLE_FLAG_INHERIT, 0))
    {
        if (errorReader != nullptr) CloseHandle(errorReader);
        if (errorWriter != nullptr) CloseHandle(errorWriter);
        CloseHandle(reader);
        CloseHandle(writer);
        return 127;
    }
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdError = errorWriter;
    startup.hStdOutput = writer;
    PROCESS_INFORMATION process = {};
    std::vector<char> arguments(line.begin() + 1, line.end());
    arguments.push_back('\0');
    // 两条流均使用管道，子进程不共享或改写外层控制台的光标、换行和颜色状态。
    const BOOL started = CreateProcessA(nullptr, arguments.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
    CloseHandle(errorWriter);
    CloseHandle(writer);
    if (!started)
    {
        CloseHandle(errorReader);
        CloseHandle(reader);
        std::fprintf(stderr, "Could not start before: %lu\n", GetLastError());
        return 127;
    }
    CloseHandle(process.hThread);
#else
    int descriptors[2];
    if (pipe(descriptors) != 0)
    {
        return 127;
    }
    std::vector<char*> arguments;
    for (const std::string& argument : command)
    {
        arguments.push_back(const_cast<char*>(argument.c_str()));
    }
    arguments.push_back(nullptr);
    const pid_t process = fork();
    if (process == -1)
    {
        close(descriptors[0]);
        close(descriptors[1]);
        return 127;
    }
    if (process == 0)
    {
        close(descriptors[0]);
        if (dup2(descriptors[1], STDOUT_FILENO) == -1)
        {
            _exit(127);
        }
        close(descriptors[1]);
        execvp(arguments[0], arguments.data());
        _exit(127);
    }
    close(descriptors[1]);
#endif
    char buffer[8192];
    std::string pending;
    size_t total = 0;
    bool valid = true;
    bool truncated = false;
    const std::string prefix = "CLOUD_REBUILD_LIBRARIESRELY\t";
#ifdef _WIN32
    bool outputOpen = true;
    bool errorOpen = true;
#endif
    for (;;)
    {
#ifdef _WIN32
        DWORD errorAvailable = 0;
        if (errorOpen && !PeekNamedPipe(errorReader, nullptr, 0, nullptr, &errorAvailable, nullptr))
        {
            valid = valid && GetLastError() == ERROR_BROKEN_PIPE;
            errorOpen = false;
        }
        if (errorOpen && errorAvailable != 0)
        {
            char errors[8192];
            DWORD received = 0;
            if (!ReadFile(errorReader, errors, sizeof(errors), &received, nullptr))
            {
                valid = valid && GetLastError() == ERROR_BROKEN_PIPE;
                errorOpen = false;
            }
            else
            {
                // stderr只转发，不把其中类似协议前缀的诊断误当作依赖声明。
                // 保留原始换行，不能经CRT文本模式把每层CRLF重复扩成CRCRLF。
                std::fflush(stderr);
                DWORD offset = 0;
                while (offset < received)
                {
                    DWORD written = 0;
                    if (!WriteFile(GetStdHandle(STD_ERROR_HANDLE), errors + offset, received - offset, &written, nullptr) || written == 0)
                    {
                        valid = false;
                        break;
                    }
                    offset += written;
                }
            }
        }
        DWORD available = 0;
        if (outputOpen && !PeekNamedPipe(reader, nullptr, 0, nullptr, &available, nullptr))
        {
            valid = valid && GetLastError() == ERROR_BROKEN_PIPE;
            outputOpen = false;
        }
        DWORD count = 0;
        if (outputOpen && available != 0 && !ReadFile(reader, buffer, sizeof(buffer), &count, nullptr))
        {
            valid = valid && GetLastError() == ERROR_BROKEN_PIPE;
            outputOpen = false;
        }
        if (count == 0)
        {
            if (!outputOpen && !errorOpen) break;
            if (available == 0 && errorAvailable == 0) Sleep(1);
            continue;
        }
#else
        const ssize_t count = read(descriptors[0], buffer, sizeof(buffer));
        if (count < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            valid = false;
            break;
        }
#endif
        if (count == 0)
        {
            break;
        }
        for (size_t index = 0; index < static_cast<size_t>(count); ++index)
        {
            const char value = buffer[index];
            if (value != '\n')
            {
                if (pending.size() < 65536)
                {
                    pending += value;
                }
                else
                {
                    truncated = true;
                }
                continue;
            }
            if (!pending.empty() && pending.back() == '\r')
            {
                pending.pop_back();
            }
            if (pending.compare(0, prefix.size(), prefix) == 0)
            {
                total += pending.size();
                if (truncated || total > 4194304 || records.size() >= 4096)
                {
                    valid = false;
                }
                else
                {
                    records.push_back(pending.substr(prefix.size()));
                }
            }
            else
            {
                std::fprintf(stderr, "%s%s\n", pending.c_str(), truncated ? " [line truncated]" : "");
                std::fflush(stderr);
            }
            pending.clear();
            truncated = false;
        }
    }
    if (pending.compare(0, prefix.size(), prefix) == 0)
    {
        valid = false; // 协议记录必须以换行结束，避免接收截断声明。
    }
    else if (!pending.empty())
    {
        std::fprintf(stderr, "%s\n", pending.c_str());
    }
#ifdef _WIN32
    CloseHandle(errorReader);
    CloseHandle(reader);
    DWORD status = 127;
    if (WaitForSingleObject(process.hProcess, INFINITE) != WAIT_OBJECT_0 || !GetExitCodeProcess(process.hProcess, &status))
    {
        valid = false;
    }
    CloseHandle(process.hProcess);
#else
    close(descriptors[0]);
    int waited = 0;
    while (waitpid(process, &waited, 0) == -1)
    {
        if (errno != EINTR)
        {
            return 127;
        }
    }
    const int status = WIFEXITED(waited) ? WEXITSTATUS(waited) : 128 + WTERMSIG(waited);
#endif
    return status != 0 ? static_cast<int>(status) : valid ? 0 : 2;
}

/** 执行before的实际分支并校验其显式声明，only保留准备步骤并禁止嵌套编译
@param [in] root 仓库根目录，末尾带分隔符
@param [in] library 当前库
@param [in] bit 目标位数
@param [in] linkage 依赖链接类型
@param [in] mode 构建配置
@param [in] scope only或same
@param [out] records 依赖、第三方归档和头文件记录，不含协议前缀
@return 0成功，其他为脚本或协议错误
*/
static int CollectBefore(const std::string& root, const std::string& library, const std::string& bit,
    const std::string& linkage, const std::string& mode, const std::string& scope, std::vector<std::string>& records)
{
    const std::string path = root + "src/" + library + "/scripts/before_" + library + ".sh";
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input)
    {
        std::fprintf(stderr, "Missing before script: %s\n", path.c_str());
        return 2;
    }
    const std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    const bool declared = content.find("CLOUD_REBUILD_LIBRARIESRELY") != std::string::npos;
    const char* original = std::getenv("CLOUD_REBUILD_SKIP_DEPENDENCY_BUILDS");
    const std::string previous(original == nullptr ? "" : original);
    if (scope == "only" && !BuildEnvironment("CLOUD_REBUILD_SKIP_DEPENDENCY_BUILDS", "1"))
    {
        return 2;
    }
    const char* configured = std::getenv("CLOUD_REBUILD_BASH");
    std::string bash(configured == nullptr || configured[0] == '\0' ? "bash" : configured);
#ifdef _WIN32
    if (bash == "bash")
    {
        const char* locations[] = { "ProgramW6432", "ProgramFiles", "ProgramFiles(x86)" };
        for (const char* location : locations)
        {
            const char* directory = std::getenv(location);
            if (directory == nullptr)
            {
                continue;
            }
            const std::string git = std::string(directory) + "/Git/bin/bash.exe";
            if (GetFileAttributesA(git.c_str()) != INVALID_FILE_ATTRIBUTES)
            {
                bash = git;
                break;
            }
        }
    }
#endif
    // before不设置errexit，也不要求显式exit；工具记录普通命令失败，等脚本完成后统一判定。
    // 将脚本路径作为bash的$0，保留既有before使用dirname "$0"的路径语义。
    const std::string runner = "__cloud_before_status=0; trap '__cloud_before_status=$?' ERR; "
        "source \"$0\" \"$@\"; test \"$__cloud_before_status\" -eq 0";
    const int status = CaptureBefore({ bash, "-c", runner, path, bit, linkage, mode, scope }, records);
    if (!BuildEnvironment("CLOUD_REBUILD_SKIP_DEPENDENCY_BUILDS", previous) || status != 0)
    {
        return status != 0 ? status : 2;
    }
    if (!declared)
    {
        // 尚未迁移的基础库只兼容既有的字面量调用；不依据实际子进程执行结果登记。
        // ProcessWork/CTaskThreadManager本轮禁止改文件，复杂旧分支须由维护者显式声明。
        std::istringstream lines(content);
        std::map<std::string, std::string> fixed;
        std::string line;
        while (std::getline(lines, line))
        {
            std::istringstream fields(line);
            std::string first;
            fields >> first;
            const size_t assignment = first.find("_dlllib=");
            if (assignment != std::string::npos)
            {
                const std::string value = first.substr(assignment + 8);
                if (value == "lib" || value == "dll")
                {
                    fixed[first.substr(0, assignment)] = value;
                }
            }
            if (first != "\"$CLOUD_REBUILD\"")
            {
                continue;
            }
            std::string child;
            fields >> child;
            if (child == "DllRelyTest")
            {
                continue;
            }
            if (!ValidLibrary(child))
            {
                std::fprintf(stderr, "Migrate before to explicit CLOUD_REBUILD_LIBRARIESRELY records: %s\n", path.c_str());
                return 2;
            }
            records.push_back("project\t" + child + "\t" + (fixed.count(child) ? fixed[child] : linkage));
        }
    }
    // 新协议在全部准备完成后统一回传，以return结束；兼容既有begin格式，便于逐步迁移。
    const bool returned = !records.empty() && records.back() == "return";
    if (returned)
    {
        records.pop_back();
    }
    bool begun = !declared || returned;
    std::map<std::string, std::string> kinds;
    for (const std::string& record : records)
    {
        const size_t separator = record.find('\t');
        const std::string kind = record.substr(0, separator);
        const std::string value = separator == std::string::npos ? "" : record.substr(separator + 1);
        bool valid = false;
        if (kind == "begin")
        {
            valid = !begun && value == library;
            begun = true;
        }
        else if (kind == "project")
        {
            const size_t split = value.find('\t');
            const std::string child = value.substr(0, split);
            const std::string link = split == std::string::npos ? "" : value.substr(split + 1);
            valid = ValidLibrary(child) && child != library && (link == "lib" || link == "dll");
            valid = valid && (!kinds.count(child) || kinds[child] == link);
            kinds[child] = link;
        }
        else if (kind == "header")
        {
            valid = ValidLibrary(value);
        }
        else if (kind == "vendor" || kind == "link")
        {
            valid = !value.empty() && value.find_first_of(";\r\n\t\0", 0, 5) == std::string::npos;
            valid = valid && value.find("..") == std::string::npos;
            valid = valid && (kind != "vendor" || value.compare(0, 6, "tools/") == 0);
        }
        if (!begun || !valid)
        {
            std::fprintf(stderr, "Invalid before declaration in %s: %s\n", library.c_str(), record.c_str());
            return 2;
        }
    }
    if (!begun)
    {
        std::fprintf(stderr, "Before did not declare its inputs: %s\n", library.c_str());
        return 2;
    }
    return 0;
}

/** 将显式before依赖展开为当前目标的链接输入，不读取或生成.links文件
@param [in] root 仓库根目录
@param [in] records 当前库的声明
@param [in] bit 目标位数
@param [in] mode 配置
@param [in,out] active 当前递归路径，用于拒绝依赖环
@param [in,out] visited 已解析库及链接类型
@param [in,out] libraries 去重后的归档路径列表
@return 是否成功展开
*/
static bool PreparedDependencies(const std::string& root, const std::vector<std::string>& records,
    const std::string& bit, const std::string& mode, std::vector<std::string>& active,
    std::map<std::string, std::string>& visited, std::vector<std::string>& libraries)
{
    for (const std::string& record : records)
    {
        std::string path;
        if (record.compare(0, 8, "project\t") == 0)
        {
            const size_t split = record.find('\t', 8);
            const std::string child = record.substr(8, split - 8);
            const std::string link = record.substr(split + 1);
            if (std::find(active.begin(), active.end(), child) != active.end() || active.size() >= 128 || visited.size() >= 1024)
            {
                std::fprintf(stderr, "Circular or excessive dependency graph: %s\n", child.c_str());
                return false;
            }
            if (visited.count(child))
            {
                if (visited[child] != link)
                {
                    return false;
                }
                continue;
            }
            visited[child] = link;
            const bool wasm = bit.compare(0, 4, "wasm") == 0;
            if (wasm && link == "dll")
            {
                continue; // 独立Wasm模块由运行时桥接，不把其静态闭包混入调用方。
            }
            path = wasm ? child + (mode == "debug" ? "d" : "") + ".a" : NativeLibraryName(child, link, mode);
            if (std::find(libraries.begin(), libraries.end(), path) == libraries.end())
            {
                libraries.push_back(path);
            }
            if (link == "lib")
            {
                std::vector<std::string> children;
                active.push_back(child);
                if (CollectBefore(root, child, bit, link, mode, "only", children) != 0 ||
                    !PreparedDependencies(root, children, bit, mode, active, visited, libraries))
                {
                    return false;
                }
                active.pop_back();
            }
        }
        else if (record.compare(0, 5, "link\t") == 0)
        {
            path = record.substr(5);
        }
        else if (bit.compare(0, 4, "wasm") == 0 && record.compare(0, 7, "vendor\t") == 0)
        {
            path = record.substr(record.find_last_of('/') + 1);
        }
        if (!path.empty() && std::find(libraries.begin(), libraries.end(), path) == libraries.end())
        {
            libraries.push_back(path);
        }
    }
    return true;
}


/** 发布当前成功构建的原生客户端入口；原库目录和真实链接名称保持不变。
@param [in] root 仓库根目录
@param [in] sdkRoot Prepared SDK destination with a trailing separator
@param [in] library 已成功构建的库
@param [in] bit 目标位数
@param [in] linkage 库类型
@param [in] mode 配置
@return 0成功，其他值不允许上层继续使用本次入口
*/
static int PublishNetSenderEntry(const std::string& root, const std::string& sdkRoot, const std::string& library,
    const std::string& bit, const std::string& linkage, const std::string& mode)
{
    if (library != "NetSender" && library != "NetWork" && library != "NetPeer") return 0;
    const std::string directory = sdkRoot + "include/NetSender";
#ifdef _WIN32
    if (!CreateDirectoryA(directory.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS) return 1;
#else
    if (mkdir(directory.c_str(), 0755) != 0 && errno != EEXIST) return 1;
#endif
    std::string content;
    if (library == "NetSender")
    {
        std::ifstream source((root + "src/NetSender/NetSender/src/NetSenderAPI.h").c_str(), std::ios::binary);
        if (!source) return 1;
        content.assign(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>());
    }
    else
    {
        std::ifstream source((sdkRoot + "include/" + library + "/NetSenderAPI.h").c_str(), std::ios::binary);
        if (!source) return 1;
        content = "\xEF\xBB\xBF#pragma once\r\n// NetSender provider: " + library + "; target: " + bit +
            "; linkage: " + linkage + "; configuration: " + mode + "\r\n#include \"../" + library + "/NetSenderAPI.h\"";
    }
    const std::string target = directory + "/NetSenderAPI.h";
#ifdef _WIN32
    const std::string staged = target + "." + std::to_string(GetCurrentProcessId()) + ".publishing";
#else
    const std::string staged = target + "." + std::to_string(getpid()) + ".publishing";
#endif
    {
        std::ofstream output(staged.c_str(), std::ios::binary | std::ios::trunc);
        output.write(content.data(), static_cast<std::streamsize>(content.size()));
        output.close();
        if (!output) { std::remove(staged.c_str()); return 1; }
    }
#ifdef _WIN32
    const bool replaced = MoveFileExA(staged.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    const bool replaced = std::rename(staged.c_str(), target.c_str()) == 0;
#endif
    if (!replaced) std::remove(staged.c_str());
    return replaced ? 0 : 1;
}

/** 清理失败的Wasm子构建登记，使before后续依赖仍能独立执行；不改变原失败码
@param [in] library 本次失败的项目名称
@return 无；无继承会话时不处理，清理失败只输出诊断
*/
static void FailedWasmNode(const std::string& library)
{
    const char* session = std::getenv("CLOUD_WASM_SESSION");
    const char* parent = std::getenv("CLOUD_WASM_PARENT");
    if (session == nullptr || session[0] == '\0' || parent == nullptr || parent[0] == '\0')
    {
        return;
    }
    const char* python = std::getenv("CLOUD_REBUILD_PYTHON");
    if (python == nullptr || python[0] == '\0')
    {
#ifdef _WIN32
        python = "python";
#else
        python = "python3";
#endif
    }
    // 仅移除父节点之后的失败子树；循环调用被拒绝时不能误删祖先节点。
    const std::string cleanup =
        "import json,sys\n"
        "from pathlib import Path\n"
        "p=Path(sys.argv[1]); parent=sys.argv[2]; library=sys.argv[3]\n"
        "s=json.loads(p.read_text(encoding='utf-8')); active=s['active']\n"
        "if Path(s['lock']).is_file() and parent in active and library in active:\n"
        "    index=active.index(library)\n"
        "    if index>active.index(parent):\n"
        "        for name in active[index:]: s['nodes'].pop(name,None)\n"
        "        s['active']=active[:index]\n"
        "        p.write_text(json.dumps(s),encoding='utf-8')\n";
    std::vector<std::string> ignored;
    if (CaptureBefore({ python, "-c", cleanup, session, parent, library }, ignored) != 0)
    {
        std::fprintf(stderr, "Could not clean failed Wasm dependency: %s\n", library.c_str());
    }
}

static int RunWasmBuild(int argc, char** argv, bool& handled)
{
    const std::string target = argc > 2 ? WasmLower(argv[2]) : "";
    handled = target.compare(0, 4, "wasm") == 0 || (argc > 3 && WasmLower(argv[3]) == "wasm");
    if (!handled)
    {
        return 0;
    }
    const std::string linkage = argc > 3 ? WasmLower(argv[3]) : "";
    const std::string mode = argc > 4 ? WasmLower(argv[4]) : "";
    const std::string scope = argc > 5 ? WasmLower(argv[5]) : "";
    if ((argc != 6 && !(argc == 7 && std::string(argv[6]).compare(0, 16, "--librariesrely=") == 0)) || (target != "wasm32" && target != "wasm64") ||
        (linkage != "dll" && linkage != "lib") || (mode != "debug" && mode != "release") ||
        (scope != "same" && scope != "only" && scope != "librely"))
    {
        std::fprintf(stderr, "Wasm usage: CloudRebuild <library> <wasm32|wasm64> <dll|lib> <debug|release> <same|only|librely>\n");
        return 2;
    }
    const char* root = std::getenv("XUEYELINGSHUANG");
    if (root == NULL || root[0] == '\0')
    {
        std::fprintf(stderr, "Wasm requires XUEYELINGSHUANG to name the repository root.\n");
        return 2;
    }
    const std::string library(argv[1]);
    if (library.empty() || !(std::isalpha(static_cast<unsigned char>(library[0])) || library[0] == '_') ||
        !std::all_of(library.begin(), library.end(), [](unsigned char value) { return std::isalnum(value) || value == '_'; }))
    {
        std::fprintf(stderr, "Invalid Wasm library name.\n");
        return 2;
    }
    std::string prefix(root);
    if (prefix.back() != '/' && prefix.back() != '\\')
    {
        prefix += '/';
    }
    const std::string scripts = prefix + "src/" + library + "/scripts/";
#ifdef _WIN32
    const std::string script = scripts + "rebuild_" + library + ".py";
    std::ifstream input(script.c_str(), std::ios::binary);
    const std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (contents.find("CLOUD_REBUILD_WASM_ENTRY = True") == std::string::npos)
    {
        std::fprintf(stderr, "%s has no migrated Windows Wasm rebuild entry.\n", library.c_str());
        return 2;
    }
    const char* python = std::getenv("CLOUD_REBUILD_PYTHON");
    if (python == NULL || python[0] == '\0')
    {
        python = "python";
    }
    const std::string quotedPython = std::string("\"") + python + "\"";
    const std::string quotedScript = std::string("\"") + script + "\"";
    const std::string forwarded = argc == 7 ? std::string("\"") + argv[6] + "\"" : "";
    const char* arguments[] = { quotedPython.c_str(), quotedScript.c_str(), target.c_str(), linkage.c_str(), mode.c_str(), scope.c_str(), argc == 7 ? forwarded.c_str() : nullptr, nullptr };
    const intptr_t status = _spawnvp(_P_WAIT, python, arguments);
    if (status == -1)
    {
        std::perror("Could not launch the library's Wasm rebuild script");
        return 127;
    }
    if (status != 0) FailedWasmNode(library);
    return static_cast<int>(status);
#else
    std::ifstream input((scripts + "CMakeLists.txt").c_str());
    const std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (contents.find("WASM_SDK_LINKAGES") == std::string::npos)
    {
        std::fprintf(stderr, "%s has no migrated Wasm CMake entry.\n", library.c_str());
        return 2;
    }
    const std::string temporary = prefix + "src/temp";
    std::string builds = temporary;
    const std::vector<std::string> levels = { "", library, target, linkage, mode };
    for (const std::string& level : levels)
    {
        if (!level.empty()) builds += "/" + level;
        if (mkdir(builds.c_str(), 0755) != 0 && errno != EEXIST)
        {
            std::perror("Could not create the library's Wasm build directory");
            return 1;
        }
    }
    const std::string pattern = builds + "/" + library + "-" + target + "-" + linkage + "-" + mode + "-XXXXXX";
    std::vector<char> directory(pattern.begin(), pattern.end());
    directory.push_back('\0');
    if (mkdtemp(&directory[0]) == NULL)
    {
        std::perror("Could not create an isolated Wasm build directory");
        return 1;
    }
    const char* configured = std::getenv("CMAKE_COMMAND");
    const std::string cmake(configured != NULL && configured[0] != '\0' ? configured : "cmake");
    const char* destination = std::getenv("CLOUD_REBUILD_SDK_ROOT");
    const std::string sdk(destination != NULL && destination[0] != '\0' ? destination : root);
    std::vector<std::string> command = { cmake, "-S", scripts, "-B", &directory[0],
        "-DBIT=" + target, "-DDLLLIB=" + linkage, "-DDEBUGRELEASE=" + mode,
        "-DCMAKE_BUILD_TYPE=" + std::string(mode == "debug" ? "Debug" : "Release"),
        "-DWASM_BUILD_SCOPE=" + scope, "-DWASM_SDK_ROOT:PATH=" + sdk, "-DWASM_SDK_PUBLISH=ON" };
    if (argc == 7)
    {
        command.push_back(std::string("-DLIBRARIESRELY=") + std::string(argv[6]).substr(16));
    }
    const int status = BuildCommand(command);
    if (status != 0)
    {
        FailedWasmNode(library);
        return status;
    }
    command = { cmake, "--build", &directory[0], "--target", library };
    const int built = BuildCommand(command);
    if (built != 0) FailedWasmNode(library);
    return built;
#endif
}

static int RunMain(int argc, char** argv)
{
#ifdef _WIN32
    // 仅最外层决定终端颜色；子构建继承选择并把颜色标记写入日志文本。
    if (std::getenv("CLOUD_REBUILD_COLOR") == nullptr)
    {
        const char* term = std::getenv("TERM");
        DWORD mode = 0;
        HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        bool color = false;
        if (std::getenv("NO_COLOR") == nullptr && (term == nullptr || std::string(term) != "dumb"))
        {
            if (GetConsoleMode(output, &mode))
            {
                color = SetConsoleMode(output, mode | 4) != 0;
                HANDLE diagnostic = GetStdHandle(STD_ERROR_HANDLE);
                if (GetConsoleMode(diagnostic, &mode)) SetConsoleMode(diagnostic, mode | 4);
            }
            else
            {
                color = GetFileType(output) == FILE_TYPE_PIPE && std::getenv("MSYSTEM") != nullptr && term != nullptr;
            }
        }
        if (!BuildEnvironment("CLOUD_REBUILD_COLOR", color ? "1" : "0")) return 2;
    }
#endif
    if (argc == 7 && std::string(argv[1]) == "--before")
    {
        const char* configuredRoot = std::getenv("XUEYELINGSHUANG");
        const std::string bit(argv[3]), linkage(argv[4]), mode(argv[5]), scope(argv[6]);
        if (configuredRoot == nullptr || configuredRoot[0] == '\0' || !ValidLibrary(argv[2]) ||
            (bit != "32" && bit != "64" && bit != "arm32" && bit != "arm64" && bit != "wasm32" && bit != "wasm64") ||
            (linkage != "dll" && linkage != "lib") || (mode != "release" && mode != "debug") ||
            (scope != "only" && scope != "same"))
        {
            return 2;
        }
        std::string root(configuredRoot);
        if (root.back() != '/' && root.back() != '\\')
        {
            root += '/';
        }
        std::vector<std::string> records;
        const int status = CollectBefore(root, argv[2], bit, linkage, mode, scope, records);
        if (status != 0)
        {
            return status;
        }
        std::vector<std::string> files;
        std::vector<std::string> active = { argv[2] };
        std::map<std::string, std::string> visited;
        if (!PreparedDependencies(root, records, bit, mode, active, visited, files))
        {
            return 2;
        }
        for (const std::string& record : records)
        {
            std::printf("CLOUD_REBUILD_LIBRARIESRELY\t%s\n", record.c_str());
        }
        std::string librariesRely;
        for (const std::string& file : files)
        {
            if (!librariesRely.empty())
            {
                librariesRely += ';';
            }
            librariesRely += file;
        }
        std::printf("CLOUD_REBUILD_LIBRARIESRELY\tlibrariesrely\t%s\n", librariesRely.c_str());
        return std::fflush(stdout) == 0 ? 0 : 1;
    }
    // only仍执行before；未迁移的基础库脚本通过该继承标记禁止嵌套重编。
    const char* skip = std::getenv("CLOUD_REBUILD_SKIP_DEPENDENCY_BUILDS");
    if (skip != nullptr && std::string(skip) == "1")
    {
        return 0;
    }

    if (argc > 1 && ValidLibrary(argv[1]))
    {
        const char* inherited = std::getenv("CLOUD_REBUILD_ACTIVE");
        std::string active(inherited == nullptr ? ";" : inherited);
        const std::string current = std::string(";") + argv[1] + ";";
        if (active.find(current) != std::string::npos || active.size() > 16384)
        {
            std::fprintf(stderr, "Circular or excessive before build: %s\n", argv[1]);
            return 2;
        }
        active += std::string(argv[1]) + ";";
        if (!BuildEnvironment("CLOUD_REBUILD_ACTIVE", active))
        {
            return 2;
        }
    }
	bool wasmHandled = false;
	const int wasmStatus = RunWasmBuild(argc, argv, wasmHandled);
	if (wasmHandled)
	{
		return wasmStatus;
	}
	std::string projectName;
	std::string bit;
	std::string dlllib;
	std::string debugRelease;
	std::string allSame;

	if (argc != 6)
	{
		return 2;
	}
	projectName = argv[1];
	bit = argv[2];
	dlllib = argv[3];
	debugRelease = argv[4];
	allSame = argv[5];
    if (!ValidLibrary(projectName) || (bit != "32" && bit != "64" && bit != "arm32" && bit != "arm64") ||
        (dlllib != "lib" && dlllib != "dll") || (debugRelease != "debug" && debugRelease != "release") ||
        (allSame != "only" && allSame != "same" && allSame != "librely"))
    {
        return 2;
    }
	
	//xueyelingshuang路径
    const char* configuredRoot = std::getenv("XUEYELINGSHUANG");
    if (configuredRoot == nullptr || configuredRoot[0] == '\0')
    {
        std::fprintf(stderr, "XUEYELINGSHUANG must name the repository root.\n");
        return 2;
    }
    std::string xueyePath(configuredRoot);
    if (xueyePath.back() != '/' && xueyePath.back() != '\\')
    {
        xueyePath += PATHLEVEL;
    }
	//工程路径
	std::string projectPath = xueyePath + "src" + PATHLEVEL + projectName + PATHLEVEL;
    std::string sdkRoot(xueyePath);
#ifndef _WIN32
    const char* configuredSdk = std::getenv("CLOUD_REBUILD_SDK_ROOT");
    if (configuredSdk != nullptr && configuredSdk[0] != '\0')
    {
        sdkRoot = configuredSdk;
        if (sdkRoot.back() != '/')
        {
            sdkRoot += '/';
        }
    }
#endif

	//准备脚本路径
	std::string beforeBatPath = projectPath + "scripts" + PATHLEVEL + "before_" + projectName + ".sh";
	//后期脚本路径
	std::string afterBatPath = projectPath + "scripts" + PATHLEVEL + "after_" + projectName + ".sh";

#ifdef _WIN32
	//32位自动化编译python脚本路径
	std::string pyPath = projectPath + "scripts" + PATHLEVEL + "rebuild_" + projectName + ".py";
#elif defined(__unix__) || defined(__APPLE__)
	//linux下CMakeLists文件路径
	std::string cmakelistsPath = projectPath + "scripts" + PATHLEVEL;
	//linux下build目录
	std::string buildPath = xueyePath + "src/temp/CloudRebuild/" + projectName + "/" + bit + "/" + dlllib + "/" + debugRelease + "/" + std::to_string(getpid());
	//lib目录
	std::string libPath = sdkRoot + "lib" + PATHLEVEL;
#endif

	std::string srcPath = projectPath + projectName + PATHLEVEL + "src" + PATHLEVEL;
	std::string includePath = sdkRoot + "include" + PATHLEVEL + projectName + PATHLEVEL;

	std::string d;
	if (debugRelease != "release")
	{
		d = "d";
	}

	std::string deleteFileCommand;
#ifdef _WIN32
	deleteFileCommand = "del";
#elif defined(__unix__) || defined(__APPLE__)
	deleteFileCommand = "rm";
#endif

    std::vector<std::string> beforeRecords;
    const int beforeStatus = CollectBefore(xueyePath, projectName, bit,
        allSame == "librely" ? "lib" : dlllib, debugRelease, allSame == "only" ? "only" : "same", beforeRecords);
    if (beforeStatus != 0)
    {
        return beforeStatus;
    }

	//编译工程
#ifdef _WIN32
    if (system((MARK(pyPath) + SPACE + bit + SPACE + dlllib + SPACE + debugRelease).c_str()) != 0) return 1;
#elif defined(__unix__) || defined(__APPLE__)
    std::vector<std::string> preparedClosure;
    std::vector<std::string> active = { projectName };
    std::map<std::string, std::string> visited;
    if (!PreparedDependencies(xueyePath, beforeRecords, bit, debugRelease, active, visited, preparedClosure))
    {
        return 2;
    }
    std::string librariesRely;
    for (const std::string& library : preparedClosure)
    {
        const std::string path = library[0] == '/' ? library : libPath + library;
        if (::access(path.c_str(), R_OK) != 0)
        {
            std::fprintf(stderr, "Missing prepared dependency: %s\n", path.c_str());
            return 2;
        }
        if (!librariesRely.empty())
        {
            librariesRely += ';';
        }
        librariesRely += library;
    }
    const char* configuredCmake = std::getenv("CMAKE_COMMAND");
    const std::string cmake(configuredCmake != nullptr && configuredCmake[0] != '\0' ? configuredCmake : "cmake");
    std::vector<std::string> command = { cmake, "-S", cmakelistsPath, "-B", buildPath,
        "-DBIT=" + bit, "-DDLLLIB=" + dlllib, "-DDEBUGRELEASE=" + debugRelease,
        "-DLIBRARIESRELY=" + librariesRely, "-DSDK_ROOT:PATH=" + sdkRoot };
    const char* settings[] = { "CMAKE_TOOLCHAIN_FILE", "CMAKE_SYSTEM_NAME", "CMAKE_SYSTEM_PROCESSOR", "CMAKE_OSX_ARCHITECTURES",
        "CMAKE_OSX_SYSROOT", "CMAKE_OSX_DEPLOYMENT_TARGET", "CMAKE_ANDROID_NDK", "CMAKE_ANDROID_ARCH_ABI",
        "ANDROID_ABI", "ANDROID_PLATFORM", "CMAKE_C_COMPILER", "CMAKE_CXX_COMPILER", "CMAKE_SYSROOT" };
    for (const char* setting : settings)
    {
        const char* value = std::getenv(setting);
        if (value != nullptr && value[0] != '\0')
        {
            command.push_back(std::string("-D") + setting + "=" + value);
        }
    }
    if (BuildCommand(command) != 0)
    {
        return 1;
    }
    command = { cmake, "--build", buildPath, "--config", debugRelease == "debug" ? "Debug" : "Release",
        "--parallel", std::to_string(GetCPUCoreCount()) };
    if (BuildCommand(command) != 0)
    {
        return 1;
    }
#endif

#ifdef _WIN32
	//向公共部分提供文件，有的时候可能没有inl文件
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(srcPath + "*.h") + SPACE + MARK(includePath)).c_str());
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(srcPath + "*.inl") + SPACE + MARK(includePath)).c_str());
#elif defined(__unix__) || defined(__APPLE__)
	if (projectName != "DllRelyTest")
	{
		system(("mkdir" + SPACE + "-p" + SPACE + MARK(includePath)).c_str());
	}
	//linux下无法拷贝自定义目录下的文件
	system(("cp" + SPACE + "-f" + SPACE + MARK(srcPath) + "*.h" + SPACE + MARK(includePath)).c_str());
	std::vector<std::string> vecFile;
	FindDirFile(srcPath.c_str(), vecFile);
	bool hasInl = false;
	int32_t index = -1;
	while (++index < static_cast<int32_t>(vecFile.size()))
	{
		std::string& fileName = vecFile[index];
		if (fileName.size() <= 4)
		{
			continue;
		}
		if (fileName.substr(fileName.size() - 4, 4) == ".inl")
		{
			hasInl = true;
			break;
		}
	}
	if (hasInl)
	{
		system(("cp" + SPACE + "-f" + SPACE + MARK(srcPath) + "*.inl" + SPACE + MARK(includePath)).c_str());
	}
#endif

	//删除宏文件，并且替换特定的宏文件
	if (dlllib == "lib")
	{
		system((deleteFileCommand + SPACE + MARK(includePath + projectName + "Macro.h")).c_str());
		system((deleteFileCommand + SPACE + MARK(includePath + projectName + "MacroDll.h")).c_str());
		rename((includePath + projectName + "MacroLib.h").c_str(), (includePath + projectName + "Macro.h").c_str());
	}
	else if (dlllib == "dll")
	{
		system((deleteFileCommand + SPACE + MARK(includePath + projectName + "Macro.h")).c_str());
		system((deleteFileCommand + SPACE + MARK(includePath + projectName + "MacroLib.h")).c_str());
		rename((includePath + projectName + "MacroDll.h").c_str(), (includePath + projectName + "Macro.h").c_str());
	}

#ifdef _WIN32
	//删除临时文件
	system(("del" + SPACE + MARK(projectPath + "@AutomationLog.txt")).c_str());
	system(("del" + SPACE + "@AutomationLog.txt").c_str());
	system(("del" + SPACE + MARK(projectPath + "scripts" + PATHLEVEL + "@AutomationLog.txt")).c_str());
	system(("del" + SPACE + MARK(projectPath + "scripts" + PATHLEVEL + "msbuild.log")).c_str());
	
	system(("del" + SPACE + MARK(xueyePath + "lib" + PATHLEVEL + projectName + d + ".ilk")).c_str());
	system(("del" + SPACE + MARK(xueyePath + "lib" + PATHLEVEL + projectName + d + ".exp")).c_str());
#elif defined(__unix__) || defined(__APPLE__)
    // 保留隔离构建目录用于定位失败，不删除其他目标的CMake缓存
#endif

	//执行后期脚本
	if (system(("bash" + SPACE + MARK(afterBatPath)).c_str()) != 0) return 1;
    const int published = PublishNetSenderEntry(xueyePath, sdkRoot, projectName, bit, dlllib, debugRelease);
    return published;
}
/** 构建工具入口，捕获标准库异常并返回可观察的构建失败
@param [in] argc 参数数量
@param [in] argv 参数数组
@return 0成功，其他表示失败
*/
int main(int argc, char** argv)
{
    try
    {
        return RunMain(argc, argv);
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "CloudRebuild failed: %s\n", error.what());
        return 1;
    }
}
