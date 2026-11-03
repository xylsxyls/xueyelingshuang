#include "NetWorkMacro.h"
#include "NetSender.h"
#include "NetWork.h"

NetWorkNetSender::NetWorkNetSender()
{

}

NetWorkNetSender::~NetWorkNetSender()
{

}

NetWorkNetSender& NetWorkNetSender::instance()
{
    static NetWorkNetSender s_instance;
    return s_instance;
}

int32_t NetWorkNetSender::createClient(NetWorkSenderState*& client)
{
    return NetWork::instance().createClient(client);
}

int32_t NetWorkNetSender::prepareGateway(NetWorkSenderState* client)
{
    return NetWork::instance().prepareGateway(client);
}

int32_t NetWorkNetSender::connectGateway(NetWorkSenderState* client, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    return NetWork::instance().connectGateway(client, options, listener);
}

int32_t NetWorkNetSender::connectServiceGateway(NetWorkSenderState* client, uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener)
{
    return NetWork::instance().connectServiceGateway(client, product, options, listener);
}

int32_t NetWorkNetSender::configureCompression(NetWorkSenderState* client, const uint32_t* types, uint32_t count)
{
    return NetWork::instance().configureCompression(client, types, count);
}

int32_t NetWorkNetSender::pollGateway(NetWorkSenderState* client)
{
    return NetWork::instance().pollGateway(client);
}

int32_t NetWorkNetSender::beginRequest(NetWorkSenderState* client, uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& id)
{
    return NetWork::instance().beginRequest(client, product, type, timeoutMs, id);
}

int32_t NetWorkNetSender::postRequest(NetWorkSenderState* client, uint64_t id, const uint8_t* bytes, uint32_t size)
{
    return NetWork::instance().postRequest(client, id, bytes, size);
}

int32_t NetWorkNetSender::cancelRequest(NetWorkSenderState* client, uint64_t id)
{
    return NetWork::instance().cancelRequest(client, id);
}

int32_t NetWorkNetSender::postData(NetWorkSenderState* client, const NetInterfaceFrame& frame)
{
    return NetWork::instance().postData(client, frame);
}

void NetWorkNetSender::closeGateway(NetWorkSenderState* client, int32_t reason)
{
    NetWork::instance().closeGateway(client, reason);
}

void NetWorkNetSender::uninitGateway(NetWorkSenderState* client)
{
    NetWork::instance().uninitGateway(client);
}

int32_t NetWorkNetSender::destroyClient(NetWorkSenderState*& client)
{
    return NetWork::instance().destroyClient(client);
}

bool NetWorkNetSender::gatewayReady(NetWorkSenderState* client) const
{
    return NetWork::instance().gatewayReady(client);
}

uint64_t NetWorkNetSender::gatewayGeneration(NetWorkSenderState* client) const
{
    return NetWork::instance().gatewayGeneration(client);
}

int32_t NetWorkNetSender::lastGatewayError(NetWorkSenderState* client) const
{
    return NetWork::instance().lastGatewayError(client);
}

int32_t NetWorkNetSender::sendRequest(NetWorkSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size)
{
    return NetWork::instance().sendRequest(client, requestId, bytes, size);
}

int32_t NetWorkNetSender::sendData(NetWorkSenderState* client, const NetInterfaceFrame& frame)
{
    return NetWork::instance().sendData(client, frame);
}

int32_t NetWorkNetSender::gatewayState(NetWorkSenderState* client, uint64_t& generation, bool& ready)
{
    return NetWork::instance().gatewayState(client, generation, ready);
}

int32_t NetWorkNetSender::beginSendRequest(NetWorkSenderState* client, uint64_t requestId, const uint8_t* bytes, uint32_t size, uint64_t& ticket)
{
    return NetWork::instance().beginSendRequest(client, requestId, bytes, size, ticket);
}

int32_t NetWorkNetSender::beginSendData(NetWorkSenderState* client, const NetInterfaceFrame& frame, uint64_t& ticket)
{
    return NetWork::instance().beginSendData(client, frame, ticket);
}

int32_t NetWorkNetSender::sendResult(NetWorkSenderState* client, uint64_t ticket)
{
    return NetWork::instance().sendResult(client, ticket);
}

NetFileProtocol& NetWorkNetSender::fileProtocol()
{
    return NetWork::instance().fileProtocol();
}

uint32_t NetWorkNetSender::pointerBytes()
{
    return NetWork::pointerBytes();
}

int32_t NetWorkNetSender::randomBytes(uint8_t* output, uint32_t size)
{
    return NetWork::randomBytes(output, size);
}

// NET_SENDER_FACADE_UNIT_BEGIN
// #include "NetSenderC.h"
// #include <cstdio>
//
// /** 记录测试就绪回调
// @param [in] context 四项计数数组
// @param [in] generation 本用例不建立真实连接
// @param [in] requests 未使用
// @param [in] chunk 未使用
// */
// static void SenderUnitReady(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
// {
//     (void)generation; (void)requests; (void)chunk;
//     ++static_cast<uint32_t*>(context)[0];
// }
//
// /** 记录测试结果回调
// @param [in] context 四项计数数组
// @param [in] generation 未使用
// @param [in] requestId 未使用
// @param [in] product 未使用
// @param [in] type 未使用
// @param [in] status 未使用
// @param [in] bytes 未使用
// @param [in] size 未使用
// */
// static void SenderUnitResult(void* context, uint64_t generation, uint64_t requestId, uint32_t product,
//     uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
// {
//     (void)generation; (void)requestId; (void)product; (void)type; (void)status; (void)bytes; (void)size;
//     ++static_cast<uint32_t*>(context)[1];
// }
//
// /** 记录帧回调且声明已消费
// @param [in] context 四项计数数组
// @param [in] generation 未使用
// @param [in] frame 未使用
// @return 1已消费
// */
// static int32_t SenderUnitFrame(void* context, uint64_t generation, const NetInterfaceFrame* frame)
// {
//     (void)generation; (void)frame;
//     ++static_cast<uint32_t*>(context)[2];
//     return 1;
// }
//
// /** 记录关闭回调
// @param [in] context 四项计数数组
// @param [in] generation 未使用
// @param [in] reason 未使用
// @param [in] detail 未使用
// */
// static void SenderUnitClosed(void* context, uint64_t generation, int32_t reason, int32_t detail)
// {
//     (void)generation; (void)reason; (void)detail;
//     ++static_cast<uint32_t*>(context)[3];
// }
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
//     uint32_t events[4] = {};
//     NetInterfaceClientCallbacks callbacks = {};
//     callbacks.structSize = sizeof(callbacks);
//     callbacks.version = 1;
//     callbacks.context = events;
//     callbacks.ready = SenderUnitReady;
//     callbacks.result = SenderUnitResult;
//     callbacks.frame = SenderUnitFrame;
//     callbacks.closed = SenderUnitClosed;
//     NetSenderClientHandle invalid = nullptr;
//     SENDER_CHECK(netSenderClientCreate(nullptr, &invalid) == NI_INVALID_ARGUMENT && invalid == nullptr);
//     SENDER_CHECK(netSenderClientCreate(&callbacks, nullptr) == NI_INVALID_ARGUMENT);
//     callbacks.version = 0;
//     SENDER_CHECK(netSenderClientCreate(&callbacks, &invalid) == NI_INVALID_ARGUMENT && invalid == nullptr);
//     callbacks.version = 1;
//     SENDER_CHECK(netSenderClientDestroy(nullptr) == NI_OK);
//     SENDER_CHECK(netSenderClientPoll(nullptr) == NI_INVALID_ARGUMENT);
//     requestId = 19;
//     SENDER_CHECK(netSenderClientBegin(nullptr, 1, 1, 10, &requestId) == NI_INVALID_ARGUMENT && requestId == 0);
//     for (uint32_t iteration = 0; iteration < 64; ++iteration)
//     {
//         NetSenderClientHandle first = nullptr;
//         NetSenderClientHandle second = nullptr;
//         SENDER_CHECK(netSenderClientCreate(&callbacks, &first) == NI_OK && first != nullptr);
//         SENDER_CHECK(netSenderClientCreate(&callbacks, &second) == NI_OK && second != first);
//         int32_t value = 1;
//         generation = 19;
//         SENDER_CHECK(netSenderClientState(first, &generation, &value) == NI_OK && generation == 0 && value == 0);
//         SENDER_CHECK(netSenderClientConfigureCompression(first, nullptr, 0) == NI_OK);
//         SENDER_CHECK(netSenderClientConfigureCompression(first, nullptr, 1) < 0);
//         SENDER_CHECK(netSenderClientConnect(first, nullptr) == NI_INVALID_ARGUMENT);
//         SENDER_CHECK(netSenderClientConnectService(first, 1, 1, nullptr) == NI_INVALID_ARGUMENT);
//         requestId = 19;
//         SENDER_CHECK(netSenderClientBegin(first, 1, 1, 10, &requestId) < 0 && requestId == 0);
//         ticket = 19;
//         SENDER_CHECK(netSenderClientSendBegin(first, 1, nullptr, 0, &ticket) < 0 && ticket == 0);
//         SENDER_CHECK(netSenderClientSendResult(first, 1) < 0);
//         SENDER_CHECK(netSenderClientPostData(first, nullptr) == NI_INVALID_ARGUMENT);
//         SENDER_CHECK(netSenderClientSendData(first, nullptr) == NI_INVALID_ARGUMENT);
//         SENDER_CHECK(netSenderClientPoll(first) == NI_OK);
//         SENDER_CHECK(netSenderClientDestroy(first) == NI_OK);
//         SENDER_CHECK(netSenderClientState(second, &generation, &value) == NI_OK && generation == 0 && value == 0);
//         SENDER_CHECK(netSenderClientDestroy(second) == NI_OK);
//     }
//     SENDER_CHECK(events[0] == 0 && events[1] == 0 && events[2] == 0 && events[3] == 0);
//     std::printf("PASS singleton facade and C adapter checks=%u\n", checks);
//     #undef SENDER_CHECK
//     return 0;
// }
// NET_SENDER_FACADE_UNIT_END

NetSenderBackend& NetWorkNetSender::backend()
{
    return NetWork::backend();
}