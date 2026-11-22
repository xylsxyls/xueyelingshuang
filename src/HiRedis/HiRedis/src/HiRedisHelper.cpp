#include "HiRedisHelper.h"
#include "HiRedisConfig.h"
#include "HiRedisResultSet.h"
#include "hiredisinclude/read.h"
#include <cctype>
#include <limits>

const char* HiRedisHelper::connectionConfigError(const HiRedisConfig& config)
{
    if (config.m_port < 1 || config.m_port > 65535)
    {
        return "TCP port must be between 1 and 65535";
    }
    if (config.m_ip.empty() || config.m_ip.find('\0') != std::string::npos)
    {
        return "redis host must be nonempty and cannot contain NUL";
    }
    if (config.m_keepAliveIdleSeconds <= 0 || config.m_keepAliveIdleSeconds > INT32_MAX / 1000)
    {
        return "redis keepalive idle seconds are out of range";
    }
    if (config.m_readerMaxUnusedBytes != -1 && (config.m_readerMaxUnusedBytes <= 0 ||
        static_cast<uint64_t>(config.m_readerMaxUnusedBytes) > (std::numeric_limits<size_t>::max)()))
    {
        return "redis unused reader buffer threshold is out of range";
    }
    if (config.m_readerMaxElements != -1 && config.m_readerMaxElements <= 0)
    {
        return "redis reader element limit is out of range";
    }
    return nullptr;
}

bool HiRedisHelper::integerReplyOk(const std::shared_ptr<HiRedisResultSet>& result)
{
    return result && result->type() == REDIS_REPLY_INTEGER && result->toStatus();
}

bool HiRedisHelper::splitCommandText(const std::string& commandText, std::vector<std::string>& args, std::string& error)
{
    args.clear();
    error.clear();
    std::string current;
    bool inQuote = false;
    bool isEscape = false;
    bool tokenStarted = false;
    char quoteChar = '\0';

    for (size_t i = 0; i < commandText.size(); ++i)
    {
        char ch = commandText[i];
        if (isEscape)
        {
            current.push_back(ch);
            tokenStarted = true;
            isEscape = false;
            continue;
        }

        if (ch == '\\')
        {
            isEscape = true;
            tokenStarted = true;
            continue;
        }

        if (inQuote)
        {
            if (ch == quoteChar)
            {
                inQuote = false;
            }
            else
            {
                current.push_back(ch);
            }
            tokenStarted = true;
            continue;
        }

        if (ch == '\'' || ch == '"')
        {
            inQuote = true;
            quoteChar = ch;
            tokenStarted = true;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(ch)) != 0)
        {
            if (tokenStarted)
            {
                args.push_back(current);
                current.clear();
                tokenStarted = false;
            }
            continue;
        }

        // 常见命令参数由可见 ASCII 组成，整段复制后再交回引号和转义状态机。
        size_t end = i + 1;
        while (end < commandText.size())
        {
            const unsigned char next = static_cast<unsigned char>(commandText[end]);
            if (next <= ' ' || next >= 128 || next == '\\' || next == '\'' || next == '"')
            {
                break;
            }
            ++end;
        }
        current.append(commandText, i, end - i);
        i = end - 1;
        tokenStarted = true;
    }

    if (isEscape)
    {
        current.push_back('\\');
    }
    if (inQuote)
    {
        error = "redis command text has unclosed quote";
        args.clear();
        return false;
    }
    if (tokenStarted)
    {
        args.push_back(current);
    }
    if (args.empty())
    {
        error = "redis command text is empty";
        return false;
    }
    return true;
}

bool HiRedisHelper::makeArgv(const std::vector<std::string>& args, std::vector<const char*>& argv, std::vector<size_t>& argvlen, std::string& error)
{
    argv.clear();
    argvlen.clear();
    error.clear();

    if (args.empty())
    {
        error = "redis command args is empty";
        return false;
    }
    if (args.size() > static_cast<size_t>((std::numeric_limits<int32_t>::max)()))
    {
        error = "redis command args is too many";
        return false;
    }

    argv.reserve(args.size());
    argvlen.reserve(args.size());
    for (size_t i = 0; i < args.size(); ++i)
    {
        argv.push_back(args[i].data());
        argvlen.push_back(args[i].size());
    }
    return true;
}