#pragma once
#include "HiRedisMacro.h"
#include <memory>
#include <map>

class HiRedisResultSet;
struct redisContext;
class HiRedisAPI HiRedis
{
public:
	HiRedis();

public:
	bool connect(const std::string& ip, int32_t port = 6379);

	std::shared_ptr<HiRedisResultSet> command(const std::string& command);

	bool setKeyValue(const std::string& key, const std::string& value);

	bool setMap(const std::map<std::string, std::string>& map);

	bool setHashMap(const std::string& tableName, const std::map<std::string, std::string>& hashMap);

	bool setTable(const std::string& tableName, const std::vector<std::vector<std::string>>& table, int32_t key = -1);

	std::shared_ptr<HiRedisResultSet> getValue(const std::string& key) const;

	std::shared_ptr<HiRedisResultSet> getValues(const std::vector<std::string>& key) const;

	std::shared_ptr<HiRedisResultSet> getHashValues(const std::string& tableName, const std::vector<std::string>& field) const;

	std::shared_ptr<HiRedisResultSet> getTableValues(const std::string& tableName, const std::vector<std::string>& key) const;

protected:
	redisContext* m_redis;
};