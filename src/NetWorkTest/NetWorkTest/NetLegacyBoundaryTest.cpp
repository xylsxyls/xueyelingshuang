#include "NetLegacyBoundaryTest.h"
#include "NetSenderTestAccess.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
#include "NetSenderLegacyTest.h"
#include <stdexcept>
#include <climits>
#include <cstdio>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif
/** 旧接入只验证内存/类型边界，不将旧明文协议认作安全接入。
@return 0通过，1失败
*/
int NetLegacyBoundaryTest::check()
{
    uint32_t assertions = 0;
#define LEGACY_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    NetSenderLegacyClientProbe client;
    NetSenderLegacyServerProbe server;
    const char packet[16] = {1, 0, 0, 0, 2, 0, 0, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    for (int32_t length = -1; length < 8; ++length)
    {
        client.inject(SERVER_MESSAGE, nullptr, length);
        server.inject(MESSAGE, nullptr, length);
        client.inject(SERVER_MESSAGE, packet + 1, length);
        server.inject(MESSAGE, packet + 1, length);
        LEGACY_CHECK(client.m_calls == 0 && server.m_calls == 0);
    }
    client.inject(SERVER_MESSAGE, packet + 1, 8);
    server.inject(MESSAGE, packet + 1, 8);
    LEGACY_CHECK(client.m_calls == 1 && server.m_calls == 1);
    client.inject(CLIENT_INIT_RESPONSE, packet, 16);
    server.inject(CLIENT_INIT, packet, 16);
    LEGACY_CHECK(client.m_calls == 1 && server.m_calls == 1);
    ProtoMessage invalid;
    invalid[SERVER_ID] = std::string("incorrect integer type");
    invalid[CLIENT_INIT_RESPONSE_INFO] = std::string("not admitted");
    std::string bytes = invalid.toString();
    client.inject(CLIENT_INIT_RESPONSE, bytes.data(), static_cast<int32_t>(bytes.size()));
    invalid.clear();
    invalid[CONNECT_ID] = 1;
    invalid[CLIENT_PID] = std::string("incorrect integer type");
    invalid[CLIENT_INIT_INFO] = std::string("not admitted");
    bytes = invalid.toString();
    server.inject(CLIENT_INIT, bytes.data(), static_cast<int32_t>(bytes.size()));
    LEGACY_CHECK(client.m_calls == 1 && server.m_calls == 1);
    invalid.clear();
    invalid[SERVER_ID] = 1;
    invalid[CLIENT_INIT_RESPONSE_INFO] = std::string("accepted");
    bytes = invalid.toString();
    client.inject(CLIENT_INIT_RESPONSE, bytes.data(), static_cast<int32_t>(bytes.size()));
    invalid.clear();
    invalid[CONNECT_ID] = 1;
    invalid[CLIENT_PID] = 2;
    invalid[CLIENT_INIT_INFO] = std::string("accepted");
    bytes = invalid.toString();
    server.inject(CLIENT_INIT, bytes.data(), static_cast<int32_t>(bytes.size()));
    LEGACY_CHECK(client.m_calls == 2 && server.m_calls == 2);
    NetSenderLegacy& sender = NetSenderLegacy::instance();
    LEGACY_CHECK(sender.netServerMessage(2, nullptr, -1).empty());
    LEGACY_CHECK(sender.netClientMessage(1, nullptr, 1).empty());
    LEGACY_CHECK(sender.netClientMessage(1, reinterpret_cast<const char*>(1), INT_MAX).empty());
    sender.sendServer(1, nullptr, 1);
    sender.postClient(1, 2, reinterpret_cast<const char*>(1), INT_MAX);
    for (int32_t length = 0; length <= 8; ++length)
    {
        const std::string encoded = sender.netServerMessage(2, packet, length);
        int32_t header = 0;
        std::memcpy(&header, encoded.data(), sizeof(header));
        LEGACY_CHECK(header == 2 && encoded.size() == static_cast<uint32_t>(length + 4));
        LEGACY_CHECK(length == 0 || std::memcmp(encoded.data() + 4, packet, static_cast<uint32_t>(length)) == 0);
    }
#ifdef _WIN32
    SYSTEM_INFO information = {};
    GetSystemInfo(&information);
    uint8_t* guarded = static_cast<uint8_t*>(VirtualAlloc(nullptr, information.dwPageSize * 2, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    LEGACY_CHECK(guarded != nullptr);
    DWORD old = 0;
    LEGACY_CHECK(VirtualProtect(guarded + information.dwPageSize, information.dwPageSize, PAGE_NOACCESS, &old) != 0);
    char* body = reinterpret_cast<char*>(guarded + information.dwPageSize - 4);
    std::memset(body, 'Q', 4);
    client.ClientReceiveCallback::NetClientMessage(body, 4);
    client.ClientReceiveCallback::NetServerMessage(body, 4);
    server.ServerReceiveCallback::NetClientMessage(1, body, 4);
    server.ServerReceiveCallback::NetServerMessage(body, 4);
    LEGACY_CHECK(VirtualFree(guarded, 0, MEM_RELEASE) != 0);
#endif
    std::printf("NetSender legacy boundary assertions=%u PASS\n", assertions);
    return 0;
#undef LEGACY_CHECK
}

void NetLegacyBoundaryTest::run()
{
    if (check() != 0)
    {
        throw std::runtime_error("NetLegacyBoundaryTest failed");
    }
}