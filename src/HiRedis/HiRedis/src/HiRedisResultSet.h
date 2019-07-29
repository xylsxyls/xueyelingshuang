#pragma once
#include <map>
#include <string>
#include <stdint.h>

struct redisReply;
class HiRedisResultSet
{
public:
	HiRedisResultSet();

	~HiRedisResultSet();

public:
	void setReply(redisReply* reply);

	void setKeys(const std::vector<std::string>& vecKey);

	std::string toString() const;

	int64_t toInt64() const;

	bool isNull() const;

	std::vector<std::string> toKeys() const;

	std::map<std::string, std::string> toMap() const;

	std::map<std::string, std::string> toHashMap() const;

	std::vector<std::vector<std::string>> toTable(int32_t key = -1, const std::string& split = ",") const;

	bool toStatus() const;

protected:
	bool check() const;

protected:
	redisReply* m_reply;
	std::vector<std::string> m_vecKey;
};
