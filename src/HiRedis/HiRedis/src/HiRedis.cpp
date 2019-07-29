#include "HiRedis.h"
#include "hiredisinclude/hiredis.h"
#include "HiRedisResultSet.h"
#include "CStringManager/CStringManagerAPI.h"

#ifdef _DEBUG
#pragma comment(lib,"hiredis_d.lib")
#pragma comment(lib,"Win32_Interop_d.lib")
#else
#pragma comment(lib,"hiredis_r.lib")
#pragma comment(lib,"Win32_Interop_r.lib")
#endif

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

bool HiRedis::selectDbIndex(int32_t index)
{
	return command(CStringManager::Format("select %d", index))->toStatus();
}

bool HiRedis::setKeyValue(const std::string& key, const std::string& value)
{
	return command("set " + key + " " + value)->toStatus();
}

bool HiRedis::setMap(const std::map<std::string, std::string>& map)
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

bool HiRedis::setHashMap(const std::string& tableName, const std::map<std::string, std::string>& hashMap)
{
	if (hashMap.empty())
	{
		return false;
	}
	std::string strCommand = "hmset " + tableName;
	for (auto itValue = hashMap.begin(); itValue != hashMap.end(); ++itValue)
	{
		strCommand.append(" " + itValue->first + " " + itValue->second);
	}
	return command(strCommand)->toStatus();
}

bool HiRedis::setTable(const std::string& tableName, const std::vector<std::vector<std::string>>& table, int32_t key, const std::string& split)
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

std::shared_ptr<HiRedisResultSet> HiRedis::getHashValues(const std::string& tableName, const std::vector<std::string>& field) const
{
	if (field.empty())
	{
		return command("hgetall " + tableName);
	}
	std::string strCommand = "hmget " + tableName;
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

bool HiRedis::deleteKey(const std::string& key)
{
	return command("del " + key)->toInt64() == 1;
}

bool HiRedis::deleteKeys(const std::vector<std::string>& key)
{
	std::string strCommand = "del";
	int32_t index = -1;
	while (index++ != key.size() - 1)
	{
		strCommand.append(" " + key[index]);
	}
	return command(strCommand)->toInt64() == key.size();
}

bool HiRedis::deleteHashMap(const std::string& tableName)
{
	return command("del " + tableName)->toInt64() == 1;
}

bool HiRedis::deleteHashMapField(const std::string& tableName, const std::string& field)
{
	return command("hdel " + tableName + " " + field)->toInt64() == 1;
}

bool HiRedis::deleteHashMapFields(const std::string& tableName, const std::vector<std::string>& field)
{
	std::string strCommand = "hdel " + tableName;
	int32_t index = -1;
	while (index++ != field.size() - 1)
	{
		strCommand.append(" " + field[index]);
	}
	return command(strCommand)->toInt64() == field.size();
}

int main()
{
	HiRedis hiRedis;
	bool res = hiRedis.connect("127.0.0.1");

	res = hiRedis.selectDbIndex(1);

	std::vector<std::vector<std::string>> vecTable;
	std::vector<std::string> vecLine;
	vecLine.push_back("11");
	vecLine.push_back("22");
	vecLine.push_back("33");
	vecTable.push_back(vecLine);
	vecLine.clear();
	vecLine.push_back("44");
	vecLine.push_back("55");
	vecLine.push_back("66");
	vecTable.push_back(vecLine);
	hiRedis.setTable("aaaaa", vecTable, 1);
	std::vector<std::string> vecField;
	vecField.push_back("aaa");
	vecField.push_back("bbb");
	std::map<std::string, std::string> setmap;
	setmap["aaa"] = "111";
	setmap["bbb"] = "222";
	setmap["ccc"] = "333";
	hiRedis.setMap(setmap);
	auto spResultSet = hiRedis.deleteKey("aaa");
	//auto sss = spResultSet->toKeys();
	return 0;
}