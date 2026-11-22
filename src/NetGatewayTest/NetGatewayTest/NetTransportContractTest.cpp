#include "NetTransportContractTest.h"
#include "NetGatewaySuiteCases.h"
#include "NetInterfaceTransportTest.h"
#include <stdexcept>
#include <cstring>
#include <cstdio>
#include <type_traits>

int NetTransportContractTest::check()
{
    uint32_t assertions = 0;
#define TRANSPORT_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    static_assert(std::is_abstract<NetGatewayTransport>::value, "gateway transport must be injected");
    static_assert(std::is_abstract<NetGatewayTransportFactory>::value, "gateway factory must be injected");
    static_assert(std::is_abstract<NetLinkTransportListener>::value, "management listener interface");
    static_assert(std::is_base_of<NetClientChannel, NetServerChannel>::value, "server extends client transport operations");
    NetInterfaceTransportTestChannel channel;
    // 此默认实现和虚析构必须来自真实NetInterface.lib，不链接NetWork或Libuv
    TRANSPORT_CHECK(static_cast<NetClientChannel*>(&channel)->configureCompression(1, nullptr, 0, false) == NI_NOT_SUPPORTED);
    uint8_t bytes[34];
    std::memset(bytes, 0xa5, sizeof(bytes));
    TRANSPORT_CHECK(NetInterface::entropyBytes(nullptr, nullptr, nullptr, 0) == NI_OK);
    TRANSPORT_CHECK(NetInterface::entropyBytes(nullptr, nullptr, nullptr, 1) == NI_INVALID_ARGUMENT);
    TRANSPORT_CHECK(NetInterface::entropyBytes(nullptr, nullptr, bytes + 1, 8193) == NI_INVALID_ARGUMENT && bytes[1] == 0xa5);
    TRANSPORT_CHECK(NetInterface::entropyBytes(nullptr, nullptr, bytes + 1, 32) == NI_INVALID_STATE);
    for (uint32_t mode = 0; mode <= 2; ++mode)
    {
        const int32_t result = NetInterface::entropyBytes(NetInterfaceTransportTestListener::entropy, &mode, bytes + 1, 32);
        TRANSPORT_CHECK(result == (mode == 0 ? NI_OK : NI_INVALID_STATE));
        TRANSPORT_CHECK(bytes[0] == 0xa5 && bytes[33] == 0xa5);
        for (uint32_t index = 1; index < 33; ++index)
        {
            TRANSPORT_CHECK(bytes[index] == (mode == 0 ? 0x42 : 0));
        }
    }
    NetInterfaceTransportTestListener listener;
    NetSession session;
    NetInterfaceHandshakeOptions hello = {};
    hello.m_platform = 1;
    hello.m_capabilities = 7;
    hello.m_maxRequests = 4;
    hello.m_maxChunk = 4096;
    hello.m_timeoutMs = 1000;
    std::memset(hello.m_device, 0x31, sizeof(hello.m_device));
    NetInterfaceTlsOptions tls = {};
    tls.handshakeTimeoutMs = 1000;
    TRANSPORT_CHECK(session.connect("unused", 1, tls, hello, &listener) == NI_NOT_SUPPORTED);
    TRANSPORT_CHECK(session.bindChannel(&channel) == NI_OK);
    TRANSPORT_CHECK(session.connect("unused", 1, tls, hello, &listener) == NI_INVALID_STATE);
    uint32_t mode = 1;
    tls.entropy = NetInterfaceTransportTestListener::entropy;
    tls.entropyContext = &mode;
    TRANSPORT_CHECK(session.connect("unused", 1, tls, hello, &listener) == NI_INVALID_STATE);
    mode = 0;
    TRANSPORT_CHECK(session.connect("unused", 1, tls, hello, &listener) == NI_NOT_SUPPORTED);
    TRANSPORT_CHECK(session.generation() == 1 && !session.ready() && listener.m_events == 0);
    uint64_t request = 9;
    TRANSPORT_CHECK(session.beginRequest(101, 1, 1000, request) == NI_INVALID_STATE && request == 0);
    session.close();
    session.shutdown();
    TRANSPORT_CHECK(session.bindChannel(nullptr) == NI_OK && listener.m_events == 0);
    NetGatewayClientChannel business(101);
    TRANSPORT_CHECK(!business.poll());
    business.close(1);
    uint64_t peer = 9;
    TRANSPORT_CHECK(business.connect("unused", 1, tls, &listener, peer) == NI_NOT_SUPPORTED && peer == 0);
    TRANSPORT_CHECK(listener.m_events == 0 && !business.poll());
    NetEndpointRouter endpoints(nullptr);
    TRANSPORT_CHECK(endpoints.localPort() == 0);
    TRANSPORT_CHECK(endpoints.listen("unused", 0, tls, 1) == NI_NOT_SUPPORTED);
    TRANSPORT_CHECK(endpoints.bindTransport(nullptr) == NI_OK);
    TRANSPORT_CHECK(endpoints.poll());
    endpoints.shutdown();
    TRANSPORT_CHECK(!endpoints.poll() && endpoints.bindTransport(nullptr) == NI_OK);
    std::printf("NetInterface protocol standalone consumer PASS %u assertions\n", assertions);
    return 0;
#undef TRANSPORT_CHECK
}


void NetTransportContractTest::run()
{
    netTestCheckpoint();
    if (check() != 0)
    {
        throw std::runtime_error("Transport factory, options and same-module ownership failed");
    }
}