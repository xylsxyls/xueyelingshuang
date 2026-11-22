#include "NetCompactTest.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <cstring>
#include <stdexcept>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdio>

/** 检查独立预期，失败由统一测试执行器捕获
@param [in] passed 必须成立的条件
@param [in] detail 失败原因
*/
static void CompactCheck(bool passed, const char* detail)
{
    if (!passed)
    {
        throw std::runtime_error(detail);
    }
}

NetCompactDrainProbe::NetCompactDrainProbe(NetSenderSubmission* submission) :
m_submission(submission), m_result(NI_OK), m_calls(0), m_close(false), m_throw(false)
{

}

NetCompactSequenceProbe::NetCompactSequenceProbe(uint32_t producers, uint32_t bodySize) :
m_producers(producers), m_bodySize(bodySize), m_received()
{

}

int32_t NetCompactSequenceProbe::consume(void* context, const NetInterfaceFrame& frame)
{
    NetCompactSequenceProbe& probe = *static_cast<NetCompactSequenceProbe*>(context);
    const uint64_t source = frame.m_requestId >> 48;
    const uint64_t sequence = frame.m_requestId & UINT64_C(0x0000ffffffffffff);
    if (source == 0 || source > probe.m_producers || sequence != probe.m_received[source - 1] + 1 ||
        frame.m_kind != NI_DATA || frame.m_productId != 101 || frame.m_messageType != 77 ||
        frame.m_payloadSize != probe.m_bodySize ||
        (probe.m_bodySize == 0 ? frame.m_payload != nullptr :
        frame.m_payload == nullptr || std::memcmp(frame.m_payload, "123456", probe.m_bodySize) != 0))
    {
        return NI_PROTOCOL_ERROR;
    }
    ++probe.m_received[source - 1];
    return NI_OK;
}

int32_t NetCompactDrainProbe::consume(void* context, const NetInterfaceFrame& frame)
{
    NetCompactDrainProbe& probe = *static_cast<NetCompactDrainProbe*>(context);
    ++probe.m_calls;
    CompactCheck(probe.m_submission->nextData() == nullptr, "drain allowed nested borrowing");
    if (probe.m_close)
    {
        probe.m_submission->disconnected(1);
    }
    CompactCheck(frame.m_payloadSize == 6 && std::memcmp(frame.m_payload, "123456", 6) == 0,
        "drain callback lost borrowed content");
    if (probe.m_throw)
    {
        throw std::runtime_error("test consumer failure");
    }
    return probe.m_result;
}

void NetCompactTest::run()
{
    const uint8_t body[] = { '1', '2', '3', '4', '5', '6' };
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_DATA;
    frame.m_productId = 1;
    frame.m_messageType = 0x7351;
    frame.m_requestId = (UINT64_C(1) << 48) | 1;
    frame.m_payload = body;
    frame.m_payloadSize = 6;
    const uint8_t expected[] = { 0xe3, NI_DATA, 1, 0xd1, 0xe6, 1, 6, 0, 1, 0, 0, 0, 0, 0, 1, '1', '2', '3', '4', '5', '6' };
    uint8_t wire[64];
    uint32_t required = 0;
    CompactCheck(NetInterface::encodeCompact(frame, wire, sizeof(wire), required) == NI_OK &&
        required == sizeof(expected) && std::memcmp(wire, expected, required) == 0, "compact golden bytes");
    for (uint32_t cut = 0; cut <= required; ++cut)
    {
        NetInterface decoder;
        decoder.allowCompact(true);
        uint32_t consumed = 0;
        const int32_t first = decoder.feed(wire, cut, consumed);
        CompactCheck(consumed == cut && first == (cut == required ? NI_READY : NI_NEED_MORE), "compact prefix");
        if (cut != required)
        {
            CompactCheck(decoder.feed(wire + cut, required - cut, consumed) == NI_READY && consumed == required - cut,
                "compact suffix");
        }
        NetInterfaceFrame decoded = {};
        CompactCheck(decoder.peek(decoded) == NI_READY && decoded.m_requestId == frame.m_requestId &&
            decoded.m_productId == 1 && decoded.m_messageType == 0x7351 && decoded.m_payloadSize == 6 &&
            std::memcmp(decoded.m_payload, body, 6) == 0, "compact split fields");
        const NetInterfaceFrame* borrowed = nullptr;
        CompactCheck(decoder.peek(borrowed) == NI_READY && borrowed != nullptr &&
            borrowed->m_payload == decoded.m_payload && borrowed->m_requestId == decoded.m_requestId &&
            borrowed->m_productId == decoded.m_productId && borrowed->m_messageType == decoded.m_messageType,
            "compact borrowed metadata");
        uint32_t rejected = 99;
        CompactCheck(decoder.feed(body, sizeof(body), rejected) == NI_READY && rejected == 0 &&
            borrowed->m_payloadSize == 6 && std::memcmp(borrowed->m_payload, body, 6) == 0,
            "borrowed metadata changed before consume");
        decoder.consume();
        CompactCheck(decoder.peek(borrowed) == NI_NEED_MORE && borrowed == nullptr,
            "consumed frame still observable");
        CompactCheck(decoder.finish() == NI_OK, "compact consume boundary");
    }
    const uint32_t values[] = {0, 1, 127, 128, 16383, 16384, 0x0fffffff, 0x10000000, UINT32_MAX};
    for (uint32_t index = 0; index < sizeof(values) / sizeof(values[0]); ++index)
    {
        frame.m_productId = values[index];
        frame.m_messageType = values[sizeof(values) / sizeof(values[0]) - index - 1];
        frame.m_requestId = index == 0 ? 0 : UINT64_MAX - index + 1;
        CompactCheck(NetInterface::encodeCompact(frame, wire, sizeof(wire), required) == NI_OK, "compact integer encode");
        NetInterface decoder;
        decoder.allowCompact(true);
        for (uint32_t byte = 0; byte < required; ++byte)
        {
            uint32_t consumed = 0;
            const int32_t status = decoder.feed(wire + byte, 1, consumed);
            CompactCheck(consumed == 1 && status == (byte + 1 == required ? NI_READY : NI_NEED_MORE), "compact byte fragment");
        }
        NetInterfaceFrame decoded = {};
        CompactCheck(decoder.peek(decoded) == NI_READY && decoded.m_productId == frame.m_productId &&
            decoded.m_requestId == frame.m_requestId && decoded.m_messageType == frame.m_messageType, "compact integer range");
        // 固定头和紧凑头可同流交错；不得把预读到缓存的正文或下一条吞掉。
        uint8_t fixed[64];
        uint32_t fixedSize = 0;
        CompactCheck(NetInterface::encode(frame, fixed, sizeof(fixed), fixedSize) == NI_OK, "legacy encode");
        std::vector<uint8_t> joined(wire, wire + required);
        joined.insert(joined.end(), fixed, fixed + fixedSize);
        decoder.consume();
        uint32_t consumed = 0;
        CompactCheck(decoder.feed(joined.data(), static_cast<uint32_t>(joined.size()), consumed) == NI_READY &&
            consumed == required, "compact concatenated boundary");
        decoder.consume();
        CompactCheck(decoder.feed(joined.data() + consumed, fixedSize, consumed) == NI_READY && consumed == fixedSize,
            "mixed fixed frame");
    }
    std::memset(wire, 0x5a, sizeof(wire));
    CompactCheck(NetInterface::encodeCompact(frame, wire, 1, required) == NI_BUFFER_TOO_SMALL && wire[0] == 0x5a,
        "compact insufficient output modified");
    uint32_t consumed = 0;
    NetInterface unnegotiated;
    CompactCheck(unnegotiated.feed(expected, sizeof(expected), consumed) == NI_PROTOCOL_ERROR,
        "compact accepted without negotiation");
    for (uint32_t kind = 0; kind < 3; ++kind)
    {
        uint8_t bad[32] = {0xe3, NI_DATA, 0x80, 0};
        if (kind == 1)
        {
            std::memset(bad + 2, 0xff, 5);
        }
        if (kind == 2)
        {
            bad[1] = 0xff;
        }
        NetInterface decoder;
        decoder.allowCompact(true);
        CompactCheck(decoder.feed(bad, sizeof(bad), consumed) == NI_PROTOCOL_ERROR &&
            decoder.feed(expected, sizeof(expected), consumed) == NI_PROTOCOL_ERROR, "compact invalid integer or kind");
    }
    for (uint32_t cut = 1; cut < sizeof(expected); ++cut)
    {
        NetInterface decoder;
        decoder.allowCompact(true);
        decoder.feed(expected, cut, consumed);
        CompactCheck(decoder.finish() == NI_TRUNCATED, "compact truncated input");
    }
    NetInterface limited(5);
    limited.allowCompact(true);
    CompactCheck(limited.feed(expected, sizeof(expected), consumed) == NI_LIMIT_EXCEEDED, "compact payload limit");
    // 连续编号短DATA采用单字节头；覆盖全部长度、每个切分和无上下文/溢出拒绝。
    for (uint32_t size = 0; size < 64; ++size)
    {
        std::vector<uint8_t> shortWire(size + 1, 0x5a);
        shortWire[0] = static_cast<uint8_t>(0x80 | size);
        for (uint32_t cut = 0; cut <= shortWire.size(); ++cut)
        {
            NetInterface decoder;
            decoder.allowCompact(true);
            uint32_t consumed = 0;
            CompactCheck(decoder.feed(expected, sizeof(expected), consumed) == NI_READY && decoder.consume() == NI_OK,
                "next DATA context");
            CompactCheck(decoder.feed(shortWire.data(), cut, consumed) == (cut == shortWire.size() ? NI_READY : NI_NEED_MORE) &&
                consumed == cut, "next DATA split prefix");
            if (cut != shortWire.size())
            {
                CompactCheck(decoder.feed(shortWire.data() + cut, static_cast<uint32_t>(shortWire.size()) - cut, consumed) == NI_READY &&
                    consumed == shortWire.size() - cut, "next DATA split suffix");
            }
            NetInterfaceFrame restored = {};
            CompactCheck(decoder.peek(restored) == NI_READY && restored.m_requestId == ((UINT64_C(1) << 48) | 2) &&
                restored.m_productId == 1 && restored.m_messageType == 0x7351 && restored.m_payloadSize == size &&
                (size == 0 || std::memcmp(restored.m_payload, shortWire.data() + 1, size) == 0), "next DATA identity/content");
            if (size != 0)
            {
                std::memset(shortWire.data() + 1, 0xa5, size);
                for (uint32_t byte = 0; byte < size; ++byte)
                {
                    CompactCheck(restored.m_payload[byte] == 0x5a, "next DATA retained caller storage");
                }
                std::memset(shortWire.data() + 1, 0x5a, size);
            }
            decoder.reset();
            CompactCheck(decoder.feed(shortWire.data(), static_cast<uint32_t>(shortWire.size()), consumed) == NI_PROTOCOL_ERROR,
                "next DATA accepted without context");
            decoder.reset();
            CompactCheck(decoder.feed(expected, sizeof(expected), consumed) == NI_READY && decoder.consume() == NI_OK,
                "next DATA EOF context");
            if (cut != 0 && cut < shortWire.size())
            {
                CompactCheck(decoder.feed(shortWire.data(), cut, consumed) == NI_NEED_MORE && decoder.finish() == NI_TRUNCATED,
                    "next DATA truncated body accepted");
            }
            decoder.reset();
            decoder.allowCompact(false);
            CompactCheck(decoder.feed(shortWire.data(), static_cast<uint32_t>(shortWire.size()), consumed) == NI_PROTOCOL_ERROR,
                "next DATA bypassed negotiation");
        }
    }
    {
        NetInterface decoder;
        decoder.allowCompact(true);
        uint8_t full[sizeof(expected)];
        std::memcpy(full, expected, sizeof(full));
        std::memset(full + 7, 0xff, 8);
        uint32_t consumed = 0;
        const uint8_t empty = 0x80;
        CompactCheck(decoder.feed(full, sizeof(full), consumed) == NI_READY && decoder.consume() == NI_OK &&
            decoder.feed(&empty, 1, consumed) == NI_PROTOCOL_ERROR, "next DATA overflow accepted");
    }
    {
        // 小帧缓存大于协议上限时，完整短帧捷径仍须拒绝超长正文。
        NetInterface decoder(6);
        decoder.allowCompact(true);
        uint32_t consumed = 0;
        const uint8_t tooLarge[] = {0x87, 1, 2, 3, 4, 5, 6, 7};
        CompactCheck(decoder.feed(expected, sizeof(expected), consumed) == NI_READY && decoder.consume() == NI_OK &&
            decoder.feed(tooLarge, sizeof(tooLarge), consumed) == NI_LIMIT_EXCEEDED,
            "complete short DATA bypassed configured payload limit");
    }
    const uint8_t delta[] = {0xe4, 6, 1, '1', '2', '3', '4', '5', '6'};
    for (uint32_t cut = 0; cut <= sizeof(delta); ++cut)
    {
        NetInterface decoder;
        decoder.allowCompact(true);
        CompactCheck(decoder.feed(expected, sizeof(expected), consumed) == NI_READY && decoder.consume() == NI_OK,
            "delta full-context prefix");
        CompactCheck(decoder.feed(delta, cut, consumed) == (cut == sizeof(delta) ? NI_READY : NI_NEED_MORE) && consumed == cut,
            "delta split prefix");
        if (cut != sizeof(delta))
        {
            CompactCheck(decoder.feed(delta + cut, sizeof(delta) - cut, consumed) == NI_READY && consumed == sizeof(delta) - cut,
                "delta split suffix");
        }
        NetInterfaceFrame decoded = {};
        CompactCheck(decoder.peek(decoded) == NI_READY && decoded.m_kind == NI_DATA && decoded.m_productId == 1 &&
            decoded.m_messageType == 0x7351 && decoded.m_requestId == (UINT64_C(1) << 48) + 2 &&
            decoded.m_payloadSize == 6 && std::memcmp(decoded.m_payload, body, 6) == 0, "delta restored full identity");
        CompactCheck(decoder.consume() == NI_OK && decoder.feed(delta, sizeof(delta), consumed) == NI_READY,
            "delta chained frame");
        CompactCheck(decoder.peek(decoded) == NI_READY && decoded.m_requestId == (UINT64_C(1) << 48) + 3,
            "delta chained identity");
        decoder.reset();
        CompactCheck(decoder.feed(delta, sizeof(delta), consumed) == NI_PROTOCOL_ERROR, "delta reset reused stale context");
    }
    for (uint32_t badKind = 0; badKind < 5; ++badKind)
    {
        NetInterface decoder;
        decoder.allowCompact(true);
        uint8_t full[sizeof(expected)];
        std::memcpy(full, expected, sizeof(expected));
        if (badKind == 4) { std::memset(full + 7, 0xff, 8); }
        CompactCheck(decoder.feed(full, sizeof(full), consumed) == NI_READY && decoder.consume() == NI_OK, "delta invalid setup");
        uint8_t bad[] = {0xe4, 6, 1, '1', '2', '3', '4', '5', '6'};
        if (badKind == 0) { decoder.allowCompact(false); decoder.allowCompact(true); }
        if (badKind == 1) { bad[2] = 0; }
        if (badKind == 2) { bad[1] = 0x80; bad[2] = 0; }
        if (badKind == 3) { bad[2] = 0xff; bad[3] = 0xff; bad[4] = 0xff; bad[5] = 0xff; bad[6] = 0x10; }
        CompactCheck(decoder.feed(bad, sizeof(bad), consumed) == NI_PROTOCOL_ERROR,
            "delta missing context, zero, noncanonical or overflowing identity accepted");
    }
    for (uint32_t cut = 1; cut < sizeof(delta); ++cut)
    {
        NetInterface decoder;
        decoder.allowCompact(true);
        CompactCheck(decoder.feed(expected, sizeof(expected), consumed) == NI_READY && decoder.consume() == NI_OK, "delta EOF setup");
        decoder.feed(delta, cut, consumed);
        CompactCheck(decoder.finish() == NI_TRUNCATED, "delta truncated frame accepted");
    }
}

/** 在测试两端之间转移有限密文，保留背压后未消费的后缀
@param [in,out] from 密文输出端
@param [in,out] to 密文接收端
@param [in] fragment 最大复制字节数
*/
static void CompactTransfer(NetInterfaceConnection& from, NetInterfaceConnection& to, uint32_t fragment)
{
    const uint8_t* bytes = nullptr;
    uint32_t size = 0;
    CompactCheck(from.peekOutput(bytes, size) >= 0, "compact TLS peek output");
    if (size != 0)
    {
        const uint32_t amount = size < fragment ? size : fragment;
        uint32_t consumed = 0;
        CompactCheck(to.feed(bytes, amount, consumed) >= 0 && consumed <= amount, "compact TLS input");
        CompactCheck(from.consumeOutput(consumed) >= 0, "compact TLS consume output");
    }
}

void NetCompactTest::tls()
{
    // 所有四种能力组合；未协商紧凑格式的会话继续使用原固定头。
    for (uint32_t mask = 0; mask < 4; ++mask)
    {
        NetInterfaceHandshakeOptions a = {};
        a.m_platform = 1;
        a.m_capabilities = (mask & 1) ? 31 : 15;
        a.m_maxRequests = 32;
        a.m_maxChunk = 65536;
        a.m_timeoutMs = 100;
        a.m_nonce[0] = a.m_device[0] = 1;
        NetInterfaceHandshakeOptions b = a;
        b.m_server = 1;
        b.m_platform = 0;
        b.m_device[0] = 0;
        b.m_nonce[0] = 2;
        b.m_capabilities = (mask & 2) ? 31 : 15;
        NetInterfaceHandshake client(a, 1000);
        NetInterfaceHandshake server(b, 1000);
        NetInterfaceFrame hello = {};
        CompactCheck(client.peekOutput(hello) == NI_READY && server.accept(hello, 1001) == NI_NEED_MORE,
            "compact HELLO negotiation");
        client.consumeOutput();
        CompactCheck(server.peekOutput(hello) == NI_READY && client.accept(hello, 1002) == NI_READY,
            "compact HELLO reply");
        server.consumeOutput();
        uint32_t fields[4] = {};
        uint8_t nonce[16] = {};
        uint8_t device[16] = {};
        CompactCheck(server.peerInfo(fields, nonce, device) == NI_READY && fields[1] == (mask == 3 ? 31U : 15U),
            "compact server negotiated capability");
        CompactCheck(client.peerInfo(fields, nonce, device) == NI_READY && fields[1] == (mask == 3 ? 31U : 15U),
            "compact client negotiated capability");
    }
    NetInterfaceConnection client(NetInterfaceTlsTestPair::options(false, "localhost"));
    NetInterfaceConnection server(NetInterfaceTlsTestPair::options(true, nullptr));
    NetConnectionPoolConfig pools;
    pools.m_dataRecords.initialCapacity = pools.m_dataRecords.maximumCapacity = 1;
    pools.m_controlRecords.initialCapacity = pools.m_controlRecords.maximumCapacity = 1;
    for (uint32_t index = 0; index < 5; ++index)
    {
        pools.m_dataBytes.m_classes[index].initialCapacity = pools.m_dataBytes.m_classes[index].maximumCapacity = 1;
        pools.m_controlBytes.m_classes[index].initialCapacity = pools.m_controlBytes.m_classes[index].maximumCapacity = 1;
    }
    CompactCheck(client.configurePools(pools) == NI_OK, "compact one-slot physical pools");
    for (uint32_t step = 0; step < 10000 && (client.status() != NET_INTERFACE_TLS_READY || server.status() != NET_INTERFACE_TLS_READY); ++step)
    {
        CompactCheck(client.progress(1000) >= 0 && server.progress(1000) >= 0, "compact TLS handshake");
        CompactTransfer(client, server, 7);
        CompactTransfer(server, client, 7);
    }
    CompactCheck(client.status() == NET_INTERFACE_TLS_READY && server.status() == NET_INTERFACE_TLS_READY, "compact TLS ready");
    client.configureCompact(true);
    server.configureCompact(true);
    const uint32_t sizes[] = {0, 6, 65, 900, 17000, 131000, 1048576, 6};
    for (uint32_t iteration = 0; iteration < 32; ++iteration)
    {
        // 连续空poll后仍必须由下一段密文唤醒TLS读取，不依赖定时重新尝试。
        for (uint32_t idle = 0; idle < 64; ++idle)
        {
            CompactCheck(client.progress(1000) == NET_INTERFACE_TLS_READY &&
                server.progress(1000) == NET_INTERFACE_TLS_READY, "idle TLS progress changed state");
            CompactTransfer(client, server, 4000);
            CompactTransfer(server, client, 4000);
        }
        const uint32_t size = sizes[iteration % 8];
        std::vector<uint8_t> body(size);
        for (uint32_t index = 0; index < size; ++index)
        {
            body[index] = static_cast<uint8_t>(index * 37 + iteration);
        }
        // 同一已协商TLS流中交错紧凑和固定头，覆盖缓冲类切换和完整宽度字段。
        client.configureCompact((iteration & 1) == 0);
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_DATA;
        frame.m_productId = UINT32_MAX;
        frame.m_messageType = iteration;
        frame.m_requestId = UINT64_MAX - iteration;
        frame.m_payload = body.empty() ? nullptr : body.data();
        frame.m_payloadSize = size;
        CompactCheck(client.send(frame) == NI_OK, "compact capacity and reuse across size classes");
        if ((iteration & 1) != 0 || size > 64)
        {
            CompactCheck(client.send(frame) == NI_LIMIT_EXCEEDED, "queued frame physical pool rollback");
        }
        bool received = false;
        for (uint32_t step = 0; step < 10000; ++step)
        {
            CompactCheck(client.progress(1000) >= 0 && server.progress(1000) >= 0, "compact TLS transfer progress");
            CompactTransfer(client, server, 4000);
            CompactTransfer(server, client, 4000);
            NetInterfaceFrame decoded = {};
            if (server.peek(decoded) == NI_READY)
            {
                CompactCheck(!received && decoded.m_kind == NI_DATA && decoded.m_requestId == frame.m_requestId &&
                    decoded.m_productId == UINT32_MAX && decoded.m_messageType == iteration && decoded.m_payloadSize == size &&
                    (size == 0 || std::memcmp(decoded.m_payload, body.data(), size) == 0), "compact TLS decoded content/order");
                const NetInterfaceFrame* borrowed = nullptr;
                CompactCheck(server.peek(borrowed) == NI_READY && borrowed != nullptr &&
                    borrowed->m_payload == decoded.m_payload && borrowed->m_payloadSize == size &&
                    borrowed->m_kind == NI_DATA && borrowed->m_requestId == frame.m_requestId &&
                    borrowed->m_productId == UINT32_MAX && borrowed->m_messageType == iteration,
                    "TLS borrowed metadata differs from snapshot");
                CompactCheck(server.decodeBuffered() == NI_READY &&
                    (size == 0 || std::memcmp(borrowed->m_payload, body.data(), size) == 0),
                    "TLS borrowed frame advanced without consume");
                received = true;
                server.consume();
                CompactCheck(server.peek(borrowed) == NI_NEED_MORE && borrowed == nullptr,
                    "TLS returned consumed metadata");
            }
            if (received && client.outputState() == NI_OK)
            {
                break;
            }
        }
        CompactCheck(received, "compact TLS no missing frame");
    }
    // DATA物理槽占用时拒绝下一条，拒绝不能推进增量上下文；控制帧越过DATA也不改变上下文。
    client.configureCompact(true);
    uint8_t queuedBody[65];
    std::memset(queuedBody, 0x5a, sizeof(queuedBody));
    NetInterfaceFrame queued = {};
    queued.m_kind = NI_DATA;
    queued.m_productId = 7;
    queued.m_messageType = 123;
    queued.m_requestId = 1000;
    queued.m_payload = queuedBody;
    queued.m_payloadSize = sizeof(queuedBody);
    CompactCheck(client.send(queued) == NI_OK, "delta rollback queued prefix");
    NetInterfaceFrame rejected = queued;
    rejected.m_requestId = 1001;
    rejected.m_payloadSize = 6;
    CompactCheck(client.send(rejected) == NI_LIMIT_EXCEEDED, "delta rollback did not exhaust physical slot");
    NetInterfaceFrame control = queued;
    control.m_kind = NI_DATA_ACK;
    control.m_requestId = 77;
    control.m_payload = nullptr;
    control.m_payloadSize = 0;
    CompactCheck(client.send(control) == NI_OK, "delta independent control allocation");
    for (uint32_t phase = 0; phase < 2; ++phase)
    {
        if (phase != 0)
        {
            rejected.m_requestId = 1002;
            CompactCheck(client.send(rejected) == NI_OK, "delta append after failed admission");
        }
        uint32_t seen = 0;
        uint32_t dataCount = 0;
        uint32_t controlCount = 0;
        const uint32_t expectedCount = phase == 0 ? 2 : 1;
        for (uint32_t turn = 0; turn < 10000 && seen != expectedCount; ++turn)
        {
            CompactCheck(client.progress(1000) >= 0 && server.progress(1000) >= 0, "delta rollback TLS progress");
            CompactTransfer(client, server, 7);
            CompactTransfer(server, client, 7);
            NetInterfaceFrame decoded = {};
            if (server.peek(decoded) != NI_READY)
            {
                continue;
            }
            if (decoded.m_kind == NI_DATA_ACK)
            {
                CompactCheck(phase == 0 && decoded.m_requestId == 77 && decoded.m_payloadSize == 0 &&
                    ++controlCount == 1, "delta control changed or duplicated");
            }
            else
            {
                const uint32_t expectedSize = phase == 0 ? 65 : 6;
                CompactCheck(decoded.m_kind == NI_DATA && decoded.m_productId == 7 && decoded.m_messageType == 123 &&
                    decoded.m_requestId == (phase == 0 ? 1000 : 1002) && decoded.m_payloadSize == expectedSize &&
                    std::memcmp(decoded.m_payload, queuedBody, expectedSize) == 0 && ++dataCount == 1,
                    "rejected DATA advanced delta context or control reordered DATA context");
            }
            ++seen;
            server.consume();
        }
        CompactCheck(seen == expectedCount && dataCount == 1, "delta rollback lost accepted frame");
    }
    const uint8_t content[] = {'1', '2', '3', '4', '5', '6'};
    NetInterfaceFrame sent = {};
    sent.m_kind = NI_DATA;
    sent.m_productId = 7;
    sent.m_messageType = 123;
    sent.m_payload = content;
    sent.m_payloadSize = sizeof(content);
    for (uint32_t index = 0; index < 1000; ++index)
    {
        sent.m_requestId = (UINT64_C(1) << 48) + index + 1;
        CompactCheck(client.send(sent) == NI_OK, "delta TLS continuous admission");
    }
    uint32_t received = 0;
    for (uint32_t turn = 0; turn < 10000 && received != 1000; ++turn)
    {
        CompactCheck(client.progress(1000) >= 0 && server.progress(1000) >= 0, "delta TLS progress");
        CompactTransfer(client, server, 137);
        CompactTransfer(server, client, 137);
        NetInterfaceFrame decoded = {};
        if (server.peek(decoded) == NI_READY)
        {
            CompactCheck(decoded.m_kind == NI_DATA && decoded.m_productId == 7 && decoded.m_messageType == 123 &&
                decoded.m_requestId == (UINT64_C(1) << 48) + received + 1 && decoded.m_payloadSize == 6 &&
                std::memcmp(decoded.m_payload, content, 6) == 0, "delta TLS identity/content/FIFO");
            ++received;
            server.consume();
        }
    }
    CompactCheck(received == 1000, "delta TLS incomplete stream");
    // 不推进运输直到TLS物理缓冲满，再逐步转移；背压后重试同一号，不能重复或丢失。
    uint32_t submitted = 0;
    uint32_t totalReceived = 0;
    bool blocked = false;
    for (; submitted < 10000; ++submitted)
    {
        sent.m_requestId = (UINT64_C(2) << 48) + submitted + 1;
        const int32_t accepted = client.send(sent);
        if (accepted == NI_LIMIT_EXCEEDED)
        {
            blocked = true;
            break;
        }
        CompactCheck(accepted == NI_OK, "short TLS burst failed before physical backpressure");
    }
    CompactCheck(blocked && submitted != 0 && submitted < 10000, "short TLS burst did not reach physical backpressure");
    for (uint32_t turn = 0; turn < 100000 && totalReceived != 10000; ++turn)
    {
        if (submitted != 10000)
        {
            sent.m_requestId = (UINT64_C(2) << 48) + submitted + 1;
            const int32_t accepted = client.send(sent);
            CompactCheck(accepted == NI_OK || accepted == NI_LIMIT_EXCEEDED, "short TLS retry failed");
            if (accepted == NI_OK)
            {
                ++submitted;
            }
        }
        CompactCheck(client.progress(1000) >= 0 && server.progress(1000) >= 0, "short TLS backpressure progress");
        CompactTransfer(client, server, 137);
        CompactTransfer(server, client, 137);
        NetInterfaceFrame decoded = {};
        if (server.peek(decoded) == NI_READY)
        {
            CompactCheck(decoded.m_kind == NI_DATA && decoded.m_productId == 7 && decoded.m_messageType == 123 &&
                decoded.m_requestId == (UINT64_C(2) << 48) + totalReceived + 1 && decoded.m_payloadSize == 6 &&
                std::memcmp(decoded.m_payload, content, 6) == 0, "short TLS retry changed content or sequence");
            ++totalReceived;
            server.consume();
        }
    }
    CompactCheck(submitted == 10000 && totalReceived == 10000, "short TLS physical backpressure lost accepted frames");
    for (uint32_t idle = 0; idle < 64; ++idle)
    {
        CompactCheck(server.progress(1000) == NET_INTERFACE_TLS_READY, "drained TLS idle changed state");
    }
    // 空读暂停不能掩盖之后到达的TCP EOF；没有close_notify必须报告截断。
    CompactCheck(server.finishInput() == NI_OK && server.progress(1001) == NET_INTERFACE_TLS_TRUNCATED &&
        server.progress(1002) == NET_INTERFACE_TLS_TRUNCATED, "idle TLS reader missed terminal input");
}

void NetCompactTest::drainConcurrent()
{
    for (uint32_t scenario = 0; scenario < 4; ++scenario)
    {
        const uint32_t producers = scenario % 2 == 0 ? 1 : 4;
        const uint32_t bodySize = scenario < 2 ? 6 : 0;
        NetSenderSubmission queue;
        NetCompactSequenceProbe probe(producers, bodySize);
        CompactCheck(queue.start(1, 101) == NI_OK && queue.ready(1, 128, 4096) == NI_OK, "direct concurrent ready");
        const uint32_t perProducer = 1000000 / producers;
        std::atomic<bool> go(false), failed(false);
        std::vector<std::thread> threads;
        try
        {
            for (uint32_t producer = 0; producer < producers; ++producer)
            {
                // 每个生产者提交独立递增序号，接收端必须逐条验证，不能只统计入队次数。
                threads.emplace_back([&, producer]()
                {
                    while (!go.load(std::memory_order_acquire)) { std::this_thread::yield(); }
                    NetInterfaceFrame frame = {};
                    frame.m_kind = NI_DATA;
                    frame.m_productId = 101;
                    frame.m_messageType = 77;
                    frame.m_payload = bodySize == 0 ? nullptr : reinterpret_cast<const uint8_t*>("123456");
                    frame.m_payloadSize = bodySize;
                    for (uint32_t index = 1; index <= perProducer && !failed.load(std::memory_order_relaxed); ++index)
                    {
                        frame.m_requestId = (static_cast<uint64_t>(producer + 1) << 48) | index;
                        while (!failed.load(std::memory_order_relaxed))
                        {
                            const int32_t result = queue.data(frame);
                            if (result == NI_OK) { break; }
                            if (result != NI_LIMIT_EXCEEDED) { failed.store(true); break; }
                            std::this_thread::yield();
                        }
                    }
                });
            }
        }
        catch (...)
        {
            failed.store(true);
            go.store(true, std::memory_order_release);
            for (size_t index = 0; index < threads.size(); ++index) { threads[index].join(); }
            throw;
        }
        uint64_t total = 0;
        const auto started = std::chrono::steady_clock::now();
        go.store(true, std::memory_order_release);
        while (total < 1000000 && !failed.load())
        {
            uint32_t messages = 0;
            uint64_t bytes = 0;
            if (queue.beginPoll() != NI_OK || queue.drainData(&probe, NetCompactSequenceProbe::consume,
                2304, 2097152, messages, bytes) != NI_OK || bytes != static_cast<uint64_t>(messages) * bodySize)
            {
                failed.store(true);
                break;
            }
            total += messages;
            if (std::chrono::steady_clock::now() - started > std::chrono::seconds(30)) { failed.store(true); }
            if (messages == 0) { std::this_thread::yield(); }
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started).count();
        for (size_t index = 0; index < threads.size(); ++index) { threads[index].join(); }
        CompactCheck(!failed.load() && queue.idle() && queue.queuedBytes() == 0, "direct concurrent FIFO/accounting");
        for (uint32_t index = 0; index < producers; ++index)
        {
            CompactCheck(probe.m_received[index] == perProducer, "direct concurrent producer starvation/loss");
        }
        std::printf("SUBMISSION_DRAIN body=%u producers=%u count=%llu elapsedUs=%lld messagesPerSecond=%.2f\n", bodySize, producers,
            static_cast<unsigned long long>(total), static_cast<long long>(elapsed), static_cast<double>(total) * 1000000 / elapsed);
    }
}

void NetCompactTest::submission()
{
    drainConcurrent();
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_DATA;
    frame.m_productId = 101;
    frame.m_messageType = 77;
    frame.m_requestId = 1;
    frame.m_payload = reinterpret_cast<const uint8_t*>("123456");
    frame.m_payloadSize = 6;
    NetSenderSubmissionPools pools;
    pools.m_frames.initialCapacity = pools.m_frames.maximumCapacity = 1;
    // 基础队列物理容量包含一个保留哨兵；65个节点对应本例64个载荷。
    pools.m_queue.initialCapacity = pools.m_queue.maximumCapacity = 65;
    NetSenderSubmission tiny;
    CompactCheck(tiny.configurePools(pools) == NI_OK && tiny.start(1, 101) == NI_OK && tiny.ready(1, 128, 4096) == NI_OK,
        "inline queue configuration");
    for (uint32_t round = 0; round < 8; ++round)
    {
        for (uint32_t index = 0; index < 64; ++index)
        {
            frame.m_requestId = index + 1;
            CompactCheck(tiny.data(frame) == NI_OK, "inline queue occupied a separate frame record");
        }
        CompactCheck(tiny.data(frame) == NI_LIMIT_EXCEEDED && tiny.queuedBytes() == 384, "inline physical queue rollback");
        CompactCheck(tiny.beginPoll() == NI_OK, "inline begin drain");
        for (uint32_t index = 0; index < 64; ++index)
        {
            NetSenderSubmissionCommand command = {};
            CompactCheck(tiny.next(command) == NI_READY && command.m_frame.m_requestId == index + 1 &&
                command.m_frame.m_payloadSize == 6 && std::memcmp(command.m_frame.m_payload, "123456", 6) == 0,
                "inline owned data or sequence");
            if (index == 0)
            {
                tiny.finish(NI_LIMIT_EXCEEDED);
                CompactCheck(tiny.beginPoll() == NI_OK && tiny.next(command) == NI_READY &&
                    command.m_frame.m_requestId == 1 && std::memcmp(command.m_frame.m_payload, "123456", 6) == 0,
                    "inline borrowed retry overwritten");
            }
            tiny.finish(NI_OK);
        }
        CompactCheck(tiny.idle() && tiny.queuedBytes() == 0, "inline batch counters did not converge");
    }
    // 空正文不占字节预算，但仍占有限物理节点；零字节不能绕过容量或变成空队列。
    {
        NetSenderSubmission empty;
        CompactCheck(empty.configurePools(pools) == NI_OK && empty.start(1, 101) == NI_OK &&
            empty.ready(1, 128, 4096) == NI_OK, "empty queue configuration");
        for (uint32_t round = 0; round < 4; ++round)
        {
            NetInterfaceFrame sent = frame;
            sent.m_payload = nullptr;
            sent.m_payloadSize = 0;
            for (uint32_t index = 0; index < 64; ++index)
            {
                sent.m_requestId = index + 1;
                CompactCheck(empty.data(sent) == NI_OK, "empty physical admission");
            }
            CompactCheck(empty.data(sent) == NI_LIMIT_EXCEEDED && empty.queuedBytes() == 0 && !empty.idle(),
                "empty bypassed node budget or changed byte accounting");
            CompactCheck(empty.beginPoll() == NI_OK, "empty begin drain");
            uint32_t expected = 1;
            uint32_t messages = 0;
            uint64_t bytes = 0;
            // context是本轮下一个独立预期序号；帧仅回调期间借用，返回值反映身份及空正文校验。
            const int32_t result = empty.drainData(&expected, [](void* context, const NetInterfaceFrame& value) -> int32_t
            {
                uint32_t& next = *static_cast<uint32_t*>(context);
                if (value.m_kind != NI_DATA || value.m_productId != 101 || value.m_messageType != 77 ||
                    value.m_requestId != next || value.m_payload != nullptr || value.m_payloadSize != 0)
                {
                    return NI_PROTOCOL_ERROR;
                }
                ++next;
                return NI_OK;
            }, 64, 1, messages, bytes);
            CompactCheck(result == NI_OK && expected == 65 && messages == 64 && bytes == 0 &&
                empty.idle() && empty.queuedBytes() == 0, "empty direct consumption/reuse/accounting");
        }
    }
    // 所有者退出时允许保留借用；后面还有多批未消费入口，析构必须全部收敛。
    for (uint32_t mode = 0; mode < 2; ++mode)
    {
        NetSenderSubmission ending;
        CompactCheck(ending.start(1, 101) == NI_OK && ending.ready(1, 128, 4096) == NI_OK, "inline destructor setup");
        for (uint32_t index = 0; index < 2048; ++index)
        {
            CompactCheck(ending.data(frame) == NI_OK, "inline destructor pending batch");
        }
        NetSenderSubmissionCommand borrowed = {};
        CompactCheck(ending.beginPoll() == NI_OK && ending.next(borrowed) == NI_READY, "inline destructor borrow");
        if (mode != 0)
        {
            ending.disconnected(1);
            CompactCheck(std::memcmp(borrowed.m_frame.m_payload, "123456", 6) == 0, "inline disconnect invalidated borrow");
        }
    }
    // 快速视图沿用同一借用及重试规则，控制帧必须回到通用动作路径。
    {
        NetSenderSubmission views;
        CompactCheck(views.start(1, 101) == NI_OK && views.ready(1, 128, 4096) == NI_OK, "data view setup");
        CompactCheck(views.data(frame) == NI_OK && views.beginPoll() == NI_OK, "data view submission");
        const NetInterfaceFrame* view = views.nextData();
        CompactCheck(view != nullptr && std::memcmp(view->m_payload, "123456", 6) == 0, "data view content");
        CompactCheck(views.nextData() == nullptr, "data view double borrow");
        views.finish(NI_LIMIT_EXCEEDED);
        CompactCheck(views.nextData() == nullptr && views.beginPoll() == NI_OK, "data view repeated same-round retry");
        view = views.nextData();
        CompactCheck(view != nullptr, "data view next-round retry");
        views.disconnected(1);
        CompactCheck(std::memcmp(view->m_payload, "123456", 6) == 0, "data view close invalidated borrow");
        views.finish(NI_OK);
        CompactCheck(views.idle() && views.queuedBytes() == 0, "data view close accounting");
        CompactCheck(views.start(2, 101) == NI_OK && views.ready(2, 128, 4096) == NI_OK, "data view restart");
        NetInterfaceFrame control = frame;
        control.m_kind = NI_DATA_ACK;
        CompactCheck(views.data(control) == NI_OK && views.data(frame) == NI_OK && views.beginPoll() == NI_OK, "data view barrier setup");
        CompactCheck(views.nextData() == nullptr, "data view bypassed control");
        NetSenderSubmissionCommand command = {};
        CompactCheck(views.next(command) == NI_READY && command.m_frame.m_kind == NI_DATA_ACK, "data view control fallback");
        views.finish(NI_OK);
        CompactCheck(views.nextData() != nullptr, "data view after control");
        views.finish(NI_OK);
        CompactCheck(views.idle(), "data view drain");
    }
    // 直接前缀半途切回通用借用，背压原项与后续控制帧仍严格保序。
    {
        NetSenderSubmission switching;
        NetCompactDrainProbe probe(&switching);
        CompactCheck(switching.start(1, 101) == NI_OK && switching.ready(1, 128, 4096) == NI_OK, "switching setup");
        for (uint32_t index = 0; index < 96; ++index)
        {
            frame.m_requestId = index + 1;
            CompactCheck(switching.data(frame) == NI_OK, "switching admission");
        }
        CompactCheck(switching.beginPoll() == NI_OK, "switching poll");
        uint32_t messages = 0;
        uint64_t bytes = 0;
        CompactCheck(switching.drainData(&probe, NetCompactDrainProbe::consume, 7, 1024, messages, bytes) == NI_OK &&
            messages == 7 && bytes == 42, "switching prefix");
        probe.m_result = NI_LIMIT_EXCEEDED;
        CompactCheck(switching.drainData(&probe, NetCompactDrainProbe::consume, 1, 1024, messages, bytes) == NI_LIMIT_EXCEEDED,
            "switching backpressure");
        NetSenderSubmissionCommand command = {};
        CompactCheck(switching.next(command) == NI_NEED_MORE && switching.beginPoll() == NI_OK,
            "switching repeated blocked frame");
        for (uint32_t index = 7; index < 96; ++index)
        {
            CompactCheck(switching.next(command) == NI_READY && command.m_frame.m_requestId == index + 1 &&
                std::memcmp(command.m_frame.m_payload, "123456", 6) == 0, "switching lost or reordered suffix");
            switching.finish(NI_OK);
        }
        CompactCheck(switching.idle() && switching.queuedBytes() == 0, "switching leaked bytes");
        frame.m_requestId = 1;
    }
    // 回调恰好在消息量子末尾关闭时，也必须立即归还未发送的直接前缀。
    for (uint32_t mode = 0; mode < 6; ++mode)
    {
        NetSenderSubmission closing;
        NetCompactDrainProbe probe(&closing);
        probe.m_close = true;
        probe.m_result = mode % 3 == 0 ? NI_OK : mode % 3 == 1 ? NI_LIMIT_EXCEEDED : NI_INVALID_STATE;
        CompactCheck(closing.start(1, 101) == NI_OK && closing.ready(1, 128, 4096) == NI_OK, "close quantum setup");
        for (uint32_t index = 0; index < 32; ++index)
        {
            CompactCheck(closing.data(frame) == NI_OK, "close quantum input");
        }
        uint32_t messages = 99;
        uint64_t bytes = 99;
        CompactCheck(closing.beginPoll() == NI_OK && closing.drainData(&probe, NetCompactDrainProbe::consume,
            mode < 3 ? 1 : 32, 4096, messages, bytes) == probe.m_result, "close quantum result");
        const uint32_t accepted = probe.m_result == NI_OK ? 1 : 0;
        CompactCheck(messages == accepted && bytes == accepted * 6 && probe.m_calls == 1 &&
            closing.idle() && closing.queuedBytes() == 0, "close quantum ownership/accounting");
    }
    // 多批前缀交错零字节、内联、独占正文和控制帧，源缓冲每次立即覆写。
    {
        NetSenderSubmission mixed;
        CompactCheck(mixed.start(1, 101) == NI_OK && mixed.ready(1, 128, 4096) == NI_OK, "mixed prefix setup");
        uint8_t content[65];
        for (uint32_t index = 0; index < 160; ++index)
        {
            NetInterfaceFrame sent = frame;
            sent.m_kind = static_cast<uint16_t>(index % 4 == 3 ? NI_DATA_ACK : NI_DATA);
            sent.m_requestId = index + 1;
            sent.m_payloadSize = index % 4 == 0 ? 0 : index % 4 == 1 ? 6 : index % 4 == 2 ? 65 : 1;
            std::memset(content, static_cast<int>(index), sizeof(content));
            sent.m_payload = sent.m_payloadSize == 0 ? nullptr : content;
            CompactCheck(mixed.data(sent) == NI_OK, "mixed prefix admission");
        }
        CompactCheck(mixed.beginPoll() == NI_OK, "mixed prefix poll");
        for (uint32_t index = 0; index < 160; ++index)
        {
            const NetInterfaceFrame* view = mixed.nextData();
            NetSenderSubmissionCommand command = {};
            if (view == nullptr)
            {
                CompactCheck(mixed.next(command) == NI_READY, "mixed prefix generic fallback");
                view = &command.m_frame;
            }
            const uint32_t size = index % 4 == 0 ? 0 : index % 4 == 1 ? 6 : index % 4 == 2 ? 65 : 1;
            CompactCheck(view->m_requestId == index + 1 && view->m_payloadSize == size &&
                view->m_kind == (index % 4 == 3 ? NI_DATA_ACK : NI_DATA), "mixed prefix FIFO metadata");
            for (uint32_t byte = 0; byte < size; ++byte)
            {
                CompactCheck(view->m_payload[byte] == static_cast<uint8_t>(index), "mixed prefix borrowed content changed");
            }
            mixed.finish(NI_OK);
        }
        CompactCheck(mixed.idle() && mixed.queuedBytes() == 0, "mixed prefix physical ownership leaked");
    }
    // 批量消费仍逐条保持借用；背压停在队首，异常不得留下占用或吞掉正文。
    {
        NetSenderSubmission drained;
        NetCompactDrainProbe probe(&drained);
        CompactCheck(drained.start(1, 101) == NI_OK && drained.ready(1, 128, 4096) == NI_OK, "drain setup");
        for (uint32_t index = 0; index < 160; ++index)
        {
            CompactCheck(drained.data(frame) == NI_OK, "drain enqueue");
        }
        uint32_t messages = 99;
        uint64_t bytes = 99;
        CompactCheck(drained.beginPoll() == NI_OK &&
            drained.drainData(&probe, nullptr, 2, 12, messages, bytes) == NI_INVALID_ARGUMENT && messages == 0 && bytes == 0,
            "drain invalid consumer");
        CompactCheck(drained.drainData(&probe, NetCompactDrainProbe::consume, 0, 12, messages, bytes) == NI_OK &&
            messages == 0 && probe.m_calls == 0, "drain zero quantum");
        CompactCheck(drained.drainData(&probe, NetCompactDrainProbe::consume, 2, 1, messages, bytes) == NI_OK &&
            messages == 1 && bytes == 6 && probe.m_calls == 1, "drain byte quantum");
        probe.m_result = NI_LIMIT_EXCEEDED;
        CompactCheck(drained.drainData(&probe, NetCompactDrainProbe::consume, 3, 18, messages, bytes) == NI_LIMIT_EXCEEDED &&
            messages == 0 && bytes == 0 && probe.m_calls == 2, "drain backpressure consumed data");
        CompactCheck(drained.drainData(&probe, NetCompactDrainProbe::consume, 3, 18, messages, bytes) == NI_OK &&
            messages == 0 && probe.m_calls == 2, "drain repeated same-round backpressure");
        probe.m_result = NI_OK;
        CompactCheck(drained.beginPoll() == NI_OK &&
            drained.drainData(&probe, NetCompactDrainProbe::consume, 1000, 10000, messages, bytes) == NI_OK &&
            messages == 159 && bytes == 954 && drained.idle() && drained.queuedBytes() == 0, "drain retry/FIFO/accounting");
        CompactCheck(drained.data(frame) == NI_OK && drained.beginPoll() == NI_OK, "drain exception input");
        probe.m_throw = true;
        CompactCheck(drained.drainData(&probe, NetCompactDrainProbe::consume, 1, 6, messages, bytes) == NI_INVALID_STATE &&
            messages == 0 && bytes == 0, "drain exception escaped or counted acceptance");
        probe.m_throw = false;
        probe.m_close = true;
        CompactCheck(drained.beginPoll() == NI_OK &&
            drained.drainData(&probe, NetCompactDrainProbe::consume, 10, 60, messages, bytes) == NI_OK && messages == 1 &&
            bytes == 6 && drained.idle() && drained.queuedBytes() == 0, "drain close invalidated borrow or leaked input");
    }
    {
        NetSenderSubmission submission;
        CompactCheck(submission.start(1, 101) == NI_OK && submission.ready(1, 8, 65536) == NI_OK, "short copy ready");
        for (uint32_t round = 0; round < 4; ++round)
        {
            for (uint32_t size = 0; size <= 66; ++size)
            {
                uint8_t bytes[68];
                std::memset(bytes, static_cast<int>(size + round), sizeof(bytes));
                NetInterfaceFrame sent = frame;
                sent.m_requestId = round * 67 + size + 1;
                sent.m_payloadSize = size;
                sent.m_payload = size == 0 ? nullptr : bytes + 1;
                CompactCheck(submission.data(sent) == NI_OK, "short copy submit");
                std::memset(bytes, 0xee, sizeof(bytes));
            }
            CompactCheck(submission.beginPoll() == NI_OK, "short copy poll");
            for (uint32_t size = 0; size <= 66; ++size)
            {
                NetSenderSubmissionCommand command = {};
                const NetInterfaceFrame* view = size % 2 == 0 ? submission.nextData() : nullptr;
                if (view == nullptr)
                {
                    CompactCheck(submission.next(command) == NI_READY, "short copy generic next");
                    view = &command.m_frame;
                }
                CompactCheck(view->m_requestId == round * 67 + size + 1 && view->m_payloadSize == size,
                    "short copy identity and size");
                for (uint32_t byte = 0; byte < size; ++byte)
                {
                    CompactCheck(view->m_payload[byte] == static_cast<uint8_t>(size + round), "short copy exact body");
                }
                submission.finish(NI_OK);
            }
            CompactCheck(submission.idle() && submission.queuedBytes() == 0, "short copy storage reusable");
        }
    }
    for (uint32_t producers = 1; producers <= 4; producers *= 4)
    {
        NetSenderSubmission queue;
        CompactCheck(queue.start(1, 101) == NI_OK && queue.ready(1, 128, 4096) == NI_OK, "inline concurrent setup");
        const uint32_t perProducer = producers == 1 ? 1000000 : 250000;
        std::atomic<bool> go(false), failed(false);
        std::vector<std::thread> threads;
        try
        {
            for (uint32_t producer = 0; producer < producers; ++producer)
            {
                threads.emplace_back([&, producer]()
                {
                    while (!go.load(std::memory_order_acquire)) { std::this_thread::yield(); }
                    NetInterfaceFrame sent = frame;
                    for (uint32_t index = 1; index <= perProducer && !failed.load(std::memory_order_relaxed); ++index)
                    {
                        sent.m_requestId = (static_cast<uint64_t>(producer + 1) << 48) | index;
                        for (;;)
                        {
                            const int32_t result = queue.data(sent);
                            if (result == NI_OK || failed.load(std::memory_order_relaxed)) { break; }
                            if (result != NI_LIMIT_EXCEEDED) { failed.store(true); break; }
                            std::this_thread::yield();
                        }
                    }
                });
            }
        }
        catch (...)
        {
            failed.store(true);
            go.store(true, std::memory_order_release);
            for (size_t index = 0; index < threads.size(); ++index) { threads[index].join(); }
            throw;
        }
        uint32_t received[4] = {};
        uint64_t total = 0;
        const auto started = std::chrono::steady_clock::now();
        go.store(true, std::memory_order_release);
        while (total < static_cast<uint64_t>(producers) * perProducer && !failed.load())
        {
            if (queue.beginPoll() != NI_OK) { failed.store(true); break; }
            uint32_t drained = 0;
            for (; drained < 512; ++drained)
            {
                NetSenderSubmissionCommand command = {};
                const int32_t result = queue.next(command);
                if (result == NI_NEED_MORE) { break; }
                const uint64_t source = command.m_frame.m_requestId >> 48;
                const uint64_t sequence = command.m_frame.m_requestId & UINT64_C(0x0000ffffffffffff);
                if (result != NI_READY || source == 0 || source > producers || sequence != received[source - 1] + 1 ||
                    command.m_frame.m_payloadSize != 6 || std::memcmp(command.m_frame.m_payload, "123456", 6) != 0)
                {
                    failed.store(true); break;
                }
                ++received[source - 1]; ++total;
                queue.finish(NI_OK);
            }
            if (std::chrono::steady_clock::now() - started > std::chrono::seconds(30)) { failed.store(true); }
            if (drained == 0) { std::this_thread::yield(); }
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started).count();
        for (size_t index = 0; index < threads.size(); ++index) { threads[index].join(); }
        CompactCheck(!failed.load() && queue.idle() && queue.queuedBytes() == 0, "inline concurrent FIFO/accounting/termination");
        std::printf("SUBMISSION_DATA producers=%u count=%llu elapsedUs=%lld messagesPerSecond=%.2f\n", producers,
            static_cast<unsigned long long>(total), static_cast<long long>(elapsed), static_cast<double>(total) * 1000000 / elapsed);
    }
}