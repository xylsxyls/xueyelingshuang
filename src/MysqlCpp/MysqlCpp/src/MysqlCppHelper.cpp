#include "MysqlCppHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cctype>
#include <cstdlib>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include "cppconn/exception.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

std::string MysqlCppHelper::firstSqlWord(const std::string& sql)
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
                return "";
            }
            index = end + 2;
            continue;
        }
        break;
    }

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
        word[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(word[i])));
    }
    return word;
}

bool MysqlCppHelper::isQuerySql(const std::string& sql)
{
    std::string word = firstSqlWord(sql);
    return word == "SELECT" || word == "SHOW" || word == "DESC" ||
        word == "DESCRIBE" || word == "EXPLAIN" || word == "WITH";
}

bool MysqlCppHelper::isLastInsertIdSql(const std::string& sql)
{
    std::string word = firstSqlWord(sql);
    return word == "INSERT" || word == "REPLACE";
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
    std::string escaped;
    escaped.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '"')
        {
            escaped += "\\\"";
        }
        else
        {
            escaped += text[i];
        }
    }
    return "\"" + escaped + "\"";
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