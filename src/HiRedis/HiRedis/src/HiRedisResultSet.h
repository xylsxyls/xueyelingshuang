#pragma once
#include <map>
#include <string>

struct redisReply;
class HiRedisResultSet
{
public:
	HiRedisResultSet();

	~HiRedisResultSet();

public:
	std::string toString() const;

	std::map<std::string, std::string> toMap() const;

	std::map<std::string, std::string> toHashMap() const;

	std::vector<std::vector<std::string>> toTable() const;

protected:
	redisReply* m_reply;
};
