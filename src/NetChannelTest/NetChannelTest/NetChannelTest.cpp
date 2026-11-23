#include "NetChannelTest.h"
#include "NetChannel/NetChannelAPI.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <vector>

#define CHECK_TEST(expr) do { if (!(expr)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (false)

static const uint8_t kWire[] = {'N','T','W','3',3,3,0,0,0,0,0,7,1,2,3,4,5,6,7,8,0,0,0,3,0,255,17};

/** 使用手写线格式验证发送与解码，READY 期间保持正文借用
@return 全部断言通过返回0
*/
static int Wire()
{
    NetChannelRecord input = {NC_BYTES, 7, 0x0102030405060708ULL, kWire + 24, 3};
    uint8_t output[sizeof(kWire)] = {};
    uint32_t used = 0;
    std::memset(output, 0xa5, sizeof(output));
    CHECK_TEST(NetChannel::encode(input, output, sizeof(output) - 1, used) == NC_BUFFER_TOO_SMALL && used == sizeof(kWire));
    for (size_t i = 0; i < sizeof(output); ++i)
    {
        CHECK_TEST(output[i] == 0xa5);
    }
    CHECK_TEST(NetChannel::encode(input, output, sizeof(output), used) == NC_OK && !std::memcmp(kWire, output, sizeof(output)));
    NetChannel decoder;
    CHECK_TEST(decoder.feed(kWire, sizeof(kWire), used) == NC_READY && used == sizeof(kWire));
    NetChannelRecord frame = {};
    CHECK_TEST(decoder.peek(frame) == NC_READY && frame.m_kind == NC_BYTES && frame.m_product == 7 &&
        frame.m_channel == input.m_channel && frame.m_size == 3 && frame.m_data && !std::memcmp(frame.m_data, kWire + 24, 3));
    CHECK_TEST(decoder.feed(kWire, 1, used) == NC_READY && used == 0 && decoder.finish() == NC_READY);
    CHECK_TEST(decoder.consume() == NC_OK && decoder.finish() == NC_OK && decoder.consume() == NC_INVALID_STATE);
    return 0;
}

/** 单独核对发送、接收两个方向的额度
@return 全部断言通过返回0
*/
static int Credit()
{
    NetChannelCredit credit;
    CHECK_TEST(credit.capacity() == 65536 && credit.available() == 65536);
    CHECK_TEST(credit.configure(65535) == NC_INVALID_ARGUMENT && credit.capacity() == 65536);
    CHECK_TEST(credit.configure(81920) == NC_OK);
    for (int i = 0; i < 5; ++i)
    {
        CHECK_TEST(credit.sent(16384) == NC_OK);
    }
    CHECK_TEST(credit.available() == 0 && credit.sent(1) == NC_LIMIT_EXCEEDED && credit.grant(17) == NC_OK);
    CHECK_TEST(credit.available() == 17 && credit.configure(65536) == NC_INVALID_STATE);
    CHECK_TEST(credit.received(13) == NC_OK && credit.pending() == 13);
    CHECK_TEST(credit.consume(14) == NC_INVALID_ARGUMENT && credit.pending() == 13);
    CHECK_TEST(credit.consume(13) == NC_OK && credit.pending() == 0);
    CHECK_TEST(credit.grant(81920) == NC_PROTOCOL_ERROR && credit.status() == NC_PROTOCOL_ERROR && credit.available() == 0);
    return 0;
}

/** 枚举每个头和正文切点，同时验证截断后的持续状态
@return 全部断言通过返回0
*/
static int Fragments()
{
    for (uint32_t split = 1; split < sizeof(kWire); ++split)
    {
        uint32_t used = 0;
        NetChannel decoder;
        NetChannel shortInput;
        CHECK_TEST(shortInput.feed(kWire, split, used) == NC_NEED_MORE && used == split);
        CHECK_TEST(shortInput.finish() == NC_TRUNCATED && shortInput.feed(nullptr, 0, used) == NC_TRUNCATED && used == 0);
        CHECK_TEST(decoder.feed(kWire, split, used) == NC_NEED_MORE && used == split);
        CHECK_TEST(decoder.feed(kWire + split, sizeof(kWire) - split, used) == NC_READY && used == sizeof(kWire) - split);
        NetChannelRecord frame = {};
        CHECK_TEST(decoder.peek(frame) == NC_READY && frame.m_kind == NC_BYTES && frame.m_product == 7 &&
            frame.m_channel == 0x0102030405060708ULL && frame.m_size == 3 && frame.m_data && !std::memcmp(frame.m_data, kWire + 24, 3));
        CHECK_TEST(decoder.consume() == NC_OK && decoder.feed(kWire, sizeof(kWire), used) == NC_READY && used == sizeof(kWire));
    }
    return 0;
}

// 独立的多字节产品号和通道号样本，头及正文均按协议手写。
static const uint8_t golden[] = {0x4e, 0x54, 0x57, 0x33, 3, 3, 0, 0,
    1, 2, 3, 4, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0, 0, 0, 4, 0, 255, 127, 128};
static const uint8_t hello[] = {'N', 'T', 'W', '3', 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const uint8_t payload[] = {0, 255, 127, 128};
static const uint32_t wireSize = sizeof(golden);

/** 以第二组手写记录核对切点、粘连与各记录类型
@return 检查通过返回0，失败返回1
*/
static int RecordStream()
{
    uint8_t output[128] = {};
    uint32_t required = 0;
    for (uint32_t split = 0; split <= wireSize; ++split)
    {
        NetChannel decoder;
        uint32_t consumed = 999;
        const int32_t first = decoder.feed(golden, split, consumed);
        CHECK_TEST(consumed == split && first == (split == wireSize ? NC_READY : NC_NEED_MORE));
        CHECK_TEST(decoder.feed(golden + split, wireSize - split, consumed) == NC_READY);
        CHECK_TEST(consumed == wireSize - split);
        NetChannelRecord view = {};
        CHECK_TEST(decoder.peek(view) == NC_READY);
        CHECK_TEST(view.m_kind == NC_BYTES && view.m_product == 0x01020304U && view.m_channel == 0x1122334455667788ULL);
        CHECK_TEST(view.m_size == sizeof(payload) && view.m_data && std::memcmp(view.m_data, payload, sizeof(payload)) == 0);
        CHECK_TEST(decoder.feed(golden, wireSize, consumed) == NC_READY && consumed == 0);
        CHECK_TEST(decoder.finish() == NC_READY && decoder.consume() == NC_OK);
        CHECK_TEST(decoder.peek(view) == NC_NEED_MORE && view.m_data == nullptr && view.m_size == 0);
        CHECK_TEST(decoder.finish() == NC_OK && decoder.consume() == NC_INVALID_STATE);
    }

    for (uint32_t cut = 1; cut < wireSize; ++cut)
    {
        NetChannel decoder;
        uint32_t consumed = 0;
        CHECK_TEST(decoder.feed(golden, cut, consumed) == NC_NEED_MORE && consumed == cut);
        CHECK_TEST(decoder.finish() == NC_TRUNCATED);
        CHECK_TEST(decoder.feed(golden, wireSize, consumed) == NC_TRUNCATED && consumed == 0);
    }
    std::vector<uint8_t> joined(golden, golden + wireSize);
    joined.insert(joined.end(), golden, golden + wireSize);
    joined[wireSize * 2 - 1] = 0x42;
    const uint8_t secondPayload[] = {0, 255, 127, 0x42};
    NetChannel joinedDecoder;
    uint32_t consumed = 0;
    CHECK_TEST(joinedDecoder.feed(joined.data(), static_cast<uint32_t>(joined.size()), consumed) == NC_READY && consumed == wireSize);
    // feed 已接纳首帧，调用者可以立即复用这段输入。
    std::fill(joined.begin(), joined.begin() + wireSize, 0);
    NetChannelRecord joinedView = {};
    CHECK_TEST(joinedDecoder.peek(joinedView) == NC_READY && joinedView.m_size == sizeof(payload) &&
        joinedView.m_data && std::memcmp(joinedView.m_data, payload, sizeof(payload)) == 0);
    CHECK_TEST(joinedDecoder.consume() == NC_OK);
    CHECK_TEST(joinedDecoder.feed(joined.data() + wireSize, wireSize, consumed) == NC_READY && consumed == wireSize);
    CHECK_TEST(joinedDecoder.peek(joinedView) == NC_READY && joinedView.m_size == sizeof(secondPayload) &&
        joinedView.m_data && std::memcmp(joinedView.m_data, secondPayload, sizeof(secondPayload)) == 0);
    CHECK_TEST(joinedDecoder.consume() == NC_OK && joinedDecoder.finish() == NC_OK);

    NetChannel empty;
    CHECK_TEST(empty.feed(nullptr, 1, consumed) == NC_INVALID_ARGUMENT && consumed == 0);
    CHECK_TEST(empty.feed(nullptr, 0, consumed) == NC_NEED_MORE && consumed == 0 && empty.finish() == NC_OK);
    CHECK_TEST(empty.feed(hello, sizeof(hello), consumed) == NC_READY && consumed == sizeof(hello));
    NetChannelRecord view = {};
    CHECK_TEST(empty.peek(view) == NC_READY && view.m_data == nullptr && view.m_size == 0);
    CHECK_TEST(NetChannel::encode(view, output, sizeof(output), required) == NC_OK && required == sizeof(hello));
    CHECK_TEST(std::memcmp(output, hello, sizeof(hello)) == 0);

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
            CHECK_TEST(decoder.feed(sample.data() + index, 1, consumed) == (index + 1 == wireSize ? NC_READY : NC_NEED_MORE));
            CHECK_TEST(consumed == 1);
        }
        CHECK_TEST(decoder.peek(view) == NC_READY && view.m_kind == kind);
        CHECK_TEST(NetChannel::encode(view, output, sizeof(output), required) == NC_OK);
        CHECK_TEST(required == wireSize && std::memcmp(output, sample.data(), wireSize) == 0);
    }
    return 0;
}

/** 逐字段验证协议错误、长度上界及持续状态
@return 检查通过返回0，失败返回1
*/
static int InvalidHeader()
{
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
        CHECK_TEST(decoder.feed(invalid.data(), wireSize, consumed) == NC_PROTOCOL_ERROR && consumed == 24);
        NetChannelRecord view = {NC_BYTES, 1, 1, payload, sizeof(payload)};
        CHECK_TEST(decoder.peek(view) == NC_PROTOCOL_ERROR && !view.m_kind && !view.m_product && !view.m_channel && !view.m_data && !view.m_size);
        CHECK_TEST(decoder.feed(golden, wireSize, consumed) == NC_PROTOCOL_ERROR && consumed == 0);
        CHECK_TEST(decoder.finish() == NC_PROTOCOL_ERROR && decoder.consume() == NC_PROTOCOL_ERROR);
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
        CHECK_TEST(decoder.feed(invalid.data(), wireSize, consumed) == NC_LIMIT_EXCEEDED && consumed == 24);
        CHECK_TEST(decoder.feed(golden, wireSize, consumed) == NC_LIMIT_EXCEEDED && consumed == 0);
        CHECK_TEST(decoder.finish() == NC_LIMIT_EXCEEDED);
    }
    // 同一类型的非法元数据与合法线格式仅相差一个字段。
    const uint8_t changes[][3] = {{1, 23, 4}, {2, 11, 1}, {2, 19, 1}, {2, 23, 0}, {3, 23, 0}, {4, 23, 3}};
    for (size_t index = 0; index < sizeof(changes) / sizeof(changes[0]); ++index)
    {
        uint8_t header[24];
        std::memcpy(header, hello, sizeof(header));
        header[5] = changes[index][0];
        header[19] = header[5] >= NC_BYTES ? 1 : 0;
        header[23] = header[5] == NC_HELLO ? 0 : 4;
        header[changes[index][1]] = changes[index][2];
        NetChannel decoder;
        uint32_t used = 0;
        CHECK_TEST(decoder.feed(header, sizeof(header), used) == NC_PROTOCOL_ERROR && used == sizeof(header));
    }
    return 0;
}

/** 核对不写坏输出、别名边界及非法地址拒绝
@return 检查通过返回0，失败返回1
*/
static int EncodeBounds()
{
    NetChannelRecord record = {NC_BYTES, 0x01020304U, 0x1122334455667788ULL, payload, sizeof(payload)};
    uint8_t output[128];
    uint32_t required = 0;
    std::memset(output, 0xa5, sizeof(output));
    CHECK_TEST(NetChannel::encode(record, nullptr, 0, required) == NC_BUFFER_TOO_SMALL && required == wireSize);
    CHECK_TEST(NetChannel::encode(record, output, wireSize - 1, required) == NC_BUFFER_TOO_SMALL);
    for (size_t index = 0; index < sizeof(output); ++index)
    {
        CHECK_TEST(output[index] == 0xa5);
    }
    CHECK_TEST(NetChannel::encode(record, output, sizeof(output), required) == NC_OK && required == wireSize);
    CHECK_TEST(std::memcmp(output, golden, wireSize) == 0 && output[wireSize] == 0xa5);
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
        CHECK_TEST(NetChannel::encode(invalid, output, sizeof(output), required) == NC_INVALID_ARGUMENT && required == 0);
        for (size_t offset = 0; offset < sizeof(output); ++offset)
        {
            CHECK_TEST(output[offset] == 0xa5);
        }
    }
    std::memset(output, 0xa5, sizeof(output));
    record.m_data = output + 30;
    CHECK_TEST(NetChannel::encode(record, output, sizeof(output), required) == NC_OK);
    record.m_data = output + wireSize;
    CHECK_TEST(NetChannel::encode(record, output, sizeof(output), required) == NC_OK);
    record.m_data = output + 4;
    CHECK_TEST(NetChannel::encode(record, output, sizeof(output), required) == NC_INVALID_ARGUMENT && required == 0);
    record.m_data = reinterpret_cast<const uint8_t*>(UINTPTR_MAX - 1);
    CHECK_TEST(NetChannel::encode(record, output, sizeof(output), required) == NC_INVALID_ARGUMENT && required == 0);
    record.m_data = payload;
    CHECK_TEST(NetChannel::encode(record, reinterpret_cast<uint8_t*>(UINTPTR_MAX - 1), 128, required) == NC_INVALID_ARGUMENT);
    CHECK_TEST(NetChannel::encode(record, nullptr, 128, required) == NC_INVALID_ARGUMENT && required == 0);
    std::vector<NetChannelRecord> aliased(3);
    aliased[0] = record;
    CHECK_TEST(NetChannel::encode(aliased[0], reinterpret_cast<uint8_t*>(aliased.data()),
        static_cast<uint32_t>(aliased.size() * sizeof(NetChannelRecord)), required) == NC_OK);
    CHECK_TEST(std::memcmp(aliased.data(), golden, wireSize) == 0);
    record.m_size = NET_CHANNEL_PAYLOAD_LIMIT + 1;
    CHECK_TEST(NetChannel::encode(record, nullptr, 0, required) == NC_INVALID_ARGUMENT && required == 0);
    uint32_t consumed = 0;
    NetChannel invalidAddress;
    CHECK_TEST(invalidAddress.feed(reinterpret_cast<const uint8_t*>(UINTPTR_MAX - 1), 4, consumed) == NC_INVALID_ARGUMENT && consumed == 0);
    CHECK_TEST(invalidAddress.finish() == NC_OK);
    CHECK_TEST(invalidAddress.feed(golden, wireSize, consumed) == NC_READY && consumed == wireSize);
    CHECK_TEST(invalidAddress.consume() == NC_OK);
    return 0;
}

/** 核对最大记录和连续复用后的全部有效字节
@return 检查通过返回0，失败返回1
*/
static int StorageReuse()
{
    NetChannelRecord record = {NC_BYTES, 0x01020304U, 0x1122334455667788ULL, payload, sizeof(payload)};
    uint32_t consumed = 0, required = 0;
    NetChannelRecord view = {};
    std::vector<uint8_t> large(NET_CHANNEL_PAYLOAD_LIMIT);
    for (size_t index = 0; index < large.size(); ++index)
    {
        large[index] = static_cast<uint8_t>(index * 37);
    }
    record.m_data = large.data();
    record.m_size = static_cast<uint32_t>(large.size());
    std::vector<uint8_t> largeWire(NET_CHANNEL_HEADER_SIZE + large.size());
    CHECK_TEST(NetChannel::encode(record, largeWire.data(), static_cast<uint32_t>(largeWire.size()), required) == NC_OK && required == largeWire.size());
    CHECK_TEST(largeWire[20] == 0 && largeWire[21] == 0 && largeWire[22] == 64 && largeWire[23] == 0);
    NetChannel largeDecoder;
    for (uint32_t offset = 0; offset < largeWire.size();)
    {
        const uint32_t part = (std::min)(257U, static_cast<uint32_t>(largeWire.size()) - offset);
        const int32_t result = largeDecoder.feed(largeWire.data() + offset, part, consumed);
        CHECK_TEST(consumed == part);
        offset += consumed;
        CHECK_TEST(result == (offset == largeWire.size() ? NC_READY : NC_NEED_MORE));
    }
    CHECK_TEST(largeDecoder.peek(view) == NC_READY && view.m_size == large.size());
    CHECK_TEST(view.m_data && std::memcmp(view.m_data, large.data(), large.size()) == 0);
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
            CHECK_TEST(consumed == count && status == (offset + count == length ? NC_READY : NC_NEED_MORE));
            offset += count;
        }
        CHECK_TEST(reuse.peek(view) == NC_READY && view.m_size == length - 24);
        CHECK_TEST(view.m_size == 0 ? view.m_data == nullptr : view.m_data && std::memcmp(view.m_data, bytes + 24, view.m_size) == 0);
        CHECK_TEST(reuse.finish() == NC_READY && reuse.consume() == NC_OK && reuse.finish() == NC_OK);
    }
    return 0;
}

/** 遍历合法窗口与收发两侧的额度错误
@return 检查通过返回0，失败返回1
*/
static int WindowLimits()
{
    NetChannelCredit configurable;
    CHECK_TEST(configurable.configure(81920) == NC_OK && configurable.configure(65536) == NC_OK);
    CHECK_TEST(configurable.received(1) == NC_OK && configurable.configure(81920) == NC_INVALID_STATE);
    CHECK_TEST(configurable.capacity() == 65536 && configurable.pending() == 1 && configurable.available() == 65536);
    CHECK_TEST(NET_CHANNEL_HEADER_SIZE == 24 && NET_CHANNEL_PAYLOAD_LIMIT == 16384);
    CHECK_TEST(NET_CHANNEL_DEFAULT_WINDOW == 65536 && NET_CHANNEL_MAX_WINDOW == 1048576);
    for (uint32_t capacity = 65536; capacity <= 1048576; capacity += 16384)
    {
        NetChannelCredit credit;
        CHECK_TEST(credit.configure(capacity) == NC_OK && credit.capacity() == capacity && credit.available() == capacity);
        CHECK_TEST(credit.sent(0) == NC_INVALID_ARGUMENT && credit.available() == capacity);
        CHECK_TEST(credit.sent(16385) == NC_INVALID_ARGUMENT && credit.available() == capacity);
        for (uint32_t offset = 0; offset < capacity; offset += NET_CHANNEL_PAYLOAD_LIMIT)
        {
            CHECK_TEST(credit.sent(NET_CHANNEL_PAYLOAD_LIMIT) == NC_OK);
            CHECK_TEST(credit.received(NET_CHANNEL_PAYLOAD_LIMIT) == NC_OK);
        }
        CHECK_TEST(credit.available() == 0 && credit.pending() == capacity);
        CHECK_TEST(credit.sent(1) == NC_LIMIT_EXCEEDED && credit.status() == NC_OK);
        CHECK_TEST(credit.consume(0) == NC_INVALID_ARGUMENT && credit.pending() == capacity);
        CHECK_TEST(credit.consume(capacity) == NC_OK && credit.pending() == 0);
        CHECK_TEST(credit.grant(capacity) == NC_OK && credit.available() == capacity);
        CHECK_TEST(credit.configure(capacity) == NC_INVALID_STATE);
        CHECK_TEST(credit.grant(1) == NC_PROTOCOL_ERROR && credit.available() == 0);
        CHECK_TEST(credit.sent(1) == NC_PROTOCOL_ERROR && credit.grant(1) == NC_PROTOCOL_ERROR);
    }
    const uint32_t wrongWindows[] = {0, 65535, 65537, 1048577, 0xffffffffU};
    for (size_t index = 0; index < sizeof(wrongWindows) / sizeof(wrongWindows[0]); ++index)
    {
        NetChannelCredit credit;
        CHECK_TEST(credit.configure(wrongWindows[index]) == NC_INVALID_ARGUMENT);
        CHECK_TEST(credit.capacity() == 65536 && credit.available() == 65536 && credit.status() == NC_OK);
    }
    NetChannelCredit received;
    CHECK_TEST(received.received(0) == NC_PROTOCOL_ERROR && received.pending() == 0);
    CHECK_TEST(received.received(1) == NC_PROTOCOL_ERROR && received.consume(1) == NC_PROTOCOL_ERROR);
    NetChannelCredit overflow;
    for (uint32_t index = 0; index < 4; ++index)
    {
        CHECK_TEST(overflow.received(16384) == NC_OK);
    }
    CHECK_TEST(overflow.received(1) == NC_PROTOCOL_ERROR && overflow.pending() == 0);
    NetChannelCredit zeroGrant;
    CHECK_TEST(zeroGrant.grant(0) == NC_PROTOCOL_ERROR && zeroGrant.status() == NC_PROTOCOL_ERROR);
    NetChannelCredit wrongConsume;
    CHECK_TEST(wrongConsume.consume(1) == NC_INVALID_ARGUMENT && wrongConsume.status() == NC_OK);
    CHECK_TEST(wrongConsume.received(16385) == NC_PROTOCOL_ERROR);
    return 0;
}

/** 度量已有输出缓冲时的固定大小记录编码，消费每次调用的结果
@return 计时完成返回0
*/
static int Benchmark()
{
    uint8_t payload[1024] = {};
    uint8_t output[1048];
    NetChannelRecord input = {NC_BYTES, 1, 1, payload, sizeof(payload)};
    for (uint32_t i = 0; i < sizeof(payload); ++i)
    {
        payload[i] = static_cast<uint8_t>(i);
    }
    uint32_t required = 0;
    const uint8_t header[] = {'N','T','W','3',3,3,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,4,0};
    CHECK_TEST(NetChannel::encode(input, output, sizeof(output), required) == NC_OK && required == sizeof(output));
    CHECK_TEST(!std::memcmp(output, header, sizeof(header)) && !std::memcmp(output + sizeof(header), payload, sizeof(payload)));
    auto volatile encode = &NetChannel::encode;
    LibraryBenchmark::run("channel.encode1024", [&](uint32_t count) -> uint64_t {
        uint64_t bytes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t used = 0;
            if (encode(input, output, sizeof(output), used) != NC_OK || used != sizeof(output))
            {
                throw std::runtime_error("encode1024");
            }
            bytes += used + output[(sizeof(output) - sizeof(payload)) + (i & 1023)];
        }
        return bytes;
    });
    return 0;
}

/** 按稳定编号选择用例，返回库测试调度器的退出码
@param [in] argc 参数个数
@param [in] argv 命令行参数
@return 全部通过为0，断言失败为1，参数错误为2
*/
int main(int argc, char** argv)
try
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return Benchmark();
    }
    const NetChannelTestCase cases[] = {
        {NetChannelCaseWire, false, "手写线格式", "编码字节完整匹配，借用到消费结束", Wire},
        {NetChannelCaseCredit, false, "双向信用额度", "发送和接收独立记账，拒绝重复返还", Credit},
        {NetChannelCaseFragments, true, "分片与断流", "每个切点复原正文，过早结束返回截断", Fragments},
        {NetChannelCaseRecordStream, false, "连续记录和粘连", "交付全部四类记录，输入可以复用", RecordStream},
        {NetChannelCaseInvalidHeader, false, "坏头与超长正文", "消费固定头后保留错误", InvalidHeader},
        {NetChannelCaseEncodeBounds, false, "编码缓冲边界", "拒绝非法重叠和长度，失败保留输出", EncodeBounds},
        {NetChannelCaseStorageReuse, true, "大小正文交替复用", "最大、短小和空正文逐字节匹配", StorageReuse},
        {NetChannelCaseWindowLimits, false, "信用窗口边界", "按协议接受全范围窗口并保持额度不变量", WindowLimits}
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        CHECK_TEST(cases[i].id == i + 1 && cases[i].m_run && cases[i].m_name[0] && cases[i].m_expected[0]);
    }
    return LibraryCaseSelection::run(argc, argv, cases, [](const NetChannelTestCase& item) -> int
    {
        const int result = item.m_run();
        if (result != 0)
        {
            std::fprintf(stderr, "%s: %s\n", item.m_name, item.m_expected);
        }
        return result;
    });
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}