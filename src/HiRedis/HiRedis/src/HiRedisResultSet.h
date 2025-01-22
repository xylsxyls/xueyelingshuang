#pragma once
#include <map>
#include <string>
#include <stdint.h>
#include <vector>
#include "HiRedisMacro.h"

struct redisReply;
class HiRedisAPI HiRedisResultSet
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

	std::vector<std::string> toGroup() const;

	bool toStatus() const;

	int32_t toCount() const;

	redisReply* toReply() const;

protected:
	bool check() const;

protected:
	redisReply* m_reply;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<std::string> m_vecKey;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};
