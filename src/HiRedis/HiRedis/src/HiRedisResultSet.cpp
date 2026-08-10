#include "HiRedisResultSet.h"
#include "CStringManager/CStringManagerAPI.h"
#include <hiredis.h>

HiRedisResultSet::HiRedisResultSet() :
m_reply(nullptr)
{

}

HiRedisResultSet::~HiRedisResultSet()
{
    if (m_reply != nullptr)
    {
        freeReplyObject(m_reply);
        m_reply = nullptr;
    }
}

bool HiRedisResultSet::isValid() const
{
    return m_reply != nullptr && m_error.empty();
}

bool HiRedisResultSet::isError() const
{
    return !m_error.empty() || (m_reply != nullptr && m_reply->type == REDIS_REPLY_ERROR);
}

std::string HiRedisResultSet::error() const
{
    if (!m_error.empty())
    {
        return m_error;
    }
    if (m_reply != nullptr && m_reply->type == REDIS_REPLY_ERROR && m_reply->str != nullptr)
    {
        return std::string(m_reply->str, m_reply->len);
    }
    return "";
}

int32_t HiRedisResultSet::type() const
{
    return m_reply == nullptr ? 0 : m_reply->type;
}

size_t HiRedisResultSet::size() const
{
    if (m_reply == nullptr)
    {
        return 0;
    }
    if (m_reply->type == REDIS_REPLY_ARRAY)
    {
        return m_reply->elements;
    }
    return 1;
}

std::string HiRedisResultSet::at(size_t index) const
{
    if (m_reply == nullptr)
    {
        return "";
    }
    if (m_reply->type != REDIS_REPLY_ARRAY)
    {
        return index == 0 ? replyToString(m_reply) : "";
    }
    if (index >= m_reply->elements || m_reply->element[index] == nullptr)
    {
        return "";
    }
    return replyToString(m_reply->element[index]);
}

void HiRedisResultSet::setReply(redisReply* reply)
{
    if (m_reply != nullptr)
    {
        freeReplyObject(m_reply);
    }
    m_reply = reply;
    m_vecKey.clear();
    m_error.clear();
    if (m_reply != nullptr && m_reply->type == REDIS_REPLY_ERROR && m_reply->str != nullptr)
    {
        m_error.assign(m_reply->str, m_reply->len);
    }
}

void HiRedisResultSet::setKeys(const std::vector<std::string>& vecKey)
{
    m_vecKey = vecKey;
}

void HiRedisResultSet::setError(const std::string& error)
{
    m_error = error;
}

std::string HiRedisResultSet::toString() const
{
    return replyToString(m_reply);
}

int64_t HiRedisResultSet::toInt64() const
{
    if (!check())
    {
        return 0;
    }
    if (m_reply->type == REDIS_REPLY_INTEGER)
    {
        return m_reply->integer;
    }
    if ((m_reply->type == REDIS_REPLY_STRING || m_reply->type == REDIS_REPLY_STATUS) && m_reply->str != nullptr)
    {
        std::string value(m_reply->str, m_reply->len);
        return CStringManager::atoi64(value.c_str());
    }
    return 0;
}

bool HiRedisResultSet::isNull() const
{
    return !check() || m_reply->type == REDIS_REPLY_NIL;
}

std::vector<std::string> HiRedisResultSet::toKeys() const
{
    std::vector<std::string> result;
    if (!check())
    {
        return result;
    }
    if (m_reply->type != REDIS_REPLY_ARRAY)
    {
        result.push_back(replyToString(m_reply));
        return result;
    }

    for (size_t i = 0; i < m_reply->elements; ++i)
    {
        result.push_back(replyToString(m_reply->element[i]));
    }
    return result;
}

std::map<std::string, std::string> HiRedisResultSet::toMap() const
{
    std::map<std::string, std::string> result;
    if (!check() || m_reply->type != REDIS_REPLY_ARRAY)
    {
        return result;
    }

    if (!m_vecKey.empty())
    {
        size_t count = m_vecKey.size() < m_reply->elements ? m_vecKey.size() : m_reply->elements;
        for (size_t i = 0; i < count; ++i)
        {
            if (m_reply->element[i] == nullptr || m_reply->element[i]->type == REDIS_REPLY_NIL)
            {
                continue;
            }
            result[m_vecKey[i]] = replyToString(m_reply->element[i]);
        }
        return result;
    }

    for (size_t i = 0; i + 1 < m_reply->elements; i += 2)
    {
        result[replyToString(m_reply->element[i])] = replyToString(m_reply->element[i + 1]);
    }
    return result;
}

std::map<std::string, std::string> HiRedisResultSet::toHashMap() const
{
    return toMap();
}

std::vector<std::vector<std::string>> HiRedisResultSet::toTable(int32_t key, const std::string& split) const
{
    std::vector<std::vector<std::string>> result;
    if (!check() || m_reply->type != REDIS_REPLY_ARRAY)
    {
        return result;
    }

    if (!m_vecKey.empty())
    {
        size_t count = m_vecKey.size() < m_reply->elements ? m_vecKey.size() : m_reply->elements;
        for (size_t i = 0; i < count; ++i)
        {
            if (m_reply->element[i] == nullptr || m_reply->element[i]->type == REDIS_REPLY_NIL)
            {
                continue;
            }
            std::vector<std::string> row = CStringManager::split(replyToString(m_reply->element[i]), split);
            if (key >= 0 && key <= static_cast<int32_t>(row.size()))
            {
                row.insert(row.begin() + key, m_vecKey[i]);
            }
            result.push_back(row);
        }
        return result;
    }

    for (size_t i = 0; i + 1 < m_reply->elements; i += 2)
    {
        if (m_reply->element[i] == nullptr || m_reply->element[i + 1] == nullptr || m_reply->element[i + 1]->type == REDIS_REPLY_NIL)
        {
            continue;
        }
        std::vector<std::string> row = CStringManager::split(replyToString(m_reply->element[i + 1]), split);
        if (key >= 0 && key <= static_cast<int32_t>(row.size()))
        {
            row.insert(row.begin() + key, replyToString(m_reply->element[i]));
        }
        result.push_back(row);
    }
    return result;
}

std::vector<std::string> HiRedisResultSet::toGroup() const
{
    return toKeys();
}

bool HiRedisResultSet::toStatus() const
{
    if (!check())
    {
        return false;
    }
    if (m_reply->type == REDIS_REPLY_STATUS && m_reply->str != nullptr)
    {
        return std::string(m_reply->str, m_reply->len) == "OK";
    }
    if (m_reply->type == REDIS_REPLY_INTEGER)
    {
        return m_reply->integer >= 0;
    }
    return false;
}

int32_t HiRedisResultSet::toCount() const
{
    if (!check())
    {
        return 0;
    }
    if (m_reply->type == REDIS_REPLY_ARRAY)
    {
        return static_cast<int32_t>(m_reply->elements);
    }
    if (m_reply->type == REDIS_REPLY_INTEGER)
    {
        return static_cast<int32_t>(m_reply->integer);
    }
    return 0;
}

redisReply* HiRedisResultSet::toReply() const
{
    return m_reply;
}

bool HiRedisResultSet::check() const
{
    return m_reply != nullptr;
}

std::string HiRedisResultSet::replyToString(const redisReply* reply) const
{
    if (reply == nullptr)
    {
        return "";
    }
    if ((reply->type == REDIS_REPLY_STRING || reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR) && reply->str != nullptr)
    {
        return std::string(reply->str, reply->len);
    }
    if (reply->type == REDIS_REPLY_INTEGER)
    {
        return CStringManager::toStringInt64(reply->integer);
    }
    return "";
}