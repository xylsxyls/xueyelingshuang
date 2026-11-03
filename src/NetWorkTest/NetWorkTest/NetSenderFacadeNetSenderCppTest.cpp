#include "NetSender/NetSenderAPI.h"
#include "NetSenderFacadeNetSenderCppTest.h"
#include "NetWorkTestCases.h"
#include <cstdio>
#include <stdexcept>
#include <type_traits>

// 公开入口不得再次引入桌面内部核心类
typedef int NetSenderIpc;

// 三种提供方共享同一份签名检查，包含返回值、参数、static及const约束
static_assert(std::is_same<decltype(&NetSender::instance), NetSender& (*)()>::value, "NetSender instance contract");
static_assert(std::is_same<decltype(&NetSender::createClient), int32_t (NetSender::*)(NetSenderClient*&)>::value, "NetSender createClient contract");
static_assert(std::is_same<decltype(&NetSender::prepareGateway), int32_t (NetSender::*)(NetSenderClient*)>::value, "NetSender prepareGateway contract");
static_assert(std::is_same<decltype(&NetSender::connectGateway), int32_t (NetSender::*)(NetSenderClient*, const NetSenderGatewayOptions&, NetSessionListener*)>::value, "NetSender connectGateway contract");
static_assert(std::is_same<decltype(&NetSender::connectServiceGateway), int32_t (NetSender::*)(NetSenderClient*, uint32_t, const NetSenderGatewayOptions&, NetSessionListener*)>::value, "NetSender connectServiceGateway contract");
static_assert(std::is_same<decltype(&NetSender::configureCompression), int32_t (NetSender::*)(NetSenderClient*, const uint32_t*, uint32_t)>::value, "NetSender configureCompression contract");
static_assert(std::is_same<decltype(&NetSender::pollGateway), int32_t (NetSender::*)(NetSenderClient*)>::value, "NetSender pollGateway contract");
static_assert(std::is_same<decltype(&NetSender::beginRequest), int32_t (NetSender::*)(NetSenderClient*, uint32_t, uint32_t, uint32_t, uint64_t&)>::value, "NetSender beginRequest contract");
static_assert(std::is_same<decltype(&NetSender::postRequest), int32_t (NetSender::*)(NetSenderClient*, uint64_t, const uint8_t*, uint32_t)>::value, "NetSender postRequest contract");
static_assert(std::is_same<decltype(&NetSender::cancelRequest), int32_t (NetSender::*)(NetSenderClient*, uint64_t)>::value, "NetSender cancelRequest contract");
static_assert(std::is_same<decltype(&NetSender::postData), int32_t (NetSender::*)(NetSenderClient*, const NetInterfaceFrame&)>::value, "NetSender postData contract");
static_assert(std::is_same<decltype(&NetSender::closeGateway), void (NetSender::*)(NetSenderClient*, int32_t)>::value, "NetSender closeGateway contract");
static_assert(std::is_same<decltype(&NetSender::uninitGateway), void (NetSender::*)(NetSenderClient*)>::value, "NetSender uninitGateway contract");
static_assert(std::is_same<decltype(&NetSender::destroyClient), int32_t (NetSender::*)(NetSenderClient*&)>::value, "NetSender destroyClient contract");
static_assert(std::is_same<decltype(&NetSender::gatewayReady), bool (NetSender::*)(NetSenderClient*) const>::value, "NetSender gatewayReady contract");
static_assert(std::is_same<decltype(&NetSender::gatewayGeneration), uint64_t (NetSender::*)(NetSenderClient*) const>::value, "NetSender gatewayGeneration contract");
static_assert(std::is_same<decltype(&NetSender::lastGatewayError), int32_t (NetSender::*)(NetSenderClient*) const>::value, "NetSender lastGatewayError contract");
static_assert(std::is_same<decltype(&NetSender::sendRequest), int32_t (NetSender::*)(NetSenderClient*, uint64_t, const uint8_t*, uint32_t)>::value, "NetSender sendRequest contract");
static_assert(std::is_same<decltype(&NetSender::sendData), int32_t (NetSender::*)(NetSenderClient*, const NetInterfaceFrame&)>::value, "NetSender sendData contract");
static_assert(std::is_same<decltype(&NetSender::gatewayState), int32_t (NetSender::*)(NetSenderClient*, uint64_t&, bool&)>::value, "NetSender gatewayState contract");
static_assert(std::is_same<decltype(&NetSender::beginSendRequest), int32_t (NetSender::*)(NetSenderClient*, uint64_t, const uint8_t*, uint32_t, uint64_t&)>::value, "NetSender beginSendRequest contract");
static_assert(std::is_same<decltype(&NetSender::beginSendData), int32_t (NetSender::*)(NetSenderClient*, const NetInterfaceFrame&, uint64_t&)>::value, "NetSender beginSendData contract");
static_assert(std::is_same<decltype(&NetSender::sendResult), int32_t (NetSender::*)(NetSenderClient*, uint64_t)>::value, "NetSender sendResult contract");
static_assert(std::is_same<decltype(&NetSender::fileProtocol), NetFileProtocol& (NetSender::*)()>::value, "NetSender fileProtocol contract");
static_assert(std::is_same<decltype(&NetSender::pointerBytes), uint32_t (*)()>::value, "NetSender pointerBytes contract");
static_assert(std::is_same<decltype(&NetSender::randomBytes), int32_t (*)(uint8_t*, uint32_t)>::value, "NetSender randomBytes contract");
static_assert(std::is_same<decltype(&NetSender::backend), NetSenderBackend& (*)()>::value, "NetSender backend contract");

void NetSenderFacadeNetSenderCppTest::desktopCpp()
{
    uint32_t checks = 0;
    #define SENDER_CHECK(value) do { netTestCheckpoint(); ++checks; if (!(value)) { std::fprintf(stderr, "facade desktopCpp line=%d checks=%u\n", __LINE__, checks); throw std::runtime_error("facade desktopCpp contract"); } } while (0)
    NetSender& sender = NetSender::instance();
    SENDER_CHECK(&sender == &NetSender::instance() && sizeof(NetSender) == 1);
    NetSenderClient* empty = nullptr;
    uint64_t generation = 19;
    bool ready = true;
    SENDER_CHECK(sender.destroyClient(empty) == NI_OK && empty == nullptr);
    SENDER_CHECK(sender.prepareGateway(nullptr) == NI_INVALID_ARGUMENT);
    SENDER_CHECK(sender.gatewayState(nullptr, generation, ready) == NI_INVALID_ARGUMENT && generation == 0 && !ready);
    uint64_t requestId = 19;
    SENDER_CHECK(sender.beginRequest(nullptr, 1, 1, 10, requestId) < 0 && requestId == 0);
    uint64_t ticket = 19;
    SENDER_CHECK(sender.beginSendRequest(nullptr, 1, nullptr, 0, ticket) < 0 && ticket == 0);
    SENDER_CHECK(sender.pollGateway(nullptr) == NI_INVALID_ARGUMENT);
    for (uint32_t iteration = 0; iteration < 64; ++iteration)
    {
        NetSenderClient* first = nullptr;
        NetSenderClient* second = nullptr;
        SENDER_CHECK(sender.createClient(first) == NI_OK && first != nullptr);
        SENDER_CHECK(sender.gatewayState(first, generation, ready) == NI_OK && generation == 0 && !ready);
        SENDER_CHECK(sender.pollGateway(first) == NI_OK);
        SENDER_CHECK(sender.configureCompression(first, nullptr, 0) == NI_INVALID_STATE);
        NetSenderGatewayOptions coldOptions = {};
        SENDER_CHECK(sender.connectGateway(first, coldOptions, nullptr) == NI_INVALID_STATE);
        NetSenderClient* original = first;
        SENDER_CHECK(sender.createClient(first) == NI_INVALID_ARGUMENT && first == original);
        SENDER_CHECK(sender.createClient(second) == NI_OK && second != nullptr && second != first);
        SENDER_CHECK(sender.prepareGateway(first) == NI_OK && sender.prepareGateway(first) == NI_OK);
        SENDER_CHECK(!sender.gatewayReady(first) && !sender.gatewayReady(second));
        SENDER_CHECK(sender.gatewayGeneration(first) == 0 && sender.gatewayGeneration(second) == 0);
        SENDER_CHECK(sender.configureCompression(first, nullptr, 0) == NI_OK);
        SENDER_CHECK(sender.configureCompression(first, nullptr, 1) < 0);
        requestId = 19;
        SENDER_CHECK(sender.beginRequest(first, 1, 1, 10, requestId) < 0 && requestId == 0);
        SENDER_CHECK(sender.sendRequest(first, 1, nullptr, 0) < 0);
        SENDER_CHECK(sender.postRequest(first, 1, nullptr, 0) < 0);
        SENDER_CHECK(sender.cancelRequest(first, 1) < 0);
        SENDER_CHECK(sender.pollGateway(first) == NI_OK);
        SENDER_CHECK(sender.destroyClient(first) == NI_OK && first == nullptr);
        SENDER_CHECK(sender.prepareGateway(second) == NI_OK);
        SENDER_CHECK(sender.destroyClient(second) == NI_OK && second == nullptr);
        SENDER_CHECK(sender.destroyClient(first) == NI_OK && sender.destroyClient(second) == NI_OK);
    }
    // 从正式上层入口验证文件流创建、快照和取消，不借助私有布局或C++反调C包装。
    NetInterfaceFileInfo info = {};
    info.direction = 1;
    info.state = NI_FILE_TRANSFERRING;
    info.chunk = 1024;
    info.window = 1;
    info.total = 1;
    info.stream = 9;
    info.expiresUtc = 2000000000;
    info.transfer[0] = 1;
    info.token[0] = 2;
    info.hash[0] = 3;
    NetInterfaceFileFlowHandle flow = nullptr;
    SENDER_CHECK(sender.fileProtocol().fileFlowCreate(nullptr, 101, 1, 10, 0, 1000, &flow) == NI_INVALID_ARGUMENT && flow == nullptr);
    SENDER_CHECK(sender.fileProtocol().fileFlowCreate(&info, 101, 1, 10, 0, 1000, &flow) == NI_OK && flow != nullptr);
    uint8_t wire[NET_INTERFACE_FILE_INFO_SIZE] = {};
    int32_t result = NI_INVALID_STATE;
    SENDER_CHECK(sender.fileProtocol().fileFlowSnapshotWire(flow, wire, sizeof(wire), &result) == NI_OK);
    NetInterfaceFileInfo decoded = {};
    SENDER_CHECK(sender.fileProtocol().fileParseInfo(wire, sizeof(wire), &decoded) == NI_OK);
    SENDER_CHECK(decoded.stream == 9 && decoded.total == 1 && decoded.transfer[0] == 1 && decoded.token[0] == 2);
    SENDER_CHECK(sender.fileProtocol().fileFlowCancel(flow, NI_CANCELLED) == NI_OK);
    SENDER_CHECK(sender.fileProtocol().fileFlowSnapshot(flow, &decoded, &result) == NI_OK && result == NI_CANCELLED);
    sender.fileProtocol().fileFlowDestroy(flow);
    sender.fileProtocol().fileFlowDestroy(nullptr);
    SENDER_CHECK(&NetSender::backend() == &NetSender::backend());
    SENDER_CHECK(NetSender::pointerBytes() == sizeof(void*));
    SENDER_CHECK(NetSender::randomBytes(nullptr, 0) == NI_OK);
    SENDER_CHECK(NetSender::randomBytes(nullptr, 1) == NI_INVALID_ARGUMENT);
    uint8_t entropy[34];
    for (uint32_t i = 0; i < 34; ++i)
    {
        entropy[i] = 0xa5;
    }
    SENDER_CHECK(NetSender::randomBytes(entropy + 1, 8193) == NI_INVALID_ARGUMENT);
    for (uint32_t i = 0; i < 34; ++i)
    {
        SENDER_CHECK(entropy[i] == 0xa5);
    }
    SENDER_CHECK(NetSender::randomBytes(entropy + 1, 32) == NI_OK);
    SENDER_CHECK(entropy[0] == 0xa5 && entropy[33] == 0xa5);
    #undef SENDER_CHECK
}