#include "NetGatewayReceiveTest.h"
#include "NetGatewaySuiteCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "../../NetServerManager/NetServerManager/NetGatewayAuthority.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

/** 检查从真实TLS及公开回调观察到的结果
@param [in] value 独立预期
@param [in] message 失败诊断
*/
static void CheckGatewayReceive(bool value, const char* message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}

NetGatewayReceiveTest::NetGatewayReceiveTest() :
m_channel(nullptr),
m_open(false),
m_reject(false),
m_mode(0),
m_ready(0),
m_received(0),
m_callbacks(0),
m_closed(0),
m_reason(NI_OK)
{
    std::string request;
    CheckGatewayReceive(NetInterfaceCertificate::createRequest("service.101", NetInterfaceTlsTestPair::strongRandom,
        nullptr, m_key, request) == NI_OK, "receive test CSR");
    CheckGatewayReceive(NetGatewayAuthority::issueLocalIdentity("service.101", request, NetInterfaceTlsTestPair::strongRandom,
        nullptr, m_certificate) == NI_OK, "receive test service identity");
    NetInterfaceTlsOptions options = {};
    options.server = 1;
    options.handshakeTimeoutMs = 5000;
    options.entropy = NetInterfaceTlsTestPair::strongRandom;
    options.certificate = reinterpret_cast<const uint8_t*>(m_certificate.c_str());
    options.certificateSize = static_cast<uint32_t>(m_certificate.size() + 1);
    options.privateKey = reinterpret_cast<const uint8_t*>(m_key.c_str());
    options.privateKeySize = static_cast<uint32_t>(m_key.size() + 1);
    m_server.reset(new NetInterfaceConnection(options));
    CheckGatewayReceive(m_server->status() >= 0, "receive test TLS start");
}

NetGatewayReceiveTest::~NetGatewayReceiveTest()
{
    m_server.reset();
    if (!m_key.empty())
    {
        NetInterface::wipe(reinterpret_cast<uint8_t*>(&m_key[0]), static_cast<uint32_t>(m_key.size()));
    }
}

bool NetGatewayReceiveTest::poll()
{
    if (!NetGatewayChannelTestTransport::poll())
    {
        return false;
    }
    if (m_open)
    {
        m_open = false;
        ProtoMessage message;
        message.setCode(NI_OK);
        message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(static_cast<uint64_t>(1)));
        message.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(65536)));
        std::string payload;
        CheckGatewayReceive(message.toString(payload), "receive test OPEN encode");
        NetInterfaceFrame reply = {};
        reply.m_kind = NI_RESPONSE;
        reply.m_messageType = NI_GATEWAY_OPEN;
        reply.m_requestId = 1;
        reply.m_payload = reinterpret_cast<const uint8_t*>(payload.data());
        reply.m_payloadSize = static_cast<uint32_t>(payload.size());
        CheckGatewayReceive(m_listener->onLinkControl(7, reply), "receive test OPEN response");
    }
    CheckGatewayReceive(m_server->progress(NetSenderSubmission::now()) >= 0, "receive test TLS progress");
    const uint8_t* data = nullptr;
    uint32_t size = 0;
    CheckGatewayReceive(m_server->peekOutput(data, size) >= 0, "receive test TLS output");
    if (size != 0)
    {
        NetInterfaceTunnelRecord record = {};
        record.m_kind = NI_TUNNEL_BYTES;
        record.m_product = 101;
        record.m_channel = 1;
        record.m_data = data;
        record.m_size = (std::min)(size, 16384U);
        if (m_listener->onLinkRecord(7, record))
        {
            CheckGatewayReceive(m_server->consumeOutput(record.m_size) == NI_OK, "receive test output consume");
        }
    }
    return true;
}

int32_t NetGatewayReceiveTest::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != 7)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (frame.m_messageType == NI_GATEWAY_OPEN)
    {
        m_open = true;
    }
    return NI_OK;
}

int32_t NetGatewayReceiveTest::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    CheckGatewayReceive(peer == 7 && record.m_channel == 1 && record.m_kind == NI_TUNNEL_BYTES, "receive test ciphertext route");
    uint32_t consumed = 0;
    CheckGatewayReceive(m_server->feed(record.m_data, record.m_size, consumed) >= 0 && consumed == record.m_size,
        "receive test server input");
    return NI_OK;
}

void NetGatewayReceiveTest::onReady(uint64_t peer)
{
    CheckGatewayReceive(peer == 1, "receive test ready generation");
    ++m_ready;
}

bool NetGatewayReceiveTest::onFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    ++m_callbacks;
    CheckGatewayReceive(peer == 1 && frame.m_productId == 101 && frame.m_requestId == m_received + 1 &&
        frame.m_payloadSize == 6 && std::memcmp(frame.m_payload, "123456", 6) == 0, "receive test frame order/content");
    if (m_reject)
    {
        return false;
    }
    ++m_received;
    if (m_mode == 2)
    {
        m_channel->close(1, NI_CANCELLED);
    }
    if (m_mode == 3)
    {
        throw std::runtime_error("injected receive callback");
    }
    return true;
}

void NetGatewayReceiveTest::onClosed(uint64_t peer, int32_t reason, int32_t detail)
{
    (void)detail;
    CheckGatewayReceive(peer == 1, "receive test closed generation");
    ++m_closed;
    m_reason = reason;
}

void NetGatewayReceiveTest::run()
{
    for (uint32_t mode = 0; mode < 4; ++mode)
    {
        NetGatewayReceiveTest test;
        NetGatewayClientChannel channel(101, &test);
        test.m_channel = &channel;
        test.m_mode = mode;
        NetInterfaceTlsOptions options = {};
        options.entropy = NetInterfaceTlsTestPair::strongRandom;
        options.handshakeTimeoutMs = 5000;
        uint64_t peer = 0;
        CheckGatewayReceive(channel.connect("127.0.0.1", 1, options, &test, peer) == NI_OK, "receive test connect");
        for (uint32_t step = 0; test.m_ready == 0 && step < 1000; ++step)
        {
            netTestCheckpoint();
            CheckGatewayReceive(channel.poll(), "receive test handshake prematurely closed");
        }
        CheckGatewayReceive(test.m_ready == 1 && test.m_server->status() == NET_INTERFACE_TLS_READY, "receive test handshake timeout");
        for (uint32_t index = 1; index <= 49; ++index)
        {
            NetInterfaceFrame frame = {};
            frame.m_kind = NI_DATA;
            frame.m_productId = 101;
            frame.m_messageType = 77;
            frame.m_requestId = index;
            frame.m_payload = reinterpret_cast<const uint8_t*>("123456");
            frame.m_payloadSize = 6;
            CheckGatewayReceive(test.m_server->send(frame) == NI_OK, "receive test burst admission");
        }
        if (mode == 1)
        {
            test.m_reject = true;
            for (uint32_t round = 0; round < 3; ++round)
            {
                CheckGatewayReceive(channel.poll() && test.m_received == 0 && test.m_callbacks == round + 1,
                    "receive backpressure consumed or looped on frame");
            }
            test.m_reject = false;
        }
        channel.poll();
        if (mode < 2)
        {
            CheckGatewayReceive(test.m_received == 32 && test.m_closed == 0, "receive did not drain exactly 32 ready frames");
            CheckGatewayReceive(channel.poll() && test.m_received == 49, "receive did not drain burst suffix");
            CheckGatewayReceive(channel.poll() && test.m_received == 49, "receive duplicated burst");
        }
        else
        {
            CheckGatewayReceive(test.m_received == 1 && test.m_closed == 1 && !channel.poll(),
                "receive delivered after callback close/exception");
            CheckGatewayReceive(test.m_reason == (mode == 2 ? NI_CANCELLED : NI_NO_MEMORY), "receive lost first close reason");
        }
        channel.shutdown();
        CheckGatewayReceive(test.m_closed == 1, "receive duplicate close");
    }
}