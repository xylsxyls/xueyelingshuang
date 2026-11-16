#include "NetTestProcess.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <chrono>
#include <algorithm>
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

NetTestProcess::NetTestProcess() :
m_monitor(0),
m_job(0),
m_pid(0),
m_exited(false),
m_exitCode(0)
{

}

NetTestProcess::~NetTestProcess()
{
    terminate();
#ifdef _WIN32
    CSystem::closeProcessMonitor(m_monitor);
    if (m_job != 0)
    {
        CloseHandle(reinterpret_cast<HANDLE>(static_cast<uintptr_t>(m_job)));
    }
#endif
}

bool NetTestProcess::start(const std::string& executable, const std::vector<std::string>& arguments,
    const std::string& logFile, bool normalStartup)
{
    if (m_pid != 0 || executable.empty() || logFile.empty() ||
        executable.find('\0') != std::string::npos || logFile.find('\0') != std::string::npos)
    {
        return false;
    }
    for (size_t index = 0; index < arguments.size(); ++index)
    {
        if (arguments[index].find('\0') != std::string::npos)
        {
            return false;
        }
    }
    m_logFile = logFile;
#ifdef _WIN32
    return startWindows(executable, arguments, logFile, normalStartup);
#else
    (void)normalStartup;
    if (executable[0] != '/' || logFile[0] != '/')
    {
        return false;
    }
    std::vector<char*> command;
    command.push_back(const_cast<char*>(executable.c_str()));
    for (size_t index = 0; index < arguments.size(); ++index)
    {
        command.push_back(const_cast<char*>(arguments[index].c_str()));
    }
    command.push_back(nullptr);
    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0)
    {
        return false;
    }
    int error = posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO,
        logFile.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (error == 0)
    {
        error = posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO);
    }
    if (error == 0)
    {
        error = posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    }
    pid_t child = 0;
    if (error == 0)
    {
        error = posix_spawn(&child, executable.c_str(), &actions, nullptr, command.data(), environ);
    }
    posix_spawn_file_actions_destroy(&actions);
    if (error != 0)
    {
        return false;
    }
    m_pid = static_cast<uint32_t>(child);
    return true;
#endif
}

bool NetTestProcess::startWindows(const std::string& executable, const std::vector<std::string>& arguments,
    const std::string& logFile, bool normalStartup)
{
#ifdef _WIN32
    const std::wstring path = CStringManager::Utf8ToUnicode(executable);
    const std::wstring destination = CStringManager::Utf8ToUnicode(logFile);
    // 检查value是否为Windows绝对路径；相对路径不能触发当前目录下的同名程序。
    const auto absolute = [](const std::wstring& value) -> bool
    {
        return value.size() > 2 && ((value[1] == L':' && (value[2] == L'/' || value[2] == L'\\')) ||
            (value[0] == L'\\' && value[1] == L'\\'));
    };
    if (!absolute(path) || !absolute(destination) || path.size() > 32750 || destination.size() > 32750 ||
        CStringManager::UnicodeToUtf8(path) != executable || CStringManager::UnicodeToUtf8(destination) != logFile ||
        path.find(L'"') != std::wstring::npos || arguments.size() > 64)
    {
        return false;
    }
    std::wstring command = L"\"" + path + L"\"";
    for (size_t index = 0; index < arguments.size(); ++index)
    {
        if (arguments[index].size() > 32760)
        {
            return false;
        }
        const std::wstring value = CStringManager::Utf8ToUnicode(arguments[index]);
        if (CStringManager::UnicodeToUtf8(value) != arguments[index])
        {
            return false;
        }
        command += L" \"";
        size_t slashes = 0;
        for (size_t character = 0; character < value.size(); ++character)
        {
            if (value[character] == L'\\')
            {
                ++slashes;
                continue;
            }
            command.append(slashes * (value[character] == L'"' ? 2 : 1), L'\\');
            slashes = 0;
            if (value[character] == L'"')
            {
                command += L'\\';
            }
            command += value[character];
        }
        command.append(slashes * 2, L'\\');
        command += L'"';
        if (command.size() > 32760)
        {
            return false;
        }
    }
    std::vector<wchar_t> buffer(command.begin(), command.end());
    buffer.push_back(L'\0');
    SIZE_T attributeSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeSize);
    if (attributeSize == 0)
    {
        return false;
    }
    // 所有可能分配容器内存的步骤放在内核资源获取前。
    std::vector<uint8_t> attributes(attributeSize);
    STARTUPINFOEXW startup = {};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES | (normalStartup ? 0 : STARTF_USESHOWWINDOW);
    startup.StartupInfo.wShowWindow = SW_HIDE;
    startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
    if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attributeSize))
    {
        return false;
    }
    SECURITY_ATTRIBUTES security = {};
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;
    HANDLE output = CreateFileW(destination.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    HANDLE inherited[] = { output, input };
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    bool valid = output != INVALID_HANDLE_VALUE && input != INVALID_HANDLE_VALUE && job != nullptr &&
        SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) &&
        UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
            inherited, sizeof(inherited), nullptr, nullptr);
    startup.StartupInfo.hStdOutput = output;
    startup.StartupInfo.hStdError = output;
    startup.StartupInfo.hStdInput = input;
    PROCESS_INFORMATION child = {};
    if (valid)
    {
        valid = CreateProcessW(path.c_str(), buffer.data(), nullptr, nullptr, TRUE,
            CREATE_SUSPENDED | (normalStartup ? 0 : CREATE_NO_WINDOW) | CREATE_NEW_PROCESS_GROUP | EXTENDED_STARTUPINFO_PRESENT,
            nullptr, nullptr, &startup.StartupInfo, &child) != FALSE;
    }
    if (valid)
    {
        valid = AssignProcessToJobObject(job, child.hProcess) != FALSE && ResumeThread(child.hThread) != DWORD(-1);
    }
    if (child.hProcess != nullptr && !valid)
    {
        // 绑定失败时主线程尚未运行，不会产生逃逸后代。
        TerminateProcess(child.hProcess, 125);
        WaitForSingleObject(child.hProcess, 5000);
        CloseHandle(child.hProcess);
    }
    if (child.hThread != nullptr)
    {
        CloseHandle(child.hThread);
    }
    DeleteProcThreadAttributeList(startup.lpAttributeList);
    if (output != INVALID_HANDLE_VALUE)
    {
        CloseHandle(output);
    }
    if (input != INVALID_HANDLE_VALUE)
    {
        CloseHandle(input);
    }
    if (!valid)
    {
        if (job != nullptr)
        {
            CloseHandle(job);
        }
        return false;
    }
    m_monitor = reinterpret_cast<uintptr_t>(child.hProcess);
    m_job = reinterpret_cast<uintptr_t>(job);
    m_pid = child.dwProcessId;
    return true;
#else
    (void)executable;
    (void)arguments;
    (void)logFile;
    (void)normalStartup;
    return false;
#endif
}

int32_t NetTestProcess::poll(int32_t& exitCode)
{
    exitCode = 0;
    if (m_pid == 0)
    {
        return -1;
    }
    if (m_exited)
    {
        exitCode = m_exitCode;
        return 1;
    }
#ifdef _WIN32
    const int32_t state = CSystem::pollProcessMonitor(m_monitor, m_exitCode);
    if (state <= 0)
    {
        return state;
    }
#else
    int status = 0;
    const pid_t result = waitpid(static_cast<pid_t>(m_pid), &status, WNOHANG);
    if (result == 0 || (result < 0 && errno == EINTR))
    {
        return 0;
    }
    if (result < 0 || (!WIFEXITED(status) && !WIFSIGNALED(status)))
    {
        return -1;
    }
    m_exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
#endif
    m_exited = true;
    exitCode = m_exitCode;
    return 1;
}

int32_t NetTestProcess::wait(uint32_t timeoutMs, int32_t& exitCode)
{
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    for (;;)
    {
        const int32_t state = poll(exitCode);
        if (state != 0 || std::chrono::steady_clock::now() >= deadline)
        {
            return state;
        }
        CSystem::Sleep(1);
    }
}

int32_t NetTestProcess::waitTree(uint32_t timeoutMs) const
{
#ifdef _WIN32
    if (m_job == 0)
    {
        return -1;
    }
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    for (;;)
    {
        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting = {};
        if (!QueryInformationJobObject(reinterpret_cast<HANDLE>(static_cast<uintptr_t>(m_job)),
            JobObjectBasicAccountingInformation, &accounting, sizeof(accounting), nullptr))
        {
            return -1;
        }
        if (accounting.ActiveProcesses == 0)
        {
            return 1;
        }
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return 0;
        }
        CSystem::Sleep(1);
    }
#else
    (void)timeoutMs;
    return -1;
#endif
}

bool NetTestProcess::terminate()
{
    if (m_pid == 0 || m_exited)
    {
        return true;
    }
    int32_t code = 0;
    const int32_t state = poll(code);
    if (state != 0)
    {
        return state == 1;
    }
#ifdef _WIN32
    // 使用启动时的内核句柄，不通过可复用PID重新打开进程
    if (!TerminateProcess(reinterpret_cast<HANDLE>(static_cast<uintptr_t>(m_monitor)), 125))
    {
        return poll(code) == 1;
    }
#else
    if (kill(static_cast<pid_t>(m_pid), SIGKILL) != 0 && errno != ESRCH)
    {
        return false;
    }
#endif
    return wait(5000, code) == 1;
}

bool NetTestProcess::output(std::string& text) const
{
    std::vector<uint8_t> bytes;
    if (!CSystem::readBinaryFile(CStringManager::Utf8ToUnicode(m_logFile), 16 * 1024 * 1024, bytes))
    {
        return false;
    }
    std::string value(bytes.begin(), bytes.end());
    text.swap(value);
    return true;
}

uint32_t NetTestProcess::pid() const
{
    return m_pid;
}

bool NetTestProcess::ownedProcesses(const std::string& executable, std::vector<uint32_t>& processes) const
{
#ifdef _WIN32
    if (m_job == 0) { return false; }
    std::wstring expected = CStringManager::Utf8ToUnicode(CSystem::canonicalPathUtf8(executable, true));
    std::replace(expected.begin(), expected.end(), L'/', L'\\');
    if (expected.empty()) { return false; }
    HANDLE job = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(m_job));
    std::vector<uint32_t> found;
    // 这里只约束测试夹具快照的资源预算，不限制被测网关的接纳能力。
    for (size_t capacity = 16; capacity <= 4096; capacity *= 2)
    {
        std::vector<uint64_t> storage((sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) +
            capacity * sizeof(ULONG_PTR) + sizeof(uint64_t) - 1) / sizeof(uint64_t));
        JOBOBJECT_BASIC_PROCESS_ID_LIST* list = reinterpret_cast<JOBOBJECT_BASIC_PROCESS_ID_LIST*>(storage.data());
        if (!QueryInformationJobObject(job, JobObjectBasicProcessIdList, list,
            static_cast<DWORD>(storage.size() * sizeof(uint64_t)), nullptr))
        {
            if (GetLastError() == ERROR_MORE_DATA) { continue; }
            return false;
        }
        for (DWORD index = 0; index < list->NumberOfProcessIdsInList; ++index)
        {
            const DWORD id = static_cast<DWORD>(list->ProcessIdList[index]);
            HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, id);
            if (process == nullptr)
            {
                if (GetLastError() == ERROR_INVALID_PARAMETER) { continue; }
                return false;
            }
            wchar_t path[32768] = {};
            DWORD size = 32768;
            BOOL member = FALSE;
            const bool queried = IsProcessInJob(process, job, &member) &&
                QueryFullProcessImageNameW(process, 0, path, &size);
            const DWORD status = WaitForSingleObject(process, 0);
            CloseHandle(process);
            if (status == WAIT_OBJECT_0) { continue; }
            if (!queried || status != WAIT_TIMEOUT) { return false; }
            if (member && _wcsicmp(path, expected.c_str()) == 0) { found.push_back(id); }
        }
        processes.swap(found);
        return true;
    }
    return false;
#else
    (void)executable;
    (void)processes;
    return false;
#endif
}

bool NetTestProcess::terminateOwned(uint32_t processId, const std::string& executable) const
{
#ifdef _WIN32
    std::wstring expected = CStringManager::Utf8ToUnicode(CSystem::canonicalPathUtf8(executable, true));
    std::replace(expected.begin(), expected.end(), L'/', L'\\');
    if (m_job == 0 || processId == 0 || expected.empty()) { return false; }
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE, FALSE, processId);
    if (process == nullptr) { return false; }
    wchar_t path[32768] = {};
    DWORD size = 32768;
    BOOL member = FALSE;
    const bool valid = IsProcessInJob(process, reinterpret_cast<HANDLE>(static_cast<uintptr_t>(m_job)), &member) &&
        member && QueryFullProcessImageNameW(process, 0, path, &size) && _wcsicmp(path, expected.c_str()) == 0 &&
        WaitForSingleObject(process, 0) == WAIT_TIMEOUT;
    const bool stopped = valid && TerminateProcess(process, 125) && WaitForSingleObject(process, 5000) == WAIT_OBJECT_0;
    CloseHandle(process);
    return stopped;
#else
    (void)processId;
    (void)executable;
    return false;
#endif
}