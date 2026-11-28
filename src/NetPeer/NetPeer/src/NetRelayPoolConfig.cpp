#include "NetRelayPoolConfig.h"
#include "NetRelayTransportState.h"

NetRelayPoolConfig::NetRelayPoolConfig()
{
    m_dataRecords.initialCapacity = 128;
    m_dataRecords.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetRelayTransportWrite>>);
    m_dataRecords.directoryCapacity = 128;
    m_dataRecords.lookupCacheCapacity = 0;
    m_controlRecords = m_dataRecords;
}

bool NetRelayPoolConfig::valid() const
{
    return m_stepBudget != 0 && m_startupMs != 0 && m_backpressureMs != 0;
}

/** 核对链路及其中继、控制TLS的独立预算
@param [in] config 一条路径的参数
@return 数值范围有效为true
*/
static bool ValidLinkConfig(const NetLinkTransportConfig& config)
{
    return config.m_host.m_eventBudget && config.m_stepBudget && config.m_backpressureMs && config.m_relay.valid() &&
        config.m_link.m_pingIntervalMs &&
        config.m_link.m_pongTimeoutMs && config.m_link.m_drainTimeoutMs && config.m_link.m_controlStepBudget &&
        config.m_link.m_controlConnection.validCodecSettings();
}

bool NetGatewayDiscoveryConfig::valid() const
{
    return m_retryMs && m_responseMs && m_refreshMs && ValidLinkConfig(m_transport);
}

bool NetPeerConfig::valid() const
{
    return m_direct.m_host.m_eventBudget && m_direct.m_outputBudget && m_direct.m_frameBudget && m_direct.m_drainTimeoutMs &&
        m_direct.m_payloadLimit && m_direct.m_payloadLimit <= NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT &&
        m_direct.m_connection.validCodecSettings() && ValidLinkConfig(m_service) && m_discovery.valid();
}