#include "NetSenderMacro.h"
#include "NetSenderIpc.h"
#include "NetSender.h"

NetSender::NetSender()
{

}

NetSender::~NetSender()
{

}

NetSender& NetSender::instance()
{
    static NetSender s_instance;
    return s_instance;
}

int32_t NetSender::createClient(NetSenderGatewayState*& client)
{
    return NetSenderIpc::instance().createClient(client);
}

int32_t NetSender::prepareGateway(NetSenderGatewayState* client)
{
    return NetSenderIpc::instance().prepareGateway(client);
}

int32_t NetSender::connectGateway(NetSenderGatewayState* client, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    return NetSenderIpc::instance().connectGateway(client, options, listener);
}

int32_t NetSender::connectServiceGateway(NetSenderGatewayState* client, uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    return NetSenderIpc::instance().connectServiceGateway(client, product, options, listener);
}

int32_t NetSender::configureCompression(NetSenderGatewayState* client, const uint32_t* types, uint32_t count)
{
    return NetSenderIpc::instance().configureCompression(client, types, count);
}

int32_t NetSender::pollGateway(NetSenderGatewayState* client)
{
    return NetSenderIpc::instance().pollGateway(client);
}

int32_t NetSender::beginRequest(NetSenderGatewayState* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId)
{
    return NetSenderIpc::instance().beginRequest(client, product, type, timeoutMs, requestId);
}

int32_t NetSender::postRequest(NetSenderGatewayState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    return NetSenderIpc::instance().postRequest(client, requestId, bytes, size);
}

int32_t NetSender::cancelRequest(NetSenderGatewayState* client, uint64_t requestId)
{
    return NetSenderIpc::instance().cancelRequest(client, requestId);
}

int32_t NetSender::postData(NetSenderGatewayState* client, const NetInterfaceFrame& frame)
{
    return NetSenderIpc::instance().postData(client, frame);
}

void NetSender::closeGateway(NetSenderGatewayState* client, int32_t reason)
{
    NetSenderIpc::instance().closeGateway(client, reason);
}

void NetSender::uninitGateway(NetSenderGatewayState* client)
{
    NetSenderIpc::instance().uninitGateway(client);
}

int32_t NetSender::destroyClient(NetSenderGatewayState*& client)
{
    return NetSenderIpc::instance().destroyClient(client);
}

bool NetSender::gatewayReady(NetSenderGatewayState* client) const
{
    return NetSenderIpc::instance().gatewayReady(client);
}

uint64_t NetSender::gatewayGeneration(NetSenderGatewayState* client) const
{
    return NetSenderIpc::instance().gatewayGeneration(client);
}

int32_t NetSender::lastGatewayError(NetSenderGatewayState* client) const
{
    return NetSenderIpc::instance().lastGatewayError(client);
}

int32_t NetSender::sendRequest(NetSenderGatewayState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    return NetSenderIpc::instance().sendRequest(client, requestId, bytes, size);
}

int32_t NetSender::sendData(NetSenderGatewayState* client, const NetInterfaceFrame& frame)
{
    return NetSenderIpc::instance().sendData(client, frame);
}

#if 0 // LEGACY_COMPAT_UNIT_TEST_BEGIN
#include "NetSenderLegacy.h"
#include "NetSender/NetSenderLegacyTest.h"
#include <climits>
#include <cstdio>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif

NetSenderLegacyClientProbe::NetSenderLegacyClientProbe() :
m_calls(0)
{

}

void NetSenderLegacyClientProbe::inject(MessageType type, const char* bytes, int32_t size)
{
    receive(19, bytes, size, type);
}

void NetSenderLegacyClientProbe::clientInitResponse(int32_t serverId, const char* bytes, int32_t size)
{
    (void)serverId;
    (void)bytes;
    (void)size;
    ++m_calls;
}

void NetSenderLegacyClientProbe::ServerMessage(int32_t serverId, const char* bytes, int32_t size)
{
    (void)serverId;
    (void)bytes;
    (void)size;
    ++m_calls;
}

NetSenderLegacyServerProbe::NetSenderLegacyServerProbe() :
m_calls(0)
{

}

void NetSenderLegacyServerProbe::inject(MessageType type, const char* bytes, int32_t size)
{
    receive(19, bytes, size, type);
}

void NetSenderLegacyServerProbe::clientInit(int32_t connection, int32_t pid, const char* bytes, int32_t size)
{
    (void)connection;
    (void)pid;
    (void)bytes;
    (void)size;
    ++m_calls;
}

void NetSenderLegacyServerProbe::serverInitResponse(const char* bytes, int32_t size)
{
    (void)bytes;
    (void)size;
    ++m_calls;
}

void NetSenderLegacyServerProbe::ClientMessage(int32_t connection, int32_t pid, const char* bytes, int32_t size)
{
    (void)connection;
    (void)pid;
    (void)bytes;
    (void)size;
    ++m_calls;
}

/** 旧接入只验证内存/类型边界，不将旧明文协议认作安全接入。
@return 0通过，1失败
*/
int main()
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

#endif // LEGACY_COMPAT_UNIT_TEST_END

int32_t NetSender::gatewayState(NetSenderGatewayState* client, uint64_t& generation, bool& ready)
{
    return NetSenderIpc::instance().gatewayState(client, generation, ready);
}

int32_t NetSender::beginSendRequest(NetSenderGatewayState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t& ticket)
{
    return NetSenderIpc::instance().beginSendRequest(client, requestId, bytes, size, ticket);
}

int32_t NetSender::beginSendData(NetSenderGatewayState* client, const NetInterfaceFrame& frame, uint64_t& ticket)
{
    return NetSenderIpc::instance().beginSendData(client, frame, ticket);
}

int32_t NetSender::sendResult(NetSenderGatewayState* client, uint64_t ticket)
{
    return NetSenderIpc::instance().sendResult(client, ticket);
}

// NET_SENDER_FACADE_UNIT_BEGIN
// #include <cstdio>
//
// int main()
// {
//     uint32_t checks = 0;
//     #define SENDER_CHECK(value) do { ++checks; if (!(value)) { std::printf("FAIL line=%d checks=%u\n", __LINE__, checks); return 1; } } while (0)
//     NetSender& sender = NetSender::instance();
//     SENDER_CHECK(&sender == &NetSender::instance() && sizeof(NetSender) == 1);
//     NetSenderClient* empty = nullptr;
//     uint64_t generation = 19;
//     bool ready = true;
//     SENDER_CHECK(sender.destroyClient(empty) == NI_OK && empty == nullptr);
//     SENDER_CHECK(sender.prepareGateway(nullptr) == NI_INVALID_ARGUMENT);
//     SENDER_CHECK(sender.gatewayState(nullptr, generation, ready) == NI_INVALID_ARGUMENT && generation == 0 && !ready);
//     uint64_t requestId = 19;
//     SENDER_CHECK(sender.beginRequest(nullptr, 1, 1, 10, requestId) < 0 && requestId == 0);
//     uint64_t ticket = 19;
//     SENDER_CHECK(sender.beginSendRequest(nullptr, 1, nullptr, 0, ticket) < 0 && ticket == 0);
//     SENDER_CHECK(sender.pollGateway(nullptr) == NI_INVALID_ARGUMENT);
//     for (uint32_t iteration = 0; iteration < 64; ++iteration)
//     {
//         NetSenderClient* first = nullptr;
//         NetSenderClient* second = nullptr;
//         SENDER_CHECK(sender.createClient(first) == NI_OK && first != nullptr);
//         SENDER_CHECK(sender.gatewayState(first, generation, ready) == NI_OK && generation == 0 && !ready);
//         SENDER_CHECK(sender.pollGateway(first) == NI_OK);
//         SENDER_CHECK(sender.configureCompression(first, nullptr, 0) == NI_INVALID_STATE);
//         NetSenderGatewayOptions coldOptions = {};
//         SENDER_CHECK(sender.connectGateway(first, coldOptions, nullptr) == NI_INVALID_STATE);
//         NetSenderClient* original = first;
//         SENDER_CHECK(sender.createClient(first) == NI_INVALID_ARGUMENT && first == original);
//         SENDER_CHECK(sender.createClient(second) == NI_OK && second != nullptr && second != first);
//         SENDER_CHECK(sender.prepareGateway(first) == NI_OK && sender.prepareGateway(first) == NI_OK);
//         SENDER_CHECK(!sender.gatewayReady(first) && !sender.gatewayReady(second));
//         SENDER_CHECK(sender.gatewayGeneration(first) == 0 && sender.gatewayGeneration(second) == 0);
//         SENDER_CHECK(sender.configureCompression(first, nullptr, 0) == NI_OK);
//         SENDER_CHECK(sender.configureCompression(first, nullptr, 1) < 0);
//         requestId = 19;
//         SENDER_CHECK(sender.beginRequest(first, 1, 1, 10, requestId) < 0 && requestId == 0);
//         SENDER_CHECK(sender.sendRequest(first, 1, nullptr, 0) < 0);
//         SENDER_CHECK(sender.postRequest(first, 1, nullptr, 0) < 0);
//         SENDER_CHECK(sender.cancelRequest(first, 1) < 0);
//         SENDER_CHECK(sender.pollGateway(first) == NI_OK);
//         SENDER_CHECK(sender.destroyClient(first) == NI_OK && first == nullptr);
//         SENDER_CHECK(sender.prepareGateway(second) == NI_OK);
//         SENDER_CHECK(sender.destroyClient(second) == NI_OK && second == nullptr);
//         SENDER_CHECK(sender.destroyClient(first) == NI_OK && sender.destroyClient(second) == NI_OK);
//     }
//     std::printf("PASS desktop singleton facade checks=%u\n", checks);
//     #undef SENDER_CHECK
//     return 0;
// }
// NET_SENDER_FACADE_UNIT_END

NetFileProtocol& NetSender::fileProtocol()
{
    return NetSenderIpc::instance().fileProtocol();
}

uint32_t NetSender::pointerBytes()
{
    return NetSenderIpc::pointerBytes();
}

int32_t NetSender::randomBytes(uint8_t* output, uint32_t size)
{
    return NetSenderIpc::randomBytes(output, size);
}

NetSenderBackend& NetSender::backend()
{
    return NetSenderIpc::backend();
}