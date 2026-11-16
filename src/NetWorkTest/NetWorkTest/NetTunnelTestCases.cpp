#include "NetTunnelTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetInterfaceTestAccess.h"
#include <algorithm>
#include <cstring>

#include "NetChannel/NetChannelAPI.h"
#include <cstdio>
#include <vector>

// 独立测试检查，不受NDEBUG影响；生产库不保留测试开关。
#define CHANNEL_CHECK(condition) \
    { \
        ++checks; \
        if (!(condition)) \
        { \
            std::fprintf(stderr, "NetChannel check failed at line %d: %s\n", __LINE__, #condition); \
            return 1; \
        } \
    }

/** 直接调用公开NetChannel校验黄金字节、任意切分、边界与信用
@return 全部检查成功返回0，否则返回1
*/
static int ChannelStorageAudit()
{
    uint32_t checks = 0;
    // 独立手写线格式，不以同一编码器生成解码预期
    const uint8_t golden[] = {0x4e, 0x54, 0x57, 0x33, 3, 3, 0, 0,
        1, 2, 3, 4, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0, 0, 0, 4, 0, 255, 127, 128};
    const uint8_t payload[] = {0, 255, 127, 128};
    const uint32_t wireSize = static_cast<uint32_t>(sizeof(golden));
    NetChannelRecord record = {};
    record.m_kind = NC_BYTES;
    record.m_product = 0x01020304U;
    record.m_channel = 0x1122334455667788ULL;
    record.m_data = payload;
    record.m_size = sizeof(payload);
    uint8_t output[128];
    uint32_t required = 0;
    std::memset(output, 0xa5, sizeof(output));
    CHANNEL_CHECK(NetChannel::encode(record, nullptr, 0, required) == NC_BUFFER_TOO_SMALL && required == wireSize);
    CHANNEL_CHECK(NetChannel::encode(record, output, wireSize - 1, required) == NC_BUFFER_TOO_SMALL);
    for (size_t index = 0; index < sizeof(output); ++index)
    {
        CHANNEL_CHECK(output[index] == 0xa5);
    }
    CHANNEL_CHECK(NetChannel::encode(record, output, sizeof(output), required) == NC_OK && required == wireSize);
    CHANNEL_CHECK(std::memcmp(output, golden, wireSize) == 0 && output[wireSize] == 0xa5);

    for (uint32_t split = 0; split <= wireSize; ++split)
    {
        NetChannel decoder;
        uint32_t consumed = 999;
        const int32_t first = decoder.feed(golden, split, consumed);
        CHANNEL_CHECK(consumed == split && first == (split == wireSize ? NC_READY : NC_NEED_MORE));
        CHANNEL_CHECK(decoder.feed(golden + split, wireSize - split, consumed) == NC_READY);
        CHANNEL_CHECK(consumed == wireSize - split);
        NetChannelRecord view = {};
        CHANNEL_CHECK(decoder.peek(view) == NC_READY);
        CHANNEL_CHECK(view.m_kind == NC_BYTES && view.m_product == 0x01020304U && view.m_channel == 0x1122334455667788ULL);
        CHANNEL_CHECK(view.m_size == sizeof(payload) && std::memcmp(view.m_data, payload, sizeof(payload)) == 0);
        CHANNEL_CHECK(decoder.feed(golden, wireSize, consumed) == NC_READY && consumed == 0);
        CHANNEL_CHECK(decoder.finish() == NC_READY && decoder.consume() == NC_OK);
        CHANNEL_CHECK(decoder.peek(view) == NC_NEED_MORE && view.m_data == nullptr && view.m_size == 0);
        CHANNEL_CHECK(decoder.finish() == NC_OK && decoder.consume() == NC_INVALID_STATE);
    }

    for (uint32_t cut = 1; cut < wireSize; ++cut)
    {
        NetChannel decoder;
        uint32_t consumed = 0;
        CHANNEL_CHECK(decoder.feed(golden, cut, consumed) == NC_NEED_MORE && consumed == cut);
        CHANNEL_CHECK(decoder.finish() == NC_TRUNCATED);
        CHANNEL_CHECK(decoder.feed(golden, wireSize, consumed) == NC_TRUNCATED && consumed == 0);
    }

    for (uint32_t field = 0; field < 9; ++field)
    {
        std::vector<uint8_t> invalid(golden, golden + wireSize);
        if (field < 4)
        {
            invalid[field] = 0;
        }
        else if (field < 8)
        {
            invalid[field] = field == 4 ? 2 : field == 5 ? 99 : 1;
        }
        else
        {
            std::fill(invalid.begin() + 12, invalid.begin() + 20, 0);
        }
        NetChannel decoder;
        uint32_t consumed = 0;
        CHANNEL_CHECK(decoder.feed(invalid.data(), wireSize, consumed) == NC_PROTOCOL_ERROR && consumed == 24);
        CHANNEL_CHECK(decoder.finish() == NC_PROTOCOL_ERROR && decoder.consume() == NC_PROTOCOL_ERROR);
    }

    const uint32_t invalidLengths[] = {16385U, 0xffffffffU};
    for (size_t index = 0; index < sizeof(invalidLengths) / sizeof(invalidLengths[0]); ++index)
    {
        std::vector<uint8_t> invalid(golden, golden + wireSize);
        const uint32_t length = invalidLengths[index];
        invalid[20] = static_cast<uint8_t>(length >> 24);
        invalid[21] = static_cast<uint8_t>(length >> 16);
        invalid[22] = static_cast<uint8_t>(length >> 8);
        invalid[23] = static_cast<uint8_t>(length);
        NetChannel decoder;
        uint32_t consumed = 0;
        CHANNEL_CHECK(decoder.feed(invalid.data(), wireSize, consumed) == NC_LIMIT_EXCEEDED && consumed == 24);
    }

    std::vector<uint8_t> joined(golden, golden + wireSize);
    joined.insert(joined.end(), golden, golden + wireSize);
    NetChannel joinedDecoder;
    uint32_t consumed = 0;
    CHANNEL_CHECK(joinedDecoder.feed(joined.data(), static_cast<uint32_t>(joined.size()), consumed) == NC_READY && consumed == wireSize);
    CHANNEL_CHECK(joinedDecoder.consume() == NC_OK);
    CHANNEL_CHECK(joinedDecoder.feed(joined.data() + wireSize, wireSize, consumed) == NC_READY && consumed == wireSize);

    NetChannel empty;
    CHANNEL_CHECK(empty.feed(nullptr, 1, consumed) == NC_INVALID_ARGUMENT && consumed == 0);
    CHANNEL_CHECK(empty.feed(nullptr, 0, consumed) == NC_NEED_MORE && consumed == 0 && empty.finish() == NC_OK);
    const uint8_t hello[] = {0x4e, 0x54, 0x57, 0x33, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    CHANNEL_CHECK(empty.feed(hello, sizeof(hello), consumed) == NC_READY && consumed == sizeof(hello));
    NetChannelRecord view = {};
    CHANNEL_CHECK(empty.peek(view) == NC_READY && view.m_data == nullptr && view.m_size == 0);
    CHANNEL_CHECK(NetChannel::encode(view, output, sizeof(output), required) == NC_OK && required == sizeof(hello));
    CHANNEL_CHECK(std::memcmp(output, hello, sizeof(hello)) == 0);

    for (uint32_t kind = NC_CONTROL; kind <= NC_CLOSE; ++kind)
    {
        std::vector<uint8_t> sample(golden, golden + wireSize);
        sample[5] = static_cast<uint8_t>(kind);
        if (kind == NC_CONTROL)
        {
            std::fill(sample.begin() + 8, sample.begin() + 20, 0);
        }
        NetChannel decoder;
        for (uint32_t index = 0; index < wireSize; ++index)
        {
            CHANNEL_CHECK(decoder.feed(sample.data() + index, 1, consumed) == (index + 1 == wireSize ? NC_READY : NC_NEED_MORE));
            CHANNEL_CHECK(consumed == 1);
        }
        CHANNEL_CHECK(decoder.peek(view) == NC_READY && view.m_kind == kind);
        CHANNEL_CHECK(NetChannel::encode(view, output, sizeof(output), required) == NC_OK);
        CHANNEL_CHECK(required == wireSize && std::memcmp(output, sample.data(), wireSize) == 0);
    }

    const uint32_t wrongKinds[] = {0, NC_HELLO, NC_CONTROL, NC_CLOSE, NC_BYTES};
    for (size_t index = 0; index < sizeof(wrongKinds) / sizeof(wrongKinds[0]); ++index)
    {
        NetChannelRecord invalid = record;
        invalid.m_kind = wrongKinds[index];
        if (invalid.m_kind == NC_CLOSE)
        {
            invalid.m_size = 3;
        }
        if (invalid.m_kind == NC_BYTES)
        {
            invalid.m_data = nullptr;
        }
        std::memset(output, 0xa5, sizeof(output));
        CHANNEL_CHECK(NetChannel::encode(invalid, output, sizeof(output), required) == NC_INVALID_ARGUMENT && required == 0);
        for (size_t offset = 0; offset < sizeof(output); ++offset)
        {
            CHANNEL_CHECK(output[offset] == 0xa5);
        }
    }

    std::vector<uint8_t> large(NET_CHANNEL_PAYLOAD_LIMIT);
    for (size_t index = 0; index < large.size(); ++index)
    {
        large[index] = static_cast<uint8_t>(index * 37);
    }
    record.m_data = large.data();
    record.m_size = static_cast<uint32_t>(large.size());
    std::vector<uint8_t> largeWire(NET_CHANNEL_HEADER_SIZE + large.size());
    CHANNEL_CHECK(NetChannel::encode(record, largeWire.data(), static_cast<uint32_t>(largeWire.size()), required) == NC_OK);
    CHANNEL_CHECK(largeWire[20] == 0 && largeWire[21] == 0 && largeWire[22] == 64 && largeWire[23] == 0);
    NetChannel largeDecoder;
    for (uint32_t offset = 0; offset < largeWire.size();)
    {
        const uint32_t part = (std::min)(257U, static_cast<uint32_t>(largeWire.size()) - offset);
        const int32_t result = largeDecoder.feed(largeWire.data() + offset, part, consumed);
        CHANNEL_CHECK(consumed == part);
        offset += consumed;
        CHANNEL_CHECK(result == (offset == largeWire.size() ? NC_READY : NC_NEED_MORE));
    }
    CHANNEL_CHECK(largeDecoder.peek(view) == NC_READY && view.m_size == large.size());
    CHANNEL_CHECK(std::memcmp(view.m_data, large.data(), large.size()) == 0);

    record.m_data = payload;
    record.m_size = sizeof(payload);
    std::memset(output, 0xa5, sizeof(output));
    record.m_data = output + 30;
    CHANNEL_CHECK(NetChannel::encode(record, output, sizeof(output), required) == NC_OK);
    record.m_data = output + 4;
    CHANNEL_CHECK(NetChannel::encode(record, output, sizeof(output), required) == NC_INVALID_ARGUMENT && required == 0);
    record.m_data = reinterpret_cast<const uint8_t*>(UINTPTR_MAX - 1);
    CHANNEL_CHECK(NetChannel::encode(record, output, sizeof(output), required) == NC_INVALID_ARGUMENT && required == 0);
    record.m_data = payload;
    CHANNEL_CHECK(NetChannel::encode(record, reinterpret_cast<uint8_t*>(UINTPTR_MAX - 1), 128, required) == NC_INVALID_ARGUMENT);
    CHANNEL_CHECK(NetChannel::encode(record, nullptr, 128, required) == NC_INVALID_ARGUMENT && required == 0);
    std::vector<NetChannelRecord> aliased(3);
    aliased[0] = record;
    CHANNEL_CHECK(NetChannel::encode(aliased[0], reinterpret_cast<uint8_t*>(aliased.data()),
        static_cast<uint32_t>(aliased.size() * sizeof(NetChannelRecord)), required) == NC_OK);
    CHANNEL_CHECK(std::memcmp(aliased.data(), golden, wireSize) == 0);

    for (uint32_t capacity = NET_CHANNEL_DEFAULT_WINDOW; capacity <= NET_CHANNEL_MAX_WINDOW; capacity += NET_CHANNEL_PAYLOAD_LIMIT)
    {
        NetChannelCredit credit;
        CHANNEL_CHECK(credit.configure(capacity) == NC_OK && credit.capacity() == capacity && credit.available() == capacity);
        CHANNEL_CHECK(credit.sent(0) == NC_INVALID_ARGUMENT && credit.available() == capacity);
        CHANNEL_CHECK(credit.sent(16385) == NC_INVALID_ARGUMENT && credit.available() == capacity);
        for (uint32_t offset = 0; offset < capacity; offset += NET_CHANNEL_PAYLOAD_LIMIT)
        {
            CHANNEL_CHECK(credit.sent(NET_CHANNEL_PAYLOAD_LIMIT) == NC_OK);
            CHANNEL_CHECK(credit.received(NET_CHANNEL_PAYLOAD_LIMIT) == NC_OK);
        }
        CHANNEL_CHECK(credit.available() == 0 && credit.pending() == capacity);
        CHANNEL_CHECK(credit.sent(1) == NC_LIMIT_EXCEEDED && credit.status() == NC_OK);
        CHANNEL_CHECK(credit.consume(0) == NC_INVALID_ARGUMENT && credit.pending() == capacity);
        CHANNEL_CHECK(credit.consume(capacity) == NC_OK && credit.pending() == 0);
        CHANNEL_CHECK(credit.grant(capacity) == NC_OK && credit.available() == capacity);
        CHANNEL_CHECK(credit.configure(capacity) == NC_INVALID_STATE);
        CHANNEL_CHECK(credit.grant(1) == NC_PROTOCOL_ERROR && credit.available() == 0);
        CHANNEL_CHECK(credit.sent(1) == NC_PROTOCOL_ERROR && credit.grant(1) == NC_PROTOCOL_ERROR);
    }
    const uint32_t wrongWindows[] = {0, 65535, 65537, 1048577, 0xffffffffU};
    for (size_t index = 0; index < sizeof(wrongWindows) / sizeof(wrongWindows[0]); ++index)
    {
        NetChannelCredit credit;
        CHANNEL_CHECK(credit.configure(wrongWindows[index]) == NC_INVALID_ARGUMENT);
        CHANNEL_CHECK(credit.capacity() == 65536 && credit.available() == 65536 && credit.status() == NC_OK);
    }
    NetChannelCredit received;
    CHANNEL_CHECK(received.received(0) == NC_PROTOCOL_ERROR && received.pending() == 0);
    CHANNEL_CHECK(received.received(1) == NC_PROTOCOL_ERROR && received.consume(1) == NC_PROTOCOL_ERROR);
    NetChannelCredit overflow;
    for (uint32_t index = 0; index < 4; ++index)
    {
        CHANNEL_CHECK(overflow.received(16384) == NC_OK);
    }
    CHANNEL_CHECK(overflow.received(1) == NC_PROTOCOL_ERROR && overflow.pending() == 0);
    NetChannelCredit zeroGrant;
    CHANNEL_CHECK(zeroGrant.grant(0) == NC_PROTOCOL_ERROR && zeroGrant.status() == NC_PROTOCOL_ERROR);
    NetChannelCredit wrongConsume;
    CHANNEL_CHECK(wrongConsume.consume(1) == NC_INVALID_ARGUMENT && wrongConsume.status() == NC_OK);
    CHANNEL_CHECK(wrongConsume.received(16385) == NC_PROTOCOL_ERROR);
    NetChannel invalidAddress;
    CHANNEL_CHECK(invalidAddress.feed(reinterpret_cast<const uint8_t*>(UINTPTR_MAX - 1), 4, consumed) == NC_INVALID_ARGUMENT && consumed == 0);
    CHANNEL_CHECK(invalidAddress.finish() == NC_OK);
    CHANNEL_CHECK(invalidAddress.feed(golden, wireSize, consumed) == NC_READY && consumed == wireSize);
    CHANNEL_CHECK(invalidAddress.consume() == NC_OK);

    // 同一解码器交替接收大、小、空记录；原字节全部校验，不能读出上条残留
    NetChannel reuse;
    for (uint32_t round = 0; round < 64; ++round)
    {
        const uint8_t* bytes = round % 3 == 0 ? largeWire.data() : round % 3 == 1 ? golden : hello;
        const uint32_t length = round % 3 == 0 ? static_cast<uint32_t>(largeWire.size()) : round % 3 == 1 ? wireSize : sizeof(hello);
        uint32_t offset = 0;
        while (offset < length)
        {
            const uint32_t count = (std::min)(1U + ((round * 17U + offset) % 997U), length - offset);
            const int32_t status = reuse.feed(bytes + offset, count, consumed);
            CHANNEL_CHECK(consumed == count && status == (offset + count == length ? NC_READY : NC_NEED_MORE));
            offset += count;
        }
        CHANNEL_CHECK(reuse.peek(view) == NC_READY && view.m_size == length - 24);
        CHANNEL_CHECK(view.m_size == 0 ? view.m_data == nullptr : std::memcmp(view.m_data, bytes + 24, view.m_size) == 0);
        CHANNEL_CHECK(reuse.finish() == NC_READY && reuse.consume() == NC_OK && reuse.finish() == NC_OK);
    }

    std::printf("NetChannel unit PASS: %u assertions, golden/fragment/EOF/bounds/alias/window/forged-credit\n", checks);
    return 0;
}

#undef CHANNEL_CHECK

void NetTunnelTestCases::portableRecords()
{
    NetInterfaceTlsTestPair::require(ChannelStorageAudit() == 0, "NetChannel complete direct contract audit");
    const uint8_t cipher[] = {0x16, 0x03, 0x03, 0x00, 0xff, 0x10, 0x42};
    const uint32_t fields[] = {NI_TUNNEL_BYTES, 7, 0x76543210U, 0xfedcba98U, sizeof(cipher)};
    uint8_t wire[31] = {};
    uint32_t size = 0;
    NetInterfaceTlsTestPair::require(NetInterface::tunnelEncode(fields, cipher, nullptr, 0, &size) == NI_BUFFER_TOO_SMALL &&
        size == sizeof(wire), "route envelope size probe failed");
    NetInterfaceTlsTestPair::require(NetInterface::tunnelEncode(fields, cipher, wire, sizeof(wire), &size) == NI_OK,
        "encode route envelope failed");
    const uint8_t fixed[] = {'N','T','W','3',3,3,0,0,0,0,0,7,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10,0,0,0,7};
    NetInterfaceTlsTestPair::require(std::memcmp(wire, fixed, sizeof(fixed)) == 0 &&
        std::memcmp(wire + sizeof(fixed), cipher, sizeof(cipher)) == 0, "outer route changed opaque TLS bytes or wire vector");
    for (uint32_t fragment = 1; fragment <= sizeof(wire); ++fragment)
    {
        NetInterfaceTunnelHandle decoder = nullptr;
        NetInterfaceTlsTestPair::require(NetInterface::tunnelCreate(&decoder) == NI_OK, "create route decoder failed");
        uint32_t offset = 0;
        int32_t result = NI_NEED_MORE;
        while (offset < sizeof(wire))
        {
            uint32_t consumed = 0;
            const uint32_t count = (std::min)(fragment, static_cast<uint32_t>(sizeof(wire)) - offset);
            result = NetInterface::tunnelFeed(decoder, wire + offset, count, &consumed);
            NetInterfaceTlsTestPair::require(consumed == count && result >= 0, "fragmented route record rejected");
            offset += consumed;
        }
        uint32_t actual[5] = {};
        const uint8_t* payload = nullptr;
        NetInterfaceTlsTestPair::require(result == NI_READY && NetInterface::tunnelPeek(decoder, actual, &payload) == NI_READY &&
            std::memcmp(fields, actual, sizeof(fields)) == 0 && std::memcmp(cipher, payload, sizeof(cipher)) == 0,
            "route record corrupted the 64-bit channel or TLS bytes");
        uint32_t consumed = 99;
        NetInterfaceTlsTestPair::require(NetInterface::tunnelFeed(decoder, wire, sizeof(wire), &consumed) == NI_READY && consumed == 0,
            "unconsumed route record lost backpressure");
        NetInterfaceTlsTestPair::require(NetInterface::tunnelConsume(decoder) == NI_OK && NetInterface::tunnelFinish(decoder) == NI_OK,
            "route record completion failed");
        NetInterfaceTlsTestPair::require(NetInterface::tunnelFeed(decoder, wire, sizeof(wire) - 1, &consumed) == NI_NEED_MORE &&
            NetInterface::tunnelFinish(decoder) == NI_TRUNCATED, "truncated route record accepted");
        NetInterface::tunnelDestroy(decoder);
    }
    for (uint32_t variant = 0; variant < 4; ++variant)
    {
        uint8_t invalid[24] = {};
        std::memcpy(invalid, wire, sizeof(invalid));
        if (variant == 0) { invalid[4] = 2; }
        if (variant == 1) { invalid[6] = 1; }
        if (variant == 2) { std::memset(invalid + 12, 0, 8); }
        if (variant == 3) { invalid[20] = 0xff; }
        NetInterfaceTunnel decoder;
        uint32_t consumed = 0;
        NetInterfaceTlsTestPair::require(decoder.feed(invalid, sizeof(invalid), consumed) < 0 && consumed == sizeof(invalid),
            "invalid version, flags, channel or oversized body accepted");
    }
    uint8_t sentinel[31];
    std::memset(sentinel, 0xab, sizeof(sentinel));
    NetInterfaceTlsTestPair::require(NetInterface::tunnelEncode(fields, cipher, sentinel, sizeof(sentinel) - 1, &size) ==
        NI_BUFFER_TOO_SMALL && sentinel[0] == 0xab && sentinel[30] == 0xab, "size probe wrote into caller buffer");
    NetInterfaceTlsTestPair::require(NetInterface::tunnelEncode(fields, sentinel + 24, sentinel, sizeof(sentinel), &size) ==
        NI_INVALID_ARGUMENT && sentinel[0] == 0xab, "overlapping envelope memory was accepted");
}