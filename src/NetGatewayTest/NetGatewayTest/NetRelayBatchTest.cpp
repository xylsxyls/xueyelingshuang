#include "NetRelayBatchTest.h"
#include "NetGatewaySuiteCases.h"
#include "NetPeer/NetHostSocketState.h"
#include <cstring>
#include <stdexcept>

/** 断言外部观察值，不使用被测队列的内部状态
@param [in] condition 必须成立的独立预期
@param [in] message 失败诊断
*/
static void CheckRelayBatch(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

NetRelayBatchTest::NetRelayBatchTest() :
m_previous(NetHostSocketState::m_default),
m_limit(32768),
m_budget(UINT32_MAX)
{
    m_syncConnect = true;
    NetHostFunctions host = functions();
    host.write = capture;
    NetHostSocketState::m_default = host;
}

NetRelayBatchTest::~NetRelayBatchTest()
{
    NetHostSocketState::m_default = m_previous;
}

int32_t NetRelayBatchTest::capture(void* instance, uint64_t peer, const uint8_t* data, uint32_t size)
{
    (void)peer;
    NetRelayBatchTest& test = *static_cast<NetRelayBatchTest*>(static_cast<NetHostSocketTestDriver*>(instance));
    test.m_attempts.push_back(size);
    if (test.m_writeResult != 0)
    {
        return test.m_writeResult;
    }
    if (size > test.m_limit || size > test.m_budget)
    {
        return 1;
    }
    test.m_wire.insert(test.m_wire.end(), data, data + size);
    test.m_budget -= size;
    return 0;
}

void NetRelayBatchTest::enqueue(NetRelayTransport& relay, uint64_t peer, uint32_t count, uint32_t size, bool mixed)
{
    std::vector<uint8_t> payload(size);
    std::vector<uint8_t> encoded(size + NET_INTERFACE_TUNNEL_HEADER_SIZE);
    for (uint32_t index = 0; index < count; ++index)
    {
        netTestCheckpoint();
        for (uint32_t offset = 0; offset < size; ++offset)
        {
            payload[offset] = static_cast<uint8_t>(index * 19 + offset);
        }
        NetInterfaceTunnelRecord record = {};
        record.m_kind = mixed && index % 2 ? NI_TUNNEL_CLOSE : NI_TUNNEL_BYTES;
        record.m_product = 7;
        record.m_channel = index + 1;
        record.m_size = record.m_kind == NI_TUNNEL_CLOSE ? 4 : size;
        record.m_data = payload.data();
        uint32_t required = 0;
        CheckRelayBatch(NetInterfaceTunnel::encode(record, encoded.data(), static_cast<uint32_t>(encoded.size()), required) == NI_OK,
            "batch reference encoding");
        CheckRelayBatch(relay.send(peer, record) == NI_OK, "batch admission");
        m_expected.insert(m_expected.end(), encoded.begin(), encoded.begin() + required);
        std::memset(payload.data(), 0xee, payload.size());
    }
}

void NetRelayBatchTest::run()
{
    for (uint32_t scenario = 0; scenario < 8; ++scenario)
    {
        NetRelayBatchTest host;
        NetRelayTransportProbe probe;
        NetRelayTransport& relay = probe.m_transport;
        if (scenario == 7)
        {
            NetRelayPoolConfig config;
            config.m_dataRecords.initialCapacity = 0;
            config.m_dataRecords.maximumCapacity = 2;
            config.m_controlRecords = config.m_dataRecords;
            CheckRelayBatch(relay.configurePools(config) == NI_OK, "relay explicit record pools");
            config.m_dataRecords.initialCapacity = 3;
            CheckRelayBatch(relay.configurePools(config) == NI_INVALID_ARGUMENT, "relay invalid pool rollback");
        }
        uint64_t peer = 0;
        CheckRelayBatch(relay.connect("127.0.0.1", 14327, &probe, peer) == NI_OK && relay.poll() && probe.m_peer == peer,
            "batch connect");
        if (scenario == 0)
        {
            for (uint32_t round = 0; round < 3; ++round)
            {
                host.enqueue(relay, peer, 64, 6, true);
                CheckRelayBatch(relay.outputState(peer) == NI_READY, "batch lost pending output");
                relay.poll();
                CheckRelayBatch(host.m_wire == host.m_expected && host.m_attempts.size() == round + 1 &&
                    host.m_attempts.back() == 32 * (30 + 28) && relay.outputState(peer) == NI_OK,
                    "mixed batch bytes/order/wrap or write count");
            }
        }
        else if (scenario == 1)
        {
            host.enqueue(relay, peer, 4, 8192, false);
            relay.poll();
            CheckRelayBatch(host.m_wire == host.m_expected && host.m_attempts.size() == 2 &&
                host.m_attempts[0] == 3 * 8216 && host.m_attempts[1] == 8216, "batch 32768 boundary");
        }
        else if (scenario == 2)
        {
            host.enqueue(relay, peer, 3, 6, false);
            host.m_budget = 30;
            relay.poll();
            CheckRelayBatch(host.m_wire.size() == 30 && host.m_attempts.size() == 4 &&
                host.m_attempts[0] == 90 && host.m_attempts[1] == 30 &&
                std::memcmp(host.m_wire.data(), host.m_expected.data(), 30) == 0 && relay.outputState(peer) == NI_READY,
                "batch full did not retry admissible head");
            host.m_budget = UINT32_MAX;
            relay.poll();
            CheckRelayBatch(host.m_wire == host.m_expected && relay.outputState(peer) == NI_OK,
                "batch rejection lost or duplicated suffix");
        }
        else if (scenario == 3)
        {
            host.enqueue(relay, peer, 8, 6, false);
            host.m_budget = 0;
            relay.poll();
            CheckRelayBatch(host.m_wire.empty() && host.m_attempts.size() == 2 && relay.outputState(peer) == NI_READY,
                "batch full looped or consumed input");
            host.m_budget = UINT32_MAX;
            relay.poll();
            CheckRelayBatch(host.m_wire == host.m_expected && relay.outputState(peer) == NI_OK, "batch resume");
        }
        else if (scenario == 4)
        {
            host.enqueue(relay, peer, 8, 6, false);
            host.m_writeResult = -1;
            relay.poll();
            relay.poll();
            CheckRelayBatch(host.m_wire.empty() && host.m_attempts.size() == 1 && probe.m_closedCount == 1 &&
                relay.outputState(peer) < 0, "batch failure retried or lost terminal");
        }
        else if (scenario == 5)
        {
            host.enqueue(relay, peer, 8, 6, false);
            host.m_limit = 30;
            relay.poll();
            CheckRelayBatch(host.m_wire == host.m_expected && host.m_attempts.size() == 15 && relay.outputState(peer) == NI_OK,
                "batch host accepts only one record");
        }
        else if (scenario == 6)
        {
            for (uint32_t round = 0; round < 2; ++round)
            {
                host.enqueue(relay, peer, 2048, 6, true);
                uint32_t polls = 0;
                while (relay.outputState(peer) == NI_READY && polls++ < 128)
                {
                    CheckRelayBatch(relay.poll(), "relay burst stopped early");
                }
                CheckRelayBatch(host.m_wire == host.m_expected && relay.outputState(peer) == NI_OK && polls == 32,
                    "relay burst FIFO, release or fairness quantum");
            }
        }
        else
        {
            const uint8_t payload[] = {1, 2, 3, 4};
            NetInterfaceTunnelRecord record = {};
            record.m_kind = NI_TUNNEL_BYTES;
            record.m_product = 7;
            record.m_channel = 1;
            record.m_data = payload;
            record.m_size = 4;
            for (uint32_t round = 0; round < 2; ++round)
            {
                host.enqueue(relay, peer, 4, 6, true);
                CheckRelayBatch(relay.send(peer, record) == NI_LIMIT_EXCEEDED, "relay data record pool full");
                record.m_kind = NI_TUNNEL_CLOSE;
                CheckRelayBatch(relay.send(peer, record) == NI_LIMIT_EXCEEDED, "relay control record pool full");
                record.m_kind = NI_TUNNEL_BYTES;
                host.m_budget = 0;
                relay.poll();
                CheckRelayBatch(relay.outputState(peer) == NI_READY && relay.send(peer, record) == NI_LIMIT_EXCEEDED,
                    "relay borrowed head returned its pool slot early");
                host.m_budget = UINT32_MAX;
                relay.poll();
                CheckRelayBatch(host.m_wire == host.m_expected && relay.outputState(peer) == NI_OK,
                    "relay pool reuse or rejection mutated FIFO");
            }
        }
        relay.shutdown();
        CheckRelayBatch(probe.m_closedCount == 1 && !relay.poll(), "batch shutdown");
    }
}