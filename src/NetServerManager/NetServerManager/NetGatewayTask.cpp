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

void NetGatewayTask::DoTask()
{
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
                    if (m_configuration.m_runForMs != 0 &&
                        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count() >=
                            m_configuration.m_runForMs) { break; }
                    if (!node.poll())
                    {
                        // 未请求停止时监听宿主不应自行静止，不能向进程监督者报告成功。
                        if (!m_stop.load()) { m_error.store(NI_INVALID_STATE); }
                        break;
                    }
                    CSystem::sleepPrecise(2);
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
                CSystem::sleepPrecise(2);
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

void NetGatewayTask::StopTask()
{
    m_stop.store(true);
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

#if 0 // NET_GATEWAY_TASK_UNIT_BEGIN
// 独立测试入口：停用NetServerManager.cpp的main，启用此处；私钥只保存在指定临时目录。
#include "NetGatewayTask.h"
#include "NetGatewayIdentity.h"
#include <NetWork/NetWorkAPI.h>
#include <LibuvTcp/LibuvTcpAPI.h>
#include <CSystem/CSystemAPI.h>
#include <cstdio>
#include <stdexcept>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    try
    {
        static uint32_t s_fault = 0;
        static uint32_t s_calls = 0;
        NetHostFunctions functions = {};
        functions.structSize = sizeof(functions);
        functions.version = 1;
        functions.create = LibuvTcp::hostCreate;
        functions.listen = LibuvTcp::hostListen;
        functions.connect = LibuvTcp::hostConnect;
        functions.poll = [](void* instance) -> int32_t
        {
            ++s_calls;
            if (s_calls >= 4)
            {
                if (s_fault == 1) { return -1; }
                if (s_fault == 2) { throw std::runtime_error("test host poll failure"); }
                if (s_fault == 3) { return 0; }
            }
            return LibuvTcp::hostPoll(instance);
        };
        functions.write = LibuvTcp::hostWrite;
        functions.pauseRead = LibuvTcp::hostPauseRead;
        functions.close = LibuvTcp::hostClose;
        functions.shutdown = LibuvTcp::hostShutdown;
        functions.destroy = LibuvTcp::hostDestroy;
        functions.localPort = LibuvTcp::hostLocalPort;
        functions.peerCount = LibuvTcp::hostPeerCount;
        functions.entropy = CSystem::secureRandomBytesCallback;
        if (NetWork::configureHost(functions) != NI_OK || NetWork::configureNativeFileIo() != NI_OK)
        {
            return 3;
        }
        NetGatewayIdentity identity;
        if (identity.loadOrCreate(argv[1]) != NI_OK)
        {
            return 4;
        }
        uint32_t checks = 0;
        for (uint32_t protocol = 2; protocol <= 3; ++protocol)
        {
            for (uint32_t mode = 0; mode < 5; ++mode)
            {
                s_fault = mode;
                s_calls = 0;
                NetGatewayConfiguration configuration;
                configuration.m_address = "127.0.0.1";
                configuration.m_advertise = "127.0.0.1";
                configuration.m_port = 0;
                configuration.m_runForMs = mode >= 1 && mode <= 3 ? 1000 : 20;
                configuration.m_protocol = protocol;
                configuration.m_identityDirectory = argv[1];
                configuration.m_nodeName = identity.m_name;
                configuration.m_certificate = identity.m_certificate;
                configuration.m_privateKey = identity.m_privateKey;
                NetGatewayTask task(configuration);
                if (mode == 4)
                {
                    task.StopTask();
                }
                task.DoTask();
                const int32_t expected = mode == 0 || mode == 4 ? NI_OK : NI_INVALID_STATE;
                const bool passed = task.done() && task.port() != 0 && task.error() == expected && (mode == 0 || mode == 4 || s_calls >= 4);
                std::printf("GATEWAY_TASK protocol=%u mode=%u polls=%u done=%u port_ready=%u error=%d expected=%d %s\n",
                    protocol, mode, s_calls, task.done() ? 1U : 0U, task.port() != 0 ? 1U : 0U,
                    task.error(), expected, passed ? "PASS" : "FAIL");
                if (!passed)
                {
                    return 1;
                }
                ++checks;
            }
        }
        std::printf("GATEWAY_TASK checks=%u PASS\n", checks);
        return 0;
    }
    catch (...)
    {
        std::puts("GATEWAY_TASK unexpected test exception");
        return 1;
    }
}

#endif // NET_GATEWAY_TASK_UNIT_END