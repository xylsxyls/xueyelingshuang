#include "NetSenderTestClient.h"
#include "NetWork/NetWorkAPI.h"
#include "NetPeer/NetPeerAPI.h"
#include "NetWorkInProcessTestCases.h"
#include "NetGatewayTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetWorkTestCases.h"
#include "CSystem/CSystemAPI.h"
#include <cstring>
#include <stdexcept>
#include <vector>

/** 真实互通断言同时经过runner取消/期限检查
@param [in] condition 独立预期
@param [in] message 失败原因
*/
static void RequireInProcess(bool condition, const char* message)
{
    netTestCheckpoint();
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

NetWorkInProcessProbe::NetWorkInProcessProbe(NetSenderTestClient<NetWork, NetWorkSenderState>& sender) :
m_sender(&sender),
m_generation(0),
m_request(0),
m_results(0),
m_closed(0),
m_bad(false)
{

}

void NetWorkInProcessProbe::onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk)
{
    m_bad = m_bad || m_generation != 0 || generation != 1 || maxRequests != 4 || maxChunk != 4096;
    m_generation = generation;
    m_bad = m_bad || m_sender->releaseGateway() != NI_INVALID_STATE;
    m_bad = m_bad || m_sender->prepareGateway() != NI_INVALID_STATE;
    m_bad = m_bad || m_sender->lastGatewayError() != NI_INVALID_STATE || m_sender->gatewayGeneration() != generation;
}

void NetWorkInProcessProbe::onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product,
    uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size)
{
    ++m_results;
    m_bad = m_bad || generation != m_generation || requestId != m_request || product != 4 || type != 700 ||
        status != NI_RPC_SUCCESS || size != 65537 || bytes == nullptr;
    for (uint32_t index = 0; !m_bad && index < size; ++index)
    {
        m_bad = bytes[index] != static_cast<uint8_t>(index * 31U + 17U);
    }
}

bool NetWorkInProcessProbe::onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame)
{
    (void)generation;
    (void)frame;
    m_bad = true;
    return true;
}

void NetWorkInProcessProbe::onSessionClosed(uint64_t generation, int32_t reason, int32_t detail)
{
    (void)detail;
    m_bad = m_bad || generation != m_generation || reason != NI_OK;
    ++m_closed;
}

NetWorkInProcessSenderOwner::NetWorkInProcessSenderOwner(NetSenderTestClient<NetWork, NetWorkSenderState>& sender) :
m_sender(&sender)
{
    if (m_sender->prepareGateway() != NI_OK)
    {
        throw std::runtime_error("in-process C++ sender preparation");
    }
}

NetWorkInProcessSenderOwner::~NetWorkInProcessSenderOwner()
{
    // 本测试的非GUI生命期owner保留观察者，直至独占协议任务实际退出。
    m_sender->uninitGateway();
    while (m_sender->pollGateway())
    {
        CSystem::sleepPrecise(1);
    }
    m_sender->releaseGateway();
}

void NetWorkInProcessTestCases::clientPlatforms()
{
    NetGatewayTestSink sink;
    NetGatewayServer gateway(&sink);
    NetInterfaceTlsOptions serverTls = NetInterfaceTlsTestPair::options(true, "localhost");
    serverTls.entropy = nullptr;
    serverTls.entropyContext = nullptr;
    RequireInProcess(gateway.listen("127.0.0.1", 0, serverTls) == NI_OK, "in-process TLS endpoint listen");
    uint64_t service = 0;
    RequireInProcess(gateway.addService(4, false, 4, service) == NI_OK, "in-process test service registration");
    for (uint32_t platform = 1; platform <= 3; ++platform)
    {
        NetSenderTestClient<NetWork, NetWorkSenderState>& sender = NetSenderTestClient<NetWork, NetWorkSenderState>::instance();
        NetWorkInProcessProbe events(sender);
        NetWorkInProcessSenderOwner owner(sender);
        NetSenderGatewayOptions options = {};
        options.m_network.structSize = sizeof(options.m_network);
        options.m_network.version = 1;
        options.m_network.address = "127.0.0.1";
        options.m_network.port = gateway.localPort();
        options.m_network.platform = platform;
        options.m_network.capabilities = 7;
        options.m_network.maxRequests = 4;
        options.m_network.maxChunk = 4096;
        options.m_network.handshakeTimeoutMs = 5000;
        options.m_network.tls = NetInterfaceTlsTestPair::options(false, "localhost");
        options.m_network.tls.entropy = nullptr;
        options.m_network.tls.entropyContext = nullptr;
        std::memset(options.m_network.device, static_cast<int32_t>(0x40U + platform), 16);
        RequireInProcess(sender.connectGateway(options, &events) == NI_OK, "in-process C++ platform connection admission");
        while (events.m_generation == 0)
        {
            gateway.poll();
            RequireInProcess(sender.pollGateway() && !events.m_bad, "in-process C++ platform handshake");
            CSystem::sleepPrecise(1);
        }
        std::vector<uint8_t> body(65537);
        for (uint32_t index = 0; index < body.size(); ++index)
        {
            body[index] = static_cast<uint8_t>(index * 31U + 17U);
        }
        const size_t deliveryIndex = sink.m_deliveries.size();
        RequireInProcess(sender.beginRequest(4, 700, 5000, events.m_request) == NI_OK && events.m_request != 0 &&
            (platform == 1 ? sender.sendRequest(events.m_request, body.data(), static_cast<uint32_t>(body.size())) :
                sender.postRequest(events.m_request, body.data(), static_cast<uint32_t>(body.size()))) == NI_OK,
            "in-process C++ request admission");
        while (sink.m_deliveries.size() == deliveryIndex)
        {
            gateway.poll();
            RequireInProcess(sender.pollGateway() && !events.m_bad, "in-process C++ request delivery");
            CSystem::sleepPrecise(1);
        }
        const NetGatewayTestDelivery& delivery = sink.m_deliveries[deliveryIndex];
        NetInterfaceFrame frame = delivery.m_message.m_frame;
        frame.m_payload = delivery.m_message.m_payload.data();
        NetInterfaceRpcView rpc = {};
        RequireInProcess(NetInterfaceRpc::parse(frame, rpc) == NI_OK && rpc.m_parent == 0 && rpc.m_size == body.size() &&
            std::memcmp(rpc.m_data, body.data(), body.size()) == 0 && delivery.m_origin.m_platform == platform &&
            delivery.m_origin.m_device[0] == static_cast<uint8_t>(0x40U + platform), "in-process C++ trusted origin");
        std::vector<uint8_t> reply(body.size() + 16);
        uint32_t required = 0;
        RequireInProcess(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, rpc.m_data, rpc.m_size,
            reply.data(), static_cast<uint32_t>(reply.size()), required) == NI_OK, "in-process C++ fixture response");
        frame.m_kind = NI_RESPONSE;
        frame.m_payload = reply.data();
        frame.m_payloadSize = required;
        RequireInProcess(gateway.receiveService(service, frame) == NI_OK, "in-process C++ response admission");
        while (events.m_results == 0)
        {
            gateway.poll();
            RequireInProcess(sender.pollGateway() && !events.m_bad, "in-process C++ response bytes");
            CSystem::sleepPrecise(1);
        }
        sender.closeGateway();
        while (events.m_closed == 0)
        {
            gateway.poll();
            sender.pollGateway();
            RequireInProcess(!events.m_bad, "in-process C++ close");
        }
        RequireInProcess(events.m_results == 1 && events.m_closed == 1, "in-process C++ unique terminal");
    }
}

void NetWorkInProcessTestCases::mainFactories()
{
    {
        NetPeer& entry = NetPeer::instance();
        NetPeerSenderState* first = nullptr;
        NetPeerSenderState* second = nullptr;
        RequireInProcess(&entry == &NetPeer::instance(), "singleton identity");
        RequireInProcess(sizeof(NetPeer) == 1, "stateless core entry has no members");
        RequireInProcess(entry.destroyClient(first) == NI_OK, "null destroy");
        RequireInProcess(entry.createClient(first) == NI_OK && first != nullptr, "factory create");
        NetPeerSenderState* original = first;
        RequireInProcess(entry.createClient(first) == NI_INVALID_ARGUMENT && first == original, "nonempty output retained");
        RequireInProcess(entry.createClient(second) == NI_OK && second != first, "independent clients");
        RequireInProcess(entry.prepareGateway(first) == NI_OK, "first preparation");
        RequireInProcess(entry.gatewayGeneration(second) == 0 && !entry.gatewayReady(second), "independent state");
        RequireInProcess(entry.destroyClient(first) == NI_OK && first == nullptr, "prepared destroy");
        RequireInProcess(entry.destroyClient(second) == NI_OK && second == nullptr, "unprepared destroy");
        RequireInProcess(entry.destroyClient(second) == NI_OK, "repeat null destroy");
    }
    {
        NetWork& entry = NetWork::instance();
        NetWorkSenderState* first = nullptr;
        NetWorkSenderState* second = nullptr;
        RequireInProcess(&entry == &NetWork::instance(), "singleton identity");
        RequireInProcess(sizeof(NetWork) == 1, "stateless core entry has no members");
        RequireInProcess(entry.destroyClient(first) == NI_OK, "null destroy");
        RequireInProcess(entry.createClient(first) == NI_OK && first != nullptr, "factory create");
        NetWorkSenderState* original = first;
        RequireInProcess(entry.createClient(first) == NI_INVALID_ARGUMENT && first == original, "nonempty output retained");
        RequireInProcess(entry.createClient(second) == NI_OK && second != first, "independent clients");
        RequireInProcess(entry.prepareGateway(first) == NI_OK, "first preparation");
        RequireInProcess(entry.gatewayGeneration(second) == 0 && !entry.gatewayReady(second), "independent state");
        RequireInProcess(entry.destroyClient(first) == NI_OK && first == nullptr, "prepared destroy");
        RequireInProcess(entry.destroyClient(second) == NI_OK && second == nullptr, "unprepared destroy");
        RequireInProcess(entry.destroyClient(second) == NI_OK, "repeat null destroy");
    }
}