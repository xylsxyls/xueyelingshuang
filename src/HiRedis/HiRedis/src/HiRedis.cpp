#include "HiRedis.h"
#include "HiRedisHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include <hiredis.h>

HiRedis::HiRedis() :
m_redis(nullptr),
m_isInit(false)
{

}

HiRedis::~HiRedis()
{
    uninit();
}

bool HiRedis::init(const std::string& ip, int32_t port)
{
    HiRedisConfig config;
    config.m_ip = ip;
    config.m_port = port;
    return init(config);
}

bool HiRedis::init(const HiRedisConfig& config)
{
    // toTimeval入参：timeoutMs是毫秒级超时时间，小于等于0时使用默认值。
    // toTimeval出参：无。
    // toTimeval返回值：返回hiredis连接和命令超时需要的timeval结构。
    auto toTimeval = [](int32_t timeoutMs) -> struct timeval
    {
        if (timeoutMs <= 0)
        {
            timeoutMs = 3000;
        }
        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        return tv;
    };

    std::lock_guard<std::mutex> lock(m_mutex);
    closeNoLock();
    m_config = config;
    m_isInit = false;

    struct timeval connectTimeout = toTimeval(config.m_connectTimeoutMs);
    m_redis = redisConnectWithTimeout(config.m_ip.c_str(), config.m_port, connectTimeout);
    if (m_redis == nullptr)
    {
        setLastError("redis connect returned null");
        return false;
    }
    if (m_redis->err)
    {
        setLastError(m_redis->errstr == nullptr ? "redis connect failed" : m_redis->errstr);
        closeNoLock();
        return false;
    }

    struct timeval commandTimeout = toTimeval(config.m_commandTimeoutMs);
    redisSetTimeout(m_redis, commandTimeout);
#ifdef _WIN32
    if (config.m_enableKeepAlive)
    {
        redisEnableKeepAlive(m_redis);
    }
#endif

    if (!config.m_password.empty())
    {
        std::shared_ptr<HiRedisResultSet> auth = commandArgvNoLock(std::vector<std::string>{ "AUTH", config.m_password });
        if (!auth->toStatus())
        {
            setLastError(auth->error().empty() ? "redis auth failed" : auth->error());
            closeNoLock();
            return false;
        }
    }

    if (config.m_dbIndex >= 0)
    {
        std::shared_ptr<HiRedisResultSet> select = commandArgvNoLock(std::vector<std::string>{ "SELECT", CStringManager::toStringInt32(config.m_dbIndex) });
        if (!select->toStatus())
        {
            setLastError(select->error().empty() ? "redis select failed" : select->error());
            closeNoLock();
            return false;
        }
    }

    setLastError("");
    m_isInit = true;
    return true;
}

void HiRedis::uninit()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    closeNoLock();
    setLastError("");
}

bool HiRedis::connect(const std::string& ip, int32_t port)
{
    return init(ip, port);
}

bool HiRedis::connect(const HiRedisConfig& config)
{
    return init(config);
}

void HiRedis::close()
{
    uninit();
}

void HiRedis::closeNoLock()
{
    if (m_redis != nullptr)
    {
        redisFree(m_redis);
        m_redis = nullptr;
    }
    m_isInit = false;
}

bool HiRedis::reconnect()
{
    HiRedisConfig config;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        config = m_config;
    }
    return init(config);
}

bool HiRedis::ping()
{
    std::shared_ptr<HiRedisResultSet> result = command(std::vector<std::string>{ "PING" });
    return result.get() != nullptr && (result->toString() == "PONG" || result->toStatus());
}

bool HiRedis::isConnected()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isInit && m_redis != nullptr && !m_redis->err;
}

std::string HiRedis::lastError()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

std::shared_ptr<HiRedisResultSet> HiRedis::command(const std::string& commandText)
{
    std::vector<std::string> args;
    std::string error;
    if (!HiRedisHelper::splitCommandText(commandText, args, error))
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::shared_ptr<HiRedisResultSet> result(new HiRedisResultSet);
        result->setError(error);
        setLastError(error);
        return result;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    return commandArgvNoLock(args);
}

std::shared_ptr<HiRedisResultSet> HiRedis::command(const std::vector<std::string>& args)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return commandArgvNoLock(args);
}

std::shared_ptr<HiRedisResultSet> HiRedis::command(const HiRedisCommand& commandInfo)
{
    return command(commandInfo.m_args);
}

std::vector<std::shared_ptr<HiRedisResultSet>> HiRedis::pipeline(const std::vector<HiRedisCommand>& commands)
{
    std::vector<std::shared_ptr<HiRedisResultSet>> result;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_redis == nullptr)
    {
        std::shared_ptr<HiRedisResultSet> error(new HiRedisResultSet);
        error->setError("redis is not connected");
        result.push_back(error);
        setLastError(error->error());
        return result;
    }

    for (size_t i = 0; i < commands.size(); ++i)
    {
        if (commands[i].m_args.empty())
        {
            std::shared_ptr<HiRedisResultSet> error(new HiRedisResultSet);
            error->setError("redis pipeline command args is empty");
            result.push_back(error);
            setLastError(error->error());
            return result;
        }
    }

    for (size_t i = 0; i < commands.size(); ++i)
    {
        if (!appendCommandArgvNoLock(commands[i].m_args))
        {
            std::shared_ptr<HiRedisResultSet> error(new HiRedisResultSet);
            error->setError(m_lastError);
            result.push_back(error);
            closeNoLock();
            return result;
        }
    }

    for (size_t i = 0; i < commands.size(); ++i)
    {
        redisReply* reply = nullptr;
        int32_t rc = redisGetReply(m_redis, reinterpret_cast<void**>(&reply));
        std::shared_ptr<HiRedisResultSet> item(new HiRedisResultSet);
        item->setReply(reply);
        if (rc != REDIS_OK)
        {
            item->setError(m_redis->errstr == nullptr ? "redis pipeline failed" : m_redis->errstr);
            setLastError(item->error());
            result.push_back(item);
            closeNoLock();
            return result;
        }
        result.push_back(item);
    }
    return result;
}

std::shared_ptr<HiRedisResultSet> HiRedis::execTransaction(const std::vector<HiRedisCommand>& commands)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < commands.size(); ++i)
    {
        if (commands[i].m_args.empty())
        {
            std::shared_ptr<HiRedisResultSet> result(new HiRedisResultSet);
            result->setError("redis transaction command args is empty");
            setLastError(result->error());
            return result;
        }
    }

    std::vector<std::string> multi;
    multi.push_back("MULTI");
    std::shared_ptr<HiRedisResultSet> begin = commandArgvNoLock(multi);
    if (!begin->toStatus())
    {
        return begin;
    }
    for (size_t i = 0; i < commands.size(); ++i)
    {
        std::shared_ptr<HiRedisResultSet> queued = commandArgvNoLock(commands[i].m_args);
        if (!queued->toStatus())
        {
            commandArgvNoLock(std::vector<std::string>{ "DISCARD" });
            return queued;
        }
    }
    return commandArgvNoLock(std::vector<std::string>{ "EXEC" });
}

bool HiRedis::selectDbIndex(int32_t index)
{
    return selectDb(index);
}

bool HiRedis::selectDb(int32_t index)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::shared_ptr<HiRedisResultSet> result =
        commandArgvNoLock(std::vector<std::string>{ "SELECT", CStringManager::toStringInt32(index) });
    if (result->toStatus())
    {
        m_config.m_dbIndex = index;
        return true;
    }
    return false;
}

bool HiRedis::auth(const std::string& password)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::shared_ptr<HiRedisResultSet> result = commandArgvNoLock(std::vector<std::string>{ "AUTH", password });
    if (result->toStatus())
    {
        m_config.m_password = password;
        return true;
    }
    return false;
}

bool HiRedis::setKeyValue(const std::string& key, const std::string& value)
{
    return set(key, value);
}

bool HiRedis::set(const std::string& key, const std::string& value)
{
    return command(std::vector<std::string>{ "SET", key, value })->toStatus();
}

bool HiRedis::setEx(const std::string& key, const std::string& value, int32_t ttlSeconds)
{
    return command(std::vector<std::string>{ "SETEX", key, CStringManager::toStringInt32(ttlSeconds), value })->toStatus();
}

bool HiRedis::setNx(const std::string& key, const std::string& value, int32_t ttlSeconds)
{
    std::vector<std::string> args;
    args.push_back("SET");
    args.push_back(key);
    args.push_back(value);
    args.push_back("NX");
    if (ttlSeconds > 0)
    {
        args.push_back("EX");
        args.push_back(CStringManager::toStringInt32(ttlSeconds));
    }
    return command(args)->toStatus();
}

std::shared_ptr<HiRedisResultSet> HiRedis::getValue(const std::string& key)
{
    return get(key);
}

std::shared_ptr<HiRedisResultSet> HiRedis::get(const std::string& key)
{
    return command(std::vector<std::string>{ "GET", key });
}

std::shared_ptr<HiRedisResultSet> HiRedis::getValues(const std::vector<std::string>& key)
{
    return mGet(key);
}

std::shared_ptr<HiRedisResultSet> HiRedis::mGet(const std::vector<std::string>& keys)
{
    std::vector<std::string> args;
    args.push_back("MGET");
    args.insert(args.end(), keys.begin(), keys.end());
    std::shared_ptr<HiRedisResultSet> result = command(args);
    result->setKeys(keys);
    return result;
}

int64_t HiRedis::incrBy(const std::string& key, int64_t value)
{
    return command(std::vector<std::string>{ "INCRBY", key, CStringManager::toStringInt64(value) })->toInt64();
}

int64_t HiRedis::decrBy(const std::string& key, int64_t value)
{
    return command(std::vector<std::string>{ "DECRBY", key, CStringManager::toStringInt64(value) })->toInt64();
}

bool HiRedis::expire(const std::string& key, int32_t ttlSeconds)
{
    return command(std::vector<std::string>{ "EXPIRE", key, CStringManager::toStringInt32(ttlSeconds) })->toInt64() == 1;
}

int64_t HiRedis::ttl(const std::string& key)
{
    return command(std::vector<std::string>{ "TTL", key })->toInt64();
}

bool HiRedis::setMap(const std::map<std::string, std::string>& map)
{
    if (map.empty())
    {
        return false;
    }
    std::vector<std::string> args;
    args.push_back("MSET");
    for (std::map<std::string, std::string>::const_iterator it = map.begin(); it != map.end(); ++it)
    {
        args.push_back(it->first);
        args.push_back(it->second);
    }
    return command(args)->toStatus();
}

bool HiRedis::setHashMap(const std::string& mapName, const std::map<std::string, std::string>& hashMap)
{
    return hmSet(mapName, hashMap);
}

bool HiRedis::hSet(const std::string& mapName, const std::string& field, const std::string& value)
{
    return HiRedisHelper::integerReplyOk(command(std::vector<std::string>{ "HSET", mapName, field, value }));
}

bool HiRedis::hmSet(const std::string& mapName, const std::map<std::string, std::string>& hashMap)
{
    if (hashMap.empty())
    {
        return false;
    }
    std::vector<std::string> args;
    args.push_back("HMSET");
    args.push_back(mapName);
    for (std::map<std::string, std::string>::const_iterator it = hashMap.begin(); it != hashMap.end(); ++it)
    {
        args.push_back(it->first);
        args.push_back(it->second);
    }
    return command(args)->toStatus();
}

std::shared_ptr<HiRedisResultSet> HiRedis::hGet(const std::string& mapName, const std::string& field)
{
    return command(std::vector<std::string>{ "HGET", mapName, field });
}

std::shared_ptr<HiRedisResultSet> HiRedis::getHashValues(const std::string& mapName, const std::vector<std::string>& field)
{
    if (field.empty())
    {
        return hGetAll(mapName);
    }
    return hmGet(mapName, field);
}

std::shared_ptr<HiRedisResultSet> HiRedis::hGetAll(const std::string& mapName)
{
    return command(std::vector<std::string>{ "HGETALL", mapName });
}

std::shared_ptr<HiRedisResultSet> HiRedis::hmGet(const std::string& mapName, const std::vector<std::string>& field)
{
    std::vector<std::string> args;
    args.push_back("HMGET");
    args.push_back(mapName);
    args.insert(args.end(), field.begin(), field.end());
    std::shared_ptr<HiRedisResultSet> result = command(args);
    result->setKeys(field);
    return result;
}

std::shared_ptr<HiRedisResultSet> HiRedis::getHashKeys(const std::string& tableName)
{
    return hKeys(tableName);
}

std::shared_ptr<HiRedisResultSet> HiRedis::hKeys(const std::string& tableName)
{
    return command(std::vector<std::string>{ "HKEYS", tableName });
}

bool HiRedis::deleteHashMap(const std::string& tableName)
{
    return deleteKey(tableName);
}

bool HiRedis::deleteHashMapField(const std::string& tableName, const std::string& field)
{
    std::vector<std::string> fields;
    fields.push_back(field);
    return hDel(tableName, fields);
}

bool HiRedis::deleteHashMapFields(const std::string& tableName, const std::vector<std::string>& field)
{
    return hDel(tableName, field);
}

bool HiRedis::hDel(const std::string& tableName, const std::vector<std::string>& field)
{
    if (field.empty())
    {
        return false;
    }
    std::vector<std::string> args;
    args.push_back("HDEL");
    args.push_back(tableName);
    args.insert(args.end(), field.begin(), field.end());
    return HiRedisHelper::integerReplyOk(command(args));
}

bool HiRedis::setTable(const std::string& tableName, const std::vector<std::vector<std::string>>& table, int32_t key, const std::string& split)
{
    if (table.empty())
    {
        return false;
    }
    std::map<std::string, std::string> hashMap;
    for (size_t lineIndex = 0; lineIndex < table.size(); ++lineIndex)
    {
        const std::vector<std::string>& row = table[lineIndex];
        if (row.empty())
        {
            continue;
        }
        std::string field = (key < 0 || key >= static_cast<int32_t>(row.size())) ?
            CStringManager::toStringInt64(static_cast<int64_t>(lineIndex)) : row[static_cast<size_t>(key)];
        std::string value;
        for (size_t col = 0; col < row.size(); ++col)
        {
            if (static_cast<int32_t>(col) == key)
            {
                continue;
            }
            if (!value.empty())
            {
                value += split;
            }
            value += row[col];
        }
        hashMap[field] = value;
    }
    return hmSet(tableName, hashMap);
}

std::shared_ptr<HiRedisResultSet> HiRedis::getTableValues(const std::string& tableName, const std::vector<std::string>& field)
{
    std::shared_ptr<HiRedisResultSet> result = getHashValues(tableName, field);
    result->setKeys(field);
    return result;
}

bool HiRedis::setGroup(const std::string& groupName, const std::string& element)
{
    return command(std::vector<std::string>{ "SADD", groupName, element })->toInt64() == 1;
}

bool HiRedis::setGroups(const std::string& groupName, const std::vector<std::string>& group)
{
    if (group.empty())
    {
        return false;
    }
    std::vector<std::string> args;
    args.push_back("SADD");
    args.push_back(groupName);
    args.insert(args.end(), group.begin(), group.end());
    return HiRedisHelper::integerReplyOk(command(args));
}

std::shared_ptr<HiRedisResultSet> HiRedis::getGroup(const std::string& groupName)
{
    return command(std::vector<std::string>{ "SMEMBERS", groupName });
}

bool HiRedis::groupElementExist(const std::string& groupName, const std::string& element)
{
    return command(std::vector<std::string>{ "SISMEMBER", groupName, element })->toInt64() == 1;
}

int64_t HiRedis::getGroupCount(const std::string& groupName)
{
    return command(std::vector<std::string>{ "SCARD", groupName })->toInt64();
}

bool HiRedis::deleteGroupElement(const std::string& groupName, const std::string& element)
{
    std::vector<std::string> elements;
    elements.push_back(element);
    return deleteGroupElements(groupName, elements);
}

bool HiRedis::deleteGroupElements(const std::string& groupName, const std::vector<std::string>& element)
{
    if (element.empty())
    {
        return false;
    }
    std::vector<std::string> args;
    args.push_back("SREM");
    args.push_back(groupName);
    args.insert(args.end(), element.begin(), element.end());
    return HiRedisHelper::integerReplyOk(command(args));
}

bool HiRedis::setOrderGroup(const std::string& groupName, int32_t score, const std::string& element)
{
    return HiRedisHelper::integerReplyOk(command(std::vector<std::string>{ "ZADD", groupName, CStringManager::toStringInt32(score), element }));
}

bool HiRedis::setOrderGroups(const std::string& groupName, const std::vector<std::pair<int32_t, std::string>>& orderGroup)
{
    if (orderGroup.empty())
    {
        return false;
    }
    std::vector<std::string> args;
    args.push_back("ZADD");
    args.push_back(groupName);
    for (size_t i = 0; i < orderGroup.size(); ++i)
    {
        args.push_back(CStringManager::toStringInt32(orderGroup[i].first));
        args.push_back(orderGroup[i].second);
    }
    return HiRedisHelper::integerReplyOk(command(args));
}

std::shared_ptr<HiRedisResultSet> HiRedis::getOrderGroup(const std::string& groupName)
{
    return command(std::vector<std::string>{ "ZRANGE", groupName, "0", "-1" });
}

bool HiRedis::orderGroupElementExist(const std::string& groupName, const std::string& element)
{
    return !command(std::vector<std::string>{ "ZRANK", groupName, element })->isNull();
}

int64_t HiRedis::getOrderGroupElementIndex(const std::string& groupName, const std::string& element)
{
    std::shared_ptr<HiRedisResultSet> result = command(std::vector<std::string>{ "ZRANK", groupName, element });
    return result->isNull() ? -1 : result->toInt64();
}

int64_t HiRedis::getOrderGroupElementScore(const std::string& groupName, const std::string& element)
{
    std::shared_ptr<HiRedisResultSet> result = command(std::vector<std::string>{ "ZSCORE", groupName, element });
    return result->isNull() ? -1 : result->toInt64();
}

int64_t HiRedis::getOrderGroupCount(const std::string& groupName)
{
    return command(std::vector<std::string>{ "ZCARD", groupName })->toInt64();
}

std::shared_ptr<HiRedisResultSet> HiRedis::getOrderGroupByIndex(const std::string& groupName, int32_t min, int32_t max)
{
    return command(std::vector<std::string>{ "ZRANGE", groupName, CStringManager::toStringInt32(min), CStringManager::toStringInt32(max) });
}

std::shared_ptr<HiRedisResultSet> HiRedis::getOrderGroupByScore(const std::string& groupName, int32_t min, int32_t max)
{
    return command(std::vector<std::string>{ "ZRANGEBYSCORE", groupName, CStringManager::toStringInt32(min), CStringManager::toStringInt32(max) });
}

int64_t HiRedis::getOrderGroupCountByScore(const std::string& groupName, int32_t min, int32_t max)
{
    return command(std::vector<std::string>{ "ZCOUNT", groupName, CStringManager::toStringInt32(min), CStringManager::toStringInt32(max) })->toInt64();
}

bool HiRedis::deleteOrderGroupElement(const std::string& groupName, const std::string& element)
{
    std::vector<std::string> elements;
    elements.push_back(element);
    return deleteOrderGroupElements(groupName, elements);
}

bool HiRedis::deleteOrderGroupElements(const std::string& groupName, const std::vector<std::string>& element)
{
    if (element.empty())
    {
        return false;
    }
    std::vector<std::string> args;
    args.push_back("ZREM");
    args.push_back(groupName);
    args.insert(args.end(), element.begin(), element.end());
    return HiRedisHelper::integerReplyOk(command(args));
}

int64_t HiRedis::deleteOrderGroupElementsByIndex(const std::string& groupName, int32_t min, int32_t max)
{
    return command(std::vector<std::string>{ "ZREMRANGEBYRANK", groupName, CStringManager::toStringInt32(min), CStringManager::toStringInt32(max) })->toInt64();
}

int64_t HiRedis::deleteOrderGroupElementsByScore(const std::string& groupName, int32_t min, int32_t max)
{
    return command(std::vector<std::string>{ "ZREMRANGEBYSCORE", groupName, CStringManager::toStringInt32(min), CStringManager::toStringInt32(max) })->toInt64();
}

bool HiRedis::keyExist(const std::string& key)
{
    return command(std::vector<std::string>{ "EXISTS", key })->toInt64() == 1;
}

bool HiRedis::hashMapExist(const std::string& mapName)
{
    return keyExist(mapName);
}

bool HiRedis::tableExist(const std::string& tableName)
{
    return keyExist(tableName);
}

bool HiRedis::groupExist(const std::string& key)
{
    return keyExist(key);
}

bool HiRedis::orderGroupExist(const std::string& key)
{
    return keyExist(key);
}

bool HiRedis::deleteKey(const std::string& key)
{
    std::vector<std::string> keys;
    keys.push_back(key);
    return del(keys) == 1;
}

bool HiRedis::deleteKeys(const std::vector<std::string>& key)
{
    return del(key) == static_cast<int64_t>(key.size());
}

int64_t HiRedis::del(const std::vector<std::string>& keys)
{
    if (keys.empty())
    {
        return 0;
    }
    std::vector<std::string> args;
    args.push_back("DEL");
    args.insert(args.end(), keys.begin(), keys.end());
    return command(args)->toInt64();
}

std::shared_ptr<HiRedisResultSet> HiRedis::commandArgvNoLock(const std::vector<std::string>& args)
{
    std::shared_ptr<HiRedisResultSet> result(new HiRedisResultSet);
    if (m_redis == nullptr)
    {
        result->setError("redis is not connected");
        setLastError(result->error());
        return result;
    }
    if (args.empty())
    {
        result->setError("redis command args is empty");
        setLastError(result->error());
        return result;
    }

    std::vector<const char*> argv;
    std::vector<size_t> argvlen;
    std::string error;
    if (!HiRedisHelper::makeArgv(args, argv, argvlen, error))
    {
        result->setError(error);
        setLastError(error);
        return result;
    }

    redisReply* reply = static_cast<redisReply*>(redisCommandArgv(m_redis, static_cast<int32_t>(argv.size()), &argv[0], &argvlen[0]));
    result->setReply(reply);
    if (reply == nullptr)
    {
        result->setError(m_redis->errstr == nullptr ? "redis command failed" : m_redis->errstr);
    }
    setLastError(result->error());
    return result;
}

bool HiRedis::appendCommandArgvNoLock(const std::vector<std::string>& args)
{
    if (m_redis == nullptr)
    {
        setLastError("redis is not connected");
        return false;
    }

    std::vector<const char*> argv;
    std::vector<size_t> argvlen;
    std::string error;
    if (!HiRedisHelper::makeArgv(args, argv, argvlen, error))
    {
        setLastError(error);
        return false;
    }

    int32_t rc = redisAppendCommandArgv(m_redis, static_cast<int32_t>(argv.size()), &argv[0], &argvlen[0]);
    if (rc != REDIS_OK)
    {
        setLastError(m_redis->errstr == nullptr ? "redis append command failed" : m_redis->errstr);
        return false;
    }
    return true;
}

void HiRedis::setLastError(const std::string& error)
{
    m_lastError = error;
}

//#include "HiRedisConnectionPool.h"
//#include <atomic>
//#include <cstdlib>
//#include <ctime>
//#include <functional>
//#include <iostream>
//#include <sstream>
//#include <thread>
//
//#ifdef _WIN32
//#include <process.h>
//#else
//#include <unistd.h>
//#endif
//
//int main()
//{
//	int32_t totalCount = 0;
//	int32_t failCount = 0;
//	int32_t skipCount = 0;
//	std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
//	{
//		++totalCount;
//		std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
//		if (!ok)
//		{
//			++failCount;
//		}
//	};
//	std::function<void(const std::string&)> skip = [&skipCount](const std::string& name) -> void
//	{
//		++skipCount;
//		std::cout << "[SKIP] " << name << std::endl;
//	};
//	std::function<std::string(const char*, const std::string&)> envString = [](const char* name, const std::string& defaultValue) -> std::string
//	{
//		const char* value = std::getenv(name);
//		return value == nullptr ? defaultValue : std::string(value);
//	};
//	std::function<int32_t(const char*, int32_t)> envInt = [&envString](const char* name, int32_t defaultValue) -> int32_t
//	{
//		std::string value = envString(name, "");
//		return value.empty() ? defaultValue : static_cast<int32_t>(std::atoi(value.c_str()));
//	};
//	std::function<int32_t()> currentProcessId = []() -> int32_t
//	{
//#ifdef _WIN32
//		return static_cast<int32_t>(_getpid());
//#else
//		return static_cast<int32_t>(getpid());
//#endif
//	};
//
//	HiRedis disconnected;
//	check(!disconnected.isConnected(), "HiRedis default disconnected");
//	check(!disconnected.ping(), "HiRedis ping disconnected");
//	std::shared_ptr<HiRedisResultSet> disconnectedResult = disconnected.command(std::vector<std::string>{"GET", "missing"});
//	check(disconnectedResult.get() != nullptr && disconnectedResult->isError(), "HiRedis disconnected command error");
//	std::vector<HiRedisCommand> disconnectedCommands;
//	disconnectedCommands.push_back(HiRedisCommand(std::vector<std::string>{"GET", "missing"}));
//	std::vector<std::shared_ptr<HiRedisResultSet>> disconnectedPipelineResult = disconnected.pipeline(disconnectedCommands);
//	check(disconnectedPipelineResult.size() == 1 && disconnectedPipelineResult[0]->isError(), "HiRedis disconnected pipeline error");
//	std::shared_ptr<HiRedisResultSet> disconnectedTransactionResult = disconnected.execTransaction(disconnectedCommands);
//	check(disconnectedTransactionResult.get() != nullptr && disconnectedTransactionResult->isError(), "HiRedis disconnected transaction error");
//
//	HiRedisConfig config;
//	config.m_ip = envString("HIREDIS_TEST_IP", "127.0.0.1");
//	config.m_port = envInt("HIREDIS_TEST_PORT", 6379);
//	config.m_password = envString("HIREDIS_TEST_PASSWORD", "");
//	config.m_dbIndex = envInt("HIREDIS_TEST_DB", 0);
//	config.m_connectTimeoutMs = envInt("HIREDIS_TEST_CONNECT_TIMEOUT_MS", 3000);
//	config.m_commandTimeoutMs = envInt("HIREDIS_TEST_COMMAND_TIMEOUT_MS", 3000);
//
//	HiRedis redis;
//	if (!redis.init(config))
//	{
//		skip("HiRedis external redis tests, set HIREDIS_TEST_IP/PORT/PASSWORD/DB when redis is available: " + redis.lastError());
//		std::cout << "HiRedis test " << (failCount == 0 ? "PASS" : "FAIL") <<
//			", total=" << totalCount << ", failed=" << failCount << ", skipped=" << skipCount << std::endl;
//		return failCount == 0 ? 0 : 1;
//	}
//	{
//		HiRedis scopedRedis;
//		check(scopedRedis.init(config), "HiRedis destructor scoped init");
//	}
//	check(true, "HiRedis destructor uninit safe");
//
//	std::ostringstream prefixStream;
//	prefixStream << "hiredis_test_" << static_cast<int64_t>(std::time(nullptr)) << "_" <<
//		currentProcessId() << "_" << reinterpret_cast<uintptr_t>(&prefixStream) << "_";
//	const std::string prefix = prefixStream.str();
//	const std::string stringKey = prefix + "string";
//	const std::string binaryKey = prefix + "binary";
//	const std::string rawPercentKey = prefix + "raw_percent";
//	const std::string rawEscapeKey = prefix + "raw_escape";
//	const std::string rawTrailingEscapeKey = prefix + "raw_trailing_escape";
//	const std::string nxKey = prefix + "nx";
//	const std::string intKey = prefix + "int";
//	const std::string mapKey1 = prefix + "map_1";
//	const std::string mapKey2 = prefix + "map_2";
//	const std::string deleteOneKey = prefix + "delete_one";
//	const std::string deleteManyKey1 = prefix + "delete_many_1";
//	const std::string deleteManyKey2 = prefix + "delete_many_2";
//	const std::string hashKey = prefix + "hash";
//	const std::string tableKey = prefix + "table";
//	const std::string groupKey = prefix + "group";
//	const std::string orderKey = prefix + "order";
//	const std::string transactionKey = prefix + "transaction";
//
//	check(redis.isConnected(), "HiRedis connected");
//	check(redis.ping(), "HiRedis ping connected");
//	check(redis.selectDbIndex(0), "HiRedis selectDbIndex");
//	check(redis.selectDb(0), "HiRedis selectDb");
//	HiRedis compatRedis;
//	check(compatRedis.connect(config), "HiRedis connect compatibility");
//	check(compatRedis.ping(), "HiRedis connect compatibility ping");
//	compatRedis.close();
//	check(!compatRedis.isConnected(), "HiRedis close compatibility");
//	check(redis.reconnect() && redis.isConnected() && redis.ping(), "HiRedis reconnect");
//	HiRedisConfig invalidPortConfig = config;
//	invalidPortConfig.m_port = 1;
//	invalidPortConfig.m_connectTimeoutMs = 50;
//	HiRedis invalidPortRedis;
//	check(!invalidPortRedis.init(invalidPortConfig) && !invalidPortRedis.isConnected(), "HiRedis init invalid port failed");
//	HiRedisConfig invalidDbConfig = config;
//	invalidDbConfig.m_dbIndex = 100000;
//	HiRedis invalidDbRedis;
//	check(!invalidDbRedis.init(invalidDbConfig) && !invalidDbRedis.isConnected(), "HiRedis init invalid db failed");
//	HiRedisConfig invalidAuthConfig = config;
//	invalidAuthConfig.m_password = config.m_password.empty() ? "__hiredis_bad_password__" : config.m_password + "__bad";
//	HiRedis invalidAuthRedis;
//	check(!invalidAuthRedis.init(invalidAuthConfig) && !invalidAuthRedis.isConnected(), "HiRedis init invalid auth failed");
//	check(redis.set(stringKey, "value"), "HiRedis set");
//	check(redis.get(stringKey)->toString() == "value", "HiRedis get");
//	check(redis.setKeyValue(stringKey, "value2"), "HiRedis setKeyValue");
//	check(redis.getValue(stringKey)->toString() == "value2", "HiRedis getValue");
//	std::string binaryValue("a\0b", 3);
//	check(redis.set(binaryKey, binaryValue), "HiRedis set binary value");
//	std::string binaryRead = redis.get(binaryKey)->toString();
//	check(binaryRead.size() == 3 && binaryRead[0] == 'a' && binaryRead[1] == '\0' && binaryRead[2] == 'b', "HiRedis get binary value");
//	check(redis.command(std::string("SET ") + rawPercentKey + " \"%hello world\"")->toStatus(), "HiRedis raw command quote percent");
//	check(redis.get(rawPercentKey)->toString() == "%hello world", "HiRedis raw command quote result");
//	check(redis.command(std::string("SET ") + rawEscapeKey + " a\\ b")->toStatus(), "HiRedis raw command escape space");
//	check(redis.get(rawEscapeKey)->toString() == "a b", "HiRedis raw command escape result");
//	check(redis.command(std::string("SET ") + rawTrailingEscapeKey + " abc\\")->toStatus(), "HiRedis raw command trailing escape");
//	check(redis.get(rawTrailingEscapeKey)->toString() == "abc\\", "HiRedis raw command trailing escape result");
//	check(redis.command("")->isError(), "HiRedis empty raw command error");
//	check(redis.command("GET \"bad")->isError(), "HiRedis unclosed quote command error");
//	check(redis.command(std::vector<std::string>())->isError(), "HiRedis empty argv command error");
//	check(redis.setEx(prefix + "expire", "1", 30), "HiRedis setEx");
//	check(redis.ttl(prefix + "expire") > 0, "HiRedis ttl");
//	check(redis.setNx(nxKey, "first"), "HiRedis setNx first");
//	check(!redis.setNx(nxKey, "second"), "HiRedis setNx second false");
//	check(redis.incrBy(intKey, 10) == 10, "HiRedis incrBy");
//	check(redis.decrBy(intKey, 3) == 7, "HiRedis decrBy");
//	check(redis.expire(intKey, 30), "HiRedis expire");
//	check(redis.keyExist(intKey), "HiRedis keyExist");
//	check(redis.del(std::vector<std::string>()) == 0, "HiRedis del empty");
//	check(redis.set(deleteOneKey, "1") && redis.deleteKey(deleteOneKey) && !redis.keyExist(deleteOneKey), "HiRedis deleteKey");
//	check(redis.set(deleteManyKey1, "1") && redis.set(deleteManyKey2, "2") &&
//		redis.deleteKeys(std::vector<std::string>{deleteManyKey1, deleteManyKey2}), "HiRedis deleteKeys");
//
//	std::map<std::string, std::string> valueMap;
//	valueMap[mapKey1] = "m1";
//	valueMap[mapKey2] = "m2";
//	check(redis.setMap(valueMap), "HiRedis setMap");
//	check(!redis.setMap(std::map<std::string, std::string>()), "HiRedis setMap empty false");
//	std::map<std::string, std::string> getMap = redis.mGet(std::vector<std::string>{mapKey1, mapKey2})->toMap();
//	check(getMap[mapKey1] == "m1" && getMap[mapKey2] == "m2", "HiRedis mGet toMap");
//	std::map<std::string, std::string> getValuesMap = redis.getValues(std::vector<std::string>{mapKey1, mapKey2})->toMap();
//	check(getValuesMap[mapKey1] == "m1" && getValuesMap[mapKey2] == "m2", "HiRedis getValues");
//	std::map<std::string, std::string> missingMap = redis.mGet(std::vector<std::string>{prefix + "missing", mapKey1})->toMap();
//	check(missingMap.find(prefix + "missing") == missingMap.end() && missingMap[mapKey1] == "m1", "HiRedis mGet missing nil skip");
//
//	check(redis.hSet(hashKey, "field1", "value1"), "HiRedis hSet");
//	std::map<std::string, std::string> hashMap;
//	hashMap["field2"] = "value2";
//	hashMap["field3"] = "value3";
//	check(redis.hmSet(hashKey, hashMap), "HiRedis hmSet");
//	check(redis.setHashMap(hashKey, hashMap), "HiRedis setHashMap");
//	check(!redis.hmSet(hashKey, std::map<std::string, std::string>()), "HiRedis hmSet empty false");
//	check(redis.hGet(hashKey, "field1")->toString() == "value1", "HiRedis hGet");
//	std::map<std::string, std::string> readHash = redis.hGetAll(hashKey)->toHashMap();
//	check(readHash["field2"] == "value2" && readHash["field3"] == "value3", "HiRedis hGetAll");
//	check(redis.hmGet(hashKey, std::vector<std::string>{"field1", "field2"})->toMap()["field1"] == "value1", "HiRedis hmGet");
//	check(redis.getHashValues(hashKey)->toHashMap()["field1"] == "value1", "HiRedis getHashValues all");
//	check(redis.getHashValues(hashKey, std::vector<std::string>{"field1"})->toMap()["field1"] == "value1", "HiRedis getHashValues fields");
//	check(redis.hKeys(hashKey)->toKeys().size() >= 3, "HiRedis hKeys");
//	check(redis.getHashKeys(hashKey)->toKeys().size() >= 3, "HiRedis getHashKeys");
//	check(redis.hashMapExist(hashKey), "HiRedis hashMapExist");
//	check(!redis.hDel(hashKey, std::vector<std::string>()), "HiRedis hDel empty false");
//	check(redis.deleteHashMapField(hashKey, "field3"), "HiRedis deleteHashMapField");
//	check(redis.deleteHashMapFields(hashKey, std::vector<std::string>{"field2"}), "HiRedis deleteHashMapFields");
//	check(redis.deleteHashMap(hashKey) && !redis.hashMapExist(hashKey), "HiRedis deleteHashMap");
//
//	std::vector<std::vector<std::string>> table;
//	table.push_back(std::vector<std::string>{"row1", "name1", "score1"});
//	table.push_back(std::vector<std::string>{"row2", "name2", "score2"});
//	check(redis.setTable(tableKey, table, 0, "|"), "HiRedis setTable");
//	check(!redis.setTable(prefix + "empty_table", std::vector<std::vector<std::string>>(), 0, "|"), "HiRedis setTable empty false");
//	check(redis.tableExist(tableKey), "HiRedis tableExist");
//	std::vector<std::vector<std::string>> tableRead = redis.getTableValues(tableKey)->toTable(0, "|");
//	check(tableRead.size() == 2 && tableRead[0].size() == 3, "HiRedis getTableValues");
//	std::vector<std::vector<std::string>> tableMissingRead = redis.getTableValues(tableKey, std::vector<std::string>{prefix + "missing", "row1"})->toTable(0, "|");
//	check(tableMissingRead.size() == 1 && !tableMissingRead[0].empty() && tableMissingRead[0][0] == "row1", "HiRedis getTableValues missing nil skip");
//
//	check(redis.setGroup(groupKey, "a"), "HiRedis setGroup");
//	check(redis.setGroups(groupKey, std::vector<std::string>{"b", "c"}), "HiRedis setGroups");
//	check(!redis.setGroups(groupKey, std::vector<std::string>()), "HiRedis setGroups empty false");
//	check(redis.groupExist(groupKey), "HiRedis groupExist");
//	check(redis.groupElementExist(groupKey, "b"), "HiRedis groupElementExist");
//	check(redis.getGroupCount(groupKey) >= 3, "HiRedis getGroupCount");
//	check(redis.getGroup(groupKey)->toGroup().size() >= 3, "HiRedis getGroup");
//	check(redis.deleteGroupElement(groupKey, "c"), "HiRedis deleteGroupElement");
//	check(!redis.deleteGroupElements(groupKey, std::vector<std::string>()), "HiRedis deleteGroupElements empty false");
//	check(redis.setGroups(groupKey, std::vector<std::string>{"d", "e"}), "HiRedis setGroups for deleteGroupElements");
//	check(redis.deleteGroupElements(groupKey, std::vector<std::string>{"d", "e"}), "HiRedis deleteGroupElements");
//
//	check(redis.setOrderGroup(orderKey, 10, "oa"), "HiRedis setOrderGroup");
//	check(redis.setOrderGroups(orderKey, std::vector<std::pair<int32_t, std::string>>{std::make_pair(20, "ob"), std::make_pair(30, "oc")}), "HiRedis setOrderGroups");
//	check(!redis.setOrderGroups(orderKey, std::vector<std::pair<int32_t, std::string>>()), "HiRedis setOrderGroups empty false");
//	check(redis.orderGroupExist(orderKey), "HiRedis orderGroupExist");
//	check(redis.orderGroupElementExist(orderKey, "ob"), "HiRedis orderGroupElementExist");
//	check(redis.getOrderGroupElementIndex(orderKey, "ob") >= 0, "HiRedis getOrderGroupElementIndex");
//	check(redis.getOrderGroupElementScore(orderKey, "ob") == 20, "HiRedis getOrderGroupElementScore");
//	check(redis.getOrderGroupCount(orderKey) >= 3, "HiRedis getOrderGroupCount");
//	check(redis.getOrderGroup(orderKey)->toKeys().size() >= 3, "HiRedis getOrderGroup");
//	check(redis.getOrderGroupByIndex(orderKey, 0, -1)->toKeys().size() >= 3, "HiRedis getOrderGroupByIndex");
//	check(redis.getOrderGroupByScore(orderKey, 10, 30)->toKeys().size() >= 3, "HiRedis getOrderGroupByScore");
//	check(redis.getOrderGroupCountByScore(orderKey, 10, 30) >= 3, "HiRedis getOrderGroupCountByScore");
//	check(redis.deleteOrderGroupElement(orderKey, "oc"), "HiRedis deleteOrderGroupElement");
//	check(!redis.deleteOrderGroupElements(orderKey, std::vector<std::string>()), "HiRedis deleteOrderGroupElements empty false");
//	check(redis.setOrderGroups(orderKey, std::vector<std::pair<int32_t, std::string>>{std::make_pair(40, "od"), std::make_pair(50, "oe"), std::make_pair(60, "of")}), "HiRedis setOrderGroups for range delete");
//	check(redis.deleteOrderGroupElements(orderKey, std::vector<std::string>{"od"}), "HiRedis deleteOrderGroupElements");
//	check(redis.deleteOrderGroupElementsByScore(orderKey, 50, 50) == 1, "HiRedis deleteOrderGroupElementsByScore");
//	check(redis.deleteOrderGroupElementsByIndex(orderKey, 0, 0) >= 1, "HiRedis deleteOrderGroupElementsByIndex");
//
//	std::vector<HiRedisCommand> pipelineCommands;
//	pipelineCommands.push_back(HiRedisCommand(std::vector<std::string>{"SET", prefix + "pipeline", "pv"}));
//	pipelineCommands.push_back(HiRedisCommand(std::vector<std::string>{"GET", prefix + "pipeline"}));
//	std::vector<std::shared_ptr<HiRedisResultSet>> pipelineResult = redis.pipeline(pipelineCommands);
//	check(pipelineResult.size() == 2 && pipelineResult[0]->toStatus() && pipelineResult[1]->toString() == "pv", "HiRedis pipeline");
//	check(redis.pipeline(std::vector<HiRedisCommand>()).empty(), "HiRedis pipeline empty");
//	std::vector<HiRedisCommand> invalidPipelineCommands;
//	invalidPipelineCommands.push_back(HiRedisCommand());
//	std::vector<std::shared_ptr<HiRedisResultSet>> invalidPipelineResult = redis.pipeline(invalidPipelineCommands);
//	check(invalidPipelineResult.size() == 1 && invalidPipelineResult[0]->isError() && redis.ping(), "HiRedis pipeline invalid command safe");
//
//	std::vector<HiRedisCommand> transactionCommands;
//	transactionCommands.push_back(HiRedisCommand(std::vector<std::string>{"SET", transactionKey, "tv"}));
//	transactionCommands.push_back(HiRedisCommand(std::vector<std::string>{"GET", transactionKey}));
//	std::shared_ptr<HiRedisResultSet> transactionResult = redis.execTransaction(transactionCommands);
//	check(transactionResult.get() != nullptr && transactionResult->isValid(), "HiRedis execTransaction");
//	std::shared_ptr<HiRedisResultSet> invalidTransactionResult = redis.execTransaction(invalidPipelineCommands);
//	check(invalidTransactionResult.get() != nullptr && invalidTransactionResult->isError() && redis.ping(), "HiRedis execTransaction invalid command safe");
//
//	HiRedisConnectionPool emptyPool;
//	check(!emptyPool.init(config, 0), "HiRedisConnectionPool init zero false");
//	check(emptyPool.acquire(1).get() == nullptr, "HiRedisConnectionPool acquire before init false");
//	emptyPool.uninit();
//	emptyPool.uninit();
//	check(true, "HiRedisConnectionPool duplicate uninit without init safe");
//	HiRedisConnectionPool pool;
//	bool poolOk = pool.init(config, 8);
//	check(poolOk, "HiRedisConnectionPool init");
//	if (poolOk)
//	{
//		check(pool.init(config, 8), "HiRedisConnectionPool double init no-op");
//		check(pool.totalCount() == 8 && pool.availableCount() == 8, "HiRedisConnectionPool double init keep old pool");
//		std::shared_ptr<HiRedis> poolRedis = pool.acquire(1000);
//		check(poolRedis.get() != nullptr && poolRedis->ping(), "HiRedisConnectionPool acquire");
//		poolRedis.reset();
//		check(pool.availableCount() == 8, "HiRedisConnectionPool release");
//
//		HiRedisConnectionPool onePool;
//		bool onePoolOk = onePool.init(config, 1);
//		check(onePoolOk, "HiRedisConnectionPool single init");
//		if (onePoolOk)
//		{
//			std::shared_ptr<HiRedis> holdConnection = onePool.acquire(1000);
//			std::shared_ptr<HiRedis> timeoutConnection = onePool.acquire(1);
//			check(holdConnection.get() != nullptr && timeoutConnection.get() == nullptr, "HiRedisConnectionPool acquire timeout");
//			holdConnection.reset();
//			onePool.uninit();
//		}
//
//		HiRedisConnectionPool borrowedPool;
//		bool borrowedPoolOk = borrowedPool.init(config, 1);
//		check(borrowedPoolOk, "HiRedisConnectionPool borrowed init");
//		if (borrowedPoolOk)
//		{
//			std::shared_ptr<HiRedis> borrowedConnection = borrowedPool.acquire(1000);
//			check(borrowedConnection.get() != nullptr && borrowedPool.init(config, 1), "HiRedisConnectionPool double init with borrowed no-op");
//			check(borrowedPool.totalCount() == 1 && borrowedPool.availableCount() == 0, "HiRedisConnectionPool double init keep borrowed state");
//			borrowedPool.uninit();
//			borrowedPool.uninit();
//			check(borrowedPool.totalCount() == 1 && borrowedPool.availableCount() == 0, "HiRedisConnectionPool double uninit kept borrowed state");
//			borrowedConnection.reset();
//			check(borrowedPool.totalCount() == 0 && borrowedPool.availableCount() == 0, "HiRedisConnectionPool borrowed release after uninit");
//		}
//
//		HiRedisConnectionPool* destroyPool = new HiRedisConnectionPool;
//		bool destroyPoolOk = destroyPool->init(config, 1);
//		check(destroyPoolOk, "HiRedisConnectionPool destroy with borrowed init");
//		if (destroyPoolOk)
//		{
//			std::shared_ptr<HiRedis> borrowedAfterDestroy = destroyPool->acquire(1000);
//			check(borrowedAfterDestroy.get() != nullptr, "HiRedisConnectionPool acquire before destroy");
//			delete destroyPool;
//			borrowedAfterDestroy.reset();
//			check(true, "HiRedisConnectionPool borrowed release after pool destroy safe");
//		}
//		else
//		{
//			delete destroyPool;
//		}
//
//		HiRedisConnectionPool* idleDestroyPool = new HiRedisConnectionPool;
//		bool idleDestroyPoolOk = idleDestroyPool->init(config, 1);
//		check(idleDestroyPoolOk, "HiRedisConnectionPool destroy with idle init");
//		delete idleDestroyPool;
//		check(true, "HiRedisConnectionPool idle destroy safe");
//
//		std::atomic<int32_t> threadFailCount(0);
//		std::vector<std::thread> threads;
//		for (int32_t threadIndex = 0; threadIndex < 6; ++threadIndex)
//		{
//			threads.push_back(std::thread([threadIndex, &pool, &prefix, &threadFailCount]() -> void
//			{
//				for (int32_t loopIndex = 0; loopIndex < 40; ++loopIndex)
//				{
//					std::shared_ptr<HiRedis> connection = pool.acquire(10000);
//					if (connection.get() == nullptr)
//					{
//						threadFailCount.fetch_add(1);
//						continue;
//					}
//					std::ostringstream keyStream;
//					keyStream << prefix << "thread_" << threadIndex << "_" << loopIndex;
//					std::string key = keyStream.str();
//					if (!connection->set(key, "thread") || connection->get(key)->toString() != "thread")
//					{
//						threadFailCount.fetch_add(1);
//					}
//				}
//			}));
//		}
//		for (size_t i = 0; i < threads.size(); ++i)
//		{
//			threads[i].join();
//		}
//		check(threadFailCount.load() == 0, "HiRedisConnectionPool multithread pressure");
//		pool.uninit();
//	}
//	else
//	{
//		skip("HiRedisConnectionPool multithread pressure");
//	}
//
//	std::vector<std::string> cleanKeys;
//	cleanKeys.push_back(stringKey);
//	cleanKeys.push_back(binaryKey);
//	cleanKeys.push_back(rawPercentKey);
//	cleanKeys.push_back(rawEscapeKey);
//	cleanKeys.push_back(rawTrailingEscapeKey);
//	cleanKeys.push_back(nxKey);
//	cleanKeys.push_back(intKey);
//	cleanKeys.push_back(mapKey1);
//	cleanKeys.push_back(mapKey2);
//	cleanKeys.push_back(deleteOneKey);
//	cleanKeys.push_back(deleteManyKey1);
//	cleanKeys.push_back(deleteManyKey2);
//	cleanKeys.push_back(hashKey);
//	cleanKeys.push_back(tableKey);
//	cleanKeys.push_back(groupKey);
//	cleanKeys.push_back(orderKey);
//	cleanKeys.push_back(transactionKey);
//	cleanKeys.push_back(prefix + "expire");
//	cleanKeys.push_back(prefix + "pipeline");
//	for (int32_t threadIndex = 0; threadIndex < 6; ++threadIndex)
//	{
//		for (int32_t loopIndex = 0; loopIndex < 40; ++loopIndex)
//		{
//			std::ostringstream keyStream;
//			keyStream << prefix << "thread_" << threadIndex << "_" << loopIndex;
//			cleanKeys.push_back(keyStream.str());
//		}
//	}
//	redis.del(cleanKeys);
//	redis.uninit();
//	check(!redis.isConnected(), "HiRedis uninit disconnected");
//
//	std::cout << "HiRedis test " << (failCount == 0 ? "PASS" : "FAIL") <<
//		", total=" << totalCount << ", failed=" << failCount << ", skipped=" << skipCount << std::endl;
//	return failCount == 0 ? 0 : 1;
//}