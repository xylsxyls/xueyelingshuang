#include "HttpServiceHelper.h"
#include <cctype>

bool HttpServiceHelper::hasHeader(const std::map<std::string, std::string>& headers, const std::string& name)
{
    // headerEquals入参：left和right是两个待比较的HTTP头名称。
    // headerEquals出参：无。
    // headerEquals返回值：返回true表示两个头名称在忽略大小写后相同。
    auto headerEquals = [](const std::string& left, const std::string& right) -> bool
    {
        if (left.size() != right.size())
        {
            return false;
        }
        for (size_t i = 0; i < left.size(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(left[i])) !=
                std::tolower(static_cast<unsigned char>(right[i])))
            {
                return false;
            }
        }
        return true;
    };

    std::map<std::string, std::string>::const_iterator it = headers.begin();
    for (; it != headers.end(); ++it)
    {
        if (headerEquals(it->first, name))
        {
            return true;
        }
    }
    return false;
}