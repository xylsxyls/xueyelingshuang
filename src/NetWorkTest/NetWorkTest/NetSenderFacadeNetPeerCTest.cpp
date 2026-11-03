#define NET_SENDER_USE_C
#include "NetPeer/NetSenderAPI.h"
#include "NetSenderFacadeNetPeerCTest.h"
#include "NetWorkTestCases.h"
#include <cstdio>
#include <stdexcept>

// 若业务入口重新泄漏核心主类或内部宿主，此编译期探针会发生重定义错误
typedef int NetPeer;
typedef int NetWork;
typedef int NetHostSocket;
typedef int NetPeerClientBackend;
typedef int NetWorkHostBridge;
typedef int NetFileIo;


/** 记录测试就绪回调
@param [in] context 四项计数数组
@param [in] generation 本用例不建立真实连接
@param [in] requests 未使用
@param [in] chunk 未使用
*/
static void SenderUnitReady(void* context, uint64_t generation, uint32_t requests, uint32_t chunk)
{
    (void)generation; (void)requests; (void)chunk;
    ++static_cast<uint32_t*>(context)[0];
}

/** 记录测试结果回调
@param [in] context 四项计数数组
@param [in] generation 未使用
@param [in] requestId 未使用
@param [in] product 未使用
@param [in] type 未使用
@param [in] status 未使用
@param [in] bytes 未使用
@param [in] size 未使用
*/
static void SenderUnitResult(void* context, uint64_t generation, uint64_t requestId, uint32_t product,
    uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    (void)generation; (void)requestId; (void)product; (void)type; (void)status; (void)bytes; (void)size;
    ++static_cast<uint32_t*>(context)[1];
}

/** 记录帧回调且声明已消费
@param [in] context 四项计数数组
@param [in] generation 未使用
@param [in] frame 未使用
@return 1已消费
*/
static int32_t SenderUnitFrame(void* context, uint64_t generation, const NetInterfaceFrame* frame)
{
    (void)generation; (void)frame;
    ++static_cast<uint32_t*>(context)[2];
    return 1;
}

/** 记录关闭回调
@param [in] context 四项计数数组
@param [in] generation 未使用
@param [in] reason 未使用
@param [in] detail 未使用
*/
static void SenderUnitClosed(void* context, uint64_t generation, int32_t reason, int32_t detail)
{
    (void)generation; (void)reason; (void)detail;
    ++static_cast<uint32_t*>(context)[3];
}

void NetSenderFacadeNetPeerCTest::peerC()
{
    uint32_t checks = 0;
    #define SENDER_CHECK(value) do { netTestCheckpoint(); ++checks; if (!(value)) { std::fprintf(stderr, "facade peerC line=%d checks=%u\n", __LINE__, checks); throw std::runtime_error("facade peerC contract"); } } while (0)
    uint64_t generation = 0;
    uint64_t requestId = 0;
    uint64_t ticket = 0;
    uint32_t events[4] = {};
    NetInterfaceClientCallbacks callbacks = {};
    callbacks.structSize = sizeof(callbacks);
    callbacks.version = 1;
    callbacks.context = events;
    callbacks.ready = SenderUnitReady;
    callbacks.result = SenderUnitResult;
    callbacks.frame = SenderUnitFrame;
    callbacks.closed = SenderUnitClosed;
    NetSenderClientHandle invalid = nullptr;
    SENDER_CHECK(netSenderClientCreate(nullptr, &invalid) == NI_INVALID_ARGUMENT && invalid == nullptr);
    SENDER_CHECK(netSenderClientCreate(&callbacks, nullptr) == NI_INVALID_ARGUMENT);
    callbacks.version = 0;
    SENDER_CHECK(netSenderClientCreate(&callbacks, &invalid) == NI_INVALID_ARGUMENT && invalid == nullptr);
    callbacks.version = 1;
    SENDER_CHECK(netSenderClientDestroy(nullptr) == NI_OK);
    SENDER_CHECK(netSenderClientPoll(nullptr) == NI_INVALID_ARGUMENT);
    requestId = 19;
    SENDER_CHECK(netSenderClientBegin(nullptr, 1, 1, 10, &requestId) == NI_INVALID_ARGUMENT && requestId == 0);
    SENDER_CHECK(netSenderClientConfigureGateways(nullptr, nullptr, nullptr) == NI_INVALID_ARGUMENT);
    for (uint32_t iteration = 0; iteration < 64; ++iteration)
    {
        NetSenderClientHandle first = nullptr;
        NetSenderClientHandle second = nullptr;
        SENDER_CHECK(netSenderClientCreate(&callbacks, &first) == NI_OK && first != nullptr);
        SENDER_CHECK(netSenderClientCreate(&callbacks, &second) == NI_OK && second != first);
        int32_t value = 1;
        generation = 19;
        SENDER_CHECK(netSenderClientState(first, &generation, &value) == NI_OK && generation == 0 && value == 0);
        SENDER_CHECK(netSenderClientConfigureCompression(first, nullptr, 0) == NI_OK);
        SENDER_CHECK(netSenderClientConfigureCompression(first, nullptr, 1) < 0);
        SENDER_CHECK(netSenderClientConfigureGateways(first, "invalid", nullptr) == NI_INVALID_ARGUMENT);
        SENDER_CHECK(netSenderClientConfigureGateways(first, "127.0.0.1:1", nullptr) == NI_OK);
        char oversized[4098] = {};
        for (uint32_t index = 0; index < 4097; ++index)
        {
            oversized[index] = 'x';
        }
        SENDER_CHECK(netSenderClientConfigureGateways(first, oversized, nullptr) == NI_INVALID_ARGUMENT);
        SENDER_CHECK(netSenderClientConfigureGateways(first, nullptr, oversized) == NI_INVALID_ARGUMENT);
        SENDER_CHECK(netSenderClientConfigureGateways(first, nullptr, nullptr) == NI_OK);
        SENDER_CHECK(netSenderClientConnect(first, nullptr) == NI_INVALID_ARGUMENT);
        SENDER_CHECK(netSenderClientConnectService(first, 1, 1, nullptr) == NI_INVALID_ARGUMENT);
        requestId = 19;
        SENDER_CHECK(netSenderClientBegin(first, 1, 1, 10, &requestId) < 0 && requestId == 0);
        ticket = 19;
        SENDER_CHECK(netSenderClientSendBegin(first, 1, nullptr, 0, &ticket) < 0 && ticket == 0);
        SENDER_CHECK(netSenderClientSendResult(first, 1) < 0);
        SENDER_CHECK(netSenderClientPostData(first, nullptr) == NI_INVALID_ARGUMENT);
        SENDER_CHECK(netSenderClientSendData(first, nullptr) == NI_INVALID_ARGUMENT);
        SENDER_CHECK(netSenderClientPoll(first) == NI_OK);
        SENDER_CHECK(netSenderClientDestroy(first) == NI_OK);
        SENDER_CHECK(netSenderClientState(second, &generation, &value) == NI_OK && generation == 0 && value == 0);
        SENDER_CHECK(netSenderClientDestroy(second) == NI_OK);
    }
    SENDER_CHECK(events[0] == 0 && events[1] == 0 && events[2] == 0 && events[3] == 0);
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
    SENDER_CHECK(netPeerFileFlowCreate(nullptr, 101, 1, 10, 0, 1000, &flow) == NI_INVALID_ARGUMENT && flow == nullptr);
    SENDER_CHECK(netPeerFileFlowCreate(&info, 101, 1, 10, 0, 1000, &flow) == NI_OK && flow != nullptr);
    uint8_t wire[NET_INTERFACE_FILE_INFO_SIZE] = {};
    int32_t result = NI_INVALID_STATE;
    SENDER_CHECK(netPeerFileFlowSnapshotWire(flow, wire, sizeof(wire), &result) == NI_OK);
    NetInterfaceFileInfo decoded = {};
    SENDER_CHECK(netPeerFileParseInfo(wire, sizeof(wire), &decoded) == NI_OK);
    SENDER_CHECK(decoded.stream == 9 && decoded.total == 1 && decoded.transfer[0] == 1 && decoded.token[0] == 2);
    SENDER_CHECK(netPeerFileFlowCancel(flow, NI_CANCELLED) == NI_OK);
    SENDER_CHECK(netPeerFileFlowSnapshot(flow, &decoded, &result) == NI_OK && result == NI_CANCELLED);
    netPeerFileFlowDestroy(flow);
    netPeerFileFlowDestroy(nullptr);
    #undef SENDER_CHECK
}