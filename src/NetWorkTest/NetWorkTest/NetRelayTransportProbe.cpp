#include "NetRelayTransportProbe.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetWorkTestCases.h"
#include "CSystem/CSystemAPI.h"
#include <algorithm>
#include <chrono>
#include <cstring>

NetRelayTransportProbe::NetRelayTransportProbe() : m_partner(nullptr), m_inputOffset(0), m_inputBytes(0), m_peer(0),
    m_forwarded(0), m_closedCount(0)
{

}

NetRelayTransportProbe::~NetRelayTransportProbe()
{
    m_transport.shutdown();
}

void NetRelayTransportProbe::onRelayConnected(uint64_t peer)
{
    m_peer = peer;
}

bool NetRelayTransportProbe::onRelayRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (peer != m_peer || record.m_kind != NI_TUNNEL_BYTES || record.m_product != 7 || record.m_channel != UINT64_C(0x1234567887654321))
    {
        m_error = "unexpected route ownership or outer record";
        m_transport.close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    if (m_partner != nullptr)
    {
        if (m_partner->m_peer == 0)
        {
            return false;
        }
        const int32_t result = m_partner->m_transport.send(m_partner->m_peer, record);
        if (result == NI_LIMIT_EXCEEDED)
        {
            return false;
        }
        if (result != NI_OK)
        {
            m_error = "opaque relay send failed";
            m_transport.close(peer, result);
        }
        else
        {
            m_forwarded += record.m_size;
        }
        return true;
    }
    if (m_inputBytes + record.m_size > 65536)
    {
        return false;
    }
    m_input.push_back(std::vector<uint8_t>(record.m_data, record.m_data + record.m_size));
    m_inputBytes += record.m_size;
    return true;
}

void NetRelayTransportProbe::onRelayClosed(uint64_t peer, int32_t reason)
{
    if (peer == m_peer)
    {
        ++m_closedCount;
    }
    if (reason != 0)
    {
        m_error = "raw transport reported abnormal close";
    }
}

void NetRelayTransportProbe::progress()
{
    if (!m_tls)
    {
        return;
    }
    const uint64_t now = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
    for (uint32_t step = 0; step < 64; ++step)
    {
        if (!m_input.empty())
        {
            std::vector<uint8_t>& front = m_input.front();
            uint32_t consumed = 0;
            const int32_t result = m_tls->feed(front.data() + m_inputOffset,
                static_cast<uint32_t>(front.size()) - m_inputOffset, consumed);
            if (result < 0)
            {
                m_error = "endpoint rejected TLS input";
                return;
            }
            m_inputOffset += consumed;
            m_inputBytes -= consumed;
            if (m_inputOffset == front.size())
            {
                m_input.pop_front();
                m_inputOffset = 0;
            }
        }
        if (m_tls->progress(now) < 0)
        {
            m_error = "endpoint TLS progress failed";
            return;
        }
        const uint8_t* output = nullptr;
        uint32_t size = 0;
        m_tls->peekOutput(output, size);
        if (size == 0 || m_peer == 0)
        {
            break;
        }
        NetInterfaceTunnelRecord record = {};
        record.m_kind = NI_TUNNEL_BYTES;
        record.m_product = 7;
        record.m_channel = UINT64_C(0x1234567887654321);
        record.m_data = output;
        record.m_size = (std::min)(size, NET_INTERFACE_TUNNEL_PAYLOAD_LIMIT);
        const int32_t sent = m_transport.send(m_peer, record);
        if (sent == NI_LIMIT_EXCEEDED)
        {
            break;
        }
        if (sent != NI_OK)
        {
            m_error = "endpoint could not send TLS bytes";
            return;
        }
        m_tls->consumeOutput(record.m_size);
    }
}

/** 两段真实TCP中间只透传外层记录，TLS会话只存在于两个业务端点
*/
void netRelayTlsTcpTest()
{
    NetRelayTransportProbe client;
    NetRelayTransportProbe front;
    NetRelayTransportProbe back;
    NetRelayTransportProbe server;
    front.m_partner = &back;
    back.m_partner = &front;
    client.m_tls.reset(new NetInterfaceTls(NetInterfaceTlsTestPair::options(false, "localhost")));
    server.m_tls.reset(new NetInterfaceTls(NetInterfaceTlsTestPair::options(true, "localhost")));
    NetInterfaceTlsTestPair::require(server.m_transport.listen("127.0.0.1", 0, &server) == NI_OK &&
        front.m_transport.listen("127.0.0.1", 0, &front) == NI_OK, "listen isolated relay TCP failed");
    uint64_t backId = 0;
    uint64_t clientId = 0;
    NetInterfaceTlsTestPair::require(back.m_transport.connect("127.0.0.1", server.m_transport.localPort(), &back, backId) == NI_OK &&
        client.m_transport.connect("127.0.0.1", front.m_transport.localPort(), &client, clientId) == NI_OK,
        "connect isolated relay TCP failed");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    std::vector<uint8_t> source(65537);
    for (uint32_t index = 0; index < source.size(); ++index)
    {
        source[index] = static_cast<uint8_t>(index * 31);
    }
    uint32_t sent = 0;
    uint32_t echoed = 0;
    std::vector<uint8_t> atServer;
    std::vector<uint8_t> atClient;
    while (atClient.size() != source.size())
    {
        netTestCheckpoint();
        NetInterfaceTlsTestPair::require(std::chrono::steady_clock::now() < deadline, "opaque relay TCP timed out");
        client.m_transport.poll();
        front.m_transport.poll();
        back.m_transport.poll();
        server.m_transport.poll();
        client.progress();
        server.progress();
        NetInterfaceTlsTestPair::require(client.m_error.empty() && front.m_error.empty() && back.m_error.empty() &&
            server.m_error.empty(), "TCP relay or endpoint failed");
        if (client.m_tls->status() == NET_INTERFACE_TLS_READY && sent < source.size())
        {
            uint32_t used = 0;
            NetInterfaceTlsTestPair::require(client.m_tls->write(source.data() + sent,
                static_cast<uint32_t>(source.size()) - sent, used) >= 0, "client TLS write failed");
            sent += used;
        }
        uint8_t plain[4096] = {};
        uint32_t count = 0;
        if (server.m_tls->status() == NET_INTERFACE_TLS_READY)
        {
            NetInterfaceTlsTestPair::require(server.m_tls->read(plain, sizeof(plain), count) >= 0, "server TLS read failed");
            atServer.insert(atServer.end(), plain, plain + count);
            if (echoed < atServer.size())
            {
                uint32_t used = 0;
                NetInterfaceTlsTestPair::require(server.m_tls->write(atServer.data() + echoed,
                    static_cast<uint32_t>(atServer.size()) - echoed, used) >= 0, "server TLS reply failed");
                echoed += used;
            }
        }
        count = 0;
        if (client.m_tls->status() == NET_INTERFACE_TLS_READY)
        {
            NetInterfaceTlsTestPair::require(client.m_tls->read(plain, sizeof(plain), count) >= 0, "client TLS reply read failed");
            atClient.insert(atClient.end(), plain, plain + count);
        }
        CSystem::Sleep(1);
    }
    NetInterfaceTlsTestPair::require(atClient == source && atServer == source && front.m_forwarded > source.size() &&
        back.m_forwarded > source.size() && !front.m_tls && !back.m_tls, "opaque relay TLS roundtrip mismatch");
    client.m_transport.shutdown();
    front.m_transport.shutdown();
    back.m_transport.shutdown();
    server.m_transport.shutdown();
    NetInterfaceTlsTestPair::require(client.m_closedCount == 1 && front.m_closedCount == 1 &&
        back.m_closedCount == 1 && server.m_closedCount == 1, "raw transport closure not exactly once");
}

void netRelayQueueTcpTest()
{
    NetRelayTransportProbe client;
    NetRelayTransportProbe server;
    NetInterfaceTlsTestPair::require(server.m_transport.listen("127.0.0.1", 0, &server) == NI_OK, "relay queue listen failed");
    uint64_t id = 0;
    NetInterfaceTlsTestPair::require(client.m_transport.connect("127.0.0.1", server.m_transport.localPort(), &client, id) == NI_OK,
        "relay queue connect failed");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    while (client.m_peer == 0 || server.m_peer == 0)
    {
        netTestCheckpoint();
        NetInterfaceTlsTestPair::require(std::chrono::steady_clock::now() < deadline, "relay queue connect timed out");
        client.m_transport.poll();
        server.m_transport.poll();
        CSystem::Sleep(1);
    }
    uint8_t byte = 0;
    NetInterfaceTunnelRecord record = {};
    record.m_kind = NI_TUNNEL_BYTES;
    record.m_product = 7;
    record.m_channel = UINT64_C(0x1234567887654321);
    record.m_data = &byte;
    record.m_size = 1;
    for (uint32_t round = 0; round < 17; ++round)
    {
        for (uint32_t index = 0; index < 32; ++index)
        {
            byte = static_cast<uint8_t>(round * 32 + index);
            NetInterfaceTlsTestPair::require(client.m_transport.send(id, record) == NI_OK, "relay queue rejected valid record");
        }
        byte = 0xff;
        NetInterfaceTlsTestPair::require(client.m_transport.send(id, record) == NI_LIMIT_EXCEEDED,
            "relay queue exceeded record capacity");
        NetInterfaceTlsTestPair::require(client.m_transport.outputState(id) == NI_READY, "relay queue did not retain accepted records");
        while (server.m_input.size() != 32)
        {
            netTestCheckpoint();
            NetInterfaceTlsTestPair::require(std::chrono::steady_clock::now() < deadline, "relay queue delivery timed out");
            client.m_transport.poll();
            server.m_transport.poll();
            NetInterfaceTlsTestPair::require(server.m_input.size() <= 32 && client.m_error.empty() && server.m_error.empty(),
                "relay queue delivered rejected record or transport failed");
            CSystem::Sleep(1);
        }
        for (uint32_t index = 0; index < 32; ++index)
        {
            const std::vector<uint8_t>& received = server.m_input.front();
            NetInterfaceTlsTestPair::require(received.size() == 1 && received[0] == static_cast<uint8_t>(round * 32 + index),
                "relay queue reordered or retained borrowed input");
            server.m_input.pop_front();
        }
        server.m_inputBytes = 0;
        NetInterfaceTlsTestPair::require(client.m_transport.outputState(id) == NI_OK, "relay queue did not release accepted output");
    }
    client.m_transport.shutdown();
    server.m_transport.shutdown();
    NetInterfaceTlsTestPair::require(client.m_closedCount == 1 && server.m_closedCount == 1, "relay queue close not exactly once");
}