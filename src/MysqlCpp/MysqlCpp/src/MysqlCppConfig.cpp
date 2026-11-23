#include "MysqlCppConfig.h"
#include "MysqlCppHelper.h"

/** 检查 TCP 端点能否作为独立的主机和端口传给驱动
@param [in] host 独立主机文本，可用一对方括号包住整个地址
@param [in] port TCP 端口
@return 端口在 1～65535 且主机包含可无歧义传给驱动的非空地址时返回 true
*/
static bool IsValidTcpEndpoint(const std::string& host, int32_t port)
{
    return port >= 1 && port <= 65535 && MysqlCppHelper::validHost(host);
}

MysqlCppConfig::MysqlCppConfig() :
m_ip("127.0.0.1"),
m_port(3306),
m_autoCommit(false),
m_autoReconnect(true),
m_connectTimeoutSeconds(5),
m_readTimeoutSeconds(10),
m_writeTimeoutSeconds(10),
m_poolMinConnections(1),
m_poolMaxConnections(1),
m_poolAcquireTimeoutMs(3000)
{

}

bool MysqlCppConfig::setEndpoint(const std::string& host, int32_t port)
{
    if (!IsValidTcpEndpoint(host, port))
    {
        return false;
    }
    m_ip = host;
    m_port = port;
    return true;
}

bool MysqlCppConfig::setCredentials(const std::string& account, const std::string& password)
{
    if (account.find('\0') != std::string::npos || password.find('\0') != std::string::npos)
    {
        return false;
    }
    std::string newAccount(account);
    std::string newPassword(password);
    m_account.swap(newAccount);
    m_password.swap(newPassword);
    return true;
}

bool MysqlCppConfig::setDatabase(const std::string& database)
{
    if (database.find('\0') != std::string::npos)
    {
        return false;
    }
    m_dbName = database;
    return true;
}

void MysqlCppConfig::setAutoCommit(bool enabled)
{
    m_autoCommit = enabled;
}

void MysqlCppConfig::setAutoReconnect(bool enabled)
{
    m_autoReconnect = enabled;
}

void MysqlCppConfig::setTimeouts(int32_t connectSeconds, int32_t readSeconds, int32_t writeSeconds)
{
    m_connectTimeoutSeconds = connectSeconds;
    m_readTimeoutSeconds = readSeconds;
    m_writeTimeoutSeconds = writeSeconds;
}

bool MysqlCppConfig::setPoolLimits(int32_t minimum, int32_t maximum)
{
    if (minimum < 0 || (maximum != -1 && (maximum <= 0 || maximum < minimum)))
    {
        return false;
    }
    m_poolMinConnections = minimum;
    m_poolMaxConnections = maximum;
    return true;
}

void MysqlCppConfig::setPoolAcquireTimeout(int32_t milliseconds)
{
    m_poolAcquireTimeoutMs = milliseconds;
}

bool MysqlCppConfig::validConnection() const
{
    return IsValidTcpEndpoint(m_ip, m_port) &&
        m_account.find('\0') == std::string::npos && m_password.find('\0') == std::string::npos &&
        m_dbName.find('\0') == std::string::npos;
}