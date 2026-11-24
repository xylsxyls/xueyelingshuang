#include "NetInterfaceTest.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "PlatformTest/PlatformTestAPI.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
#include <cstdio>
#include <cstring>
#include <exception>
#include <vector>
#include <memory>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif
#include "../../NetGatewayTest/NetGatewayTest/NetTestTlsCertificateData.h"

#define CHECK_TEST(expr) do { if (!(expr)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (false)

// 手工构造的 NIF2 网络字节序样本，正文含零字节。
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
        view->m_requestId == input.m_requestId && view->m_messageType == 9 && view->m_payloadSize == 3 && view->m_payload &&
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
    const uint8_t expected[] = {0xe3,3,0x80,1,0x80,0x80,1,3,0xf1,2,3,4,5,6,7,8,0,255,17};
    CHECK_TEST(NetInterface::encodeCompact(input, buffer, sizeof(buffer), size) == NI_OK && size == sizeof(expected) &&
        !std::memcmp(buffer, expected, size));
    NetInterface plain;
    CHECK_TEST(plain.feed(expected, sizeof(expected), used) == NI_PROTOCOL_ERROR && plain.finish() == NI_PROTOCOL_ERROR);
    NetInterface compact;
    compact.allowCompact(true);
    CHECK_TEST(compact.feed(expected, sizeof(expected), used) == NI_READY && used == size);
    NetInterfaceFrame value = {};
    CHECK_TEST(compact.peek(value) == NI_READY && value.m_requestId == input.m_requestId && value.m_productId == 128 &&
        value.m_messageType == 16384 && value.m_kind == NI_REQUEST && value.m_payloadSize == 3 && value.m_payload && !std::memcmp(value.m_payload, kFrame + 32, 3));
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
    std::unique_ptr<NetInterfaceHashOpaque, void (*)(NetInterfaceHashHandle)> owner(hash, NetInterface::hashDestroy);
    CHECK_TEST(NetInterface::hashUpdate(hash, abc, 1) == NI_OK);
    CHECK_TEST(NetInterface::hashUpdate(hash, nullptr, 1) == NI_INVALID_ARGUMENT);
    CHECK_TEST(NetInterface::hashUpdate(hash, nullptr, 0) == NI_OK);
    CHECK_TEST(NetInterface::hashUpdate(hash, abc + 1, 2) == NI_OK);
    uint8_t untouched[32], streamed[32];
    std::memset(untouched, 0xa5, sizeof(untouched));
    std::memcpy(streamed, untouched, sizeof(streamed));
    CHECK_TEST(NetInterface::hashFinish(hash, streamed, sizeof(streamed) - 1) == NI_BUFFER_TOO_SMALL);
    CHECK_TEST(!std::memcmp(streamed, untouched, sizeof(streamed)));
    CHECK_TEST(NetInterface::hashFinish(hash, streamed, sizeof(streamed)) == NI_OK);
    CHECK_TEST(!std::memcmp(streamed, expected, sizeof(streamed)));
    CHECK_TEST(NetInterface::hashUpdate(hash, abc, 1) == NI_INVALID_STATE);
    std::memcpy(streamed, untouched, sizeof(streamed));
    CHECK_TEST(NetInterface::hashFinish(hash, streamed, sizeof(streamed)) == NI_INVALID_STATE);
    CHECK_TEST(!std::memcmp(streamed, untouched, sizeof(streamed)));
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
    NetInterfaceMessageHandle received = nullptr;
    const bool created = NetInterface::messageCreate(&received) == NI_OK && received;
    uint8_t wire[128], out[3] = {};
    uint32_t n = 0, need = 0;
    int32_t code = 0;
    const bool valid = created && NetInterface::messageSetCode(handle, 73, value, 3) == NI_OK &&
        NetInterface::messageEncode(handle, wire, sizeof(wire), &n) == NI_OK && n > 0 &&
        NetInterface::messageClear(handle) == NI_OK &&
        NetInterface::messageGetCode(handle, &code, nullptr, 0, &need) == NI_OK && code == 0 && need == 0 &&
        NetInterface::messageParse(received, wire, n) == NI_OK &&
        NetInterface::messageGetCode(received, &code, nullptr, 0, &need) == NI_BUFFER_TOO_SMALL && need == 3 &&
        NetInterface::messageGetCode(received, &code, out, 3, &need) == NI_OK && code == 73 && !std::memcmp(out, value, 3);
    NetInterface::messageDestroy(received);
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
    const uint8_t header[] = {'N','I','F','2',0,2,0,3,0,0,0,7,0,0,0,0,0,0,0,1,0,0,4,0,0,0,0,9,0,0,0,0};
    uint32_t size = 0;
    CHECK_TEST(encode(frame, output, sizeof(output), size) == NI_OK && size == sizeof(output) &&
        !std::memcmp(output, header, sizeof(header)) && !std::memcmp(output + sizeof(header), payload, sizeof(payload)));
    bool valid = true;
    LibraryBenchmark::run("interface.encode1024", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t size = 0;
            if (encode(frame, output, sizeof(output), size) != NI_OK || size != sizeof(output))
            {
                valid = false;
                return 0;
            }
            sum += size + output[(sizeof(output) - sizeof(payload)) + (i & 1023)];
        }
        return sum;
    });
    CHECK_TEST(valid);
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
        CHECK_TEST(decoder.peek(frame) == NI_READY && frame && frame->m_payloadSize == 3 && frame->m_payload &&
            !std::memcmp(frame->m_payload, kFrame + 32, 3));
        CHECK_TEST(decoder.consume() == NI_OK && decoder.finish() == NI_OK);
    }
    return 0;
}

/** 连续两帧只消费当前帧，输入可复用，短输出缓冲保持原样
@return 成功为0
*/
static int FrameBoundaries()
{
    uint8_t stream[2 * sizeof(kFrame)];
    std::memcpy(stream, kFrame, sizeof(kFrame));
    std::memcpy(stream + sizeof(kFrame), kFrame, sizeof(kFrame));
    stream[sizeof(stream) - 1] = 73;
    const NetInterfaceFrame input = {7,1,9,NI_REQUEST,3,kFrame + 32};
    uint32_t n = 0;
    CHECK_TEST(NetInterface::encode(input, nullptr, 0, n) == NI_BUFFER_TOO_SMALL && n == sizeof(kFrame));
    uint8_t shortOutput[sizeof(kFrame) + 2];
    std::memset(shortOutput, 0xa5, sizeof(shortOutput));
    CHECK_TEST(NetInterface::encode(input, shortOutput + 1, n - 1, n) == NI_BUFFER_TOO_SMALL && n == sizeof(kFrame));
    for (size_t i = 0; i < sizeof(shortOutput); ++i)
    {
        CHECK_TEST(shortOutput[i] == 0xa5);
    }
    NetInterface decoder(3);
    const NetInterfaceFrame* view = nullptr;
    CHECK_TEST(decoder.feed(stream, sizeof(stream), n) == NI_READY && n == sizeof(kFrame));
    stream[34] = 99;
    CHECK_TEST(decoder.feed(stream + n, sizeof(kFrame), n) == NI_READY && n == 0);
    CHECK_TEST(decoder.peek(view) == NI_READY && view && view->m_payload && view->m_payloadSize == 3 && view->m_payload[2] == 17);
    CHECK_TEST(decoder.consume() == NI_OK);
    CHECK_TEST(decoder.feed(stream + sizeof(kFrame), sizeof(kFrame), n) == NI_READY && n == sizeof(kFrame));
    CHECK_TEST(decoder.peek(view) == NI_READY && view && view->m_payload && view->m_payloadSize == 3 && view->m_payload[2] == 73);
    CHECK_TEST(decoder.consume() == NI_OK && decoder.finish() == NI_OK);
    NetInterface invalid(0);
    invalid.reset();
    CHECK_TEST(invalid.status() == NI_INVALID_ARGUMENT);
    return 0;
}

/** 检查两种增量头继承的产品、类型和请求号，以及重置和撤销许可后的基线
@return 成功为0
*/
static int CompactSequence()
{
    const uint8_t base[] = {0xe3,NI_DATA,7,9,0,0,0,0,0,0,0,0,40};
    const uint8_t next[] = {0x82,0,255};
    const uint8_t jump[] = {0xe4,1,3,77};
    NetInterface decoder(2);
    decoder.allowCompact(true);
    uint32_t used = 0;
    NetInterfaceFrame frame = {};
    CHECK_TEST(decoder.feed(base,sizeof(base),used) == NI_READY && used == sizeof(base));
    CHECK_TEST(decoder.consume() == NI_OK && decoder.feed(next,sizeof(next),used) == NI_READY && used == sizeof(next));
    CHECK_TEST(decoder.peek(frame) == NI_READY && frame.m_productId == 7 && frame.m_kind == NI_DATA &&
        frame.m_messageType == 9 && frame.m_requestId == 41 && frame.m_payloadSize == 2 && frame.m_payload &&
        !std::memcmp(frame.m_payload,next + 1,2));
    CHECK_TEST(decoder.consume() == NI_OK && decoder.feed(jump,1,used) == NI_NEED_MORE && used == 1);
    CHECK_TEST(decoder.feed(jump + 1,3,used) == NI_READY && used == 3 && decoder.peek(frame) == NI_READY &&
        frame.m_requestId == 44 && frame.m_payloadSize == 1 && frame.m_payload && frame.m_payload[0] == 77);
    decoder.reset();
    CHECK_TEST(decoder.feed(next,sizeof(next),used) == NI_PROTOCOL_ERROR);
    decoder.reset();
    CHECK_TEST(decoder.feed(base,sizeof(base),used) == NI_READY && decoder.consume() == NI_OK);
    decoder.allowCompact(false);
    decoder.allowCompact(true);
    CHECK_TEST(decoder.feed(jump,sizeof(jump),used) == NI_PROTOCOL_ERROR);

    // 单独切换已就绪和正文尚未收全的帧，两种增量头都不能跨越许可边界。
    const uint8_t full[] = {0xe3,NI_DATA,7,9,2,0,0,0,0,0,0,0,40,0x51,0x72};
    for (unsigned split = sizeof(full) - 1; split <= sizeof(full); ++split)
    {
        for (unsigned encoding = 0; encoding < 2; ++encoding)
        {
            decoder.reset();
            CHECK_TEST(decoder.feed(full, split, used) == (split == sizeof(full) ? NI_READY : NI_NEED_MORE) && used == split);
            decoder.allowCompact(false);
            decoder.allowCompact(true);
            if (split < sizeof(full))
            {
                CHECK_TEST(decoder.feed(full + split, sizeof(full) - split, used) == NI_READY && used == sizeof(full) - split);
            }
            CHECK_TEST(decoder.peek(frame) == NI_READY && frame.m_requestId == 40 && frame.m_payloadSize == 2 &&
                frame.m_payload && !std::memcmp(frame.m_payload, full + sizeof(full) - 2, 2));
            CHECK_TEST(decoder.consume() == NI_OK);
            CHECK_TEST(decoder.feed(encoding == 0 ? next : jump, encoding == 0 ? sizeof(next) : sizeof(jump), used) == NI_PROTOCOL_ERROR);
        }
    }
    decoder.reset();
    CHECK_TEST(decoder.feed(base, sizeof(base), used) == NI_READY && decoder.consume() == NI_OK);
    decoder.allowCompact(true);
    CHECK_TEST(decoder.feed(next, sizeof(next), used) == NI_READY && decoder.peek(frame) == NI_READY && frame.m_requestId == 41);
    CHECK_TEST(decoder.consume() == NI_OK && decoder.feed(jump, sizeof(jump), used) == NI_READY &&
        decoder.peek(frame) == NI_READY && frame.m_requestId == 44);
    CHECK_TEST(decoder.consume() == NI_OK && decoder.feed(next, sizeof(next), used) == NI_READY &&
        decoder.peek(frame) == NI_READY && frame.m_requestId == 45);
    return 0;
}

/** 用独立的 NRP2 样本核对时间、父请求号、正文偏移及原地心跳回复
@return 成功为0
*/
static int Rpc()
{
    const uint8_t wire[] = {'N','R','P','2',0,0,1,44,1,2,3,4,5,6,7,8,0,255};
    uint8_t output[sizeof(wire)] = {};
    uint32_t n = 0;
    CHECK_TEST(NetInterface::rpcEncode(NI_REQUEST,300,0x05060708,0x01020304,wire + 16,2,output,sizeof(output),&n) == NI_OK &&
        n == sizeof(wire) && !std::memcmp(output,wire,n));
    CHECK_TEST(NetInterface::rpcEncode(NI_REQUEST,300,0,0,output + 1,2,output,sizeof(output),&n) == NI_INVALID_ARGUMENT &&
        n == 0 && !std::memcmp(output,wire,sizeof(wire)));
    uint32_t shared[5] = {42, 43, 44, 45, 46};
    CHECK_TEST(NetInterface::rpcEncode(NI_REQUEST, 1, 0, 0, nullptr, 0,
        reinterpret_cast<uint8_t*>(shared), sizeof(shared), shared) == NI_INVALID_ARGUMENT && shared[0] == 42);
    CHECK_TEST(NetInterface::rpcEncode(65536 + NI_REQUEST, 1, 0, 0, nullptr, 0,
        output, sizeof(output), &n) == NI_INVALID_ARGUMENT && n == 0);
    uint8_t adjacent[sizeof(wire) + 2] = {};
    adjacent[0] = 42;
    CHECK_TEST(NetInterface::rpcEncode(NI_REQUEST,1,0,0,adjacent,2,adjacent + 2,sizeof(wire),&n) == NI_OK &&
        n == sizeof(wire) && adjacent[18] == 42);
    const uint32_t header[] = {NI_REQUEST,7,1,0,9,sizeof(wire)};
    uint32_t view[5] = {};
    CHECK_TEST(NetInterface::rpcParse(header,6,wire,view) == NI_OK && view[0] == 300 && view[1] == 0x05060708 &&
        view[2] == 0x01020304 && view[3] == 16 && view[4] == 2);
    const NetInterfaceOrigin origin = {};
    uint32_t aligned[9] = {};
    uint8_t* local = reinterpret_cast<uint8_t*>(aligned);
    CHECK_TEST(NetInterfaceService::wrap(origin, wire + 16, 2, local, sizeof(aligned), n) == NI_OK && n == 34 &&
        !std::memcmp(local, "NOI2", 4) && local[32] == 0 && local[33] == 255);
    const uint32_t saved = aligned[0];
    CHECK_TEST(NetInterfaceService::wrap(origin, local, 2, local, sizeof(aligned), n) == NI_INVALID_ARGUMENT &&
        n == 0 && aligned[0] == saved && local[33] == 255);
    CHECK_TEST(NetInterfaceService::wrap(origin, nullptr, 0, local, sizeof(aligned), aligned[0]) == NI_INVALID_ARGUMENT &&
        aligned[0] == saved);
    uint32_t ping[] = {NI_PING,0,17,4,0,0};
    CHECK_TEST(NetInterface::rpcControl(ping,6,ping) == NI_READY && ping[0] == NI_PONG && ping[2] == 17 && ping[3] == 4);
    CHECK_TEST(NetInterface::rpcControl(ping,6,ping) == NI_OK && ping[0] == 0);
    return 0;
}

/** 检查收发额度独立，耗用后的窗口不能重新配置，协议错误是终态
@return 成功为0
*/
static int Credit()
{
    NetInterfaceCreditHandle h = nullptr;
    CHECK_TEST(NetInterface::creditCreate(&h) == NI_OK && h);
    uint32_t fields[2] = {};
    const bool ok = NetInterface::creditGet(h,fields) == NI_OK && fields[0] == 65536 && fields[1] == 0 &&
        NetInterface::creditConfigure(h,65537) == NI_INVALID_ARGUMENT && NetInterface::creditConfigure(h,131072) == NI_OK &&
        NetInterface::creditSent(h,7) == NI_OK && NetInterface::creditReceived(h,3) == NI_OK &&
        NetInterface::creditGet(h,fields) == NI_OK && fields[0] == 131065 && fields[1] == 3 &&
        NetInterface::creditConfigure(h,65536) == NI_INVALID_STATE && NetInterface::creditGrant(h,7) == NI_OK &&
        NetInterface::creditConsume(h,3) == NI_OK && NetInterface::creditGet(h,fields) == NI_OK && fields[0] == 131072 && fields[1] == 0 &&
        NetInterface::creditGrant(h,1) == NI_PROTOCOL_ERROR && NetInterface::creditGet(h,fields) == NI_PROTOCOL_ERROR;
    NetInterface::creditDestroy(h);
    CHECK_TEST(ok);
    return 0;
}

/** 手写 Zstandard 单字节重复块验证解压长度、协商开关和明文上限
@return 成功为0
*/
static int Compression()
{
    const uint8_t zipped[] = {'N','Z','C','2',0,0,4,0,0x28,0xb5,0x2f,0xfd,0x60,0,3,3,0x20,0,'A'};
    uint8_t wire[NET_INTERFACE_HEADER_SIZE + sizeof(zipped)];
    std::memcpy(wire,kFrame,NET_INTERFACE_HEADER_SIZE);
    wire[23] = sizeof(zipped);
    wire[31] = 1;
    std::memcpy(wire + NET_INTERFACE_HEADER_SIZE,zipped,sizeof(zipped));
    uint32_t used = 0;
    NetInterface plain(1024), limited(1023), decoder(1024);
    CHECK_TEST(plain.feed(wire,sizeof(wire),used) == NI_PROTOCOL_ERROR);
    limited.allowCompression(true);
    CHECK_TEST(limited.feed(wire,sizeof(wire),used) == NI_LIMIT_EXCEEDED);
    decoder.allowCompression(true);
    CHECK_TEST(decoder.feed(wire,sizeof(wire),used) == NI_READY && used == sizeof(wire));
    NetInterfaceFrame frame = {};
    CHECK_TEST(decoder.peek(frame) == NI_READY && frame.m_payload && frame.m_payloadSize == 1024);
    for (uint32_t i = 0; i < frame.m_payloadSize; ++i)
    {
        CHECK_TEST(frame.m_payload[i] == 'A');
    }
    return 0;
}

/** 两端业务握手选择共同能力和较小窗口，截止时刻未完成则超时
@return 成功为0
*/
static int Handshake()
{
    const uint32_t clientOptions[] = {0,1,7,32,16384,100};
    const uint32_t serverOptions[] = {1,0,3,16,4096,100};
    uint8_t local[32] = {1}, remote[32] = {3}, peer[32] = {};
    local[16] = 2;
    NetInterfaceHandshakeHandle a = nullptr, b = nullptr, late = nullptr;
    uint32_t fields[6] = {};
    const uint8_t* bytes = nullptr;
    const bool ok = NetInterface::handshakeCreate(clientOptions,6,local,32,1000,0,&a) == NI_NEED_MORE &&
        NetInterface::handshakeCreate(serverOptions,6,remote,32,1000,0,&b) == NI_NEED_MORE &&
        NetInterface::handshakeCreate(clientOptions,6,local,32,1000,0,&late) == NI_NEED_MORE &&
        NetInterface::handshakeProgress(late,1099,0) == NI_NEED_MORE && NetInterface::handshakeProgress(late,1100,0) == NI_TIMEOUT &&
        NetInterface::handshakePeekOutput(a,fields,&bytes) == NI_READY && fields[0] == NI_HELLO && fields[5] == 64 && bytes &&
        NetInterface::handshakeAccept(b,fields,bytes,1001,0) == NI_NEED_MORE && NetInterface::handshakeConsumeOutput(a) == NI_NEED_MORE &&
        NetInterface::handshakePeekOutput(b,fields,&bytes) == NI_READY && fields[0] == NI_HELLO_REPLY && bytes &&
        NetInterface::handshakeAccept(a,fields,bytes,1002,0) == NI_READY && NetInterface::handshakeConsumeOutput(b) == NI_READY &&
        NetInterface::handshakePeerInfo(a,fields,peer) == NI_READY && fields[0] == 1 && fields[1] == 3 &&
        fields[2] == 16 && fields[3] == 4096 && peer[0] == 3 && peer[16] == 2;
    NetInterface::handshakeDestroy(late);
    NetInterface::handshakeDestroy(b);
    NetInterface::handshakeDestroy(a);
    CHECK_TEST(ok);
    return 0;
}

/** 只给本用例生成可复现的 ClientHello；不会建立网络连接
@param [in,out] context 当前伪随机状态
@param [out] bytes 要填满的缓冲
@param [in] size 字节数
@return 0表示写完
*/
static int32_t HelloEntropy(void* context, uint8_t* bytes, uint32_t size)
{
    uint32_t& state = *static_cast<uint32_t*>(context);
    for (uint32_t i = 0; i < size; ++i)
    {
        state = state * 1664525U + 1013904223U;
        bytes[i] = static_cast<uint8_t>(state >> 24);
    }
    return 0;
}

std::string NetInterfaceTestGroupedDigits::do_grouping() const
{
    return std::string(1, 3);
}

/** 从当前记录中的完整 ClientHello 读取扩展，只接受指定的单个 DNS 名
@param [in] bytes TLS 客户端刚生成的首批输出
@param [in] size 可读取的字节数
@param [in] name 以 NUL 结尾的预期 ASCII 域名
@return server_name 扩展完整、唯一且与域名相同
*/
static bool HelloHasServerName(const uint8_t* bytes, size_t size, const char* name)
{
    if (bytes == nullptr || size < 9 || bytes[0] != 22 || bytes[5] != 1)
    {
        return false;
    }
    const size_t record = (size_t(bytes[3]) << 8) | bytes[4];
    const size_t hello = (size_t(bytes[6]) << 16) | (size_t(bytes[7]) << 8) | bytes[8];
    if (record > size - 5 || record < 4 || hello > record - 4 || hello < 35)
    {
        return false;
    }
    const size_t end = 9 + hello;
    size_t pos = 9 + 2 + 32;
    // 跳过协议定义的会话号、密码套件和压缩方法，各先读长度再核对剩余字节。
    const size_t widths[] = {1, 2, 1};
    for (size_t i = 0; i < 3; ++i)
    {
        const size_t width = widths[i];
        if (end - pos < width)
        {
            return false;
        }
        const size_t length = width == 1 ? bytes[pos] : (size_t(bytes[pos]) << 8) | bytes[pos + 1];
        pos += width;
        if (length > end - pos)
        {
            return false;
        }
        pos += length;
    }
    if (end - pos < 2 || ((size_t(bytes[pos]) << 8) | bytes[pos + 1]) != end - pos - 2)
    {
        return false;
    }
    pos += 2;
    const size_t expected = std::strlen(name);
    bool found = false;
    while (pos < end)
    {
        if (end - pos < 4)
        {
            return false;
        }
        const size_t type = (size_t(bytes[pos]) << 8) | bytes[pos + 1];
        const size_t length = (size_t(bytes[pos + 2]) << 8) | bytes[pos + 3];
        pos += 4;
        if (length > end - pos)
        {
            return false;
        }
        if (type == 0)
        {
            if (found || length != 5 + expected || ((size_t(bytes[pos]) << 8) | bytes[pos + 1]) != length - 2 ||
                bytes[pos + 2] != 0 || ((size_t(bytes[pos + 3]) << 8) | bytes[pos + 4]) != expected ||
                std::memcmp(bytes + pos + 5, name, expected) != 0)
            {
                return false;
            }
            found = true;
        }
        pos += length;
    }
    return found;
}

/** 产品号必须产生固定的 TLS 主机名，核对实际 ClientHello 的 SNI 条目
@return 成功为0
*/
static int ServiceName()
{
    uint32_t state = 1;
    NetInterfaceConnectionHandle connection = nullptr;
    int32_t detail = 0;
    const std::locale original = std::locale::global(std::locale(std::locale(),new NetInterfaceTestGroupedDigits));
    const int32_t result = NetInterface::connectionCreateServiceClient(12345,HelloEntropy,&state,5000,&connection,&detail);
    std::locale::global(original);

    const uint8_t* bytes = nullptr;
    uint32_t size = 0;
    const bool found = result == NET_INTERFACE_TLS_HANDSHAKE && NetInterface::connectionProgress(connection, 1, 0) >= 0 &&
        NetInterface::connectionPeekOutput(connection, &bytes, &size) >= 0 && HelloHasServerName(bytes, size, "service.12345");
    NetInterface::connectionDestroy(connection);
    CHECK_TEST(found);
    return 0;
}

/** 把本轮已生成的一段密文送到另一端，保留任何未被接纳的后缀
@param [in,out] from 密文提供端
@param [in,out] to 对端解密器
@return 是否继续驱动连接
*/
static bool TransferCiphertext(NetInterfaceConnection& from, NetInterfaceConnection& to)
{
    const uint8_t* bytes = nullptr;
    uint32_t size = 0, accepted = 0;
    if (from.peekOutput(bytes, size) < 0) { return false; }
    if (size == 0) { return true; }
    const uint32_t fragment = size < 97 ? size : 97;
    return bytes && to.feed(bytes, fragment, accepted) >= 0 && accepted <= fragment &&
        from.consumeOutput(accepted) >= 0;
}

/** 双端通过真实 TLS 交付明文；普通与控制池独立满池、归还后可重用
@return 成功为0
*/
static int TlsPools()
{
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM;
    static const uint8_t crt[] = TEST_SRV_CRT_RSA_SHA256_PEM;
    static const uint8_t key[] = TEST_SRV_KEY_RSA_PEM;
    uint32_t states[] = {7, 11};
    NetInterfaceTlsOptions options = {};
    options.authority = ca;
    options.authoritySize = sizeof(ca);
    options.hostname = "localhost";
    options.handshakeTimeoutMs = 10000;
    options.entropy = HelloEntropy;
    options.entropyContext = states;
    std::unique_ptr<NetInterfaceConnection> clientOwner(new NetInterfaceConnection(options));
    NetInterfaceConnection& client = *clientOwner;
    options.server = 1;
    options.hostname = nullptr;
    options.certificate = crt;
    options.certificateSize = sizeof(crt);
    options.privateKey = key;
    options.privateKeySize = sizeof(key);
    options.entropyContext = states + 1;
    std::unique_ptr<NetInterfaceConnection> serverOwner(new NetInterfaceConnection(options));
    NetInterfaceConnection& server = *serverOwner;
    NetConnectionPoolConfig pools;
    pools.m_dataRecords.initialCapacity = 1;
    pools.m_dataRecords.maximumCapacity = 1;
    pools.m_controlRecords.initialCapacity = 1;
    pools.m_controlRecords.maximumCapacity = 1;
    CHECK_TEST(client.configurePools(pools) == NI_OK);
    pools.m_dataRecords.initialCapacity = 2;
    CHECK_TEST(client.configurePools(pools) == NI_INVALID_ARGUMENT);
    uint32_t now = 0;
    while ((client.status() != NET_INTERFACE_TLS_READY || server.status() != NET_INTERFACE_TLS_READY) && now < 5000)
    {
        ++now;
        CHECK_TEST(client.progress(now) >= 0 && server.progress(now) >= 0 &&
            TransferCiphertext(client, server) && TransferCiphertext(server, client));
    }
    CHECK_TEST(client.status() == NET_INTERFACE_TLS_READY && server.status() == NET_INTERFACE_TLS_READY);
    pools.m_dataRecords.initialCapacity = 1;
    CHECK_TEST(client.configurePools(pools) == NI_INVALID_STATE);
    const uint8_t body[] = {0, 255, 42};
    for (uint32_t round = 1; round <= 2; ++round)
    {
        const NetInterfaceFrame request = {7, round, 9, NI_REQUEST, sizeof(body), body};
        const NetInterfaceFrame ping = {0, round, 0, NI_PING, 0, nullptr};
        CHECK_TEST(client.send(request) == NI_OK && client.send(request) == NI_LIMIT_EXCEEDED &&
            client.send(ping) == NI_OK && client.send(ping) == NI_LIMIT_EXCEEDED);
        uint32_t dataCount = 0, pingCount = 0;
        for (uint32_t step = 0; step < 1024 && dataCount + pingCount < 2; ++step)
        {
            ++now;
            CHECK_TEST(client.progress(now) >= 0 && TransferCiphertext(client, server) && server.progress(now) >= 0 &&
                TransferCiphertext(server, client));
            const NetInterfaceFrame* view = nullptr;
            const int32_t result = server.peek(view);
            CHECK_TEST(result == NI_NEED_MORE || result == NI_READY);
            if (result != NI_READY) { continue; }
            CHECK_TEST(view && view->m_requestId == round);
            if (view->m_kind == NI_PING)
            {
                CHECK_TEST(view->m_payloadSize == 0 && view->m_productId == 0 && ++pingCount == 1);
            }
            else
            {
                CHECK_TEST(view->m_kind == NI_REQUEST && view->m_productId == 7 && view->m_messageType == 9 &&
                    view->m_payloadSize == sizeof(body) && view->m_payload && !std::memcmp(view->m_payload, body, sizeof(body)));
                ++dataCount;
            }
            CHECK_TEST(server.consume() == NI_OK);
        }
        CHECK_TEST(dataCount == 1 && pingCount == 1);
    }
    // 所有已提交帧都完成后再排空一次，拒绝的请求不能延迟出现。
    for (uint32_t tail = 0; tail < 16; ++tail)
    {
        ++now;
        CHECK_TEST(client.progress(now) >= 0 && TransferCiphertext(client, server) &&
            server.progress(now) >= 0 && TransferCiphertext(server, client));
        const NetInterfaceFrame* view = nullptr;
        CHECK_TEST(server.peek(view) == NI_NEED_MORE && !view);
    }
    return 0;
}

/** 给五种尺寸各配置一个槽；一类耗尽仍可从其他类分配，归还只释放所属槽
@return 成功为0
*/
static int BufferClasses()
{
    NetBufferPoolConfig config;
    for (uint32_t i = 0; i < 5; ++i)
    {
        config.m_classes[i].initialCapacity = 1;
        config.m_classes[i].maximumCapacity = 1;
    }
    NetBufferPool pool(config);
    CHECK_TEST(pool.status() == NI_OK);
    NetBufferLease leases[5], spare;
    const uint32_t requests[] = {1,65,1025,16449,131073};
    const uint32_t capacities[] = {64,1024,16448,131072,1048704};
    CHECK_TEST(pool.allocate(0,spare) == NI_OK && !spare.data());
    for (uint32_t i = 0; i < 5; ++i)
    {
        CHECK_TEST(pool.allocate(requests[i],leases[i]) == NI_OK && leases[i].data() && leases[i].capacity() == capacities[i]);
        CHECK_TEST(pool.allocate(requests[i],spare) == NI_LIMIT_EXCEEDED && !spare.data());
    }
    for (uint32_t i = 0; i < 5; ++i)
    {
        leases[i].swap(spare);
        CHECK_TEST(!leases[i].data() && spare.capacity() == capacities[i]);
        spare.reset();
        CHECK_TEST(pool.allocate(capacities[i],leases[i]) == NI_OK && leases[i].capacity() == capacities[i]);
    }
    CHECK_TEST(pool.allocate(capacities[4] + 1,spare) == NI_INVALID_ARGUMENT);
    return 0;
}

/** 构造独立的节点样本，编号同时编码到节点名、端口和启动代次
@param [in] index 0至442的样本下标
@return 已填写大小和版本的公开值记录
*/
static NetInterfaceDirectoryNodeData DirectoryNode(uint32_t index)
{
    NetInterfaceDirectoryNodeData node = {};
    node.m_structSize = sizeof(node);
    node.m_version = 1;
    const char digits[] = "0123456789abcdef";
    std::memcpy(node.m_name, "node.00000000000000000000000000000000", 38);
    node.m_name[34] = digits[(index >> 8) & 15];
    node.m_name[35] = digits[(index >> 4) & 15];
    node.m_name[36] = digits[index & 15];
    std::memcpy(node.m_address, "::1", 4);
    node.m_port = 2000 + index;
    node.m_capacity = 1 + index;
    node.m_boot = (UINT64_C(1) << 32) + index;
    return node;
}

/** 验证超过64项的合法目录、完整输出、按数值去重及整体线预算
@return 全部独立预期满足时为0
*/
static int DirectoryNodes()
{
    std::vector<NetInterfaceDirectoryNodeData> nodes;
    for (uint32_t i = 0; i < 65; ++i)
    {
        nodes.push_back(DirectoryNode(i));
    }
    NetInterfaceDirectoryHandle raw = nullptr;
    CHECK_TEST(NetInterface::directoryCreateNodes(nodes.data(), 65, &raw) == NI_OK && raw);
    std::unique_ptr<NetInterfaceDirectoryOpaque, void (*)(NetInterfaceDirectoryHandle)> owner(raw, NetInterface::directoryDestroy);
    uint32_t size = 0;
    CHECK_TEST(NetInterface::directoryCache(raw, 1000, 0, nullptr, 0, &size) == NI_BUFFER_TOO_SMALL && size > 48);
    std::vector<uint8_t> wire(size);
    CHECK_TEST(NetInterface::directoryCache(raw, 1000, 0, wire.data(), size, &size) == NI_OK && size == wire.size());
    NetInterfaceDirectoryHandle restored = nullptr;
    CHECK_TEST(NetInterface::directoryCreateCache(wire.data(), size, 1001, 0, &restored) == NI_OK && restored);
    std::unique_ptr<NetInterfaceDirectoryOpaque, void (*)(NetInterfaceDirectoryHandle)> received(restored, NetInterface::directoryDestroy);
    std::vector<NetInterfaceDirectoryNodeData> output(65, DirectoryNode(100));
    uint32_t count = 0;
    CHECK_TEST(NetInterface::directoryCopyNodes(restored, output.data(), 64, &count) == NI_BUFFER_TOO_SMALL && count == 65);
    output.back().m_version = 2;
    CHECK_TEST(NetInterface::directoryCopyNodes(restored, output.data(), 65, &count) == NI_INVALID_ARGUMENT && count == 0);
    for (size_t i = 0; i < output.size(); ++i)
    {
        CHECK_TEST(output[i].m_port == 2100);
    }
    output.back().m_version = 1;
    CHECK_TEST(NetInterface::directoryCopyNodes(restored, output.data(), 65, &count) == NI_OK && count == 65);
    for (size_t i = 0; i < output.size(); ++i)
    {
        CHECK_TEST(!std::strcmp(output[i].m_name, nodes[i].m_name) && !std::strcmp(output[i].m_address, "::1") &&
            output[i].m_port == 2000 + i && output[i].m_capacity == 1 + i && output[i].m_boot == (UINT64_C(1) << 32) + i);
    }
    nodes[1].m_port = nodes[0].m_port;
    const char sameAddress[] = "0:0:0:0:0:0:0:1";
    std::memcpy(nodes[1].m_address, sameAddress, sizeof(sameAddress));
    CHECK_TEST(NetInterface::directoryCreateNodes(nodes.data(), 2, &raw) == NI_INVALID_ARGUMENT && !raw);
    nodes[1] = DirectoryNode(1);
    for (uint32_t i = 65; i < 443; ++i)
    {
        nodes.push_back(DirectoryNode(i));
    }
    CHECK_TEST(NetInterface::directoryCreateNodes(nodes.data(), 400, &raw) == NI_LIMIT_EXCEEDED && !raw);
    CHECK_TEST(NetInterface::directoryCreateNodes(nodes.data(), 443, &raw) == NI_LIMIT_EXCEEDED && !raw);
    return 0;
}

/** 验证NGC3缓存的时间和摘要，成功与失败均检查返回句柄
@return 全部通过时为0
*/
static int DirectoryCache()
{
    const NetInterfaceDirectoryNodeData node = DirectoryNode(0);
    NetInterfaceDirectoryHandle raw = nullptr;
    CHECK_TEST(NetInterface::directoryCreateNodes(&node, 1, &raw) == NI_OK && raw);
    std::unique_ptr<NetInterfaceDirectoryOpaque, void (*)(NetInterfaceDirectoryHandle)> owner(raw, NetInterface::directoryDestroy);
    const uint32_t saved = 1000000;
    uint32_t bytes = 0;
    CHECK_TEST(NetInterface::directoryCache(raw, saved, 0, nullptr, 0, &bytes) == NI_BUFFER_TOO_SMALL && bytes > 48);
    std::vector<uint8_t> cache(bytes);
    CHECK_TEST(NetInterface::directoryCache(raw, saved, 0, cache.data(), bytes, &bytes) == NI_OK && cache.size() == bytes);
    CHECK_TEST(!std::memcmp(cache.data(), "NGC3", 4));
    const uint32_t times[] = {saved - 300000, saved - 300001, saved + 2592000000U, saved + 2592000001U, saved};
    const int32_t expected[] = {NI_OK, NI_TIMEOUT, NI_OK, NI_TIMEOUT, NI_PROTOCOL_ERROR};
    for (uint32_t i = 0; i < 5; ++i)
    {
        if (i == 4)
        {
            cache.back() ^= 1;
        }
        NetInterfaceDirectoryHandle parsed = nullptr;
        const int32_t result = NetInterface::directoryCreateCache(cache.data(), bytes, times[i], 0, &parsed);
        std::unique_ptr<NetInterfaceDirectoryOpaque, void (*)(NetInterfaceDirectoryHandle)> release(parsed, NetInterface::directoryDestroy);
        CHECK_TEST(result == expected[i] && (parsed != nullptr) == (result == NI_OK));
        if (parsed)
        {
            uint32_t count = 0;
            CHECK_TEST(NetInterface::directoryCount(parsed, &count) == NI_OK && count == 1);
        }
    }
    return 0;
}

/** 使用两个来源的同一原号，分别验证完成、取消与到期结果的归属和唯一性
@return 全部预期满足时为0
*/
static int RequestLifecycle()
{
    NetInterfaceRequestsHandle raw = nullptr;
    CHECK_TEST(NetInterface::requestsCreate(4, &raw) == NI_OK && raw);
    std::unique_ptr<NetInterfaceRequestsOpaque, void (*)(NetInterfaceRequestsHandle)> owner(raw, NetInterface::requestsDestroy);
    CHECK_TEST(NetInterface::requestsAddEndpoint(raw, 1, 0, 0, 0, 1, 0) == NI_OK &&
        NetInterface::requestsAddEndpoint(raw, 2, 0, 0, 0, 1, 0) == NI_OK &&
        NetInterface::requestsAddEndpoint(raw, 3, 0, 1, 7, 1, 0) == NI_OK);
    uint32_t fields[] = {1,0,42,1,3,0,7,9,0,0,100,0,5};
    uint32_t out[16] = {}, zero[16] = {};
    CHECK_TEST(NetInterface::requestsBegin(raw, fields, 13, 0, out) == NI_OK && out[0] != 0 && out[1] == 0 &&
        out[2] == 1 && out[4] == 42 && out[5] == 1 && out[12] == 105);
    const uint32_t first = out[0];
    fields[0] = 2;
    CHECK_TEST(NetInterface::requestsBegin(raw, fields, 13, 0, out) == NI_OK && out[0] != first && out[2] == 2 &&
        NetInterface::requestsCount(raw) == 2 && NetInterface::requestsRemoveEndpoint(raw, 3, 0) == NI_INVALID_STATE);
    const uint32_t second = out[0];
    uint32_t reply[] = {2,0,first,0,7,9,101,0};
    CHECK_TEST(NetInterface::requestsComplete(raw, reply, 8, out) == NI_PROTOCOL_ERROR && !std::memcmp(zero, out, sizeof(out)) &&
        NetInterface::requestsCount(raw) == 2);
    reply[0] = 3;
    CHECK_TEST(NetInterface::requestsComplete(raw, reply, 8, out) == NI_READY && out[0] == first && out[2] == 1 &&
        NetInterface::requestsComplete(raw, reply, 8, out) == NI_NOT_FOUND && !std::memcmp(zero, out, sizeof(out)));
    CHECK_TEST(NetInterface::requestsTake(raw, 0, 104, 0, out) == NI_NOT_FOUND && NetInterface::requestsCount(raw) == 1 &&
        NetInterface::requestsTake(raw, 0, 105, 0, out) == NI_READY && out[0] == second && out[2] == 2 && out[4] == 42 && out[5] == 1 &&
        NetInterface::requestsTake(raw, 0, 105, 0, out) == NI_NOT_FOUND);
    CHECK_TEST(NetInterface::requestsCancel(raw, 1, 0, 43, 1, out) == NI_OK && !std::memcmp(zero, out, sizeof(out)));
    fields[2] = 43;
    fields[10] = 106;
    CHECK_TEST(NetInterface::requestsBegin(raw, fields, 13, 0, out) == NI_OK && out[2] == 2);
    fields[0] = 1;
    CHECK_TEST(NetInterface::requestsBegin(raw, fields, 13, 0, out) == NI_CANCELLED && !std::memcmp(zero, out, sizeof(out)) &&
        NetInterface::requestsCount(raw) == 1);
    CHECK_TEST(NetInterface::requestsCancel(raw, 2, 0, 43, 1, out) == NI_READY && out[2] == 2 && out[4] == 43 &&
        NetInterface::requestsCancel(raw, 2, 0, 43, 1, out) == NI_NOT_FOUND && NetInterface::requestsCount(raw) == 0);
    for (uint32_t id = 1; id <= 3; ++id)
    {
        CHECK_TEST(NetInterface::requestsRemoveEndpoint(raw, id, 0) == NI_OK);
    }
    return 0;
}

/** 填满指定的4097个物理请求槽，核对计数、新号重试及相同截止的稳定取出顺序
@return 全部预期满足时为0
*/
static int RequestCapacity()
{
    const uint32_t capacity = 4097;
    NetInterfaceRequests requests(capacity);
    NetInterfaceRequest value = {};
    CHECK_TEST(requests.status() == NI_OK && requests.addEndpoint(1, false, 0, 1) == NI_OK &&
        requests.addEndpoint(2, true, 7, 1) == NI_OK);
    for (uint32_t original = 1; original <= capacity; ++original)
    {
        CHECK_TEST(requests.begin(1, original, 2, 7, 9, 0, 100, 10, value) == NI_OK && value.m_original == original);
    }
    CHECK_TEST(requests.count() == capacity && requests.begin(1, capacity + 1, 2, 7, 9, 0, 100, 10, value) == NI_LIMIT_EXCEEDED &&
        value.m_route == 0 && requests.count() == capacity);
    CHECK_TEST(requests.cancel(1, 1, value) == NI_READY && value.m_original == 1 && requests.count() == capacity - 1);
    CHECK_TEST(requests.begin(1, capacity + 1, 2, 7, 9, 0, 100, 10, value) == NI_PROTOCOL_ERROR &&
        requests.beginNext(1, 2, 7, 9, 0, 100, 20, value) == NI_OK && value.m_original == capacity + 2);
    for (uint32_t original = 2; original <= capacity; ++original)
    {
        CHECK_TEST(requests.takeExpired(110, value) == NI_READY && value.m_original == original);
    }
    CHECK_TEST(requests.takeExpired(119, value) == NI_NOT_FOUND && requests.takeExpired(120, value) == NI_READY &&
        value.m_original == capacity + 2 && requests.count() == 0);
    CHECK_TEST(requests.removeEndpoint(2) == NI_OK && requests.removeEndpoint(1) == NI_OK);
    return 0;
}

/** 用固定最长 IPv6 地址及独立端口验证值数组，并确保错误不会部分发布
@return 全部预期满足时为0
*/
static int DiscoverySeeds()
{
    const std::string address = "2001:0db8:0000:0000:0000:0000:0000:0001";
    std::string seeds;
    for (uint32_t i = 0; i < 100; ++i)
    {
        // 构造十进制端口 2000 至 2099，避免受宿主 locale 影响。
        char port[] = "2000";
        port[2] = static_cast<char>('0' + i / 10);
        port[3] = static_cast<char>('0' + i % 10);
        if (!seeds.empty())
        {
            seeds += ',';
        }
        seeds += "[" + address + "]:" + port;
    }
    seeds += ",[2001:db8::1]:2000";
    uint32_t count = 0;
    CHECK_TEST(NetClientDiscovery::parseSeeds(seeds, nullptr, 0, count) == NI_BUFFER_TOO_SMALL && count == 100);
    NetClientDiscoveryPoint initial = {};
    initial.m_structSize = sizeof(initial);
    initial.m_version = 1;
    initial.m_port = 77;
    std::vector<NetClientDiscoveryPoint> output(100, initial);
    CHECK_TEST(NetClientDiscovery::parseSeeds(seeds, output.data(), 99, count) == NI_BUFFER_TOO_SMALL && count == 100);
    for (size_t i = 0; i < output.size(); ++i)
    {
        CHECK_TEST(output[i].m_port == 77 && output[i].m_address[0] == 0);
    }
    CHECK_TEST(NetClientDiscovery::parseSeeds(seeds, output.data(), 100, count) == NI_OK && count == 100);
    for (size_t i = 0; i < output.size(); ++i)
    {
        CHECK_TEST(output[i].m_port == 2000 + i && !std::memcmp(output[i].m_address, address.c_str(), sizeof(initial.m_address)));
    }
    seeds += ",[" + address + "1]:9999";
    output.assign(100, initial);
    CHECK_TEST(NetClientDiscovery::parseSeeds(seeds, output.data(), 100, count) == NI_INVALID_ARGUMENT && count == 0);
    for (size_t i = 0; i < output.size(); ++i)
    {
        CHECK_TEST(output[i].m_port == 77 && output[i].m_address[0] == 0);
    }
    return 0;
}

/** 验证分片跨越初始截止时间时的合并写盘、提交确认与无进度超时
@return 所有阶段及字节均符合预期时为0
*/
static int FileBatchProgress()
{
    uint8_t content[1537];
    for (size_t i = 0; i < sizeof(content); ++i)
    {
        content[i] = static_cast<uint8_t>(i % 251);
    }
    // 上述固定字节串的 SHA-256 期望，由独立工具计算。
    const uint8_t digest[] = {0x44,0xaf,0x9c,0x37,0x20,0x3c,0x91,0x84,0xad,0x00,0xae,0x02,0x52,0x02,0xdf,0xb9,
        0xa4,0x93,0x84,0x2e,0x80,0xfd,0x1e,0x88,0xfc,0xd1,0xd3,0xe5,0xee,0x0e,0xd3,0xe3};
    NetInterfaceFileInfo info = {};
    info.direction = 1;
    info.state = NI_FILE_TRANSFERRING;
    info.chunk = 1024;
    info.window = 2;
    info.total = sizeof(content);
    info.stream = 61;
    info.expiresUtc = 3600;
    info.transfer[0] = 1;
    info.token[0] = 2;
    std::memcpy(info.hash, digest, sizeof(digest));
    NetInterfaceFileFlow sender(info, 7, true, 100, 1000), receiver(info, 7, false, 100, 1000);
    NetInterfaceFileFlow stalled(info, 7, false, 100, 1000);
    CHECK_TEST(receiver.setWriteBatchChunks(3) == NI_INVALID_ARGUMENT && receiver.setWriteBatchChunks(2) == NI_OK &&
        stalled.setWriteBatchChunks(2) == NI_OK);

    NetInterfaceFrame data = {}, ack = {};
    const uint8_t* borrowed = nullptr;
    uint32_t size = 0;
    uint64_t offset = 0;
    CHECK_TEST(sender.progress(800) == NI_OK && receiver.progress(800) == NI_OK && stalled.progress(800) == NI_OK);
    CHECK_TEST(sender.readRequest(offset, size) == NI_READY && offset == 0 && size == 1024 &&
        sender.produced(0, content, 1024) == NI_OK && sender.peekOutput(data) == NI_READY && data.m_kind == NI_DATA &&
        receiver.accept(data) == NI_OK && stalled.accept(data) == NI_OK);
    CHECK_TEST(receiver.writeRequest(offset, borrowed, size) == NI_NEED_MORE && !borrowed && size == 0 &&
        receiver.peekOutput(ack) == NI_NEED_MORE);
    CHECK_TEST(stalled.progress(1700) == NI_OK && stalled.accept(data) == NI_OK && stalled.progress(1799) == NI_OK &&
        stalled.progress(1800) == NI_TIMEOUT && stalled.progress(1801) == NI_TIMEOUT);
    CHECK_TEST(sender.consumeOutput() == NI_OK && sender.progress(1500) == NI_OK && receiver.progress(1500) == NI_OK);
    CHECK_TEST(sender.readRequest(offset, size) == NI_READY && offset == 1024 && size == 513 &&
        sender.produced(offset, content + 1024, size) == NI_OK && sender.peekOutput(data) == NI_READY &&
        receiver.accept(data) == NI_OK && sender.consumeOutput() == NI_OK);
    CHECK_TEST(receiver.writeRequest(offset, borrowed, size) == NI_READY && offset == 0 && size == sizeof(content) &&
        borrowed && !std::memcmp(borrowed, content, sizeof(content)) && receiver.peekOutput(ack) == NI_NEED_MORE);
    NetInterfaceFileInfo snapshot = {};
    int32_t status = NI_OK;
    receiver.snapshot(snapshot, status);
    CHECK_TEST(status == NI_OK && snapshot.offset == 0 && receiver.written(0, size, NI_OK) == NI_OK &&
        receiver.peekOutput(ack) == NI_READY && ack.m_kind == NI_DATA_ACK && sender.accept(ack) == NI_OK &&
        receiver.consumeOutput() == NI_OK);
    CHECK_TEST(sender.peekOutput(data) == NI_READY && receiver.accept(data) == NI_OK && sender.consumeOutput() == NI_OK &&
        receiver.takeCommit() == NI_READY && receiver.takeCommit() == NI_NEED_MORE && receiver.peekOutput(ack) == NI_NEED_MORE);
    CHECK_TEST(receiver.committed(NI_OK) == NI_READY && receiver.peekOutput(ack) == NI_READY &&
        ack.m_kind == NI_DATA_ACK && sender.accept(ack) == NI_OK && receiver.consumeOutput() == NI_OK);
    sender.snapshot(snapshot, status);
    CHECK_TEST(status == NI_READY && snapshot.state == NI_FILE_COMPLETED && snapshot.offset == sizeof(content) && snapshot.stream == 0);
    receiver.snapshot(snapshot, status);
    CHECK_TEST(status == NI_READY && snapshot.state == NI_FILE_COMPLETED && snapshot.offset == sizeof(content) && snapshot.stream == 0 &&
        receiver.peekOutput(ack) == NI_NEED_MORE);
    return 0;
}

#if defined(_MSC_VER) && defined(_DEBUG)
// 调试 CRT 的原分配钩子，在单线程专项结束后立即恢复。
static _CRT_ALLOC_HOOK previousAllocationHook = nullptr;
// 当前用例指定的目标分配大小，单位字节。
static size_t rejectedSize = 0;
// 为验证标准库缩小容量后重试的路径，拒绝所有达到目标大小的分配。
static bool refuseAtLeast = false;
// 此次作用域是否已拒绝过目标分配。
static bool allocationRefused = false;

/** 按用例指定的大小拒绝首次分配，或持续拒绝达到阈值的普通块
@param [in] operation 分配、重分配或释放操作
@param [in] data CRT 提供的原内存地址
@param [in] size 请求字节数
@param [in] block CRT 内存块类别
@param [in] request 分配序号
@param [in] file 可空的来源文件名
@param [in] line 来源行号
@return 目标分配返回0，其余交给原钩子
*/
static int __cdecl FilterTestAllocation(int operation, void* data, size_t size, int block, long request,
    const unsigned char* file, int line)
{
    if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK &&
        (refuseAtLeast ? size >= rejectedSize : !allocationRefused && size == rejectedSize))
    {
        allocationRefused = true;
        return 0;
    }
    return previousAllocationHook ? previousAllocationHook(operation, data, size, block, request, file, line) : 1;
}
#endif

/** 空文件直接提交，最终确认可在消费前再次读取
@return 双端终态和确认帧均符合约定时为0
*/
static int EmptyFileCommit()
{
    const uint8_t digest[] = {0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,
        0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55};
    NetInterfaceFileInfo info = {};
    info.direction = 1;
    info.state = NI_FILE_TRANSFERRING;
    info.chunk = 1024;
    info.window = 1;
    info.stream = 62;
    info.expiresUtc = 3600;
    info.transfer[0] = 1;
    info.token[0] = 2;
    std::memcpy(info.hash, digest, sizeof(digest));
    NetInterfaceFileFlow sender(info, 7, true, 100, 1000), receiver(info, 7, false, 100, 1000);
    NetInterfaceFrame end = {}, ack = {}, again = {};
    CHECK_TEST(sender.peekOutput(end) == NI_READY && receiver.accept(end) == NI_OK && sender.consumeOutput() == NI_OK);
    CHECK_TEST(receiver.takeCommit() == NI_READY && receiver.committed(NI_OK) == NI_READY);
#if defined(_MSC_VER) && defined(_DEBUG)
    rejectedSize = NET_INTERFACE_FILE_ACK_SIZE;
    allocationRefused = false;
    previousAllocationHook = _CrtSetAllocHook(FilterTestAllocation);
    const int32_t failure = receiver.peekOutput(ack);
    _CrtSetAllocHook(previousAllocationHook);
    CHECK_TEST(allocationRefused && failure == NI_NO_MEMORY && !ack.m_payload && ack.m_payloadSize == 0);
#endif
    NetInterfaceFileInfo snapshot = {};
    int32_t status = NI_OK;
    receiver.snapshot(snapshot, status);
    CHECK_TEST(status == NI_READY && snapshot.state == NI_FILE_COMPLETED && snapshot.offset == 0);
    CHECK_TEST(receiver.takeCommit() == NI_NEED_MORE && receiver.peekOutput(ack) == NI_READY &&
        ack.m_kind == NI_DATA_ACK && ack.m_payload && ack.m_payloadSize == NET_INTERFACE_FILE_ACK_SIZE);
    CHECK_TEST(receiver.peekOutput(again) == NI_READY && again.m_payload == ack.m_payload &&
        sender.accept(again) == NI_OK && receiver.consumeOutput() == NI_OK && receiver.peekOutput(ack) == NI_NEED_MORE);
    sender.snapshot(snapshot, status);
    CHECK_TEST(status == NI_READY && snapshot.state == NI_FILE_COMPLETED && snapshot.offset == 0);
    return 0;
}

/** 通过公开消息句柄验证查询、独立分组、内存不足和 protobuf 往返
@return 字段类型、完整字节和失败后的原值均符合预期时为0
*/
static int MessageFields()
{
    NetInterfaceMessageHandle raw = nullptr;
    CHECK_TEST(NetInterface::messageCreate(&raw) == NI_OK && raw);
    std::unique_ptr<NetInterfaceMessageOpaque, void (*)(NetInterfaceMessageHandle)> owner(raw, NetInterface::messageDestroy);
    uint8_t input[513], output[sizeof(input)];
    for (size_t i = 0; i < sizeof(input); ++i)
    {
        input[i] = static_cast<uint8_t>(i % 251);
    }
    CHECK_TEST(NetInterface::messageSetValue(raw, 1, 17, 5, input, sizeof(input)) == NI_OK &&
        NetInterface::messageSetValue(raw, 2, 17, 4, nullptr, 0) == NI_OK &&
        NetInterface::messageSetCode(raw, 73, input, 3) == NI_OK);
    int32_t type = -1;
    uint32_t required = 0;
#if defined(_MSC_VER) && defined(_DEBUG)
    rejectedSize = sizeof(input);
    allocationRefused = false;
    previousAllocationHook = _CrtSetAllocHook(FilterTestAllocation);
#endif
    const int32_t query = NetInterface::messageGetValue(raw, 1, 17, &type, nullptr, 0, &required);
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetAllocHook(previousAllocationHook);
    CHECK_TEST(!allocationRefused);
#endif
    CHECK_TEST(query == NI_BUFFER_TOO_SMALL && required == sizeof(input) && type == 5);
    std::memset(output, 0xa5, sizeof(output));
    CHECK_TEST(NetInterface::messageGetValue(raw, 1, 17, &type, output, sizeof(output) - 1, &required) == NI_BUFFER_TOO_SMALL);
    for (size_t i = 0; i < sizeof(output); ++i)
    {
        CHECK_TEST(output[i] == 0xa5);
    }
#if defined(_MSC_VER) && defined(_DEBUG)
    rejectedSize = sizeof(output);
    allocationRefused = false;
    previousAllocationHook = _CrtSetAllocHook(FilterTestAllocation);
    const int32_t updated = NetInterface::messageSetValue(raw, 1, 17, 4, output, sizeof(output));
    _CrtSetAllocHook(previousAllocationHook);
    CHECK_TEST(allocationRefused && updated == NI_NO_MEMORY);
    rejectedSize = sizeof(input);
    allocationRefused = false;
    refuseAtLeast = true;
    previousAllocationHook = _CrtSetAllocHook(FilterTestAllocation);
    const int32_t setCode = NetInterface::messageSetCode(raw, 74, input, sizeof(input));
    _CrtSetAllocHook(previousAllocationHook);
    refuseAtLeast = false;
    CHECK_TEST(allocationRefused && setCode == NI_NO_MEMORY);
#endif
    int32_t code = 0;
    CHECK_TEST(NetInterface::messageGetCode(raw, &code, output, sizeof(output), &required) == NI_OK &&
        code == 73 && required == 3 && !std::memcmp(input, output, required));
    CHECK_TEST(NetInterface::messageSetValue(raw, 1, 17, 1, input, 3) == NI_INVALID_ARGUMENT &&
        NetInterface::messageSetValue(raw, 1, 17, 256, input, 1) == NI_INVALID_ARGUMENT &&
        NetInterface::messageGetValue(raw, 1, 17, &type, output, sizeof(output), &required) == NI_OK &&
        type == 5 && required == sizeof(input) && !std::memcmp(input, output, sizeof(input)));
    uint8_t wire[1024];
    uint32_t size = 0;
    CHECK_TEST(NetInterface::messageEncode(raw, wire, sizeof(wire), &size) == NI_OK && size > sizeof(input));
    // 两个字段、一次状态写入及即将加入的第三个字段恰好用尽累计修改预算。
    const uint32_t remaining = NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT - 3 * 64 - sizeof(input) - (32 + 3);
    std::vector<uint8_t> lastField(remaining, 0x5a);
    CHECK_TEST(NetInterface::messageSetValue(raw, 3, 18, 5, lastField.data(), remaining) == NI_OK &&
        NetInterface::messageSetValue(raw, 3, 19, 4, nullptr, 0) == NI_LIMIT_EXCEEDED);
    CHECK_TEST(NetInterface::messageClear(raw) == NI_OK && NetInterface::messageParse(raw, wire, size) == NI_OK);
    std::memset(output, 0, sizeof(output));
    CHECK_TEST(NetInterface::messageGetValue(raw, 1, 17, &type, output, sizeof(output), &required) == NI_OK &&
        type == 5 && required == sizeof(input) && !std::memcmp(input, output, sizeof(input)));
    CHECK_TEST(NetInterface::messageGetValue(raw, 2, 17, &type, nullptr, 0, &required) == NI_OK && type == 4 && required == 0);
    CHECK_TEST(NetInterface::messageGetValue(raw, 3, 17, &type, nullptr, 0, &required) == NI_NOT_FOUND && type == 0 && required == 0);
    CHECK_TEST(NetInterface::messageParse(raw, nullptr, 0) == NI_OK &&
        NetInterface::messageGetValue(raw, 1, 17, &type, nullptr, 0, &required) == NI_NOT_FOUND);
    return 0;
}

/** 检查 OPEN 可选窗口字段的分组、类型、范围以及配置失败后的额度
@return 缺省、拒绝和实际记账均与预期一致时为0
*/
static int CreditNegotiation()
{
    ProtoMessage message;
    uint32_t bytes = 0;
    const int32_t values[] = {65536, 1048576, 65535, 1048577, 65537, -1};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
    {
        message.setKeyValue(NI_GATEWAY_WINDOW, Variant(values[i]), 7);
        const int32_t result = NetInterfaceCredit::window(message, bytes, 7);
        if (i < 2)
        {
            CHECK_TEST(result == NI_OK && bytes == static_cast<uint32_t>(values[i]));
        }
        else
        {
            CHECK_TEST(result == NI_PROTOCOL_ERROR);
        }
    }
    CHECK_TEST(NetInterfaceCredit::window(message, bytes) == NI_OK && bytes == 65536);
    NetInterfaceCreditHandle raw = nullptr;
    CHECK_TEST(NetInterface::creditCreate(&raw) == NI_OK && raw);
    std::unique_ptr<NetInterfaceCreditOpaque, void (*)(NetInterfaceCreditHandle)> owner(raw, NetInterface::creditDestroy);
    CHECK_TEST(NetInterface::creditConfigure(raw, 131072) == NI_OK);
    std::string missingWire;
    CHECK_TEST(message.toString(missingWire) && !missingWire.empty());
    uint8_t invalid[513] = {};
    CHECK_TEST(message.setBytes(NI_GATEWAY_WINDOW, invalid, sizeof(invalid), Variant::BLOB));
#if defined(_MSC_VER) && defined(_DEBUG)
    rejectedSize = sizeof(invalid);
    allocationRefused = false;
    previousAllocationHook = _CrtSetAllocHook(FilterTestAllocation);
#endif
    const int32_t result = NetInterfaceCredit::window(message, bytes);
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetAllocHook(previousAllocationHook);
#endif
    CHECK_TEST(result == NI_PROTOCOL_ERROR);
#if defined(_MSC_VER) && defined(_DEBUG)
    CHECK_TEST(!allocationRefused);
#endif
    std::string wire;
    CHECK_TEST(message.toString(wire) && !wire.empty());
    uint32_t credit[2] = {};
    CHECK_TEST(NetInterface::creditConfigureMessage(raw, reinterpret_cast<const uint8_t*>(wire.data()),
        static_cast<uint32_t>(wire.size())) == NI_PROTOCOL_ERROR);
    CHECK_TEST(NetInterface::creditGet(raw, credit) == NI_OK && credit[0] == 131072 && credit[1] == 0);
    CHECK_TEST(NetInterface::creditConfigureMessage(raw, reinterpret_cast<const uint8_t*>(missingWire.data()),
        static_cast<uint32_t>(missingWire.size())) == NI_OK);
    CHECK_TEST(NetInterface::creditGet(raw, credit) == NI_OK && credit[0] == 65536 && credit[1] == 0);
    message.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(262144)));
    CHECK_TEST(message.toString(wire));
    const uint8_t* input = reinterpret_cast<const uint8_t*>(wire.data());
    const uint32_t length = static_cast<uint32_t>(wire.size());
    CHECK_TEST(NetInterface::creditConfigureMessage(raw, input, length) == NI_OK && NetInterface::creditSent(raw, 1) == NI_OK);
    CHECK_TEST(NetInterface::creditConfigureMessage(raw, input, length) == NI_INVALID_STATE &&
        NetInterface::creditGet(raw, credit) == NI_OK && credit[0] == 262143 && credit[1] == 0);
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
        {CaseFrame, false, Frame},
        {CaseCompact, false, Compact},
        {CaseDigest, false, Digest},
        {CaseMessage, false, Message},
        {CaseRepeatedOperations, true, RepeatedOperations},
        {CaseFrameBoundaries, false, FrameBoundaries},
        {CaseCompactSequence, false, CompactSequence},
        {CaseRpc, false, Rpc},
        {CaseCredit, false, Credit},
        {CaseCompression, false, Compression},
        {CaseHandshake, false, Handshake},
        {CaseServiceName, false, ServiceName},
        {CaseTlsPools, false, TlsPools},
        {CaseBufferClasses, false, BufferClasses},
        {CaseDirectoryNodes, false, DirectoryNodes},
        {CaseDirectoryCache, false, DirectoryCache},
        {CaseRequestLifecycle, false, RequestLifecycle},
        {CaseRequestCapacity, true, RequestCapacity},
        {CaseDiscoverySeeds, false, DiscoverySeeds},
        {CaseFileBatchProgress, false, FileBatchProgress},
        {CaseEmptyFileCommit, false, EmptyFileCommit},
        {CaseMessageFields, false, MessageFields},
        {CaseCreditNegotiation, false, CreditNegotiation}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}