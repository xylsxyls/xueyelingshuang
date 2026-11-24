#include "NetHeartbeatConfig.h"

NetHeartbeatConfig::NetHeartbeatConfig() :
m_pingIntervalMs(30000),
m_pongTimeoutMs(60000),
m_receiveTimeoutMs(90000)
{

}

bool NetHeartbeatConfig::valid() const
{
    return m_pingIntervalMs != 0 && m_pongTimeoutMs != 0 && m_receiveTimeoutMs > m_pingIntervalMs;
}