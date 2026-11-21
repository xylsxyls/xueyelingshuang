#include "HttpServiceHelper.h"
#include <cstring>

bool HttpServiceHelper::validName(const std::string& name)
{
    for (unsigned char c : name)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) { continue; }
        if (c == 0 || std::strchr("!#$%&'*+-.^_`|~", c) == nullptr) { return false; }
    }
    return !name.empty();
}

bool HttpServiceHelper::validValue(const std::string& value)
{
    for (unsigned char c : value)
    {
        if ((c < 32 && c != '\t') || c == 127) { return false; }
    }
    return true;
}

bool HttpServiceHelper::sameName(const std::string& left, const std::string& right)
{
    if (left.size() != right.size()) { return false; }
    for (size_t i = 0; i < left.size(); ++i)
    {
        const char a = left[i], b = right[i];
        if ((a >= 'A' && a <= 'Z' ? a + 32 : a) != (b >= 'A' && b <= 'Z' ? b + 32 : b)) { return false; }
    }
    return true;
}

bool HttpServiceHelper::hasHeader(const std::map<std::string, std::string>& headers, const std::string& name)
{
    for (const auto& field : headers)
    {
        if (sameName(field.first, name) && validValue(field.second)) { return true; }
    }
    return false;
}