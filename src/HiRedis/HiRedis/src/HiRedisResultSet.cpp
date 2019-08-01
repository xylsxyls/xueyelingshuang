#include "HiRedisResultSet.h"
#include "hiredisinclude/hiredis.h"
#include "CStringManager/CStringManagerAPI.h"

HiRedisResultSet::HiRedisResultSet():
m_reply(nullptr)
{

}

HiRedisResultSet::~HiRedisResultSet()
{
	if (m_reply != nullptr)
	{
		freeReplyObject(m_reply);
	}
}

void HiRedisResultSet::setReply(redisReply* reply)
{
	if (m_reply != nullptr)
	{
		freeReplyObject(m_reply);
	}
	m_reply = reply;
}

void HiRedisResultSet::setKeys(const std::vector<std::string>& vecKey)
{
	m_vecKey = vecKey;
}

std::string HiRedisResultSet::toString() const
{
	if (!check() || (m_reply->type != REDIS_REPLY_STRING))
	{
		return "";
	}
	return m_reply->str;
}

int64_t HiRedisResultSet::toInt64() const
{
	if (!check() || (m_reply->type != REDIS_REPLY_INTEGER))
	{
		return 0;
	}
	return m_reply->integer;
}

bool HiRedisResultSet::isNull() const
{
	if (!check() || (m_reply->type != REDIS_REPLY_NIL))
	{
		return false;
	}
	return true;
}

std::vector<std::string> HiRedisResultSet::toKeys() const
{
	std::vector<std::string> result;
	if (!check())
	{
		return result;
	}

	int32_t index = -1;
	while (index++ != m_reply->elements - 1)
	{
		result.push_back(m_reply->element[index]->str);
	}
	return result;
}

std::map<std::string, std::string> HiRedisResultSet::toMap() const
{
	std::map<std::string, std::string> result;
	if (!check())
	{
		return result;
	}

	int32_t index = -1;
	while (index++ != m_reply->elements - 1)
	{
		if (m_reply->element[index]->type == REDIS_REPLY_NIL)
		{
			continue;
		}
		std::string field = m_vecKey.empty() ? m_reply->element[index++]->str : m_vecKey[index];
		result[field] = m_reply->element[index]->str;
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
	if (!check())
	{
		return result;
	}

	int32_t index = -1;
	while (index++ != m_reply->elements - 1)
	{
		std::string field = m_vecKey.empty() ? m_reply->element[index++]->str : m_vecKey[index];
		std::vector<std::string> vecLine = CStringManager::split(m_reply->element[index]->str, split);
		if (key != -1)
		{
			vecLine.insert(vecLine.begin() + key, field);
		}
		result.push_back(vecLine);
	}
	return result;
}

std::vector<std::string> HiRedisResultSet::toGroup() const
{
	return toKeys();
}

bool HiRedisResultSet::toStatus() const
{
	if (!check() || (m_reply->type != REDIS_REPLY_STATUS))
	{
		return false;
	}
	return (std::string(m_reply->str) == "OK");
}

bool HiRedisResultSet::check() const
{
	return m_reply != nullptr;
}