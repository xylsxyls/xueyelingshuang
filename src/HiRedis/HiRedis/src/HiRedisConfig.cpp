#include "HiRedisConfig.h"
#include "hiredisinclude/hiredis.h"

HiRedisConfig::HiRedisConfig() :
m_ip("127.0.0.1"),
m_port(6379),
m_dbIndex(0),
m_connectTimeoutMs(3000),
m_commandTimeoutMs(3000),
m_enableKeepAlive(true),
m_poolMinConnections(1),
m_poolMaxConnections(1),
m_poolAcquireTimeoutMs(3000),
m_keepAliveIdleSeconds(REDIS_KEEPALIVE_INTERVAL),
m_readerMaxUnusedBytes(REDIS_READER_MAX_BUF),
m_readerMaxElements(REDIS_READER_MAX_ARRAY_ELEMENTS)
{

}

void HiRedisConfig::setEndpoint(const std::string& ip, int32_t port)
{
    m_ip = ip;
    m_port = port;
}

void HiRedisConfig::setPassword(const std::string& password)
{
    m_password = password;
}

void HiRedisConfig::setDatabase(int32_t index)
{
    m_dbIndex = index;
}

void HiRedisConfig::setTimeouts(int32_t connectMs, int32_t commandMs)
{
    m_connectTimeoutMs = connectMs;
    m_commandTimeoutMs = commandMs;
}

void HiRedisConfig::setKeepAlive(bool enabled)
{
    m_enableKeepAlive = enabled;
}

void HiRedisConfig::setKeepAliveIdleSeconds(int32_t seconds)
{
    m_keepAliveIdleSeconds = seconds;
}

void HiRedisConfig::setReaderLimits(int64_t maxUnusedBytes, int64_t maxElements)
{
    m_readerMaxUnusedBytes = maxUnusedBytes;
    m_readerMaxElements = maxElements;
}

void HiRedisConfig::setPoolBounds(int32_t minimum, int32_t maximum)
{
    m_poolMinConnections = minimum;
    m_poolMaxConnections = maximum;
}

void HiRedisConfig::setPoolAcquireTimeout(int32_t timeoutMs)
{
    m_poolAcquireTimeoutMs = timeoutMs;
}