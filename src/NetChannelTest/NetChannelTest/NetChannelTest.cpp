#include "NetChannelTest.h"
#include "NetChannel/NetChannelAPI.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

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
        frame.m_channel == input.m_channel && frame.m_size == 3 && !std::memcmp(frame.m_data, kWire + 24, 3));
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
        CHECK_TEST(decoder.consume() == NC_OK && decoder.feed(kWire, sizeof(kWire), used) == NC_READY);
    }
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
    const LibraryTestCase cases[] = {
        {1, false, Wire},
        {2, false, Credit},
        {3, true, Fragments}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}