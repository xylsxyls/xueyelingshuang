#include "MysqlCppConfig.h"

MysqlCppConfig::MysqlCppConfig() :
m_ip("127.0.0.1"),
m_port(3306),
m_autoCommit(false),
m_autoReconnect(true),
m_connectTimeoutSeconds(5),
m_readTimeoutSeconds(10),
m_writeTimeoutSeconds(10),
m_poolMinConnections(1),
m_poolMaxConnections(1)
{

}