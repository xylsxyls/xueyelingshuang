#include "HiRedisHelper.h"
#include "HiRedisResultSet.h"
#include <cctype>
#include <limits>

bool HiRedisHelper::integerReplyOk(const std::shared_ptr<HiRedisResultSet>& result)
{
    return result.get() != nullptr && !result->isError() && result->toInt64() >= 0;
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

        current.push_back(ch);
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