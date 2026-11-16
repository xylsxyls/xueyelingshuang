#include "NetGatewayTaskTest.h"
#include "NetIsolatedTest.h"
#include "../../NetServerManager/NetServerManager/NetGatewayTask.h"
#include "../../NetServerManager/NetServerManager/NetGatewayIdentity.h"
#include <NetWork/NetWorkAPI.h>
#include <LibuvTcp/LibuvTcpAPI.h>
#include <CSystem/CSystemAPI.h>
#include <cstdio>
#include <stdexcept>

int NetGatewayTaskTest::check(const std::string& root)
{
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
        functions.write = LibuvTcp::post;
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
        if (identity.loadOrCreate(root) != NI_OK)
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
                configuration.m_identityDirectory = root;
                configuration.m_nodeName = identity.m_name;
                configuration.m_certificate = identity.m_certificate;
                configuration.m_privateKey = identity.m_privateKey;
                NetGatewayTask task(configuration);
                if (mode == 4)
                {
                    task.stopTask();
                }
                task.doTask();
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



void NetGatewayTaskTest::run()
{
    if (NetIsolatedTest::run("--isolated-manager=task", 30000) != 0)
    {
        throw std::runtime_error("gateway task lifecycle failed");
    }
}