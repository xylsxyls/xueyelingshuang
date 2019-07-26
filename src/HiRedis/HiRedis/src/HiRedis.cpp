#include "HiRedis.h"
#include "hiredisinclude/hiredis.h"
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

std::shared_ptr<HiRedisResultSet> HiRedis::command(const std::string& command)
{
	return std::shared_ptr<HiRedisResultSet>();
}

bool HiRedis::setKeyValue(const std::string& key, const std::string& value)
{
	return false;
}

bool HiRedis::setMap(const std::map<std::string, std::string>& map)
{
	return false;
}

bool HiRedis::setHashMap(const std::string& tableName, const std::map<std::string, std::string>& hashMap)
{
	return false;
}

bool HiRedis::setTable(const std::string& tableName, const std::vector<std::vector<std::string>>& table, int32_t key /*= -1*/)
{
	return false;
}

std::shared_ptr<HiRedisResultSet> HiRedis::getValue(const std::string& key) const
{
	return std::shared_ptr<HiRedisResultSet>();
}

std::shared_ptr<HiRedisResultSet> HiRedis::getValues(const std::vector<std::string>& key) const
{
	return std::shared_ptr<HiRedisResultSet>();
}

std::shared_ptr<HiRedisResultSet> HiRedis::getHashValues(const std::string& tableName, const std::vector<std::string>& field) const
{
	return std::shared_ptr<HiRedisResultSet>();
}

std::shared_ptr<HiRedisResultSet> HiRedis::getTableValues(const std::string& tableName, const std::vector<std::string>& key) const
{
	return std::shared_ptr<HiRedisResultSet>();
}