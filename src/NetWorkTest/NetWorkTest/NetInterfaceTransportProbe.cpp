#include "NetInterfaceTransportProbe.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetWorkTestCases.h"
#include "NetInterfaceTcpProbe.h"
#include <stdexcept>

NetInterfaceTransportProbe::NetInterfaceTransportProbe() :
m_transport(nullptr), m_echo(false), m_hold(false), m_throw(false),
m_stopOnFrame(false), m_attempts(0)
{

}

void NetInterfaceTransportProbe::onReady(uint64_t peer)
{
    if (!m_ready.insert(peer).second)
    {
        throw std::runtime_error("duplicate TLS ready");
    }
}

bool NetInterfaceTransportProbe::onFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    ++m_attempts;
    if (m_throw)
    {
        throw std::runtime_error("controlled application failure");
    }
    if (m_hold)
    {
        return false;
    }
    if (m_stopOnFrame)
    {
        m_transport->shutdown();
        return true;
    }
    if (m_echo)
    {
        const int32_t result = m_transport->send(peer, frame);
        if (result == NI_LIMIT_EXCEEDED)
        {
            return false;
        }
        NetInterfaceTlsTestPair::require(result == NI_OK, "echo send failed");
    }
    NetInterfaceTransportMessage message;
    message.m_frame = frame;
    message.m_frame.m_payload = nullptr;
    if (frame.m_payloadSize != 0)
    {
        message.m_payload.assign(frame.m_payload, frame.m_payload + frame.m_payloadSize);
    }
    m_received.push_back(message);
    return true;
}

void NetInterfaceTransportProbe::onClosed(uint64_t peer, int32_t reason, int32_t detail)
{
    (void)detail;
    ++m_closed[peer];
    m_reasons[peer] = reason;
}

/** 取得真实证书配置，使用产品内的原生系统随机函数
@param [in] server 是否服务端
@param [in] hostname 客户端验证域名
@return 仅测试证书，entropy为空由NetWork填充
*/
static NetInterfaceTlsOptions TransportOptions(bool server, const char* hostname = "localhost")
{
    NetInterfaceTlsOptions options = NetInterfaceTlsTestPair::options(server, hostname);
    options.entropy = nullptr;
    options.entropyContext = nullptr;
    return options;
}

/** 驱动两端直到真实TLS都就绪，统一检查取消和单项截止时间
@param [in,out] server 服务端
@param [in,out] client 客户端
@param [in] serverEvents 服务端真实事件
@param [in] clientEvents 客户端真实事件
*/
static void WaitTransport(NetTransport& server, NetTransport& client,
    const NetInterfaceTransportProbe& serverEvents, const NetInterfaceTransportProbe& clientEvents)
{
    while (serverEvents.m_ready.empty() || clientEvents.m_ready.empty())
    {
        netTestCheckpoint();
        server.poll();
        client.poll();
        NetInterfaceTlsTestPair::require(clientEvents.m_closed.empty(),
            "transport closed during handshake");
    }
}

void NetInterfaceTransportTestCases::nativeTransport()
{
    NetInterfaceTransportProbe serverEvents;
    NetInterfaceTransportProbe clientEvents;
    NetTransport server;
    NetTransport client;
    serverEvents.m_transport = &server;
    serverEvents.m_echo = true;
    serverEvents.m_hold = true;
    uint64_t id = 0;
    NetInterfaceTlsTestPair::require(server.listen("127.0.0.1", 0, TransportOptions(true), &serverEvents) == 0,
        "listen composed transport");
    NetInterfaceTlsTestPair::require(client.connect("127.0.0.1", server.localPort(), TransportOptions(false),
        &clientEvents, id) == 0 && id != 0, "connect composed transport");
    WaitTransport(server, client, serverEvents, clientEvents);
    std::vector<uint8_t> body(1048576);
    for (size_t index = 0; index < body.size(); ++index)
    {
        body[index] = static_cast<uint8_t>(index * 31 + 13);
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_REQUEST;
    frame.m_productId = 7;
    frame.m_requestId = UINT64_C(0xfedcba9876543210);
    frame.m_messageType = 42;
    frame.m_payloadSize = static_cast<uint32_t>(body.size());
    frame.m_payload = &body[0];
    NetInterfaceTlsTestPair::require(client.send(id, frame) == NI_OK, "1MiB send admission");
    ++frame.m_requestId;
    frame.m_payloadSize = 3;
    NetInterfaceTlsTestPair::require(client.send(id, frame) == NI_OK, "following frame admission");
    while (serverEvents.m_attempts == 0)
    {
        netTestCheckpoint();
        server.poll();
        client.poll();
    }
    NetInterfaceTlsTestPair::require(serverEvents.m_received.empty() && clientEvents.m_received.empty(),
        "application backpressure ignored");
    serverEvents.m_hold = false;
    while (clientEvents.m_received.size() < 2)
    {
        netTestCheckpoint();
        server.poll();
        client.poll();
    }
    const NetInterfaceTransportMessage& first = clientEvents.m_received[0];
    const NetInterfaceTransportMessage& second = clientEvents.m_received[1];
    NetInterfaceTlsTestPair::require(first.m_payload == body && first.m_frame.m_requestId ==
        UINT64_C(0xfedcba9876543210) && first.m_frame.m_productId == 7 && first.m_frame.m_messageType == 42,
        "native composed binary or identity mismatch");
    NetInterfaceTlsTestPair::require(second.m_payload == std::vector<uint8_t>(body.begin(), body.begin() + 3) &&
        second.m_frame.m_requestId == UINT64_C(0xfedcba9876543211), "tail frame lost");
    client.shutdown();
    server.shutdown();
    NetInterfaceTlsTestPair::require(clientEvents.m_closed[id] == 1 && client.peerCount() == 0 &&
        server.peerCount() == 0 && !client.poll() && !server.poll(), "native shutdown resources");
}

void NetInterfaceTransportTestCases::transportFailures()
{
    NetInterfaceTransportProbe serverEvents;
    NetInterfaceTransportProbe wrongEvents;
    NetTransport server;
    NetTransport wrong;
    uint64_t wrongId = 0;
    NetInterfaceTlsTestPair::require(server.listen("127.0.0.1", 0, TransportOptions(true), &serverEvents) == 0 &&
        wrong.connect("127.0.0.1", server.localPort(), TransportOptions(false, "wrong.invalid"),
        &wrongEvents, wrongId) == 0, "setup wrong hostname");
    while (wrongEvents.m_closed.empty())
    {
        netTestCheckpoint();
        server.poll();
        wrong.poll();
    }
    NetInterfaceTlsTestPair::require(wrongEvents.m_ready.empty() &&
        wrongEvents.m_reasons[wrongId] == NET_INTERFACE_TLS_PROTOCOL, "wrong hostname accepted");
    NetInterfaceTcpProbe silentEvents;
    LibuvChannel silent;
    NetInterfaceTransportProbe timeoutEvents;
    NetTransport timeout;
    uint64_t timeoutId = 0;
    NetInterfaceTlsOptions shortDeadline = TransportOptions(false);
    shortDeadline.handshakeTimeoutMs = 100;
    NetInterfaceTlsTestPair::require(silent.listen("127.0.0.1", 0, &silentEvents) == 0 &&
        timeout.connect("127.0.0.1", silent.localPort(), shortDeadline, &timeoutEvents, timeoutId) == 0,
        "setup silent TLS peer");
    while (timeoutEvents.m_closed.empty())
    {
        netTestCheckpoint();
        silent.poll();
        timeout.poll();
    }
    NetInterfaceTlsTestPair::require(timeoutEvents.m_ready.empty() &&
        timeoutEvents.m_reasons[timeoutId] == NET_INTERFACE_TLS_TIMEOUT, "native handshake deadline missing");
    NetInterfaceTransportProbe liveEvents;
    NetTransport live;
    uint64_t liveId = 0;
    NetInterfaceTlsTestPair::require(live.connect("127.0.0.1", server.localPort(), TransportOptions(false),
        &liveEvents, liveId) == 0, "setup unclean close");
    WaitTransport(server, live, serverEvents, liveEvents);
    server.shutdown();
    while (liveEvents.m_closed.empty())
    {
        netTestCheckpoint();
        live.poll();
    }
    NetInterfaceTlsTestPair::require(liveEvents.m_reasons[liveId] == NET_INTERFACE_TLS_TRUNCATED &&
        liveEvents.m_closed[liveId] == 1, "bare TCP EOF accepted or close duplicated");
}

void NetInterfaceTransportTestCases::transportLifetime()
{
    NetInterfaceTransportProbe serverEvents;
    NetInterfaceTransportProbe firstEvents;
    NetInterfaceTransportProbe secondEvents;
    NetTransport server;
    NetTransport first;
    NetTransport second;
    serverEvents.m_transport = &server;
    serverEvents.m_throw = true;
    uint64_t firstId = 0;
    NetInterfaceTlsTestPair::require(server.listen("127.0.0.1", 0, TransportOptions(true), &serverEvents) == 0 &&
        first.connect("127.0.0.1", server.localPort(), TransportOptions(false), &firstEvents, firstId) == 0,
        "setup lifecycle");
    WaitTransport(server, first, serverEvents, firstEvents);
    const uint64_t oldId = *serverEvents.m_ready.begin();
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_REQUEST;
    frame.m_requestId = 10;
    NetInterfaceTlsTestPair::require(first.send(firstId, frame) == NI_OK, "trigger callback failure");
    while (serverEvents.m_closed.empty())
    {
        netTestCheckpoint();
        server.poll();
        first.poll();
    }
    NetInterfaceTlsTestPair::require(serverEvents.m_reasons[oldId] == NI_INVALID_STATE &&
        serverEvents.m_closed[oldId] == 1, "callback failure terminal result");
    serverEvents.m_throw = false;
    serverEvents.m_stopOnFrame = true;
    uint64_t secondId = 0;
    NetInterfaceTlsTestPair::require(second.connect("127.0.0.1", server.localPort(), TransportOptions(false),
        &secondEvents, secondId) == 0, "replacement connect");
    while (serverEvents.m_ready.size() < 2 || secondEvents.m_ready.empty())
    {
        netTestCheckpoint();
        server.poll();
        second.poll();
    }
    NetInterfaceTlsTestPair::require(server.send(oldId, frame) == NI_INVALID_STATE &&
        second.send(secondId, frame) == NI_OK, "stale transport ID reused");
    while (serverEvents.m_closed.size() < 2)
    {
        netTestCheckpoint();
        server.poll();
        second.poll();
    }
    NetInterfaceTlsTestPair::require(!server.poll() && server.peerCount() == 0 &&
        serverEvents.m_attempts == 2, "callback shutdown failed to converge");
}