#include "NetGatewayOneWayTest.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif

NetGatewayOneWayTest::NetGatewayOneWayTest() :
m_expected(0),
m_producers(0),
m_received(0),
m_sequences(),
m_finished(0)
{

}

bool NetGatewayOneWayTest::configure(uint32_t count, uint32_t size, uint32_t producers)
{
    if (count == 0 || count > 10000000 || size > 65536 || (producers != 1 && producers != 4) || count < producers)
    {
        return false;
    }
    std::vector<uint8_t> body(size);
    for (uint32_t index = 0; index < size; ++index)
    {
        body[index] = size == 6 ? static_cast<uint8_t>('1' + index) : static_cast<uint8_t>(index * 31U + 17U);
    }
    m_body.swap(body);
    m_expected = count;
    m_producers = producers;
    m_received = 0;
    std::memset(m_sequences, 0, sizeof(m_sequences));
    m_finished = 0;
    return true;
}

bool NetGatewayOneWayTest::receive(const NetInterfaceFrame& frame)
{
    const uint64_t producer = frame.m_requestId >> 48;
    const uint64_t sequence = frame.m_requestId & 0x0000ffffffffffffULL;
    if (m_expected == 0 || m_received >= m_expected || frame.m_kind != NI_DATA || frame.m_productId != 1 ||
        frame.m_messageType != 0x7351 || producer == 0 || producer > m_producers ||
        sequence != static_cast<uint64_t>(m_sequences[producer - 1]) + 1 ||
        sequence > m_expected / m_producers + (producer <= m_expected % m_producers ? 1U : 0U) ||
        frame.m_payloadSize != m_body.size() || (frame.m_payloadSize != 0 &&
        (frame.m_payload == nullptr || std::memcmp(frame.m_payload, m_body.data(), m_body.size()) != 0)))
    {
        return false;
    }
    ++m_sequences[producer - 1];
    if (++m_received == m_expected)
    {
        m_finished = now();
    }
    return true;
}

bool NetGatewayOneWayTest::enabled() const
{
    return m_expected != 0;
}

bool NetGatewayOneWayTest::complete() const
{
    return m_expected != 0 && m_received == m_expected && m_finished != 0;
}

void NetGatewayOneWayTest::report() const
{
    std::printf("ONEWAY_RECEIVED {\"received\":%u,\"bodyBytes\":%u,\"finishedUs\":%llu,\"producerReceived\":[",
        m_received, static_cast<uint32_t>(m_body.size()), static_cast<unsigned long long>(m_finished));
    for (uint32_t index = 0; index < m_producers; ++index)
    {
        std::printf("%s%u", index == 0 ? "" : ",", m_sequences[index]);
    }
    std::puts("]}");
    std::fflush(stdout);
}

uint64_t NetGatewayOneWayTest::now()
{
#ifdef _WIN32
    LARGE_INTEGER frequency = {};
    LARGE_INTEGER ticks = {};
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0 ||
        !QueryPerformanceCounter(&ticks) || ticks.QuadPart < 0)
    {
        return 0;
    }
    const uint64_t value = static_cast<uint64_t>(ticks.QuadPart);
    const uint64_t hz = static_cast<uint64_t>(frequency.QuadPart);
    return value / hz * 1000000ULL + value % hz * 1000000ULL / hz;
#else
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}

void NetGatewayOneWayTest::run()
{
    NetGatewayOneWayTest test;
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_DATA;
    frame.m_productId = 1;
    frame.m_messageType = 0x7351;
    frame.m_requestId = (1ULL << 48) | 1;
    frame.m_payload = reinterpret_cast<const uint8_t*>("123456");
    frame.m_payloadSize = 6;
    if (test.enabled() || test.complete() || test.receive(frame) || test.configure(0, 6, 1) ||
        test.configure(10000001, 6, 1) || test.configure(8, 65537, 1) || test.configure(8, 6, 2) ||
        test.configure(3, 6, 4) || !test.configure(13, 6, 4))
    {
        throw std::runtime_error("One-way configuration boundary");
    }
    for (uint32_t variant = 0; variant < 9; ++variant)
    {
        NetInterfaceFrame bad = frame;
        switch (variant)
        {
        case 0: bad.m_kind = NI_REQUEST; break;
        case 1: bad.m_productId = 2; break;
        case 2: bad.m_messageType = 0; break;
        case 3: bad.m_requestId = 1; break;
        case 4: bad.m_requestId = (5ULL << 48) | 1; break;
        case 5: bad.m_requestId = (1ULL << 48) | 2; break;
        case 6: bad.m_payload = nullptr; break;
        case 7: bad.m_payload = reinterpret_cast<const uint8_t*>("123457"); break;
        default: bad.m_payloadSize = 5; break;
        }
        if (test.receive(bad) || test.m_received != 0 || test.complete())
        {
            throw std::runtime_error("One-way invalid frame advanced counters");
        }
    }
    for (uint32_t sequence = 1; sequence <= 4; ++sequence)
    {
        for (uint32_t producer = 1; producer <= (sequence == 4 ? 1U : 4U); ++producer)
        {
            frame.m_requestId = (static_cast<uint64_t>(producer) << 48) | sequence;
            if (!test.receive(frame) || test.receive(frame) || test.complete() != (sequence == 4))
            {
                throw std::runtime_error("One-way order, duplicate or early completion");
            }
        }
    }
    if (!test.complete() || test.m_received != 13 || test.m_sequences[0] != 4 || test.m_sequences[3] != 3 ||
        !test.configure(1, 0, 1))
    {
        throw std::runtime_error("One-way final producer counts");
    }
    frame.m_requestId = (1ULL << 48) | 1;
    frame.m_payload = nullptr;
    frame.m_payloadSize = 0;
    if (!test.receive(frame) || !test.complete() || test.receive(frame))
    {
        throw std::runtime_error("One-way empty body or overflow");
    }
}