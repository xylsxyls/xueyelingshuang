#include "HiRedis.h"
#include "hiredisinclude/hiredis.h"
#include "HiRedisResultSet.h"
#include "CStringManager/CStringManagerAPI.h"

HiRedis::HiRedis():
m_redis(nullptr)
{

}

HiRedis::~HiRedis()
{
	if (m_redis != nullptr)
	{
		redisFree(m_redis);
	}
}

bool HiRedis::connect(const std::string& ip, int32_t port)
{
	m_redis = redisConnect(ip.c_str(), port);
	if (m_redis == nullptr || m_redis->err)
	{
		redisFree(m_redis);
		return false;
	}
	return true;
}

std::shared_ptr<HiRedisResultSet> HiRedis::command(const std::string& command) const
{
	std::shared_ptr<HiRedisResultSet> spHiRedisResultSet(new HiRedisResultSet);
	spHiRedisResultSet->setReply((redisReply*)redisCommand(m_redis, command.c_str()));
	return spHiRedisResultSet;
}

bool HiRedis::selectDbIndex(int32_t index) const
{
	return command(CStringManager::Format("select %d", index))->toStatus();
}

bool HiRedis::setKeyValue(const std::string& key, const std::string& value) const
{
	return command("set " + key + " " + value)->toStatus();
}

bool HiRedis::setMap(const std::map<std::string, std::string>& map) const
{
	if (map.empty())
	{
		return false;
	}
	std::string strCommand = "mset";
	for (auto itValue = map.begin(); itValue != map.end(); ++itValue)
	{
		strCommand.append(" " + itValue->first + " " + itValue->second);
	}
	return command(strCommand)->toStatus();
}

bool HiRedis::setHashMap(const std::string& mapName, const std::map<std::string, std::string>& hashMap) const
{
	if (hashMap.empty())
	{
		return false;
	}
	std::string strCommand = "hmset " + mapName;
	for (auto itValue = hashMap.begin(); itValue != hashMap.end(); ++itValue)
	{
		strCommand.append(" " + itValue->first + " " + itValue->second);
	}
	return command(strCommand)->toStatus();
}

bool HiRedis::setTable(const std::string& tableName, const std::vector<std::vector<std::string>>& table, int32_t key, const std::string& split) const
{
	if (table.empty())
	{
		return false;
	}
	
	std::string strCommand = "hmset " + tableName;
	int32_t lineIndex = -1;
	while (lineIndex++ != table.size() - 1)
	{
		int32_t cowIndex = -1;
		std::string lineValue;
		const std::vector<std::string>& vecLine = table[lineIndex];
		while (cowIndex++ != vecLine.size() - 2)
		{
			if (cowIndex == key)
			{
				continue;
			}
			lineValue.append(vecLine[cowIndex] + split);
		}
		if (cowIndex != key)
		{
			lineValue.append(vecLine[cowIndex]);
		}
		strCommand.append(CStringManager::Format(" %s %s", (key == -1) ? CStringManager::Format("%d",lineIndex).c_str() : vecLine[key].c_str(), lineValue.c_str()));
	}
	return command(strCommand)->toStatus();
}

bool HiRedis::setGroup(const std::string& groupName, const std::string& element) const
{
	return command("sadd " + groupName + " " + element)->toInt64() == 1;
}

bool HiRedis::setGroups(const std::string& groupName, const std::vector<std::string>& group) const
{
	std::string strCommand = "sadd " + groupName;
	int32_t index = -1;
	while (index++ != group.size() - 1)
	{
		strCommand.append(" " + group[index]);
	}
	return command(strCommand)->toInt64() == group.size();
}

bool HiRedis::setOrderGroup(const std::string& groupName, int32_t score, const std::string& element) const
{
	return command(CStringManager::Format("zadd %s %d %s", groupName.c_str(), score, element.c_str()))->toInt64() == 1;
}

bool HiRedis::setOrderGroups(const std::string& groupName, const std::vector<std::pair<int32_t, std::string>>& orderGroup) const
{
	std::string strCommand = "zadd " + groupName;
	int32_t index = -1;
	while (index++ != orderGroup.size() - 1)
	{
		strCommand.append(CStringManager::Format(" %d %s", orderGroup[index].first, orderGroup[index].second.c_str()));
	}
	return command(strCommand)->toInt64() == orderGroup.size();
}

bool HiRedis::keyExist(const std::string& key) const
{
	return command("exists " + key)->toInt64() == 1;
}

bool HiRedis::hashMapExist(const std::string& mapName) const
{
	return keyExist(mapName);
}

bool HiRedis::tableExist(const std::string& tableName) const
{
	return keyExist(tableName);
}

bool HiRedis::groupExist(const std::string& key) const
{
	return keyExist(key);
}

bool HiRedis::orderGroupExist(const std::string& key) const
{
	return keyExist(key);
}

std::shared_ptr<HiRedisResultSet> HiRedis::getValue(const std::string& key) const
{
	return command("get " + key);
}

std::shared_ptr<HiRedisResultSet> HiRedis::getValues(const std::vector<std::string>& key) const
{
	std::string strCommand = "mget";
	int32_t index = -1;
	while (index++ != key.size() - 1)
	{
		strCommand.append(" " + key[index]);
	}
	std::shared_ptr<HiRedisResultSet> spResultSet = command(strCommand);
	spResultSet->setKeys(key);
	return spResultSet;
}

std::shared_ptr<HiRedisResultSet> HiRedis::getHashValues(const std::string& mapName, const std::vector<std::string>& field) const
{
	if (field.empty())
	{
		return command("hgetall " + mapName);
	}
	std::string strCommand = "hmget " + mapName;
	int32_t index = -1;
	while (index++ != field.size() - 1)
	{
		strCommand.append(" " + field[index]);
	}
	std::shared_ptr<HiRedisResultSet> spResultSet = command(strCommand);
	spResultSet->setKeys(field);
	return spResultSet;
}

std::shared_ptr<HiRedisResultSet> HiRedis::getTableValues(const std::string& tableName, const std::vector<std::string>& field) const
{
	std::shared_ptr<HiRedisResultSet> spResultSet = getHashValues(tableName, field);
	spResultSet->setKeys(field);
	return spResultSet;
}

std::shared_ptr<HiRedisResultSet> HiRedis::getHashKeys(const std::string& tableName) const
{
	return command("hkeys " + tableName);
}

std::shared_ptr<HiRedisResultSet> HiRedis::getGroup(const std::string& key) const
{
	return command("smembers " + key);
}

bool HiRedis::groupElementExist(const std::string& groupName, const std::string& element) const
{
	return command("sismember " + groupName + " " + element)->toInt64() == 1;
}

int64_t HiRedis::getGroupCount(const std::string& groupName) const
{
	return command("scard " + groupName)->toInt64();
}

std::shared_ptr<HiRedisResultSet> HiRedis::getOrderGroup(const std::string& groupName) const
{
	return command(CStringManager::Format("zrange %s 0 %d", groupName.c_str(), getOrderGroupCount(groupName) - 1));
}

bool HiRedis::orderGroupElementExist(const std::string& groupName, const std::string& element) const
{
	return !(command("zrank " + groupName + " " + element)->isNull());
}

int64_t HiRedis::getOrderGroupElementIndex(const std::string& groupName, const std::string& element) const
{
	std::shared_ptr<HiRedisResultSet> spResultSet = command("zrank " + groupName + " " + element);
	if (spResultSet->isNull())
	{
		return -1;
	}
	return spResultSet->toInt64();
}

int64_t HiRedis::getOrderGroupElementScore(const std::string& groupName, const std::string& element) const
{
	std::shared_ptr<HiRedisResultSet> spResultSet = command("zscore " + groupName + " " + element);
	if (spResultSet->isNull())
	{
		return -1;
	}
	return CStringManager::atoi64(spResultSet->toString().c_str());
}

int64_t HiRedis::getOrderGroupCount(const std::string& groupName) const
{
	return command("zcard " + groupName)->toInt64();
}

std::shared_ptr<HiRedisResultSet> HiRedis::getOrderGroupByIndex(const std::string& groupName, int32_t min, int32_t max) const
{
	return command(CStringManager::Format("zrange %s %d %d", groupName.c_str(), min, max));
}

std::shared_ptr<HiRedisResultSet> HiRedis::getOrderGroupByScore(const std::string& groupName, int32_t min, int32_t max) const
{
	return command(CStringManager::Format("zrangebyscore %s %d %d", groupName.c_str(), min, max));
}

int64_t HiRedis::getOrderGroupCountByScore(const std::string& groupName, int32_t min, int32_t max) const
{
	return command(CStringManager::Format("zcount %s %d %d", groupName.c_str(), min, max))->toInt64();
}

bool HiRedis::deleteKey(const std::string& key) const
{
	return command("del " + key)->toInt64() == 1;
}

bool HiRedis::deleteKeys(const std::vector<std::string>& key) const
{
	std::string strCommand = "del";
	int32_t index = -1;
	while (index++ != key.size() - 1)
	{
		strCommand.append(" " + key[index]);
	}
	return command(strCommand)->toInt64() == key.size();
}

bool HiRedis::deleteHashMap(const std::string& tableName) const
{
	return command("del " + tableName)->toInt64() == 1;
}

bool HiRedis::deleteHashMapField(const std::string& tableName, const std::string& field) const
{
	return command("hdel " + tableName + " " + field)->toInt64() == 1;
}

bool HiRedis::deleteHashMapFields(const std::string& tableName, const std::vector<std::string>& field) const
{
	std::string strCommand = "hdel " + tableName;
	int32_t index = -1;
	while (index++ != field.size() - 1)
	{
		strCommand.append(" " + field[index]);
	}
	return command(strCommand)->toInt64() == field.size();
}

bool HiRedis::deleteGroupElement(const std::string& groupName, const std::string& element) const
{
	return command("srem " + groupName + " " + element)->toInt64() == 1;
}

bool HiRedis::deleteGroupElements(const std::string& groupName, const std::vector<std::string>& element) const
{
	std::string strCommand = "srem " + groupName;
	int32_t index = -1;
	while (index++ != element.size() - 1)
	{
		strCommand.append(" " + element[index]);
	}
	return command(strCommand)->toInt64() == element.size();
}

bool HiRedis::deleteOrderGroupElement(const std::string& groupName, const std::string& element) const
{
	return command("zrem " + groupName + " " + element)->toInt64() == 1;
}

bool HiRedis::deleteOrderGroupElements(const std::string& groupName, const std::vector<std::string>& element) const
{
	std::string strCommand = "zrem " + groupName;
	int32_t index = -1;
	while (index++ != element.size() - 1)
	{
		strCommand.append(" " + element[index]);
	}
	return command(strCommand)->toInt64() == element.size();
}

int64_t HiRedis::deleteOrderGroupElementsByIndex(const std::string& groupName, int32_t min, int32_t max) const
{
	return command(CStringManager::Format("zremrangebyrank %s %d %d", groupName.c_str(), min, max))->toInt64();
}

int64_t HiRedis::deleteOrderGroupElementsByScore(const std::string& groupName, int32_t min, int32_t max) const
{
	return command(CStringManager::Format("zremrangebyscore %s %d %d", groupName.c_str(), min, max))->toInt64();
}

//int main()
//{
//	HiRedis hiRedis;
//	bool res = hiRedis.connect("127.0.0.1");
//
//	res = hiRedis.selectDbIndex(0);
//	hiRedis.setOrderGroup("zadds", 20190102, "2019-01-02");
//	hiRedis.setOrderGroup("zadds", 20190104, "2019-01-04");
//	hiRedis.setOrderGroup("zadds", 20190108, "2019-01-08");
//	auto ssss = hiRedis.deleteOrderGroupElementsByScore("zadds", 20190103, 20190107);
//	auto ssdsssds = hiRedis.getOrderGroup("zadds")->toGroup();
//
//	std::vector<std::vector<std::string>> vecTable;
//	std::vector<std::string> vecLine;
//	vecLine.push_back("11");
//	vecLine.push_back("22");
//	vecLine.push_back("33");
//	vecTable.push_back(vecLine);
//	vecLine.clear();
//	vecLine.push_back("44");
//	vecLine.push_back("55");
//	vecLine.push_back("66");
//	vecTable.push_back(vecLine);
//	hiRedis.setTable("aaaaa", vecTable, 1);
//	std::vector<std::string> vecField;
//	vecField.push_back("aaa");
//	vecField.push_back("bbb");
//	std::map<std::string, std::string> setmap;
//	setmap["aaa"] = "111";
//	setmap["bbb"] = "222";
//	setmap["ccc"] = "333";
//	hiRedis.setMap(setmap);
//	auto spResultSet = hiRedis.deleteKey("aaa");
//	//auto sss = spResultSet->toKeys();
//	return 0;
//}