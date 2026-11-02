#include "NetClientGatewayTask.h"
#include "NetClientOpaqueGateway.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <cstring>

NetClientGatewayTask::NetClientGatewayTask(const NetClientGatewayConfiguration& configuration) :
m_configuration(configuration),
m_stop(false),
m_done(false),
m_ready(false),
m_error(NI_OK)
{

}

NetClientGatewayTask::~NetClientGatewayTask()
{
    netInterfaceWipe(m_configuration.m_privateKey.empty() ? nullptr : reinterpret_cast<uint8_t*>(&m_configuration.m_privateKey[0]),
                     static_cast<uint32_t>(m_configuration.m_privateKey.size()));
}

void NetClientGatewayTask::DoTask()
{
    try
    {
        if (m_configuration.m_protocol == 3)
        {
            NetClientOpaqueGateway gateway;
            NetClientGatewayAddress address;
            address.m_address = m_configuration.m_address;
            address.m_port = m_configuration.m_port;
            std::vector<NetClientGatewayAddress> addresses(1, address);
            for (const auto& seed : m_configuration.m_seeds)
            {
                bool duplicate = false;
                for (const auto& point : addresses)
                {
                    if (point.m_address == seed.m_address && point.m_port == seed.m_port) { duplicate = true; break; }
                }
                if (!duplicate) { addresses.push_back(seed); }
            }
            if (addresses.size() > 64) { m_error.store(NI_LIMIT_EXCEEDED); m_done.store(true); return; }
            m_error.store(gateway.init(addresses, m_configuration.m_cachePath));
            const auto started = std::chrono::steady_clock::now();
            while (m_error.load() == NI_OK && !m_stop.load())
            {
                if (m_configuration.m_runForMs != 0 &&
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count() >=
                        m_configuration.m_runForMs)
                {
                    break;
                }
                if (!gateway.poll()) { break; }
                m_ready.store(true);
                CSystem::sleepPrecise(2);
            }
            m_ready.store(false);
            gateway.shutdown();
            // 已停止接纳业务，生命周期执行者收敛异步缓存，不在网络poll中等待磁盘
            while (gateway.poll())
            {
                CSystem::sleepPrecise(1);
            }
            m_done.store(true);
            return;
        }
        NetClientGateway gateway;
        NetWorkClientOptions options = {};
        options.structSize = sizeof(options);
        options.version = 1;
        options.address = m_configuration.m_address.c_str();
        options.port = m_configuration.m_port;
        options.tls.authority = reinterpret_cast<const uint8_t*>(m_configuration.m_authority.c_str());
        options.tls.authoritySize = static_cast<uint32_t>(m_configuration.m_authority.size() + 1);
        options.tls.hostname = m_configuration.m_hostname.c_str();
        options.tls.handshakeTimeoutMs = 10000;
        options.platform = 1;
        options.capabilities = 7;
        options.maxRequests = 32;
        options.maxChunk = 65536;
        options.handshakeTimeoutMs = 5000;
        std::memcpy(options.device, m_configuration.m_device, sizeof(options.device));
        NetInterfaceTlsOptions local = {};
        local.server = 1;
        local.certificate = reinterpret_cast<const uint8_t*>(m_configuration.m_certificate.c_str());
        local.certificateSize = static_cast<uint32_t>(m_configuration.m_certificate.size() + 1);
        local.privateKey = reinterpret_cast<const uint8_t*>(m_configuration.m_privateKey.c_str());
        local.privateKeySize = static_cast<uint32_t>(m_configuration.m_privateKey.size() + 1);
        local.handshakeTimeoutMs = 10000;
        m_error.store(gateway.init(local, options));
        const auto started = std::chrono::steady_clock::now();
        while (m_error.load() == NI_OK && !m_stop.load())
        {
            if (m_configuration.m_runForMs != 0 &&
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count() >=
                    m_configuration.m_runForMs)
            {
                break;
            }
            if (!gateway.poll())
            {
                break;
            }
            m_ready.store(gateway.ready());
            CSystem::sleepPrecise(2);
        }
        m_ready.store(false);
        gateway.shutdown();
    }
    catch (...)
    {
        m_error.store(NI_INVALID_STATE);
    }
    m_done.store(true);
}

void NetClientGatewayTask::StopTask()
{
    m_stop.store(true);
}

bool NetClientGatewayTask::ready() const
{
    return m_ready.load();
}

bool NetClientGatewayTask::done() const
{
    return m_done.load();
}

int32_t NetClientGatewayTask::error() const
{
    return m_error.load();
}