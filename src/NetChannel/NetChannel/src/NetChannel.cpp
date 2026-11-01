#include "NetChannel.h"
#include <algorithm>
#include <cstring>
#include <new>

NetChannel::NetChannel() :
m_headerBytes(0),
m_bodyBytes(0),
m_storage(nullptr),
m_capacity(0),
m_state(NC_NEED_MORE),
m_record()
{

}

NetChannel::~NetChannel()
{
    delete[] m_storage;
}

bool NetChannel::valid(const NetChannelRecord& record)
{
    if (record.m_size > NET_CHANNEL_PAYLOAD_LIMIT)
    {
        return false;
    }
    switch (record.m_kind)
    {
    case NC_HELLO:
    {
        return record.m_product == 0 && record.m_channel == 0 && record.m_size == 0;
    }
    case NC_CONTROL:
    {
        return record.m_product == 0 && record.m_channel == 0 && record.m_size != 0;
    }
    case NC_BYTES:
    {
        return record.m_channel != 0 && record.m_size != 0;
    }
    case NC_CLOSE:
    {
        return record.m_channel != 0 && record.m_size == 4;
    }
    default:
    {
        return false;
    }
    }
}

uint64_t NetChannel::readNumber(const uint8_t* bytes, uint32_t width)
{
    uint64_t value = 0;
    for (uint32_t index = 0; index < width; ++index)
    {
        value = (value << 8) | bytes[index];
    }
    return value;
}

void NetChannel::writeNumber(uint8_t* bytes, uint64_t value, uint32_t width)
{
    for (uint32_t index = width; index != 0; --index)
    {
        bytes[index - 1] = static_cast<uint8_t>(value);
        value >>= 8;
    }
}

int32_t NetChannel::acceptHeader()
{
    if (std::memcmp(m_header, "NTW3", 4) != 0 || m_header[4] != 3 || m_header[6] != 0 || m_header[7] != 0)
    {
        return NC_PROTOCOL_ERROR;
    }
    m_record.m_kind = m_header[5];
    m_record.m_product = static_cast<uint32_t>(readNumber(m_header + 8, 4));
    m_record.m_channel = readNumber(m_header + 12, 8);
    m_record.m_size = static_cast<uint32_t>(readNumber(m_header + 20, 4));
    if (m_record.m_size > NET_CHANNEL_PAYLOAD_LIMIT)
    {
        return NC_LIMIT_EXCEEDED;
    }
    if (!valid(m_record))
    {
        return NC_PROTOCOL_ERROR;
    }
    if (m_record.m_size > m_capacity)
    {
        // 此时上一记录已consume且新正文尚未接纳，可先释放再扩容，峰值也不超硬界
        uint32_t capacity = 256;
        while (capacity < m_record.m_size)
        {
            capacity *= 2;
        }
        delete[] m_storage;
        m_storage = nullptr;
        m_capacity = 0;
        m_storage = new (std::nothrow) uint8_t[capacity];
        if (m_storage == nullptr)
        {
            return NC_NO_MEMORY;
        }
        m_capacity = capacity;
    }
    return NC_NEED_MORE;
}

int32_t NetChannel::feed(const uint8_t* data, uint32_t size, uint32_t& consumed)
{
    consumed = 0;
    if (size != 0 && (data == nullptr || reinterpret_cast<uintptr_t>(data) > UINTPTR_MAX - size))
    {
        return NC_INVALID_ARGUMENT;
    }
    if (m_state != NC_NEED_MORE)
    {
        return m_state;
    }
    if (m_headerBytes != NET_CHANNEL_HEADER_SIZE)
    {
        const uint32_t count = (std::min)(size, NET_CHANNEL_HEADER_SIZE - m_headerBytes);
        if (count != 0)
        {
            std::memcpy(m_header + m_headerBytes, data, count);
            m_headerBytes += count;
            consumed = count;
        }
        if (m_headerBytes != NET_CHANNEL_HEADER_SIZE)
        {
            return NC_NEED_MORE;
        }
        m_state = acceptHeader();
        if (m_state < 0)
        {
            return m_state;
        }
    }
    const uint32_t count = (std::min)(size - consumed, m_record.m_size - m_bodyBytes);
    if (count != 0)
    {
        std::memcpy(m_storage + m_bodyBytes, data + consumed, count);
        m_bodyBytes += count;
        consumed += count;
    }
    if (m_bodyBytes == m_record.m_size)
    {
        m_record.m_data = m_bodyBytes == 0 ? nullptr : m_storage;
        m_state = NC_READY;
    }
    return m_state;
}

int32_t NetChannel::peek(NetChannelRecord& record) const
{
    record = m_state == NC_READY ? m_record : NetChannelRecord();
    return m_state;
}

int32_t NetChannel::consume()
{
    if (m_state != NC_READY)
    {
        return m_state < 0 ? m_state : NC_INVALID_STATE;
    }
    m_record = NetChannelRecord();
    m_headerBytes = 0;
    m_bodyBytes = 0;
    m_state = NC_NEED_MORE;
    return NC_OK;
}

int32_t NetChannel::finish()
{
    if (m_state != NC_NEED_MORE)
    {
        return m_state;
    }
    if (m_headerBytes == 0)
    {
        return NC_OK;
    }
    m_state = NC_TRUNCATED;
    return m_state;
}

int32_t NetChannel::encode(const NetChannelRecord& record, uint8_t* output, uint32_t capacity, uint32_t& required)
{
    // 输出可以覆盖调用方的record结构；先固定元数据，正文仍不得与输出重叠
    const NetChannelRecord input = record;
    required = 0;
    if (!valid(input) || (input.m_size != 0 && (input.m_data == nullptr ||
        reinterpret_cast<uintptr_t>(input.m_data) > UINTPTR_MAX - input.m_size)))
    {
        return NC_INVALID_ARGUMENT;
    }
    required = NET_CHANNEL_HEADER_SIZE + input.m_size;
    if (capacity < required)
    {
        return NC_BUFFER_TOO_SMALL;
    }
    const uintptr_t destination = reinterpret_cast<uintptr_t>(output);
    const uintptr_t source = reinterpret_cast<uintptr_t>(input.m_data);
    if (output == nullptr || destination > UINTPTR_MAX - required ||
        (input.m_size != 0 && source < destination + required && destination < source + input.m_size))
    {
        required = 0;
        return NC_INVALID_ARGUMENT;
    }
    std::memcpy(output, "NTW3", 4);
    output[4] = 3;
    output[5] = static_cast<uint8_t>(input.m_kind);
    output[6] = 0;
    output[7] = 0;
    writeNumber(output + 8, input.m_product, 4);
    writeNumber(output + 12, input.m_channel, 8);
    writeNumber(output + 20, input.m_size, 4);
    if (input.m_size != 0)
    {
        std::memcpy(output + NET_CHANNEL_HEADER_SIZE, input.m_data, input.m_size);
    }
    return NC_OK;
}

// NET_CHANNEL_UNIT_TEST_BEGIN
//#include "NetChannelCredit.h"
//#include <cstdio>
//#include <vector>
//
//// 测试时放开本段逐行注释，完成后全部注释回去；检查不受NDEBUG影响
//#define CHANNEL_CHECK(condition) \
//    { \
//        ++checks; \
//        if (!(condition)) \
//        { \
//            std::fprintf(stderr, "NetChannel check failed at line %d: %s\n", __LINE__, #condition); \
//            return 1; \
//        } \
//    }
//
//int main()
//{
//    uint32_t checks = 0;
//    // 独立手写线格式，不以同一编码器生成解码预期
//    const uint8_t golden[] = {0x4e, 0x54, 0x57, 0x33, 3, 3, 0, 0,
//        1, 2, 3, 4, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0, 0, 0, 4, 0, 255, 127, 128};
//    const uint8_t payload[] = {0, 255, 127, 128};
//    const uint32_t wireSize = static_cast<uint32_t>(sizeof(golden));
//    NetChannelRecord record = {};
//    record.m_kind = NC_BYTES;
//    record.m_product = 0x01020304U;
//    record.m_channel = 0x1122334455667788ULL;
//    record.m_data = payload;
//    record.m_size = sizeof(payload);
//    uint8_t output[128];
//    uint32_t required = 0;
//    std::memset(output, 0xa5, sizeof(output));
//    CHANNEL_CHECK(NetChannel::encode(record, nullptr, 0, required) == NC_BUFFER_TOO_SMALL && required == wireSize);
//    CHANNEL_CHECK(NetChannel::encode(record, output, wireSize - 1, required) == NC_BUFFER_TOO_SMALL);
//    for (size_t index = 0; index < sizeof(output); ++index)
//    {
//        CHANNEL_CHECK(output[index] == 0xa5);
//    }
//    CHANNEL_CHECK(NetChannel::encode(record, output, sizeof(output), required) == NC_OK && required == wireSize);
//    CHANNEL_CHECK(std::memcmp(output, golden, wireSize) == 0 && output[wireSize] == 0xa5);
//
//    for (uint32_t split = 0; split <= wireSize; ++split)
//    {
//        NetChannel decoder;
//        uint32_t consumed = 999;
//        const int32_t first = decoder.feed(golden, split, consumed);
//        CHANNEL_CHECK(consumed == split && first == (split == wireSize ? NC_READY : NC_NEED_MORE));
//        CHANNEL_CHECK(decoder.feed(golden + split, wireSize - split, consumed) == NC_READY);
//        CHANNEL_CHECK(consumed == wireSize - split);
//        NetChannelRecord view = {};
//        CHANNEL_CHECK(decoder.peek(view) == NC_READY);
//        CHANNEL_CHECK(view.m_kind == NC_BYTES && view.m_product == 0x01020304U && view.m_channel == 0x1122334455667788ULL);
//        CHANNEL_CHECK(view.m_size == sizeof(payload) && std::memcmp(view.m_data, payload, sizeof(payload)) == 0);
//        CHANNEL_CHECK(decoder.feed(golden, wireSize, consumed) == NC_READY && consumed == 0);
//        CHANNEL_CHECK(decoder.finish() == NC_READY && decoder.consume() == NC_OK);
//        CHANNEL_CHECK(decoder.peek(view) == NC_NEED_MORE && view.m_data == nullptr && view.m_size == 0);
//        CHANNEL_CHECK(decoder.finish() == NC_OK && decoder.consume() == NC_INVALID_STATE);
//    }
//
//    for (uint32_t cut = 1; cut < wireSize; ++cut)
//    {
//        NetChannel decoder;
//        uint32_t consumed = 0;
//        CHANNEL_CHECK(decoder.feed(golden, cut, consumed) == NC_NEED_MORE && consumed == cut);
//        CHANNEL_CHECK(decoder.finish() == NC_TRUNCATED);
//        CHANNEL_CHECK(decoder.feed(golden, wireSize, consumed) == NC_TRUNCATED && consumed == 0);
//    }
//
//    for (uint32_t field = 0; field < 9; ++field)
//    {
//        std::vector<uint8_t> invalid(golden, golden + wireSize);
//        if (field < 4)
//        {
//            invalid[field] = 0;
//        }
//        else if (field < 8)
//        {
//            invalid[field] = field == 4 ? 2 : field == 5 ? 99 : 1;
//        }
//        else
//        {
//            std::fill(invalid.begin() + 12, invalid.begin() + 20, 0);
//        }
//        NetChannel decoder;
//        uint32_t consumed = 0;
//        CHANNEL_CHECK(decoder.feed(invalid.data(), wireSize, consumed) == NC_PROTOCOL_ERROR && consumed == 24);
//        CHANNEL_CHECK(decoder.finish() == NC_PROTOCOL_ERROR && decoder.consume() == NC_PROTOCOL_ERROR);
//    }
//
//    const uint32_t invalidLengths[] = {16385U, 0xffffffffU};
//    for (size_t index = 0; index < sizeof(invalidLengths) / sizeof(invalidLengths[0]); ++index)
//    {
//        std::vector<uint8_t> invalid(golden, golden + wireSize);
//        const uint32_t length = invalidLengths[index];
//        invalid[20] = static_cast<uint8_t>(length >> 24);
//        invalid[21] = static_cast<uint8_t>(length >> 16);
//        invalid[22] = static_cast<uint8_t>(length >> 8);
//        invalid[23] = static_cast<uint8_t>(length);
//        NetChannel decoder;
//        uint32_t consumed = 0;
//        CHANNEL_CHECK(decoder.feed(invalid.data(), wireSize, consumed) == NC_LIMIT_EXCEEDED && consumed == 24);
//    }
//
//    std::vector<uint8_t> joined(golden, golden + wireSize);
//    joined.insert(joined.end(), golden, golden + wireSize);
//    NetChannel joinedDecoder;
//    uint32_t consumed = 0;
//    CHANNEL_CHECK(joinedDecoder.feed(joined.data(), static_cast<uint32_t>(joined.size()), consumed) == NC_READY && consumed == wireSize);
//    CHANNEL_CHECK(joinedDecoder.consume() == NC_OK);
//    CHANNEL_CHECK(joinedDecoder.feed(joined.data() + wireSize, wireSize, consumed) == NC_READY && consumed == wireSize);
//
//    NetChannel empty;
//    CHANNEL_CHECK(empty.feed(nullptr, 1, consumed) == NC_INVALID_ARGUMENT && consumed == 0);
//    CHANNEL_CHECK(empty.feed(nullptr, 0, consumed) == NC_NEED_MORE && consumed == 0 && empty.finish() == NC_OK);
//    const uint8_t hello[] = {0x4e, 0x54, 0x57, 0x33, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
//    CHANNEL_CHECK(empty.feed(hello, sizeof(hello), consumed) == NC_READY && consumed == sizeof(hello));
//    NetChannelRecord view = {};
//    CHANNEL_CHECK(empty.peek(view) == NC_READY && view.m_data == nullptr && view.m_size == 0);
//    CHANNEL_CHECK(NetChannel::encode(view, output, sizeof(output), required) == NC_OK && required == sizeof(hello));
//    CHANNEL_CHECK(std::memcmp(output, hello, sizeof(hello)) == 0);
//
//    for (uint32_t kind = NC_CONTROL; kind <= NC_CLOSE; ++kind)
//    {
//        std::vector<uint8_t> sample(golden, golden + wireSize);
//        sample[5] = static_cast<uint8_t>(kind);
//        if (kind == NC_CONTROL)
//        {
//            std::fill(sample.begin() + 8, sample.begin() + 20, 0);
//        }
//        NetChannel decoder;
//        for (uint32_t index = 0; index < wireSize; ++index)
//        {
//            CHANNEL_CHECK(decoder.feed(sample.data() + index, 1, consumed) == (index + 1 == wireSize ? NC_READY : NC_NEED_MORE));
//            CHANNEL_CHECK(consumed == 1);
//        }
//        CHANNEL_CHECK(decoder.peek(view) == NC_READY && view.m_kind == kind);
//        CHANNEL_CHECK(NetChannel::encode(view, output, sizeof(output), required) == NC_OK);
//        CHANNEL_CHECK(required == wireSize && std::memcmp(output, sample.data(), wireSize) == 0);
//    }
//
//    const uint32_t wrongKinds[] = {0, NC_HELLO, NC_CONTROL, NC_CLOSE, NC_BYTES};
//    for (size_t index = 0; index < sizeof(wrongKinds) / sizeof(wrongKinds[0]); ++index)
//    {
//        NetChannelRecord invalid = record;
//        invalid.m_kind = wrongKinds[index];
//        if (invalid.m_kind == NC_CLOSE)
//        {
//            invalid.m_size = 3;
//        }
//        if (invalid.m_kind == NC_BYTES)
//        {
//            invalid.m_data = nullptr;
//        }
//        std::memset(output, 0xa5, sizeof(output));
//        CHANNEL_CHECK(NetChannel::encode(invalid, output, sizeof(output), required) == NC_INVALID_ARGUMENT && required == 0);
//        for (size_t offset = 0; offset < sizeof(output); ++offset)
//        {
//            CHANNEL_CHECK(output[offset] == 0xa5);
//        }
//    }
//
//    std::vector<uint8_t> large(NET_CHANNEL_PAYLOAD_LIMIT);
//    for (size_t index = 0; index < large.size(); ++index)
//    {
//        large[index] = static_cast<uint8_t>(index * 37);
//    }
//    record.m_data = large.data();
//    record.m_size = static_cast<uint32_t>(large.size());
//    std::vector<uint8_t> largeWire(NET_CHANNEL_HEADER_SIZE + large.size());
//    CHANNEL_CHECK(NetChannel::encode(record, largeWire.data(), static_cast<uint32_t>(largeWire.size()), required) == NC_OK);
//    CHANNEL_CHECK(largeWire[20] == 0 && largeWire[21] == 0 && largeWire[22] == 64 && largeWire[23] == 0);
//    NetChannel largeDecoder;
//    for (uint32_t offset = 0; offset < largeWire.size();)
//    {
//        const uint32_t part = (std::min)(257U, static_cast<uint32_t>(largeWire.size()) - offset);
//        const int32_t result = largeDecoder.feed(largeWire.data() + offset, part, consumed);
//        CHANNEL_CHECK(consumed == part);
//        offset += consumed;
//        CHANNEL_CHECK(result == (offset == largeWire.size() ? NC_READY : NC_NEED_MORE));
//    }
//    CHANNEL_CHECK(largeDecoder.peek(view) == NC_READY && view.m_size == large.size());
//    CHANNEL_CHECK(std::memcmp(view.m_data, large.data(), large.size()) == 0);
//
//    record.m_data = payload;
//    record.m_size = sizeof(payload);
//    std::memset(output, 0xa5, sizeof(output));
//    record.m_data = output + 30;
//    CHANNEL_CHECK(NetChannel::encode(record, output, sizeof(output), required) == NC_OK);
//    record.m_data = output + 4;
//    CHANNEL_CHECK(NetChannel::encode(record, output, sizeof(output), required) == NC_INVALID_ARGUMENT && required == 0);
//    record.m_data = reinterpret_cast<const uint8_t*>(UINTPTR_MAX - 1);
//    CHANNEL_CHECK(NetChannel::encode(record, output, sizeof(output), required) == NC_INVALID_ARGUMENT && required == 0);
//    record.m_data = payload;
//    CHANNEL_CHECK(NetChannel::encode(record, reinterpret_cast<uint8_t*>(UINTPTR_MAX - 1), 128, required) == NC_INVALID_ARGUMENT);
//    CHANNEL_CHECK(NetChannel::encode(record, nullptr, 128, required) == NC_INVALID_ARGUMENT && required == 0);
//    std::vector<NetChannelRecord> aliased(3);
//    aliased[0] = record;
//    CHANNEL_CHECK(NetChannel::encode(aliased[0], reinterpret_cast<uint8_t*>(aliased.data()),
//        static_cast<uint32_t>(aliased.size() * sizeof(NetChannelRecord)), required) == NC_OK);
//    CHANNEL_CHECK(std::memcmp(aliased.data(), golden, wireSize) == 0);
//
//    for (uint32_t capacity = NET_CHANNEL_DEFAULT_WINDOW; capacity <= NET_CHANNEL_MAX_WINDOW; capacity += NET_CHANNEL_PAYLOAD_LIMIT)
//    {
//        NetChannelCredit credit;
//        CHANNEL_CHECK(credit.configure(capacity) == NC_OK && credit.capacity() == capacity && credit.available() == capacity);
//        CHANNEL_CHECK(credit.sent(0) == NC_INVALID_ARGUMENT && credit.available() == capacity);
//        CHANNEL_CHECK(credit.sent(16385) == NC_INVALID_ARGUMENT && credit.available() == capacity);
//        for (uint32_t offset = 0; offset < capacity; offset += NET_CHANNEL_PAYLOAD_LIMIT)
//        {
//            CHANNEL_CHECK(credit.sent(NET_CHANNEL_PAYLOAD_LIMIT) == NC_OK);
//            CHANNEL_CHECK(credit.received(NET_CHANNEL_PAYLOAD_LIMIT) == NC_OK);
//        }
//        CHANNEL_CHECK(credit.available() == 0 && credit.pending() == capacity);
//        CHANNEL_CHECK(credit.sent(1) == NC_LIMIT_EXCEEDED && credit.status() == NC_OK);
//        CHANNEL_CHECK(credit.consume(0) == NC_INVALID_ARGUMENT && credit.pending() == capacity);
//        CHANNEL_CHECK(credit.consume(capacity) == NC_OK && credit.pending() == 0);
//        CHANNEL_CHECK(credit.grant(capacity) == NC_OK && credit.available() == capacity);
//        CHANNEL_CHECK(credit.configure(capacity) == NC_INVALID_STATE);
//        CHANNEL_CHECK(credit.grant(1) == NC_PROTOCOL_ERROR && credit.available() == 0);
//        CHANNEL_CHECK(credit.sent(1) == NC_PROTOCOL_ERROR && credit.grant(1) == NC_PROTOCOL_ERROR);
//    }
//    const uint32_t wrongWindows[] = {0, 65535, 65537, 1048577, 0xffffffffU};
//    for (size_t index = 0; index < sizeof(wrongWindows) / sizeof(wrongWindows[0]); ++index)
//    {
//        NetChannelCredit credit;
//        CHANNEL_CHECK(credit.configure(wrongWindows[index]) == NC_INVALID_ARGUMENT);
//        CHANNEL_CHECK(credit.capacity() == 65536 && credit.available() == 65536 && credit.status() == NC_OK);
//    }
//    NetChannelCredit received;
//    CHANNEL_CHECK(received.received(0) == NC_PROTOCOL_ERROR && received.pending() == 0);
//    CHANNEL_CHECK(received.received(1) == NC_PROTOCOL_ERROR && received.consume(1) == NC_PROTOCOL_ERROR);
//    NetChannelCredit overflow;
//    for (uint32_t index = 0; index < 4; ++index)
//    {
//        CHANNEL_CHECK(overflow.received(16384) == NC_OK);
//    }
//    CHANNEL_CHECK(overflow.received(1) == NC_PROTOCOL_ERROR && overflow.pending() == 0);
//    NetChannelCredit zeroGrant;
//    CHANNEL_CHECK(zeroGrant.grant(0) == NC_PROTOCOL_ERROR && zeroGrant.status() == NC_PROTOCOL_ERROR);
//    NetChannelCredit wrongConsume;
//    CHANNEL_CHECK(wrongConsume.consume(1) == NC_INVALID_ARGUMENT && wrongConsume.status() == NC_OK);
//    CHANNEL_CHECK(wrongConsume.received(16385) == NC_PROTOCOL_ERROR);
//    NetChannel invalidAddress;
//    CHANNEL_CHECK(invalidAddress.feed(reinterpret_cast<const uint8_t*>(UINTPTR_MAX - 1), 4, consumed) == NC_INVALID_ARGUMENT && consumed == 0);
//    CHANNEL_CHECK(invalidAddress.finish() == NC_OK);
//    CHANNEL_CHECK(invalidAddress.feed(golden, wireSize, consumed) == NC_READY && consumed == wireSize);
//    CHANNEL_CHECK(invalidAddress.consume() == NC_OK);
//
//    // 同一解码器交替接收大、小、空记录；原字节全部校验，不能读出上条残留
//    NetChannel reuse;
//    for (uint32_t round = 0; round < 64; ++round)
//    {
//        const uint8_t* bytes = round % 3 == 0 ? largeWire.data() : round % 3 == 1 ? golden : hello;
//        const uint32_t length = round % 3 == 0 ? static_cast<uint32_t>(largeWire.size()) : round % 3 == 1 ? wireSize : sizeof(hello);
//        uint32_t offset = 0;
//        while (offset < length)
//        {
//            const uint32_t count = (std::min)(1U + ((round * 17U + offset) % 997U), length - offset);
//            const int32_t status = reuse.feed(bytes + offset, count, consumed);
//            CHANNEL_CHECK(consumed == count && status == (offset + count == length ? NC_READY : NC_NEED_MORE));
//            offset += count;
//        }
//        CHANNEL_CHECK(reuse.peek(view) == NC_READY && view.m_size == length - 24);
//        CHANNEL_CHECK(view.m_size == 0 ? view.m_data == nullptr : std::memcmp(view.m_data, bytes + 24, view.m_size) == 0);
//        CHANNEL_CHECK(reuse.finish() == NC_READY && reuse.consume() == NC_OK && reuse.finish() == NC_OK);
//    }
//
//    std::printf("NetChannel unit PASS: %u assertions, golden/fragment/EOF/bounds/alias/window/forged-credit\n", checks);
//    return 0;
//}
//
//#undef CHANNEL_CHECK
// NET_CHANNEL_UNIT_TEST_END