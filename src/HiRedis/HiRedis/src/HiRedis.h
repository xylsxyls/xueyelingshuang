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

	bool selectDbIndex(int32_t index) const;

	bool setKeyValue(const std::string& key, const std::string& value) const;

	bool setMap(const std::map<std::string, std::string>& map) const;

	bool setHashMap(const std::string& mapName, const std::map<std::string, std::string>& hashMap) const;

	bool setTable(const std::string& tableName, const std::vector<std::vector<std::string>>& table, int32_t key = -1, const std::string& split = ",") const;

	bool setGroup(const std::string& groupName, const std::string& element) const;

	bool setGroups(const std::string& groupName, const std::vector<std::string>& group) const;

	bool setOrderGroup(const std::string& groupName, int32_t score, const std::string& element) const;

	bool setOrderGroups(const std::string& groupName, const std::vector<std::pair<int32_t, std::string>>& orderGroup) const;

	bool keyExist(const std::string& key) const;

	bool hashMapExist(const std::string& mapName) const;

	bool tableExist(const std::string& tableName) const;

	bool groupExist(const std::string& key) const;

	bool orderGroupExist(const std::string& key) const;

	std::shared_ptr<HiRedisResultSet> getValue(const std::string& key) const;

	std::shared_ptr<HiRedisResultSet> getValues(const std::vector<std::string>& key) const;

	std::shared_ptr<HiRedisResultSet> getHashValues(const std::string& mapName, const std::vector<std::string>& field = std::vector<std::string>()) const;

	std::shared_ptr<HiRedisResultSet> getTableValues(const std::string& tableName, const std::vector<std::string>& field = std::vector<std::string>()) const;

	std::shared_ptr<HiRedisResultSet> getHashKeys(const std::string& tableName) const;

	std::shared_ptr<HiRedisResultSet> getGroup(const std::string& groupName) const;

	bool groupElementExist(const std::string& groupName, const std::string& element) const;

	int64_t getGroupCount(const std::string& groupName) const;

	std::shared_ptr<HiRedisResultSet> getOrderGroup(const std::string& groupName) const;

	bool orderGroupElementExist(const std::string& groupName, const std::string& element) const;

	int64_t getOrderGroupElementIndex(const std::string& groupName, const std::string& element) const;

	int64_t getOrderGroupElementScore(const std::string& groupName, const std::string& element) const;

	int64_t getOrderGroupCount(const std::string& groupName) const;

	std::shared_ptr<HiRedisResultSet> getOrderGroupByIndex(const std::string& groupName, int32_t min, int32_t max) const;

	std::shared_ptr<HiRedisResultSet> getOrderGroupByScore(const std::string& groupName, int32_t min, int32_t max) const;

	int64_t getOrderGroupCountByScore(const std::string& groupName, int32_t min, int32_t max) const;

	bool deleteKey(const std::string& key) const;

	bool deleteKeys(const std::vector<std::string>& key) const;

	bool deleteHashMap(const std::string& tableName) const;

	bool deleteHashMapField(const std::string& tableName, const std::string& field) const;

	bool deleteHashMapFields(const std::string& tableName, const std::vector<std::string>& field) const;

	bool deleteGroupElement(const std::string& groupName, const std::string& element) const;

	bool deleteGroupElements(const std::string& groupName, const std::vector<std::string>& element) const;

	bool deleteOrderGroupElement(const std::string& groupName, const std::string& element) const;

	bool deleteOrderGroupElements(const std::string& groupName, const std::vector<std::string>& element) const;

	int64_t deleteOrderGroupElementsByIndex(const std::string& groupName, int32_t min, int32_t max) const;

	int64_t deleteOrderGroupElementsByScore(const std::string& groupName, int32_t min, int32_t max) const;

protected:
	redisContext* m_redis;
};