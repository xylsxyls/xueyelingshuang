#include "MysqlCppHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cstdlib>
#include <cstring>
#include <memory>
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <spawn.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include "mysql-connector-cpp/cppconn/exception.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/** 定位 SQL 开头的第一个非注释词
@param [in] sql 待检查的完整语句
@return 首词的字节位置，没有词时返回 sql.size()
*/
static size_t FirstSqlWordOffset(const std::string& sql)
{
    size_t index = 0;
    while (index < sql.size())
    {
        while (index < sql.size() && static_cast<unsigned char>(sql[index]) <= ' ')
        {
            ++index;
        }

        if (index + 1 < sql.size() && sql[index] == '-' && sql[index + 1] == '-')
        {
            index += 2;
            while (index < sql.size() && sql[index] != '\n' && sql[index] != '\r')
            {
                ++index;
            }
            continue;
        }

        if (index < sql.size() && sql[index] == '#')
        {
            ++index;
            while (index < sql.size() && sql[index] != '\n' && sql[index] != '\r')
            {
                ++index;
            }
            continue;
        }

        if (index + 1 < sql.size() && sql[index] == '/' && sql[index + 1] == '*')
        {
            size_t end = sql.find("*/", index + 2);
            if (end == std::string::npos)
            {
                return sql.size();
            }
            index = end + 2;
            continue;
        }
        break;
    }

    return index;
}

/** 比较语句首词与 ASCII 关键字，并检查关键字边界
@param [in] text 指向以零结尾的 SQL 文本首词
@param [in] keyword 以零结尾的大写关键字
@return 关键字与词尾均匹配时为 true
*/
static bool MatchesSqlKeyword(const char* text, const char* keyword)
{
    while (*keyword != '\0')
    {
        const char ch = *text;
        const char upper = ch >= 'a' && ch <= 'z' ? static_cast<char>(ch - 'a' + 'A') : ch;
        if (upper != *keyword)
        {
            return false;
        }
        ++text;
        ++keyword;
    }
    return !((*text >= 'A' && *text <= 'Z') || (*text >= 'a' && *text <= 'z'));
}

std::string MysqlCppHelper::firstSqlWord(const std::string& sql)
{
    const size_t index = FirstSqlWordOffset(sql);
    size_t end = index;
    while (end < sql.size())
    {
        unsigned char ch = static_cast<unsigned char>(sql[end]);
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')))
        {
            break;
        }
        ++end;
    }

    std::string word = sql.substr(index, end - index);
    for (size_t i = 0; i < word.size(); ++i)
    {
        if (word[i] >= 'a' && word[i] <= 'z')
        {
            word[i] = static_cast<char>(word[i] - 'a' + 'A');
        }
    }
    return word;
}

bool MysqlCppHelper::isQuerySql(const std::string& sql)
{
    const char* word = sql.c_str() + FirstSqlWordOffset(sql);
    return MatchesSqlKeyword(word, "SELECT") || MatchesSqlKeyword(word, "SHOW") ||
        MatchesSqlKeyword(word, "DESC") || MatchesSqlKeyword(word, "DESCRIBE") ||
        MatchesSqlKeyword(word, "EXPLAIN") || MatchesSqlKeyword(word, "WITH");
}

bool MysqlCppHelper::isLastInsertIdSql(const std::string& sql)
{
    const char* word = sql.c_str() + FirstSqlWordOffset(sql);
    return MatchesSqlKeyword(word, "INSERT") || MatchesSqlKeyword(word, "REPLACE");
}

int32_t MysqlCppHelper::normalizeTimeoutSeconds(int32_t timeoutSeconds, int32_t defaultSeconds)
{
    if (timeoutSeconds > 0)
    {
        return timeoutSeconds;
    }
    if (defaultSeconds > 0)
    {
        return defaultSeconds;
    }
    return 1;
}

uint32_t MysqlCppHelper::toDriverIndex(uint32_t index)
{
    if (index == UINT32_MAX)
    {
        return 0;
    }
    return index + 1;
}

std::string MysqlCppHelper::exceptionText(const sql::SQLException& e)
{
    return std::string(e.what()) + " code=" + CStringManager::toStringInt32(e.getErrorCode());
}

std::string MysqlCppHelper::quoteCommandText(const std::string& text)
{
#ifdef _WIN32
    std::string escaped(1, '"');
    size_t slashes = 0;
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '\\')
        {
            ++slashes;
        }
        else
        {
            escaped.append(text[i] == '"' ? slashes * 2 + 1 : slashes, '\\');
            escaped.push_back(text[i]);
            slashes = 0;
        }
    }
    escaped.append(slashes * 2, '\\');
    escaped.push_back('"');
    return escaped;
#else
    std::string escaped;
    escaped.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '\'')
        {
            escaped += "'\\''";
        }
        else
        {
            escaped += text[i];
        }
    }
    return "'" + escaped + "'";
#endif
}

int MysqlCppHelper::runTool(const std::vector<std::string>& arguments, const std::string& path, bool input,
    const std::string& password)
{
    if (arguments.empty() || path.empty() || path.find('\0') != std::string::npos ||
        password.find('\0') != std::string::npos)
    {
        return -1;
    }
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        if (arguments[i].find('\0') != std::string::npos)
        {
            return -1;
        }
    }
#ifdef _WIN32
    std::string command;
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        if (i != 0)
        {
            command.push_back(' ');
        }
        command += quoteCommandText(arguments[i]);
    }
    std::wstring native = CStringManager::Utf8ToUnicode(command);
    const std::wstring fileName = CStringManager::Utf8ToUnicode(path);
    if (native.empty() || fileName.empty())
    {
        return -1;
    }
    // 凭据仅出现在该子进程的环境；并行导入/导出无需临时修改进程全局环境。
    const std::wstring credential = CStringManager::Utf8ToUnicode("MYSQL_PWD=" + password);
    std::unique_ptr<wchar_t, decltype(&FreeEnvironmentStringsW)> inherited(GetEnvironmentStringsW(), FreeEnvironmentStringsW);
    if (!inherited || credential.empty())
    {
        return -1;
    }
    std::wstring environment;
    bool inserted = false;
    for (const wchar_t* entry = inherited.get(); *entry != 0; entry += wcslen(entry) + 1)
    {
        const int order = _wcsnicmp(entry, L"MYSQL_PWD=", 9);
        if (!inserted && order >= 0)
        {
            environment.append(credential).push_back(0);
            inserted = true;
        }
        if (order != 0)
        {
            environment.append(entry).push_back(0);
        }
    }
    if (!inserted)
    {
        environment.append(credential).push_back(0);
    }
    environment.push_back(0);
    SECURITY_ATTRIBUTES security = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE file = CreateFileW(fileName.c_str(), input ? GENERIC_READ : GENERIC_WRITE, FILE_SHARE_READ,
        &security, input ? OPEN_EXISTING : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    HANDLE sink = CreateFileW(L"NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE handles[] = {file, sink};
    STARTUPINFOEXW startup = {};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = input ? file : sink;
    startup.StartupInfo.hStdOutput = input ? sink : file;
    startup.StartupInfo.hStdError = sink;
    SIZE_T bytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
    startup.lpAttributeList = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(HeapAlloc(GetProcessHeap(), 0, bytes));
    PROCESS_INFORMATION child = {};
    // 子进程仅继承这个 SQL 文件及空设备，避免它额外持有同进程中其他连接或服务的句柄。
    if (sink != INVALID_HANDLE_VALUE && startup.lpAttributeList &&
        InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &bytes))
    {
        if (UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
            handles, sizeof(handles), nullptr, nullptr))
        {
            CreateProcessW(nullptr, &native[0], nullptr, nullptr, TRUE,
                CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT,
                &environment[0], nullptr, &startup.StartupInfo, &child);
        }
        DeleteProcThreadAttributeList(startup.lpAttributeList);
    }
    if (startup.lpAttributeList)
    {
        HeapFree(GetProcessHeap(), 0, startup.lpAttributeList);
    }
    CloseHandle(file);
    if (sink != INVALID_HANDLE_VALUE)
    {
        CloseHandle(sink);
    }
    if (!child.hProcess)
    {
        return -1;
    }
    CloseHandle(child.hThread);
    DWORD code = 0;
    const bool finished = WaitForSingleObject(child.hProcess, INFINITE) == WAIT_OBJECT_0 &&
        GetExitCodeProcess(child.hProcess, &code);
    CloseHandle(child.hProcess);
    return finished ? static_cast<int>(code) : -1;
#else
    std::vector<char*> argv;
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        argv.push_back(const_cast<char*>(arguments[i].c_str()));
    }
    argv.push_back(nullptr);
    std::string credential = "MYSQL_PWD=" + password;
    std::vector<char*> environment;
    for (char** entry = environ; *entry != nullptr; ++entry)
    {
        if (std::strncmp(*entry, "MYSQL_PWD=", 9) != 0)
        {
            environment.push_back(*entry);
        }
    }
    environment.push_back(&credential[0]);
    environment.push_back(nullptr);
    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions) != 0)
    {
        return -1;
    }
    pid_t child;
    const bool started = posix_spawn_file_actions_addopen(&actions, input ? STDIN_FILENO : STDOUT_FILENO,
        path.c_str(), input ? O_RDONLY : O_WRONLY | O_CREAT | O_TRUNC, 0600) == 0 &&
        posix_spawnp(&child, argv[0], &actions, nullptr, argv.data(), environment.data()) == 0;
    posix_spawn_file_actions_destroy(&actions);
    if (!started)
    {
        return -1;
    }
    int status = 0;
    pid_t waited;
    do
    {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    return waited == child && WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

std::string MysqlCppHelper::mysqlToolPath(const std::string& toolName)
{
    const char* envValue = std::getenv("MYSQL_ROOT");
    std::string root = envValue == nullptr ? std::string() : std::string(envValue);
    if (root.empty())
    {
        return toolName;
    }
#ifdef _WIN32
    char slash = '\\';
#else
    char slash = '/';
#endif
    if (!root.empty() && root[root.size() - 1] != '/' && root[root.size() - 1] != '\\')
    {
        root.push_back(slash);
    }
    return root + "bin" + slash + toolName;
}