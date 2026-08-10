#include "HiRedisConfig.h"

HiRedisConfig::HiRedisConfig() :
m_ip("127.0.0.1"),
m_port(6379),
m_dbIndex(0),
m_connectTimeoutMs(3000),
m_commandTimeoutMs(3000),
m_enableKeepAlive(true)
{

}