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

	~HiRedis();

public:
	bool connect(const std::string& ip, int32_t port = 6379);

	std::shared_ptr<HiRedisResultSet> command(const std::string& command) const;

	bool selectDbIndex(int32_t index);

	bool setKeyValue(const std::string& key, const std::string& value);

	bool setMap(const std::map<std::string, std::string>& map);

	bool setHashMap(const std::string& tableName, const std::map<std::string, std::string>& hashMap);

	bool setTable(const std::string& tableName, const std::vector<std::vector<std::string>>& table, int32_t key = -1, const std::string& split = ",");

	std::shared_ptr<HiRedisResultSet> getValue(const std::string& key) const;

	std::shared_ptr<HiRedisResultSet> getValues(const std::vector<std::string>& key) const;

	std::shared_ptr<HiRedisResultSet> getHashValues(const std::string& tableName, const std::vector<std::string>& field = std::vector<std::string>()) const;

	std::shared_ptr<HiRedisResultSet> getTableValues(const std::string& tableName, const std::vector<std::string>& field = std::vector<std::string>()) const;

	std::shared_ptr<HiRedisResultSet> getHashKeys(const std::string& tableName) const;

	bool deleteKey(const std::string& key);

	bool deleteKeys(const std::vector<std::string>& key);

	bool deleteHashMap(const std::string& tableName);

	bool deleteHashMapField(const std::string& tableName, const std::string& field);

	bool deleteHashMapFields(const std::string& tableName, const std::vector<std::string>& field);

protected:
	redisContext* m_redis;
};