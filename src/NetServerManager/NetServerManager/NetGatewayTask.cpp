#include "NetGatewayTask.h"
#include "NetGatewayServer.h"
#include "NetGatewayNode.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>

NetGatewayConfiguration::NetGatewayConfiguration() : m_address("0.0.0.0"), m_port(5203), m_peers(256), m_runForMs(0), m_protocol(3)
{

}

NetGatewayTask::NetGatewayTask(const NetGatewayConfiguration& configuration) :
m_configuration(configuration),
m_stop(false),
m_done(false),
m_port(0),
m_error(0)
{

}

void NetGatewayTask::doTask()
{
    NetExecutionScope execution(m_execution);
    if (m_execution.status() != NI_OK)
    {
        m_error.store(m_execution.status());
        m_done.store(true);
        return;
    }
    try
    {
        if (m_configuration.m_protocol == 3)
        {
            NetGatewayNode node(m_configuration.m_nodeName, m_configuration.m_certificate, m_configuration.m_privateKey);
            m_error.store(node.listen(m_configuration.m_address.c_str(), m_configuration.m_port,
                m_configuration.m_advertise, m_configuration.m_peers));
            if (m_error.load() == NI_OK)
            {
                m_error.store(node.configureCache(CSystem::joinPath(m_configuration.m_identityDirectory, "nodes.cache")));
            }
            if (m_error.load() == NI_OK)
            {
                for (const auto& seed : m_configuration.m_seeds)
                {
                    const int32_t added = node.addSeed(seed.first, seed.second);
                    if (added != NI_OK) { m_error.store(added); break; }
                }
            }
            if (m_error.load() == NI_OK)
            {
                m_port.store(node.localPort());
                const auto started = std::chrono::steady_clock::now();
                while (!m_stop.load())
                {
                    m_execution.begin();
                    if (m_configuration.m_runForMs != 0 &&
                        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count() >=
                            m_configuration.m_runForMs) { break; }
                    if (!node.poll())
                    {
                        // 未请求停止时监听宿主不应自行静止，不能向进程监督者报告成功。
                        if (!m_stop.load()) { m_error.store(NI_INVALID_STATE); }
                        break;
                    }
                    if (!m_stop.load())
                    {
                        const int32_t waited = m_execution.wait(100);
                        if (waited == NI_NEED_MORE)
                        {
                            CSystem::sleepPrecise(2);
                        }
                        else if (waited < 0)
                        {
                            m_error.store(waited);
                            break;
                        }
                    }
                }
            }
            node.shutdown();
            // 业务运输已经停止，由生命周期执行者推进缓存资源收敛
            while (node.poll())
            {
                CSystem::sleepPrecise(1);
            }
            m_done.store(true);
            return;
        }
        NetGatewayLocalServer runtime;
        NetGatewayServer& gateway = runtime.gateway();
        NetInterfaceTlsOptions tls = {};
        tls.server = 1;
        tls.certificate = reinterpret_cast<const uint8_t*>(m_configuration.m_certificate.c_str());
        tls.certificateSize = static_cast<uint32_t>(m_configuration.m_certificate.size() + 1);
        tls.privateKey = reinterpret_cast<const uint8_t*>(m_configuration.m_privateKey.c_str());
        tls.privateKeySize = static_cast<uint32_t>(m_configuration.m_privateKey.size() + 1);
        tls.handshakeTimeoutMs = 10000;
        if (!m_configuration.m_services.empty())
        {
            m_error.store(runtime.startLocal(tls, m_configuration.m_services));
        }
        if (m_error.load() == NI_OK)
        {
            m_error.store(gateway.listen(m_configuration.m_address.c_str(), m_configuration.m_port, tls, m_configuration.m_peers));
        }
        if (m_error.load() == NI_OK)
        {
            m_port.store(gateway.localPort());
            const auto started = std::chrono::steady_clock::now();
            while (!m_stop.load())
            {
                m_execution.begin();
                if (m_configuration.m_runForMs != 0 &&
                    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count() >=
                        m_configuration.m_runForMs)
                {
                    break;
                }
                if (!runtime.poll())
                {
                    if (!m_stop.load()) { m_error.store(NI_INVALID_STATE); }
                    break;
                }
                if (!m_stop.load())
                {
                    const int32_t waited = m_execution.wait(100);
                    if (waited == NI_NEED_MORE)
                    {
                        CSystem::sleepPrecise(2);
                    }
                    else if (waited < 0)
                    {
                        m_error.store(waited);
                        break;
                    }
                }
            }
        }
        runtime.shutdown();
    }
    catch (...)
    {
        m_error.store(NI_INVALID_STATE);
    }
    m_done.store(true);
}

void NetGatewayTask::stopTask()
{
    m_stop.store(true);
    m_execution.wake();
}

uint32_t NetGatewayTask::port() const
{
    return m_port.load();
}

bool NetGatewayTask::done() const
{
    return m_done.load();
}

int32_t NetGatewayTask::error() const
{
    return m_error.load();
}