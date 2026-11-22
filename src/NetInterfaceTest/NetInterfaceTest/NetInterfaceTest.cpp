#include "NetInterfaceTest.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

#define CHECK_TEST(expr) do { if (!(expr)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (false)

static const uint8_t kFrame[] = {'N','I','F','2',0,2,0,3,0,0,0,7,1,2,3,4,5,6,7,8,0,0,0,3,0,0,0,9,0,0,0,0,0,255,17};

/** 固定大小头及尾随数据的片段消费、额度和复位
@return 全部通过为0
*/
static int Frame()
{
    const NetInterfaceFrame input = {7,0x0102030405060708ULL,9,NI_REQUEST,3,kFrame + 32};
    uint8_t out[sizeof(kFrame)] = {};
    uint32_t size = 0;
    CHECK_TEST(NetInterface::encode(input, out, sizeof(out), size) == NI_OK && size == sizeof(out) &&
        !std::memcmp(out, kFrame, size));
    NetInterface decoder(3);
    CHECK_TEST(decoder.payloadLimit() == 3 && decoder.feed(kFrame, 28, size) == NI_NEED_MORE && size == 28);
    CHECK_TEST(decoder.finish() == NI_TRUNCATED && decoder.status() == NI_TRUNCATED);
    decoder.reset();
    CHECK_TEST(decoder.feed(kFrame, 33, size) == NI_NEED_MORE && size == 33);
    CHECK_TEST(decoder.feed(kFrame + 33, 2, size) == NI_READY && size == 2);
    const NetInterfaceFrame* view = nullptr;
    CHECK_TEST(decoder.peek(view) == NI_READY && view && view->m_productId == 7 && view->m_kind == NI_REQUEST &&
        view->m_requestId == input.m_requestId && view->m_messageType == 9 && view->m_payloadSize == 3 &&
        !std::memcmp(view->m_payload, kFrame + 32, 3));
    CHECK_TEST(decoder.consume() == NI_OK && decoder.finish() == NI_OK);
    NetInterface tiny(2);
    CHECK_TEST(tiny.feed(kFrame, sizeof(kFrame), size) == NI_LIMIT_EXCEEDED && size == NET_INTERFACE_HEADER_SIZE);
    return 0;
}

/** 验证紧凑帧的能力开关、无符号字段及二进制负载
@return 全部通过为0
*/
static int Compact()
{
    NetInterfaceFrame input = {128,0xf102030405060708ULL,16384,NI_REQUEST,3,kFrame + 32};
    uint8_t buffer[64] = {};
    uint32_t size = 0, used = 0;
    CHECK_TEST(NetInterface::encodeCompact(input, buffer, sizeof(buffer), size) == NI_OK && size < sizeof(kFrame));
    NetInterface plain;
    CHECK_TEST(plain.feed(buffer, size, used) < 0 || plain.finish() < 0);
    NetInterface compact;
    compact.allowCompact(true);
    CHECK_TEST(compact.feed(buffer, size, used) == NI_READY && used == size);
    NetInterfaceFrame value = {};
    CHECK_TEST(compact.peek(value) == NI_READY && value.m_requestId == input.m_requestId && value.m_productId == 128 &&
        value.m_messageType == 16384 && value.m_payloadSize == 3 && !std::memcmp(value.m_payload, kFrame + 32, 3));
    return 0;
}

/** 用已知 SHA-256 向量独立校验单次及两片更新
@return 全部通过为0
*/
static int Digest()
{
    const uint8_t abc[] = {'a','b','c'};
    const uint8_t expected[] = {0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad};
    uint8_t digest[32] = {};
    CHECK_TEST(NetInterface::sha256(abc, 3, digest, sizeof(digest)) == NI_OK && !std::memcmp(expected, digest, 32));
    NetInterfaceHashHandle hash = nullptr;
    CHECK_TEST(NetInterface::hashCreate(&hash) == NI_OK && hash);
    const bool valid = NetInterface::hashUpdate(hash, abc, 1) == NI_OK && NetInterface::hashUpdate(hash, abc + 1, 2) == NI_OK &&
        NetInterface::hashFinish(hash, digest, sizeof(digest)) == NI_OK;
    NetInterface::hashDestroy(hash);
    CHECK_TEST(valid && !std::memcmp(digest, expected, 32));
    NetInterface::wipe(digest + 1, 30);
    const uint8_t zero[30] = {};
    CHECK_TEST(digest[0] == expected[0] && !std::memcmp(digest + 1, zero, 30) && digest[31] == expected[31]);
    return 0;
}

/** 核对不透明消息句柄跨线传递含零字节的业务错误
@return 全部通过为0
*/
static int Message()
{
    const uint8_t value[] = {'a',0,'z'};
    NetInterfaceMessageHandle handle = nullptr;
    CHECK_TEST(NetInterface::messageCreate(&handle) == NI_OK && handle);
    uint8_t wire[128], out[3] = {};
    uint32_t n = 0, need = 0;
    int32_t code = 0;
    const bool valid = NetInterface::messageSetCode(handle, 73, value, 3) == NI_OK &&
        NetInterface::messageEncode(handle, wire, sizeof(wire), &n) == NI_OK && n > 0 &&
        NetInterface::messageClear(handle) == NI_OK && NetInterface::messageParse(handle, wire, n) == NI_OK &&
        NetInterface::messageGetCode(handle, &code, nullptr, 0, &need) == NI_BUFFER_TOO_SMALL && need == 3 &&
        NetInterface::messageGetCode(handle, &code, out, 3, &need) == NI_OK && code == 73 && !std::memcmp(out, value, 3);
    NetInterface::messageDestroy(handle);
    CHECK_TEST(valid);
    return 0;
}

/** 测量一次固定 NIF2 编码至复用缓冲的成本
@return 成功为0
*/
static int Benchmark()
{
    uint8_t payload[1024] = {}, output[1056];
    const NetInterfaceFrame frame = {7,1,9,NI_REQUEST,sizeof(payload),payload};
    for (uint32_t i = 0; i < sizeof(payload); ++i)
    {
        payload[i] = static_cast<uint8_t>(i);
    }
    int32_t (*volatile encode)(const NetInterfaceFrame&, uint8_t*, uint32_t, uint32_t&) = &NetInterface::encode;
    LibraryBenchmark::run("interface.encode1024", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t size = 0;
            if (encode(frame, output, sizeof(output), size) != NI_OK || size != sizeof(output))
            {
                throw std::runtime_error("encode1024");
            }
            sum += size + output[(sizeof(output) - sizeof(payload)) + (i & 1023)];
        }
        return sum;
    });
    return 0;
}

/** 分片及复用
@return 全部通过为0
*/
static int RepeatedOperations()
{
    NetInterface decoder(3);
    for (uint32_t i = 0; i < 1024; ++i)
    {
        const uint32_t total = sizeof(kFrame);
        const uint32_t split = 1 + i % (total - 1);
        uint32_t used = 0;
        CHECK_TEST(decoder.feed(kFrame, split, used) == NI_NEED_MORE && used == split);
        CHECK_TEST(decoder.feed(kFrame + split, total - split, used) == NI_READY && used == total - split);
        const NetInterfaceFrame* frame = nullptr;
        CHECK_TEST(decoder.peek(frame) == NI_READY && frame && frame->m_payloadSize == 3 &&
            !std::memcmp(frame->m_payload, kFrame + 32, 3));
        CHECK_TEST(decoder.consume() == NI_OK && decoder.finish() == NI_OK);
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
        {1, false, Frame},
        {2, false, Compact},
        {3, false, Digest},
        {4, false, Message},
        {5, true, RepeatedOperations}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}