#include "HiRedis.h"
#include "HiRedisHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#ifdef _WIN32
#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#endif
#endif
#include "hiredisinclude/hiredis.h"
#include <exception>
#if defined(__linux__) && !defined(__GLIBC__)
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#endif

HiRedis::HiRedis() :
m_redis(nullptr)
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
    std::lock_guard<std::mutex> lock(m_mutex);
    return initNoLock(config);
}

bool HiRedis::initNoLock(const HiRedisConfig& config)
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

    const char* invalid = HiRedisHelper::connectionConfigError(config);
    if (invalid)
    {
        setLastError(invalid);
        return false;
    }
    closeNoLock();
    m_config = config;

    try
    {
        struct timeval connectTimeout = toTimeval(config.m_connectTimeoutMs);
        m_redis = redisConnectWithTimeout(config.m_ip.c_str(), config.m_port, connectTimeout);
        if (m_redis == nullptr)
        {
            setLastError("redis connect returned null");
            return false;
        }
        if (m_redis->err)
        {
            setLastError(m_redis->errstr[0] ? m_redis->errstr : "redis connect failed");
            closeNoLock();
            return false;
        }

        // hiredis 用 0 表示不按阈值回收缓冲或不限制聚合元素数。
        m_redis->reader->maxbuf = config.m_readerMaxUnusedBytes == -1 ? 0 : static_cast<size_t>(config.m_readerMaxUnusedBytes);
        m_redis->reader->maxelements = config.m_readerMaxElements == -1 ? 0 : config.m_readerMaxElements;

        struct timeval commandTimeout = toTimeval(config.m_commandTimeoutMs);
        if (redisSetTimeout(m_redis, commandTimeout) != REDIS_OK)
        {
            setLastError(m_redis->errstr);
            closeNoLock();
            return false;
        }
        if (config.m_enableKeepAlive && redisEnableKeepAliveWithInterval(m_redis, config.m_keepAliveIdleSeconds) != REDIS_OK)
        {
            setLastError(m_redis->errstr[0] ? m_redis->errstr : "redis TCP keepalive failed");
            closeNoLock();
            return false;
        }

#if defined(__linux__) && !defined(__GLIBC__)
        // hiredis 在非 glibc Linux 仅设置 SO_KEEPALIVE，补上调用方要求的空闲期限。
        if (config.m_enableKeepAlive && setsockopt(m_redis->fd, IPPROTO_TCP, TCP_KEEPIDLE,
            &config.m_keepAliveIdleSeconds, sizeof(config.m_keepAliveIdleSeconds)) != 0)
        {
            setLastError("redis TCP keepalive idle time could not be set");
            closeNoLock();
            return false;
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
        return true;
    }
    catch (const std::exception&)
    {
        // 认证或选库期间的 C++ 异常也必须释放未就绪的连接。
        closeNoLock();
        throw;
    }
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
}

bool HiRedis::reconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return initNoLock(m_config);
}

bool HiRedis::ping()
{
    std::shared_ptr<HiRedisResultSet> result = command(std::vector<std::string>{ "PING" });
    return result.get() != nullptr && (result->toString() == "PONG" || result->toStatus());
}

bool HiRedis::isConnected()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_redis != nullptr && !m_redis->err;
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

    try
    {
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

        m_lastError.clear();
        for (size_t i = 0; i < commands.size(); ++i)
        {
            std::shared_ptr<HiRedisResultSet> item(new HiRedisResultSet);
            void* reply = nullptr;
            int32_t rc = redisGetReply(m_redis, &reply);
            item->setReply(static_cast<redisReply*>(reply));
            if (rc != REDIS_OK)
            {
                item->setError(m_redis->errstr[0] ? m_redis->errstr : "redis pipeline failed");
                setLastError(item->error());
                result.push_back(item);
                closeNoLock();
                return result;
            }
            if (m_lastError.empty() && item->isError())
            {
                setLastError(item->error());
            }
            result.push_back(item);
        }
        return result;
    }
    catch (const std::exception&)
    {
        // 批次可能已部分发送或读取，关闭连接以免后续调用拿到遗留回复。
        closeNoLock();
        throw;
    }
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
        // Redis 命令名按 ASCII 忽略大小写，不受进程的 LC_CTYPE（如土耳其语 i）影响。
        std::string verb = commands[i].m_args[0];
        for (size_t j = 0; j < verb.size(); ++j)
        {
            if (verb[j] >= 'a' && verb[j] <= 'z')
            {
                verb[j] = static_cast<char>(verb[j] - 'a' + 'A');
            }
        }
        if (verb == "MULTI" || verb == "EXEC" || verb == "DISCARD" || verb == "WATCH" ||
            verb == "UNWATCH" || verb == "QUIT" || verb == "RESET")
        {
            std::shared_ptr<HiRedisResultSet> result(new HiRedisResultSet);
            result->setError("redis transaction cannot contain session control commands");
            setLastError(result->error());
            return result;
        }
    }

    try
    {
        std::shared_ptr<HiRedisResultSet> begin = commandArgvNoLock({ "MULTI" });
        if (!begin->toStatus())
        {
            return begin;
        }
        for (size_t i = 0; i < commands.size(); ++i)
        {
            std::shared_ptr<HiRedisResultSet> queued = commandArgvNoLock(commands[i].m_args);
            if (queued->type() != REDIS_REPLY_STATUS || queued->toString() != "QUEUED")
            {
                if (!queued->isError())
                {
                    queued->setError("redis transaction expected QUEUED");
                }
                if (!commandArgvNoLock({ "DISCARD" })->toStatus())
                {
                    // 服务端未确认退出 MULTI，下一次命令不能复用这个会话。
                    closeNoLock();
                }
                setLastError(queued->error());
                return queued;
            }
        }
        auto executed = commandArgvNoLock({ "EXEC" });
        if (executed->isError())
        {
            // EXEC 被拒绝时服务端可能仍在 MULTI，不能将该连接用于下一次业务操作。
            closeNoLock();
        }
        return executed;
    }
    catch (const std::exception&)
    {
        // MULTI 后内存分配等失败时，关闭连接以结束未完成的事务。
        closeNoLock();
        throw;
    }
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
    return hDel(tableName, {field});
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
        std::string field = (key < 0 || static_cast<size_t>(key) >= row.size()) ?
            CStringManager::toStringInt64(static_cast<int64_t>(lineIndex)) : row[static_cast<size_t>(key)];
        std::string value;
        bool firstCell = true;
        for (size_t col = 0; col < row.size(); ++col)
        {
            if (key >= 0 && col == static_cast<size_t>(key))
            {
                continue;
            }
            if (!firstCell)
            {
                value += split;
            }
            value += row[col];
            firstCell = false;
        }
        hashMap[field] = value;
    }
    return hmSet(tableName, hashMap);
}

std::shared_ptr<HiRedisResultSet> HiRedis::getTableValues(const std::string& tableName, const std::vector<std::string>& field)
{
    return getHashValues(tableName, field);
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
    return deleteGroupElements(groupName, {element});
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
    return getOrderGroupElementIndex(groupName, element) >= 0;
}

int64_t HiRedis::getOrderGroupElementIndex(const std::string& groupName, const std::string& element)
{
    std::shared_ptr<HiRedisResultSet> result = command(std::vector<std::string>{ "ZRANK", groupName, element });
    return result->type() == REDIS_REPLY_INTEGER ? result->toInt64() : -1;
}

int64_t HiRedis::getOrderGroupElementScore(const std::string& groupName, const std::string& element)
{
    std::shared_ptr<HiRedisResultSet> result = command(std::vector<std::string>{ "ZSCORE", groupName, element });
    return result->type() == REDIS_REPLY_STRING ? result->toInt64() : -1;
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
    return deleteOrderGroupElements(groupName, {element});
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
    return del({key}) == 1;
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
        result->setError(m_redis->errstr[0] ? m_redis->errstr : "redis command failed");
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
        setLastError(m_redis->errstr[0] ? m_redis->errstr : "redis append command failed");
        return false;
    }
    return true;
}

void HiRedis::setLastError(const std::string& error)
{
    m_lastError = error;
}

// 使用示例开始。单独编译本文件时可启用下方的 main。
//#include "HiRedisConnectionPool.h"
//#include <cerrno>
//#include <chrono>
//#include <cstdlib>
//#include <iostream>
//
//int main()
//{
//    // HIREDIS_TEST_* 指向演示实例；全集合只处理本次活动报名的键。
//    const char* host = std::getenv("HIREDIS_TEST_IP");
//    // 将可选的十进制环境参数写入 value；未设置保留现值，溢出或格式错误返回 false。
//    const auto readNumber = [](const char* name, int32_t& value) -> bool
//    {
//        const char* text = std::getenv(name);
//        if (!text)
//        {
//            return true;
//        }
//        char* end = nullptr;
//        errno = 0;
//        const long parsed = std::strtol(text, &end, 10);
//        if (errno == ERANGE || end == text || *end || parsed < INT32_MIN || parsed > INT32_MAX)
//        {
//            return false;
//        }
//        value = static_cast<int32_t>(parsed);
//        return true;
//    };
//    const char* password = std::getenv("HIREDIS_TEST_PASSWORD");
//    HiRedisConfig config;
//    int32_t port = config.m_port, database = config.m_dbIndex;
//    if (!readNumber("HIREDIS_TEST_PORT", port) || !readNumber("HIREDIS_TEST_DB", database))
//    {
//        std::cerr << "Invalid Redis port or database" << std::endl;
//        return 1;
//    }
//    config.setEndpoint(host ? host : config.m_ip, port);
//    config.setPassword(password ? password : "");
//    config.setDatabase(database);
//    config.setTimeouts(2000, 3000);
//    config.setKeepAlive(true);
//    config.setKeepAliveIdleSeconds(30);
//    config.setReaderLimits(16384, 65536);
//    HiRedis redis;
//    if (!redis.connect(config) || !redis.isConnected() || !redis.ping())
//    {
//        std::cerr << redis.lastError() << std::endl;
//        return 1;
//    }
//    const auto client = redis.command(std::vector<std::string>{"CLIENT", "ID"});
//    if (client->isError() || client->toInt64() <= 0)
//    {
//        std::cerr << redis.lastError() << std::endl;
//        return 1;
//    }
//    const std::string prefix = "hiredis_demo:" + std::to_string(client->toInt64()) + ":" +
//        std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ":";
//    const std::string title = prefix + "title", slots = prefix + "slots", event = prefix + "event";
//    const std::string players = prefix + "players", scores = prefix + "scores", roster = prefix + "roster";
//    const std::vector<std::string> owned = {title, slots, event, players, scores, roster};
//    for (const std::string& key : owned)
//    {
//        if (redis.keyExist(key))
//        {
//            return 1;
//        }
//    }
//    if (!redis.setNx(title, "Spring cup", 300))
//    {
//        return 1;
//    }
//    // 返回报名及查询是否完成；所有正常失败路径汇合后统一清理本次活动的键。
//    const auto registerEvent = [&]() -> bool
//    {
//        // 报名限额用字符串计数，活动资料、选手和积分分别保存在哈希、集合、有序集合中。
//        if (!redis.setEx(slots, "20", 300) || !redis.hmSet(event, {{"venue", "Hall A"}, {"stage", "open"}}) ||
//            !redis.setGroups(players, {"Ada", "Lin"}) || !redis.setGroup(players, "Bo") ||
//            !redis.setOrderGroups(scores, {{18, "Ada"}, {12, "Lin"}}) || !redis.setOrderGroup(scores, 9, "Bo") ||
//            !redis.setTable(roster, {{"Ada", "red"}, {"Lin", "blue"}, {"Bo", "green"}}, 0))
//        {
//            std::cerr << redis.lastError() << std::endl;
//            return false;
//        }
//        // 取消一名选手，再一起发布状态和剩余名额。
//        if (!redis.deleteGroupElement(players, "Bo") || !redis.deleteOrderGroupElement(scores, "Bo") ||
//            redis.decrBy(slots, 3) != 17 || redis.incrBy(slots, 1) != 18 || !redis.hSet(event, "stage", "closed"))
//        {
//            std::cerr << redis.lastError() << std::endl;
//            return false;
//        }
//        const auto receipt = redis.execTransaction({HiRedisCommand({"HSET", event, "published", "yes"}),
//            HiRedisCommand({"EXPIRE", players, "300"}), HiRedisCommand({"EXPIRE", scores, "300"})});
//        if (receipt->isError() || receipt->size() != 3 || receipt->isErrorAt(0) || receipt->isErrorAt(1) ||
//            receipt->isErrorAt(2) || !redis.expire(event, 300) || !redis.expire(roster, 300))
//        {
//            std::cerr << redis.lastError() << std::endl;
//            return false;
//        }
//        const auto snapshot = redis.pipeline({HiRedisCommand({"SCARD", players}), HiRedisCommand({"ZCARD", scores})});
//        if (snapshot.size() != 2 || snapshot[0]->isError() || snapshot[1]->isError())
//        {
//            std::cerr << "Cannot read registration summary" << std::endl;
//            return false;
//        }
//        std::cout << redis.get(title)->toString() << " at " << redis.hGet(event, "venue")->toString()
//            << ", remaining=" << redis.getValue(slots)->toString() << ", ttl=" << redis.ttl(event) << '\n';
//        std::cout << "players=" << snapshot[0]->toInt64() << ", ranked=" << snapshot[1]->toInt64()
//            << ", qualifying=" << redis.getOrderGroupCountByScore(scores, 10, 20) << '\n';
//        if (redis.groupElementExist(players, "Ada") && redis.orderGroupElementExist(scores, "Ada"))
//        {
//            std::cout << "Ada score=" << redis.getOrderGroupElementScore(scores, "Ada")
//                << ", rank index=" << redis.getOrderGroupElementIndex(scores, "Ada") << '\n';
//        }
//        for (const auto& row : redis.getTableValues(roster)->toTable(0))
//        {
//            if (row.size() == 2)
//            {
//                std::cout << row[0] << " team=" << row[1] << '\n';
//            }
//        }
//        const auto details = redis.getHashValues(event, {"stage", "published"})->toMap();
//        const auto qualifiers = redis.getOrderGroupByScore(scores, 10, 20)->toKeys();
//        const auto top = redis.getOrderGroupByIndex(scores, -1, -1)->at(0);
//        config.setPoolBounds(1, 2);
//        config.setPoolAcquireTimeout(500);
//        HiRedisConnectionPool pool;
//        if (!pool.init(config) || pool.totalCount() != 1 || pool.availableCount() != 1)
//        {
//            std::cerr << pool.lastError() << std::endl;
//            return false;
//        }
//        auto reader = pool.acquire();
//        if (!reader || reader->getValues({title, slots})->toMap().size() != 2 ||
//            reader->hGetAll(event)->toHashMap() != redis.getHashValues(event)->toHashMap() ||
//            reader->hKeys(event)->size() != 3 || reader->getGroup(players)->toGroup().size() != 2)
//        {
//            std::cerr << "Cannot read published event" << std::endl;
//            return false;
//        }
//        reader.reset();
//        std::cout << "available readers=" << pool.availableCount() << "/" << pool.totalCount() << '\n';
//        pool.close();
//        return details == std::map<std::string, std::string>({{"stage", "closed"}, {"published", "yes"}}) &&
//            qualifiers.size() == 2 && top == "Ada" && redis.hashMapExist(event) && redis.groupExist(players) && redis.orderGroupExist(scores) &&
//            redis.tableExist(roster) && redis.getGroupCount(players) == 2 && redis.getOrderGroupCount(scores) == 2;
//    };
//    bool completed = false;
//    try
//    {
//        completed = registerEvent();
//    }
//    catch (const std::exception& error)
//    {
//        std::cerr << error.what() << std::endl;
//    }
//    const int64_t removed = redis.del(owned);
//    redis.close();
//    return completed && removed == static_cast<int64_t>(owned.size()) ? 0 : 1;
//}
// 使用示例结束。