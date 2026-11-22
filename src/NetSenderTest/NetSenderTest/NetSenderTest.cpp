#include "NetSenderTest.h"
#ifdef _WIN32
#define _NetSenderAPI
#endif
#include "NetSender/NetSender.h"
#include "NetSender/NetSenderLocalPacket.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

#define CHECK_TEST(expr) do { if (!(expr)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (false)

/** 对照独立的 56 字节 NIP2 头布局
@return 全部通过返回0
*/
static int Packets()
{
    const uint8_t data[] = {17,0,255};
    uint8_t bytes[59] = {}, expected[59] = {};
    std::memcpy(expected, "NIP2", 4);
    expected[7] = 2;
    expected[11] = 1;
    expected[15] = 2;
    expected[16] = 42;
    expected[47] = 73;
    expected[51] = 3;
    std::memcpy(expected + 56, data, 3);
    NetSenderLocalPacketView packet = {};
    packet.m_kind = NET_LOCAL_DATA;
    packet.m_packet = 0x100000002ULL;
    packet.m_source[0] = 42;
    packet.m_target[15] = 73;
    packet.m_data = data;
    packet.m_size = sizeof(data);
    uint32_t required = 0;
    CHECK_TEST(NetSenderLocalPacket::encode(packet, nullptr, 0, required) == NI_BUFFER_TOO_SMALL && required == sizeof(bytes));
    CHECK_TEST(NetSenderLocalPacket::encode(packet, bytes, sizeof(bytes), required) == NI_OK && !std::memcmp(bytes, expected, sizeof(bytes)));
    NetSenderLocalPacketView read = {};
    CHECK_TEST(NetSenderLocalPacket::parse(expected, sizeof(expected), read) == NI_OK && read.m_kind == NET_LOCAL_DATA &&
        read.m_packet == 0x100000002ULL && read.m_data == expected + 56 && read.m_size == 3 &&
        NetSenderLocalPacket::sameGeneration(read.m_source, packet.m_source) && NetSenderLocalPacket::sameGeneration(read.m_target, packet.m_target));
    packet.m_target[15] = 0;
    CHECK_TEST(NetSenderLocalPacket::encode(packet, bytes, sizeof(bytes), required) == NI_INVALID_ARGUMENT &&
        required == 0 && !std::memcmp(bytes, expected, sizeof(bytes)));
    CHECK_TEST(NetSenderLocalPacket::parse(expected, sizeof(expected) - 1, read) == NI_PROTOCOL_ERROR && !read.m_data && read.m_size == 0);
    return 0;
}

/** 依据累计收发语义检查重复及跳号
@return 全部通过返回0
*/
static int Window()
{
    NetSenderLocalWindow window;
    CHECK_TEST(window.next() == 1 && window.receipt() == 0 && window.outstanding() == 0);
    CHECK_TEST(window.published(2) == NI_INVALID_STATE && window.next() == 1 && window.published(1) == NI_OK);
    CHECK_TEST(window.published(2) == NI_OK && window.outstanding() == 2);
    CHECK_TEST(window.acknowledge(3) == NI_PROTOCOL_ERROR && window.outstanding() == 2);
    CHECK_TEST(window.acknowledge(1) == NI_OK && window.acknowledge(1) == NI_OK && window.outstanding() == 1);
    CHECK_TEST(window.acknowledge(2) == NI_OK && window.outstanding() == 0);
    CHECK_TEST(window.received(2) == NI_PROTOCOL_ERROR && window.receipt() == 0 && window.received(1) == NI_OK);
    CHECK_TEST(window.received(1) == NI_NOT_FOUND && window.received(2) == NI_OK && window.receipt() == 2);
    return 0;
}

/** 通过桌面提供方的公开门面确认独立上下文
@return 全部通过返回0
*/
static int Client()
{
    NetSender& api = NetSender::instance();
    NetSenderClient* client = nullptr;
    CHECK_TEST(NetSender::pointerBytes() == sizeof(void*) && NetSender::configureWorkers(1) == NI_OK && NetSender::configureWorkers(2) == NI_NOT_SUPPORTED);
    CHECK_TEST(api.createClient(client) == NI_OK && client);
    NetSenderClient* original = client;
    uint64_t generation = 1;
    bool ready = true;
    const bool valid = api.createClient(client) == NI_INVALID_ARGUMENT && client == original &&
        api.gatewayState(client, generation, ready) == NI_OK && !ready && generation == 0;
    CHECK_TEST(api.destroyClient(client) == NI_OK && !client && valid && api.destroyClient(client) == NI_OK);
    return 0;
}

/** 测量 NIP2 发送方复制 1024 字节密文到现成缓冲的成本
@return 成功返回0
*/
static int Benchmark()
{
    uint8_t data[1024] = {}, output[1080];
    NetSenderLocalPacketView packet = {};
    packet.m_kind = NET_LOCAL_DATA;
    packet.m_packet = 1;
    packet.m_source[0] = 1;
    packet.m_target[0] = 2;
    packet.m_data = data;
    packet.m_size = sizeof(data);
    for (uint32_t i = 0; i < sizeof(data); ++i)
    {
        data[i] = static_cast<uint8_t>(i);
    }
    auto volatile encode = &NetSenderLocalPacket::encode;
    LibraryBenchmark::run("sender.encode1024", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t n = 0;
            if (encode(packet, output, sizeof(output), n) != NI_OK || n != sizeof(output))
            {
                throw std::runtime_error("NIP2 encode");
            }
            sum += n + output[(sizeof(output) - sizeof(data)) + (i & 1023)];
        }
        return sum;
    });
    return 0;
}

/** 连续发布确认
@return 全部通过为0
*/
static int RepeatedOperations()
{
    NetSenderLocalWindow window;
    for (uint64_t packet = 1; packet <= 4096; ++packet)
    {
        CHECK_TEST(window.next() == packet && window.published(packet) == NI_OK && window.outstanding() == 1);
        CHECK_TEST(window.received(packet) == NI_OK && window.receipt() == packet);
        CHECK_TEST(window.received(packet) == NI_NOT_FOUND && window.acknowledge(packet) == NI_OK);
        CHECK_TEST(window.acknowledge(packet) == NI_OK && window.outstanding() == 0);
    }
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
        {1, false, Packets},
        {2, false, Window},
        {3, false, Client},
        {4, true, RepeatedOperations}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}