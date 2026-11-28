#include "NetInterfaceTest.h"
#include "NetInterface/NetGatewayClientTiming.h"
#if defined(NET_INTERFACE_SDK_STATIC_LINK) || defined(__EMSCRIPTEN__)
#include "NetInterface/NetEndpointFileStream.h"
#include "NetInterface/NetInterfaceCompact.h"
#include "NetInterface/NetSenderSubmissionState.h"
#endif
#include "PlatformTest/PlatformTestAPI.h"
#include "Compress/CompressAPI.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
#include "CEncodeDecode/CEncodeDecodeMbedConfig.h"
#include <mbedtls/oid.h>
#include <mbedtls/x509_csr.h>
#include <cstdio>
#include <chrono>
#include <algorithm>
#include <ctime>
#include <atomic>
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
#include <thread>
#endif
#include <cstring>
#include <exception>
#include <new>
#include <stdexcept>
#include <vector>
#include <memory>
#if defined(_WIN32) && defined(_MSC_VER) && _MSC_VER <= 1800
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif
#include "../../NetGatewayTest/NetGatewayTest/NetTestTlsCertificateData.h"

#define CHECK_TEST(expr) do { if (!(expr)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (false)

/** 按协议使用的统一时基构造测试时间点，避免v120墙钟与运行毫秒混算
@return 当前协议单调时间点，用于本测试的期限和经过时间
*/
static std::chrono::steady_clock::time_point TimerProbeNow()
{
    return std::chrono::steady_clock::time_point(std::chrono::milliseconds(NetInterface::monotonicMilliseconds()));
}

// 手工构造的 NIF2 网络字节序样本，正文含零字节。
static const uint8_t kFrame[] = {'N','I','F','2',0,2,0,3,0,0,0,7,1,2,3,4,5,6,7,8,0,0,0,3,0,0,0,9,0,0,0,0,0,255,17};

// 正常分片，仅包含 5 字节 TLS 记录头的前四字节。
static const uint8_t kTlsHeaderPrefix[] = {22, 3, 3, 0};

// 管理协议固定为 UINT64 通道 1 的八个小端正文字节。
static const uint8_t kWireChannelOne[] = {1, 0, 0, 0, 0, 0, 0, 0};

/** 按测试对端的字节常量检查字段，分别验证类型、实际长度与每个字节
@param [in] message 已解包的管理消息
@param [in] field 字段键
@param [in] type 期望的 Variant 类型
@param [in] expected 独立的预期线序字节
@param [in] group 记录组，默认全局
@return 整个字段相同时为 true
*/
template<size_t Size>
static bool HasWireField(const ProtoMessage& message, int32_t field, Variant::VariantType type,
    const uint8_t (&expected)[Size], int32_t group = PROTO_GLOBAL)
{
    const Variant* item = message.findValue(field, group);
    return item && item->type() == type && item->size() == Size && item->data() &&
        std::memcmp(item->data(), expected, Size) == 0;
}

/** 用小端字节写入运行时才确定的测试对端 INT32 字段，负数按 32 位补码发送
@param [in,out] message 接收字段的消息
@param [in] field 字段键
@param [in] value 测试场景确定的数值
@return 字段写入成功
*/
static bool SetWireInt32(ProtoMessage& message, int32_t field, int32_t value)
{
    const uint32_t bits = static_cast<uint32_t>(value);
    const uint8_t bytes[] = {static_cast<uint8_t>(bits), static_cast<uint8_t>(bits >> 8),
        static_cast<uint8_t>(bits >> 16), static_cast<uint8_t>(bits >> 24)};
    return message.setBytes(field, bytes, sizeof(bytes), Variant::INT32);
}

/** 用独立NTW3字节校验编码、接收和重试，所有输入及输出由调用者实际分配
@return 重叠参数被拒绝且未扰动解码器，独立或相邻参数得到完整记录时为0
*/
static int TunnelBuffers()
{
    const uint8_t wire[] = {'N','T','W','3',3,3,0,0,0,0,0,7,1,2,3,4,5,6,7,8,0,0,0,4,11,0,33,44};
    uint32_t body = 0;
    std::memcpy(&body, wire + 24, sizeof(body));
    const uint32_t saved = body;
    uint32_t fields[] = {NI_TUNNEL_BYTES, 7, 0x05060708, 0x01020304, 4};
    NetInterfaceTunnelRecord record = {NI_TUNNEL_BYTES, 7, UINT64_C(0x0102030405060708),
        reinterpret_cast<const uint8_t*>(&body), 4};
    for (unsigned api = 0; api < 2; ++api)
    {
        uint32_t storage[8];
        std::memset(storage, 0xa5, sizeof(storage));
        uint8_t* out = reinterpret_cast<uint8_t*>(storage);
        for (unsigned position = 0; position < 3; ++position)
        {
            uint32_t& length = position == 0 ? body : (position == 1 ? storage[0] : (api ? fields[4] : record.m_size));
            const int32_t status = api ? NetInterface::tunnelEncode(fields, record.m_data, out, 4, &length) :
                NetInterfaceTunnel::encode(record, out, 4, length);
            CHECK_TEST(status == NI_INVALID_ARGUMENT && body == saved && fields[4] == 4 && record.m_size == 4);
            for (size_t i = 0; i < sizeof(storage); ++i) { CHECK_TEST(out[i] == 0xa5); }
        }
        const int32_t status = api ? NetInterface::tunnelEncode(fields, record.m_data, out, sizeof(wire), &storage[7]) :
            NetInterfaceTunnel::encode(record, out, sizeof(wire), storage[7]);
        CHECK_TEST(status == NI_OK && storage[7] == sizeof(wire) && !std::memcmp(wire, out, sizeof(wire)));
    }
    NetInterfaceTunnelHandle tunnel = nullptr;
    CHECK_TEST(NetInterface::tunnelCreate(&tunnel) == NI_OK && tunnel);
    std::unique_ptr<NetInterfaceTunnelOpaque, void (*)(NetInterfaceTunnelHandle)> owner(tunnel, NetInterface::tunnelDestroy);
    uint32_t input[8] = {};
    std::memcpy(input, wire, sizeof(wire));
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(input);
    CHECK_TEST(NetInterface::tunnelFeed(tunnel, bytes, sizeof(wire), &input[3]) == NI_INVALID_ARGUMENT &&
        !std::memcmp(input, wire, sizeof(wire)));
    CHECK_TEST(NetInterface::tunnelFeed(tunnel, bytes, sizeof(wire), &input[7]) == NI_READY && input[7] == sizeof(wire));
    const uint8_t* payload = nullptr;
    TestBorrowedView shared = {bytes, {11,22,33,44,55,66}};
    uint32_t* overlapping = reinterpret_cast<uint32_t*>(&shared);
    CHECK_TEST(NetInterface::tunnelPeek(tunnel, overlapping, &shared.m_pointer) == NI_INVALID_ARGUMENT &&
        shared.m_pointer == bytes && shared.m_padding[0] == 11 && shared.m_padding[1] == 22);
    CHECK_TEST(NetInterface::connectionPeekOutput(nullptr, &shared.m_pointer, overlapping) == NI_INVALID_ARGUMENT &&
        shared.m_pointer == bytes);
    uint32_t identityFields[8] = {37,38,39,40,41,42,43,44};
    const uint8_t* certificate = bytes;
    CHECK_TEST(NetInterface::linkPeerInfo(nullptr, identityFields, &shared.m_pointer, &certificate, &shared.m_pointer) ==
        NI_INVALID_ARGUMENT && certificate == bytes && shared.m_pointer == bytes && identityFields[0] == 37);
    CHECK_TEST(NetInterface::handshakePeerInfo(nullptr, identityFields, reinterpret_cast<uint8_t*>(identityFields)) ==
        NI_INVALID_ARGUMENT && identityFields[0] == 37 && identityFields[7] == 44);
    uint32_t received[5] = {};
    CHECK_TEST(NetInterface::tunnelPeek(tunnel, received, &payload) == NI_READY &&
        !std::memcmp(fields, received, sizeof(fields)) && payload && !std::memcmp(payload, wire + 24, 4));
    CHECK_TEST(NetInterface::tunnelConsume(tunnel) == NI_OK && NetInterface::tunnelFinish(tunnel) == NI_OK);
    return 0;
}

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
    const uint32_t fields[] = {NI_REQUEST, 7, 0x05060708U, 0x01020304U, 9, 3};
    std::memset(out, 0xa5, sizeof(out));
    size = 0;
    CHECK_TEST(NetInterface::encode(&input, out, sizeof(out), &size) == NI_OK && size == sizeof(kFrame) &&
        std::memcmp(out, kFrame, sizeof(kFrame)) == 0);
    std::memset(out, 0xa5, sizeof(out));
    size = 0;
    CHECK_TEST(NetInterface::encodeFields(fields, input.m_payload, out, sizeof(out), &size) == NI_OK && size == sizeof(kFrame) &&
        std::memcmp(out, kFrame, sizeof(kFrame)) == 0);
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

/** 检查宿主回调的长度、失败清除和前后保护字节
@return 每次只访问声明的有效输出且按约定转交错误时为0
*/
static int EntropyOutput()
{
    const uint32_t limit = 8192;
    uint8_t storage[limit + 3];
    uint32_t calls = 0;
    NetInterfaceEntropyFunction fill = [](void* context, uint8_t* out, uint32_t size) -> int32_t {
        ++*static_cast<uint32_t*>(context);
        for (uint32_t i = 0; i < size; ++i) { out[i] = static_cast<uint8_t>(i + 7); }
        return 0;
    };
    std::memset(storage, 0xa5, sizeof(storage));
    CHECK_TEST(NetInterface::entropyBytes(fill, &calls, nullptr, 0) == NI_OK && calls == 0 &&
        NetInterface::entropyBytes(fill, &calls, nullptr, 1) == NI_INVALID_ARGUMENT && calls == 0 &&
        NetInterface::entropyBytes(fill, &calls, storage + 1, limit + 1) == NI_INVALID_ARGUMENT && calls == 0);
    for (size_t i = 0; i < sizeof(storage); ++i) { CHECK_TEST(storage[i] == 0xa5); }
    CHECK_TEST(NetInterface::entropyBytes(fill, &calls, storage + 1, limit) == NI_OK && calls == 1);
    for (uint32_t i = 0; i < limit; ++i) { CHECK_TEST(storage[i + 1] == static_cast<uint8_t>(i + 7)); }
    CHECK_TEST(storage[0] == 0xa5 && storage[limit + 1] == 0xa5 && storage[limit + 2] == 0xa5);
    NetInterfaceEntropyFunction partial = [](void* context, uint8_t* out, uint32_t) -> int32_t {
        ++*static_cast<uint32_t*>(context);
        out[0] = 0x70;
        return -1;
    };
    // 模拟宿主在合法输出区写入首字节后抛出异常，context 记录实际调用次数。
    NetInterfaceEntropyFunction throwing = [](void* context, uint8_t* out, uint32_t) -> int32_t {
        ++*static_cast<uint32_t*>(context);
        out[0] = 0x80;
        throw NI_INVALID_ARGUMENT;
    };
    const NetInterfaceEntropyFunction failures[] = {partial, throwing, nullptr};
    for (size_t i = 0; i < sizeof(failures) / sizeof(failures[0]); ++i)
    {
        uint8_t guarded[] = {1, 2, 3, 4, 5};
        const uint8_t expected[] = {1, 0, 0, 0, 5};
        const uint32_t expectedCalls = calls + (failures[i] ? 1U : 0U);
        CHECK_TEST(NetInterface::entropyBytes(failures[i], &calls, guarded + 1, 3) == NI_INVALID_STATE &&
            !std::memcmp(guarded, expected, sizeof(expected)) && calls == expectedCalls);
    }
    return 0;
}

/** 用独立 .NET 实现计算的二进制密码和盐样本校验32字节派生结果
@return 输出与固定向量逐字节一致，短缓冲和非法迭代保持原值时为0
*/
static int PasswordDerivation()
{
    const uint8_t password[] = {'p', 0, 'w'}, salt[] = {'s', 0, 'a', 'l', 't', '0', '1', '2'};
    const uint8_t expected[] = {0x91,0x84,0x0f,0x15,0x03,0xd2,0xc5,0x8c,0x85,0x08,0x03,0xb4,0xee,0xf2,0x70,0xf0,
        0xe4,0xae,0x79,0xd2,0x70,0xbf,0xdf,0x7b,0xaa,0x59,0xa6,0x59,0xc0,0x88,0x58,0x7b};
    uint8_t guarded[34];
    std::memset(guarded, 0x5a, sizeof(guarded));
    CHECK_TEST(NetInterface::pbkdf2Sha256(password, sizeof(password), salt, sizeof(salt), 2, guarded + 1, 31) == NI_BUFFER_TOO_SMALL &&
        NetInterface::pbkdf2Sha256(password, sizeof(password), salt, sizeof(salt), 0, guarded + 1, 32) == NI_INVALID_ARGUMENT &&
        NetInterface::pbkdf2Sha256(password, sizeof(password), salt, sizeof(salt), 2000001, guarded + 1, 32) == NI_INVALID_ARGUMENT);
    for (size_t i = 0; i < sizeof(guarded); ++i) { CHECK_TEST(guarded[i] == 0x5a); }
    CHECK_TEST(NetInterface::pbkdf2Sha256(password, sizeof(password), salt, sizeof(salt), 2, guarded + 1, 32) == NI_OK &&
        guarded[0] == 0x5a && guarded[33] == 0x5a && !std::memcmp(guarded + 1, expected, sizeof(expected)));
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
        // 每256次抽样恰好遍历0至255；按整周期和末尾项独立核对整批结果。
        const uint64_t tail = count % 256;
        const uint64_t expected = static_cast<uint64_t>(count) * sizeof(output) +
            static_cast<uint64_t>(count / 256) * 32640 + tail * (tail + 1) / 2 - tail;
        if (sum != expected)
        {
            valid = false;
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
    uint32_t aligned[(sizeof(kFrame) + 3) / 4 + 1] = {};
    uint8_t* inputBytes = reinterpret_cast<uint8_t*>(aligned);
    std::memcpy(inputBytes, kFrame, sizeof(kFrame));
    uint32_t& outside = aligned[(sizeof(kFrame) + 3) / 4];
    NetInterfaceHandle handle = nullptr;
    CHECK_TEST(NetInterface::create(3, &handle) == NI_OK && handle);
    std::unique_ptr<NetInterfaceOpaque, void (*)(NetInterfaceHandle)> handleOwner(handle, NetInterface::destroy);
    CHECK_TEST(NetInterface::feed(handle, inputBytes, sizeof(kFrame), &aligned[1]) == NI_INVALID_ARGUMENT &&
        !std::memcmp(inputBytes, kFrame, sizeof(kFrame)));
    CHECK_TEST(NetInterface::feed(handle, inputBytes, sizeof(kFrame), &outside) == NI_READY && outside == sizeof(kFrame));
    NetInterface direct(3);
    CHECK_TEST(direct.feed(inputBytes, sizeof(kFrame), aligned[0]) == NI_INVALID_ARGUMENT &&
        !std::memcmp(inputBytes, kFrame, sizeof(kFrame)) && direct.finish() == NI_OK);
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
    // 输出长度必须独立。被拒绝的调用不能覆盖输入字段、正文或目标中的任何字节。
    uint32_t slots[16];
    std::memset(slots, 0x5a, sizeof(slots));
    uint8_t* target = reinterpret_cast<uint8_t*>(slots);
    for (unsigned form = 0; form < 4; ++form)
    {
        NetInterfaceFrame frame = input;
        uint32_t fields[] = {NI_REQUEST, 7, 1, 0, 9, 3};
        for (unsigned location = 0; location < 3; ++location)
        {
            uint32_t body = 0xa1b2c3d4;
            frame.m_payload = reinterpret_cast<const uint8_t*>(&body);
            uint32_t& result = location == 0 ? body : (location == 1 ? slots[0] :
                (form == 3 ? fields[1] : frame.m_productId));
            const uint32_t before = result;
            const int32_t status = form == 0 ? NetInterface::encode(frame, target, sizeof(slots), result) :
                form == 1 ? NetInterface::encode(&frame, target, sizeof(slots), &result) :
                form == 2 ? NetInterface::encodeCompact(frame, target, sizeof(slots), result) :
                NetInterface::encodeFields(fields, frame.m_payload, target, sizeof(slots), &result);
            CHECK_TEST(status == NI_INVALID_ARGUMENT && result == before && body == 0xa1b2c3d4);
            for (size_t i = 0; i < sizeof(slots); ++i) { CHECK_TEST(target[i] == 0x5a); }
        }
    }
    CHECK_TEST(NetInterface::encode(input, target, 48, slots[12]) == NI_OK && slots[12] == sizeof(kFrame));
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
#if defined(NET_INTERFACE_SDK_STATIC_LINK) || defined(__EMSCRIPTEN__)
    // 静态 SDK 提供内部编码器。旧编号的存储可复用作输出，正文和长度各自独立。
    NetInterfaceFrame previous = {7, 41, 9, NI_DATA, 0, nullptr};
    const NetInterfaceFrame current = {7, 44, 9, NI_DATA, 1, jump + 3};
    uint8_t* encoded = reinterpret_cast<uint8_t*>(&previous.m_requestId);
    uint32_t required = 0;
    CHECK_TEST(NetInterfaceCompact::encode(current, encoded, sizeof(previous.m_requestId), required, &previous) == NI_OK &&
        required == sizeof(jump) && !std::memcmp(encoded, jump, sizeof(jump)));
#endif
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

/** 用手写事件头检查编码字节、两种事件语义、错误路径及借用边界
@return 定向事件和撤销组的所有独立预期均满足时为0
*/
static int EventEnvelope()
{
    const uint8_t wire[] = {'N','V','E','2',0,0,0,2,1,2,3,4,5,6,7,8,0,0,0,0,0,0,0,0,0,0x81,0,0xff};
    const uint64_t target = UINT64_C(0x0102030405060708);
    uint32_t source = 0;
    std::memcpy(&source, wire + 24, sizeof(source));
    const uint32_t saved = source;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&source);
    uint32_t slots[8];
    std::memset(slots, 0xa5, sizeof(slots));
    uint8_t* encoded = reinterpret_cast<uint8_t*>(slots);
    uint32_t count = 0;
    CHECK_TEST(NetInterfaceEvent::encode(target, 0, data, 4, nullptr, 0, count) == NI_BUFFER_TOO_SMALL && count == sizeof(wire));
    CHECK_TEST(NetInterfaceEvent::encode(target, 0, data, 4, encoded, sizeof(wire) - 1, count) == NI_BUFFER_TOO_SMALL &&
        count == sizeof(wire));
    CHECK_TEST(NetInterfaceEvent::encode(target, 0, data, 4, encoded, sizeof(wire), source) == NI_INVALID_ARGUMENT && source == saved);
    CHECK_TEST(NetInterfaceEvent::encode(target, 0, data, 4, encoded, sizeof(wire), slots[6]) == NI_INVALID_ARGUMENT);
    for (size_t i = 0; i < sizeof(slots); ++i)
    {
        CHECK_TEST(encoded[i] == 0xa5);
    }
    CHECK_TEST(NetInterfaceEvent::encode(target, 0, data, 4, encoded, sizeof(wire), slots[7]) == NI_OK &&
        slots[7] == sizeof(wire) && !std::memcmp(encoded, wire, sizeof(wire)));
    NetInterfaceEventView view = {};
    NetInterfaceFrame frame = {7, 1, 9, NI_EVENT, sizeof(wire), wire};
    CHECK_TEST(NetInterfaceEvent::parse(frame, view) == NI_OK && view.m_target == target && view.m_flags == 0 &&
        view.m_size == 4 && view.m_data == wire + 24);
    frame.m_payload = reinterpret_cast<const uint8_t*>(&view);
    frame.m_payloadSize = sizeof(view);
    CHECK_TEST(NetInterfaceEvent::parse(frame, view) == NI_INVALID_ARGUMENT && view.m_target == target && view.m_data == wire + 24);
    CHECK_TEST(NetInterfaceEvent::encode(0, 0, nullptr, 0, nullptr, 0, count) == NI_INVALID_ARGUMENT && count == 0);
    CHECK_TEST(NetInterfaceEvent::encode(target, 0, nullptr, 0, encoded, sizeof(slots), count) == NI_OK && count == 24);
    frame.m_payload = encoded;
    frame.m_payloadSize = count;
    CHECK_TEST(NetInterfaceEvent::parse(frame, view) == NI_OK && !view.m_data && !view.m_size && view.m_target == target);

    uint8_t group[16] = {};
    uint8_t revocation[40] = {};
    CHECK_TEST(NetInterfaceEvent::encode(0, 1, group, sizeof(group), revocation, sizeof(revocation), count) == NI_INVALID_ARGUMENT);
    group[15] = 0x31;
    CHECK_TEST(NetInterfaceEvent::encode(0, 1, group, sizeof(group), revocation, sizeof(revocation), count) == NI_OK &&
        count == sizeof(revocation) && revocation[19] == 1 && !std::memcmp(revocation + 24, group, sizeof(group)));
    frame.m_payload = revocation;
    frame.m_payloadSize = count;
    CHECK_TEST(NetInterfaceEvent::parse(frame, view) == NI_PROTOCOL_ERROR && !view.m_target && !view.m_data && !view.m_size);
    frame.m_messageType = NI_FILE_REVOKE_SCOPE;
    CHECK_TEST(NetInterfaceEvent::parse(frame, view) == NI_OK && !view.m_target && view.m_flags == 1 &&
        view.m_size == 16 && view.m_data == revocation + 24);
    return 0;
}

/** 通过真实NIS2握手和NOI2拆包检查输入寿命、独立结果及允许的原地操作
@return 握手结果与来源、正文都符合独立预期时为0
*/
static int ServiceEnvelopeBuffers()
{
    uint32_t secret[8] = {};
    secret[0] = 0x01020304;
    const uint8_t* key = reinterpret_cast<const uint8_t*>(secret);
    uint32_t storage[13] = {};
    uint8_t* packet = reinterpret_cast<uint8_t*>(storage);
    CHECK_TEST(NetInterfaceService::hello(key, 19, packet, 48) == NI_OK);
    NetInterfaceFrame hello = {7, 0, 3, NI_HELLO, 48, packet};
    CHECK_TEST(NetInterfaceService::authenticate(hello, key, storage[12]) == NI_OK && storage[12] == 19);
    const uint32_t encodedLimit = storage[1];
    CHECK_TEST(NetInterfaceService::authenticate(hello, key, storage[1]) == NI_INVALID_ARGUMENT && storage[1] == encodedLimit);
    CHECK_TEST(NetInterfaceService::authenticate(hello, key, secret[0]) == NI_INVALID_ARGUMENT && secret[0] == 0x01020304);
    CHECK_TEST(NetInterfaceService::authenticate(hello, key, hello.m_messageType) == NI_INVALID_ARGUMENT && hello.m_messageType == 3);
    CHECK_TEST(NetInterfaceService::authenticate(hello, key, storage[12]) == NI_OK && storage[12] == 19);

    const uint8_t local[] = {'N','O','I','2',0,0,0,1,1,2,3,4,5,6,7,8,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xab};
    NetInterfaceFrame frame = {7, 1, 9, NI_REQUEST, sizeof(local), local};
    NetInterfaceOrigin origin = {};
    CHECK_TEST(NetInterfaceService::unwrap(frame, origin, frame) == NI_OK &&
        origin.m_endpoint == UINT64_C(0x0102030405060708) && origin.m_platform == 1 && origin.m_device[0] == 1 &&
        frame.m_payload == local + 32 && frame.m_payloadSize == 1 && *frame.m_payload == 0xab);
    frame.m_payload = reinterpret_cast<const uint8_t*>(&origin);
    frame.m_payloadSize = sizeof(origin);
    NetInterfaceFrame business = frame;
    CHECK_TEST(NetInterfaceService::unwrap(frame, origin, business) == NI_INVALID_ARGUMENT &&
        origin.m_endpoint == UINT64_C(0x0102030405060708) && business.m_payload == frame.m_payload);
    frame.m_payload = reinterpret_cast<const uint8_t*>(&business);
    frame.m_payloadSize = sizeof(business);
    CHECK_TEST(NetInterfaceService::unwrap(frame, origin, business) == NI_INVALID_ARGUMENT &&
        origin.m_platform == 1 && business.m_requestId == 1);
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
    uint32_t inputOutput[6];
    std::memcpy(inputOutput, header, sizeof(header));
    CHECK_TEST(NetInterface::rpcParse(inputOutput, 6, wire, inputOutput) == NI_OK &&
        !std::memcmp(inputOutput, view, sizeof(view)));
    uint32_t aliased[5] = {};
    std::memcpy(aliased, wire, sizeof(wire));
    CHECK_TEST(NetInterface::rpcParse(header, 6, reinterpret_cast<const uint8_t*>(aliased), aliased) == NI_INVALID_ARGUMENT &&
        !std::memcmp(aliased, wire, sizeof(wire)));
    NetInterfaceRpcView borrowed = {};
    borrowed.m_parent = 73;
    const NetInterfaceFrame rpc = {7, 1, 9, NI_REQUEST, sizeof(borrowed), reinterpret_cast<const uint8_t*>(&borrowed)};
    CHECK_TEST(NetInterfaceRpc::parse(rpc, borrowed) == NI_INVALID_ARGUMENT && borrowed.m_parent == 73);
    CHECK_TEST(ServiceEnvelopeBuffers() == 0);
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

    // 新借用属于短帧；在解码器实际保留的同一数组内检查旧明文尾部。
    CHECK_TEST(decoder.consume() == NI_OK && decoder.decoderBufferCapacity() == 1024 &&
        decoder.feed(kFrame, sizeof(kFrame), used) == NI_READY && decoder.peek(frame) == NI_READY &&
        frame.m_payload && frame.m_payloadSize == 3 && frame.m_payload[1] == 255 &&
        std::all_of(frame.m_payload + 3, frame.m_payload + 1024, [](uint8_t value) { return value == 0; }));

    // 两种 RPC 都可以承载文件控制消息。完整头到达时应能决定压缩许可。
    const uint8_t kinds[] = {NI_REQUEST, NI_RESPONSE};
    for (size_t k = 0; k < sizeof(kinds); ++k)
    {
        wire[7] = kinds[k];
        for (uint32_t type = NI_FILE_OPEN_UPLOAD - 1; type <= NI_FILE_REVOKE_SCOPE + 1; ++type)
        {
            wire[26] = static_cast<uint8_t>(type >> 8);
            wire[27] = static_cast<uint8_t>(type);
            wire[31] = 1;
            decoder.reset();
            CHECK_TEST(decoder.feed(wire, 31, used) == NI_NEED_MORE && used == 31);
            if (type >= NI_FILE_OPEN_UPLOAD && type <= NI_FILE_REVOKE_SCOPE)
            {
                CHECK_TEST(decoder.feed(wire + 31, sizeof(wire) - 31, used) == NI_PROTOCOL_ERROR && used == 1);
                CHECK_TEST(decoder.peek(frame) == NI_PROTOCOL_ERROR && !frame.m_payload && !frame.m_payloadSize);
                CHECK_TEST(decoder.feed(wire, sizeof(wire), used) == NI_PROTOCOL_ERROR && used == 0);
                wire[31] = 0;
                decoder.reset();
                CHECK_TEST(decoder.feed(wire, sizeof(wire), used) == NI_READY && used == sizeof(wire));
                CHECK_TEST(decoder.peek(frame) == NI_READY && frame.m_payloadSize == sizeof(zipped) &&
                    frame.m_payload && std::memcmp(frame.m_payload, zipped, sizeof(zipped)) == 0);
            }
            else
            {
                CHECK_TEST(decoder.feed(wire + 31, sizeof(wire) - 31, used) == NI_READY && used == sizeof(wire) - 31);
                CHECK_TEST(decoder.peek(frame) == NI_READY && frame.m_payloadSize == 1024 && frame.m_payload);
                for (uint32_t i = 0; i < frame.m_payloadSize; ++i) { CHECK_TEST(frame.m_payload[i] == 'A'); }
            }
            CHECK_TEST(frame.m_messageType == type && frame.m_kind == kinds[k] && decoder.consume() == NI_OK);
        }
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
    for (uint8_t i = 0; i < 16; ++i)
    {
        local[i] = static_cast<uint8_t>(1 + i);
        remote[i] = static_cast<uint8_t>(33 + i);
        local[16 + i] = static_cast<uint8_t>(65 + i);
    }
    TestHandshakeStorage identityStorage = {};
    identityStorage.m_suffix[0] = 1;
    identityStorage.m_suffix[16] = 2;
    CHECK_TEST(NetInterface::handshakeCreate(clientOptions, 6, reinterpret_cast<const uint8_t*>(&identityStorage),
        32, 1000, 0, &identityStorage.m_handle) == NI_INVALID_ARGUMENT && !identityStorage.m_handle);
    NetInterfaceHandshakeHandle a = nullptr, b = nullptr, late = nullptr;
    uint32_t fields[6] = {};
    uint32_t shared[8] = {17, 19, 23, 29, 31, 37, 41, 43};
    const uint32_t saved[8] = {17, 19, 23, 29, 31, 37, 41, 43};
    const uint8_t* bytes = nullptr;
    const bool ok = NetInterface::handshakeCreate(clientOptions,6,local,32,1000,0,&a) == NI_NEED_MORE &&
        NetInterface::handshakeCreate(serverOptions,6,remote,32,1000,0,&b) == NI_NEED_MORE &&
        NetInterface::handshakeCreate(clientOptions,6,local,32,1000,0,&late) == NI_NEED_MORE &&
        NetInterface::handshakeProgress(late,1099,0) == NI_NEED_MORE && NetInterface::handshakeProgress(late,1100,0) == NI_TIMEOUT &&
        NetInterface::handshakePeekOutput(a,fields,&bytes) == NI_READY && fields[0] == NI_HELLO && fields[5] == 64 && bytes &&
        NetInterface::handshakeAccept(b,fields,bytes,1001,0) == NI_NEED_MORE && NetInterface::handshakeConsumeOutput(a) == NI_NEED_MORE &&
        NetInterface::handshakePeekOutput(b,fields,&bytes) == NI_READY && fields[0] == NI_HELLO_REPLY && bytes &&
        NetInterface::handshakeAccept(a,fields,bytes,1002,0) == NI_READY && NetInterface::handshakeConsumeOutput(b) == NI_READY &&
        NetInterface::handshakePeerInfo(a, shared, reinterpret_cast<uint8_t*>(shared)) == NI_INVALID_ARGUMENT &&
        !std::memcmp(shared, saved, sizeof(shared)) &&
        NetInterface::handshakePeerInfo(a,fields,peer) == NI_READY && fields[0] == 1 && fields[1] == 3 &&
        fields[2] == 16 && fields[3] == 4096 && !std::memcmp(peer, remote, 16) && !std::memcmp(peer + 16, local + 16, 16) &&
        NetInterface::handshakePeerInfo(b,fields,peer) == NI_READY && fields[0] == 1 && fields[1] == 3 &&
        fields[2] == 16 && fields[3] == 4096 && !std::memcmp(peer, local, sizeof(peer));
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

/** 在直连或服务启动的随机回调里核对启动前设置已冻结
@param [in,out] context 当前正在连接的 NetClientSession，在回调返回前有效
@param [out] bytes 随机样本的目标缓冲
@param [in] size 目标字节数
@return 配置重入已被拒绝且样本写完为0
*/
static int32_t CheckStartEntropy(void* context, uint8_t* bytes, uint32_t size)
{
    NetClientSession& session = *static_cast<NetClientSession*>(context);
    if (!session.executing() || session.configureExpiryBudget(3) != NI_INVALID_STATE)
    {
        return -1;
    }
    uint32_t seed = 91;
    return HelloEntropy(&seed, bytes, size);
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
    const uint32_t products[] = {0, 12345, UINT32_MAX};
    const char* const expected[] = {"service.0", "service.12345", "service.4294967295"};
    for (size_t i = 0; i < sizeof(products) / sizeof(products[0]); ++i)
    {
        uint32_t state = 1;
        NetInterfaceConnectionHandle connection = nullptr;
        int32_t detail = 0;
        const std::locale original = std::locale::global(std::locale(std::locale(), new NetInterfaceTestGroupedDigits));
        const int32_t result = NetInterface::connectionCreateServiceClient(products[i], HelloEntropy, &state, 5000, &connection, &detail);
        std::locale::global(original);
        const uint8_t* bytes = nullptr;
        uint32_t size = 0;
        const bool found = result == NET_INTERFACE_TLS_HANDSHAKE && NetInterface::connectionProgress(connection, 1, 0) >= 0 &&
            NetInterface::connectionPeekOutput(connection, &bytes, &size) >= 0 && HelloHasServerName(bytes, size, expected[i]);
        NetInterface::connectionDestroy(connection);
        CHECK_TEST(found);
    }
    return 0;
}

/** 把本轮已生成的一段密文送到另一端，保留任何未被接纳的后缀
@param [in,out] from 密文提供端
@param [in,out] to 对端解密器
@return 是否继续驱动连接
*/
template <typename Source, typename Destination>
static bool TransferCiphertext(Source& from, Destination& to)
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
    pools.m_decoderRetentionBytes = 2;
    CHECK_TEST(server.configurePools(pools) == NI_OK);
    pools.m_decoderRetentionBytes = -2;
    CHECK_TEST(server.configurePools(pools) == NI_INVALID_ARGUMENT);
    pools.m_decoderRetentionBytes = 2;
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
                CHECK_TEST(server.decoderBufferCapacity() >= sizeof(body));
                ++dataCount;
            }
            CHECK_TEST(server.consume() == NI_OK && server.decoderBufferCapacity() == 0);
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

/** 读取真实TLS明文，逐字节校验压缩配置如何决定应用线格式
@param [in] level 本连接独立的Zstandard等级
@param [in] threshold 开始尝试压缩的最小正文字节数
@param [in] reconfigure 是否在构造后、首次推进前应用配置
@return 成功为0
*/
static int CheckOutboundCompression(int32_t level, uint32_t threshold, bool reconfigure)
{
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM;
    static const uint8_t crt[] = TEST_SRV_CRT_RSA_SHA256_PEM;
    static const uint8_t key[] = TEST_SRV_KEY_RSA_PEM;
    uint32_t states[] = {23, 47};
    NetInterfaceTlsOptions options = {};
    options.authority = ca;
    options.authoritySize = sizeof(ca);
    options.hostname = "localhost";
    options.entropy = HelloEntropy;
    options.entropyContext = states;
    NetConnectionPoolConfig config;
    CHECK_TEST(config.m_compressionLevel == 1 && config.m_compressionMinPayload == 1024);
    config.m_compressionMinPayload = threshold;
    config.m_compressionLevel = level;
    std::unique_ptr<NetInterfaceConnection> client(new NetInterfaceConnection(options, 8192,
        reconfigure ? NetConnectionPoolConfig() : config));
    if (reconfigure) { CHECK_TEST(client->configurePools(config) == NI_OK); }
    config.m_compressionLevel = 10;
    CHECK_TEST(client->configurePools(config) == NI_INVALID_ARGUMENT);
    config.m_compressionLevel = -1;
    CHECK_TEST(client->configurePools(config) == NI_INVALID_ARGUMENT);
    options.server = 1;
    options.hostname = nullptr;
    options.certificate = crt;
    options.certificateSize = sizeof(crt);
    options.privateKey = key;
    options.privateKeySize = sizeof(key);
    options.entropyContext = states + 1;
    NetInterfaceTls server(options);
    const uint32_t type = 9;
    const uint32_t allowed[] = {type, NI_FILE_OPEN_UPLOAD};
    CHECK_TEST(client->configureCompression(allowed, 2, true) == NI_OK);
    uint32_t now = 0;
    while (now < 5000 && (client->status() != NET_INTERFACE_TLS_READY || server.status() != NET_INTERFACE_TLS_READY))
    {
        ++now;
        CHECK_TEST(client->progress(now) >= 0 && server.progress(now) >= 0 &&
            TransferCiphertext(*client, server) && TransferCiphertext(server, *client));
    }
    CHECK_TEST(client->status() == NET_INTERFACE_TLS_READY && server.status() == NET_INTERFACE_TLS_READY);
    std::vector<uint8_t> body(4096);
    for (size_t i = 0; i < body.size(); ++i)
    {
        body[i] = static_cast<uint8_t>((i % 193) ^ ((i / 97) % 3));
    }
    for (unsigned sample = 0; sample < 6; ++sample)
    {
        const uint32_t messageType = sample == 1 ? type + 1 : (sample == 3 ? static_cast<uint32_t>(NI_FILE_OPEN_UPLOAD) : type);
        const uint16_t kind = sample == 4 ? NI_EVENT : NI_REQUEST;
        if (sample == 5)
        {
            // 可复现的非重复正文，用于检验压缩空间不足以抵消信封时的回退。
            uint32_t state = 0x49a23fab;
            for (size_t i = 0; i < body.size(); ++i)
            {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                body[i] = static_cast<uint8_t>(state);
            }
        }
        const uint32_t size = sample == 2 ? 31 : static_cast<uint32_t>(body.size());
        const NetInterfaceFrame frame = {7, 1 + sample, messageType, kind, size, body.data()};
        std::vector<uint8_t> reference(Compress::rawCompressBound(size));
        uint32_t zipped = static_cast<uint32_t>(reference.size());
        CHECK_TEST(Compress::rawCompress(reference.data(), zipped, body.data(), size, level) == COMPRESS_OK);
        const bool compressed = sample == 0 && size >= threshold;
        if (sample == 0) { CHECK_TEST(zipped + 8 <= size - size / 12); }
        if (sample == 2 || sample == 5) { CHECK_TEST(zipped + 8 > size - size / 12); }
        const uint32_t wireSize = NET_INTERFACE_HEADER_SIZE + (compressed ? zipped + 8 : size);
        std::vector<uint8_t> wire(wireSize + 1);
        uint32_t total = 0;
        CHECK_TEST(client->send(frame) == NI_OK);
        for (unsigned step = 0; step < 512 && total < wireSize; ++step)
        {
            ++now;
            CHECK_TEST(client->progress(now) >= 0 && TransferCiphertext(*client, server) && server.progress(now) >= 0);
            uint32_t received = 0;
            CHECK_TEST(server.read(wire.data() + total, static_cast<uint32_t>(wire.size()) - total, received) >= 0);
            total += received;
        }
        CHECK_TEST(total == wireSize && !std::memcmp(wire.data(), "NIF2", 4) &&
            wire[31] == (compressed ? 1 : 0));
        const uint8_t* payload = wire.data() + NET_INTERFACE_HEADER_SIZE;
        if (compressed)
        {
            CHECK_TEST(!std::memcmp(payload, "NZC2", 4) && !std::memcmp(payload + 8, reference.data(), zipped));
        }
        else { CHECK_TEST(!std::memcmp(payload, body.data(), size)); }
        NetInterface decoder(8192);
        decoder.allowCompression(true);
        uint32_t used = 0;
        NetInterfaceFrame decoded = {};
        CHECK_TEST(decoder.feed(wire.data(), total, used) == NI_READY && used == total &&
            decoder.peek(decoded) == NI_READY && decoded.m_messageType == messageType &&
            decoded.m_payloadSize == size && !std::memcmp(decoded.m_payload, body.data(), size));
    }
    // 同一消息类型可用于RPC及DATA。压缩白名单只约束RPC，DATA按该方向的协商格式发送。
    const uint8_t marker = 0x4d;
    const uint64_t identifiers[] = {40, 41, 44, 171, 299, UINT64_C(0x10000012a), UINT64_C(0x20000012a), 42, 43};
    // 手写跳号的网络字节，覆盖短增量、两字节变长、最大32位增量及必须退回完整头的情况。
    const uint8_t wireFrames[][14] = {
        {0xe3, NI_DATA, 7, 9, 1, 0,0,0,0,0,0,0,40, marker},
        {0x81, marker},
        {0xe4, 1, 3, marker},
        {0xe4, 1, 127, marker},
        {0xe4, 1, 0x80, 1, marker},
        {0xe4, 1, 0xff,0xff,0xff,0xff,15, marker},
        {0xe3, NI_DATA, 7, 9, 1, 0,0,0,2,0,0,1,42, marker},
        {0xe3, NI_DATA, 7, 9, 1, 0,0,0,0,0,0,0,42, marker},
        {0xe3, NI_DATA, 7, 9, 1, 0,0,0,0,0,0,0,43, marker}
    };
    const uint32_t lengths[] = {14, 2, 4, 4, 5, 8, 14, 14, 14};
    for (size_t i = 0; i < sizeof(identifiers) / sizeof(identifiers[0]); ++i)
    {
        if (i == 8) { client->configureCompact(false); }
        client->configureCompact(true);
        NetInterfaceFrame data = {7, identifiers[i], type, NI_DATA, 1, &marker};
        const uint32_t expected = lengths[i];
        uint8_t actual[NET_INTERFACE_HEADER_SIZE + 1];
        uint32_t total = 0;
        CHECK_TEST(client->send(data) == NI_OK);
        for (unsigned step = 0; step < 128 && total < expected; ++step)
        {
            ++now;
            CHECK_TEST(client->progress(now) >= 0 && TransferCiphertext(*client, server) && server.progress(now) >= 0);
            uint32_t received = 0;
            CHECK_TEST(server.read(actual + total, sizeof(actual) - total, received) >= 0);
            total += received;
        }
        CHECK_TEST(total == expected && !std::memcmp(actual, wireFrames[i], expected));
    }
    return 0;
}

/** 分别验证构造配置和启动前重配的独立发送策略
@return 成功为0
*/
static int OutboundCompression()
{
    CHECK_TEST(CheckOutboundCompression(0, 4097, false) == 0);
    CHECK_TEST(CheckOutboundCompression(1, 4096, false) == 0);
    CHECK_TEST(CheckOutboundCompression(9, 0, true) == 0);
    return 0;
}

/** 逐级核对独立的初始容量、增长通知和硬上限；第一级的 -1 允许连续增长
@return 实际接纳量、租约内容与各级独立统计均符合配置时为0
*/
static int IndependentBufferBudgets()
{
    const uint32_t requests[] = {1, 65, 1025, 16449, 131073};
    const size_t initial[] = {1, 0, 1, 0, 0};
    const int64_t maximum[] = {-1, 4, 3, 2, 1};
    const size_t growth[] = {2, 4, 2, 2, 1};
    const size_t allocations[] = {6, 4, 3, 2, 1};
    size_t added[5] = {};
    uint64_t notifications[5] = {};
    NetBufferPoolConfig config;
    for (size_t i = 0; i < 5; ++i)
    {
        LockFreeHashMapPoolConfig& part = config.m_classes[i];
        part.setInitialCapacity(initial[i]);
        part.setMaximumCapacity(maximum[i]);
        part.setGrowFreeThreshold(0);
        part.setProtectionRecordCount(8 + i);
        const size_t step = growth[i];
        part.setGrowPolicy([step](const LockFreeHashMapGrowContext&) { return step; });
        part.setGrowthObserver([i, &added, &notifications](size_t slots, uint64_t) {
            added[i] += slots;
            ++notifications[i];
        });
    }
    NetBufferPool pool(config);
    CHECK_TEST(pool.status() == NI_OK);
    for (uint32_t i = 0; i < 5; ++i)
    {
        const LockFreeHashMapPoolStats before = pool.statistics(i);
        CHECK_TEST(before.capacity == initial[i] && before.protectionRecords == 8 + i && added[i] == 0);
        NetBufferLease leases[6], spare;
        for (size_t j = 0; j < allocations[i]; ++j)
        {
            CHECK_TEST(pool.allocate(requests[i], leases[j]) == NI_OK && leases[j].data());
            leases[j].data()[0] = static_cast<uint8_t>(j + 1);
        }
        if (maximum[i] >= 0)
        {
            CHECK_TEST(pool.allocate(requests[i], spare) == NI_LIMIT_EXCEEDED && !spare.data());
        }
        const LockFreeHashMapPoolStats full = pool.statistics(i);
        const size_t expectedGrowths = i == 0 ? 3 : 1;
        CHECK_TEST(notifications[i] == expectedGrowths && added[i] == expectedGrowths * growth[i] &&
            full.capacity == initial[i] + added[i] && full.occupied == allocations[i]);
        for (size_t j = 0; j < allocations[i]; ++j)
        {
            CHECK_TEST(leases[j].data()[0] == j + 1);
            leases[j].reset();
        }
        CHECK_TEST(pool.statistics(i).occupied == 0);
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
    return IndependentBufferBudgets();
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

#if defined(_MSC_VER) && defined(_DEBUG)
/** 在已解码字段和完整解析入口分别拒绝节点名称的目标字符串分配
@param [in] message 含合法节点记录的消息
@param [in] wire 同一消息的完整编码
@return 公开 text 清空输出、整表报告内存不足并可重试时为0
*/
static int DirectoryTextAllocation(const ProtoMessage& message, const std::string& wire);
#endif

/** 从手写数字字段解析目录，并从独立创建的节点逐字段检查导出的线序字节
@return 公开句柄在两端均得到指定数值、类型及字节时为0
*/
static int DirectoryWireNumbers()
{
    NetInterfaceDirectoryNodeData node = DirectoryNode(0);
    node.m_port = 0x1234;
    node.m_capacity = 0x89abcdefU;
    node.m_boot = UINT64_C(0xf1e2d3c4b5a69788);
    const uint8_t count[] = {1, 0, 0, 0};
    const uint8_t port[] = {0x34, 0x12, 0, 0};
    const uint8_t capacity[] = {0xef, 0xcd, 0xab, 0x89, 0, 0, 0, 0};
    const uint8_t boot[] = {0x88, 0x97, 0xa6, 0xb5, 0xc4, 0xd3, 0xe2, 0xf1};
    NetInterfaceDirectoryHandle out = nullptr;
    CHECK_TEST(NetInterface::directoryCreateNodes(&node, 1, &out) == NI_OK && out);
    std::unique_ptr<NetInterfaceDirectoryOpaque, void (*)(NetInterfaceDirectoryHandle)> created(out, NetInterface::directoryDestroy);
    uint32_t size = 0;
    CHECK_TEST(NetInterface::directoryCache(out, 1, 0, nullptr, 0, &size) == NI_BUFFER_TOO_SMALL && size > 48);
    std::vector<uint8_t> cache(size);
    CHECK_TEST(NetInterface::directoryCache(out, 1, 0, cache.data(), size, &size) == NI_OK);
    ProtoMessage exported;
    CHECK_TEST(exported.from(cache.data() + 16, size - 48) &&
        HasWireField(exported, NI_GATEWAY_NODE_COUNT, Variant::INT32, count) &&
        HasWireField(exported, NI_GATEWAY_PORT, Variant::INT32, port, 1) &&
        HasWireField(exported, NI_GATEWAY_CAPACITY, Variant::UINT64, capacity, 1) &&
        HasWireField(exported, NI_GATEWAY_BOOT, Variant::UINT64, boot, 1));

    ProtoMessage incoming;
    CHECK_TEST(incoming.setBytes(NI_GATEWAY_NODE_COUNT, count, sizeof(count), Variant::INT32) &&
        incoming.setBytes(NI_GATEWAY_NODE_ID, node.m_name, std::strlen(node.m_name), Variant::STRING, 1) &&
        incoming.setBytes(NI_GATEWAY_ADDRESS, node.m_address, std::strlen(node.m_address), Variant::STRING, 1) &&
        incoming.setBytes(NI_GATEWAY_PORT, port, sizeof(port), Variant::INT32, 1) &&
        incoming.setBytes(NI_GATEWAY_CAPACITY, capacity, sizeof(capacity), Variant::UINT64, 1) &&
        incoming.setBytes(NI_GATEWAY_BOOT, boot, sizeof(boot), Variant::UINT64, 1));
    std::string body;
    NetInterfaceDirectoryHandle in = nullptr;
    CHECK_TEST(incoming.toString(body) && NetInterface::directoryCreate(reinterpret_cast<const uint8_t*>(body.data()),
        static_cast<uint32_t>(body.size()), &in) == NI_OK && in);
    std::unique_ptr<NetInterfaceDirectoryOpaque, void (*)(NetInterfaceDirectoryHandle)> parsed(in, NetInterface::directoryDestroy);
    NetInterfaceDirectoryNodeData received = DirectoryNode(3);
    uint32_t nodes = 0;
    CHECK_TEST(NetInterface::directoryCopyNodes(in, &received, 1, &nodes) == NI_OK && nodes == 1 &&
        received.m_port == 0x1234 && received.m_capacity == 0x89abcdefU && received.m_boot == UINT64_C(0xf1e2d3c4b5a69788));
    std::string value("old");
    const uint32_t maximum = NET_INTERFACE_DIRECTORY_NAME_BYTES;
    CHECK_TEST(NetInterfaceDirectory::text(incoming, NI_GATEWAY_NODE_ID, value, maximum, 1) && value == node.m_name);
    CHECK_TEST(!NetInterfaceDirectory::text(incoming, NI_GATEWAY_NODE_ID, value, maximum - 1, 1) && value.empty());
    value = "old";
    CHECK_TEST(!NetInterfaceDirectory::text(incoming, NI_GATEWAY_NODE_ID, value, maximum) && value.empty());
    value = "old";
    CHECK_TEST(!NetInterfaceDirectory::text(incoming, NI_GATEWAY_PORT, value, maximum, 1) && value.empty());
    ProtoMessage fields;
    const char allowed[] = "aZ0.-:";
    const char embedded[] = {'n', 0, 'x'};
    CHECK_TEST(fields.setBytes(1, allowed, sizeof(allowed) - 1, Variant::STRING) &&
        fields.setBytes(2, embedded, sizeof(embedded), Variant::STRING) &&
        fields.setBytes(3, nullptr, 0, Variant::STRING) && fields.setBytes(4, "n/x", 3, Variant::STRING));
    CHECK_TEST(NetInterfaceDirectory::text(fields, 1, value, sizeof(allowed) - 1) && value == allowed);
    for (int32_t field = 2; field <= 4; ++field)
    {
        value = "old";
        CHECK_TEST(!NetInterfaceDirectory::text(fields, field, value, maximum) && value.empty());
    }
#if defined(_MSC_VER) && defined(_DEBUG)
    CHECK_TEST(DirectoryTextAllocation(incoming, body) == 0);
#endif
    const uint8_t negative[] = {1, 0, 0, 0x80};
    CHECK_TEST(incoming.setBytes(NI_GATEWAY_CAPACITY, negative, sizeof(negative), Variant::INT32, 1) && incoming.toString(body) &&
        NetInterface::directoryCreate(reinterpret_cast<const uint8_t*>(body.data()), static_cast<uint32_t>(body.size()), &in) ==
        NI_PROTOCOL_ERROR && !in);
    return 0;
}

/** 验证超过64项的合法目录、完整输出、按数值去重及整体线预算
@return 全部独立预期满足时为0
*/
static int DirectoryNodes()
{
    CHECK_TEST(DirectoryWireNumbers() == 0);
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
    NetInterfaceDirectoryHandle invalidTime = raw;
    CHECK_TEST(NetInterface::directoryCreateCache(cache.data(), bytes, 0, 0, &invalidTime) == NI_INVALID_ARGUMENT && !invalidTime);
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

/** 从句柄登记具有相同低字的端点，按来源完成父请求，再分别取走子请求和其他来源的请求
@param [in] table 已清空的等待表
@return 输出记录和一次性终态符合约定时为0
*/
static int RequestHandleOwnership(NetInterfaceRequestsHandle table)
{
    const uint32_t endpointLow = 0x2134;
    const uint32_t client = 1, service = 2, other = 3, childService = 4;
    const uint32_t product = 7, type = 9;
    CHECK_TEST(NetInterface::requestsAddEndpoint(table, endpointLow, client, 0, 0, 1, 0) == NI_OK &&
        NetInterface::requestsAddEndpoint(table, endpointLow, service, 1, product, 1, 0) == NI_OK &&
        NetInterface::requestsAddEndpoint(table, endpointLow, other, 0, 0, 1, 0) == NI_OK &&
        NetInterface::requestsAddEndpoint(table, endpointLow, childService, 1, product, 1, 1) == NI_OK);
    uint32_t out[16] = {};
    const uint32_t zero[16] = {};
    uint32_t input[] = {endpointLow, client, UINT32_MAX, 0x75, endpointLow, service, product, type, 0, 0, 0xfffffff0U, 0x31, 0x30};
    CHECK_TEST(NetInterface::requestsBegin(table, input, 13, 0, out) == NI_OK &&
        NetInterface::requestsCancel(table, endpointLow, client, UINT32_MAX, 0x75, out) == NI_READY);
    CHECK_TEST(NetInterface::requestsBegin(table, input, 13, 1, out) == NI_INVALID_ARGUMENT &&
        !std::memcmp(out, zero, sizeof(out)) && NetInterface::requestsCount(table) == 0);
    input[2] = input[3] = 0;
    CHECK_TEST(NetInterface::requestsBegin(table, input, 13, 1, out) == NI_OK && out[0] != 0);
    const uint32_t parent[] = {out[0], 0, endpointLow, client, 0, 0x76, endpointLow, service,
        endpointLow, client, 0, 0, 0x20, 0x32, product, type};
    CHECK_TEST(!std::memcmp(out, parent, sizeof(out)));

    input[1] = other;
    input[3] = 0x76;
    CHECK_TEST(NetInterface::requestsBegin(table, input, 13, 0, out) == NI_OK && out[0] != parent[0] && out[0] != 0);
    const uint32_t independent[] = {out[0], 0, endpointLow, other, 0, 0x76, endpointLow, service,
        endpointLow, other, 0, 0, 0x20, 0x32, product, type};
    CHECK_TEST(!std::memcmp(out, independent, sizeof(out)));
    const uint32_t subrequest[] = {endpointLow, service, 0, 0, endpointLow, childService, product, type + 1,
        parent[0], parent[1], 0xfffffff1U, 0x31, 0x80};
    CHECK_TEST(NetInterface::requestsBegin(table, subrequest, 13, 1, out) == NI_OK &&
        out[0] != 0 && out[0] != parent[0] && out[0] != independent[0]);
    const uint32_t child[] = {out[0], 0, endpointLow, service, 1, 0, endpointLow, childService,
        endpointLow, client, parent[0], parent[1], 0x20, 0x32, product, type + 1};
    CHECK_TEST(!std::memcmp(out, child, sizeof(out)) && NetInterface::requestsCount(table) == 3);

    uint32_t reply[] = {endpointLow, client, 0, 0x75, endpointLow, service, product, type, 0x1f, 0x32};
    CHECK_TEST(NetInterface::requestsComplete(table, reply, 10, out) == NI_NOT_FOUND &&
        !std::memcmp(out, zero, sizeof(out)) && NetInterface::requestsCount(table) == 3);
    reply[3] = 0x76;
    reply[5] = childService;
    CHECK_TEST(NetInterface::requestsComplete(table, reply, 10, out) == NI_PROTOCOL_ERROR &&
        !std::memcmp(out, zero, sizeof(out)) && NetInterface::requestsCount(table) == 3);
    reply[5] = service;
    CHECK_TEST(NetInterface::requestsComplete(table, reply, 10, out) == NI_READY && !std::memcmp(out, parent, sizeof(out)) &&
        NetInterface::requestsComplete(table, reply, 10, out) == NI_NOT_FOUND &&
        !std::memcmp(out, zero, sizeof(out)) && NetInterface::requestsCount(table) == 2);
    CHECK_TEST(NetInterface::requestsTake(table, 2, parent[0], 1, out) == NI_NOT_FOUND &&
        !std::memcmp(out, zero, sizeof(out)) && NetInterface::requestsCount(table) == 2);
    CHECK_TEST(NetInterface::requestsTake(table, 2, parent[0], parent[1], out) == NI_READY && !std::memcmp(out, child, sizeof(out)) &&
        NetInterface::requestsTake(table, 2, parent[0], parent[1], out) == NI_NOT_FOUND &&
        !std::memcmp(out, zero, sizeof(out)) && NetInterface::requestsCount(table) == 1);
    CHECK_TEST(NetInterface::requestsTake(table, 1, endpointLow, client, out) == NI_NOT_FOUND &&
        !std::memcmp(out, zero, sizeof(out)) && NetInterface::requestsCount(table) == 1);
    CHECK_TEST(NetInterface::requestsTake(table, 1, endpointLow, other, out) == NI_READY && !std::memcmp(out, independent, sizeof(out)) &&
        NetInterface::requestsTake(table, 1, endpointLow, other, out) == NI_NOT_FOUND && !std::memcmp(out, zero, sizeof(out)));
    reply[1] = other;
    CHECK_TEST(NetInterface::requestsComplete(table, reply, 10, out) == NI_NOT_FOUND && !std::memcmp(out, zero, sizeof(out)) &&
        NetInterface::requestsCount(table) == 0);
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
    return RequestHandleOwnership(raw);
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
    CHECK_TEST(requests.setEndpointLimit(1, 2) == NI_OK && requests.setEndpointLimit(1, 0) == NI_INVALID_ARGUMENT &&
        requests.setEndpointLimit(3, 1) == NI_NOT_FOUND);
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
#if defined(NET_INTERFACE_SDK_STATIC_LINK) || defined(__EMSCRIPTEN__)
    std::vector<NetGatewayAddress> local;
    CHECK_TEST(NetClientDiscovery::parseSeeds("127.0.0.1:3000,[::1]:123,[0:0:0:0:0:0:0:1]:123", local) == NI_OK &&
        local.size() == 2 && local[0].m_address == "127.0.0.1" && local[0].m_port == 3000 &&
        local[1].m_address == "::1" && local[1].m_port == 123);
    CHECK_TEST(NetClientDiscovery::parseSeeds("127.0.0.1:3000,[::1]:65536", local) == NI_INVALID_ARGUMENT && local.empty());
#endif
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
    NetFileProtocol protocol;
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
    NetInterfaceFileFlow stalled(info, 7, false, 100, 1000), defaultReceiver(info, 7, false, 100, 1000);
    NetInterfaceFileFlowHandle flow = nullptr;
    CHECK_TEST(protocol.fileFlowCreate(&info, 7, 0, 100, 0, 1000, &flow) == NI_OK && flow);
    const auto dispose = [&](NetInterfaceFileFlowHandle handle) { protocol.fileFlowDestroy(handle); };
    std::unique_ptr<NetInterfaceFileFlowOpaque, decltype(dispose)> flowOwner(flow, dispose);
    CHECK_TEST(sender.setWriteBatchChunks(1) == NI_INVALID_STATE && receiver.setWriteBatchChunks(0) == NI_INVALID_ARGUMENT &&
        protocol.fileFlowSetWriteBatchChunks(flow, 0) == NI_INVALID_ARGUMENT &&
        protocol.fileFlowSetWriteBatchChunks(flow, 2) == NI_OK &&
        protocol.fileFlowSetWriteBatchChunks(flow, 3) == NI_INVALID_ARGUMENT);
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
    CHECK_TEST(defaultReceiver.accept(data) == NI_OK && defaultReceiver.writeRequest(offset, borrowed, size) == NI_READY &&
        offset == 0 && size == 1024 && borrowed && !std::memcmp(borrowed, content, size) &&
        defaultReceiver.written(offset, size, NI_OK) == NI_OK && receiver.setWriteBatchChunks(1) == NI_INVALID_STATE);
    uint32_t fields[] = {data.m_kind, 7, 61, 0, data.m_messageType, data.m_payloadSize};
    uint32_t write[3] = {};
    CHECK_TEST(protocol.fileFlowAccept(flow, fields, data.m_payload) == NI_OK &&
        protocol.fileFlowWrite(flow, write, &borrowed) == NI_NEED_MORE && !borrowed && write[2] == 0 &&
        protocol.fileFlowSetWriteBatchChunks(flow, 1) == NI_INVALID_STATE);
    CHECK_TEST(receiver.writeRequest(offset, borrowed, size) == NI_NEED_MORE && !borrowed && size == 0 &&
        receiver.peekOutput(ack) == NI_NEED_MORE);
    CHECK_TEST(stalled.progress(1700) == NI_OK && stalled.accept(data) == NI_OK && stalled.progress(1799) == NI_OK &&
        stalled.progress(1800) == NI_TIMEOUT && stalled.progress(1801) == NI_TIMEOUT);
    CHECK_TEST(sender.consumeOutput() == NI_OK && sender.progress(1500) == NI_OK && receiver.progress(1500) == NI_OK);
    CHECK_TEST(sender.readRequest(offset, size) == NI_READY && offset == 1024 && size == 513 &&
        sender.produced(offset, content + 1024, size) == NI_OK && sender.peekOutput(data) == NI_READY &&
        receiver.accept(data) == NI_OK);
    fields[5] = data.m_payloadSize;
    CHECK_TEST(protocol.fileFlowAccept(flow, fields, data.m_payload) == NI_OK &&
        protocol.fileFlowWrite(flow, write, &borrowed) == NI_READY && write[0] == 0 && write[1] == 0 &&
        write[2] == sizeof(content) && borrowed && !std::memcmp(borrowed, content, sizeof(content)));
    CHECK_TEST(sender.consumeOutput() == NI_OK);
    CHECK_TEST(receiver.writeRequest(offset, borrowed, size) == NI_READY && offset == 0 && size == sizeof(content) &&
        borrowed && !std::memcmp(borrowed, content, sizeof(content)) && receiver.peekOutput(ack) == NI_NEED_MORE);
    NetInterfaceFileInfo snapshot = {};
    int32_t status = NI_OK;
    receiver.snapshot(snapshot, status);
    CHECK_TEST(status == NI_OK && snapshot.offset == 0 && receiver.written(0, size, NI_OK) == NI_OK &&
        receiver.peekOutput(ack) == NI_READY && ack.m_kind == NI_DATA_ACK && sender.accept(ack) == NI_OK &&
        receiver.consumeOutput() == NI_OK);
    uint32_t ackFields[6] = {};
    CHECK_TEST(protocol.fileFlowPeek(flow, ackFields, &borrowed) == NI_NEED_MORE &&
        protocol.fileFlowWritten(flow, write[0], write[1], write[2], NI_OK) == NI_OK &&
        protocol.fileFlowPeek(flow, ackFields, &borrowed) == NI_READY && ackFields[0] == NI_DATA_ACK &&
        protocol.fileFlowConsume(flow) == NI_OK);
    CHECK_TEST(sender.peekOutput(data) == NI_READY && receiver.accept(data) == NI_OK);
    fields[0] = data.m_kind;
    fields[4] = data.m_messageType;
    fields[5] = data.m_payloadSize;
    CHECK_TEST(protocol.fileFlowAccept(flow, fields, data.m_payload) == NI_OK &&
        protocol.fileFlowTakeCommit(flow) == NI_READY && protocol.fileFlowTakeCommit(flow) == NI_NEED_MORE &&
        protocol.fileFlowPeek(flow, ackFields, &borrowed) == NI_NEED_MORE &&
        protocol.fileFlowCommitted(flow, NI_OK) == NI_READY &&
        protocol.fileFlowPeek(flow, ackFields, &borrowed) == NI_READY && ackFields[0] == NI_DATA_ACK &&
        protocol.fileFlowConsume(flow) == NI_OK && protocol.fileFlowSnapshot(flow, &snapshot, &status) == NI_OK &&
        status == NI_READY && snapshot.state == NI_FILE_COMPLETED && snapshot.offset == sizeof(content) && snapshot.stream == 0);
    CHECK_TEST(sender.consumeOutput() == NI_OK &&
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
// 目录场景通过成功解析的实际字符串缓冲定位首个目标申请；0 表示普通的按大小注入。
static long directoryRequest = 0;
// 标准库在首选容量申请失败后可改按正文长度加终止符重试。
static size_t directoryRetrySize = 0;

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
    if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK)
    {
        const bool refuse = directoryRequest != 0 ?
            ((!allocationRefused && request == directoryRequest && size == rejectedSize) ||
                (allocationRefused && (size == rejectedSize || size == directoryRetrySize))) :
            (refuseAtLeast ? size >= rejectedSize : !allocationRefused && size == rejectedSize);
        if (refuse)
        {
            allocationRefused = true;
            return 0;
        }
    }
    return previousAllocationHook ? previousAllocationHook(operation, data, size, block, request, file, line) : 1;
}

static int DirectoryTextAllocation(const ProtoMessage& message, const std::string& wire)
{
    std::string value("old");
    const uint32_t length = NET_INTERFACE_DIRECTORY_NAME_BYTES;
    rejectedSize = length + 1;
    allocationRefused = false;
    refuseAtLeast = true;
    previousAllocationHook = _CrtSetAllocHook(FilterTestAllocation);
    const bool copied = NetInterfaceDirectory::text(message, NI_GATEWAY_NODE_ID, value, length, 1);
    _CrtSetAllocHook(previousAllocationHook);
    refuseAtLeast = false;
    CHECK_TEST(allocationRefused && !copied && value.empty());
    CHECK_TEST(NetInterfaceDirectory::text(message, NI_GATEWAY_NODE_ID, value, length, 1) && value.size() == length);

    // 每次解析前取一个 CRT 序号作起点；目标序号来自成功结果的真实名称缓冲，避开 protobuf 和容器分配。
    const auto marker = []() -> long {
        std::unique_ptr<char[]> stamp(new char[1]);
        long number = -1;
        return _CrtIsMemoryBlock(stamp.get(), 1, &number, nullptr, nullptr) ? number : -1;
    };
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(wire.data());
    const uint32_t size = static_cast<uint32_t>(wire.size());
    std::vector<NetInterfaceDirectoryPoint> points;
    const long start = marker();
    CHECK_TEST(start > 0 && NetInterfaceDirectory::parse(bytes, size, points) == NI_OK &&
        points.size() == 1 && points[0].m_name == value);
    long target = -1;
    rejectedSize = points[0].m_name.capacity() + 1;
    CHECK_TEST(_CrtIsMemoryBlock(points[0].m_name.data(), static_cast<unsigned int>(rejectedSize),
        &target, nullptr, nullptr) && target > start);
    const long again = marker();
    CHECK_TEST(again > target);
    directoryRequest = again + (target - start);
    directoryRetrySize = length + 1;
    allocationRefused = false;
    previousAllocationHook = _CrtSetAllocHook(FilterTestAllocation);
    const int32_t failed = NetInterfaceDirectory::parse(bytes, size, points);
    _CrtSetAllocHook(previousAllocationHook);
    directoryRequest = 0;
    CHECK_TEST(allocationRefused && failed == NI_NO_MEMORY && points.empty());
    CHECK_TEST(NetInterfaceDirectory::parse(bytes, size, points) == NI_OK && points.size() == 1 &&
        points[0].m_name == value && points[0].m_address == "::1" && points[0].m_port == 0x1234 &&
        points[0].m_capacity == 0x89abcdefU && points[0].m_boot == UINT64_C(0xf1e2d3c4b5a69788));
    return 0;
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
    uint32_t slots[3] = {17, 29, 41};
    uint8_t* bytes = reinterpret_cast<uint8_t*>(slots);
    int32_t metadata = -7;
    CHECK_TEST(NetInterface::messageEncode(raw, bytes, 5, &slots[1]) == NI_INVALID_ARGUMENT &&
        slots[0] == 17 && slots[1] == 29 && slots[2] == 41);
    CHECK_TEST(NetInterface::messageGetValue(raw, 1, 17, &metadata, bytes, 5, &slots[1]) == NI_INVALID_ARGUMENT &&
        metadata == -7 && slots[0] == 17 && slots[1] == 29 && slots[2] == 41);
    CHECK_TEST(NetInterface::messageGetCode(raw, &metadata, reinterpret_cast<uint8_t*>(&metadata),
        sizeof(metadata), &slots[2]) == NI_INVALID_ARGUMENT && metadata == -7 && slots[2] == 41);
    uint32_t* sameOutput = reinterpret_cast<uint32_t*>(&metadata);
    CHECK_TEST(NetInterface::messageGetValue(raw, 1, 17, &metadata, nullptr, 0, sameOutput) == NI_INVALID_ARGUMENT && metadata == -7);
    CHECK_TEST(NetInterface::messageGetCode(raw, &metadata, nullptr, 0, sameOutput) == NI_INVALID_ARGUMENT && metadata == -7);
    CHECK_TEST(NetInterface::messageGetCode(raw, &metadata, bytes, sizeof(slots[0]), &slots[1]) == NI_OK &&
        metadata == 73 && slots[1] == 3 && slots[2] == 41 && !std::memcmp(input, bytes, 3));
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
    const uint8_t wireValues[][4] = {{0,0,1,0}, {0,0,16,0}, {255,255,0,0}, {1,0,16,0}, {1,0,1,0}, {255,255,255,255}};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
    {
        CHECK_TEST(message.setBytes(NI_GATEWAY_WINDOW, wireValues[i], sizeof(wireValues[i]), Variant::INT32, 7));
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
    const uint8_t quarterMiB[] = {0, 0, 4, 0};
    CHECK_TEST(message.setBytes(NI_GATEWAY_WINDOW, quarterMiB, sizeof(quarterMiB), Variant::INT32));
    CHECK_TEST(message.toString(wire));
    const uint8_t* input = reinterpret_cast<const uint8_t*>(wire.data());
    const uint32_t length = static_cast<uint32_t>(wire.size());
    CHECK_TEST(NetInterface::creditConfigureMessage(raw, input, length) == NI_OK && NetInterface::creditSent(raw, 1) == NI_OK);
    CHECK_TEST(NetInterface::creditConfigureMessage(raw, input, length) == NI_INVALID_STATE &&
        NetInterface::creditGet(raw, credit) == NI_OK && credit[0] == 262143 && credit[1] == 0);
    return 0;
}

/** 验证三个句柄工厂在交付所有权前实际采用指定池
@return 成功为0，任一配置被忽略时失败
*/
static int HandlePools()
{
    NetRequestTablePools tables;
    tables.m_endpoints.initialCapacity = 1;
    tables.m_endpoints.maximumCapacity = 1;
    CHECK_TEST(NetInterface::requestsCreate(tables, reinterpret_cast<NetInterfaceRequestsHandle*>(&tables)) == NI_INVALID_ARGUMENT &&
        tables.m_endpoints.initialCapacity == 1 && tables.m_endpoints.maximumCapacity == 1);
    NetInterfaceRequestsHandle raw = nullptr;
    CHECK_TEST(NetInterface::requestsCreate(tables, &raw) == NI_OK && raw);
    std::unique_ptr<NetInterfaceRequestsOpaque, void (*)(NetInterfaceRequestsHandle)> owner(raw, NetInterface::requestsDestroy);
    CHECK_TEST(NetInterface::requestsAddEndpoint(raw, 1, 0, 0, 0, 1, 0) == NI_OK &&
        NetInterface::requestsAddEndpoint(raw, 2, 0, 0, 0, 1, 0) == NI_LIMIT_EXCEEDED &&
        NetInterface::requestsRemoveEndpoint(raw, 1, 0) == NI_OK &&
        NetInterface::requestsAddEndpoint(raw, 2, 0, 0, 0, 1, 0) == NI_OK);
    LockFreeHashMapPoolConfig* tablePools[] = {&tables.m_requests, &tables.m_endpoints, &tables.m_cancellations};
    for (size_t i = 0; i < 3; ++i)
    {
        const int64_t saved = tablePools[i]->maximumCapacity;
        tablePools[i]->maximumCapacity = -2;
        raw = owner.get();
        CHECK_TEST(NetInterface::requestsCreate(tables, &raw) == NI_INVALID_ARGUMENT && !raw);
        tablePools[i]->maximumCapacity = saved;
    }

    NetInterfaceLinkOptions options = {};
    uint32_t entropy = 31;
    options.m_version = 3;
    options.m_role = NI_LINK_CLIENT;
    options.m_tls.entropy = HelloEntropy;
    options.m_tls.entropyContext = &entropy;
    options.m_tls.handshakeTimeoutMs = 5000;
    NetInterfaceLinkPools linkPools;
    NetInterfaceLinkHandle link = nullptr;
    options.m_claims = reinterpret_cast<const uint8_t*>(&link);
    options.m_claimsSize = sizeof(link);
    CHECK_TEST(NetInterface::linkCreate(&options, &link, &linkPools) == NI_INVALID_ARGUMENT && !link);
    options.m_claims = nullptr;
    options.m_claimsSize = 0;
    CHECK_TEST(NetInterface::linkCreate(&options, &link, &linkPools) >= 0 && link);
    std::unique_ptr<NetInterfaceLinkOpaque, void (*)(NetInterfaceLinkHandle)> linkOwner(link, NetInterface::linkDestroy);
    uint32_t* deadlines[] = {&linkPools.m_pingIntervalMs, &linkPools.m_pongTimeoutMs, &linkPools.m_drainTimeoutMs, &linkPools.m_controlStepBudget};
    for (unsigned i = 0; i < sizeof(deadlines) / sizeof(deadlines[0]); ++i)
    {
        const uint32_t saved = *deadlines[i];
        *deadlines[i] = 0;
        link = linkOwner.get();
        CHECK_TEST(NetInterface::linkCreate(&options, &link, &linkPools) == NI_INVALID_ARGUMENT && !link);
        *deadlines[i] = saved;
    }
    NetConnectionPoolConfig& tls = linkPools.m_controlConnection;
    LockFreeHashMapPoolConfig* records[] = {&linkPools.m_outputRecords, &tls.m_dataRecords, &tls.m_controlRecords};
    for (size_t i = 0; i < 3; ++i)
    {
        const int64_t saved = records[i]->maximumCapacity;
        records[i]->maximumCapacity = -2;
        link = linkOwner.get();
        CHECK_TEST(NetInterface::linkCreate(&options, &link, &linkPools) == NI_INVALID_ARGUMENT && !link);
        records[i]->maximumCapacity = saved;
    }
    NetBufferPoolConfig* groups[] = {&linkPools.m_outputBodies, &tls.m_dataBytes, &tls.m_controlBytes};
    for (size_t g = 0; g < 3; ++g)
    {
        for (size_t i = 0; i < 5; ++i)
        {
            LockFreeHashMapPoolConfig& pool = groups[g]->m_classes[i];
            const int64_t saved = pool.maximumCapacity;
            pool.maximumCapacity = -2;
            link = linkOwner.get();
            CHECK_TEST(NetInterface::linkCreate(&options, &link, &linkPools) == NI_INVALID_ARGUMENT && !link);
            pool.maximumCapacity = saved;
        }
    }
    NetInterfaceConnectionHandle conn = nullptr;
    int32_t detail = 0;
    CHECK_TEST(NetInterface::connectionCreateServiceClient(7, HelloEntropy, &entropy, 5000, &conn, &detail, &tls)
        == NET_INTERFACE_TLS_HANDSHAKE && conn);
    std::unique_ptr<NetInterfaceConnectionOpaque, void (*)(NetInterfaceConnectionHandle)> connection(conn, NetInterface::connectionDestroy);
    tls.m_dataRecords.maximumCapacity = -2;
    CHECK_TEST(NetInterface::connectionCreateServiceClient(7, HelloEntropy, &entropy, 5000, &conn, &detail, &tls)
        == NI_INVALID_ARGUMENT && !conn);
    return 0;
}

/** 只把一个指定的记录池或索引限制为一个槽，观察正常提交与无关短帧的接纳结果
@param [in] member 本轮要限制的物理池成员
@return 涉及该池的首项接纳、第二项背压且已接纳资源能全部结算时为0
*/
static int SubmissionPoolCapacity(LockFreeHashMapPoolConfig NetSenderSubmissionPools::* member)
{
    NetSenderSubmissionPools pools;
    LockFreeHashMapPoolConfig& limit = pools.*member;
    limit.initialCapacity = 1;
    limit.maximumCapacity = 1;
    NetSenderSubmission submission;
    CHECK_TEST(submission.configurePools(pools) == NI_OK && submission.start(1, 7) == NI_OK &&
        submission.ready(1, 8, 1024) == NI_OK);
    const uint8_t body[65] = {0x51};
    NetInterfaceFrame frame = {7, 1, 8, NI_DATA, sizeof(body), body};
    uint64_t request = 0, receipt = 0, refused = UINT64_MAX;
    if (member == &NetSenderSubmissionPools::m_requests || member == &NetSenderSubmissionPools::m_fronts ||
        member == &NetSenderSubmissionPools::m_requestIndex || member == &NetSenderSubmissionPools::m_inputs)
    {
        CHECK_TEST(submission.begin(7, 8, 60000, request) == NI_OK && request &&
            submission.begin(7, 8, 60000, refused) == NI_LIMIT_EXCEEDED && !refused);
    }
    else if (member == &NetSenderSubmissionPools::m_receipts || member == &NetSenderSubmissionPools::m_receiptIndex ||
        member == &NetSenderSubmissionPools::m_payloads)
    {
        CHECK_TEST(submission.dataSynchronous(frame, receipt) == NI_OK && receipt);
        ++frame.m_requestId;
        CHECK_TEST(submission.dataSynchronous(frame, refused) == NI_LIMIT_EXCEEDED && !refused);
    }
    else
    {
        if (member != &NetSenderSubmissionPools::m_frames)
        {
            frame.m_kind = NI_DATA_ACK;
        }
        CHECK_TEST(submission.data(frame) == NI_OK);
        ++frame.m_requestId;
        CHECK_TEST(submission.data(frame) == NI_LIMIT_EXCEEDED);
    }
    const NetInterfaceFrame independent = {7, 3, 8, NI_DATA, 1, body};
    CHECK_TEST(submission.data(independent) == NI_OK);
    submission.disconnected(1);
    if (receipt)
    {
        CHECK_TEST(submission.synchronousResult(receipt, true) == NI_INVALID_STATE);
    }
    NetSenderSubmissionCommand command = {};
    if (request)
    {
        CHECK_TEST(submission.takeClosed(command) && command.m_id == request);
    }
    CHECK_TEST(!submission.takeClosed(command) && submission.idle() && submission.queuedBytes() == 0);
    return 0;
}

/** 检查请求编号在执行者领取前后的可见性，以及断线期间的借用和队列归还
@return 每项请求只结算一次，重新握手后仅能读到新代正文时为0
*/
static int SubmissionOwnerLifetime()
{
    NetSenderSubmissionPools pools;
    NetSenderSubmissionOptions options;
    options.m_inputBatchBudget = options.m_inputBatchSize = 1;
    NetSenderSubmission submission(pools, options);
    CHECK_TEST(submission.start(1, 7) == NI_OK && submission.ready(1, 8, 1024) == NI_OK);
    const uint8_t value = 0x51;
    NetInterfaceFrame frame = {7, 0, 3, NI_DATA, 1, &value};
    for (uint32_t id = 1; id <= 8; ++id)
    {
        frame.m_requestId = id;
        CHECK_TEST(submission.data(frame) == NI_OK);
    }
    CHECK_TEST(submission.beginPoll() == NI_OK);
    const NetInterfaceFrame* borrowed = submission.nextData();
    CHECK_TEST(borrowed && borrowed->m_payload && borrowed->m_requestId == 1);
    uint64_t pending = 0, ticket = 0;
    const uint64_t before = NetSenderSubmission::now();
    CHECK_TEST(submission.begin(7, 8, 60000, pending) == NI_OK && pending);
    const uint64_t originalBody = UINT64_C(0x2122232425262728);
    uint64_t bodyAndTicket = originalBody;
    const uint8_t* requestBody = reinterpret_cast<const uint8_t*>(&bodyAndTicket);
    CHECK_TEST(submission.sendSynchronous(pending, requestBody, sizeof(bodyAndTicket), bodyAndTicket) == NI_INVALID_ARGUMENT &&
        bodyAndTicket == originalBody && !submission.synchronousPending());
    CHECK_TEST(submission.sendSynchronous(pending, &value, 1, ticket) == NI_OK && ticket);
    uint32_t remaining = 0;
    CHECK_TEST(submission.synchronousResult(ticket, false, &remaining) == NI_READY);
    CHECK_TEST(remaining <= 60000 && remaining + NetSenderSubmission::now() - before >= 60000);
    submission.disconnected(1);
    NetSenderSubmissionCommand command = {};
    CHECK_TEST(submission.synchronousResult(ticket, true) == NI_INVALID_STATE);
    CHECK_TEST(submission.takeClosed(command) && command.m_id == pending && command.m_type == 8 &&
        !submission.takeClosed(command));
    CHECK_TEST(submission.ready(2, 8, 1024) == NI_PROTOCOL_ERROR &&
        borrowed->m_payloadSize == 1 && borrowed->m_payload[0] == value);
    submission.finish(NI_OK);
    CHECK_TEST(submission.ready(2, 8, 1024) == NI_OK && submission.idle() && submission.queuedBytes() == 0);
    frame.m_requestId = 9;
    CHECK_TEST(submission.data(frame) == NI_OK && submission.beginPoll() == NI_OK);
    borrowed = submission.nextData();
    CHECK_TEST(borrowed && borrowed->m_requestId == 9 && borrowed->m_payload && borrowed->m_payload[0] == value);
    submission.finish(NI_OK);

    CHECK_TEST(submission.begin(7, 14, 60000, pending) == NI_OK);
    command.m_id = UINT64_MAX;
    CHECK_TEST(!submission.complete(2, pending, command) && !command.m_id && command.m_kind == NET_SUBMIT_NONE);
    CHECK_TEST(!submission.complete(2, pending, command) && submission.beginPoll() == NI_OK);
    CHECK_TEST(!submission.complete(1, pending, command) && submission.complete(2, pending, command) &&
        command.m_id == pending && command.m_generation == 2 && command.m_product == 7 && command.m_type == 14);
    CHECK_TEST(!submission.complete(2, pending, command) && submission.idle());

    uint64_t first = 0, second = 0, last = 0;
    CHECK_TEST(submission.begin(7, 10, 60000, first) == NI_OK &&
        submission.begin(7, 11, 60000, second) == NI_OK &&
        submission.begin(7, 12, 60000, last) == NI_OK && first && first < second && second < last);
    CHECK_TEST(!submission.complete(2, last, command) && command.m_kind == NET_SUBMIT_NONE);
    CHECK_TEST(submission.ready(2, 8, 1024) == NI_OK);
    submission.disconnected(2);
    CHECK_TEST(submission.ready(2, 8, 1024) == NI_PROTOCOL_ERROR &&
        submission.ready(3, 8, 1024) == NI_PROTOCOL_ERROR);
    CHECK_TEST(submission.takeClosed(command) && command.m_id == first && command.m_type == 10);
    CHECK_TEST(submission.takeClosed(command) && command.m_id == second && command.m_type == 11);
    CHECK_TEST(submission.takeClosed(command) && command.m_id == last && command.m_type == 12);
    CHECK_TEST(!submission.takeClosed(command) && submission.idle());

    CHECK_TEST(submission.ready(2, 8, 1024) == NI_OK && submission.ready(3, 8, 1024) == NI_OK);
    frame.m_requestId = 41;
    CHECK_TEST(submission.data(frame) == NI_OK);
    frame.m_requestId = 42;
    CHECK_TEST(submission.data(frame) == NI_OK);
    CHECK_TEST(submission.begin(7, 13, 60000, pending) == NI_OK && submission.beginPoll() == NI_OK);
    uint32_t messages = 0;
    uint64_t bytes = 0;
    // context借用当前提交表；接收第一帧时断线，在回调返回前检查正文仍属于该帧。
    CHECK_TEST(submission.drainData(&submission, [](void* context, const NetInterfaceFrame& view) -> int32_t {
        NetSenderSubmission& owner = *static_cast<NetSenderSubmission*>(context);
        owner.disconnected(3);
        return view.m_requestId == 41 && view.m_payloadSize == 1 && view.m_payload && view.m_payload[0] == 0x51 ?
            NI_OK : NI_PROTOCOL_ERROR;
    }, 8, 32, messages, bytes) == NI_OK && messages == 1 && bytes == 1);
    CHECK_TEST(submission.takeClosed(command) && command.m_id == pending && command.m_type == 13 &&
        !submission.takeClosed(command) && submission.idle() && submission.queuedBytes() == 0);
    return 0;
}

/** 填满入口后只推进一次，通过生产者能重新占用的槽数观测预取预算
@param [in] batches 每次允许领取的批数
@param [in] items 每批领取的项数
@return 每个已领取的槽可再次接纳一项，所有长帧按序交付时为0
*/
static int SubmissionPrefetch(uint32_t batches, uint32_t items)
{
    NetSenderSubmissionPools pools;
    pools.m_queue.initialCapacity = 16;
    pools.m_queue.maximumCapacity = 16;
    NetSenderSubmissionOptions options;
    CHECK_TEST(options.m_inputBatchBudget == 8 && options.m_inputBatchSize == 32);
    options.m_inputBatchBudget = batches;
    options.m_inputBatchSize = items;
    NetSenderSubmission submission;
    CHECK_TEST(submission.configurePools(pools, options) == NI_OK);
    options.m_inputBatchBudget = 0;
    CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT);
    options.m_inputBatchBudget = batches;
    options.m_inputBatchSize = 33;
    CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT);
    options.m_inputBatchSize = 0;
    CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT &&
        submission.start(1, 7) == NI_OK && submission.ready(1, 32, 1024) == NI_OK);
    uint8_t body[65] = {};
    for (size_t i = 1; i < sizeof(body); ++i)
    {
        body[i] = static_cast<uint8_t>(3 * i + 7);
    }
    NetInterfaceFrame frame = {7, 1, 9, NI_DATA, sizeof(body), body};
    uint32_t original = 0;
    int32_t status = NI_OK;
    while (original <= 16)
    {
        frame.m_requestId = original + 1;
        body[0] = static_cast<uint8_t>(frame.m_requestId);
        status = submission.data(frame);
        if (status != NI_OK) { break; }
        ++original;
    }
    CHECK_TEST(status == NI_LIMIT_EXCEEDED && original >= 8 && original <= 16 && submission.beginPoll() == NI_OK);
    const uint32_t maximum = (std::min)(original, batches * items);
    uint32_t freed = 0;
    for (; freed <= maximum; ++freed)
    {
        frame.m_requestId = original + freed + 1;
        body[0] = static_cast<uint8_t>(frame.m_requestId);
        status = submission.data(frame);
        if (status != NI_OK) { break; }
    }
    CHECK_TEST(status == NI_LIMIT_EXCEEDED && freed <= maximum);
    // 基础队列收到短前缀后可切换到逐项领取。小批量验证上下界，长前缀另验准确数量。
    if (items == 3) { CHECK_TEST(freed >= batches); }
    else { CHECK_TEST(freed == maximum); }
    for (uint32_t id = 1; id <= original + freed; ++id)
    {
        const NetInterfaceFrame* next = submission.nextData();
        CHECK_TEST(next && next->m_requestId == id && next->m_payloadSize == sizeof(body) &&
            next->m_payload && next->m_payload[0] == id &&
            std::memcmp(next->m_payload + 1, body + 1, sizeof(body) - 1) == 0);
        if (id == 1)
        {
            NetSenderSubmissionCommand command = {};
            CHECK_TEST(submission.beginPoll() == NI_INVALID_STATE &&
                submission.next(command) == NI_INVALID_STATE && command.m_kind == NET_SUBMIT_NONE);
            CHECK_TEST(submission.data(frame) == NI_LIMIT_EXCEEDED && next->m_payload[0] == 1);
        }
        submission.finish(NI_OK);
    }
    CHECK_TEST(submission.idle() && submission.queuedBytes() == 0);
    return 0;
}

/** 在回执索引扩容期间占用唯一保护记录，验证公开查询和通知登记在恢复后的行为
@return 状态查询、一次通知和两个唯一终态均正确时返回0
*/
static int SubmissionReceiptProtection()
{
    const std::shared_ptr<TestSubmissionObserver> observer(new TestSubmissionObserver());
    uint64_t first = 0;
    bool checked = false;
    NetSenderSubmissionWait wait;
    wait.m_syncPollSleepMs = 7;
    NetSenderSubmission submission;
    NetSenderSubmissionPools pools;
    pools.m_receiptIndex.initialCapacity = 1;
    pools.m_receiptIndex.maximumCapacity = 2;
    pools.m_receiptIndex.setProtectionRecordCount(1);
    pools.m_receiptIndex.setGrowPolicy([&](const LockFreeHashMapGrowContext&) -> size_t {
        // 第二张票据正在登记；第一张已完成但尚未消费。
        uint32_t remaining = UINT32_MAX;
        int32_t reason = NI_INVALID_STATE;
        checked = submission.synchronousResult(first, true, &remaining) == NI_READY && remaining == 7 &&
            submission.synchronousResult(first, false) == NI_READY &&
            submission.observeSynchronous(first, observer) == NI_LIMIT_EXCEEDED && observer->m_count == 0 &&
            submission.synchronousResult(0, true, &remaining) == NI_NOT_FOUND && remaining == 0 &&
            submission.synchronousResult(UINT64_MAX, true) == NI_NOT_FOUND &&
            submission.observeSynchronous(UINT64_MAX, observer) == NI_NOT_FOUND &&
            !submission.closeRequested(reason);
        return 1;
    });
    CHECK_TEST(submission.configureProducerWait(wait) == NI_OK && submission.configurePools(pools) == NI_OK && submission.start(1, 7) == NI_OK &&
        submission.ready(1, 8, 1024) == NI_OK);
    const uint8_t body = 0x51;
    NetInterfaceFrame frame = {7, 1, 8, NI_DATA, 1, &body};
    NetSenderSubmissionCommand command = {};
    CHECK_TEST(submission.dataSynchronous(frame, first) == NI_OK && first != 0 &&
        submission.beginPoll() == NI_OK && submission.next(command) == NI_READY && command.m_frame.m_requestId == 1);
    submission.finish(NI_OK);
    submission.advanceSynchronous(NI_OK);
    CHECK_TEST(submission.synchronousResult(first, false) == NI_OK);

    uint64_t second = 0;
    frame.m_requestId = 2;
    CHECK_TEST(submission.dataSynchronous(frame, second) == NI_OK && second > first && checked);
    int32_t reason = NI_INVALID_STATE;
    CHECK_TEST(!submission.closeRequested(reason) && submission.observeSynchronous(first, observer) == NI_OK &&
        observer->m_count == 1 && submission.synchronousResult(first, true) == NI_OK &&
        submission.synchronousResult(first, true) == NI_NOT_FOUND);
    CHECK_TEST(submission.beginPoll() == NI_OK && submission.next(command) == NI_READY && command.m_frame.m_requestId == 2);
    submission.finish(NI_OK);
    submission.advanceSynchronous(NI_OK);
    CHECK_TEST(submission.synchronousResult(second, true) == NI_OK && submission.synchronousResult(second, true) == NI_NOT_FOUND &&
        !submission.closeRequested(reason) && observer->m_count == 1 && submission.idle());
    return 0;
}

/** 检查提交层的每个物理池和索引均能独立拒绝错误配置，并观察入口真实扩容、按序交付和归还
@return 成功为0
*/
static int SubmissionConfiguration()
{
    CHECK_TEST(SubmissionPrefetch(1, 1) == 0 && SubmissionPrefetch(2, 3) == 0 &&
        SubmissionPrefetch(2, 4) == 0 && SubmissionPrefetch(8, 32) == 0);
    CHECK_TEST(SubmissionOwnerLifetime() == 0);
    CHECK_TEST(SubmissionReceiptProtection() == 0);
    const auto boundedPools = {
        &NetSenderSubmissionPools::m_requests, &NetSenderSubmissionPools::m_frames,
        &NetSenderSubmissionPools::m_controls, &NetSenderSubmissionPools::m_fronts,
        &NetSenderSubmissionPools::m_receipts, &NetSenderSubmissionPools::m_inputs,
        &NetSenderSubmissionPools::m_payloads, &NetSenderSubmissionPools::m_controlPayloads,
        &NetSenderSubmissionPools::m_requestIndex, &NetSenderSubmissionPools::m_receiptIndex
    };
    for (auto pool : boundedPools)
    {
        CHECK_TEST(SubmissionPoolCapacity(pool) == 0);
    }
    unsigned grows = 0, published = 0, maintenance = 0;
    NetSenderSubmissionPools pools;
    pools.m_queue.initialCapacity = 2;
    pools.m_queue.maximumCapacity = -1;
    pools.m_queue.m_onGrowth = [&](size_t added, uint64_t) { if (added) { ++published; } };
    NetSenderSubmissionOptions options;
    CHECK_TEST(options.m_capacityRetriesBeforeSleep == 4096 && options.m_capacitySleepMilliseconds == 1);
    options.m_capacityRetriesBeforeSleep = 0;
    options.m_capacitySleepMilliseconds = 2;
    options.m_requestBuckets = 13;
    options.m_receiptBuckets = 31;
    options.m_distributedCounters = false;
    options.m_cancelableReservations = true;
    options.m_maintenanceInterval = 1;
    options.m_maintenanceBudget = 1;
    options.m_consumerYieldInterval = 1;
    options.m_growPolicy = [&](const LockFreeQueueGrowContext&) -> size_t { ++grows; return 4; };
    options.m_shrinkPolicy = [&](const LockFreeQueueShrinkContext&) -> LockFreeQueueShrinkDecision {
        ++maintenance;
        const LockFreeQueueShrinkDecision keep = {0, 0};
        return keep;
    };
    options.m_deliveryTimeoutMs = 1;
    uint32_t wakes = 0;
    NetSenderSubmissionGate gate = {};
    gate.m_context = &wakes;
    gate.m_wake = [](void* context) { ++*static_cast<uint32_t*>(context); };
    NetSenderSubmission submission(pools, options);
    NetSenderSubmissionWait wait;
    NetSenderSubmission independent;
    CHECK_TEST(wait.m_yieldInterval == 64 && wait.m_quiesceSleepMs == 1 &&
        submission.producerWait().m_yieldInterval == 64 && submission.producerWait().m_quiesceSleepMs == 1);
    CHECK_TEST(wait.m_syncPollSleepMs == 1 && submission.producerWait().m_syncPollSleepMs == 1);
    wait.m_yieldInterval = UINT32_MAX;
    wait.m_quiesceSleepMs = UINT32_MAX;
    wait.m_syncPollSleepMs = UINT32_MAX;
    CHECK_TEST(submission.configureProducerWait(wait) == NI_OK);
    wait.m_yieldInterval = 1;
    wait.m_quiesceSleepMs = 2;
    wait.m_syncPollSleepMs = 3;
    CHECK_TEST(submission.producerWait().m_syncPollSleepMs == UINT32_MAX && submission.producerWait().m_yieldInterval == UINT32_MAX &&
        submission.producerWait().m_quiesceSleepMs == UINT32_MAX && submission.configureProducerWait(wait) == NI_OK);
    wait.m_yieldInterval = 0;
    CHECK_TEST(submission.configureProducerWait(wait) == NI_INVALID_ARGUMENT);
    wait.m_yieldInterval = 7;
    wait.m_syncPollSleepMs = 0;
    CHECK_TEST(submission.configureProducerWait(wait) == NI_INVALID_ARGUMENT);
    wait.m_syncPollSleepMs = 3;
    wait.m_quiesceSleepMs = 0;
    CHECK_TEST(submission.configureProducerWait(wait) == NI_INVALID_ARGUMENT &&
        independent.producerWait().m_yieldInterval == 64 && independent.producerWait().m_quiesceSleepMs == 1);
    CHECK_TEST(submission.configureGate(gate) == NI_OK && submission.configureGate(gate) == NI_INVALID_STATE);
    CHECK_TEST(submission.configurePools(pools, options) == NI_OK);
    LockFreeHashMapPoolConfig* records[] = {&pools.m_requests, &pools.m_frames, &pools.m_controls,
        &pools.m_fronts, &pools.m_receipts, &pools.m_inputs, &pools.m_payloads, &pools.m_controlPayloads,
        &pools.m_requestIndex, &pools.m_receiptIndex};
    for (size_t i = 0; i < sizeof(records) / sizeof(records[0]); ++i)
    {
        const int64_t saved = records[i]->maximumCapacity;
        records[i]->maximumCapacity = -2;
        CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT);
        records[i]->maximumCapacity = saved;
    }
    for (size_t i = 0; i < 5; ++i)
    {
        NetBufferPoolConfig* groups[] = {&pools.m_bodies, &pools.m_controlBodies};
        for (size_t g = 0; g < 2; ++g)
        {
            LockFreeHashMapPoolConfig& cfg = groups[g]->m_classes[i];
            const int64_t saved = cfg.maximumCapacity;
            cfg.maximumCapacity = -2;
            CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT);
            cfg.maximumCapacity = saved;
        }
    }
    size_t* indexes[] = {&options.m_requestBuckets, &options.m_receiptBuckets};
    for (size_t i = 0; i < 2; ++i)
    {
        const size_t saved = *indexes[i];
        *indexes[i] = SIZE_MAX;
        CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT);
        *indexes[i] = saved;
    }
    size_t* budgets[] = {&options.m_queuePollBudget, &options.m_requestPollBudget, &options.m_receiptPollBudget};
    for (size_t i = 0; i < 3; ++i)
    {
        const size_t saved = *budgets[i];
        *budgets[i] = 0;
        CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT);
        *budgets[i] = saved;
    }
    options.m_deliveryTimeoutMs = 0;
    CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT);
    {
        NetSenderSubmission invalid(pools, options);
        CHECK_TEST(invalid.start(1, 7) == NI_INVALID_ARGUMENT && invalid.beginPoll() == NI_INVALID_ARGUMENT);
    }
    options.m_deliveryTimeoutMs = 1;
    options.m_maintenanceInterval = 0;
    CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT);
    options.m_maintenanceInterval = 1;
    options.m_maintenanceBudget = 0;
    CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT);
    options.m_maintenanceBudget = 1;
    options.m_capacitySleepMilliseconds = 0;
    CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT);
    options.m_capacitySleepMilliseconds = 2;
    options.m_consumerYieldInterval = 65537;
    CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT);
    options.m_consumerYieldInterval = 1;
    pools.m_queue.maximumCapacity = -2;
    CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_ARGUMENT);
    pools.m_queue.maximumCapacity = -1;
    CHECK_TEST(submission.start(1, 7) == NI_OK && submission.ready(1, 1, 1024) == NI_OK);
    CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_STATE &&
        submission.configureProducerWait(wait) == NI_INVALID_STATE &&
        submission.producerWait().m_yieldInterval == 1 && submission.producerWait().m_quiesceSleepMs == 2 &&
        submission.producerWait().m_syncPollSleepMs == 3);
    for (uint32_t i = 1; i <= 32; ++i)
    {
        const uint8_t byte = static_cast<uint8_t>(i);
        const NetInterfaceFrame frame = {7, i, 0, NI_DATA, 1, &byte};
        CHECK_TEST(submission.data(frame) == NI_OK);
    }
    CHECK_TEST(wakes > 0 && grows > 0 && published > 0 && submission.queuedBytes() == 32);
    for (uint32_t i = 1; i <= 32; ++i)
    {
        CHECK_TEST(submission.beginPoll() == NI_OK);
        const NetInterfaceFrame* frame = submission.nextData();
        CHECK_TEST(frame && frame->m_requestId == i && frame->m_payloadSize == 1 && frame->m_payload[0] == i);
        submission.finish(NI_OK);
    }
    CHECK_TEST(submission.beginPoll() == NI_OK && !submission.nextData() && submission.idle() &&
        submission.queuedBytes() == 0 && maintenance > 0);
    const uint64_t originalBody = UINT64_C(0x3132333435363738);
    uint64_t dataAndTicket = originalBody;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&dataAndTicket);
    NetInterfaceFrame synchronous = {7, 33, 0, NI_DATA, sizeof(dataAndTicket), data};
    CHECK_TEST(submission.dataSynchronous(synchronous, synchronous.m_requestId) == NI_INVALID_ARGUMENT &&
        synchronous.m_requestId == 33 && submission.idle());
    CHECK_TEST(submission.dataSynchronous(synchronous, dataAndTicket) == NI_INVALID_ARGUMENT &&
        dataAndTicket == originalBody && submission.idle());
    uint64_t ticket = 0;
    CHECK_TEST(submission.dataSynchronous(synchronous, ticket) == NI_OK && ticket);
    std::shared_ptr<TestSubmissionObserver> observer(new TestSubmissionObserver);
    CHECK_TEST(submission.observeSynchronous(ticket, std::shared_ptr<NetSenderSubmissionSignal>()) == NI_INVALID_ARGUMENT &&
        submission.observeSynchronous(ticket, observer) == NI_OK && submission.observeSynchronous(ticket, observer) == NI_INVALID_STATE);
    uint32_t remaining = UINT32_MAX;
    CHECK_TEST(submission.synchronousResult(ticket, false, &remaining) == NI_READY && remaining <= 1);
    submission.advanceSynchronous(NI_READY, NetSenderSubmission::now() + 1);
    remaining = UINT32_MAX;
    CHECK_TEST(observer->m_count == 1 && submission.synchronousResult(ticket, true, &remaining) == NI_TIMEOUT && remaining == 0 &&
        submission.synchronousResult(ticket, true, &remaining) == NI_NOT_FOUND && remaining == 0 &&
        submission.observeSynchronous(ticket, observer) == NI_NOT_FOUND);
    NetSenderSubmissionPools bounded;
    bounded.m_queue.initialCapacity = 2;
    bounded.m_queue.maximumCapacity = 2;
    NetSenderSubmission limited(bounded, options);
    CHECK_TEST(limited.start(1, 7) == NI_OK && limited.ready(1, 1, 1024) == NI_OK);
    uint64_t accepted = 0;
    int32_t result = NI_OK;
    for (uint64_t id = 1; id <= 3 && result == NI_OK; ++id)
    {
        const NetInterfaceFrame item = {7, id, 0, NI_DATA, 0, nullptr};
        result = limited.data(item);
        if (result == NI_OK) { ++accepted; }
    }
    CHECK_TEST(accepted > 0 && result == NI_LIMIT_EXCEEDED);
    for (uint64_t id = 1; id <= accepted; ++id)
    {
        CHECK_TEST(limited.beginPoll() == NI_OK);
        const NetInterfaceFrame* item = limited.nextData();
        CHECK_TEST(item && item->m_requestId == id);
        limited.finish(NI_OK);
    }
    const NetInterfaceFrame retry = {7, accepted + 1, 0, NI_DATA, 0, nullptr};
    CHECK_TEST(limited.data(retry) == NI_OK && limited.beginPoll() == NI_OK);
    const NetInterfaceFrame* item = limited.nextData();
    CHECK_TEST(item && item->m_requestId == retry.m_requestId);
    limited.finish(NI_OK);
    return 0;
}

PoolTestSink::PoolTestSink() :
m_router(nullptr),
m_closed(0),
m_lastClosed(0)
{

}

int32_t PoolTestSink::send(uint64_t, const NetInterfaceFrame&, const NetEndpointOrigin&)
{
    return NI_NOT_SUPPORTED;
}

void PoolTestSink::close(uint64_t endpoint)
{
    ++m_closed;
    m_lastClosed = endpoint;
    if (m_router != nullptr) { m_router->shutdown(); }
}

/** 使路由端点池实际耗尽、回收并重用，验证两层会话转交各自的请求池设置
@return 成功为0
*/
static int OwnerPools()
{
    {
        PoolTestSink closing;
        NetEndpointRouter router(&closing);
        closing.m_router = &router;
        uint64_t first = 0, second = 0;
        CHECK_TEST(router.addService(7, false, 1, first) == NI_OK &&
            router.addService(8, false, 1, second) == NI_OK);
        router.closeEndpoint(first, NI_CANCELLED);
        CHECK_TEST(closing.m_closed == 1 && closing.m_lastClosed == first);
        CHECK_TEST(!router.poll() && closing.m_closed == 2 && closing.m_lastClosed == second);
        CHECK_TEST(!router.poll() && closing.m_closed == 2 && router.bindTransport(nullptr) == NI_OK);
    }
    PoolTestSink sink;
    for (unsigned attached = 0; attached < 2; ++attached)
    {
        NetEndpointRouter limited(&sink, attached ? &sink : nullptr);
        NetEndpointPools pools;
        pools.m_requests.m_endpoints.initialCapacity = 1;
        pools.m_requests.m_endpoints.maximumCapacity = 1;
        CHECK_TEST(limited.configurePools(pools) == NI_OK);
        NetEndpointOrigin origin = {};
        origin.m_endpoint = 88;
        origin.m_platform = origin.m_device[0] = 1;
        const uint32_t features[] = {1, 3, 32, 1024};
        uint64_t id = 99;
        const int32_t rejected = attached ? limited.addClient(origin, features, id) : limited.addService(7, false, 32, id);
        CHECK_TEST(rejected == NI_LIMIT_EXCEEDED && id == 0 && limited.pendingCount() == 0);
        pools.m_requests.m_endpoints.maximumCapacity = 2;
        CHECK_TEST(limited.configurePools(pools) == NI_OK && limited.configureFileLeaseLimit(30) == NI_OK);
        const int32_t accepted = attached ? limited.addClient(origin, features, id) : limited.addService(7, false, 32, id);
        CHECK_TEST(accepted == NI_OK && id > 1 && limited.configureFileLeaseLimit(31) == NI_INVALID_STATE);
    }
    NetEndpointRouter router(&sink);
    NetEndpointPools config;
    LockFreeHashMapPoolConfig* all[] = {&config.m_files, &config.m_endpoints, &config.m_requests.m_endpoints,
        &config.m_requests.m_requests, &config.m_requests.m_cancellations};
    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i)
    {
        const int64_t saved = all[i]->maximumCapacity;
        all[i]->maximumCapacity = -2;
        CHECK_TEST(router.configurePools(config) == NI_INVALID_ARGUMENT);
        all[i]->maximumCapacity = saved;
    }
    uint32_t* budgets[] = {&config.m_expiredRequestBudget, &config.m_closingEndpointBudget, &config.m_expiredFileBudget};
    for (size_t i = 0; i < 3; ++i)
    {
        CHECK_TEST(*budgets[i] == 64);
        *budgets[i] = 0;
        CHECK_TEST(router.configurePools(config) == NI_INVALID_ARGUMENT);
        *budgets[i] = 1;
    }
    config.m_endpoints.initialCapacity = 1;
    config.m_endpoints.maximumCapacity = 1;
    CHECK_TEST(router.configurePools(config) == NI_OK);
    uint64_t first = 0, second = 0;
    CHECK_TEST(router.addService(7, false, 1, first) == NI_OK && first);
    CHECK_TEST(router.addService(8, false, 1, second) == NI_LIMIT_EXCEEDED && !second);
    CHECK_TEST(router.configurePools(config) == NI_INVALID_STATE);
    router.closeEndpoint(first, NI_CANCELLED);
    router.poll();
    CHECK_TEST(router.addService(8, false, 1, second) == NI_OK && second > first);

    NetClientSession client;
    NetSession session;
    LockFreeHashMapPoolConfig cfg;
    cfg.maximumCapacity = -2;
    CHECK_TEST(session.configureRequestPool(cfg) == NI_INVALID_ARGUMENT && client.configureRequestPool(cfg) == NI_INVALID_ARGUMENT);
    cfg.initialCapacity = 1;
    cfg.maximumCapacity = -1;
    CHECK_TEST(session.configureRequestPool(cfg) == NI_OK && client.configureRequestPool(cfg) == NI_OK && !session.generation());
    CHECK_TEST(client.configureCompression(nullptr, 1) == NI_INVALID_ARGUMENT);
    const uint32_t types[] = {1, 2};
    CHECK_TEST(client.configureCompression(types, 2) == NI_OK && session.configureCompression(types, 2) == NI_OK);
    CHECK_TEST(client.configureGateways("127.0.0.1:8000", "") == NI_OK &&
        client.configureGateways("127.0.0.1:8000", std::string("a\0b", 3)) == NI_INVALID_ARGUMENT);
    NetConnectionPoolConfig conn;
    CHECK_TEST(client.configureConnectionPools(conn) == NI_OK);
    conn.m_compressionLevel = 10;
    CHECK_TEST(client.configureConnectionPools(conn) == NI_INVALID_ARGUMENT);
    NetInterfaceTlsOptions tls = {};
    NetInterfaceHandshakeOptions hello = {};
    CHECK_TEST(client.connect("127.0.0.1", 8000, tls, hello, nullptr) == NI_NOT_SUPPORTED);
    return 0;
}

/** 分别填满底层 TLS 密文输入和输出，驱动另一条连接验证无上限模式及零值默认握手时限
@return 成功为0
*/
static int CiphertextConfiguration()
{
    CEncodeDecodeTlsQueueLimits limits;
    CHECK_TEST(limits.inputBytes() == 32768 && limits.outputBytes() == 32768);
    CHECK_TEST(limits.set(7, 19) && !limits.set(0, 3) && !limits.set(3, -2) &&
        limits.inputBytes() == 7 && limits.outputBytes() == 19);
    uint32_t entropy = 17;
    NetInterfaceTlsOptions tls = {};
    tls.authority = NetInterface::trustAuthority(&tls.authoritySize);
    tls.hostname = "service.7";
    tls.entropy = HelloEntropy;
    tls.entropyContext = &entropy;
    CHECK_TEST(tls.effectiveHandshakeTimeout() == 10000 && tls.setHandshakeTimeout(100) &&
        !tls.setHandshakeTimeout(99) && tls.handshakeTimeoutMs == 100 && tls.setHandshakeTimeout(60000) &&
        !tls.setHandshakeTimeout(60001) && tls.handshakeTimeoutMs == 60000 && tls.setHandshakeTimeout(0));
    uint32_t used = 0, length = 0;
    std::vector<uint8_t> incoming(40000, 0);
    tls.server = 1;
    NetInterfaceTlsHandle aliasRaw = nullptr;
    const int32_t aliasStatus = NetInterface::tlsCreate(&tls, &aliasRaw, &tls.server, limits);
    std::unique_ptr<NetInterfaceTlsOpaque, void (*)(NetInterfaceTlsHandle)> aliasOwner(aliasRaw, NetInterface::tlsDestroy);
    CHECK_TEST(aliasStatus == NET_INTERFACE_TLS_ARGUMENT && tls.server == 1 && !aliasRaw);
    NetInterfaceConnectionHandle aliasConnection = nullptr;
    CHECK_TEST(NetInterface::connectionCreate(&tls, &aliasConnection, &tls.server) == NI_INVALID_ARGUMENT &&
        tls.server == 1 && !aliasConnection);
    int32_t certWord = 37, keyWord = 19;
    tls.certificate = reinterpret_cast<const uint8_t*>(&certWord);
    tls.certificateSize = sizeof(certWord);
    tls.privateKey = reinterpret_cast<const uint8_t*>(&keyWord);
    tls.privateKeySize = sizeof(keyWord);
    CHECK_TEST(NetInterface::tlsCreate(&tls, &aliasRaw, &certWord) == NET_INTERFACE_TLS_ARGUMENT && certWord == 37 && !aliasRaw);
    CHECK_TEST(NetInterface::connectionCreate(&tls, &aliasConnection, &keyWord) == NI_INVALID_ARGUMENT &&
        keyWord == 19 && !aliasConnection);
    tls.server = 0;
    NetConnectionPoolConfig factoryPools;
    const int32_t originalLevel = factoryPools.m_compressionLevel;
    CHECK_TEST(NetInterface::connectionCreateServiceClient(7, HelloEntropy, &entropy, 0, &aliasConnection,
        &factoryPools.m_compressionLevel, &factoryPools, &limits) == NI_INVALID_ARGUMENT &&
        !aliasConnection && factoryPools.m_compressionLevel == originalLevel);
    int32_t caWord = 17;
    CHECK_TEST(NetInterface::tlsCreateClient(reinterpret_cast<const uint8_t*>(&caWord), sizeof(caWord),
        tls.hostname, HelloEntropy, &entropy, 0, &aliasRaw, &caWord) == NET_INTERFACE_TLS_ARGUMENT && !aliasRaw && caWord == 17);
    TestTlsOutputs sharedOutput = {};
    CHECK_TEST(NetInterface::tlsCreate(&tls, &sharedOutput.m_handle, &sharedOutput.m_detail) == NET_INTERFACE_TLS_ARGUMENT &&
        !sharedOutput.m_handle);
    int32_t hostStorage[4] = {};
    char* nameBytes = reinterpret_cast<char*>(hostStorage);
    std::memcpy(nameBytes, "service.7", 10);
    CHECK_TEST(NetInterface::tlsCreateClient(tls.authority, tls.authoritySize, nameBytes, HelloEntropy, &entropy,
        0, &aliasRaw, &hostStorage[1], limits) == NET_INTERFACE_TLS_ARGUMENT && !aliasRaw &&
        std::memcmp(nameBytes, "service.7", 10) == 0);
    CHECK_TEST(NetInterface::tlsCreateClient(tls.authority, tls.authoritySize, nameBytes, HelloEntropy, &entropy,
        0, &aliasRaw, &hostStorage[3], limits) == NET_INTERFACE_TLS_HANDSHAKE && aliasRaw);
    NetInterface::tlsDestroy(aliasRaw);
    NetInterfaceTlsHandle raw = nullptr;
    CHECK_TEST(NetInterface::tlsCreate(&tls, &raw, nullptr, limits) == NET_INTERFACE_TLS_HANDSHAKE && raw);
    std::unique_ptr<NetInterfaceTlsOpaque, void (*)(NetInterfaceTlsHandle)> owner(raw, NetInterface::tlsDestroy);
    uint32_t overlapped = 0xa5a55a5aU;
    uint8_t* shared = reinterpret_cast<uint8_t*>(&overlapped);
    CHECK_TEST(NetInterface::tlsFeed(raw, shared, 4, &overlapped) == NET_INTERFACE_TLS_ARGUMENT &&
        NetInterface::tlsWrite(raw, shared, 4, &overlapped) == NET_INTERFACE_TLS_ARGUMENT &&
        NetInterface::tlsRead(raw, shared, 4, &overlapped) == NET_INTERFACE_TLS_ARGUMENT && overlapped == 0xa5a55a5aU);
    NetInterfaceTls rawObject(tls, limits);
    CHECK_TEST(rawObject.append(shared, 4, overlapped) == NET_INTERFACE_TLS_ARGUMENT && overlapped == 0xa5a55a5aU);
    CHECK_TEST(NetInterface::tlsFeed(raw, incoming.data(), 8, &used) == NET_INTERFACE_TLS_WANT_IO && used == 7);
    CHECK_TEST(NetInterface::tlsFeed(raw, incoming.data(), 1, &used) == NET_INTERFACE_TLS_WANT_IO && used == 0);

    NetInterfaceConnectionHandle conn = nullptr;
    CHECK_TEST(NetInterface::connectionCreateServiceClient(7, HelloEntropy, &entropy, 0, &conn, nullptr, nullptr, &limits)
        == NET_INTERFACE_TLS_HANDSHAKE && conn);
    std::unique_ptr<NetInterfaceConnectionOpaque, void (*)(NetInterfaceConnectionHandle)> connection(conn, NetInterface::connectionDestroy);
    CHECK_TEST(NetInterface::connectionFeed(conn, shared, 4, &overlapped) == NET_INTERFACE_TLS_ARGUMENT &&
        overlapped == 0xa5a55a5aU);
    const uint32_t compressType = 9;
    CHECK_TEST(NetInterface::connectionConfigureCompression(conn, &compressType, 1, 2) == NI_INVALID_ARGUMENT &&
        NetInterface::connectionConfigureCompression(conn, nullptr, 1, 1) == NI_INVALID_ARGUMENT &&
        NetInterface::connectionConfigureCompression(conn, &compressType, 1, 1) == NI_OK &&
        NetInterface::connectionConfigureCompression(conn, nullptr, 0, 0) == NI_OK);
    CHECK_TEST(NetInterface::connectionProgress(conn, 100, 0) >= 0);
    const uint8_t* bytes = nullptr;
    CHECK_TEST(NetInterface::connectionPeekOutput(conn, &bytes, &length) >= 0 && bytes && length == 19);
    CHECK_TEST(NetInterface::connectionFeed(conn, incoming.data(), 8, &used) == NET_INTERFACE_TLS_WANT_IO && used == 7);

    CHECK_TEST(limits.set(-1, -1));
    NetInterfaceTls unlimited(tls, limits);
    CHECK_TEST(unlimited.feed(incoming.data(), static_cast<uint32_t>(incoming.size()), used) == NET_INTERFACE_TLS_OK &&
        used == incoming.size());
    NetInterfaceTls deadline(tls);
    CHECK_TEST(deadline.progress(100) >= 0 && deadline.progress(10099) >= 0 &&
        deadline.progress(10100) == NET_INTERFACE_TLS_TIMEOUT);
    NetInterfaceLinkOptions options = {};
    options.m_version = 3;
    options.m_role = NI_LINK_CLIENT;
    options.m_tls = tls;
    NetInterfaceLinkPools pools;
    CHECK_TEST(pools.m_controlCiphertext.set(31, 11));
    NetInterfaceLink link(options, pools);
    NetInterfaceTunnelRecord record = {};
    CHECK_TEST(link.outputState() == NI_READY);
    CHECK_TEST(link.peekOutput(record) == NI_READY && record.m_kind == NI_TUNNEL_HELLO && record.m_size == 0 &&
        link.outputState() == NI_READY && link.consumeOutput() == NI_OK && link.outputState() == NI_OK);
    CHECK_TEST(link.progress(50) >= 0 && link.outputState() == NI_READY &&
        link.peekOutput(record) == NI_READY && record.m_kind == NI_TUNNEL_CONTROL && record.m_size == 11 && record.m_data);
    CHECK_TEST(link.progress(10049) >= 0 && link.progress(10050) == NI_TIMEOUT && link.outputState() == NI_TIMEOUT);
    return 0;
}

CiphertextProbeTransport::CiphertextProbeTransport(CiphertextProbeHost& host) : m_host(host), m_listener(nullptr), m_ready(false)
{

}

int32_t CiphertextProbeTransport::connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener,
    uint64_t& peer)
{
    peer = 0;
    if (m_host.m_fault == 8)
    {
        throw std::runtime_error("transport connect");
    }
    if (m_host.m_fault == 9)
    {
        throw std::bad_alloc();
    }
    if (m_host.m_refuseConnect) { return NI_PROTOCOL_ERROR; }
    if (!listener || options.m_tls.handshakeTimeoutMs != m_host.m_expectedHandshakeMs)
    {
        return NI_INVALID_ARGUMENT;
    }
    m_host.m_connectedAddress = address;
    m_host.m_connectedPort = port;
    m_listener = listener;
    peer = 1;
    return NI_OK;
}

bool CiphertextProbeTransport::poll()
{
    return m_host.m_manualCallbacks ? m_listener != nullptr : dispatchNext();
}

bool CiphertextProbeTransport::dispatchCredit(int32_t amount)
{
    ProtoMessage credit;
    std::string body;
    if (!m_listener || !credit.setBytes(NI_GATEWAY_CHANNEL, kWireChannelOne, sizeof(kWireChannelOne), Variant::UINT64) ||
        !SetWireInt32(credit, NI_GATEWAY_CREDIT_BYTES, amount) || !credit.toString(body)) { return false; }
    const NetInterfaceFrame event = {0, 0, NI_GATEWAY_CREDIT, NI_EVENT,
        static_cast<uint32_t>(body.size()), reinterpret_cast<const uint8_t*>(body.data())};
    return m_listener->onLinkControl(1, event);
}

bool CiphertextProbeTransport::dispatchNext()
{
    if (!m_listener)
    {
        return false;
    }
    if (!m_ready)
    {
        NetInterfaceLinkPeer identity = {};
        identity.m_role = NI_LINK_NODE;
        m_ready = true;
        m_listener->onLinkReady(1, identity);
    }
    else if (m_host.m_fault == 2 || m_host.m_fault == 3)
    {
        const unsigned fault = m_host.m_fault;
        m_host.m_fault = 0;
        if (fault == 2) { m_listener->onLinkClosed(1, 0); }
        else { const NetInterfaceTunnelRecord record = {}; m_listener->onLinkRecord(1, record); }
    }
    else if (m_host.m_fault == 4 || m_host.m_fault == 5)
    {
        if (m_host.m_fault == 5)
        {
            m_listener->onLinkClosed(1, NI_LIMIT_EXCEEDED);
        }
        m_listener = nullptr;
        return false;
    }
    else if (m_host.m_fault == 6)
    {
        throw std::runtime_error("transport poll");
    }
    else if (m_host.m_fault == 7)
    {
        throw std::bad_alloc();
    }
    else if (!m_response.empty())
    {
        const NetInterfaceFrame response = {0, 1, NI_GATEWAY_OPEN, NI_RESPONSE,
            static_cast<uint32_t>(m_response.size()), reinterpret_cast<const uint8_t*>(m_response.data())};
        m_listener->onLinkControl(1, response);
        m_response.clear();
    }
    else if (!m_host.m_closeEvent.empty() && m_host.m_first != 0 && !m_host.m_endAfterPrefix)
    {
        const NetInterfaceFrame event = {0, 0, NI_GATEWAY_CHANNEL_CLOSE, NI_EVENT,
            static_cast<uint32_t>(m_host.m_closeEvent.size()), reinterpret_cast<const uint8_t*>(m_host.m_closeEvent.data())};
        m_listener->onLinkControl(1, event);
        m_host.m_closeEvent.clear();
    }
    else if (m_host.m_offerPrefix && !m_host.m_prefixSent && m_host.m_first != 0)
    {
        // 记录正文声明为 512 字节；前缀先被网关接纳并归还密文额度。
        static const uint8_t widePrefix[258] = {22, 3, 3, 2, 0};
        NetInterfaceTunnelRecord record = {};
        record.m_kind = NI_TUNNEL_BYTES;
        record.m_product = 7;
        record.m_channel = 1;
        record.m_data = m_host.m_widePrefix ? widePrefix : kTlsHeaderPrefix;
        record.m_size = m_host.m_widePrefix ? sizeof(widePrefix) : sizeof(kTlsHeaderPrefix);
        m_host.m_prefixSent = m_listener->onLinkRecord(1, record);
        if (m_host.m_prefixSent && m_host.m_endAfterPrefix != 0)
        {
            if (m_host.m_endAfterPrefix == 1)
            {
                const NetInterfaceFrame event = {0, 0, NI_GATEWAY_CHANNEL_CLOSE, NI_EVENT,
                    static_cast<uint32_t>(m_host.m_closeEvent.size()), reinterpret_cast<const uint8_t*>(m_host.m_closeEvent.data())};
                m_listener->onLinkControl(1, event);
                m_host.m_closeEvent.clear();
            }
            if (m_host.m_endAfterPrefix == 2) { m_listener->onLinkClosed(1, NI_CANCELLED); }
            return false;
        }
    }
    else if (m_host.m_offerCreditTail && m_host.m_prefixSent && !m_host.m_creditTailSent)
    {
        const uint8_t length = 4;
        const NetInterfaceTunnelRecord record = {NI_TUNNEL_BYTES, 7, 1, &length, 1};
        m_host.m_creditTailSent = m_listener->onLinkRecord(1, record);
    }
    return true;
}

int32_t CiphertextProbeTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (m_host.m_offerPrefix && peer == 1 && frame.m_kind == NI_EVENT && frame.m_messageType == NI_GATEWAY_CREDIT)
    {
        ProtoMessage credit;
        if (!frame.m_payload || !credit.from(frame.m_payload, frame.m_payloadSize))
        {
            return NI_PROTOCOL_ERROR;
        }
        const uint8_t amounts[][4] = {{4, 0, 0, 0}, {1, 0, 0, 0}, {2, 1, 0, 0}};
        const uint32_t lengths[] = {4, 1, 258};
        const bool tail = m_host.m_creditTailSent && m_host.m_prefixCredit == 4;
        const size_t index = m_host.m_widePrefix ? 2 : (tail ? 1 : 0);
        if (!HasWireField(credit, NI_GATEWAY_CHANNEL, Variant::UINT64, kWireChannelOne) ||
            !HasWireField(credit, NI_GATEWAY_CREDIT_BYTES, Variant::INT32, amounts[index]))
        {
            return NI_PROTOCOL_ERROR;
        }
        ++m_host.m_creditAttempts;
        if (m_host.m_refuseCredit) { return NI_LIMIT_EXCEEDED; }
        m_host.m_prefixCredit += lengths[index];
        return NI_OK;
    }
    if (peer != 1 || frame.m_kind != NI_REQUEST || frame.m_messageType != NI_GATEWAY_OPEN || frame.m_requestId != 1)
    {
        return NI_PROTOCOL_ERROR;
    }
    ProtoMessage open;
    if (frame.m_productId != 0 || !frame.m_payload || !frame.m_payloadSize ||
        !open.from(frame.m_payload, frame.m_payloadSize)) { return NI_PROTOCOL_ERROR; }
    if (!HasWireField(open, NI_GATEWAY_CHANNEL, Variant::UINT64, kWireChannelOne)) { return NI_PROTOCOL_ERROR; }
    const uint8_t windowBytes[][4] = {{0, 0, 0x10, 0}, {0, 0, 2, 0}, {0, 0, 1, 0}};
    const uint32_t windows[] = {1048576, 131072, 65536};
    size_t index = 0;
    while (index < 3 && !HasWireField(open, NI_GATEWAY_WINDOW, Variant::INT32, windowBytes[index])) { ++index; }
    if (index == 3) { return NI_PROTOCOL_ERROR; }
    m_host.m_offered = windows[index];
    ProtoMessage answer;
    const uint8_t targetZero[8] = {};
    const uint8_t targetSeven[8] = {7};
    const uint8_t targetWide[8] = {4, 3, 2, 1, 0, 0, 0, 0};
    if (HasWireField(open, NI_GATEWAY_TARGET_PRODUCT, Variant::UINT64, targetZero)) { m_host.m_openTarget = 0; }
    else if (HasWireField(open, NI_GATEWAY_TARGET_PRODUCT, Variant::UINT64, targetSeven)) { m_host.m_openTarget = 7; }
    else if (HasWireField(open, NI_GATEWAY_TARGET_PRODUCT, Variant::UINT64, targetWide)) { m_host.m_openTarget = 0x01020304; }
    else { return NI_PROTOCOL_ERROR; }
    if (m_host.m_openTarget != 7) { answer.setCode(NI_NOT_SUPPORTED); }
    if (m_host.m_reply && !SetWireInt32(answer, NI_GATEWAY_WINDOW, static_cast<int32_t>(m_host.m_reply))) { return NI_NO_MEMORY; }
    return answer.setBytes(NI_GATEWAY_CHANNEL, kWireChannelOne, sizeof(kWireChannelOne), Variant::UINT64) &&
        answer.toString(m_response) ? NI_OK : NI_NO_MEMORY;
}

int32_t CiphertextProbeTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (peer != 1 || record.m_kind != NI_TUNNEL_BYTES || record.m_product != 7 || record.m_channel != 1 ||
        !record.m_data || record.m_size == 0) {
        return NI_PROTOCOL_ERROR;
    }
    if (m_host.m_cipherReceiver)
    {
        uint32_t used = 0;
        if (m_host.m_cipherReceiver->feed(record.m_data, record.m_size, used) < 0 || used != record.m_size) { return NI_PROTOCOL_ERROR; }
        m_host.m_cipherReceived += used;
        return NI_OK;
    }
    if (record.m_data[0] != 22) { return NI_PROTOCOL_ERROR; }
    m_host.m_first = record.m_size;
    return m_host.m_offerPrefix ? NI_OK : NI_LIMIT_EXCEEDED;
}

void CiphertextProbeTransport::shutdown()
{
    m_listener = nullptr;
}

int32_t CiphertextProbeTransport::outputState(uint64_t peer) const
{
    return peer == 1 && m_listener != nullptr ? NI_OK : NI_INVALID_STATE;
}

CiphertextProbeHost::CiphertextProbeHost() :
m_expectedHandshakeMs(10000),
m_first(0),
m_offerPrefix(false),
m_connectedPort(0),
m_widePrefix(false),
m_prefixSent(false),
m_prefixCredit(0),
m_refuseCredit(false),
m_creditAttempts(0),
m_offerCreditTail(false),
m_creditTailSent(false),
m_endAfterPrefix(0),
m_manualCallbacks(false),
m_cipherReceiver(nullptr),
m_cipherReceived(0),
m_offered(0),
m_openTarget(UINT32_MAX),
m_reply(0),
m_refuseConnect(false),
m_fault(0),
m_throwFrame(false),
m_frames(0),
m_resultId(0),
m_resultStatus(0),
m_firstResultStatus(0),
m_firstResultId(0),
m_closeOnResult(nullptr),
m_startOnReady(nullptr),
m_startResult(NI_READY),
m_resultByte(256),
m_results(0),
m_resultsAtClose(0),
m_closes(0),
m_timeouts(0),
m_closedGeneration(0),
m_closed(NI_OK)
{

}

NetGatewayTransport* CiphertextProbeHost::create()
{
    return m_fault == 1 ? nullptr : new CiphertextProbeTransport(*this);
}

void CiphertextProbeHost::onSessionReady(uint64_t generation, uint32_t, uint32_t)
{
    if (m_startOnReady != nullptr)
    {
        m_startResult = m_startOnReady->started(generation, 7);
    }
}

void CiphertextProbeHost::onSessionResult(uint64_t, uint64_t requestId, uint32_t, uint32_t, uint32_t status, const uint8_t* data, uint32_t size)
{
    if (m_results == 0)
    {
        m_firstResultId = requestId;
        m_firstResultStatus = status;
        if (m_closeOnResult != nullptr)
        {
            m_closeOnResult->close(NI_OK);
        }
    }
    ++m_results;
    m_resultId = requestId;
    m_resultStatus = status;
    m_resultByte = data && size == 1 ? data[0] : 256;
    if (status == NI_RPC_TIMEOUT)
    {
        ++m_timeouts;
        m_timeoutIds.push_back(requestId);
    }
}

bool CiphertextProbeHost::onSessionFrame(uint64_t, const NetInterfaceFrame& frame)
{
    if (frame.m_kind == NI_EVENT && frame.m_requestId == 99 && frame.m_payloadSize == 2 && frame.m_payload &&
        frame.m_payload[0] == 19 && frame.m_payload[1] == 31) { ++m_frames; }
    if (m_throwFrame) { throw std::runtime_error("frame"); }
    return false;
}

void CiphertextProbeHost::onSessionClosed(uint64_t generation, int32_t reason, int32_t)
{
    m_closedGeneration = generation;
    m_resultsAtClose = m_results;
    ++m_closes;
    m_closed = reason;
}

/** 验证暂存在业务通道内的 TLS 密文不需要新的运输事件即可继续推进
@return 等待预算和信用确认正确时为零
*/
static int StagedCiphertextProgress();

/** 用真实 HELLO 与应答核对客户端显式声明的压缩、紧凑帧能力以及对端缩小声明时的回退
@return 服务端协商值和客户端运输设置均为双方能力交集时为0
*/
static int CheckSessionCapabilities()
{
    const uint32_t offers[][2] = {{1, 31}, {7, 31}, {9, 31}, {17, 31}, {25, 31}, {31, 9}, {31, 1}};
    for (const auto& offer : offers)
    {
        NetInterfaceHandshakeOptions hello = {};
        hello.m_server = hello.m_maxRequests = 1;
        hello.m_capabilities = offer[1];
        hello.m_maxChunk = 1024;
        hello.m_timeoutMs = 5000;
        hello.m_nonce[0] = 19;
        NetInterfaceHandshake server(hello, 0);
        SessionProbeChannel channel(server);
        CiphertextProbeHost events;
        NetClientSession client;
        uint32_t seed = 27;
        NetInterfaceTlsOptions tls = {};
        tls.entropy = HelloEntropy;
        tls.entropyContext = &seed;
        hello.m_server = 0;
        hello.m_platform = hello.m_device[0] = 1;
        hello.m_capabilities = offer[0];
        uint32_t types[] = {17, 0x12345678};
        CHECK_TEST(client.configureCompression(types, 2) == NI_OK);
        types[1] = 0;
        CHECK_TEST(client.configureCompression(types, 2) == NI_INVALID_ARGUMENT);
        CHECK_TEST(client.bindChannel(&channel) == NI_OK && client.connect("localhost", 9001, tls, hello, &events) == NI_OK &&
            client.poll() && client.ready());
        uint32_t agreed[4] = {};
        uint8_t nonce[16], device[16];
        const uint32_t expected = offer[0] & offer[1];
        CHECK_TEST(server.peerInfo(agreed, nonce, device) == NI_READY && agreed[1] == expected &&
            channel.m_codecCapabilities == (expected & 24));
        if (expected & 8)
        {
            CHECK_TEST(channel.m_codecTypes.size() == 2 && channel.m_codecTypes[0] == 17 &&
                channel.m_codecTypes[1] == 0x12345678);
        }
        else
        {
            CHECK_TEST(channel.m_codecTypes.empty());
        }
        client.shutdown();
    }
    return 0;
}

/** 单独创建两个客户端会话，在运输边界核对各自的 TLS 输出预算
@return 成功为0
*/
static int ServiceCiphertextConfiguration()
{
    CHECK_TEST(CheckSessionCapabilities() == 0);
    uint32_t entropy = 11;
    NetInterfaceTlsOptions tls = {};
    tls.entropy = HelloEntropy;
    tls.entropyContext = &entropy;
    NetInterfaceHandshakeOptions hello = {};
    hello.m_platform = 1;
    hello.m_capabilities = 1;
    hello.m_maxRequests = 8;
    hello.m_maxChunk = 1024;
    hello.m_timeoutMs = 1000;
    hello.m_device[0] = 1;
    const uint32_t outputs[] = {13, 29, 17};
    {
        CiphertextProbeHost host;
        host.m_offerPrefix = host.m_widePrefix = true;
        CiphertextProbeTransport transport(host);
        FrameDeliveryProbe listener;
        NetGatewayClientChannel gateway(7, &transport);
        uint64_t peer = 0;
        CHECK_TEST(gateway.connect("127.0.0.1", 8000, tls, &listener, peer) == NI_OK && peer == 1 &&
            gateway.poll() && gateway.poll() && host.m_first != 0 && gateway.poll() && host.m_prefixSent &&
            host.m_creditAttempts == 1 && host.m_prefixCredit == 258 && gateway.outputState(peer) == NI_OK);
        const uint32_t allowed[] = {7, 11, 7};
        CHECK_TEST(gateway.configureCompact(peer + 1, true) == NI_INVALID_STATE &&
            gateway.configureCompact(peer, true) == NI_OK && gateway.configureCompact(peer, false) == NI_OK);
        CHECK_TEST(gateway.configureCompression(peer + 1, allowed, 3, true) == NI_INVALID_STATE &&
            gateway.configureCompression(peer, nullptr, 1, true) == NI_INVALID_ARGUMENT &&
            gateway.configureCompression(peer, allowed, 3, true) == NI_OK &&
            gateway.configureCompression(peer, nullptr, 0, false) == NI_OK);
    }
    // 普通连接在OPEN前仍须区分业务产品参数和协议就绪状态，目标0的OPEN由节点拒绝。
    const uint32_t targets[] = {0, 7, 0x01020304};
    const uint32_t products[] = {7, 9, 0};
    for (const uint32_t target : targets)
    {
        CiphertextProbeHost host;
        NetClientSession session;
        CHECK_TEST(session.connectService(target, "127.0.0.1", 8000, tls, hello, &host, &host) == NI_OK &&
            session.serviceMode() && !session.ready());
        for (const uint32_t product : products)
        {
            uint64_t id = 99;
            const NetInterfaceFrame data = {product, 1, 3, NI_DATA, 0, nullptr};
            const int32_t begun = session.beginRequest(product, 9, 1000, id);
            const int32_t imported = session.beginRequestAt(1, product, 9, 0, 1000);
            const int32_t sent = session.sendData(data);
            const int32_t expected = product && (!target || product == target) ? NI_INVALID_STATE : NI_INVALID_ARGUMENT;
            if (begun != expected || imported != expected || sent != expected)
            {
                std::fprintf(stderr, "target %u product %u begin %d beginAt %d sendData %d expected %d\n",
                    target, product, begun, imported, sent, expected);
                return 1;
            }
            CHECK_TEST(id == 0 && session.cancel(1) == NI_NOT_FOUND && session.outputState() == NI_INVALID_STATE && !host.m_results);
        }
        for (unsigned step = 0; step < 16 && !host.m_first && !host.m_closes; ++step) { session.poll(); }
        CHECK_TEST(host.m_openTarget == target && host.m_offered != 0 && !session.ready() && !host.m_results);
        if (target != 7) { CHECK_TEST(host.m_closes == 1 && host.m_closed == NI_NOT_SUPPORTED && !host.m_first); }
        else { CHECK_TEST(!host.m_closes && host.m_first != 0); }
        session.shutdown();
    }
    const uint32_t windows[] = {1048576, 131072, 65536};
    const uint32_t replies[] = {0, 131072, 81920};
    CiphertextProbeHost hosts[3];
    NetClientSession sessions[3];
    for (size_t i = 0; i < 3; ++i)
    {
        CiphertextProbeHost& host = hosts[i];
        NetClientSession& session = sessions[i];
        host.m_reply = replies[i];
        if (i != 0)
        {
            CHECK_TEST(session.configureServiceWindow(windows[i]) == NI_OK);
        }
        CHECK_TEST(session.configureServicePollBudgets(1, 2) == NI_OK &&
            session.configureServicePollBudgets(0, 2) == NI_INVALID_ARGUMENT &&
            session.configureServicePollBudgets(1, 0) == NI_INVALID_ARGUMENT);
        CHECK_TEST(session.configureServiceWindow(65537) == NI_INVALID_ARGUMENT);
        CEncodeDecodeTlsQueueLimits limits;
        CHECK_TEST(limits.set(-1, outputs[i]) && session.configureServiceTlsQueueLimits(limits) == NI_OK);
        CHECK_TEST(limits.set(-1, 101));
        CHECK_TEST(session.connectService(7, "127.0.0.1", 8000, tls, hello, &host, &host) == NI_OK);
        CHECK_TEST(session.configureServiceTlsQueueLimits(limits) == NI_INVALID_STATE);
        CHECK_TEST(session.configureServiceWindow(65536) == NI_INVALID_STATE &&
            session.configureServicePollBudgets(2, 3) == NI_INVALID_STATE);
    }
    for (size_t i = 0; i < 3; ++i)
    {
        CiphertextProbeHost& host = hosts[i];
        NetClientSession& session = sessions[i];
        for (unsigned step = 0; step != 16 && !host.m_first && !host.m_closed; ++step)
        {
            session.poll();
        }
        CHECK_TEST(host.m_offered == windows[i]);
        if (replies[i] > windows[i])
        {
            CHECK_TEST(host.m_closed == NI_PROTOCOL_ERROR && host.m_first == 0);
        }
        else
        {
            CHECK_TEST(host.m_first == outputs[i] && host.m_closed == NI_OK);
        }
        session.shutdown();
    }
    for (unsigned fault = 8; fault <= 9; ++fault)
    {
        CiphertextProbeHost host;
        host.m_fault = fault;
        CiphertextProbeTransport transport(host);
        FrameDeliveryProbe listener;
        NetGatewayClientChannel gateway(7, &transport);
        uint64_t peer = 99;
        CEncodeDecodeTlsQueueLimits limits;
        CHECK_TEST(limits.set(29, 31) && gateway.configureTlsQueueLimits(limits) == NI_OK &&
            gateway.connect("", 8000, tls, &listener, peer) == NI_INVALID_ARGUMENT && peer == 0);
        CHECK_TEST(gateway.connect("127.0.0.1", 8000, tls, &listener, peer) ==
            (fault == 8 ? NI_INVALID_STATE : NI_NO_MEMORY) && peer == 0 && !gateway.poll());
    }
    const int32_t expectedReasons[] = {NI_NO_MEMORY, NI_TRUNCATED, NI_PROTOCOL_ERROR, NI_TRUNCATED,
        NI_LIMIT_EXCEEDED, NI_INVALID_STATE, NI_NO_MEMORY};
    for (unsigned fault = 1; fault <= sizeof(expectedReasons) / sizeof(expectedReasons[0]); ++fault)
    {
        CiphertextProbeHost host;
        host.m_fault = fault;
        NetClientSession session;
        CHECK_TEST(session.connectService(7, "127.0.0.1", 8000, tls, hello, &host, &host) == NI_OK);
        for (unsigned i = 0; i < 16 && !host.m_closes; ++i) { session.poll(); }
        CHECK_TEST(host.m_closes == 1 && host.m_closed == expectedReasons[fault - 1]);
        if (expectedReasons[fault - 1] == NI_NO_MEMORY)
        {
            CHECK_TEST(session.quiescent() &&
                session.connect("127.0.0.1", 8000, tls, hello, &host) == NI_NOT_SUPPORTED &&
                session.generation() == 1 && host.m_closes == 1);
        }
        session.shutdown();
        CHECK_TEST(!session.poll() && host.m_closes == 1);
    }
    const int32_t closeReasons[] = {NI_NO_MEMORY, NI_TRUNCATED, NI_PROTOCOL_ERROR, NI_PROTOCOL_ERROR, NI_PROTOCOL_ERROR, -0x01020304};
    for (unsigned index = 0; index < sizeof(closeReasons) / sizeof(closeReasons[0]); ++index)
    {
        CiphertextProbeHost host;
        ProtoMessage event;
        CHECK_TEST(event.setBytes(NI_GATEWAY_CHANNEL, kWireChannelOne, sizeof(kWireChannelOne), Variant::UINT64));
        const uint8_t reasons[][4] = {{0xfb, 0xff, 0xff, 0xff}, {}, {}, {}, {2, 0, 0, 0}, {0xfc, 0xfc, 0xfd, 0xfe}};
        if (index == 3) { CHECK_TEST(event.setBytes(NI_GATEWAY_REASON, kWireChannelOne, sizeof(kWireChannelOne), Variant::UINT64)); }
        else if (index != 2) { CHECK_TEST(event.setBytes(NI_GATEWAY_REASON, reasons[index], sizeof(reasons[index]), Variant::INT32)); }
        CHECK_TEST(event.toString(host.m_closeEvent));
        NetClientSession session;
        CHECK_TEST(session.connectService(7, "127.0.0.1", 8000, tls, hello, &host, &host) == NI_OK);
        for (unsigned step = 0; step < 16 && !host.m_closes; ++step) { session.poll(); }
        CHECK_TEST(host.m_first != 0 && host.m_closeEvent.empty() && host.m_closes == 1 && host.m_closed == closeReasons[index]);
        session.shutdown();
        CHECK_TEST(!session.poll() && host.m_closes == 1);
    }
    return StagedCiphertextProgress();
}

/** 独立实例在消费与复位时按各自预算保留或回收正文，重新配置需处于空闲边界
@return 全部容量和状态符合预期时为0
*/
static int DecoderRetention()
{
    std::vector<uint8_t> payload(5001, 0x5a), wire(32 + payload.size());
    const NetInterfaceFrame frame = {7, 1, 9, NI_REQUEST, static_cast<uint32_t>(payload.size()), payload.data()};
    uint32_t size = 0, used = 0;
    CHECK_TEST(NetInterface::encode(frame, wire.data(), static_cast<uint32_t>(wire.size()), size) == NI_OK);
    NetInterface ordinary, cached(6000);
    CHECK_TEST(cached.configureDecoderRetention(-1) == NI_OK && cached.decoderBufferCapacity() == 0);
    CHECK_TEST(ordinary.feed(wire.data(), size, used) == NI_READY && used == size &&
        ordinary.decoderBufferCapacity() == payload.size() && ordinary.consume() == NI_OK && ordinary.decoderBufferCapacity() == 0);
    CHECK_TEST(cached.feed(wire.data(), 1, used) == NI_NEED_MORE && used == 1 &&
        cached.configureDecoderRetention(0) == NI_INVALID_STATE);
    CHECK_TEST(cached.feed(wire.data() + 1, size - 1, used) == NI_READY && used == size - 1);
    const uint32_t retained = cached.decoderBufferCapacity();
    CHECK_TEST(retained >= payload.size() && retained <= cached.payloadLimit() &&
        cached.configureDecoderRetention(0) == NI_INVALID_STATE && cached.consume() == NI_OK &&
        cached.decoderBufferCapacity() == retained);
    NetInterfaceFrame current = {};
    CHECK_TEST(cached.feed(kFrame, sizeof(kFrame), used) == NI_READY && cached.peek(current) == NI_READY &&
        current.m_payloadSize == 3 && current.m_payload && current.m_payload[1] == 255 &&
        std::all_of(current.m_payload + 3, current.m_payload + payload.size(), [](uint8_t value) { return value == 0; }) &&
        cached.consume() == NI_OK);
    CHECK_TEST(cached.configureDecoderRetention(-2) == NI_INVALID_ARGUMENT && cached.decoderBufferCapacity() == retained);
    CHECK_TEST(cached.configureDecoderRetention(3) == NI_OK && cached.decoderBufferCapacity() == 0 &&
        cached.feed(kFrame, sizeof(kFrame), used) == NI_READY && cached.consume() == NI_OK && cached.decoderBufferCapacity() == 3);
    cached.reset();
    CHECK_TEST(cached.decoderBufferCapacity() == 0 && cached.feed(kFrame, sizeof(kFrame), used) == NI_READY &&
        cached.consume() == NI_OK && cached.decoderBufferCapacity() == 3);
    CHECK_TEST(cached.configureDecoderRetention(0) == NI_OK && cached.feed(kFrame, sizeof(kFrame), used) == NI_READY &&
        cached.decoderBufferCapacity() == 3 && cached.consume() == NI_OK && cached.decoderBufferCapacity() == 0);
    NetInterfaceHandle handle = nullptr;
    CHECK_TEST(NetInterface::create(3, &handle) == NI_OK && handle);
    std::unique_ptr<NetInterfaceOpaque, void (*)(NetInterfaceHandle)> owner(handle, NetInterface::destroy);
    CHECK_TEST(NetInterface::configureDecoderRetention(handle, 0) == NI_OK &&
        NetInterface::feed(handle, kFrame, sizeof(kFrame), &used) == NI_READY &&
        NetInterface::configureDecoderRetention(handle, -1) == NI_INVALID_STATE &&
        NetInterface::consume(handle) == NI_OK && NetInterface::configureDecoderRetention(handle, -1) == NI_OK &&
        NetInterface::configureDecoderRetention(nullptr, 0) == NI_INVALID_ARGUMENT);
    return 0;
}

/** 核对累计成本和错误文本预算的独立设置、精确边界及解析时的记账
@return 所有操作遵守句柄预算和线路长度时为0
*/
static int MessageLimits()
{
    NetInterfaceMessageHandle raw = nullptr;
    CHECK_TEST(NetInterface::messageCreate(&raw) == NI_OK && raw);
    std::unique_ptr<NetInterfaceMessageOpaque, void (*)(NetInterfaceMessageHandle)> owner(raw, NetInterface::messageDestroy);
    const uint8_t text[] = {42, 0, 99};
    CHECK_TEST(NetInterface::messageConfigureLimits(raw, 34, 2) == NI_OK &&
        NetInterface::messageConfigureLimits(raw, -2, 0) == NI_INVALID_ARGUMENT &&
        NetInterface::messageConfigureLimits(raw, 0, -2) == NI_INVALID_ARGUMENT &&
        NetInterface::messageSetCode(raw, 7, text, 3) == NI_LIMIT_EXCEEDED &&
        NetInterface::messageSetCode(raw, 7, text, 2) == NI_OK &&
        NetInterface::messageSetCode(raw, 0, nullptr, 0) == NI_LIMIT_EXCEEDED &&
        NetInterface::messageConfigureLimits(raw, -1, -1) == NI_INVALID_STATE);
    uint8_t wire[128];
    uint32_t size = 0;
    CHECK_TEST(NetInterface::messageEncode(raw, wire, sizeof(wire), &size) == NI_OK && size > 1);
    CHECK_TEST(NetInterface::messageClear(raw) == NI_OK && NetInterface::messageSetCode(raw, 0, text, 3) == NI_LIMIT_EXCEEDED);
    CHECK_TEST(NetInterface::messageConfigureLimits(raw, size - 1, 0) == NI_OK &&
        NetInterface::messageParse(raw, wire, size) == NI_LIMIT_EXCEEDED &&
        NetInterface::messageConfigureLimits(raw, size, 0) == NI_OK && NetInterface::messageParse(raw, wire, size) == NI_OK &&
        NetInterface::messageSetValue(raw, 0, 1, 4, nullptr, 0) == NI_LIMIT_EXCEEDED);
    CHECK_TEST(NetInterface::messageClear(raw) == NI_OK && NetInterface::messageConfigureLimits(raw, 0, -1) == NI_OK &&
        NetInterface::messageSetCode(raw, 0, nullptr, 0) == NI_LIMIT_EXCEEDED);
    std::vector<uint8_t> large(8192, 0x5a);
    CHECK_TEST(NetInterface::messageConfigureLimits(raw, -1, -1) == NI_OK);
    for (unsigned i = 0; i < 140; ++i)
    {
        CHECK_TEST(NetInterface::messageSetCode(raw, 9, large.data(), static_cast<uint32_t>(large.size())) == NI_OK);
    }
    int32_t code = 0;
    CHECK_TEST(NetInterface::messageGetCode(raw, &code, nullptr, 0, &size) == NI_BUFFER_TOO_SMALL &&
        code == 9 && size == large.size());
    return 0;
}

/** 登记真实执行宿主，验证函数表生效、无进展等待、唤醒与退出时归还
@return 所有回调均属于本实例且顺序正确时为0
*/
static int ExecutionConfiguration()
{
    // 函数表没有宿主上下文字段，用例串行运行，创建者返回的实例可在所有回调中比对。
    static uint64_t generations[2];
    static uint32_t creates;
    static uint32_t waited;
    static uint32_t releases;
    static uint64_t observedAtWait;
    static int externalLoop;
    static void* active;
    generations[0] = 12;
    generations[1] = 28;
    creates = releases = 0;
    waited = UINT32_MAX;
    observedAtWait = 0;
    active = &externalLoop;
    NetHostExecutionFunctions f = {};
    f.create = [](void** instance) -> int32_t {
        if (creates == 2) { return -1; }
        *instance = &generations[creates++];
        return 0;
    };
    f.exchange = [](void* instance) -> void* { void* previous = active; active = instance; return previous; };
    f.begin = [](void* instance) -> uint64_t { return *static_cast<uint64_t*>(instance); };
    f.wait = [](void* instance, uint64_t observed, uint32_t timeout) -> int32_t {
        waited = timeout;
        observedAtWait = observed;
        return active == instance ? 0 : -1;
    };
    f.wake = [](void* instance) { ++*static_cast<uint64_t*>(instance); };
    f.destroy = [](void* instance) -> int32_t {
        if (instance == &generations[0] || instance == &generations[1]) { ++releases; }
        return 0;
    };
    CHECK_TEST(NetExecution::configure(f) == NI_OK);
    NetHostExecutionFunctions incomplete = f;
    incomplete.begin = nullptr;
    bool valid = NetExecution::configure(incomplete) == NI_INVALID_ARGUMENT;
    {
        NetExecution outer;
        NetExecution inner;
        valid = valid && outer.status() == NI_OK && inner.status() == NI_OK && !NetExecution::current();
        {
            NetExecutionScope scope(outer);
            outer.begin();
            valid = valid && NetExecution::current() == &outer && active == &generations[0] &&
                outer.wait(37) == NI_OK && waited == 37 && observedAtWait == 12;
            outer.wake();
            {
                NetExecutionScope nested(inner);
                inner.begin();
                valid = valid && NetExecution::current() == &inner && active == &generations[1] &&
                    inner.wait(19) == NI_OK && observedAtWait == 28 && waited == 19;
            }
            NetExecution::deadline(6, 7);
            valid = valid && NetExecution::current() == &outer && generations[0] == 13 &&
                outer.wait(37) == NI_OK && observedAtWait == 12 && waited == 0;
            outer.begin();
            NetExecution::progress();
            valid = valid && outer.wait(37) == NI_OK && observedAtWait == 13 && waited == 0;
        }
        valid = valid && active == &externalLoop && !NetExecution::current();
        void* previousHost = nullptr;
        NetExecution* previous = NetExecution::exchange(&outer, previousHost);
        void* displacedHost = nullptr;
        const bool restored = NetExecution::bind(previous, previousHost, displacedHost);
        valid = valid && previous == nullptr && previousHost == &externalLoop &&
            restored && displacedHost == &generations[0] &&
            active == &externalLoop && NetExecution::current() == nullptr;
    }
    NetHostExecutionFunctions empty = {};
    CHECK_TEST(NetExecution::configure(empty) == NI_OK && valid && releases == 2);
    NetExecution passive;
    CHECK_TEST(passive.status() == NI_OK && passive.wait(10) == NI_NEED_MORE);
    return 0;
}

/** 验证执行宿主抛异常时的错误、绑定归属、回调次数和析构
@return 宿主故障均由执行者收敛时返回0
*/
static int ExecutionExceptions()
{
    static unsigned fault, destroys, binds, waits;
    static int loop, external;
    static void* active;
    NetHostExecutionFunctions host = {};
    host.create = [](void** instance) -> int32_t {
        *instance = &loop;
        if (fault == 1) { throw std::runtime_error("create"); }
        return fault == 8 ? NI_INVALID_STATE : 0;
    };
    host.exchange = [](void* to) -> void* {
        ++binds;
        if ((fault == 2 && binds == 1) || (fault == 6 && binds == 2)) { throw std::runtime_error("exchange"); }
        void* old = active;
        active = to;
        return old;
    };
    host.begin = [](void*) -> uint64_t {
        if (fault == 3) { throw std::runtime_error("begin"); }
        return 7;
    };
    host.wait = [](void*, uint64_t, uint32_t) -> int32_t {
        ++waits;
        if (fault == 4) { throw std::runtime_error("wait"); }
        return 0;
    };
    host.wake = [](void*) { if (fault == 5) { throw std::runtime_error("wake"); } };
    host.destroy = [](void*) -> int32_t {
        ++destroys;
        if (fault == 7) { throw std::runtime_error("destroy"); }
        return 0;
    };
    CHECK_TEST(NetExecution::configure(host) == NI_OK);
    bool valid = true;
    for (fault = 1; fault <= 8; ++fault)
    {
        destroys = binds = waits = 0;
        active = &external;
        {
            NetExecution execution;
            if (fault == 1 || fault == 8)
            {
                void* previousHost = &external;
                NetExecutionScope rejected(execution);
                valid = valid && !execution.bind(previousHost) && !previousHost &&
                    !NetExecution::exchange(&execution, previousHost) && !previousHost &&
                    !NetExecution::current() && binds == 0 && active == &external && execution.wait(0) == NI_INVALID_STATE;
            }
            else
            {
                {
                    NetExecutionScope scope(execution);
                    valid = valid && NetExecution::current() == (fault == 2 ? nullptr : &execution);
                    execution.begin();
                    execution.wake();
                    const int32_t status = execution.wait(0);
                    valid = valid && status == (fault >= 2 && fault <= 5 ? NI_INVALID_STATE : NI_OK);
                }
                valid = valid && !NetExecution::current() && binds == (fault == 2 ? 1U : 2U);
                if (fault != 6) { valid = valid && active == &external; }
            }
            valid = valid && execution.status() == (fault == 7 ? NI_OK : NI_INVALID_STATE);
            valid = valid && waits == (fault == 4 || fault == 6 || fault == 7 ? 1U : 0U);
        }
        valid = valid && destroys == 1;
    }
    NetHostExecutionFunctions empty = {};
    CHECK_TEST(NetExecution::configure(empty) == NI_OK && valid);
    NetExecution local;
    CHECK_TEST(NetExecution::exchange(&local) == nullptr && NetExecution::current() == &local);
    CHECK_TEST(NetExecution::exchange(nullptr) == &local && !NetExecution::current());
    return 0;
}

RouterHandshakeTransport::RouterHandshakeTransport(NetInterfaceHandshake& client) :
m_client(client),
m_listener(nullptr),
m_events(0),
m_router(nullptr),
m_reentryRejected(true),
m_failListen(false),
m_stopDuringRollback(false),
m_pollCalls(0),
m_pingFailure(0),
m_pingId(0),
m_pongDue(false),
m_deferHello(false),
m_deferReply(false),
m_listenCalls(0),
m_listenPeers(0),
m_stops(0),
m_activityInput(false),
m_endInput(false),
m_result(NI_NEED_MORE)
{

}

int32_t RouterHandshakeTransport::connect(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener*, uint64_t& peer)
{
    peer = 0;
    return NI_NOT_SUPPORTED;
}

int32_t RouterHandshakeTransport::listen(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener* listener, uint32_t peers)
{
    m_listenPeers = peers;
    ++m_listenCalls;
    m_listener = listener;
    if (m_router != nullptr)
    {
        m_reentryRejected = m_reentryRejected && m_router->bindTransport(this) == NI_INVALID_STATE &&
            m_router->configureFileLeaseLimit(3) == NI_INVALID_STATE;
    }
    if (m_failListen) { throw std::bad_alloc(); }
    return NI_OK;
}

bool RouterHandshakeTransport::poll()
{
    ++m_pollCalls;
    if (m_listener == nullptr) { return false; }
    if (m_pongDue)
    {
        m_pongDue = false;
        const NetInterfaceFrame pong = {0, m_pingId, 0, NI_PONG, 0, nullptr};
        m_listener->onFrame(9, pong);
        return true;
    }
    if (m_activityInput)
    {
        const NetInterfaceFrame ping = {0, 97, 0, NI_PING, 0, nullptr};
        if (!m_listener->onFrame(9, ping)) { m_result = NI_PROTOCOL_ERROR; }
        return true;
    }
    if (m_endInput) { m_endInput = false; m_listener->onClosed(9, NI_CANCELLED, 0); return true; }
    if (m_events == 0)
    {
        ++m_events;
        m_listener->onReady(9);
    }
    else if (m_events == 1 && !m_deferHello)
    {
        NetInterfaceFrame hello = {};
        if (m_client.peekOutput(hello) != NI_READY || !m_listener->onFrame(9, hello) || m_client.consumeOutput() != NI_NEED_MORE)
        {
            m_result = NI_PROTOCOL_ERROR;
            return false;
        }
        ++m_events;
    }
    return true;
}

int32_t RouterHandshakeTransport::send(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer == 9 && frame.m_kind == NI_PONG && m_activityInput && frame.m_requestId == 97)
    {
        m_activityInput = false;
        return NI_OK;
    }
    if (peer == 9 && frame.m_kind == NI_PING)
    {
        m_pingId = frame.m_requestId;
        if (m_pingFailure == 2) { throw std::runtime_error("send"); }
        if (m_pingFailure == 3) { throw std::bad_alloc(); }
        return m_pingFailure == 1 ? NI_TRUNCATED : NI_OK;
    }
    if (peer != 9 || frame.m_kind != NI_HELLO_REPLY) { return NI_PROTOCOL_ERROR; }
    if (m_deferReply) { return NI_LIMIT_EXCEEDED; }
    m_result = m_client.accept(frame, 1);
    return m_result == NI_READY ? NI_OK : NI_PROTOCOL_ERROR;
}

void RouterHandshakeTransport::close(uint64_t, int32_t reason)
{
    m_result = reason;
}

void RouterHandshakeTransport::shutdown()
{
    ++m_stops;
    if (m_stopDuringRollback && m_router != nullptr)
    {
        m_stopDuringRollback = false;
        m_router->shutdown();
    }
    if (m_failListen && m_router != nullptr)
    {
        m_reentryRejected = m_reentryRejected && m_router->bindTransport(this) == NI_INVALID_STATE &&
            m_router->configureFileLeaseLimit(4) == NI_INVALID_STATE;
    }
    m_listener = nullptr;
}

uint32_t RouterHandshakeTransport::localPort() const
{
    return m_listener == nullptr ? 0 : 8081;
}

uint32_t RouterHandshakeTransport::peerCount() const
{
    return m_events == 0 || m_listener == nullptr ? 0 : 1;
}

/** 推进用例和协议共用的单调时间
@param [in] milliseconds 要经过的毫秒数
*/
static void YieldTimerProbe(uint32_t milliseconds);

/** 核对直接监听时协商的三项值及真正交给宿主的握手超时
@param [in] custom 是否使用独立非默认配置
@param [in] waited 最近一次事件等待回调收到的预算
@return 全部符合预期时为0
*/
static int CheckRouterHandshake(bool custom, const uint32_t& waited)
{
    NetInterfaceHandshakeOptions offer = {};
    offer.m_platform = 2;
    offer.m_capabilities = 7;
    offer.m_maxRequests = 1000;
    offer.m_maxChunk = 65536;
    offer.m_timeoutMs = 60000;
    offer.m_nonce[0] = 1;
    offer.m_device[0] = 2;
    NetInterfaceHandshake client(offer, 0);
    RouterHandshakeTransport transport(client);
    NetEndpointRouter router(nullptr);
    uint32_t seed = 11;
    NetInterfaceTlsOptions tls = {};
    tls.entropy = HelloEntropy;
    tls.entropyContext = &seed;
    NetEndpointPools pools;
    CHECK_TEST(router.bindTransport(&transport) == NI_OK && router.poll() && transport.m_pollCalls == 0);
    if (custom)
    {
        pools.m_heartbeat.m_pingIntervalMs = 0;
        CHECK_TEST(router.configurePools(pools) == NI_INVALID_ARGUMENT);
        pools.m_heartbeat.m_pingIntervalMs = 12345;
        CHECK_TEST(router.configureClientHandshake(3, 9, 2048, 24000) == NI_OK &&
            router.configurePools(pools) == NI_OK &&
            router.configureClientHandshake(0, 4, 4096, 5000) == NI_INVALID_ARGUMENT &&
            router.configureClientHandshake(9, 4, 4096, 5000) == NI_INVALID_ARGUMENT &&
            router.configureClientHandshake(7, 0, 4096, 5000) == NI_INVALID_ARGUMENT &&
            router.configureClientHandshake(7, 4, 1023, 5000) == NI_INVALID_ARGUMENT &&
            router.configureClientHandshake(7, 4, 65537, 5000) == NI_INVALID_ARGUMENT &&
            router.configureClientHandshake(7, 4, 4096, 99) == NI_INVALID_ARGUMENT &&
            router.configureClientHandshake(7, 4, 4096, 60001) == NI_INVALID_ARGUMENT);
    }
    if (custom)
    {
        RouterHandshakeTransport cancelled(client);
        NetEndpointRouter stopping(nullptr);
        cancelled.m_router = &stopping;
        cancelled.m_failListen = true;
        cancelled.m_stopDuringRollback = true;
        NetInterfaceTlsOptions serverTls = tls;
        serverTls.server = 1;
        NetExecution owner;
        NetExecutionScope scope(owner);
        owner.begin();
        CHECK_TEST(stopping.bindTransport(&cancelled) == NI_OK &&
            stopping.listen("127.0.0.1", 8081, serverTls, 1) == NI_NO_MEMORY && !cancelled.m_stopDuringRollback);
        CHECK_TEST(owner.wait(60000) == NI_OK && waited == 0);
        owner.begin();
        CHECK_TEST(stopping.configurePools(pools) == NI_INVALID_STATE && stopping.configureFileLeaseLimit(3) == NI_INVALID_STATE &&
            stopping.configureClientHandshake(1, 1, 1024, 100) == NI_INVALID_STATE &&
            stopping.bindTransport(nullptr) == NI_INVALID_STATE && !stopping.poll() && stopping.bindTransport(nullptr) == NI_OK);
        CHECK_TEST(owner.wait(60000) == NI_OK && waited == 60000);
    }
    transport.m_router = &router;
    CHECK_TEST(router.listen("127.0.0.1", 8081, tls, 1) == NI_INVALID_ARGUMENT);
    tls.server = 1;
    CHECK_TEST(router.listen("", 8081, tls, 1) == NI_INVALID_ARGUMENT);
    tls.handshakeTimeoutMs = 99;
    CHECK_TEST(router.listen("127.0.0.1", 8081, tls, 1) == NI_INVALID_ARGUMENT);
    tls.handshakeTimeoutMs = 60001;
    CHECK_TEST(router.listen("127.0.0.1", 8081, tls, 1) == NI_INVALID_ARGUMENT && transport.m_listenCalls == 0);
    tls.handshakeTimeoutMs = custom ? 100 : 0;
    transport.m_failListen = true;
    CHECK_TEST(router.listen("127.0.0.1", 8081, tls, 1) == NI_NO_MEMORY && transport.m_stops == 1 &&
        transport.m_reentryRejected && router.localPort() == 0 && router.poll() && transport.m_pollCalls == 0);
    transport.m_failListen = false;
    const int32_t listening = custom ? router.listen("127.0.0.1", 8081, tls, 1) : router.listen("127.0.0.1", 8081, tls);
    CHECK_TEST(listening == NI_OK && transport.m_listenCalls == 2 && transport.m_listenPeers == (custom ? 1U : 64U) &&
        transport.m_reentryRejected && router.localPort() == 8081 &&
        router.configureClientHandshake(1, 1, 1024, 100) == NI_INVALID_STATE);
    NetExecution owner;
    NetExecutionScope scope(owner);
    owner.begin();
    const auto start = TimerProbeNow();
    CHECK_TEST(router.poll() && owner.wait(60000) == NI_OK);
    const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        TimerProbeNow() - start).count());
    const uint32_t timeout = custom ? 24000 : 5000;
    CHECK_TEST(waited <= timeout && waited + elapsed + 2 >= timeout);
    owner.begin();
    CHECK_TEST(router.poll() && transport.m_pollCalls == 2 && transport.m_result == NI_READY && owner.wait(60000) == NI_OK);
    const uint32_t ping = custom ? 12345 : 30000;
    CHECK_TEST(!custom || waited <= ping);
    owner.begin();
    CHECK_TEST(router.poll() && owner.wait(60000) == NI_OK);
    const uint64_t sinceReady = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        TimerProbeNow() - start).count());
    CHECK_TEST(waited <= ping && waited + sinceReady + 2 >= ping);
    if (custom)
    {
        YieldTimerProbe(50);
        transport.m_activityInput = true;
        owner.begin();
        const auto activity = TimerProbeNow();
        CHECK_TEST(router.poll() && !transport.m_activityInput && transport.m_result == NI_READY && owner.wait(60000) == NI_OK);
        const uint64_t processing = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            TimerProbeNow() - activity).count());
        CHECK_TEST(waited <= ping && waited + processing + 2 >= ping);
    }
    uint32_t fields[4] = {};
    uint8_t nonce[16], device[16], sentinel[16];
    std::memset(sentinel, 0xa5, sizeof(sentinel));
    std::memcpy(nonce, sentinel, sizeof(nonce));
    std::memcpy(device, sentinel, sizeof(device));
    fields[0] = 17;
    CHECK_TEST(client.peerInfo(fields, nonce, nonce) == NI_INVALID_ARGUMENT);
    CHECK_TEST(client.peerInfo(fields, reinterpret_cast<uint8_t*>(fields), device) == NI_INVALID_ARGUMENT);
    CHECK_TEST(client.peerInfo(fields, nonce, reinterpret_cast<uint8_t*>(fields)) == NI_INVALID_ARGUMENT);
    CHECK_TEST(client.peerInfo(fields, nonce, nullptr) == NI_INVALID_ARGUMENT);
    CHECK_TEST(fields[0] == 17 && !std::memcmp(nonce, sentinel, sizeof(nonce)) && !std::memcmp(device, sentinel, sizeof(device)));
    CHECK_TEST(client.peerInfo(fields, nonce, device) == NI_READY && fields[0] == 2 &&
        fields[1] == (custom ? 3U : 7U) && fields[2] == (custom ? 9U : 32U) &&
        fields[3] == (custom ? 2048U : 65536U) && !std::memcmp(device, offer.m_device, sizeof(device)));
    transport.m_endInput = true;
    CHECK_TEST(router.poll() && !transport.m_endInput && !router.pendingCount());
    return 0;
}

/** 登记只记录预算的等待宿主并在用例完成后解除绑定
@param [in] scenario 使用当前等待预算的功能验证
@return 功能验证的退出码
*/
template <typename Scenario>
static int WithWaitProbe(Scenario scenario)
{
    static uint32_t waited;
    waited = UINT32_MAX;
    NetHostExecutionFunctions host = {};
    host.create = [](void** instance) -> int32_t { *instance = &waited; return 0; };
    host.exchange = [](void* instance) -> void* {
        static void* bound = nullptr;
        void* previous = bound;
        bound = instance;
        return previous;
    };
    host.begin = [](void*) -> uint64_t { return 0; };
    host.wait = [](void*, uint64_t, uint32_t timeout) -> int32_t { waited = timeout; return 0; };
    host.wake = [](void*) {};
    host.destroy = [](void*) -> int32_t { return 0; };
    CHECK_TEST(NetExecution::configure(host) == NI_OK);
    const int result = scenario(waited);
    NetHostExecutionFunctions empty = {};
    CHECK_TEST(NetExecution::configure(empty) == NI_OK);
    return result;
}

GatewayDrainListener::GatewayDrainListener() :
m_received(0)
{

}

bool GatewayDrainListener::onFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    ++m_calls;
    m_valid = m_valid && peer == 1 && frame.m_kind == NI_DATA && frame.m_productId == 7 &&
        frame.m_requestId == 5 + m_received && frame.m_messageType == 11 && frame.m_payloadSize == 2 &&
        frame.m_payload && frame.m_payload[0] == 0 && frame.m_payload[1] == 98 + m_received;
    if (m_accept) { ++m_received; }
    return m_accept;
}

#if defined(NET_INTERFACE_SDK_STATIC_LINK) || defined(__EMSCRIPTEN__)
#include "NetInterface/NetGatewayClientChannelState.h"

/** 用真实密文耗尽外层窗口，只在 CREDIT 让积压输出重新可发时要求零等待
@param [in,out] channel 已认证的业务通道状态
@param [in,out] server 接收并解密全部密文的真实服务端
@param [in] now 业务 TLS 单调毫秒
@param [in,out] owner 同一线程上的事件执行者
@param [in] waited 宿主收到的等待预算
@return 信用、宿主等待及解密后的全部正文符合预期时为0
*/
static int CheckGatewayCreditProgress(NetGatewayClientChannelState& channel, NetInterfaceTls& server,
    uint64_t now, NetExecution& owner, const uint32_t& waited)
{
    CiphertextProbeHost host;
    CiphertextProbeTransport transport(host);
    host.m_cipherReceiver = &server;
    channel.m_transport = &transport;
    channel.m_linkPeer = 1;
    CHECK_TEST(channel.m_credit.configure(65536) == NI_OK);
    uint8_t body[1024] = {};
    uint8_t plain[NET_INTERFACE_HEADER_SIZE + sizeof(body)];
    std::vector<uint8_t> expected;
    size_t received = 0;
    const auto read = [&]() -> bool {
        for (;;)
        {
            uint32_t size = 0;
            if (server.read(plain, sizeof(plain), size) < 0) { return false; }
            if (size == 0) { return true; }
            if (received + size > expected.size() || std::memcmp(plain, expected.data() + received, size)) { return false; }
            received += size;
        }
    };
    const auto grant = [&](uint32_t bytes) -> bool {
        ProtoMessage credit;
        if (!SetWireInt32(credit, NI_GATEWAY_CREDIT_BYTES, static_cast<int32_t>(bytes))) { return false; }
        channel.acceptCredit(credit);
        return true;
    };
    for (uint32_t id = 1; id <= 64; ++id)
    {
        body[0] = static_cast<uint8_t>(id);
        const NetInterfaceFrame frame = {7, id, 25, NI_REQUEST, sizeof(body), body};
        uint32_t size = 0;
        CHECK_TEST(NetInterface::encode(frame, plain, sizeof(plain), size) == NI_OK);
        expected.insert(expected.end(), plain, plain + size);
        CHECK_TEST(channel.m_connection->send(frame) == NI_OK);
        channel.advanceInput(now);
        channel.sendOutput();
        CHECK_TEST(channel.m_phase == NET_GATEWAY_CLIENT_ACTIVE && read());
    }
    CHECK_TEST(host.m_cipherReceived == 65536 && channel.m_credit.available() == 0 &&
        channel.m_connection->outputState() == NI_READY && received < expected.size());
    owner.begin();
    CHECK_TEST(grant(1));
    CHECK_TEST(channel.m_credit.available() == 1 && owner.wait(5000) == NI_OK && waited == 0);
    owner.begin();
    CHECK_TEST(grant(1));
    CHECK_TEST(channel.m_credit.available() == 2 && owner.wait(5000) == NI_OK && waited == 5000);
    channel.sendOutput();
    CHECK_TEST(host.m_cipherReceived == 65538 && channel.m_credit.available() == 0 && read());
    for (unsigned turn = 0; turn < 256 && channel.m_connection->outputState() == NI_READY; ++turn)
    {
        channel.advanceInput(now);
        const uint8_t* bytes = nullptr;
        uint32_t size = 0;
        CHECK_TEST(channel.m_connection->peekOutput(bytes, size) >= 0 && size != 0);
        const uint32_t part = (std::min)(size, static_cast<uint32_t>(16384));
        CHECK_TEST(grant(part));
        channel.sendOutput();
        CHECK_TEST(channel.m_credit.available() == 0 && read());
    }
    CHECK_TEST(received == expected.size() && channel.m_connection->outputState() == NI_OK);
    owner.begin();
    CHECK_TEST(grant(1));
    CHECK_TEST(channel.m_credit.available() == 1 && owner.wait(5000) == NI_OK && waited == 5000);
    channel.m_transport = nullptr;
    return 0;
}

/** 用测试 CA 建立真实业务 TLS，同一轮接纳末批密文并交付 CHANNEL_CLOSE
@param [in] scenario 正常、缺少认证关闭、显式错误、恢复接纳、持续拒收、显式错误且拒收、应用半帧
@param [in,out] owner 记录宿主实际可等待的时间
@param [in] waited 最近一次宿主等待预算
@return 正文、关闭顺序、固定期限和终态均符合预期时为0
*/
static int GatewayTlsDrain(unsigned scenario, NetExecution& owner, const uint32_t& waited)
{
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM;
    static const uint8_t crt[] = TEST_SRV_CRT_RSA_SHA256_PEM;
    static const uint8_t key[] = TEST_SRV_KEY_RSA_PEM;
    uint32_t seeds[] = {73, 97};
    NetInterfaceTlsOptions options = {};
    options.entropy = HelloEntropy;
    options.entropyContext = seeds;
    options.hostname = "localhost";
    options.authority = ca;
    options.authoritySize = sizeof(ca);
    NetGatewayClientChannelState channel(7, nullptr);
    CEncodeDecodeTlsQueueLimits limits;
    CHECK_TEST(limits.set(17, -1));
    channel.m_connection.reset(new NetInterfaceConnection(options, 1024, NetConnectionPoolConfig(), limits));
    options.server = 1;
    options.hostname = nullptr;
    options.entropyContext = seeds + 1;
    options.certificate = crt;
    options.certificateSize = sizeof(crt);
    options.privateKey = key;
    options.privateKeySize = sizeof(key);
    NetInterfaceTls server(options);
    uint64_t now = 0;
    for (; now < 5000; ++now)
    {
        CHECK_TEST(channel.m_connection->progress(now) >= 0 && server.progress(now) >= 0 &&
            TransferCiphertext(*channel.m_connection, server) && TransferCiphertext(server, *channel.m_connection));
        if (channel.m_connection->status() == NET_INTERFACE_TLS_READY && server.status() == NET_INTERFACE_TLS_READY &&
            channel.m_connection->outputState() == NI_OK && server.outputState() == NI_OK) { break; }
    }
    CHECK_TEST(now < 5000);
    GatewayDrainListener listener;
    listener.m_accept = scenario < 3;
    channel.m_listener = &listener;
    channel.m_phase = NET_GATEWAY_CLIENT_HANDSHAKING;
    CHECK_TEST(channel.m_timing.configure(0, 100) == NI_OK && channel.m_timing.configurePollBudgets(1, 1) == NI_OK);
    channel.advanceInput(++now);
    CHECK_TEST(channel.m_phase == NET_GATEWAY_CLIENT_ACTIVE && !listener.m_calls);
    if (scenario == 0 && CheckGatewayCreditProgress(channel, server, now, owner, waited)) { return 1; }

    uint8_t body[] = {0, 98};
    NetInterfaceFrame frame = {7, 5, 11, NI_DATA, sizeof(body), body};
    uint8_t plain[2 * (NET_INTERFACE_HEADER_SIZE + sizeof(body))];
    uint32_t frameSize = 0, accepted = 0;
    CHECK_TEST(NetInterface::encode(frame, plain, sizeof(plain), frameSize) == NI_OK && frameSize == sizeof(plain) / 2);
    ++frame.m_requestId;
    ++body[1];
    uint32_t secondSize = 0;
    CHECK_TEST(NetInterface::encode(frame, plain + frameSize, frameSize, secondSize) == NI_OK && secondSize == frameSize);
    const uint32_t size = scenario == 6 ? frameSize - 1 : static_cast<uint32_t>(sizeof(plain));
    CHECK_TEST(server.write(plain, size, accepted) >= 0 && accepted == size && server.progress(now) >= 0);
    if (scenario != 1) { CHECK_TEST(server.close() == NET_INTERFACE_TLS_OK); }
    std::vector<uint8_t> wire;
    for (;;)
    {
        const uint8_t* bytes = nullptr;
        uint32_t available = 0;
        CHECK_TEST(server.peekOutput(bytes, available) >= 0);
        if (!available) { break; }
        wire.insert(wire.end(), bytes, bytes + available);
        CHECK_TEST(server.consumeOutput(available) >= 0);
    }
    if (scenario == 1) { wire.insert(wire.end(), kTlsHeaderPrefix, kTlsHeaderPrefix + sizeof(kTlsHeaderPrefix)); }
    CHECK_TEST(wire.size() > 17 && wire.size() <= 16384);
    const NetInterfaceTunnelRecord record = {NI_TUNNEL_BYTES, 7, 1, wire.data(), static_cast<uint32_t>(wire.size())};
    if (scenario == 0)
    {
        const NetInterfaceTunnelRecord first = {NI_TUNNEL_BYTES, 7, 1, wire.data(), 17};
        const NetInterfaceTunnelRecord rest = {NI_TUNNEL_BYTES, 7, 1, wire.data() + 17, record.m_size - 17};
        CHECK_TEST(channel.acceptRecord(first) && channel.m_inputSize == 0);
        owner.begin();
        CHECK_TEST(channel.acceptRecord(rest) && owner.wait(5000) == NI_OK && waited == 0);
        owner.begin();
        CHECK_TEST(!channel.acceptRecord(rest) && owner.wait(5000) == NI_OK && waited == 5000);
    }
    else { CHECK_TEST(channel.acceptRecord(record)); }
    CHECK_TEST(channel.m_inputSize == wire.size() - 17 && !listener.m_calls);
    const int32_t reason = scenario == 2 || scenario == 5 ? NI_CANCELLED : NI_OK;
    ProtoMessage event;
    CHECK_TEST(SetWireInt32(event, NI_GATEWAY_REASON, reason));
    channel.acceptClose(event, now);
    channel.endInput(NI_PROTOCOL_ERROR, now);
    CHECK_TEST(channel.m_phase == NET_GATEWAY_CLIENT_ACTIVE && channel.m_drainDeadline == now + 100);

    for (unsigned step = 0; step < 32 && !listener.m_calls && channel.m_phase == NET_GATEWAY_CLIENT_ACTIVE; ++step)
    {
        owner.begin();
        channel.advanceInput(now);
        if (!listener.m_calls && channel.m_phase == NET_GATEWAY_CLIENT_ACTIVE)
        {
            CHECK_TEST(owner.wait(60000) == NI_OK && waited == 0);
        }
    }
    if (scenario == 6)
    {
        CHECK_TEST(channel.m_phase == NET_GATEWAY_CLIENT_CLOSING && channel.m_reason == NI_TRUNCATED && !listener.m_calls);
        return 0;
    }
    CHECK_TEST(listener.m_valid && listener.m_calls == 1 && channel.m_phase == NET_GATEWAY_CLIENT_ACTIVE);
    if (scenario >= 3)
    {
        CHECK_TEST(channel.m_blockedDeadline == now + 100);
        listener.m_accept = true;
        channel.advanceInput(now + 90);
        CHECK_TEST(listener.m_calls == 2 && channel.m_blockedDeadline == 0);
        if (scenario >= 4)
        {
            listener.m_accept = false;
            owner.begin();
            channel.advanceInput(now + 95);
            CHECK_TEST(listener.m_calls == 3 && channel.m_blockedDeadline == now + 195 &&
                owner.wait(60000) == NI_OK && waited <= 5);
            channel.advanceInput(now + 100);
            CHECK_TEST(channel.m_phase == NET_GATEWAY_CLIENT_CLOSING && listener.m_valid && listener.m_calls == 3 && listener.m_received == 1 &&
                channel.m_reason == (scenario == 5 ? NI_CANCELLED : NI_TIMEOUT));
            return 0;
        }
        now += 90;
    }
    for (unsigned step = 0; step < 32 && channel.m_phase == NET_GATEWAY_CLIENT_ACTIVE; ++step) { channel.advanceInput(now); }
    CHECK_TEST(listener.m_valid && listener.m_received == 2 && listener.m_calls == (scenario == 3 ? 3U : 2U) &&
        channel.m_phase == NET_GATEWAY_CLIENT_CLOSING && channel.m_reason == (scenario == 1 ? NI_TRUNCATED : reason));
    return 0;
}
#endif

/** 限制业务 TLS 输入队列并观察每轮等待与信用归还
@return 缓冲可推进时不等待且前缀只归还一次时为零
*/
static int StagedCiphertextProgress()
{
    return WithWaitProbe([](const uint32_t& waited) -> int {
        NetExecution owner;
        NetExecutionScope scope(owner);
        uint32_t entropy = 173;
#if defined(NET_INTERFACE_SDK_STATIC_LINK) || defined(__EMSCRIPTEN__)
        for (unsigned scenario = 0; scenario <= 6; ++scenario) { if (GatewayTlsDrain(scenario, owner, waited)) { return 1; } }
#endif
        NetInterfaceTlsOptions tls = {};
        tls.entropy = HelloEntropy;
        tls.entropyContext = &entropy;
        {
            CiphertextProbeHost probe;
            CiphertextProbeTransport transport(probe);
            FrameDeliveryProbe listener;
            NetGatewayClientChannel gateway(7, &transport);
            probe.m_manualCallbacks = true;
            probe.m_offerPrefix = true;
            uint64_t peer = 0;
            CHECK_TEST(gateway.connect("127.0.0.1", 8000, tls, &listener, peer) == NI_OK && gateway.poll() && !probe.m_offered);
            owner.begin();
            CHECK_TEST(transport.dispatchNext() && owner.wait(5000) == NI_OK && waited == 0 && !probe.m_offered);
            CHECK_TEST(gateway.poll() && probe.m_offered != 0 && !probe.m_first);
            owner.begin();
            CHECK_TEST(transport.dispatchNext() && owner.wait(5000) == NI_OK && waited == 0 && !probe.m_first);
            CHECK_TEST(gateway.poll() && probe.m_first > 1 && gateway.outputState(peer) == NI_OK);
            owner.begin();
            CHECK_TEST(transport.dispatchCredit(1) && owner.wait(5000) == NI_OK && waited == 5000);
            owner.begin();
            probe.m_fault = 3;
            CHECK_TEST(transport.dispatchNext() && owner.wait(5000) == NI_OK && waited == 0 && listener.m_reason == NI_OK);
            owner.begin();
            probe.m_fault = 3;
            CHECK_TEST(transport.dispatchNext() && owner.wait(5000) == NI_OK && waited == 5000 &&
                !gateway.poll() && listener.m_reason == NI_PROTOCOL_ERROR);
        }
        {
            CiphertextProbeHost probe;
            CiphertextProbeTransport transport(probe);
            FrameDeliveryProbe listener;
            NetGatewayClientChannel gateway(7, &transport);
            uint64_t peer = 0;
            CHECK_TEST(gateway.connect("127.0.0.1", 8000, tls, &listener, peer) == NI_OK);
            owner.begin();
            gateway.shutdown();
            CHECK_TEST(!gateway.poll() && listener.m_reason == NI_OK && owner.wait(5000) == NI_OK && waited == 5000);
        }
        for (unsigned end = 0; end <= 3; ++end)
        {
            CiphertextProbeHost probe;
            CiphertextProbeTransport transport(probe);
            FrameDeliveryProbe listener;
            NetGatewayClientChannel gateway(7, &transport);
            CEncodeDecodeTlsQueueLimits limits;
            probe.m_offerPrefix = true;
            probe.m_endAfterPrefix = end;
            probe.m_refuseCredit = end == 0;
            probe.m_offerCreditTail = end == 0;
            ProtoMessage close;
            CHECK_TEST(close.setBytes(NI_GATEWAY_CHANNEL, kWireChannelOne, sizeof(kWireChannelOne), Variant::UINT64) &&
                SetWireInt32(close, NI_GATEWAY_REASON, 0));
            if (end == 1) { CHECK_TEST(close.toString(probe.m_closeEvent)); }
            uint64_t peer = 0;
            CHECK_TEST(limits.set(end == 0 ? -1 : 1, -1) && gateway.configureTlsQueueLimits(limits) == NI_OK &&
                gateway.connect("127.0.0.1", 8000, tls, &listener, peer) == NI_OK && gateway.poll() && gateway.poll());
            CHECK_TEST(probe.m_first != 0 && gateway.outputState(peer) == NI_OK && !probe.m_prefixSent);
            owner.begin();
            CHECK_TEST(gateway.poll() && probe.m_prefixSent && owner.wait(60000) == NI_OK && (end == 0 || waited == 0));
            if (end == 0)
            {
                CHECK_TEST(transport.outputState(peer) == NI_OK && gateway.outputState(peer) == NI_READY &&
                    gateway.poll() && probe.m_creditTailSent && probe.m_creditAttempts == 2 &&
                    probe.m_prefixCredit == 0 && gateway.outputState(peer) == NI_READY);
                probe.m_refuseCredit = false;
                owner.begin();
                CHECK_TEST(gateway.poll() && probe.m_creditAttempts == 3 && probe.m_prefixCredit == 4 &&
                    gateway.outputState(peer) == NI_READY && owner.wait(60000) == NI_OK && waited == 0);
                CHECK_TEST(gateway.poll() && probe.m_creditAttempts == 4 && probe.m_prefixCredit == 5 &&
                    gateway.outputState(peer) == NI_OK && gateway.poll() && probe.m_creditAttempts == 4);
            }
            else
            {
                CHECK_TEST(probe.m_closeEvent.empty() && gateway.outputState(peer) == NI_INVALID_STATE);
                for (unsigned i = 0; i < 8 && gateway.poll(); ++i) {}
                CHECK_TEST(!gateway.poll() && probe.m_creditAttempts == 0 &&
                    listener.m_reason == (end == 2 ? NI_CANCELLED : NI_TRUNCATED));
            }
        }
        CiphertextProbeHost host;
        host.m_offerPrefix = true;
        CiphertextProbeTransport transport(host);
        FrameDeliveryProbe listener;
        NetGatewayClientChannel gateway(7, &transport);
        CEncodeDecodeTlsQueueLimits limits;
        uint64_t peer = 0;
        NetInterfaceTlsOptions shortHandshake = tls;
        host.m_expectedHandshakeMs = shortHandshake.handshakeTimeoutMs = 250;
        CHECK_TEST(limits.set(1, -1) && gateway.configureTlsQueueLimits(limits) == NI_OK &&
            gateway.configureServiceTimeouts(350, 1000) == NI_OK &&
            gateway.connect("127.0.0.1", 8000, shortHandshake, &listener, peer) == NI_OK && peer == 1);
        CHECK_TEST(gateway.poll() && gateway.poll() && host.m_first > 0 && !host.m_prefixSent);
        for (uint32_t remaining = 3; remaining != 0; --remaining)
        {
            owner.begin();
            CHECK_TEST(gateway.poll() && host.m_prefixSent && host.m_prefixCredit == 4);
            CHECK_TEST(owner.wait(60000) == NI_OK && waited == 0);
        }
        owner.begin();
        CHECK_TEST(gateway.poll() && owner.wait(60000) == NI_OK && waited > 0 &&
            waited <= shortHandshake.handshakeTimeoutMs && host.m_prefixCredit == 4);
        YieldTimerProbe(351);
        CHECK_TEST(!gateway.poll() && listener.m_reason == NET_INTERFACE_TLS_TIMEOUT);
        CiphertextProbeHost silent;
        CiphertextProbeTransport slow(silent);
        FrameDeliveryProbe stopped;
        NetGatewayClientChannel outer(7, &slow);
        CHECK_TEST(outer.configureServiceTimeouts(150, 1000) == NI_OK &&
            outer.connect("127.0.0.1", 8000, tls, &stopped, peer) == NI_OK);
        owner.begin();
        CHECK_TEST(outer.poll() && outer.poll() && silent.m_first != 0 && owner.wait(60000) == NI_OK && waited <= 150);
        YieldTimerProbe(151);
        CHECK_TEST(!outer.poll() && stopped.m_reason == NI_TIMEOUT);
        gateway.shutdown();

        // TLS 只接纳第一个字节。未接纳的后缀、零字节和重复 EOF 都不能让宿主反复空转。
        NetInterfaceTlsOptions rawOptions = tls;
        rawOptions.authority = NetInterface::trustAuthority(&rawOptions.authoritySize);
        rawOptions.hostname = "service.7";
        CEncodeDecodeTlsQueueLimits rawLimits;
        CHECK_TEST(rawLimits.set(1, -1));
        NetInterfaceTls raw(rawOptions, rawLimits);
        CHECK_TEST(raw.progress(100) == NET_INTERFACE_TLS_HANDSHAKE);
        uint32_t used = 99;
        owner.begin();
        CHECK_TEST(raw.feed(kTlsHeaderPrefix, sizeof(kTlsHeaderPrefix), used) == NET_INTERFACE_TLS_WANT_IO &&
            used == 1 && owner.wait(5000) == NI_OK && waited == 0);
        owner.begin();
        CHECK_TEST(raw.feed(kTlsHeaderPrefix + 1, 3, used) == NET_INTERFACE_TLS_WANT_IO && !used &&
            raw.feed(nullptr, 0, used) == NET_INTERFACE_TLS_OK && !used &&
            raw.feed(nullptr, 1, used) == NET_INTERFACE_TLS_ARGUMENT && !used && owner.wait(5000) == NI_OK && waited == 5000);
        bool first = false;
        owner.begin();
        CHECK_TEST(raw.finishInput(first) == NET_INTERFACE_TLS_OK && first && owner.wait(5000) == NI_OK && waited == 0);
        owner.begin();
        CHECK_TEST(raw.finishInput(first) == NET_INTERFACE_TLS_OK && !first &&
            raw.finishInput() == NET_INTERFACE_TLS_OK && owner.wait(5000) == NI_OK && waited == 5000);
        CHECK_TEST(raw.progress(101) == NET_INTERFACE_TLS_TRUNCATED);

        NetInterfaceLinkOptions options = {};
        options.m_version = 3;
        options.m_role = NI_LINK_CLIENT;
        options.m_tls = tls;
        NetInterfaceLinkPools pools;
        pools.m_controlStepBudget = 1;
        CHECK_TEST(pools.m_controlCiphertext.set(1, -1));
        NetInterfaceLink link(options, pools);
        NetInterfaceTunnelRecord record = {};
        record.m_kind = NI_TUNNEL_HELLO;
        CHECK_TEST(link.progress(199) == NI_NEED_MORE);
        owner.begin();
        CHECK_TEST(link.accept(record) == NI_OK &&
            owner.wait(60000) == NI_OK && waited == 0 && link.peekOutput(record) == NI_READY &&
            record.m_kind == NI_TUNNEL_HELLO && link.consumeOutput() == NI_OK);
        CHECK_TEST(link.progress(200) == NI_NEED_MORE && link.peekOutput(record) == NI_READY &&
            record.m_kind == NI_TUNNEL_CONTROL && link.consumeOutput() == NI_OK);
        record.m_kind = NI_TUNNEL_CONTROL;
        record.m_data = kTlsHeaderPrefix;
        record.m_size = sizeof(kTlsHeaderPrefix);
        owner.begin();
        CHECK_TEST(link.accept(record) == NI_OK && owner.wait(5000) == NI_OK && waited == 0);
        owner.begin();
        CHECK_TEST(link.accept(record) == NI_LIMIT_EXCEEDED && owner.wait(5000) == NI_OK && waited == 5000);
        // 每一轮实际向 1 字节 TLS 队列提交一个前缀，包含第四个也是最后一个字节。
        for (uint32_t step = 1; step <= sizeof(kTlsHeaderPrefix); ++step)
        {
            owner.begin();
            CHECK_TEST(link.progress(200 + step) == NI_NEED_MORE && owner.wait(60000) == NI_OK && waited == 0);
        }
        owner.begin();
        CHECK_TEST(link.progress(205) == NI_NEED_MORE && owner.wait(60000) == NI_OK && waited > 0);

        // 双向各一字节。让 ClientHello 暂留在已满的输出队列，TLS 不能继续读取。
        CHECK_TEST(pools.m_controlCiphertext.set(1, 1));
        NetInterfaceLink blocked(options, pools);
        record = NetInterfaceTunnelRecord();
        record.m_kind = NI_TUNNEL_HELLO;
        CHECK_TEST(blocked.accept(record) == NI_OK && blocked.peekOutput(record) == NI_READY &&
            blocked.consumeOutput() == NI_OK && blocked.progress(300) == NI_NEED_MORE);
        CHECK_TEST(blocked.peekOutput(record) == NI_READY && record.m_kind == NI_TUNNEL_CONTROL && record.m_size == 1);
        record.m_data = kTlsHeaderPrefix;
        record.m_size = sizeof(kTlsHeaderPrefix);
        CHECK_TEST(blocked.accept(record) == NI_OK);
        owner.begin();
        CHECK_TEST(blocked.progress(301) == NI_NEED_MORE && owner.wait(60000) == NI_OK && waited == 0);
        owner.begin();
        CHECK_TEST(blocked.progress(302) == NI_NEED_MORE && owner.wait(60000) == NI_OK && waited > 0);
        return 0;
    });
}

SessionProbeChannel::SessionProbeChannel(NetInterfaceHandshake& server) :
m_configuringSession(nullptr),
m_reentryRejected(true),
m_poolCalls(0),
m_recordLimit(0),
m_connectException(NI_OK),
m_pollException(NI_OK),
m_throwClose(false),
m_throwStop(false),
m_deliveringFrame(false),
m_stoppedInFrame(false),
m_stops(0),
m_poolResult(NI_OK),
m_codecCapabilities(0),
m_lastRequest(0),
m_lastCancel(0),
m_acceptedRequests(0),
m_acceptedCancels(0),
m_requestTimeout(0),
m_blockRequest(false),
m_deferReply(false),
m_deferClose(false),
m_disconnectReason(NI_READY),
m_closeCalls(0),
m_hellos(0),
m_pings(0),
m_pingResult(NI_OK),
m_heartbeatTls(nullptr),
m_heartbeatTlsTime(0),
m_event(nullptr),
m_server(server),
m_listener(nullptr),
m_announced(false)
{

}

int32_t SessionProbeChannel::configureConnectionPools(const NetConnectionPoolConfig& config)
{
    ++m_poolCalls;
    m_recordLimit = config.m_dataRecords.maximumCapacity;
    if (m_configuringSession != nullptr)
    {
        m_reentryRejected = m_reentryRejected && m_configuringSession->executing() &&
            m_configuringSession->bindChannel(nullptr) == NI_INVALID_STATE &&
            m_configuringSession->configureConnectionPools(config) == NI_INVALID_STATE;
    }
    if (m_poolResult == NI_NO_MEMORY)
    {
        throw std::bad_alloc();
    }
    if (m_poolResult == NI_INVALID_STATE)
    {
        throw std::runtime_error("configureConnectionPools");
    }
    return m_poolResult;
}

int32_t SessionProbeChannel::connect(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener* listener, uint64_t& peer)
{
    if (m_connectException == NI_NO_MEMORY)
    {
        throw std::bad_alloc();
    }
    if (m_connectException == NI_INVALID_STATE)
    {
        throw std::runtime_error("channel connect");
    }
    if (m_connectException == NI_LIMIT_EXCEEDED) { return NI_LIMIT_EXCEEDED; }
    peer = m_connectException == NI_PROTOCOL_ERROR ? 0 : 7;
    m_lastRequest = 0;
    m_codecCapabilities = 0;
    m_codecTypes.clear();
    m_announced = false;
    m_listener = listener;
    return NI_OK;
}

bool SessionProbeChannel::poll()
{
    if (m_pollException == NI_NO_MEMORY) { throw std::bad_alloc(); }
    if (m_pollException == NI_INVALID_STATE) { throw std::runtime_error("channel poll"); }
    if (m_listener == nullptr) { return false; }
    if (m_disconnectReason <= NI_OK)
    {
        NetTransportListener* listener = m_listener;
        m_listener = nullptr;
        listener->onClosed(7, m_disconnectReason, 0);
        return false;
    }
    if (!m_announced)
    {
        m_announced = true;
        m_listener->onReady(7);
    }
    if (m_heartbeatTls != nullptr)
    {
        if (m_heartbeatTls->progress(m_heartbeatTlsTime) < 0) { return false; }
        const NetInterfaceFrame* received = nullptr;
        const int32_t status = m_heartbeatTls->peek(received);
        if (status < 0) { return false; }
        if (status == NI_READY && m_listener->onFrame(7, *received)) { m_heartbeatTls->consume(); }
    }
    NetInterfaceFrame frame = {};
    if (m_event != nullptr)
    {
        m_deliveringFrame = true;
        if (m_listener->onFrame(7, *m_event))
        {
            m_event = nullptr;
        }
        m_deliveringFrame = false;
    }
    if (!m_deferReply && m_server.peekOutput(frame) == NI_READY && m_listener->onFrame(7, frame))
    {
        m_server.consumeOutput();
    }
    return true;
}

int32_t SessionProbeChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != 7) { return NI_INVALID_ARGUMENT; }
    if (frame.m_kind == NI_HELLO)
    {
        ++m_hellos;
        return m_server.accept(frame, 1) < 0 ? NI_PROTOCOL_ERROR : NI_OK;
    }
    if (frame.m_kind == NI_PING)
    {
        const int32_t result = m_heartbeatTls ? m_heartbeatTls->send(frame) : m_pingResult;
        if (result == NI_OK) { ++m_pings; }
        return result;
    }
    if (frame.m_kind == NI_PONG) { return NI_LIMIT_EXCEEDED; }
    if (frame.m_kind == NI_DATA)
    {
        if (frame.m_requestId != m_dataIds.size() + 1 || frame.m_payloadSize != 2 ||
            !frame.m_payload || frame.m_payload[0] != 19 || frame.m_payload[1] != 31) { return NI_PROTOCOL_ERROR; }
        m_dataIds.push_back(frame.m_requestId);
        return NI_OK;
    }
    if (frame.m_kind == NI_CANCEL)
    {
        m_lastCancel = frame.m_requestId;
        ++m_acceptedCancels;
        return NI_OK;
    }
    NetInterfaceRpcView rpc = {};
    if (frame.m_kind != NI_REQUEST || NetInterfaceRpc::parse(frame, rpc) != NI_OK) { return NI_PROTOCOL_ERROR; }
    if (m_blockRequest) { return NI_LIMIT_EXCEEDED; }
    if (frame.m_requestId <= m_lastRequest) { return NI_PROTOCOL_ERROR; }
    if (rpc.m_size == 0)
    {
        m_body.clear();
    }
    else
    {
        m_body.assign(rpc.m_data, rpc.m_data + rpc.m_size);
    }
    m_lastRequest = frame.m_requestId;
    m_requestTimeout = rpc.m_value;
    ++m_acceptedRequests;
    return NI_OK;
}

int32_t SessionProbeChannel::outputState(uint64_t peer) const
{
    return peer == 7 ? NI_OK : NI_INVALID_ARGUMENT;
}

int32_t SessionProbeChannel::configureCompact(uint64_t peer, bool enabled)
{
    if (peer != 7) { return NI_INVALID_ARGUMENT; }
    if (enabled) { m_codecCapabilities |= 16; }
    return NI_OK;
}

int32_t SessionProbeChannel::configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated)
{
    if (peer != 7 || !negotiated || (count && !types)) { return NI_INVALID_ARGUMENT; }
    m_codecTypes.clear();
    if (count) { m_codecTypes.assign(types, types + count); }
    m_codecCapabilities |= 8;
    return NI_OK;
}

void SessionProbeChannel::close(uint64_t, int32_t)
{
    ++m_closeCalls;
    if (m_throwClose)
    {
        throw std::runtime_error("channel close");
    }
    if (!m_deferClose)
    {
        m_listener = nullptr;
    }
}

void SessionProbeChannel::shutdown()
{
    ++m_stops;
    m_stoppedInFrame = m_stoppedInFrame || m_deliveringFrame;
    m_listener = nullptr;
    if (m_throwStop)
    {
        throw std::runtime_error("channel shutdown");
    }
}

#if defined(__EMSCRIPTEN__)
#include <time.h>

// 每个测试线程独立推进其协议时钟，其他系统时钟仍取宿主原值。
static thread_local uint64_t s_timerAdvanceMs = 0;

/** 链接器提供的原始系统时钟
@param [in] clock 时钟类型
@param [out] value 系统返回的时间
@return libc 的返回值
*/
extern "C" int __real_clock_gettime(clockid_t clock, struct timespec* value);

/** 为本测试线程的单调时钟累加已经模拟的等待
@param [in] clock 时钟类型
@param [out] value 成功时写入时间
@return 保留原时钟调用的成功或失败结果
*/
extern "C" int __wrap_clock_gettime(clockid_t clock, struct timespec* value)
{
    const int result = __real_clock_gettime(clock, value);
    if (result == 0 && clock == CLOCK_MONOTONIC && s_timerAdvanceMs != 0)
    {
        const long nanos = value->tv_nsec + static_cast<long>(s_timerAdvanceMs % 1000) * 1000000L;
        value->tv_sec += static_cast<time_t>(s_timerAdvanceMs / 1000 + nanos / 1000000000L);
        value->tv_nsec = nanos % 1000000000L;
    }
    return result;
}
#endif

/** 推进协议与用例共用的单调时钟；原生等待，Wasm 测试宿主直接推进
@param [in] milliseconds 要经过的毫秒数，默认一次推进一毫秒
*/
static void YieldTimerProbe(uint32_t milliseconds = 1)
{
#if defined(__EMSCRIPTEN__)
    s_timerAdvanceMs += milliseconds;
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}

/** 等到给定的测试时刻，系统提前返回时补足剩余时间
@param [in] due 短时间用例的单调期限
*/
static void WaitTimerProbe(std::chrono::steady_clock::time_point due)
{
    for (;;)
    {
        const auto now = TimerProbeNow();
        if (now >= due) { return; }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(due - now);
        YieldTimerProbe(static_cast<uint32_t>(remaining.count()) + 1);
    }
}

/** 对真实业务请求检查会话缓存的保留和释放，同时核对实际传给宿主的等待上限
@param [in] retention 请求编码缓存预算，-1保留当前容量
@param [in] heartbeatMode 0检查空闲期限，1、2分别让总无输入或PONG期限先到
@param [in] waited 最近一次事件等待回调收到的预算
@return 正文、容量和期限均符合预期时为0
*/
static int CheckSessionOptions(int64_t retention, const uint32_t& waited, unsigned heartbeatMode = 0)
{
    NetInterfaceHandshakeOptions serverOptions = {};
    serverOptions.m_server = 1;
    serverOptions.m_capabilities = 1;
    serverOptions.m_maxRequests = 4;
    serverOptions.m_maxChunk = 1024;
    serverOptions.m_timeoutMs = 5000;
    serverOptions.m_nonce[0] = 19;
    NetInterfaceHandshake server(serverOptions, 0);
    NetInterfaceHandshakeOptions options = serverOptions;
    SessionProbeChannel channel(server);
    CiphertextProbeHost events;
    NetClientSession client;
    NetHeartbeatConfig heartbeat;
    const bool custom = retention != 4096;
    if (custom)
    {
        heartbeat.m_pingIntervalMs = 11000;
        heartbeat.m_pongTimeoutMs = 19000;
        heartbeat.m_receiveTimeoutMs = 35000;
        if (heartbeatMode != 0)
        {
            heartbeat.m_pingIntervalMs = 32;
            heartbeat.m_receiveTimeoutMs = heartbeatMode == 1 ? 525 : 1800;
            heartbeat.m_pongTimeoutMs = heartbeatMode == 1 ? 1500 : 325;
        }
        CHECK_TEST(client.configureHeartbeat(heartbeat) == NI_OK && client.configurePayloadRetention(retention) == NI_OK);
    }
    heartbeat.m_pongTimeoutMs = 0;
    CHECK_TEST(client.configureHeartbeat(heartbeat) == NI_INVALID_ARGUMENT && client.configurePayloadRetention(-2) == NI_INVALID_ARGUMENT);
    options.m_server = 0;
    options.m_platform = 2;
    options.m_device[0] = 2;
    NetInterfaceTlsOptions tls = {};
    uint32_t seed = 91;
    tls.entropy = HelloEntropy;
    tls.entropyContext = &seed;
    if (custom) { CHECK_TEST(client.configureExpiryBudget(1) == NI_OK); }
    CHECK_TEST(client.configureExpiryBudget(0) == NI_INVALID_ARGUMENT && client.bindChannel(&channel) == NI_OK);
    if (custom)
    {
        NetConnectionPoolConfig pools;
        pools.m_dataRecords.maximumCapacity = 257;
        CHECK_TEST(client.configureConnectionPools(pools) == NI_OK);
        pools.m_dataRecords.maximumCapacity = 513;
        channel.m_configuringSession = &client;
        channel.m_poolResult = NI_LIMIT_EXCEEDED;
        CHECK_TEST(client.connect(nullptr, 9001, tls, options, &events) == NI_INVALID_ARGUMENT &&
            client.connect("", 9001, tls, options, &events) == NI_INVALID_ARGUMENT &&
            channel.m_poolCalls == 0 && seed == 91);
        CHECK_TEST(client.connect("127.0.0.1", 0, tls, options, &events) == NI_INVALID_ARGUMENT &&
            client.connect("127.0.0.1", 65536, tls, options, &events) == NI_INVALID_ARGUMENT &&
            client.connect("127.0.0.1", 9001, tls, options, nullptr) == NI_INVALID_ARGUMENT);
        NetInterfaceTlsOptions invalidTls = tls;
        invalidTls.handshakeTimeoutMs = 99;
        CHECK_TEST(client.connect("127.0.0.1", 9001, invalidTls, options, &events) == NI_INVALID_ARGUMENT);
        invalidTls = tls;
        invalidTls.entropy = nullptr;
        CHECK_TEST(client.connect("127.0.0.1", 9001, invalidTls, options, &events) == NI_INVALID_STATE);
        NetInterfaceHandshakeOptions invalidHello = options;
        invalidHello.m_maxRequests = 0;
        CHECK_TEST(client.connect("127.0.0.1", 9001, tls, invalidHello, &events) == NI_INVALID_ARGUMENT &&
            !channel.m_poolCalls && client.generation() == 0 && client.quiescent() && !events.m_closes);
        const int32_t failures[] = {NI_LIMIT_EXCEEDED, NI_NO_MEMORY, NI_INVALID_STATE};
        for (size_t i = 0; i < sizeof(failures) / sizeof(failures[0]); ++i)
        {
            channel.m_poolResult = failures[i];
            CHECK_TEST(client.connect("127.0.0.1", 9001, tls, options, &events) == failures[i] &&
                channel.m_poolCalls == i + 1 && channel.m_recordLimit == 257 &&
                client.generation() == 0 && client.quiescent() && !events.m_closes);
        }
        channel.m_poolResult = NI_OK;
    }
    if (!custom)
    {
        const uint32_t invalidTimeouts[] = {1, 99, 60001, UINT32_MAX};
        for (size_t i = 0; i < sizeof(invalidTimeouts) / sizeof(invalidTimeouts[0]); ++i)
        {
            tls.handshakeTimeoutMs = invalidTimeouts[i];
            CHECK_TEST(client.connect("127.0.0.1", 9001, tls, options, &events) == NI_INVALID_ARGUMENT &&
                client.generation() == 0 && client.quiescent() && seed == 91);
        }
    }
    if (!custom)
    {
        tls.handshakeTimeoutMs = 0;
        NetSession protocol;
        CHECK_TEST(protocol.bindChannel(&channel) == NI_OK);
        // 以底层会话为上下文，在返回测试随机字节前核对初次连接的重入边界。
        tls.entropy = [](void* context, uint8_t* bytes, uint32_t size) -> int32_t {
            NetSession& session = *static_cast<NetSession*>(context);
            uint32_t closingStatus = UINT32_MAX;
            if (session.closedRequestStatus(closingStatus) || session.configureExpiryBudget(2) != NI_INVALID_STATE ||
                session.configurePayloadRetention(0) != NI_INVALID_STATE ||
                session.configureHeartbeat(NetHeartbeatConfig()) != NI_INVALID_STATE)
            {
                return -1;
            }
            uint32_t state = 17;
            return HelloEntropy(&state, bytes, size);
        };
        tls.entropyContext = &protocol;
        NetConnectionPoolConfig invalidPools;
        invalidPools.m_controlBurst = 0;
        CHECK_TEST(protocol.connectWithPools("127.0.0.1", 9001, tls, options, &events, &invalidPools) == NI_INVALID_ARGUMENT &&
            protocol.generation() == 0 && channel.m_poolCalls == 0);
        const int32_t errors[] = {NI_INVALID_STATE, NI_NO_MEMORY, NI_LIMIT_EXCEEDED, NI_PROTOCOL_ERROR};
        for (size_t i = 0; i < sizeof(errors) / sizeof(errors[0]); ++i)
        {
            channel.m_connectException = errors[i];
            CHECK_TEST(protocol.connect("127.0.0.1", 9001, tls, options, &events) == errors[i] &&
                !protocol.poll() && !events.m_closes && protocol.generation() == 0 && channel.m_stops == i + 1);
            CHECK_TEST(protocol.configureRequestPool(LockFreeHashMapPoolConfig()) == NI_OK &&
                protocol.configureHeartbeat(NetHeartbeatConfig()) == NI_OK && protocol.configureExpiryBudget(2) == NI_OK &&
                protocol.configurePayloadRetention(0) == NI_OK);
        }
        channel.m_connectException = NI_OK;
        CHECK_TEST(protocol.connect("127.0.0.1", 9001, tls, options, &events) == NI_OK && protocol.generation() == 1);
        channel.m_deferClose = true;
        protocol.close(NI_TIMEOUT);
        protocol.close(NI_CANCELLED);
        CHECK_TEST(channel.m_closeCalls == 1 && protocol.poll() && !protocol.ready() && !channel.m_hellos);
        protocol.shutdown();
        CHECK_TEST(events.m_closes == 1 && events.m_closed == NI_TIMEOUT && !protocol.poll());
        channel.m_deferClose = false;
        CHECK_TEST(protocol.connect("127.0.0.1", 9001, tls, options, &events) == NI_NOT_SUPPORTED &&
            protocol.bindChannel(&channel) == NI_OK);
        channel.m_connectException = NI_LIMIT_EXCEEDED;
        CHECK_TEST(protocol.connect("127.0.0.1", 9001, tls, options, &events) == NI_LIMIT_EXCEEDED &&
            protocol.generation() == 1 && events.m_closes == 1 && channel.m_stops == 6);
        protocol.shutdown();
        CHECK_TEST(channel.m_stops == 6);
        channel.m_connectException = NI_OK;

        NetInterfaceHandshake lateServer(serverOptions, 0);
        SessionProbeChannel lateChannel(lateServer);
        NetSession lateProtocol;
        tls.entropyContext = &lateProtocol;
        lateChannel.m_deferReply = true;
        lateChannel.m_deferClose = true;
        CHECK_TEST(lateProtocol.bindChannel(&lateChannel) == NI_OK &&
            lateProtocol.connect("127.0.0.1", 9001, tls, options, &events) == NI_OK &&
            lateProtocol.poll() && !lateProtocol.ready() && lateChannel.m_hellos == 1);
        NetInterfaceFrame reply = {};
        CHECK_TEST(lateChannel.m_closeCalls == 0 && lateServer.peekOutput(reply) == NI_READY && reply.m_kind == NI_HELLO_REPLY);
        lateProtocol.close(NI_CANCELLED);
        lateChannel.m_deferReply = false;
        CHECK_TEST(lateProtocol.poll() && !lateProtocol.ready() && lateChannel.m_closeCalls == 1 &&
            lateServer.peekOutput(reply) == NI_NEED_MORE);
        lateProtocol.shutdown();
        CHECK_TEST(events.m_closes == 2 && events.m_closed == NI_CANCELLED);

        const int32_t pollFaults[] = {NI_INVALID_STATE, NI_NO_MEMORY, NI_INVALID_STATE};
        for (size_t i = 0; i < sizeof(pollFaults) / sizeof(pollFaults[0]); ++i)
        {
            NetInterfaceHandshake faultServer(serverOptions, 0);
            SessionProbeChannel faultChannel(faultServer);
            CiphertextProbeHost observer;
            NetClientSession session;
            tls.entropy = CheckStartEntropy;
            tls.entropyContext = &session;
            CHECK_TEST(session.bindChannel(&faultChannel) == NI_OK);
            faultChannel.m_connectException = NI_LIMIT_EXCEEDED;
            CHECK_TEST(session.connect("localhost", 9001, tls, options, &observer) == NI_LIMIT_EXCEEDED &&
                session.generation() == 0 && session.quiescent() && !observer.m_closes &&
                session.configureRequestPool(LockFreeHashMapPoolConfig()) == NI_OK &&
                session.configureHeartbeat(NetHeartbeatConfig()) == NI_OK && session.configureExpiryBudget(2) == NI_OK &&
                session.configurePayloadRetention(0) == NI_OK);
            faultChannel.m_connectException = NI_OK;
            uint64_t id = 0;
            CHECK_TEST(session.connect("localhost", 9001, tls, options, &observer) == NI_OK &&
                session.generation() == 1 && session.poll() && session.ready() &&
                session.beginRequest(7, 1, 60000, id) == NI_OK && id != 0);
            faultChannel.m_deferClose = true;
            if (i == 2) { session.close(NI_CANCELLED); }
            faultChannel.m_pollException = pollFaults[i];
            CHECK_TEST(!session.poll() && session.quiescent() && !session.ready() &&
                !session.poll() && observer.m_closes == 1 && observer.m_results == 1 && observer.m_resultId == id &&
                observer.m_closed == (i == 2 ? NI_CANCELLED : pollFaults[i]) &&
                observer.m_resultStatus == (i == 2 ? NI_RPC_CANCELLED : NI_RPC_UNAVAILABLE));
            session.shutdown();
            CHECK_TEST(faultChannel.m_stops == 2 && observer.m_closes == 1 && observer.m_results == 1);
        }
    }
    tls.entropy = CheckStartEntropy;
    tls.entropyContext = &client;
    tls.handshakeTimeoutMs = custom ? (heartbeatMode == 0 ? 100U : 60000U) : 0U;
    CHECK_TEST(client.connect("127.0.0.1", 9001, tls, options, &events) == NI_OK &&
        channel.m_poolCalls == (custom ? 4U : 0U) && channel.m_reentryRejected);
    CHECK_TEST(client.configureExpiryBudget(2) == NI_INVALID_STATE);
    NetExecution owner;
    NetExecutionScope scope(owner);
    const auto start = TimerProbeNow();
    CHECK_TEST(client.poll() && client.ready());
    owner.begin();
    CHECK_TEST(client.poll() && owner.wait(120000) == NI_OK);
    const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        TimerProbeNow() - start).count());
    if (heartbeatMode != 0)
    {
        const NetInterfaceFrame pending = {0, 99, 0, static_cast<uint16_t>(heartbeatMode == 1 ? NI_PING : NI_EVENT), 0, nullptr};
        channel.m_event = &pending;
        WaitTimerProbe(start + std::chrono::milliseconds(35));
        CHECK_TEST(client.poll() && channel.m_event == &pending && channel.m_pings == 1 && client.ready());
        owner.begin();
        CHECK_TEST(client.poll() && owner.wait(120000) == NI_OK);
        const uint64_t sinceReady = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            TimerProbeNow() - start).count());
        const uint32_t next = heartbeatMode == 1 ? 525 : 325;
        CHECK_TEST(waited <= next && waited + sinceReady + 2 >= next && channel.m_pings == 1);
        const NetInterfaceFrame late = {0, 1, 0, NI_PONG, 0, nullptr};
        channel.m_event = &late;
        WaitTimerProbe(TimerProbeNow() + std::chrono::milliseconds(next + 1));
        client.poll();
        client.poll();
        CHECK_TEST(!client.ready() && channel.m_closeCalls == 1 && events.m_closes == 1 && events.m_closed == NI_TIMEOUT);
        client.shutdown();
        return 0;
    }
    const uint32_t idle = custom ? 11000 : 30000;
    CHECK_TEST(waited <= idle && elapsed + waited + 2 >= idle);
    CHECK_TEST(client.configurePayloadRetention(0) == NI_INVALID_STATE && client.configureHeartbeat(heartbeat) == NI_INVALID_STATE);
    for (unsigned pass = 0; pass < 2; ++pass)
    {
        std::vector<uint8_t> body(pass == 0 ? 40 : 8192, static_cast<uint8_t>(0xa3 + pass));
        uint64_t id = 0;
        CHECK_TEST(client.beginRequest(7, 9, 60000, id) == NI_OK && id &&
            client.sendRequest(id, body.data(), static_cast<uint32_t>(body.size())) == NI_OK && channel.m_body == body);
        CHECK_TEST(retention == -1 || (retention == 4096 && pass == 0) ?
            client.payloadCapacity() >= body.size() + 16 : client.payloadCapacity() == 0);
        CHECK_TEST(client.cancel(id) == NI_OK);
    }
    uint64_t emptyId = 0;
    CHECK_TEST(client.beginRequest(7, 9, 60000, emptyId) == NI_OK && emptyId &&
        client.sendRequest(emptyId, nullptr, 0) == NI_OK && channel.m_body.empty() && client.cancel(emptyId) == NI_OK);
    owner.begin();
    CHECK_TEST(client.poll() && client.beginRequest(0, 9, 25, emptyId) == NI_INVALID_ARGUMENT &&
        emptyId == 0 && owner.wait(120000) == NI_OK && waited > 25);
    const auto reserved = TimerProbeNow();
    for (unsigned i = 0; i < 3; ++i)
    {
        uint64_t id = 0;
        CHECK_TEST(client.beginRequest(7, 9, 25, id) == NI_OK && id != 0);
    }
    CHECK_TEST(owner.wait(120000) == NI_OK);
    const uint64_t reservingMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        TimerProbeNow() - reserved).count());
    CHECK_TEST(waited <= 25 && waited + reservingMs + 2 >= 25);
    const auto expired = TimerProbeNow() + std::chrono::milliseconds(27);
    WaitTimerProbe(expired);
    CHECK_TEST(client.poll() && events.m_timeouts == (custom ? 1U : 3U));
    if (custom) { CHECK_TEST(client.poll() && events.m_timeouts == 2 && client.poll() && events.m_timeouts == 3); }
    if (!custom)
    {
        const auto startTime = TimerProbeNow();
        const uint64_t started = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            startTime.time_since_epoch()).count());
        const uint32_t deadlines[] = {60000, 100, 60000, 100, 50, 100, 50000};
        for (uint32_t i = 0; i < sizeof(deadlines) / sizeof(deadlines[0]); ++i)
        {
            CHECK_TEST(client.beginRequestAt(100 + i, 7, 9, started, deadlines[i]) == NI_OK);
        }
        CHECK_TEST(client.cancel(102) == NI_OK && client.cancel(106) == NI_OK);
        const auto flush = startTime + std::chrono::milliseconds(105);
        WaitTimerProbe(flush);
        events.m_timeoutIds.clear();
        const uint64_t expected[] = {104, 101, 103, 105};
        CHECK_TEST(client.poll() && events.m_timeoutIds.size() == 4 &&
            std::equal(events.m_timeoutIds.begin(), events.m_timeoutIds.end(), expected) && client.cancel(100) == NI_OK);
    }
    const bool localClose = retention == 0;
    uint64_t pending = 0;
    CHECK_TEST(client.beginRequest(7, 9, 60000, pending) == NI_OK && pending != 0);
    const uint32_t results = events.m_results;
    const uint32_t closes = events.m_closes;
    if (localClose)
    {
        client.close(NI_CANCELLED);
    }
    else
    {
        channel.shutdown();
    }
    CHECK_TEST(!client.poll() && client.quiescent() && !client.ready() && events.m_closes == closes + 1 &&
        events.m_closed == (localClose ? NI_CANCELLED : NI_TRUNCATED) && events.m_results == results + 1 &&
        events.m_resultId == pending && events.m_resultStatus == (localClose ? NI_RPC_CANCELLED : NI_RPC_UNAVAILABLE));
    client.shutdown();
    CHECK_TEST(!client.poll() && events.m_closes == closes + 1 && events.m_results == results + 1 &&
        client.payloadCapacity() == 0);
    return 0;
}

/** 从运输实际接纳的编号生成响应，并核对逆序发送、背压、取消及到期后的业务原号
@return 线路按接纳顺序递增、每条业务请求恰有一次正确结果时为0
*/
static int CheckSessionRequestOrder()
{
    NetInterfaceHandshakeOptions hello = {};
    hello.m_server = hello.m_capabilities = hello.m_maxRequests = 1;
    hello.m_maxChunk = 1024;
    hello.m_timeoutMs = 5000;
    hello.m_nonce[0] = 17;
    NetInterfaceHandshake server(hello, 0);
    SessionProbeChannel channel(server);
    CHECK_TEST(channel.NetClientChannel::configureCompact(1, false) == NI_OK &&
        channel.NetClientChannel::configureCompact(1, true) == NI_NOT_SUPPORTED);
    CiphertextProbeHost events;
    NetClientSession client;
    uint32_t seed = 27;
    NetInterfaceTlsOptions tls = {};
    tls.entropy = HelloEntropy;
    tls.entropyContext = &seed;
    hello.m_server = 0;
    hello.m_platform = 2;
    hello.m_device[0] = 7;
    CHECK_TEST(client.bindChannel(&channel) == NI_OK && client.connect("localhost", 9001, tls, hello, &events) == NI_OK &&
        client.poll() && client.ready());

    uint64_t local[4] = {};
    uint64_t wire[4] = {};
    for (uint32_t i = 0; i < 4; ++i)
    {
        CHECK_TEST(client.beginRequest(7, 10 + i, 60000, local[i]) == NI_OK && local[i] && (!i || local[i] > local[i - 1]));
    }
    auto send = [&](uint32_t i) -> bool {
        const uint8_t marker = static_cast<uint8_t>(10 + i);
        if (client.sendRequest(local[i], &marker, 1) != NI_OK || channel.m_body.size() != 1 || channel.m_body[0] != marker) { return false; }
        wire[i] = channel.m_lastRequest;
        return true;
    };
    auto reply = [&](uint32_t i) -> bool {
        const uint8_t marker = static_cast<uint8_t>(40 + i);
        uint8_t envelope[17];
        uint32_t size = 0;
        if (NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, &marker, 1, envelope, sizeof(envelope), size) != NI_OK) { return false; }
        const NetInterfaceFrame response = {7, wire[i], 10 + i, NI_RESPONSE, size, envelope};
        channel.m_event = &response;
        return client.poll() && client.ready() && !channel.m_event;
    };
    CHECK_TEST(send(1));
    channel.m_blockRequest = true;
    CHECK_TEST(client.sendRequest(local[0], nullptr, 0) == NI_LIMIT_EXCEEDED && channel.m_acceptedRequests == 1 && !events.m_results);
    channel.m_blockRequest = false;
    CHECK_TEST(send(2) && wire[2] > wire[1] && reply(2) && events.m_results == 1 && events.m_resultId == local[2] &&
        events.m_resultStatus == NI_RPC_SUCCESS && events.m_resultByte == 42);
    CHECK_TEST(send(0) && wire[0] > wire[2] && channel.m_acceptedRequests == 3 &&
        client.sendRequest(local[0], nullptr, 0) == NI_INVALID_STATE && reply(0) && events.m_results == 2 &&
        events.m_resultId == local[0] && events.m_resultByte == 40 && reply(0) && events.m_results == 2);
    CHECK_TEST(reply(1) && events.m_results == 3 && events.m_resultId == local[1] && events.m_resultByte == 41);
    CHECK_TEST(send(3) && wire[3] > wire[0] && client.cancel(local[3]) == NI_OK && channel.m_acceptedCancels == 1 &&
        channel.m_lastCancel == wire[3] && events.m_results == 4 && events.m_resultId == local[3] &&
        events.m_resultStatus == NI_RPC_CANCELLED && reply(3) && events.m_results == 4);

    const auto start = TimerProbeNow();
    const uint64_t now = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(start.time_since_epoch()).count());
    const uint32_t elapsed = static_cast<uint32_t>((std::min)(now, uint64_t(10000)));
    CHECK_TEST(client.beginRequestAt(100, 7, 20, now - elapsed, elapsed + 200) == NI_OK &&
        client.beginRequestAt(101, 7, 21, now, 60000) == NI_OK && client.beginRequestAt(102, 7, 22, now, 60000) == NI_OK);
    CHECK_TEST(client.sendRequest(102, nullptr, 0) == NI_OK && channel.m_lastRequest == 102 &&
        client.sendRequest(101, nullptr, 0) == NI_OK && channel.m_lastRequest > 102);
    const uint64_t closingWire = channel.m_lastRequest;
    CHECK_TEST(client.sendRequest(100, nullptr, 0) == NI_OK && channel.m_lastRequest > closingWire && channel.m_requestTimeout <= 200);
    const uint64_t expiringWire = channel.m_lastRequest;
    CHECK_TEST(client.cancel(102) == NI_OK && channel.m_lastCancel == 102 && events.m_resultId == 102 && events.m_results == 5);
    WaitTimerProbe(start + std::chrono::milliseconds(201));
    CHECK_TEST(client.poll() && events.m_timeouts == 1 && events.m_resultId == 100 && events.m_results == 6 &&
        events.m_resultStatus == NI_RPC_TIMEOUT && channel.m_acceptedCancels == 3 && channel.m_lastCancel == expiringWire);
    client.shutdown();
    CHECK_TEST(events.m_closes == 1 && events.m_resultsAtClose == 7 && events.m_resultId == 101 &&
        events.m_resultStatus == NI_RPC_CANCELLED && events.m_results == 7);
    return 0;
}

/** 在 API 调用和运输帧回调中触发关闭异常，核对清理顺序、请求结果及首次关闭原因
@return 三种收尾路径均只停止一次并交付唯一终态时为0
*/
static int CheckCloseException()
{
    for (uint32_t mode = 0; mode < 3; ++mode)
    {
        NetInterfaceHandshakeOptions hello = {};
        hello.m_server = hello.m_capabilities = hello.m_maxRequests = 1;
        hello.m_maxChunk = 1024;
        hello.m_timeoutMs = 5000;
        hello.m_nonce[0] = 13;
        NetInterfaceHandshake server(hello, 0);
        SessionProbeChannel channel(server);
        CiphertextProbeHost events;
        NetSession session;
        uint32_t seed = 27;
        NetInterfaceTlsOptions tls = {};
        tls.entropy = HelloEntropy;
        tls.entropyContext = &seed;
        hello.m_server = 0;
        hello.m_platform = 2;
        hello.m_device[0] = 7;
        uint64_t request = 0;
        CHECK_TEST(session.bindChannel(&channel) == NI_OK &&
            session.connect("localhost", 9001, tls, hello, &events) == NI_OK && session.poll() && session.ready() &&
            session.beginRequest(7, 9, 60000, request) == NI_OK && request != 0);
        channel.m_throwClose = true;
        channel.m_throwStop = mode == 2;
        const int32_t reason = mode == 0 ? NI_CANCELLED : mode == 1 ? NI_INVALID_STATE : NI_OK;
        if (mode == 1)
        {
            const uint8_t body[] = {19, 31};
            const NetInterfaceFrame frame = {7, 99, 11, NI_EVENT, sizeof(body), body};
            channel.m_event = &frame;
            events.m_throwFrame = true;
            CHECK_TEST(!session.poll() && events.m_frames == 1 && !channel.m_stoppedInFrame);
        }
        else
        {
            session.close(reason);
            session.close(NI_TIMEOUT);
            CHECK_TEST(!session.ready() && !events.m_closes && !events.m_results && !channel.m_stops);
            CHECK_TEST(session.bindChannel(nullptr) == NI_INVALID_STATE &&
                session.configureCompression(nullptr, 0) == NI_INVALID_STATE &&
                session.configureRequestPool(LockFreeHashMapPoolConfig()) == NI_INVALID_STATE &&
                session.configureExpiryBudget(1) == NI_INVALID_STATE && session.configurePayloadRetention(-1) == NI_INVALID_STATE &&
                session.configureHeartbeat(NetHeartbeatConfig()) == NI_INVALID_STATE);
            if (mode == 2)
            {
                session.shutdown();
            }
            else
            {
                CHECK_TEST(!session.poll());
            }
        }
        CHECK_TEST(channel.m_closeCalls == 1 && channel.m_stops == 1 && !channel.m_stoppedInFrame &&
            events.m_closed == reason && events.m_closes == 1 && events.m_results == 1 && events.m_resultsAtClose == 1 &&
            events.m_resultId == request &&
            events.m_resultStatus == (mode == 1 ? NI_RPC_UNAVAILABLE : NI_RPC_CANCELLED));
        session.shutdown();
        CHECK_TEST(!session.poll() && channel.m_stops == 1 && events.m_closes == 1 && events.m_results == 1 &&
            session.configureCompression(nullptr, 0) == NI_OK && session.configureExpiryBudget(1) == NI_INVALID_STATE);
    }
    return 0;
}

/** 核对默认、丢弃及无限保留三种会话，并验证心跳在无待办时仍能唤醒宿主
@return 全部通过为0
*/
static int SessionOptions()
{
    if (CheckSessionRequestOrder()) { return 1; }
    return WithWaitProbe([](const uint32_t& waited) -> int {
        if (CheckCloseException() || CheckSessionOptions(4096, waited) || CheckSessionOptions(0, waited)) { return 1; }
        if (CheckSessionOptions(-1, waited) || CheckSessionOptions(-1, waited, 1)) { return 1; }
        return CheckSessionOptions(-1, waited, 2);
    });
}

PendingDiscovery::PendingDiscovery() :
m_startCalls(0),
m_fault(DiscoveryFault::None),
m_allocationFailure(false),
m_provide(false),
m_headersValid(true),
m_holdStop(false),
m_failures(0),
m_attempts(0),
m_running(false)
{

}

void PendingDiscovery::checkFault(DiscoveryFault operation) const
{
    if (m_fault == operation)
    {
        if (m_allocationFailure) { throw std::bad_alloc(); }
        throw std::runtime_error("discovery unavailable");
    }
}

int32_t PendingDiscovery::start(const std::vector<NetGatewayAddress>& seeds, const std::string& cachePath, const NetInterfaceTlsOptions&)
{
    ++m_startCalls;
    m_running = true;
    checkFault(DiscoveryFault::Start);
    m_startedSeeds = seeds;
    m_startedPath = cachePath;
    return NI_OK;
}

bool PendingDiscovery::poll()
{
    checkFault(DiscoveryFault::Poll);
    return m_running;
}

void PendingDiscovery::shutdown()
{
    if (!m_holdStop) { m_running = false; }
}

bool PendingDiscovery::stopped() const
{
    return !m_running;
}

bool PendingDiscovery::choose(NetClientDiscoveryPoint& endpoint)
{
    ++m_attempts;
    m_headersValid = m_headersValid && endpoint.m_structSize == sizeof(endpoint) && endpoint.m_version == 1;
    checkFault(DiscoveryFault::Choose);
    if (!m_provide || !m_headersValid)
    {
        endpoint = NetClientDiscoveryPoint();
        return false;
    }
    std::memcpy(endpoint.m_address, "127.0.0.2", sizeof("127.0.0.2"));
    endpoint.m_port = 8001;
    return true;
}

void PendingDiscovery::failed(const NetClientDiscoveryPoint& endpoint)
{
    if (endpoint.m_port == 8001) { ++m_failures; }
    checkFault(DiscoveryFault::Failed);
}

FrameDeliveryProbe::FrameDeliveryProbe() :
m_accept(false),
m_reason(NI_OK),
m_valid(true),
m_calls(0)
{

}

void FrameDeliveryProbe::onReady(uint64_t)
{

}

bool FrameDeliveryProbe::onFrame(uint64_t peer, const NetInterfaceFrame& frame)
{
    ++m_calls;
    m_valid = m_valid && peer == 1 && frame.m_kind == NI_DATA && frame.m_productId == 7 &&
        frame.m_requestId == 5 && frame.m_messageType == 11 && frame.m_payloadSize == 2 &&
        frame.m_payload && frame.m_payload[0] == 0 && frame.m_payload[1] == 98;
    return m_accept;
}

void FrameDeliveryProbe::onClosed(uint64_t, int32_t reason, int32_t)
{
    m_reason = reason;
}

/** 使用独立的帧观察者，核对真实交付策略在零时刻、重试、过期、接纳时的行为
@param [in] waited 事件宿主观测的等待预算
@return 有限期限不续期、接纳后复位且到期不再交付时为0
*/
static int FrameBackpressure(const uint32_t& waited)
{
    NetGatewayClientTiming timing;
    FrameDeliveryProbe listener;
    const uint8_t body[] = {0, 98};
    const NetInterfaceFrame frame = {7, 5, 11, NI_DATA, 2, body};
    NetExecution owner;
    NetExecutionScope scope(owner);
    const uint32_t readBudget[] = {32, 1, 4};
    const uint32_t writeBudget[] = {32, 3, 2};
    for (unsigned mode = 0; mode < 3; ++mode)
    {
        if (mode) { CHECK_TEST(timing.configurePollBudgets(readBudget[mode], writeBudget[mode]) == NI_OK); }
        CHECK_TEST(timing.configurePollBudgets(0, 8) == NI_INVALID_ARGUMENT &&
            timing.configurePollBudgets(8, 0) == NI_INVALID_ARGUMENT);
        uint32_t reads = 0, writes = 0;
        uint64_t deadline = 0;
        listener.m_accept = true;
        const uint32_t previous = listener.m_calls;
        owner.begin();
        timing.readFrames([&](uint32_t index) -> bool {
            if (index != reads || timing.deliver(listener, frame, 2, deadline) != NI_OK) { return false; }
            ++reads;
            return true;
        });
        CHECK_TEST(listener.m_valid && listener.m_calls - previous == readBudget[mode] &&
            reads == readBudget[mode] && owner.wait(1000) == NI_OK && waited == 0);
        owner.begin();
        timing.writeRecords([&](uint32_t index) -> bool {
            if (index != writes) { return false; }
            ++writes;
            return true;
        });
        CHECK_TEST(writes == writeBudget[mode] && owner.wait(1000) == NI_OK && waited == 0);
        reads = writes = 0;
        owner.begin();
        timing.readFrames([&](uint32_t) -> bool { ++reads; return false; });
        timing.writeRecords([&](uint32_t) -> bool { ++writes; return false; });
        CHECK_TEST(reads == 1 && writes == 1 && owner.wait(1000) == NI_OK && waited == 1000);
    }
    for (unsigned pass = 0; pass < 2; ++pass)
    {
        const uint32_t expected = pass ? 750 : 5000;
        if (pass) { CHECK_TEST(timing.configure(123, 750) == NI_OK); }
        CHECK_TEST(timing.configure(222, 0) == NI_INVALID_ARGUMENT);
        uint64_t deadline = 0;
        listener.m_accept = false;
        const uint32_t count = listener.m_calls;
        owner.begin();
        const auto start = TimerProbeNow();
        CHECK_TEST(timing.deliver(listener, frame, 0, deadline) == NI_LIMIT_EXCEEDED && deadline == expected &&
            owner.wait(60000) == NI_OK);
        const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            TimerProbeNow() - start).count());
        CHECK_TEST(waited <= expected && waited + elapsed + 2 >= expected);
        CHECK_TEST(timing.deliver(listener, frame, expected - 1, deadline) == NI_LIMIT_EXCEEDED && deadline == expected);
        listener.m_accept = true;
        CHECK_TEST(timing.deliver(listener, frame, expected, deadline) == NI_TIMEOUT && listener.m_calls == count + 2);
        deadline = 0;
        listener.m_accept = false;
        CHECK_TEST(timing.deliver(listener, frame, 10, deadline) == NI_LIMIT_EXCEEDED && deadline == 10 + expected);
        listener.m_accept = true;
        CHECK_TEST(timing.deliver(listener, frame, 11, deadline) == NI_OK && deadline == 0 && listener.m_valid);
    }
    CHECK_TEST(timing.configure(0, UINT32_MAX) == NI_OK);
    uint64_t deadline = 0;
    const uint32_t calls = listener.m_calls;
    listener.m_accept = false;
    CHECK_TEST(timing.deliver(listener, frame, UINT64_MAX - 3, deadline) == NI_LIMIT_EXCEEDED && deadline == UINT64_MAX);
    listener.m_accept = true;
    CHECK_TEST(timing.deliver(listener, frame, UINT64_MAX, deadline) == NI_TIMEOUT && listener.m_calls == calls + 1);
    return 0;
}

/** 固定退避抖动为零，握手随机数仍使用本用例的独立确定性数据
@param [in,out] context 用例种子
@param [out] output 请求的测试字节
@param [in] size 字节数，四字节请求是重试抖动
@return 成功为0
*/
static int32_t BackoffEntropy(void* context, uint8_t* output, uint32_t size)
{
    if (size != sizeof(uint32_t)) { return HelloEntropy(context, output, size); }
    std::memset(output, 0, size);
    return 0;
}

/** 连续两次物理连接失败时核对已设置的退避上限
@param [in] waited 宿主收到的等待时间
@return 默认抖动已固定时两个期限均符合预期则为0
*/
static int RecoveryBackoff(const uint32_t& waited)
{
    CiphertextProbeHost host;
    host.m_refuseConnect = true;
    NetClientSession session;
    uint32_t seed = 17;
    NetInterfaceTlsOptions tls = {};
    tls.entropy = BackoffEntropy;
    tls.entropyContext = &seed;
    NetInterfaceHandshakeOptions hello = {};
    hello.m_platform = hello.m_capabilities = hello.m_maxRequests = 1;
    hello.m_maxChunk = 1024;
    hello.m_timeoutMs = 1000;
    hello.m_device[0] = 2;
    NetExecution owner;
    NetExecutionScope scope(owner);
    NetClientDiscoveryPoint unterminated = {};
    std::memset(unterminated.m_address, '1', sizeof(unterminated.m_address));
    CHECK_TEST(session.connectService(7, unterminated.m_address, 8000, tls, hello, &host, &host) == NI_INVALID_ARGUMENT &&
        session.generation() == 0 && host.m_closes == 0);
    CHECK_TEST(session.configureRecovery(90000, 32, 40) == NI_OK &&
        session.connectService(7, "127.0.0.1", 8000, tls, hello, &host, &host) == NI_OK);
    for (unsigned step = 1; step <= 2; ++step)
    {
        const auto watchdog = TimerProbeNow() + std::chrono::seconds(2);
        while (host.m_closes < step && TimerProbeNow() < watchdog)
        {
            owner.begin();
            const auto start = TimerProbeNow();
            CHECK_TEST(session.poll());
            if (host.m_closes != step)
            {
                YieldTimerProbe();
                continue;
            }
            CHECK_TEST(host.m_closed == NI_PROTOCOL_ERROR && owner.wait(120000) == NI_OK);
            const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                TimerProbeNow() - start).count());
            const uint32_t expected = step == 1 ? 16 : 20;
            CHECK_TEST(waited <= expected && waited + elapsed + 2 >= expected);
        }
        CHECK_TEST(host.m_closes == step);
    }
    session.shutdown();
    return 0;
}

/** 验证握手前关闭的自动恢复代次也能经实际分派器到达业务监听者
@param [in] timeout true 让总期限早于下一次重试，false 连续拒绝两次运输连接
@return 逐代通知、提交端状态和过期回调判定均正确时为0
*/
static int DispatchRecovery(bool timeout)
{
    CiphertextProbeHost host;
    host.m_refuseConnect = true;
    NetClientSession session;
    NetSenderDispatch dispatch(&session);
    uint32_t seed = 17;
    NetInterfaceTlsOptions tls = {};
    tls.entropy = BackoffEntropy;
    tls.entropyContext = &seed;
    NetInterfaceHandshakeOptions hello = {};
    hello.m_platform = hello.m_capabilities = hello.m_maxRequests = 1;
    hello.m_maxChunk = 1024;
    hello.m_timeoutMs = 1000;
    hello.m_device[0] = 2;
    const auto stop = [](NetSenderDispatch* active) { active->shutdown(); };
    std::unique_ptr<NetSenderDispatch, decltype(stop)> lifetime(&dispatch, stop);
    CHECK_TEST(session.configureRecovery(timeout ? 100 : 60000, timeout ? 5000 : 32, timeout ? 5000 : 40) == NI_OK &&
        dispatch.prepare(&host) == NI_OK &&
        session.connectService(7, "127.0.0.1", 8000, tls, hello, dispatch.listener(), &host) == NI_OK &&
        dispatch.started(session.generation(), 7) == NI_OK);
    for (uint64_t expected = 1; expected <= 2; ++expected)
    {
        const auto watchdog = TimerProbeNow() + std::chrono::seconds(3);
        while (host.m_closes < expected && TimerProbeNow() < watchdog)
        {
            dispatch.poll();
            if (host.m_closes < expected) { YieldTimerProbe(); }
        }
        uint64_t current = 0;
        bool ready = true;
        dispatch.submission().state(current, ready);
        CHECK_TEST(host.m_closes == expected && host.m_closedGeneration == expected &&
            host.m_closed == (timeout && expected == 2 ? NI_TIMEOUT : NI_PROTOCOL_ERROR) && current == expected && !ready);
        dispatch.listener()->onSessionClosed(expected - 1, NI_CANCELLED, 0);
        dispatch.listener()->onSessionClosed(expected, NI_CANCELLED, 0);
        dispatch.listener()->onSessionClosed(expected + 1, NI_CANCELLED, 0);
        CHECK_TEST(host.m_closes == expected);
    }
    lifetime.reset();
    CHECK_TEST(host.m_closes == 2);
    return 0;
}

/** 从发现宿主读取实际生效的种子与缓存配置，检查默认入口和显式 IPC 入口
@return 接纳、拒绝、配置副本和生命周期均符合预期时返回 0
*/
static int DiscoverySettings()
{
    CiphertextProbeHost host;
    CiphertextProbeHost ipcFactory;
    PendingDiscovery discovery;
    NetClientSession session;
    uint32_t seed = 17;
    NetInterfaceTlsOptions tls = {};
    tls.entropy = HelloEntropy;
    tls.entropyContext = &seed;
    NetInterfaceHandshakeOptions hello = {};
    hello.m_platform = hello.m_capabilities = hello.m_maxRequests = 1;
    hello.m_maxChunk = 1024;
    hello.m_timeoutMs = 1000;
    hello.m_device[0] = 2;
    {
        NetInterfaceHandshakeOptions directHello = hello;
        directHello.m_server = 1;
        directHello.m_nonce[0] = 1;
        NetInterfaceHandshake directServer(directHello, 0);
        SessionProbeChannel direct(directServer);
        NetClientSession bindings;
        CHECK_TEST(bindings.bindChannel(&direct) == NI_OK &&
            bindings.bindGateway(&host, &discovery) == NI_OK &&
            bindings.connect("127.0.0.1", 8000, tls, hello, &host) == NI_NOT_SUPPORTED &&
            bindings.bindChannel(&direct) == NI_OK && bindings.bindGateway(nullptr) == NI_OK &&
            bindings.connect("127.0.0.1", 8000, tls, hello, &host) == NI_NOT_SUPPORTED);
    }
    CHECK_TEST(session.bindGateway(nullptr, &discovery) == NI_INVALID_ARGUMENT &&
        session.bindGateway(&host) == NI_OK && session.configureGateways("", "path") == NI_OK &&
        session.connectService(7, "127.0.0.1", 8000, tls, hello, &host) == NI_NOT_SUPPORTED &&
        session.configureGateways("127.0.0.2:81", "") == NI_OK &&
        session.connectService(7, "127.0.0.1", 8000, tls, hello, &host) == NI_NOT_SUPPORTED &&
        session.generation() == 0 && session.configureGateways("", "") == NI_OK &&
        session.bindGateway(&host, &discovery) == NI_OK);
    const char* invalid[] = {"example.test", "0.0.0.0", "::"};
    for (const char* address : invalid)
    {
        CHECK_TEST(session.connectService(7, address, 8000, tls, hello, &host) == NI_INVALID_ARGUMENT &&
            session.generation() == 0 && discovery.m_startCalls == 0);
    }
    std::string seeds = "127.0.0.2:81,[::1]:82,[0:0:0:0:0:0:0:1]:82";
    std::string path(4096, 'q');
    CHECK_TEST(session.configureGateways(seeds, path) == NI_OK);
    seeds.clear();
    path.assign(4097, 'x');
    CHECK_TEST(session.configureGateways("127.0.0.3:83", path) == NI_INVALID_ARGUMENT &&
        session.configureGateways("127.0.0.3:0", "") == NI_INVALID_ARGUMENT &&
        session.configureGateways("", std::string("a\0b", 3)) == NI_INVALID_ARGUMENT);
    CHECK_TEST(session.connectService(7, "127.0.0.1", 8000, tls, hello, &host) == NI_OK &&
        discovery.m_startCalls == 1 && discovery.m_startedPath == std::string(4096, 'q') &&
        discovery.m_startedSeeds.size() == 2 && discovery.m_startedSeeds[0].m_address == "127.0.0.2" &&
        discovery.m_startedSeeds[0].m_port == 81 && discovery.m_startedSeeds[1].m_address == "::1" &&
        discovery.m_startedSeeds[1].m_port == 82 && session.configureGateways("", "") == NI_INVALID_STATE);
    session.shutdown();
    CHECK_TEST(discovery.stopped() && session.configureGateways("", "") == NI_OK &&
        session.connectService(7, "::1", 9000, tls, hello, &host) == NI_OK && discovery.m_startCalls == 2 &&
        discovery.m_startedPath.empty() && discovery.m_startedSeeds.size() == 1 &&
        discovery.m_startedSeeds[0].m_address == "::1" && discovery.m_startedSeeds[0].m_port == 9000);
    session.shutdown();
    CHECK_TEST(session.connectService(7, "local-ipc", 1, tls, hello, &host, &ipcFactory) == NI_OK &&
        session.poll() && discovery.m_startCalls == 2 && host.m_connectedAddress.empty() &&
        ipcFactory.m_connectedAddress == "local-ipc" && ipcFactory.m_connectedPort == 1);
    session.shutdown();
    CHECK_TEST(session.bindGateway(nullptr) == NI_OK &&
        session.connectService(7, "127.0.0.1", 8000, tls, hello, &host) == NI_NOT_SUPPORTED);
    return 0;
}

/** 从实际服务连接入口验证发现器异常的分类、停止和重试
@return 每个已接纳代次只报告一次预期错误时为0
*/
static int DiscoveryErrors()
{
    const DiscoveryFault operations[] = {DiscoveryFault::Start, DiscoveryFault::Poll,
        DiscoveryFault::Choose, DiscoveryFault::Failed};
    for (const auto operation : operations)
    {
        for (unsigned allocation = 0; allocation < 2; ++allocation)
        {
            CiphertextProbeHost host;
            PendingDiscovery discovery;
            NetClientSession session;
            uint32_t seed = 17;
            NetInterfaceTlsOptions tls = {};
            tls.entropy = HelloEntropy;
            tls.entropyContext = &seed;
            NetInterfaceHandshakeOptions hello = {};
            hello.m_platform = hello.m_capabilities = hello.m_maxRequests = 1;
            hello.m_maxChunk = 1024;
            hello.m_timeoutMs = 1000;
            hello.m_device[0] = 2;
            host.m_refuseConnect = true;
            // Poll 的异常安排在运输已接纳以后，并让发现器暂时无法停止。
            discovery.m_fault = operation == DiscoveryFault::Poll ? DiscoveryFault::None : operation;
            discovery.m_provide = true;
            discovery.m_allocationFailure = allocation != 0;
            const int32_t error = allocation ? NI_NO_MEMORY : NI_INVALID_STATE;
            CHECK_TEST(session.bindGateway(&host, &discovery) == NI_OK);
            const int32_t result = session.connectService(7, "127.0.0.1", 8000, tls, hello, &host);
            if (operation == DiscoveryFault::Start)
            {
                CHECK_TEST(result == error && session.generation() == 0 && session.quiescent() && host.m_closes == 0);
                discovery.m_fault = DiscoveryFault::None;
                CHECK_TEST(session.connectService(7, "127.0.0.1", 8000, tls, hello, &host) == NI_OK);
                session.shutdown();
                CHECK_TEST(host.m_closes == 1 && session.quiescent());
                continue;
            }
            CHECK_TEST(result == NI_OK);
            if (operation == DiscoveryFault::Poll)
            {
                host.m_refuseConnect = false;
                CHECK_TEST(session.poll() && discovery.m_attempts == 1 && host.m_closes == 0);
                discovery.m_holdStop = true;
                discovery.m_fault = operation;
            }
            for (unsigned i = 0; i < 8 && host.m_closes == 0; ++i) { session.poll(); }
            CHECK_TEST(host.m_closes == 1 && host.m_closed == error && discovery.m_headersValid);
            CHECK_TEST(discovery.m_failures == (operation == DiscoveryFault::Failed ? 1U : 0U));
            if (operation == DiscoveryFault::Poll)
            {
                CHECK_TEST(!session.quiescent() && session.configureGateways("", "pending") == NI_INVALID_STATE &&
                    session.configureCompression(nullptr, 0) == NI_INVALID_STATE);
            }
            discovery.m_fault = DiscoveryFault::None;
            discovery.m_holdStop = false;
            session.shutdown();
            CHECK_TEST(!session.poll() && session.quiescent() && host.m_closes == 1 &&
                session.configureGateways("", "released") == NI_OK && session.configureCompression(nullptr, 0) == NI_OK);
        }
    }
    return 0;
}

/** 无候选时核对实际宿主等待预算，分别让重试和总期限先到
@return 全部通过为0
*/
static int RecoveryWait()
{
    return WithWaitProbe([](const uint32_t& waited) -> int {
        CHECK_TEST(DiscoverySettings() == 0 && FrameBackpressure(waited) == 0 && RecoveryBackoff(waited) == 0);
        CHECK_TEST(DispatchRecovery(false) == 0 && DispatchRecovery(true) == 0);
        const uint32_t expected[] = {250, 1400, 700};
        for (unsigned i = 0; i < 3; ++i)
        {
            CiphertextProbeHost host;
            PendingDiscovery discovery;
            NetClientSession session;
            if (i != 0) { CHECK_TEST(session.configureRecovery(i == 1 ? 8000 : 700, 1400, 4000) == NI_OK); }
            CHECK_TEST(session.configureRecovery(0, 1, 3) == NI_INVALID_ARGUMENT &&
                session.configureRecovery(5, 0, 3) == NI_INVALID_ARGUMENT && session.configureRecovery(5, 4, 3) == NI_INVALID_ARGUMENT);
            NetInterfaceTlsOptions tls = {};
            tls.entropy = CheckStartEntropy;
            tls.entropyContext = &session;
            NetInterfaceHandshakeOptions options = {};
            options.m_platform = options.m_capabilities = options.m_maxRequests = 1;
            options.m_maxChunk = 1024;
            options.m_timeoutMs = 1000;
            options.m_device[0] = 1;
            LockFreeHashMapPoolConfig requestPool;
            requestPool.initialCapacity = 1;
            requestPool.maximumCapacity = 2;
            CHECK_TEST(session.configureRequestPool(requestPool) == NI_OK);
            CHECK_TEST(session.bindGateway(&host, &discovery) == NI_OK);
            NetExecution owner;
            NetExecutionScope scope(owner);
            owner.begin();
            const auto start = TimerProbeNow();
            CHECK_TEST(session.connectService(7, "127.0.0.1", 8000, tls, options, &host) == NI_OK);
            CHECK_TEST(session.poll() && discovery.m_attempts == 1 && owner.wait(120000) == NI_OK);
            const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                TimerProbeNow() - start).count());
            CHECK_TEST(waited <= expected[i] && elapsed + waited + 2 >= expected[i]);
            if (i == 0)
            {
                discovery.m_provide = true;
                const auto watchdog = TimerProbeNow() + std::chrono::seconds(3);
                while (discovery.m_attempts < 2 && TimerProbeNow() < watchdog)
                {
                    CHECK_TEST(session.poll());
                    YieldTimerProbe();
                }
                CHECK_TEST(discovery.m_headersValid && discovery.m_attempts == 2);
                CHECK_TEST(session.poll() && host.m_offered != 0 && host.m_closes == 0 &&
                    host.m_connectedAddress == "127.0.0.2" && host.m_connectedPort == 8001);
            }
            CHECK_TEST(session.configureRecovery(50, 1, 2) == NI_INVALID_STATE);
            session.shutdown();
            CHECK_TEST(discovery.stopped());
            // 已建立服务连接代次，即使发现一直没有候选、内部协议会话尚未启动，池也必须冻结。
            CHECK_TEST(session.generation() != 0 && session.configureRequestPool(requestPool) == NI_INVALID_STATE);
        }
        for (unsigned i = 0; i < 2; ++i)
        {
            CiphertextProbeHost host;
            NetClientSession session;
            uint32_t seed = 17;
            NetInterfaceTlsOptions tls = {};
            tls.entropy = HelloEntropy;
            tls.entropyContext = &seed;
            NetInterfaceHandshakeOptions hello = {};
            hello.m_platform = hello.m_capabilities = hello.m_maxRequests = 1;
            hello.m_maxChunk = 1024;
            hello.m_timeoutMs = 1000;
            hello.m_device[0] = 2;
            if (i) { CHECK_TEST(session.configureServiceTimeouts(2100, 8000) == NI_OK); }
            CHECK_TEST(session.configureServiceTimeouts(50, 0) == NI_INVALID_ARGUMENT &&
                session.configureRecovery(90000, 250, 5000) == NI_OK);
            NetExecution owner;
            NetExecutionScope scope(owner);
            const auto start = TimerProbeNow();
            CHECK_TEST(session.connectService(7, "127.0.0.1", 8000, tls, hello, &host, &host) == NI_OK);
            owner.begin();
            CHECK_TEST(session.poll() && owner.wait(120000) == NI_OK && waited == 0);
            owner.begin();
            CHECK_TEST(session.poll() && host.m_offered != 0 && owner.wait(120000) == NI_OK);
            const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                TimerProbeNow() - start).count());
            const uint32_t limit = i ? 2100 : 30000;
            CHECK_TEST(waited <= limit && waited + elapsed + 2 >= limit &&
                session.configureServiceTimeouts(100, 500) == NI_INVALID_STATE);
            session.shutdown();
        }
        return DiscoveryErrors();
    });
}

/** 用真实路由器分别协商默认及自定义值，不等待墙钟流逝
@return 默认、独立配置和外部附加模式均符合契约时为0
*/
static int RouterHandshake()
{
    NetHeartbeatConfig heartbeat;
    CHECK_TEST(!heartbeat.expired(0, 0) && !heartbeat.expired(heartbeat.m_receiveTimeoutMs - 1, heartbeat.m_pongTimeoutMs - 1) &&
        heartbeat.expired(heartbeat.m_receiveTimeoutMs, 0) && heartbeat.expired(0, heartbeat.m_pongTimeoutMs) &&
        heartbeat.expired(UINT64_MAX, UINT64_MAX));
    const int32_t reasons[] = {NI_TRUNCATED, NI_INVALID_STATE, NI_NO_MEMORY};
    for (unsigned fault = 1; fault <= 5; ++fault)
    {
        NetInterfaceHandshakeOptions hello = {};
        hello.m_platform = hello.m_capabilities = hello.m_maxRequests = 1;
        hello.m_maxChunk = 1024;
        hello.m_timeoutMs = 5000;
        hello.m_device[0] = hello.m_nonce[0] = 1;
        NetInterfaceHandshake client(hello, 0);
        RouterHandshakeTransport transport(client);
        NetEndpointRouter router(nullptr);
        transport.m_router = &router;
        transport.m_pingFailure = fault;
        NetEndpointPools pools;
        pools.m_heartbeat.m_pingIntervalMs = fault == 5 ? 40 : 1;
        pools.m_heartbeat.m_pongTimeoutMs = fault == 4 ? 60 : 800;
        pools.m_heartbeat.m_receiveTimeoutMs = fault == 5 ? 100 : 60000;
        uint32_t seed = 11;
        NetInterfaceTlsOptions tls = {};
        tls.server = 1;
        tls.entropy = HelloEntropy;
        tls.entropyContext = &seed;
        CHECK_TEST(router.bindTransport(&transport) == NI_OK && router.configurePools(pools) == NI_OK &&
            router.listen("127.0.0.1", 8081, tls, 1) == NI_OK && router.poll());
        if (fault == 5)
        {
            transport.m_deferHello = true;
            WaitTimerProbe(TimerProbeNow() + std::chrono::milliseconds(101));
            CHECK_TEST(router.poll() && transport.m_result == NI_NEED_MORE);
            transport.m_deferHello = false;
            transport.m_deferReply = true;
            CHECK_TEST(router.poll() && transport.m_result == NI_NEED_MORE && transport.m_pingId == 0);
            WaitTimerProbe(TimerProbeNow() + std::chrono::milliseconds(101));
            CHECK_TEST(router.poll() && transport.m_result == NI_NEED_MORE && transport.m_pingId == 0);
            transport.m_deferReply = false;
        }
        const int ready = WithWaitProbe([&](const uint32_t& waited) -> int {
            NetExecution owner;
            NetExecutionScope scope(owner);
            owner.begin();
            const auto start = TimerProbeNow();
            CHECK_TEST(router.poll() && transport.m_result == NI_READY && transport.m_pingId == 0 &&
                owner.wait(5000) == NI_OK);
            const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                TimerProbeNow() - start).count());
            CHECK_TEST(waited <= pools.m_heartbeat.m_pingIntervalMs &&
                waited + elapsed + 2 >= pools.m_heartbeat.m_pingIntervalMs);
            return 0;
        });
        CHECK_TEST(ready == 0);
        if (fault == 5)
        {
            // 保持无输入至接收期限，再交付 PING 并验证未向过期连接回复 PONG。
            transport.m_activityInput = true;
            WaitTimerProbe(TimerProbeNow() + std::chrono::milliseconds(pools.m_heartbeat.m_receiveTimeoutMs));
            CHECK_TEST(router.poll() && transport.m_result == NI_TIMEOUT && transport.m_activityInput);
            continue;
        }
        WaitTimerProbe(TimerProbeNow() + std::chrono::milliseconds(pools.m_heartbeat.m_pingIntervalMs));
        CHECK_TEST(router.poll() && transport.m_pingId != 0);
        if (fault <= 3) { CHECK_TEST(transport.m_result == reasons[fault - 1]); }
        else
        {
            transport.m_pongDue = true;
            WaitTimerProbe(TimerProbeNow() + std::chrono::milliseconds(pools.m_heartbeat.m_pongTimeoutMs));
            CHECK_TEST(router.poll() && transport.m_result == NI_TIMEOUT);
        }
    }
    const int result = WithWaitProbe([](const uint32_t& waited) -> int {
        return CheckRouterHandshake(false, waited) == 0 ? CheckRouterHandshake(true, waited) : 1;
    });
    CHECK_TEST(result == 0);
    PoolTestSink clients;
    NetEndpointRouter attached(nullptr, &clients);
    CHECK_TEST(attached.configureClientHandshake(1, 1, 1024, 100) == NI_INVALID_STATE);
    return 0;
}

RouterFileSink::RouterFileSink() :
m_request(0),
m_requestResult(NI_OK),
m_cancels(0),
m_lastCancel(0),
m_stopOnRequest(nullptr),
m_stream(0),
m_expiresUtc(0),
m_events(0)
{

}

int32_t RouterFileSink::send(uint64_t, const NetInterfaceFrame& frame, const NetEndpointOrigin&)
{
    if (frame.m_kind == NI_EVENT && frame.m_messageType == 72)
    {
        if (frame.m_productId != 7 || !frame.m_requestId || frame.m_payloadSize != 2 ||
            !frame.m_payload || frame.m_payload[0] != 0 || frame.m_payload[1] != 81) { return NI_PROTOCOL_ERROR; }
        ++m_events;
    }
    else if (frame.m_kind == NI_EVENT)
    {
        NetInterfaceFileNotice notice = {};
        if (NetInterfaceFile::parseNotice(frame, notice) != NI_OK) { return NI_PROTOCOL_ERROR; }
        m_notices.push_back(frame.m_requestId);
    }
    else if (frame.m_kind == NI_RESPONSE && frame.m_messageType == 711)
    {
        NetInterfaceRpcView response = {};
        if (NetInterfaceRpc::parse(frame, response) != NI_OK || response.m_value != NI_RPC_TIMEOUT) { return NI_PROTOCOL_ERROR; }
        m_timeoutIds.push_back(frame.m_requestId);
    }
    else if (frame.m_kind == NI_RESPONSE && frame.m_messageType == NI_FILE_OPEN_DOWNLOAD)
    {
        NetInterfaceRpcView rpc = {};
        NetInterfaceFileInfo info = {};
        if (NetInterfaceRpc::parse(frame, rpc) != NI_OK || rpc.m_value != NI_RPC_SUCCESS ||
            NetInterfaceFile::parseInfo(rpc.m_data, rpc.m_size, info) != NI_OK) { return NI_PROTOCOL_ERROR; }
        m_stream = info.stream;
        m_expiresUtc = info.expiresUtc;
    }
    else if (frame.m_kind == NI_REQUEST)
    {
        m_request = frame.m_requestId;
        if (m_stopOnRequest != nullptr) { m_stopOnRequest->shutdown(); }
        return m_requestResult;
    }
    else if (frame.m_kind == NI_CANCEL)
    {
        ++m_cancels;
        m_lastCancel = frame.m_requestId;
    }
    return NI_OK;
}

void RouterFileSink::close(uint64_t)
{

}

/** 核对挂接端点在路由器已经轮询后交付请求、关闭端点和请求停止时的等待预算
@param [in] waited 实际交给宿主的等待毫秒
@return 等待反映在册请求和待回收工作时为0
*/
static int CheckAttachedRouterWait(const uint32_t& waited)
{
    RouterFileSink services, clients;
    NetEndpointRouter router(&services, &clients);
    NetExecution owner;
    NetExecutionScope scope(owner);
    owner.begin();
    CHECK_TEST(router.poll());
    NetEndpointOrigin origin = {};
    origin.m_endpoint = 77;
    origin.m_platform = origin.m_device[0] = 1;
    const uint32_t negotiated[] = {1, 3, 32, 1024};
    uint64_t source = 0, server = 0, internal = 0;
    CHECK_TEST(router.addService(7, false, 32, server) == NI_OK && router.addService(0, true, 32, internal) == NI_OK &&
        router.addClient(origin, negotiated, source) == NI_OK && owner.wait(60000) == NI_OK && waited == 60000);
    uint8_t bytes[16];
    uint32_t length = 0;
    CHECK_TEST(NetInterfaceRpc::encode(NI_REQUEST, 30000, 0, nullptr, 0, bytes, sizeof(bytes), length) == NI_OK);
    NetInterfaceFrame frame = {8, 1, 9, NI_REQUEST, length, bytes};
    CHECK_TEST(router.receiveClient(source, frame) == NI_OK && router.pendingCount() == 0 &&
        owner.wait(60000) == NI_OK && waited == 60000);
    services.m_requestResult = NI_LIMIT_EXCEEDED;
    frame.m_productId = 7;
    frame.m_requestId = 2;
    CHECK_TEST(router.receiveClient(source, frame) == NI_OK && router.pendingCount() == 0 &&
        owner.wait(60000) == NI_OK && waited == 60000);
    services.m_requestResult = NI_OK;
    frame.m_requestId = 3;
    const uint64_t start = NetSenderSubmission::now();
    CHECK_TEST(router.receiveClient(source, frame) == NI_OK && router.pendingCount() == 1 && owner.wait(60000) == NI_OK);
    CHECK_TEST(waited <= 30000 && waited + NetSenderSubmission::now() - start + 2 >= 30000);
    const uint64_t parent = services.m_request;
    owner.begin();
    CHECK_TEST(router.poll() && NetInterfaceRpc::encode(NI_REQUEST, 5000, parent, nullptr, 0, bytes, sizeof(bytes), length) == NI_OK);
    frame.m_requestId = 1;
    const uint64_t childStart = NetSenderSubmission::now();
    CHECK_TEST(router.receiveService(server, frame) == NI_OK && router.pendingCount() == 2 && owner.wait(60000) == NI_OK);
    CHECK_TEST(waited <= 5000 && waited + NetSenderSubmission::now() - childStart + 2 >= 5000);
    CHECK_TEST(NetInterfaceRpc::encode(NI_REQUEST, 60000, parent, nullptr, 0, bytes, sizeof(bytes), length) == NI_OK);
    frame.m_requestId = 2;
    CHECK_TEST(router.receiveService(server, frame) == NI_OK && router.pendingCount() == 3 && owner.wait(60000) == NI_OK);
    CHECK_TEST(waited <= 5000 && waited + NetSenderSubmission::now() - childStart + 2 >= 5000);
    const NetInterfaceFrame cancel = {7, 3, 9, NI_CANCEL, 0, nullptr};
    CHECK_TEST(router.receiveClient(source, cancel) == NI_OK && !router.pendingCount());
    owner.begin();
    CHECK_TEST(router.poll() && owner.wait(60000) == NI_OK && waited == 60000);
    router.closeEndpoint(UINT64_MAX, NI_CANCELLED);
    CHECK_TEST(owner.wait(60000) == NI_OK && waited == 60000);
    router.closeEndpoint(internal, NI_CANCELLED);
    CHECK_TEST(owner.wait(60000) == NI_OK && waited == 0);
    owner.begin();
    router.closeEndpoint(internal, NI_CANCELLED);
    CHECK_TEST(router.poll() && owner.wait(60000) == NI_OK && waited == 60000);
#if defined(_MSC_VER) && defined(_DEBUG)
    // 同一来源先成功交付一条，再让第二条在转发缓冲扩容时失败。
    ++origin.m_endpoint;
    uint64_t other = 0;
    CHECK_TEST(router.addClient(origin, negotiated, other) == NI_OK &&
        NetInterfaceRpc::encode(NI_REQUEST, 30000, 0, nullptr, 0, bytes, sizeof(bytes), length) == NI_OK);
    frame.m_requestId = 1;
    CHECK_TEST(router.receiveClient(other, frame) == NI_OK && router.pendingCount() == 1);
    const uint64_t delivered = services.m_request;
    const uint32_t cancellations = services.m_cancels;
    uint8_t body[921] = {}, wire[sizeof(body) + 16];
    CHECK_TEST(NetInterfaceRpc::encode(NI_REQUEST, 30000, 0, body, sizeof(body), wire, sizeof(wire), length) == NI_OK);
    const NetInterfaceFrame large = {7, 2, 9, NI_REQUEST, length, wire};
    rejectedSize = sizeof(wire);
    allocationRefused = false;
    previousAllocationHook = _CrtSetAllocHook(FilterTestAllocation);
    const int32_t failed = router.receiveClient(other, large);
    _CrtSetAllocHook(previousAllocationHook);
    CHECK_TEST(allocationRefused && failed == NI_NO_MEMORY && services.m_request == delivered &&
        services.m_cancels == cancellations && router.pendingCount() == 1 && router.receiveClient(other, large) == NI_NOT_FOUND);
    owner.begin();
    CHECK_TEST(router.poll() && !router.pendingCount() && services.m_request == delivered &&
        services.m_cancels == cancellations + 1 && services.m_lastCancel == delivered);
#endif
    services.m_stopOnRequest = &router;
    CHECK_TEST(NetInterfaceRpc::encode(NI_REQUEST, 30000, 0, nullptr, 0, bytes, sizeof(bytes), length) == NI_OK);
    frame.m_requestId = 4;
    CHECK_TEST(router.receiveClient(source, frame) == NI_OK && owner.wait(60000) == NI_OK && waited == 0);
    owner.begin();
    CHECK_TEST(!router.poll() && !router.pendingCount() && owner.wait(60000) == NI_OK && waited == 60000);
    return 0;
}

/** 经真实请求和服务回复建立三个不同期限的文件流，核对转发缓冲及独立回收
@param [in] retention 转发后允许保留的暂存容量
@param [in] waited 本轮实际交给事件宿主的等待毫秒
@param [in] expireTogether 是否让三个文件与三个请求同时到期，并核对每次poll的工作量
@return 设置有效且三个不同流均恰好终止一次时为0
*/
static int CheckRouterStorage(int64_t retention, const uint32_t& waited, bool expireTogether = false)
{
    RouterFileSink service, client;
    NetEndpointRouter router(&service, &client);
    NetEndpointPools pools;
    CHECK_TEST(pools.m_payloadRetentionBytes == 4096);
    pools.m_payloadRetentionBytes = retention;
    if (retention == 0)
    {
        CHECK_TEST(router.configureFileLeaseLimit(1) == NI_OK && router.configureFileLeaseLimit(1800) == NI_OK &&
            router.configureFileLeaseLimit(45) == NI_OK && router.configureFileLeaseLimit(0) == NI_INVALID_ARGUMENT &&
            router.configureFileLeaseLimit(1801) == NI_INVALID_ARGUMENT && router.configureFileLeaseLimit(UINT32_MAX) == NI_INVALID_ARGUMENT);
    }
    if (expireTogether)
    {
        pools.m_expiredRequestBudget = 2;
        pools.m_closingEndpointBudget = 1;
        pools.m_expiredFileBudget = 1;
        pools.m_endpoints.initialCapacity = 2;
        pools.m_endpoints.maximumCapacity = 2;
    }
    CHECK_TEST(router.configurePools(pools) == NI_OK);
    pools.m_payloadRetentionBytes = -2;
    CHECK_TEST(router.configurePools(pools) == NI_INVALID_ARGUMENT);
    NetEndpointOrigin origin = {};
    origin.m_endpoint = 88;
    origin.m_platform = origin.m_device[0] = 1;
    const uint32_t features[] = {1, 3, 32, 1024};
    uint64_t server = 0, source = 0;
    CHECK_TEST(router.addService(7, false, 32, server) == NI_OK && router.addClient(origin, features, source) == NI_OK);
    std::vector<uint8_t> body(8192, 0x35), wire(body.size() + 16);
    uint32_t length = 0;
    CHECK_TEST(NetInterfaceRpc::encode(NI_REQUEST, 30000, 0, body.data(), static_cast<uint32_t>(body.size()),
        wire.data(), static_cast<uint32_t>(wire.size()), length) == NI_OK);
    NetInterfaceFrame frame = {7, 1, 9, NI_REQUEST, length, wire.data()};
    CHECK_TEST(router.receiveClient(source, frame) == NI_OK && service.m_request != 0 &&
        (retention < 0 ? router.payloadCapacity() >= wire.size() : router.payloadCapacity() == 0));
    CHECK_TEST(NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, nullptr, 0, wire.data(),
        static_cast<uint32_t>(wire.size()), length) == NI_OK);
    frame.m_kind = NI_RESPONSE;
    frame.m_requestId = service.m_request;
    frame.m_payloadSize = length;
    CHECK_TEST(router.receiveService(server, frame) == NI_OK && router.pendingCount() == 0);
    const uint8_t business[] = {0,81};
    uint8_t envelope[64] = {};
    CHECK_TEST(NetInterfaceEvent::encode(origin.m_endpoint, 0, business, sizeof(business), envelope, sizeof(envelope), length) == NI_OK);
    const NetInterfaceFrame event = {7, 100, 72, NI_EVENT, length, envelope};
    CHECK_TEST(router.receiveService(server, event) == NI_OK && client.m_events == 1 && !service.m_events);
    const auto utcMillis = []() -> uint64_t {
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    };
    const auto steadyMillis = []() -> uint64_t {
        return NetInterface::monotonicMilliseconds();
    };
    uint8_t payload[NET_INTERFACE_FILE_INFO_SIZE];
    uint64_t ids[3] = {};
    uint64_t earliest[3] = {}, latest[3] = {};
    NetExecution owner;
    NetExecutionScope scope(owner);
    const uint32_t durations[] = {90, 30, 10, 60, 5};
    for (unsigned iteration = 0; iteration < 5; ++iteration)
    {
        // 对最后一条流再次索取同一授权，更新已有期限并复用公开流号。
        const unsigned i = (std::min)(iteration, 2U);
        NetInterfaceFileControl open = {};
        open.operation = NI_FILE_OPEN_DOWNLOAD;
        open.chunk = 1024;
        open.window = 1;
        open.key = body.data();
        open.keySize = 1;
        open.transfer[0] = static_cast<uint8_t>(i + 1);
        uint32_t count = 0;
        CHECK_TEST(NetInterfaceFile::encodeControl(open, payload, sizeof(payload), count) == NI_OK &&
            NetInterfaceRpc::encode(NI_REQUEST, 60000, 0, payload, count, wire.data(), static_cast<uint32_t>(wire.size()), length) == NI_OK);
        frame.m_requestId = 2 + iteration;
        frame.m_messageType = NI_FILE_OPEN_DOWNLOAD;
        frame.m_kind = NI_REQUEST;
        frame.m_payloadSize = length;
        CHECK_TEST(router.receiveClient(source, frame) == NI_OK);
        NetInterfaceFileInfo info = {};
        info.direction = 2;
        info.state = NI_FILE_TRANSFERRING;
        info.chunk = 1024;
        info.window = 1;
        info.total = 1;
        info.stream = 100 + i;
        info.expiresUtc = utcMillis() / 1000 + (expireTogether ? 2 : durations[iteration]);
        if (retention == 0 && iteration == 0) { info.expiresUtc = INT64_MAX; }
        info.transfer[0] = open.transfer[0];
        info.hash[0] = info.token[0] = 1;
        CHECK_TEST(NetInterfaceFile::encodeInfo(info, payload, sizeof(payload)) == NI_OK &&
            NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, payload, sizeof(payload),
                wire.data(), static_cast<uint32_t>(wire.size()), length) == NI_OK);
        frame.m_requestId = service.m_request;
        frame.m_kind = NI_RESPONSE;
        frame.m_payloadSize = length;
        client.m_stream = client.m_expiresUtc = 0;
        owner.begin();
        CHECK_TEST(router.poll());
        const uint64_t steadyBefore = steadyMillis();
        const uint64_t beforeReply = utcMillis();
        CHECK_TEST(router.receiveService(server, frame) == NI_OK && client.m_stream != 0);
        const uint64_t afterReply = utcMillis();
        const uint64_t steadyAfter = steadyMillis();
        const uint64_t limit = retention == 0 ? 45 : 1800;
        CHECK_TEST(client.m_expiresUtc >= (std::min)(info.expiresUtc, beforeReply / 1000 + limit) &&
            client.m_expiresUtc <= (std::min)(info.expiresUtc, afterReply / 1000 + limit));
        // 接纳前后分别采两种时钟，客户端收到的 UTC 整秒必须落在这个单调期限区间。
        earliest[i] = steadyBefore + client.m_expiresUtc * 1000 - afterReply;
        latest[i] = steadyAfter + client.m_expiresUtc * 1000 - beforeReply;
        if (!expireTogether && (iteration == 1 || iteration == 2 || iteration == 4))
        {
            // 新绑定或缩短的期限均早于本轮 poll 已登记的请求和旧租约。
            CHECK_TEST(owner.wait(120000) == NI_OK && steadyBefore + waited <= latest[i] + 2 &&
                steadyMillis() + waited + 2 >= earliest[i]);
        }
        CHECK_TEST(iteration < 3 || ids[i] == client.m_stream);
        ids[i] = client.m_stream;
    }
    CHECK_TEST(ids[0] != ids[1] && ids[1] != ids[2] && ids[0] != ids[2]);
    if (expireTogether)
    {
        CHECK_TEST(NetInterfaceRpc::encode(NI_REQUEST, 25, 0, nullptr, 0, wire.data(),
            static_cast<uint32_t>(wire.size()), length) == NI_OK);
        frame.m_kind = NI_REQUEST;
        frame.m_messageType = 9;
        frame.m_payloadSize = length;
        for (uint32_t i = 0; i < 3; ++i)
        {
            frame.m_requestId = 10 + i;
            CHECK_TEST(router.receiveClient(source, frame) == NI_OK);
        }
        CHECK_TEST(router.pendingCount() == 3);
        const auto due = TimerProbeNow() + std::chrono::milliseconds(2050);
        WaitTimerProbe(due);
        for (unsigned i = 0; i < 3; ++i)
        {
            CHECK_TEST(router.poll() && router.pendingCount() == (i == 0 ? 1U : 0U) &&
                service.m_notices.size() == i + 1 && client.m_notices.size() == i + 1);
        }
        std::sort(service.m_notices.begin(), service.m_notices.end());
        std::sort(client.m_notices.begin(), client.m_notices.end());
        std::sort(ids, ids + 3);
        for (unsigned i = 0; i < 3; ++i)
        {
            CHECK_TEST(service.m_notices[i] == 100 + i && client.m_notices[i] == ids[i]);
        }
        router.closeEndpoint(source, NI_CANCELLED);
        router.closeEndpoint(server, NI_CANCELLED);
        CHECK_TEST(owner.wait(60000) == NI_OK && waited == 0);
        uint64_t first = 0, second = 0;
        owner.begin();
        CHECK_TEST(router.poll() && router.addService(8, false, 1, first) == NI_OK && first != 0 &&
            router.addService(9, false, 1, second) == NI_LIMIT_EXCEEDED && !second && owner.wait(60000) == NI_OK && waited == 0);
        owner.begin();
        CHECK_TEST(router.poll() && router.addService(9, false, 1, second) == NI_OK && second != first &&
            owner.wait(60000) == NI_OK && waited == 60000);
        router.shutdown();
        return 0;
    }
    const unsigned order[] = {1, 0, 2};
    CHECK_TEST(router.configureFileLeaseLimit(5) == NI_INVALID_STATE);
    for (unsigned i = 0; i < 3; ++i)
    {
        const uint64_t beforePoll = steadyMillis();
        owner.begin();
        CHECK_TEST(router.poll() && owner.wait(120000) == NI_OK);
        const uint64_t afterWait = steadyMillis();
        CHECK_TEST(beforePoll + waited <= *std::min_element(latest, latest + 3) &&
            afterWait + waited >= *std::min_element(earliest, earliest + 3));
        const unsigned index = order[i];
        NetInterfaceFrame cancel = {7, ids[index], NI_FILE_DATA_TYPE, NI_CANCEL, 0, nullptr};
        CHECK_TEST(router.receiveClient(source, cancel) == NI_OK && router.receiveClient(source, cancel) == NI_OK &&
            service.m_notices.size() == i + 1 && service.m_notices[i] == 100 + index);
        earliest[index] = latest[index] = UINT64_MAX;
    }
    // 已取消文件流的迟到记录可忽略；其他 DATA 类型没有对应的路由协议。
    NetInterfaceFrame data = {7, ids[0], NI_FILE_DATA_TYPE, NI_DATA, 0, nullptr};
    CHECK_TEST(router.receiveClient(source, data) == NI_OK);
    data.m_requestId = 100;
    data.m_kind = NI_DATA_ACK;
    CHECK_TEST(router.receiveService(server, data) == NI_OK);
    data.m_messageType = 17;
    CHECK_TEST(router.receiveService(server, data) == NI_NOT_SUPPORTED &&
        router.receiveService(server, data) == NI_NOT_FOUND);
    data.m_requestId = ids[0];
    data.m_kind = NI_DATA;
    CHECK_TEST(router.receiveClient(source, data) == NI_NOT_SUPPORTED &&
        router.receiveClient(source, data) == NI_NOT_FOUND);
    router.shutdown();
    CHECK_TEST(router.payloadCapacity() == 0);
    return 0;
}

/** 验证默认、零及无限暂存预算和文件期限堆摘除后的路由行为
@return 全部通过时为0
*/
static int RouterStorage()
{
#if defined(NET_INTERFACE_SDK_STATIC_LINK) || defined(__EMSCRIPTEN__)
    uint64_t expires = 102, deadline = 0;
    CHECK_TEST(NetEndpointFileStream::leaseDeadline(101875, 7000, 45, expires, deadline) && expires == 102 && deadline == 7125);
    expires = INT64_MAX;
    CHECK_TEST(NetEndpointFileStream::leaseDeadline(101875, 7000, 45, expires, deadline) && expires == 146 && deadline == 51125);
    expires = 102;
    CHECK_TEST(NetEndpointFileStream::leaseDeadline(101999, 7000, 1, expires, deadline) && expires == 102 && deadline == 7001);
    CHECK_TEST(!NetEndpointFileStream::leaseDeadline(102000, 7001, 1, expires, deadline) && expires == 102 && deadline == 7001);
    CHECK_TEST(!NetEndpointFileStream::leaseDeadline(-1, 7001, 1, expires, deadline) && expires == 102 && deadline == 7001);
    CHECK_TEST(NetEndpointFileStream::leaseDeadline(101000, UINT64_MAX - 2, 1, expires, deadline) && expires == 102 && deadline == UINT64_MAX);
#endif
    return WithWaitProbe([](const uint32_t& waited) -> int {
        CHECK_TEST(CheckAttachedRouterWait(waited) == 0);
        const int defaults = CheckRouterStorage(4096, waited);
        if (defaults != 0) { return defaults; }
        const int discard = CheckRouterStorage(0, waited);
        if (discard != 0) { return discard; }
        return CheckRouterStorage(-1, waited);
    });
}

/** 让父请求和两个内部子请求同刻到期，逐轮核对请求数和原始编号
@return 一步预算恰好结算一条请求时为0
*/
static int RelatedRequestTimeouts()
{
    RouterFileSink services, clients;
    NetEndpointRouter router(&services, &clients);
    NetEndpointPools pools;
    pools.m_expiredRequestBudget = 1;
    NetEndpointOrigin origin = {};
    origin.m_endpoint = 88;
    origin.m_platform = origin.m_device[0] = 1;
    const uint32_t features[] = {1, 3, 32, 1024};
    uint64_t server = 0, internal = 0, source = 0;
    CHECK_TEST(router.configurePools(pools) == NI_OK && router.addService(7, false, 32, server) == NI_OK &&
        router.addService(0, true, 32, internal) == NI_OK && router.addClient(origin, features, source) == NI_OK);
    uint8_t bytes[16];
    uint32_t length = 0;
    CHECK_TEST(NetInterfaceRpc::encode(NI_REQUEST, 150, 0, nullptr, 0, bytes, sizeof(bytes), length) == NI_OK);
    NetInterfaceFrame frame = {7, 20, 711, NI_REQUEST, length, bytes};
    CHECK_TEST(router.receiveClient(source, frame) == NI_OK && services.m_request != 0);
    const uint64_t parent = services.m_request;
    CHECK_TEST(NetInterfaceRpc::encode(NI_REQUEST, 60000, parent, nullptr, 0, bytes, sizeof(bytes), length) == NI_OK);
    for (uint64_t id = 41; id <= 42; ++id)
    {
        frame.m_requestId = id;
        CHECK_TEST(router.receiveService(server, frame) == NI_OK);
    }
    CHECK_TEST(router.pendingCount() == 3);
    YieldTimerProbe(151);
    for (unsigned step = 0; step < 3; ++step)
    {
        CHECK_TEST(router.poll() && router.pendingCount() == 2 - step &&
            clients.m_timeoutIds.size() == 1 && clients.m_timeoutIds[0] == 20 && services.m_timeoutIds.size() == step);
    }
    CHECK_TEST(services.m_timeoutIds[0] == 41 && services.m_timeoutIds[1] == 42);
    CHECK_TEST(NetInterfaceRpc::encode(NI_REQUEST, 60000, 0, nullptr, 0, bytes, sizeof(bytes), length) == NI_OK);
    frame.m_messageType = 712;
    frame.m_requestId = 21;
    CHECK_TEST(router.receiveClient(source, frame) == NI_OK &&
        NetInterfaceRpc::encode(NI_REQUEST, 60000, services.m_request, nullptr, 0, bytes, sizeof(bytes), length) == NI_OK);
    frame.m_requestId = 43;
    CHECK_TEST(router.receiveService(server, frame) == NI_OK && router.pendingCount() == 2);
    const NetInterfaceFrame cancel = {7, 21, 712, NI_CANCEL, 0, nullptr};
    CHECK_TEST(router.receiveClient(source, cancel) == NI_OK && router.pendingCount() == 0);
    return 0;
}

/** 使已接纳的三条文件流与三条请求同时到期，逐轮检查两种预算并回收端点
@return 真实回调、在册请求数及归还槽数符合各自预算时为0
*/
static int RouterPollBudgets()
{
    CHECK_TEST(RelatedRequestTimeouts() == 0);
#if defined(__EMSCRIPTEN__)
    const auto before = TimerProbeNow();
    YieldTimerProbe(2050);
    CHECK_TEST(TimerProbeNow() - before >= std::chrono::milliseconds(2050));
#endif
    return WithWaitProbe([](const uint32_t& waited) -> int { return CheckRouterStorage(4096, waited, true); });
}

/** 交给另一端一条真实管理隧道记录
@param [in,out] from 源管理链路
@param [in,out] to 目标管理链路
@return 没有输出或记录已完整交付时为true
*/
static bool MoveLinkRecord(NetInterfaceLink& from, NetInterfaceLink& to)
{
    NetInterfaceTunnelRecord record = {};
    const int32_t result = from.peekOutput(record);
    return result == NI_NEED_MORE || (result == NI_READY && to.accept(record) == NI_OK && from.consumeOutput() == NI_OK);
}

/** 通过已认证链路验证空闲、PONG及EOF排空期限，不实际休眠
@param [in,out] client 独立客户端
@param [in,out] server 隔离测试节点
@param [in] custom 是否验证本用例的自定义期限
@param [in] waited 宿主观察到的等待预算
@return 握手、期限和唯一终态符合预期时为0
*/
static int CheckLinkTimers(NetInterfaceLink& client, NetInterfaceLink& server, bool custom, const uint32_t& waited)
{
    NetInterfaceLinkPools pools;
    pools.m_outputRecords.initialCapacity = 2;
    pools.m_outputRecords.maximumCapacity = 2;
    pools.m_outputBodies.m_classes[0].initialCapacity = 1;
    pools.m_outputBodies.m_classes[0].maximumCapacity = 1;
    CHECK_TEST(client.configureOutputPools(pools.m_outputBodies, pools.m_outputRecords) == NI_OK);
    pools.m_outputRecords.maximumCapacity = 1;
    CHECK_TEST(client.configureOutputPools(pools.m_outputBodies, pools.m_outputRecords) == NI_INVALID_ARGUMENT);
    pools.m_outputRecords.maximumCapacity = 2;
    for (unsigned i = 0; i < 1024 && (client.status() != NI_READY || server.status() != NI_READY); ++i)
    {
        CHECK_TEST(client.progress(1000) >= 0 && server.progress(1000) >= 0 &&
            MoveLinkRecord(client, server) && MoveLinkRecord(server, client));
    }
    CHECK_TEST(client.status() == NI_READY && server.status() == NI_READY);
    const uint8_t* grant = nullptr;
    uint32_t grantSize = 99;
    CHECK_TEST(client.serviceGrant(grant, grantSize) == NI_INVALID_STATE && !grant && !grantSize);
    CHECK_TEST(client.configureOutputPools(pools.m_outputBodies, pools.m_outputRecords) == NI_INVALID_STATE);
    uint8_t bytes[1025] = {31};
    NetInterfaceTunnelRecord smallRecord = {NI_TUNNEL_BYTES, 7, 1, bytes, 1};
    NetInterfaceTunnelRecord mediumRecord = {NI_TUNNEL_BYTES, 7, 2, bytes, 65};
    NetInterfaceTunnelRecord largeRecord = {NI_TUNNEL_BYTES, 7, 3, bytes, sizeof(bytes)};
    NetExecution owner;
    NetExecutionScope scope(owner);
    const NetInterfaceTunnelRecord inputs[] = {smallRecord, {NI_TUNNEL_CLOSE, 7, 1, bytes, 4}};
    for (const NetInterfaceTunnelRecord& input : inputs)
    {
        CHECK_TEST(server.progress(1000) == NI_READY);
        owner.begin();
        CHECK_TEST(server.accept(input) == NI_OK && owner.wait(60000) == NI_OK && waited == 0);
        owner.begin();
        CHECK_TEST(server.accept(input) == NI_LIMIT_EXCEEDED && owner.wait(60000) == NI_OK && waited == 60000);
        NetInterfaceTunnelRecord held = {};
        CHECK_TEST(server.peekRecord(held) == NI_READY && held.m_kind == input.m_kind && held.m_product == 7 &&
            held.m_channel == 1 && held.m_size == input.m_size && held.m_data && !std::memcmp(held.m_data, bytes, held.m_size) &&
            server.consumeRecord() == NI_OK);
    }
    for (unsigned round = 0; round < 2; ++round)
    {
        owner.begin();
        CHECK_TEST(client.sendRecord(smallRecord) == NI_OK && owner.wait(60000) == NI_OK && waited == 0);
        owner.begin();
        CHECK_TEST(client.sendRecord(smallRecord) == NI_LIMIT_EXCEEDED && owner.wait(60000) == NI_OK && waited == 60000);
        owner.begin();
        CHECK_TEST(client.sendRecord(mediumRecord) == NI_OK && owner.wait(60000) == NI_OK && waited == 0);
        owner.begin();
        CHECK_TEST(client.sendRecord(largeRecord) == NI_LIMIT_EXCEEDED && owner.wait(60000) == NI_OK && waited == 60000);
        uint32_t seen = 0;
        for (unsigned step = 0; step < 128 && seen < 2; ++step)
        {
            NetInterfaceTunnelRecord record = {};
            CHECK_TEST(client.peekOutput(record) == NI_READY);
            if (record.m_kind == NI_TUNNEL_CONTROL) { CHECK_TEST(server.accept(record) >= 0); }
            else
            {
                CHECK_TEST(record.m_kind == NI_TUNNEL_BYTES && record.m_channel == seen + 1 &&
                    record.m_size == (seen == 0 ? 1U : 65U) && record.m_data && !std::memcmp(record.m_data, bytes, record.m_size));
                ++seen;
            }
            CHECK_TEST(client.consumeOutput() == NI_OK);
        }
        CHECK_TEST(seen == 2);
    }
    auto checkWait = [&](NetInterfaceLink& link, uint64_t now, uint32_t expected) -> bool {
        owner.begin();
        const auto start = TimerProbeNow();
        if (link.progress(now) != NI_READY || owner.wait(60000) != NI_OK) { return false; }
        const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            TimerProbeNow() - start).count());
        return waited <= expected && waited + elapsed + 2 >= expected;
    };
    const uint32_t idle = custom ? 9000 : 15000;
    const uint32_t pong = custom ? 7000 : 10000;
    const uint32_t drain = custom ? 3000 : 5000;
    const uint64_t pingAt = 1000 + idle;
    CHECK_TEST(checkWait(client, 1500, idle - 500));
    // 生成心跳和随后把已排队的心跳交给 TLS 均有本地工作。交付后可等待原 PONG 期限。
    CHECK_TEST(checkWait(client, pingAt, 0) && checkWait(client, pingAt + 500, 0) &&
        checkWait(client, pingAt + 500, pong - 500));
    const uint8_t body[] = {0, 17, 41};
    const NetInterfaceFrame event = {0, 17, 42, NI_EVENT, sizeof(body), body};
    NetInterfaceFrame invalid = event;
    const uint16_t tunnelOnly[] = {NI_DATA, NI_DATA_ACK};
    for (uint16_t kind : tunnelOnly)
    {
        invalid.m_kind = kind;
        CHECK_TEST(client.sendControl(invalid) == NI_INVALID_ARGUMENT);
    }
    invalid = event;
    invalid.m_messageType = NET_INTERFACE_LINK_HEARTBEAT;
    CHECK_TEST(client.sendControl(invalid) == NI_INVALID_ARGUMENT);
    CHECK_TEST(client.sendControl(event) == NI_OK);
    std::vector<uint8_t> combined;
    for (unsigned step = 0; step < 16; ++step)
    {
        NetInterfaceTunnelRecord part = {};
        const int32_t status = client.peekOutput(part);
        if (status == NI_NEED_MORE) { break; }
        CHECK_TEST(status == NI_READY && part.m_kind == NI_TUNNEL_CONTROL && part.m_data && part.m_size &&
            combined.size() + part.m_size <= 16384);
        combined.insert(combined.end(), part.m_data, part.m_data + part.m_size);
        CHECK_TEST(client.consumeOutput() == NI_OK);
    }
    CHECK_TEST(!combined.empty() && client.outputState() == NI_OK);
    const NetInterfaceTunnelRecord wire = {NI_TUNNEL_CONTROL, 0, 0, combined.data(), static_cast<uint32_t>(combined.size())};
    CHECK_TEST(server.accept(wire) == NI_OK && server.progress(pingAt + 500) == NI_READY);
    NetInterfaceFrame received = {};
    if (custom)
    {
        CHECK_TEST(server.peekControl(received) == NI_NEED_MORE && server.progress(pingAt + 500) == NI_READY);
    }
    CHECK_TEST(server.peekControl(received) == NI_READY && received.m_kind == NI_EVENT &&
        received.m_messageType == 42 && received.m_requestId == 17 && received.m_payloadSize == sizeof(body) &&
        received.m_payload && !std::memcmp(received.m_payload, body, sizeof(body)) && server.consumeControl() == NI_OK);
    CHECK_TEST(client.progress(pingAt + pong - 1) == NI_READY);
    // 对端已回复，直到等待期限才把密文交给客户端推进。
    NetInterfaceTunnelRecord pongRecord = {};
    CHECK_TEST(server.peekOutput(pongRecord) == NI_READY && pongRecord.m_kind == NI_TUNNEL_CONTROL &&
        client.accept(pongRecord) == NI_OK && server.consumeOutput() == NI_OK &&
        client.progress(pingAt + pong) == NI_TIMEOUT);
    const uint8_t byte = 1;
    NetInterfaceTunnelRecord pending = {NI_TUNNEL_BYTES, 7, 1, &byte, 1};
    const uint64_t drainAt = pingAt + 501;
    CHECK_TEST(server.accept(pending) == NI_OK && server.drainInput(drainAt) == NI_READY &&
        checkWait(server, drainAt + 1000, drain - 1000) && server.progress(drainAt + drain - 1) == NI_READY &&
        server.progress(drainAt + drain) == NI_TIMEOUT);
    return 0;
}

/** 验证不透明管理链路的身份视图、双向控制与隧道记录及终态。
@param [in] clientOptions 客户端配置
@param [in] nodeOptions 持隔离证书的节点配置
@return 句柄按约定接受、交付和消费全部字段时返回0
*/
static int AuthenticatedLinkHandles(const NetInterfaceLinkOptions& clientOptions, const NetInterfaceLinkOptions& nodeOptions)
{
    NetInterfaceLinkHandle client = nullptr, node = nullptr;
    NetInterfaceLinkPools pools;
    NetInterfaceLinkOptions options = clientOptions;
    options.m_claimsSize = 1;
    CHECK_TEST(NetInterface::linkCreate(&options, &client) == NI_INVALID_ARGUMENT && !client);
    std::vector<uint8_t> supplied(4096);
    for (size_t i = 0; i < supplied.size(); ++i) { supplied[i] = static_cast<uint8_t>(i); }
    const std::vector<uint8_t> expected = supplied;
    options.m_claims = supplied.data();
    options.m_claimsSize = static_cast<uint32_t>(supplied.size()) + 1;
    CHECK_TEST(NetInterface::linkCreate(&options, &client) == NI_INVALID_ARGUMENT && !client);
    options.m_claimsSize = static_cast<uint32_t>(supplied.size());
    CHECK_TEST(NetInterface::linkCreate(&options, &client) >= 0 && client);
    std::fill(supplied.begin(), supplied.end(), 0);
    std::unique_ptr<NetInterfaceLinkOpaque, void (*)(NetInterfaceLinkHandle)> left(client, NetInterface::linkDestroy);
    CHECK_TEST(NetInterface::linkCreate(&nodeOptions, &node, &pools) >= 0 && node);
    std::unique_ptr<NetInterfaceLinkOpaque, void (*)(NetInterfaceLinkHandle)> right(node, NetInterface::linkDestroy);
    const auto move = [](NetInterfaceLinkHandle from, NetInterfaceLinkHandle to) -> bool {
        uint32_t fields[5] = {};
        const uint8_t* data = nullptr;
        const int32_t status = NetInterface::linkPeekOutput(from, fields, &data);
        return status == NI_NEED_MORE || (status == NI_READY && NetInterface::linkAccept(to, fields, data) == NI_OK &&
            NetInterface::linkConsumeOutput(from) == NI_OK);
    };
    const auto pump = [&]() -> bool {
        return NetInterface::linkProgress(client, 100, 1) >= 0 && NetInterface::linkProgress(node, 100, 1) >= 0 &&
            move(client, node) && move(node, client);
    };
    for (unsigned i = 0; i < 1024 && (NetInterface::linkStatus(client) != NI_READY ||
        NetInterface::linkStatus(node) != NI_READY); ++i) { CHECK_TEST(pump()); }
    const uint8_t* name = nullptr;
    const uint8_t* certificate = nullptr;
    const uint8_t* claims = nullptr;
    uint32_t fields[6] = {};
    CHECK_TEST(NetInterface::linkPeerInfo(client, fields, &name, &certificate, &claims) == NI_READY &&
        fields[0] == NI_LINK_NODE && fields[1] == std::strlen(nodeOptions.m_name) && name &&
        !std::memcmp(name, nodeOptions.m_name, fields[1]) && fields[2] && certificate && !fields[3] && !claims);
    CHECK_TEST(NetInterface::linkPeerInfo(node, fields, &name, &certificate, &claims) == NI_READY &&
        fields[0] == NI_LINK_CLIENT && !fields[1] && !fields[2] && !certificate && fields[3] == expected.size() &&
        claims && !std::memcmp(claims, expected.data(), expected.size()));
    const uint8_t body[] = {0, 203, 71};
    const uint32_t record[] = {NI_TUNNEL_BYTES, 9, 31, 7, sizeof(body)};
    const uint32_t control[] = {NI_EVENT, 0, 71, 3, 42, sizeof(body)};
    CHECK_TEST(NetInterface::linkConsumeRecord(node) == NI_INVALID_STATE &&
        NetInterface::linkConsumeControl(client) == NI_INVALID_STATE &&
        NetInterface::linkSendRecord(client, record, body) == NI_OK && NetInterface::linkSendControl(node, control, body) == NI_OK);
    const uint8_t* view = nullptr;
    for (unsigned i = 0; i < 128 && (NetInterface::linkPeekRecord(node, fields, &view) != NI_READY ||
        NetInterface::linkPeekControl(client, fields, &view) != NI_READY); ++i) { CHECK_TEST(pump()); }
    CHECK_TEST(NetInterface::linkPeekRecord(node, fields, &view) == NI_READY && !std::memcmp(fields, record, sizeof(record)) &&
        view && !std::memcmp(view, body, sizeof(body)) && NetInterface::linkConsumeRecord(node) == NI_OK);
    CHECK_TEST(NetInterface::linkPeekControl(client, fields, &view) == NI_READY && !std::memcmp(fields, control, sizeof(control)) &&
        view && !std::memcmp(view, body, sizeof(body)) && NetInterface::linkConsumeControl(client) == NI_OK);
    CHECK_TEST(NetInterface::linkFinishInput(node) == NI_TRUNCATED && NetInterface::linkStatus(node) == NI_TRUNCATED &&
        NetInterface::linkPeekRecord(node, fields, &view) == NI_TRUNCATED && !fields[4] && !view);
    return 0;
}

/** 从显式传入的隔离测试叶身份构造真实认证的管理TLS对
@param [in] path 测试宿主提供的NGI3文件，私钥不写入程序或日志
@return 0通过，1样本或完整链路验证失败
*/
static int LinkTimers(const char* path)
{
    std::unique_ptr<FILE, int (*)(FILE*)> file(std::fopen(path, "rb"), std::fclose);
    CHECK_TEST(file.get() != nullptr);
    static const uint32_t kIdentityCapacity = 24577;
    uint8_t bytes[kIdentityCapacity] = {};
    const auto wipe = [](uint8_t* data) { NetInterface::wipe(data, kIdentityCapacity); };
    std::unique_ptr<uint8_t, decltype(wipe)> cleanup(bytes, wipe);
    const size_t n = std::fread(bytes, 1, sizeof(bytes), file.get());
    CHECK_TEST(!std::ferror(file.get()) && n >= 48 && n < sizeof(bytes) && !std::memcmp(bytes, "NGI3", 4));
    uint32_t lengths[3] = {};
    for (unsigned i = 0; i < 3; ++i)
    {
        for (unsigned j = 0; j < 4; ++j) { lengths[i] = lengths[i] * 256 + bytes[4 + 4 * i + j]; }
    }
    uint8_t digest[32];
    CHECK_TEST(lengths[0] == 37 && lengths[1] && lengths[1] <= 4096 && lengths[2] && lengths[2] <= 16384 &&
        48 + lengths[0] + lengths[1] + lengths[2] == n &&
        NetInterface::sha256(bytes, static_cast<uint32_t>(n - 32), digest, sizeof(digest)) == NI_OK &&
        !std::memcmp(digest, bytes + n - 32, 32));
    const std::string name(reinterpret_cast<const char*>(bytes + 16), lengths[0]);
    uint32_t seeds[] = {11, 17};
    NetInterfaceLinkOptions options = {};
    options.m_version = 3;
    options.m_role = NI_LINK_CLIENT;
    options.m_tls.entropy = HelloEntropy;
    options.m_tls.entropyContext = seeds;
    options.m_tls.handshakeTimeoutMs = 5000;
    const NetInterfaceLinkOptions clientOptions = options;
    options.m_role = NI_LINK_NODE;
    options.m_name = name.c_str();
    options.m_peerRoles = 1U << NI_LINK_CLIENT;
    options.m_tls.server = 1;
    options.m_tls.privateKey = bytes + 16 + lengths[0];
    options.m_tls.privateKeySize = lengths[1];
    options.m_tls.certificate = options.m_tls.privateKey + lengths[1];
    options.m_tls.certificateSize = lengths[2];
    options.m_tls.entropyContext = seeds + 1;
#if defined(NET_INTERFACE_SDK_STATIC_LINK) || defined(__EMSCRIPTEN__)
    uint8_t challenge[] = {5, 0, 19, 87};
    std::vector<uint8_t> proof(challenge, challenge + sizeof(challenge));
    const uint8_t* cert = options.m_tls.certificate;
    const uint8_t* key = options.m_tls.privateKey;
    const std::string certPem(reinterpret_cast<const char*>(cert), lengths[2] - (cert[lengths[2] - 1] == 0));
    std::string keyPem(reinterpret_cast<const char*>(key), lengths[1] - (key[lengths[1] - 1] == 0));
    CHECK_TEST(!keyPem.empty());
    const int32_t signedResult = NetInterfaceCertificate::signProof(keyPem, proof.data(),
        static_cast<uint32_t>(proof.size()), HelloEntropy, seeds + 1, proof);
    NetInterface::wipe(reinterpret_cast<uint8_t*>(&keyPem[0]), static_cast<uint32_t>(keyPem.size()));
    CHECK_TEST(signedResult == NI_OK && !proof.empty() && proof.size() <= 80 &&
        NetInterfaceCertificate::verifyProof(certPem, name, challenge, sizeof(challenge), proof.data(),
        static_cast<uint32_t>(proof.size())) == NI_OK);
    ++challenge[0];
    CHECK_TEST(NetInterfaceCertificate::verifyProof(certPem, name, challenge, sizeof(challenge), proof.data(),
        static_cast<uint32_t>(proof.size())) == NI_PROTOCOL_ERROR);
#endif
    char unterminated[97];
    std::memset(unterminated, 'n', sizeof(unterminated));
    NetInterfaceLinkOptions invalidName = options;
    invalidName.m_name = unterminated;
    NetInterfaceLink invalidLink(invalidName);
    CHECK_TEST(invalidLink.status() == NI_INVALID_ARGUMENT);
    CHECK_TEST(AuthenticatedLinkHandles(clientOptions, options) == 0);
    {
        uint32_t proveCalls = 0, verifyCalls = 0;
        NetInterfaceLinkOptions service = clientOptions;
        const uint8_t serviceKey[] = TEST_SRV_KEY_EC_PEM;
        service.m_role = NI_LINK_SERVICE;
        service.m_name = "service.7";
        service.m_tls.privateKey = serviceKey;
        service.m_tls.privateKeySize = sizeof(serviceKey);
        service.m_serviceContext = &proveCalls;
        NetInterfaceLinkOptions node = options;
        node.m_peerRoles = 1U << NI_LINK_SERVICE;
        node.m_serviceContext = &verifyCalls;
        NetInterfaceLinkHandle handle = nullptr;
        CHECK_TEST(NetInterface::linkCreate(&service, &handle) == NI_INVALID_ARGUMENT && !handle &&
            NetInterface::linkCreate(&node, &handle) == NI_INVALID_ARGUMENT && !handle);
        // 宿主拒绝提供证明。仅在收到完整声明及约定大小的输出区时记录调用。
        service.m_serviceProof = [](void* context, const uint8_t* statement, uint32_t size, uint8_t* proof, uint32_t capacity) -> int32_t {
            if (context && statement && size && proof && capacity == 32)
            {
                ++*static_cast<uint32_t*>(context);
            }
            return NI_CANCELLED;
        };
        // 没有向节点提交证明，因此不会调用校验或生成授权。
        node.m_serviceVerify = [](void* context, const NetInterfaceLinkPeer*, const uint8_t*, uint32_t, const uint8_t*,
            uint32_t, uint8_t*, uint32_t, uint32_t*) -> int32_t {
            ++*static_cast<uint32_t*>(context);
            return NI_CANCELLED;
        };
        NetInterfaceLink serviceLink(service), nodeLink(node);
        CHECK_TEST(serviceLink.status() == NI_NEED_MORE && nodeLink.status() == NI_NEED_MORE);
        for (uint64_t now = 1; now <= 128 && serviceLink.status() >= 0; ++now)
        {
            if (serviceLink.progress(now) < 0) { break; }
            CHECK_TEST(nodeLink.progress(now) >= 0 && MoveLinkRecord(serviceLink, nodeLink) && MoveLinkRecord(nodeLink, serviceLink));
        }
        CHECK_TEST(serviceLink.status() == NI_PROTOCOL_ERROR && proveCalls == 1 && verifyCalls == 0 &&
            nodeLink.status() != NI_READY);
    }
    return WithWaitProbe([&](const uint32_t& waited) -> int {
        NetInterfaceLinkPools pools;
        for (unsigned i = 0; i < 2; ++i)
        {
            if (i != 0)
            {
                pools.m_controlStepBudget = 1;
                pools.m_pingIntervalMs = 9000;
                pools.m_pongTimeoutMs = 7000;
                pools.m_drainTimeoutMs = 3000;
            }
            NetInterfaceLink client(clientOptions, pools);
            NetInterfaceLink server(options, pools);
            CHECK_TEST(CheckLinkTimers(client, server, i != 0, waited) == 0);
        }
        return 0;
    });
}

/** 把本轮已产生的全部密文交给对端，独立核对每个已解密帧的编号、种类和正文
@param [in,out] sender 已握手的发送端，本函数不推进其发送队列
@param [in,out] receiver 已握手的接收端
@param [in,out] now 同一测试内的单调毫秒
@param [in] expected 按接收顺序独立给出的编号
@param [in] eventBody 事件的预期完整正文
@param [in,out] received 已核对的累计帧数
@return 全部符合为0
*/
static int CollectScheduled(NetInterfaceConnection& sender, NetInterfaceConnection& receiver, uint32_t& now,
    const std::vector<uint64_t>& expected, const std::vector<uint8_t>& eventBody, size_t& received)
{
    const uint8_t* cipher = nullptr;
    uint32_t size = 0;
    CHECK_TEST(sender.peekOutput(cipher, size) >= 0);
    while (size != 0)
    {
        uint32_t used = 0;
        CHECK_TEST(cipher && receiver.feed(cipher, size, used) >= 0 && used != 0 && used <= size &&
            sender.consumeOutput(used) >= 0 && sender.peekOutput(cipher, size) >= 0);
    }
    for (;;)
    {
        CHECK_TEST(receiver.progress(++now) >= 0);
        const NetInterfaceFrame* frame = nullptr;
        const int32_t status = receiver.peek(frame);
        if (status == NI_NEED_MORE) { return 0; }
        CHECK_TEST(status == NI_READY && frame && received < expected.size() && frame->m_requestId == expected[received]);
        if (frame->m_requestId == 1001)
        {
            CHECK_TEST(frame->m_kind == NI_EVENT && frame->m_payloadSize == eventBody.size() &&
                frame->m_payload && !std::memcmp(frame->m_payload, eventBody.data(), eventBody.size()));
        }
        else
        {
            CHECK_TEST(frame->m_kind == (frame->m_requestId == 1000 ? NI_REQUEST : NI_PING) && frame->m_payloadSize == 0);
        }
        ++received;
        CHECK_TEST(receiver.consume() == NI_OK);
        const int32_t buffered = receiver.decodeBuffered();
        CHECK_TEST(buffered == NI_NEED_MORE || (buffered == NI_READY && received < expected.size()));
    }
}

/** 会话在本轮运输推进结束后才发送心跳，检查零等待、真实密文交付及匹配 PONG 后的下一次心跳
@param [in,out] sender 已握手的业务客户端
@param [in,out] receiver 已握手的业务对端
@param [in] now 两端当前 TLS 单调毫秒
@param [in,out] owner 测试事件执行者
@param [in] waited 最近一次宿主等待预算
@return 心跳连续两次通过 TLS 并被会话匹配时为0
*/
static int CheckQueuedHeartbeat(NetInterfaceConnection& sender, NetInterfaceConnection& receiver, uint32_t now,
    NetExecution& owner, const uint32_t& waited)
{
    NetInterfaceHandshakeOptions hello = {};
    hello.m_server = 1;
    hello.m_capabilities = 1;
    hello.m_maxRequests = 4;
    hello.m_maxChunk = 1024;
    hello.m_timeoutMs = 5000;
    hello.m_nonce[0] = 19;
    NetInterfaceHandshake handshake(hello, 0);
    SessionProbeChannel channel(handshake);
    channel.m_heartbeatTls = &sender;
    channel.m_heartbeatTlsTime = now;
    CiphertextProbeHost events;
    NetSession session;
    NetHeartbeatConfig heartbeat;
    heartbeat.m_pingIntervalMs = 64;
    heartbeat.m_pongTimeoutMs = 5000;
    heartbeat.m_receiveTimeoutMs = 10000;
    uint32_t seed = 11;
    NetInterfaceTlsOptions options = {};
    options.entropy = HelloEntropy;
    options.entropyContext = &seed;
    hello.m_server = 0;
    hello.m_platform = 2;
    hello.m_device[0] = 1;
    CHECK_TEST(session.bindChannel(&channel) == NI_OK && session.configureHeartbeat(heartbeat) == NI_OK &&
        session.connect("127.0.0.1", 9001, options, hello, &events) == NI_OK && session.poll() && session.ready());
    for (uint32_t id = 1; id <= 2; ++id)
    {
        WaitTimerProbe(TimerProbeNow() + std::chrono::milliseconds(heartbeat.m_pingIntervalMs + 1));
        owner.begin();
        const uint8_t* bytes = nullptr;
        uint32_t size = 0;
        CHECK_TEST(session.poll() && session.ready() && channel.m_pings == id && sender.outputState() == NI_READY &&
            sender.peekOutput(bytes, size) >= 0 && size == 0 && owner.wait(heartbeat.m_pongTimeoutMs) == NI_OK && waited == 0);
        NetInterfaceFrame ping = {};
        for (unsigned step = 0; step < 16 && receiver.peek(ping) != NI_READY; ++step)
        {
            CHECK_TEST(session.poll() && TransferCiphertext(sender, receiver) && receiver.progress(now) >= 0);
        }
        NetInterfaceFrame pong = {};
        CHECK_TEST(receiver.peek(ping) == NI_READY && ping.m_kind == NI_PING && ping.m_requestId == id &&
            NetInterfaceRpc::control(ping, pong) == NI_READY && pong.m_kind == NI_PONG && pong.m_requestId == id);
        owner.begin();
        CHECK_TEST(receiver.send(pong) == NI_OK && owner.wait(5000) == NI_OK && waited == 0 && receiver.consume() == NI_OK);
        for (unsigned step = 0; step < 16 && receiver.outputState() == NI_READY; ++step)
        {
            CHECK_TEST(receiver.progress(now) >= 0 && TransferCiphertext(receiver, sender));
        }
        CHECK_TEST(receiver.outputState() == NI_OK && session.poll() && session.ready() && !events.m_closes);
        owner.begin();
        CHECK_TEST(session.poll() && session.ready() && channel.m_pings == id &&
            owner.wait(heartbeat.m_pongTimeoutMs) == NI_OK && waited > 0 && waited <= heartbeat.m_pingIntervalMs);
    }
    return 0;
}

/** 检查紧凑 DATA 完整头、连续编号和增量头各自的接纳提示以及无效输入的等待状态
@param [in,out] sender 业务客户端
@param [in,out] receiver 业务对端
@param [in] now 两端 TLS 单调毫秒
@param [in,out] owner 测试事件执行者
@param [in] waited 最近一次宿主等待预算
@return 三帧均按序解密且拒绝后未错误提示进展时为0
*/
static int CheckCompactSendProgress(NetInterfaceConnection& sender, NetInterfaceConnection& receiver, uint32_t now,
    NetExecution& owner, const uint32_t& waited)
{
    sender.configureCompact(true);
    receiver.configureCompact(true);
    const uint64_t ids[] = {9, 10, 14};
    const uint8_t bodies[] = {0, 0xa3, 0xff};
    for (unsigned i = 0; i < 3; ++i)
    {
        const NetInterfaceFrame frame = {7, ids[i], 9, NI_DATA, 1, bodies + i};
        owner.begin();
        CHECK_TEST(sender.send(frame) == NI_OK && owner.wait(5000) == NI_OK && waited == 0);
    }
    const NetInterfaceFrame invalid = {7, 15, 9, NI_DATA, 1, nullptr};
    owner.begin();
    CHECK_TEST(sender.send(invalid) == NI_INVALID_ARGUMENT && owner.wait(5000) == NI_OK && waited == 5000);
    CHECK_TEST(sender.progress(now) >= 0);
    const uint8_t* bytes = nullptr;
    uint32_t size = 0, used = 0;
    CHECK_TEST(sender.peekOutput(bytes, size) >= 0 && size != 0);
    while (size != 0)
    {
        owner.begin();
        CHECK_TEST(receiver.feed(bytes, size, used) == NET_INTERFACE_TLS_OK && used == size &&
            owner.wait(5000) == NI_OK && waited == 0);
        CHECK_TEST(sender.consumeOutput(used) >= 0 && sender.peekOutput(bytes, size) >= 0);
    }
    for (unsigned i = 0; i < 3; ++i)
    {
        NetInterfaceFrame frame = {};
        for (unsigned step = 0; step < 16 && receiver.peek(frame) != NI_READY; ++step)
        {
            CHECK_TEST(receiver.progress(now) >= 0);
        }
        CHECK_TEST(receiver.peek(frame) == NI_READY && frame.m_kind == NI_DATA && frame.m_productId == 7 &&
            frame.m_messageType == 9 && frame.m_requestId == ids[i] && frame.m_payloadSize == 1 &&
            frame.m_payload && *frame.m_payload == bodies[i] && receiver.consume() == NI_OK);
    }
    owner.begin();
    CHECK_TEST(receiver.progress(now) == NET_INTERFACE_TLS_READY && receiver.feed(nullptr, 0, used) == NET_INTERFACE_TLS_OK &&
        used == 0 && owner.wait(5000) == NI_OK && waited == 5000);
    owner.begin();
    CHECK_TEST(receiver.finishInput() == NET_INTERFACE_TLS_OK && owner.wait(5000) == NI_OK && waited == 0);
    owner.begin();
    CHECK_TEST(receiver.finishInput() == NET_INTERFACE_TLS_OK && owner.wait(5000) == NI_OK && waited == 5000 &&
        receiver.progress(now) == NET_INTERFACE_TLS_TRUNCATED);
    owner.begin();
    CHECK_TEST(receiver.finishInput() == NET_INTERFACE_TLS_TRUNCATED && owner.wait(5000) == NI_OK && waited == 5000);
    return 0;
}

/** 持续积压两个FIFO，验证默认轮转及自定义轮转、分流和推进预算
@param [in] mode 0默认，1小步推进，2允许任意合法长度控制帧优先
@param [in] waited 测试宿主最近观察到的等待毫秒
@return 收到全部预期帧，耗尽预算可继续推进且排空后可等待时为0
*/
static int CheckTlsScheduling(unsigned mode, const uint32_t& waited)
{
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM;
    static const uint8_t crt[] = TEST_SRV_CRT_RSA_SHA256_PEM;
    static const uint8_t key[] = TEST_SRV_KEY_RSA_PEM;
    uint32_t entropy[] = {71, 29};
    NetInterfaceTlsOptions tls = {};
    tls.authority = ca;
    tls.authoritySize = sizeof(ca);
    tls.hostname = "localhost";
    tls.entropy = HelloEntropy;
    tls.entropyContext = entropy;
    NetConnectionPoolConfig config;
    CHECK_TEST(config.m_controlBurst == 32 && config.m_controlPayloadLimit == 4096 &&
        config.m_progressFrameBudget == 512 && config.m_progressStepBudget == 64);
    if (mode != 0)
    {
        config.m_controlBurst = 2;
        config.m_controlPayloadLimit = mode == 1 ? 0 : -1;
        config.m_progressFrameBudget = 2;
        config.m_progressStepBudget = mode == 1 ? 1 : 64;
        if (mode == 1)
        {
            config.m_controlRecords.initialCapacity = 3;
            config.m_controlRecords.maximumCapacity = 3;
        }
    }
    std::unique_ptr<NetInterfaceConnection> senderOwner(new NetInterfaceConnection(tls, 16384,
        mode == 2 ? config : NetConnectionPoolConfig()));
    NetInterfaceConnection& sender = *senderOwner;
    if (mode == 1)
    {
        uint32_t word = 0x375a1bc2U;
        CHECK_TEST(sender.feed(reinterpret_cast<const uint8_t*>(&word), sizeof(word), word) == NET_INTERFACE_TLS_ARGUMENT &&
            word == 0x375a1bc2U && sender.configurePools(config) == NI_OK);
    }
    uint32_t* budgets[] = {&config.m_controlBurst, &config.m_progressFrameBudget, &config.m_progressStepBudget};
    for (size_t i = 0; i < 3; ++i)
    {
        const uint32_t value = *budgets[i];
        *budgets[i] = 0;
        CHECK_TEST(sender.configurePools(config) == NI_INVALID_ARGUMENT);
        *budgets[i] = value;
    }
    config.m_controlPayloadLimit = -2;
    CHECK_TEST(sender.configurePools(config) == NI_INVALID_ARGUMENT);
    tls.server = 1;
    tls.hostname = nullptr;
    tls.certificate = crt;
    tls.certificateSize = sizeof(crt);
    tls.privateKey = key;
    tls.privateKeySize = sizeof(key);
    tls.entropyContext = entropy + 1;
    std::unique_ptr<NetInterfaceConnection> receiverOwner(new NetInterfaceConnection(tls));
    NetInterfaceConnection& receiver = *receiverOwner;
    uint32_t now = 0;
    for (; now < 5000 && (sender.status() != NET_INTERFACE_TLS_READY || receiver.status() != NET_INTERFACE_TLS_READY); ++now)
    {
        CHECK_TEST(sender.progress(now) >= 0 && receiver.progress(now) >= 0 &&
            TransferCiphertext(sender, receiver) && TransferCiphertext(receiver, sender));
    }
    CHECK_TEST(sender.status() == NET_INTERFACE_TLS_READY && receiver.status() == NET_INTERFACE_TLS_READY);
    const uint32_t burst = mode ? 2 : 32;
    std::vector<uint8_t> body(mode == 1 ? 1 : 4097, 0x6d);
    const NetInterfaceFrame data = {7, 1000, 9, NI_REQUEST, 0, nullptr};
    const NetInterfaceFrame event = {7, 1001, 9, NI_EVENT, static_cast<uint32_t>(body.size()), body.data()};
    CHECK_TEST(sender.send(data) == NI_OK && sender.send(event) == NI_OK);
    for (uint32_t i = 1; i <= burst + 1; ++i)
    {
        const NetInterfaceFrame ping = {0, i, 0, NI_PING, 0, nullptr};
        CHECK_TEST(sender.send(ping) == NI_OK);
    }
    std::vector<uint64_t> expected;
    if (mode == 2)
    {
        const uint64_t order[] = {1001, 1, 1000, 2, 3};
        expected.assign(order, order + 5);
    }
    else
    {
        for (uint32_t i = 1; i <= burst; ++i) { expected.push_back(i); }
        expected.push_back(1000);
        expected.push_back(burst + 1);
        expected.push_back(1001);
    }
    NetExecution owner;
    NetExecutionScope scope(owner);
    if (mode == 1)
    {
        const NetInterfaceFrame extra = {0, 99, 0, NI_PING, 0, nullptr};
        owner.begin();
        CHECK_TEST(sender.send(extra) == NI_LIMIT_EXCEEDED && owner.wait(5000) == NI_OK && waited == 5000);
    }
    size_t received = 0;
    for (unsigned turn = 0; turn < 32 && received < expected.size(); ++turn)
    {
        owner.begin();
        CHECK_TEST(sender.progress(++now) >= 0);
        if (mode && turn == 0) { CHECK_TEST(owner.wait(5000) == NI_OK && waited == 0); }
        CHECK_TEST(CollectScheduled(sender, receiver, now, expected, body, received) == 0);
        if (mode == 1 && turn < 2) { CHECK_TEST(received == (turn == 0 ? 0U : 2U)); }
        if (mode == 2 && turn == 0) { CHECK_TEST(received == 2); }
    }
    CHECK_TEST(received == expected.size());
    owner.begin();
    CHECK_TEST(sender.progress(++now) >= 0 && owner.wait(5000) == NI_OK && waited == 5000 &&
        CollectScheduled(sender, receiver, now, expected, body, received) == 0);
    if (mode == 0)
    {
        if (CheckQueuedHeartbeat(sender, receiver, now, owner, waited)) { return 1; }
        return CheckCompactSendProgress(sender, receiver, now, owner, waited);
    }
    return 0;
}

/** 在同一测试宿主中逐个创建三对独立TLS连接并验证实际调度
@return 三种配置全部通过为0
*/
static int TlsScheduling()
{
    return WithWaitProbe([](const uint32_t& waited) -> int {
        for (unsigned mode = 0; mode < 3; ++mode)
        {
            if (CheckTlsScheduling(mode, waited) != 0) { return 1; }
        }
        return 0;
    });
}

/** 核对一批DATA经过真实分派器时每轮交付量、FIFO及排空后等待
@param [in] commands 本轮命令数
@param [in] bytes 本轮正文字节数
@param [in] expected 首两轮每轮可交付的完整帧数
@param [in] waited 宿主记录的最长等待时间
@return 三项预算边界都符合时为0
*/
static int CheckDispatchBudget(uint32_t commands, uint32_t bytes, size_t expected, const uint32_t& waited)
{
    NetInterfaceHandshakeOptions hello = {};
    hello.m_server = 1;
    hello.m_capabilities = 3;
    hello.m_maxRequests = 8;
    hello.m_maxChunk = 1024;
    hello.m_timeoutMs = 5000;
    hello.m_nonce[0] = 19;
    NetInterfaceHandshake server(hello, 0);
    SessionProbeChannel channel(server);
    CiphertextProbeHost events;
    NetClientSession client;
    NetSenderSubmission submission;
    NetSenderDispatch dispatch(&client);
    events.m_startOnReady = &dispatch;
    const auto stop = [](NetSenderDispatch* active) { active->shutdown(); };
    std::unique_ptr<NetSenderDispatch, decltype(stop)> lifetime(&dispatch, stop);
    CHECK_TEST(dispatch.bindSubmission(nullptr) == NI_INVALID_ARGUMENT && dispatch.bindSubmission(&submission) == NI_OK &&
        &dispatch.submission() == &submission);
    CHECK_TEST(dispatch.started(1, 7) == NI_INVALID_STATE);
    uint64_t inactiveGeneration = UINT64_MAX;
    bool inactiveReady = true;
    dispatch.submission().state(inactiveGeneration, inactiveReady);
    CHECK_TEST(inactiveGeneration == 0 && !inactiveReady);
    dispatch.listener()->onSessionReady(1, 8, 1024);
    dispatch.submission().state(inactiveGeneration, inactiveReady);
    CHECK_TEST(dispatch.error() == NI_INVALID_STATE && inactiveGeneration == 0 && !inactiveReady);
    CHECK_TEST(dispatch.configurePollBudgets(commands, bytes) == NI_OK &&
        dispatch.configurePollBudgets(0, 1) == NI_INVALID_ARGUMENT &&
        dispatch.configurePollBudgets(1, 0) == NI_INVALID_ARGUMENT && dispatch.prepare(&events) == NI_OK &&
        dispatch.configurePollBudgets(1, 1) == NI_INVALID_STATE);
    CHECK_TEST(dispatch.started(1, 7) == NI_INVALID_STATE);
    dispatch.submission().state(inactiveGeneration, inactiveReady);
    CHECK_TEST(inactiveGeneration == 0 && !inactiveReady);
    hello.m_server = 0;
    hello.m_platform = 2;
    hello.m_device[0] = 2;
    uint32_t entropy = 37;
    NetInterfaceTlsOptions tls = {};
    tls.entropy = HelloEntropy;
    tls.entropyContext = &entropy;
    CHECK_TEST(client.bindChannel(&channel) == NI_OK &&
        client.connect("127.0.0.1", 9001, tls, hello, dispatch.listener()) == NI_OK &&
        dispatch.started(client.generation() + 1, 7) == NI_INVALID_STATE);
    dispatch.submission().state(inactiveGeneration, inactiveReady);
    CHECK_TEST(inactiveGeneration == 0 && !inactiveReady && dispatch.started(client.generation(), 7) == NI_OK &&
        dispatch.poll() && client.ready() && events.m_startResult == NI_INVALID_STATE &&
        dispatch.started(client.generation(), 7) == NI_INVALID_STATE);
    dispatch.submission().state(inactiveGeneration, inactiveReady);
    CHECK_TEST(inactiveGeneration == client.generation() && inactiveReady);
    const uint8_t body[] = {19,31};
    const NetInterfaceFrame event = {7, 99, 72, NI_EVENT, sizeof(body), body};
    channel.m_event = &event;
    CHECK_TEST(dispatch.poll() && channel.m_event == &event && events.m_frames > 0 && dispatch.error() == NI_OK);
    const unsigned refused = events.m_frames;
    channel.m_event = nullptr;
    for (uint32_t i = 1; i <= 5; ++i)
    {
        const NetInterfaceFrame data = {7, i, 9, NI_DATA, sizeof(body), body};
        CHECK_TEST(dispatch.submission().data(data) == NI_OK);
    }
    NetExecution owner;
    NetExecutionScope scope(owner);
    for (size_t count = 0; count < 5; count += expected)
    {
        owner.begin();
        CHECK_TEST(dispatch.poll() && channel.m_dataIds.size() == (std::min)(count + expected, size_t(5)) &&
            dispatch.error() == NI_OK);
        if (count + expected <= 5) { CHECK_TEST(owner.wait(5000) == NI_OK && waited == 0); }
    }
    owner.begin();
    CHECK_TEST(dispatch.poll() && owner.wait(5000) == NI_OK && waited == 5000);
    uint64_t id = 0;
    if (commands == 10 && bytes == 3)
    {
        const uint32_t prior = events.m_timeouts;
        const auto start = TimerProbeNow();
        CHECK_TEST(submission.begin(7, 11, 250, id) == NI_OK && id);
        owner.begin();
        CHECK_TEST(dispatch.poll() && owner.wait(5000) == NI_OK && waited <= 250);
        WaitTimerProbe(start + std::chrono::milliseconds(255));
        CHECK_TEST(dispatch.poll() && events.m_timeouts == prior + 1 && events.m_resultId == id &&
            submission.cancel(id) == NI_NOT_FOUND);
    }
    CHECK_TEST(submission.begin(7, 11, 60000, id) == NI_OK && id && submission.send(id, body, sizeof(body)) == NI_OK);
    for (unsigned i = 0; i < 32 && channel.m_body.empty(); ++i) { CHECK_TEST(dispatch.poll()); }
    CHECK_TEST(channel.m_body.size() == sizeof(body) && !std::memcmp(channel.m_body.data(), body, sizeof(body)) &&
        submission.cancel(id) == NI_OK);
    for (unsigned i = 0; i < 8; ++i) { CHECK_TEST(dispatch.poll()); }
    CHECK_TEST(submission.cancel(id) == NI_NOT_FOUND);
    NetInterfaceFrame sixth = {7, 6, 9, NI_DATA, sizeof(body), body};
    CHECK_TEST(submission.begin(7, 13, 60000, id) == NI_OK);
#if defined(__EMSCRIPTEN__)
    CHECK_TEST(dispatch.sendRequest(id, body, sizeof(body)) == NI_NOT_SUPPORTED && dispatch.sendData(sixth) == NI_NOT_SUPPORTED);
#else
    CHECK_TEST(dispatch.sendRequest(id, body, sizeof(body)) == NI_OK && dispatch.sendData(sixth) == NI_OK &&
        channel.m_dataIds.size() == 6 && channel.m_dataIds.back() == 6);
#endif
    submission.advanceSynchronous(NI_OK);
    events.m_throwFrame = true;
    channel.m_event = &event;
    dispatch.poll();
    CHECK_TEST(dispatch.error() == NI_INVALID_STATE && events.m_frames == refused + 1);
    return 0;
}

/** 比较同代请求在协议层和提交队列中遇到关闭时的业务终态
@return 两个原始编号各自结算一次，终态仅由关闭发起方和原因决定时为0
*/
static int CheckDispatchClose()
{
    const int32_t reasons[] = {NI_OK, NI_CANCELLED, NI_INVALID_STATE};
    NetClientSession client;
    NetHeartbeatConfig heartbeat;
    heartbeat.m_pingIntervalMs = 64;
    CHECK_TEST(client.configureHeartbeat(heartbeat) == NI_OK);
    for (unsigned origin = 0; origin != 6; ++origin)
    {
        for (unsigned index = 0; index != (origin == 3 || origin == 5 ? 1U : 3U); ++index)
        {
            const int32_t reason = origin == 5 ? NI_CANCELLED : reasons[index];
            const uint32_t expected = ((origin >= 1 && origin <= 3 && index < 2) || origin == 5) ?
                NI_RPC_CANCELLED : NI_RPC_UNAVAILABLE;
            NetInterfaceHandshakeOptions hello = {};
            hello.m_server = hello.m_capabilities = 1;
            hello.m_maxRequests = 8;
            hello.m_maxChunk = 1024;
            hello.m_timeoutMs = 5000;
            hello.m_nonce[0] = 19;
            NetInterfaceHandshake server(hello, 0);
            SessionProbeChannel channel(server);
            CiphertextProbeHost events;
            if (origin == 4)
            {
                events.m_closeOnResult = &client;
            }
            NetSenderDispatch dispatch(&client);
            // active借用本次分派器，离开场景时收敛会话，不返回结果。
            const auto stop = [](NetSenderDispatch* active) { active->shutdown(); };
            std::unique_ptr<NetSenderDispatch, decltype(stop)> lifetime(&dispatch, stop);
            uint32_t entropy = 37;
            NetInterfaceTlsOptions tls = {};
            tls.entropy = HelloEntropy;
            tls.entropyContext = &entropy;
            hello.m_server = 0;
            hello.m_platform = hello.m_device[0] = 1;
            CHECK_TEST(dispatch.prepare(&events) == NI_OK && client.bindChannel(&channel) == NI_OK &&
                client.connect("127.0.0.1", 9001, tls, hello, dispatch.listener()) == NI_OK &&
                dispatch.started(client.generation(), 7) == NI_OK && dispatch.poll() && client.ready());
            uint64_t imported = 0, queued = 0;
            const uint8_t body[] = {1, 2};
            CHECK_TEST(dispatch.submission().begin(7, 11, 60000, imported) == NI_OK && imported &&
                dispatch.submission().send(imported, body, sizeof(body)) == NI_OK &&
                dispatch.poll() && events.m_results == 0 && !channel.m_body.empty() &&
                dispatch.submission().begin(7, 12, 60000, queued) == NI_OK && queued != imported);
            if (origin == 0 || origin == 4)
            {
                channel.m_disconnectReason = reason;
            }
            else if (origin == 1)
            {
                client.close(reason);
            }
            else if (origin == 2)
            {
                dispatch.submission().close(reason);
            }
            else if (origin == 3)
            {
                dispatch.shutdown();
            }
            else
            {
                CHECK_TEST(dispatch.submission().send(queued, body, sizeof(body)) == NI_OK);
                channel.m_pingResult = NI_CANCELLED;
                WaitTimerProbe(TimerProbeNow() + std::chrono::milliseconds(heartbeat.m_pingIntervalMs + 1));
            }
            dispatch.poll();
            if (events.m_firstResultStatus != expected || events.m_resultStatus != expected)
            {
                std::fprintf(stderr, "close origin=%u reason=%d first=%u queued=%u expected=%u\n",
                    origin, reason, events.m_firstResultStatus, events.m_resultStatus, expected);
            }
            CHECK_TEST(events.m_results == 2 && events.m_firstResultId == imported && events.m_resultId == queued);
            CHECK_TEST(events.m_firstResultStatus == expected);
            CHECK_TEST(events.m_resultStatus == expected);
            CHECK_TEST(events.m_closes == 1 && events.m_closed == reason && dispatch.quiescent());
            dispatch.shutdown();
            CHECK_TEST(events.m_results == 2 && events.m_closes == 1 &&
                dispatch.submission().cancel(imported) == NI_NOT_FOUND &&
                dispatch.submission().cancel(queued) == NI_NOT_FOUND);
        }
    }
    return 0;
}

/** 对比命令预算与字节预算，两者先耗尽的决定交付边界
@return 各个独立实例通过时为0
*/
static int OwnerBudgets()
{
    CHECK_TEST(CheckDispatchClose() == 0);
    return WithWaitProbe([](const uint32_t& waited) -> int {
        return CheckDispatchBudget(1, 100, 1, waited) || CheckDispatchBudget(10, 3, 2, waited) ||
            CheckDispatchBudget(10, 1, 1, waited);
    });
}

/** 独立校验文件确认帧的网络字节序、负数结果和非对齐字节缓冲
@return 发送字节匹配样本且接收方能独立解析手写样本时为0
*/
static int FileAckWireOrder()
{
    uint8_t expected[NET_INTERFACE_FILE_ACK_SIZE] = {
        'N','F','A','2', 0,0,0,2, 0xfe,0xfd,0xfc,0xfc, 0,0,0,0, 1,2,3,4,5,6,7,8};
    expected[24] = 0xa5;
    expected[87] = 0x5a;
    NetInterfaceFileAck value = {};
    value.flags = 2;
    value.result = -0x01020304;
    value.offset = UINT64_C(0x0102030405060708);
    value.token[0] = 0xa5;
    value.hash[31] = 0x5a;

    uint64_t storage[(NET_INTERFACE_FILE_ACK_SIZE + 9) / 8];
    uint8_t* guarded = reinterpret_cast<uint8_t*>(storage);
    std::memset(guarded, 0xcc, sizeof(storage));
    uint8_t* wire = guarded + 1;
    CHECK_TEST(NetInterfaceFile::encodeAck(value, wire, sizeof(expected)) == NI_OK &&
        guarded[0] == 0xcc && wire[sizeof(expected)] == 0xcc && !std::memcmp(wire, expected, sizeof(expected)));
    std::memcpy(wire, expected, sizeof(expected));
    const NetInterfaceFrame frame = {7, 1, NI_FILE_DATA_TYPE, NI_DATA_ACK, sizeof(expected), wire};
    NetInterfaceFileAck parsed = {};
    CHECK_TEST(NetInterfaceFile::parseAck(frame, parsed) == NI_OK && parsed.flags == 2 &&
        parsed.result == -0x01020304 && parsed.offset == UINT64_C(0x0102030405060708) &&
        !std::memcmp(parsed.token, value.token, sizeof(parsed.token)) && !std::memcmp(parsed.hash, value.hash, sizeof(parsed.hash)));
    return 0;
}

/** 文件协议门面核对线格式、恢复前缀、宿主读长度和取消终态
@return 全部符合预期时返回0
*/
static int FileProtocolFacade()
{
    CHECK_TEST(FileAckWireOrder() == 0);
    NetFileProtocol protocol;
    uint8_t payload[1026] = {17, 29, 43};
    payload[1025] = 97;
    NetInterfaceFileInfo info = {};
    info.direction = 1;
    info.state = NI_FILE_TRANSFERRING;
    info.chunk = 1024;
    info.window = 1;
    info.total = sizeof(payload);
    info.offset = 1024;
    info.stream = 17;
    info.expiresUtc = 1000;
    info.transfer[0] = 3;
    info.token[0] = 5;
    CHECK_TEST(NetInterface::sha256(payload, sizeof(payload), info.hash, sizeof(info.hash)) == NI_OK);
    uint8_t wire[NET_INTERFACE_FILE_INFO_SIZE] = {};
    CHECK_TEST(NetInterfaceFile::encodeInfo(info, wire, sizeof(wire)) == NI_OK);
    NetInterfaceFileInfo parsed = {};
    CHECK_TEST(protocol.fileParseInfo(wire, sizeof(wire) - 1, &parsed) < 0 && !parsed.stream);
    CHECK_TEST(protocol.fileParseInfo(wire, sizeof(wire), &parsed) == NI_OK && parsed.offset == 1024 && parsed.stream == 17);
    uint32_t fields[12] = {};
    uint8_t transfer[16], token[32], hash[32];
    CHECK_TEST(protocol.fileInfoFields(wire, sizeof(wire), fields, transfer, token, hash) == NI_OK &&
        fields[4] == sizeof(payload) && fields[6] == 1024 && fields[8] == 17 && transfer[0] == 3 && token[0] == 5 &&
        !std::memcmp(hash, info.hash, sizeof(hash)));
    const uint32_t control[] = {NI_FILE_OPEN_UPLOAD, 1024, 1, 3, 0};
    uint8_t emptyToken[32] = {};
    uint32_t size = 0;
    CHECK_TEST(protocol.fileControlEncode(control, transfer, emptyToken, hash, payload, 3, nullptr, 0, nullptr, 0, &size) ==
        NI_BUFFER_TOO_SMALL && size == NET_INTERFACE_FILE_CONTROL_HEADER_SIZE + 3);
    std::vector<uint8_t> packet(size);
    CHECK_TEST(protocol.fileControlEncode(control, transfer, emptyToken, hash, payload, 3, nullptr, 0, packet.data(), size, &size) == NI_OK);
    NetInterfaceFileControl decoded = {};
    CHECK_TEST(NetInterfaceFile::parseControl(control[0], packet.data(), size, decoded) == NI_OK && decoded.operation == control[0] &&
        decoded.keySize == 3 && !std::memcmp(decoded.key, payload, 3));
    NetInterfaceFileFlowHandle handle = nullptr;
    CHECK_TEST(protocol.fileFlowCreateWire(wire, sizeof(wire), 7, 2, 0, 0, 1000, &handle) == NI_INVALID_ARGUMENT && !handle);
    CHECK_TEST(protocol.fileFlowCreateWire(wire, sizeof(wire), 7, 1, 0, 0, 1000, &handle) == NI_OK && handle);
    const auto release = [&](NetInterfaceFileFlowHandle flow) { protocol.fileFlowDestroy(flow); };
    std::unique_ptr<NetInterfaceFileFlowOpaque, decltype(release)> owner(handle, release);
    CHECK_TEST(protocol.fileFlowRead(handle, fields) == NI_NEED_MORE && !fields[0] && !fields[1] && !fields[2]);
    CHECK_TEST(protocol.fileFlowPrime(handle, 1, 0, payload, 1) == NI_INVALID_ARGUMENT);
    CHECK_TEST(protocol.fileFlowPrime(handle, 0, 0, payload, 511) == NI_OK);
    CHECK_TEST(protocol.fileFlowPrime(handle, 511, 0, nullptr, 1) == NI_INVALID_ARGUMENT);
    CHECK_TEST(protocol.fileFlowPrime(handle, 0, 0, payload, 511) == NI_INVALID_ARGUMENT);
    CHECK_TEST(protocol.fileFlowProgress(handle, 1, 0) == NI_NEED_MORE);
    CHECK_TEST(protocol.fileFlowPrime(handle, 511, 0, payload + 511, 513) == NI_OK && protocol.fileFlowProgress(handle, 2, 0) == NI_OK);
    CHECK_TEST(protocol.fileFlowRead(handle, fields) == NI_READY && fields[0] == 1024 && fields[1] == 0 && fields[2] == 2);
    CHECK_TEST(protocol.fileFlowProduced(handle, 1024, 0, payload + 1024, 1) == NI_INVALID_ARGUMENT);
    CHECK_TEST(protocol.fileFlowProduced(handle, 1024, 0, payload + 1024, 2) == NI_OK);
    const uint8_t* output = nullptr;
    CHECK_TEST(protocol.fileFlowPeek(handle, fields, &output) == NI_READY && fields[0] == NI_DATA &&
        protocol.fileFlowConsume(handle) == NI_OK);
    uint32_t ackFields[] = {NI_DATA_ACK, 7, 17, 0, NI_FILE_DATA_TYPE, NET_INTERFACE_FILE_ACK_SIZE};
    NetInterfaceFileAck ack = {};
    ack.offset = sizeof(payload);
    std::memcpy(ack.token, token, sizeof(token));
    uint8_t ackWire[NET_INTERFACE_FILE_ACK_SIZE];
    CHECK_TEST(NetInterfaceFile::encodeAck(ack, ackWire, sizeof(ackWire)) == NI_OK &&
        protocol.fileFlowAccept(handle, ackFields, ackWire) == NI_OK);
    // 独立 SHA-256 向量覆盖恢复前缀及最后两个字节。
    const uint8_t expectedHash[32] = {
        0x3a, 0x01, 0xbd, 0xde, 0x02, 0x24, 0xb3, 0x59, 0xbe, 0x85, 0x54, 0x52, 0x9c, 0x8f, 0xd9, 0xd5,
        0x84, 0xcc, 0x83, 0x34, 0x9d, 0x29, 0xcb, 0xec, 0x75, 0x1d, 0x9d, 0xef, 0xae, 0x55, 0x43, 0xad};
    CHECK_TEST(protocol.fileFlowPeek(handle, fields, &output) == NI_READY && output && fields[0] == NI_DATA &&
        fields[5] == NET_INTERFACE_FILE_DATA_HEADER_SIZE && output[7] == 1 &&
        std::memcmp(output + 56, expectedHash, sizeof(expectedHash)) == 0);
    CHECK_TEST(protocol.fileFlowCancel(handle, NI_CANCELLED) == NI_OK);
    int32_t status = 0;
    CHECK_TEST(protocol.fileFlowSnapshotWire(handle, wire, sizeof(wire), &status) == NI_OK && status == NI_CANCELLED);
    CHECK_TEST(protocol.fileParseInfo(wire, sizeof(wire), &parsed) == NI_OK && parsed.state == NI_FILE_CANCELLED && parsed.stream == 0);
    return 0;
}

/** 推进一个静默的 TLS 服务端，比较协议结果和宿主收到的最长等待毫秒数
@param [in,out] server 原始 TLS 或带帧的连接
@param [in,out] owner 当前执行者
@param [in] waited 宿主最近接到的实际等待预算
@param [in] now 输入 TLS 的单调时间
@param [in] state 预期协议结果
@param [in] budget 预期剩余期限，60000表示本轮沿用宿主上限
@return 返回值和等待预算均正确时为true
*/
template <typename Server>
static bool SilentTlsStep(Server& server, NetExecution& owner, const uint32_t& waited, uint64_t now, int32_t state, uint32_t budget)
{
    const auto start = TimerProbeNow();
    owner.begin();
    const int32_t result = server.progress(now);
    if (owner.wait(60000) != NI_OK) { return false; }
    const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        TimerProbeNow() - start).count());
    if (result != state || waited > budget || waited + elapsed + 1 < budget)
    {
        std::fprintf(stderr, "silent TLS status %d expected %d, wait %u expected %u ms\n", result, state, waited, budget);
        return false;
    }
    return true;
}

/** 核对首次推进起算的真实握手期限，涵盖默认值、溢出边界、倒退及持续错误
@param [in] options 带有测试证书的服务端配置
@param [in] waited 宿主实际接到的等待预算
@return 原始及帧接口均按底层握手时钟登记期限时为0
*/
static int SilentTlsDeadlines(NetInterfaceTlsOptions options, const uint32_t& waited)
{
    CHECK_TEST(options.setHandshakeTimeout(100));
    NetInterfaceTls server(options), nearLimit(options);
    CHECK_TEST(options.setHandshakeTimeout(0));
    NetInterfaceConnection framed(options);
    // 首轮在没有事件宿主的情况下启动，随后进入作用域不能重新计时。
    CHECK_TEST(server.progress(0) == NET_INTERFACE_TLS_HANDSHAKE && framed.progress(42) == NET_INTERFACE_TLS_HANDSHAKE);
    NetExecution owner;
    NetExecutionScope scope(owner);
    CHECK_TEST(SilentTlsStep(server, owner, waited, 0, NET_INTERFACE_TLS_HANDSHAKE, 100));
    CHECK_TEST(SilentTlsStep(server, owner, waited, 25, NET_INTERFACE_TLS_HANDSHAKE, 75));
    CHECK_TEST(SilentTlsStep(server, owner, waited, 100, NET_INTERFACE_TLS_TIMEOUT, 60000));
    CHECK_TEST(SilentTlsStep(server, owner, waited, 101, NET_INTERFACE_TLS_TIMEOUT, 60000));
    CHECK_TEST(SilentTlsStep(framed, owner, waited, 42, NET_INTERFACE_TLS_HANDSHAKE, 10000));
    CHECK_TEST(SilentTlsStep(framed, owner, waited, 9042, NET_INTERFACE_TLS_HANDSHAKE, 1000));
    CHECK_TEST(SilentTlsStep(framed, owner, waited, 10042, NET_INTERFACE_TLS_TIMEOUT, 60000));
    CHECK_TEST(SilentTlsStep(nearLimit, owner, waited, UINT64_MAX - 49, NET_INTERFACE_TLS_HANDSHAKE, 100));
    CHECK_TEST(SilentTlsStep(nearLimit, owner, waited, UINT64_MAX, NET_INTERFACE_TLS_HANDSHAKE, 51));
    CHECK_TEST(SilentTlsStep(nearLimit, owner, waited, 0, NET_INTERFACE_TLS_ARGUMENT, 60000));
    return 0;
}

/** 使用有界密文输出的真实 TLS 双端，逐次检查原始 API 的等待预算及对端收到的字节
@param [in] clientOptions 客户端信任的测试 CA、身份名和随机源
@param [in] serverOptions 与客户端配套的服务器证书、密钥和随机源
@param [in] waited 等待宿主实际收到的毫秒数
@return 输入接纳、背压、交付与关闭各自推进正确时为0
*/
static int RawTlsOutput(const NetInterfaceTlsOptions& clientOptions, const NetInterfaceTlsOptions& serverOptions, const uint32_t& waited)
{
    CEncodeDecodeTlsQueueLimits limits;
    CHECK_TEST(limits.set(-1, 1024));
    NetInterfaceTls sender(clientOptions, limits), receiver(serverOptions);
    NetExecution owner;
    NetExecutionScope scope(owner);
    const auto waitFor = [&](uint32_t expected) { return owner.wait(5000) == NI_OK && waited == expected; };
    owner.begin();
    CHECK_TEST(sender.close() == NET_INTERFACE_TLS_STATE && waitFor(5000));
    uint64_t now = 0;
    for (; now < 3000 && (sender.status() != NET_INTERFACE_TLS_READY || receiver.status() != NET_INTERFACE_TLS_READY); ++now)
    {
        CHECK_TEST(sender.progress(now) >= 0 && receiver.progress(now) >= 0 &&
            TransferCiphertext(sender, receiver) && TransferCiphertext(receiver, sender));
    }
    CHECK_TEST(sender.status() == NET_INTERFACE_TLS_READY && receiver.status() == NET_INTERFACE_TLS_READY);
    CHECK_TEST(SilentTlsStep(sender, owner, waited, ++now, NET_INTERFACE_TLS_READY, 60000));
    const uint8_t* cipher = nullptr;
    uint32_t size = 0, count = 99;
    for (unsigned step = 0; step < 32; ++step)
    {
        CHECK_TEST(sender.peekOutput(cipher, size) == NET_INTERFACE_TLS_OK);
        if (!size) { break; }
        CHECK_TEST(TransferCiphertext(sender, receiver));
    }
    owner.begin();
    CHECK_TEST(sender.write(nullptr, 0, count) == NET_INTERFACE_TLS_OK && !count &&
        sender.append(nullptr, 0, count) == NET_INTERFACE_TLS_OK && !count && sender.appendExact(nullptr, 0) == NET_INTERFACE_TLS_OK &&
        sender.appendExact(nullptr, 0, nullptr, 0) == NET_INTERFACE_TLS_OK && sender.consumeOutput(0) == NET_INTERFACE_TLS_OK &&
        sender.consumeOutput(1) == NET_INTERFACE_TLS_ARGUMENT && sender.read(nullptr, 0, count) == NET_INTERFACE_TLS_OK && !count &&
        sender.appendExact(nullptr, 1) == NET_INTERFACE_TLS_ARGUMENT && sender.outputState() == NET_INTERFACE_TLS_OK &&
        sender.peekOutput(cipher, size) == NET_INTERFACE_TLS_OK && !size && waitFor(5000));

    std::vector<uint8_t> expected(16384, 0x79);
    for (unsigned i = 0; i < 5; ++i) { expected[i] = static_cast<uint8_t>(0x90 + i); }
    owner.begin();
    CHECK_TEST(sender.write(expected.data(), 1, count) == NET_INTERFACE_TLS_OK && count == 1 && waitFor(0));
    owner.begin();
    CHECK_TEST(sender.append(expected.data() + 1, 1, count) == NET_INTERFACE_TLS_OK && count == 1 && waitFor(0));
    owner.begin();
    CHECK_TEST(sender.appendExact(expected.data() + 2, 1) == NET_INTERFACE_TLS_OK && waitFor(0));
    owner.begin();
    CHECK_TEST(sender.appendExact(expected.data() + 3, 1, expected.data() + 4, 1) == NET_INTERFACE_TLS_OK && waitFor(0));
    std::vector<uint8_t> tail(16384, 0x79);
    owner.begin();
    CHECK_TEST(sender.append(tail.data(), 16384, count) == NET_INTERFACE_TLS_OK && count == 16379 && waitFor(0));
    owner.begin();
    CHECK_TEST(sender.write(tail.data(), 1, count) == NET_INTERFACE_TLS_WANT_IO && !count &&
        sender.append(tail.data(), 1, count) == NET_INTERFACE_TLS_WANT_IO && !count && waitFor(5000));
    owner.begin();
    CHECK_TEST(sender.appendExact(tail.data(), 1) == NET_INTERFACE_TLS_WANT_IO && waitFor(0));
    owner.begin();
    CHECK_TEST(sender.appendExact(tail.data(), 1) == NET_INTERFACE_TLS_WANT_IO && waitFor(5000));
    owner.begin();
    CHECK_TEST(sender.close() == NET_INTERFACE_TLS_WANT_IO && waitFor(0));
    owner.begin();
    CHECK_TEST(sender.close() == NET_INTERFACE_TLS_WANT_IO && sender.write(tail.data(), 1, count) == NET_INTERFACE_TLS_STATE &&
        !count && waitFor(5000));

    uint32_t delivered = 0;
    int32_t closed = NET_INTERFACE_TLS_WANT_IO;
    for (unsigned step = 0; step < 128 && receiver.status() != NET_INTERFACE_TLS_CLOSED; ++step)
    {
        CHECK_TEST(sender.peekOutput(cipher, size) == NET_INTERFACE_TLS_OK);
        if (size)
        {
            CHECK_TEST(receiver.feed(cipher, size, count) == NET_INTERFACE_TLS_OK && count == size);
            owner.begin();
            CHECK_TEST(sender.consumeOutput(size) == NET_INTERFACE_TLS_OK && waitFor(0));
        }
        closed = sender.close();
        CHECK_TEST(closed == NET_INTERFACE_TLS_WANT_IO || closed == NET_INTERFACE_TLS_OK);
        uint8_t clear[1024];
        owner.begin();
        const int32_t read = receiver.read(clear, sizeof(clear), count);
        CHECK_TEST(read >= 0 && count <= expected.size() - delivered &&
            !std::memcmp(expected.data() + delivered, clear, count) && waitFor(count ? 0 : 5000));
        delivered += count;
    }
    CHECK_TEST(delivered == expected.size() && receiver.status() == NET_INTERFACE_TLS_CLOSED && closed == NET_INTERFACE_TLS_OK);
    CHECK_TEST(SilentTlsStep(receiver, owner, waited, ++now, NET_INTERFACE_TLS_CLOSED, 60000));
    owner.begin();
    CHECK_TEST(sender.close() == NET_INTERFACE_TLS_OK && sender.outputState() == NET_INTERFACE_TLS_OK &&
        sender.peekOutput(cipher, size) == NET_INTERFACE_TLS_OK && !size && waitFor(5000));
    return 0;
}

/** 通过不透明句柄完成双向 TLS 握手、输入输出交接和普通帧解码
@return 全部符合预期时返回0
*/
static int TlsHandles()
{
    static const uint8_t ca[] = TEST_CA_CRT_RSA_SHA256_PEM, crt[] = TEST_SRV_CRT_RSA_SHA256_PEM, key[] = TEST_SRV_KEY_RSA_PEM;
    uint32_t seeds[2] = {13, 17};
    NetInterfaceTlsOptions cli = {}, srv = {};
    cli.entropy = srv.entropy = HelloEntropy;
    cli.entropyContext = &seeds[0];
    srv.entropyContext = &seeds[1];
    cli.authority = ca; cli.authoritySize = sizeof(ca); cli.hostname = "localhost";
    srv.server = 1; srv.certificate = crt; srv.certificateSize = sizeof(crt);
    srv.privateKey = key; srv.privateKeySize = sizeof(key);
    const int scheduling = WithWaitProbe([&](const uint32_t& waited) -> int {
        CHECK_TEST(SilentTlsDeadlines(srv, waited) == 0);
        return RawTlsOutput(cli, srv, waited);
    });
    CHECK_TEST(scheduling == 0);
    NetInterfaceConnectionHandle client = nullptr, server = nullptr;
    int32_t detail = 73;
    CHECK_TEST(NetInterface::connectionCreate(nullptr, &client, &detail) == NI_INVALID_ARGUMENT && !client && !detail);
    CHECK_TEST(NetInterface::connectionCreate(&cli, &client, &detail) == NET_INTERFACE_TLS_HANDSHAKE && client);
    std::unique_ptr<NetInterfaceConnectionOpaque, void (*)(NetInterfaceConnectionHandle)> left(client, NetInterface::connectionDestroy);
    CHECK_TEST(NetInterface::connectionCreate(&srv, &server, &detail) == NET_INTERFACE_TLS_HANDSHAKE && server);
    std::unique_ptr<NetInterfaceConnectionOpaque, void (*)(NetInterfaceConnectionHandle)> right(server, NetInterface::connectionDestroy);
    const auto move = [](NetInterfaceConnectionHandle from, NetInterfaceConnectionHandle to) -> bool {
        const uint8_t* bytes = nullptr;
        uint32_t size = 0, used = 0;
        if (NetInterface::connectionPeekOutput(from, &bytes, &size) < 0) { return false; }
        return !size || (bytes && NetInterface::connectionFeed(to, bytes, size, &used) >= 0 && used <= size &&
            NetInterface::connectionConsumeOutput(from, used) == NI_OK);
    };
    const uint8_t body[] = {9, 0, 77};
    uint32_t fields[] = {NI_REQUEST, 19, 41, 3, 7, sizeof(body)};
    CHECK_TEST(NetInterface::connectionSend(client, fields, body) == NI_INVALID_STATE);
    for (unsigned i = 0; i < 1024 && (NetInterface::connectionGetStatus(client, nullptr) != NET_INTERFACE_TLS_READY ||
        NetInterface::connectionGetStatus(server, &detail) != NET_INTERFACE_TLS_READY); ++i)
    {
        CHECK_TEST(NetInterface::connectionProgress(client, i, 1) >= 0 && NetInterface::connectionProgress(server, i, 1) >= 0 &&
            move(client, server) && move(server, client));
    }
    CHECK_TEST(NetInterface::connectionGetStatus(client, &detail) == NET_INTERFACE_TLS_READY && detail == 0);
    CHECK_TEST(NetInterface::connectionSend(client, fields, body) == NI_OK);
    uint32_t received[6] = {};
    const uint8_t* data = nullptr;
    for (unsigned i = 0; i < 1024 && NetInterface::connectionPeek(server, received, &data) != NI_READY; ++i)
    {
        CHECK_TEST(NetInterface::connectionProgress(client, 1024 + i, 1) >= 0 && move(client, server) &&
            NetInterface::connectionProgress(server, 1024 + i, 1) >= 0);
    }
    CHECK_TEST(NetInterface::connectionPeek(server, received, &data) == NI_READY && data &&
        !std::memcmp(received, fields, sizeof(fields)) && !std::memcmp(data, body, sizeof(body)));
    CHECK_TEST(NetInterface::connectionConsume(server) == NI_OK && NetInterface::connectionPeek(server, received, &data) == NI_NEED_MORE &&
        !data && !received[5] && NetInterface::connectionFinishInput(server) >= 0);
    NetInterfaceTlsHandle tls = nullptr, listener = nullptr;
    CHECK_TEST(NetInterface::tlsCreateClient(ca, sizeof(ca), "localhost", HelloEntropy, seeds, 5000, &tls, &detail) ==
        NET_INTERFACE_TLS_HANDSHAKE && tls);
    std::unique_ptr<NetInterfaceTlsOpaque, void (*)(NetInterfaceTlsHandle)> sender(tls, NetInterface::tlsDestroy);
    CHECK_TEST(NetInterface::tlsCreate(&srv, &listener, &detail) == NET_INTERFACE_TLS_HANDSHAKE && listener);
    std::unique_ptr<NetInterfaceTlsOpaque, void (*)(NetInterfaceTlsHandle)> receiver(listener, NetInterface::tlsDestroy);
    const auto transfer = [](NetInterfaceTlsHandle from, NetInterfaceTlsHandle to) -> bool {
        const uint8_t* bytes = nullptr;
        uint32_t size = 0, used = 0;
        return NetInterface::tlsPeekOutput(from, &bytes, &size) >= 0 && (!size || (bytes &&
            NetInterface::tlsFeed(to, bytes, size, &used) >= 0 && used <= size && NetInterface::tlsConsumeOutput(from, used) == NET_INTERFACE_TLS_OK));
    };
    for (unsigned i = 0; i < 1024 && (NetInterface::tlsGetStatus(tls, &detail) != NET_INTERFACE_TLS_READY ||
        NetInterface::tlsGetStatus(listener, nullptr) != NET_INTERFACE_TLS_READY); ++i)
    {
        CHECK_TEST(NetInterface::tlsProgress(tls, i, 1) >= 0 && NetInterface::tlsProgress(listener, i, 1) >= 0 &&
            transfer(tls, listener) && transfer(listener, tls));
    }
    CHECK_TEST(NetInterface::tlsGetStatus(tls, &detail) == NET_INTERFACE_TLS_READY && !detail);
    uint32_t used = 0;
    CHECK_TEST(NetInterface::tlsWrite(tls, body, sizeof(body), &used) >= 0 && used == sizeof(body));
    uint8_t clear[5] = {1, 1, 1, 1, 1};
    for (unsigned i = 0; i < 32; ++i)
    {
        CHECK_TEST(NetInterface::tlsProgress(tls, 1024 + i, 1) >= 0 && transfer(tls, listener));
        CHECK_TEST(NetInterface::tlsRead(listener, clear + 1, sizeof(body), &used) >= 0);
        if (used) { break; }
    }
    CHECK_TEST(used == sizeof(body) && !std::memcmp(clear + 1, body, sizeof(body)) && clear[0] == 1 && clear[4] == 1);
    return WithWaitProbe([&](const uint32_t& waited) -> int {
        NetExecution owner;
        NetExecutionScope scope(owner);
        owner.begin();
        CHECK_TEST(NetInterface::tlsClose(tls) == NET_INTERFACE_TLS_OK && owner.wait(5000) == NI_OK && waited == 0);
        owner.begin();
        CHECK_TEST(NetInterface::tlsClose(tls) == NET_INTERFACE_TLS_OK && owner.wait(5000) == NI_OK && waited == 5000 &&
            transfer(tls, listener) && NetInterface::tlsFinishInput(listener) >= 0);
        return 0;
    });
}

/** 核对解码句柄的完整视图、短输入、消费、复位和终态
@return 全部符合预期时返回0
*/
static int DecoderHandleFields()
{
    NetInterfaceHandle handle = nullptr;
    CHECK_TEST(NetInterface::create(3, &handle) == NI_OK && handle);
    std::unique_ptr<NetInterfaceOpaque, void (*)(NetInterfaceHandle)> owner(handle, NetInterface::destroy);
    uint32_t used = 9, fields[6] = {};
    const uint8_t* data = kFrame;
    NetInterfaceFrame frame = {};
    CHECK_TEST(NetInterface::peek(nullptr, &frame) == NI_INVALID_ARGUMENT && !frame.m_payload &&
        NetInterface::peekFields(handle, fields, &data) == NI_NEED_MORE && !data);
    CHECK_TEST(NetInterface::feed(handle, kFrame, 1, &used) == NI_NEED_MORE && used == 1 &&
        NetInterface::finish(handle) == NI_TRUNCATED && NetInterface::reset(handle) == NI_OK);
    CHECK_TEST(NetInterface::feed(handle, kFrame, sizeof(kFrame), &used) == NI_READY && used == sizeof(kFrame));
    const uint32_t expected[] = {NI_REQUEST, 7, 0x05060708, 0x01020304, 9, 3};
    CHECK_TEST(NetInterface::peekFields(handle, fields, &data) == NI_READY && !std::memcmp(fields, expected, sizeof(fields)) &&
        data && !std::memcmp(data, kFrame + 32, 3) && NetInterface::peek(handle, &frame) == NI_READY &&
        frame.m_payload == data && frame.m_requestId == UINT64_C(0x0102030405060708));
    CHECK_TEST(NetInterface::consume(handle) == NI_OK && NetInterface::finish(handle) == NI_OK &&
        NetInterface::peekFields(handle, fields, &data) == NI_NEED_MORE && !data && !fields[5] &&
        NetInterface::reset(nullptr) == NI_INVALID_ARGUMENT && NetInterface::finish(nullptr) == NI_INVALID_ARGUMENT);
    return 0;
}

TestSubmissionObserver::TestSubmissionObserver() :
m_count(0)
{

}

void TestSubmissionObserver::notify()
{
    ++m_count;
}

TestAttachedSession::~TestAttachedSession()
{
    detachBindingsForDestruction();
}

TestReferenceNode::TestReferenceNode(unsigned& destroyed, bool reject) : m_destroyed(destroyed)
{
    if (reject) { throw std::runtime_error("construction refused"); }
}

TestReferenceNode::~TestReferenceNode()
{
    ++m_destroyed;
}

/** 验证固定容量池的构造回滚、嵌套所有权转交和可观察的实际析构次数
@return 所有用例条件成立返回0
*/
static int ObjectReferences()
{
    for (unsigned move = 0; move != 2; ++move)
    {
        unsigned destroyed = 0;
        LockFreeHashMapPoolConfig config;
        config.setInitialCapacity(2);
        config.setMaximumCapacity(2);
        NetObjectPool<TestReferenceNode> pool(config);
        NetObjectReference<TestReferenceNode> root, spare;
        CHECK_TEST(pool.status() == NI_OK && !pool.value(nullptr) && !root.get() && !root.storage() && !root.release());
        pool.retain(nullptr);
        pool.release(nullptr);
        CHECK_TEST(pool.create(root, destroyed, true) == NI_NO_MEMORY && !root && !destroyed &&
            pool.create(root, destroyed, false) == NI_OK && pool.create(root, destroyed, false) == NI_INVALID_ARGUMENT);
        CHECK_TEST(pool.create(root->m_child, destroyed, false) == NI_OK && pool.create(spare, destroyed, false) == NI_LIMIT_EXCEEDED);
        TestReferenceNode* child = root->m_child.get();
        if (move) { root = std::move(root->m_child); } else { root = root->m_child; }
        CHECK_TEST(root && root.get() == child && &*root == child && destroyed == 1 && pool.statistics().occupied == 1);
        NetObjectReference<TestReferenceNode> copy(root);
        NetObjectReference<TestReferenceNode> moved(std::move(copy));
        root = root;
        root = std::move(root);
        CHECK_TEST(!copy && root.get() == child && moved.get() == child);
        void* storage = moved.storage();
        CHECK_TEST(moved.release() == child && !moved && pool.value(storage) == child);
        root.reset();
        CHECK_TEST(pool.value(storage) == child && destroyed == 1 && pool.statistics().occupied == 1);
        pool.release(storage);
        CHECK_TEST(destroyed == 2 && !pool.statistics().occupied);
    }
    return 0;
}

/** 校验目录编码句柄、候选规范化、精确缓冲容量及证书申请的输出边界
@return 全部符合预期时返回0
*/
static int DirectoryHandles()
{
    NetInterfaceDirectoryPoint point;
    const NetInterfaceDirectoryNodeData node = DirectoryNode(0);
    point.m_name = node.m_name;
    point.m_address = node.m_address;
    point.m_port = node.m_port;
    point.m_capacity = node.m_capacity;
    point.m_boot = node.m_boot;
    CHECK_TEST(NetInterfaceDirectory::valid(point));
    NetInterfaceDirectoryHandle original = nullptr;
    NetInterfaceDirectoryNodeData invalid = node;
    std::memset(invalid.m_name, 'n', sizeof(invalid.m_name));
    CHECK_TEST(NetInterface::directoryCreateNodes(&invalid, 1, &original) == NI_INVALID_ARGUMENT && !original);
    invalid = node;
    std::memset(invalid.m_address, '1', sizeof(invalid.m_address));
    CHECK_TEST(NetInterface::directoryCreateNodes(&invalid, 1, &original) == NI_INVALID_ARGUMENT && !original);
    CHECK_TEST(NetInterface::directoryCreateNodes(&node, 1, &original) == NI_OK && original);
    std::unique_ptr<NetInterfaceDirectoryOpaque, void (*)(NetInterfaceDirectoryHandle)> encoded(original, NetInterface::directoryDestroy);
    uint32_t cacheSize = 0;
    CHECK_TEST(NetInterface::directoryCache(original, 1, 0, nullptr, 0, &cacheSize) == NI_BUFFER_TOO_SMALL && cacheSize > 48);
    std::vector<uint8_t> cache(cacheSize);
    CHECK_TEST(NetInterface::directoryCache(original, 1, 0, cache.data(), cacheSize, &cacheSize) == NI_OK);
    point.m_port = 65536;
    CHECK_TEST(!NetInterfaceDirectory::valid(point) && NetInterfaceDirectory::address("::1") &&
        !NetInterfaceDirectory::address("::") && !NetInterfaceDirectory::address("127.0.0.256"));
    std::string privateText("key"), requestText("request");
    CHECK_TEST(NetInterfaceCertificate::createRequest(privateText, nullptr, nullptr, privateText, requestText) ==
        NI_INVALID_ARGUMENT && privateText == "key" && requestText == "request");
    CHECK_TEST(NetInterfaceCertificate::createRequest(requestText, nullptr, nullptr, privateText, requestText) ==
        NI_INVALID_ARGUMENT && privateText == "key" && requestText == "request");
    CHECK_TEST(NetInterfaceCertificate::createRequest("node", nullptr, nullptr, privateText, privateText) ==
        NI_INVALID_ARGUMENT && privateText == "key");
    CHECK_TEST(NetInterfaceCertificate::createRequest("invalid.", nullptr, nullptr, privateText, requestText) ==
        NI_INVALID_ARGUMENT && privateText.empty() && requestText.empty());
    NetInterfaceDirectoryHandle handle = nullptr;
    // NGC3 的16字节头与32字节摘要之间是目录线上正文。
    CHECK_TEST(NetInterface::directoryCreate(cache.data() + 16, cacheSize - 48, &handle) == NI_OK && handle);
    std::unique_ptr<NetInterfaceDirectoryOpaque, void (*)(NetInterfaceDirectoryHandle)> owner(handle, NetInterface::directoryDestroy);
    char name[sizeof(node.m_name)] = {}, address[sizeof(node.m_address)] = {};
    uint32_t fields[4] = {};
    CHECK_TEST(NetInterface::directoryNode(handle, 1, name, sizeof(name), address, sizeof(address), fields) == NI_NOT_FOUND &&
        !fields[0] && NetInterface::directoryNode(handle, 0, name, static_cast<uint32_t>(point.m_name.size()), address,
        sizeof(address), fields) == NI_BUFFER_TOO_SMALL && !name[0]);
    CHECK_TEST(NetInterface::directoryNode(handle, 0, name, sizeof(name), address, sizeof(address), fields) == NI_OK &&
        !std::strcmp(name, node.m_name) && !std::strcmp(address, node.m_address) && fields[0] == node.m_port &&
        fields[1] == node.m_capacity && fields[2] == 0 && fields[3] == 1);
    NetClientDiscoveryPoint candidate = {sizeof(NetClientDiscoveryPoint), 1, "127.0.0.1", 65535};
    uint8_t key[16] = {};
    CHECK_TEST(NetClientDiscovery::validPoint(candidate) && NetClientDiscovery::addressKey(candidate, key) &&
        key[10] == 255 && key[11] == 255 && key[12] == 127 && key[15] == 1);
    std::memset(candidate.m_address, '1', sizeof(candidate.m_address));
    CHECK_TEST(!NetClientDiscovery::validPoint(candidate) && !NetClientDiscovery::addressKey(candidate, key) && key[12] == 127);
    uint8_t privateKey[NET_INTERFACE_IDENTITY_PEM_BYTES] = {}, request[NET_INTERFACE_IDENTITY_PEM_BYTES] = {};
    const uint8_t validName[] = {'n','o','d','e'};
    uint32_t keySize = 77, requestSize = 88, seed = 13;
    // 每种输入只让一对区域相交，其余区域分别独立。
    const unsigned keyOffsets[] = {2, 8, 4};
    const unsigned requestOffsets[] = {8, 2, 6};
    for (unsigned pair = 0; pair < 3; ++pair)
    {
        uint8_t storage[] = {'n','o','d','e',5,6,7,8,9,10,11,12};
        const uint8_t expected[] = {'n','o','d','e',5,6,7,8,9,10,11,12};
        uint8_t* key = storage + keyOffsets[pair];
        uint8_t* csr = storage + requestOffsets[pair];
        CHECK_TEST(NetInterface::certificateCreateRequest(storage, 4, HelloEntropy, &seed, key, 4, &keySize,
            csr, 4, &requestSize) == NI_INVALID_ARGUMENT && keySize == 77 && requestSize == 88 &&
            seed == 13 && !std::memcmp(storage, expected, sizeof(storage)));
    }
    CHECK_TEST(NetInterface::certificateCreateRequest(validName, sizeof(validName), nullptr, &seed, privateKey, sizeof(privateKey),
        &keySize, request, sizeof(request), &requestSize) == NI_INVALID_ARGUMENT && !keySize && !requestSize);
    const int32_t result = NetInterface::certificateCreateRequest(validName, sizeof(validName), HelloEntropy, &seed,
        privateKey, sizeof(privateKey), &keySize, request, sizeof(request), &requestSize);
    mbedtls_pk_context parsedKey;
    mbedtls_x509_csr parsedRequest;
    mbedtls_pk_init(&parsedKey);
    mbedtls_x509_csr_init(&parsedRequest);
    const auto randomBytes = [](void* state, unsigned char* output, size_t size) -> int {
        return size <= UINT32_MAX ? HelloEntropy(state, output, static_cast<uint32_t>(size)) : -1;
    };
    const bool pairMatches = result == NI_OK && keySize > 1 && keySize <= sizeof(privateKey) &&
        requestSize > 1 && requestSize <= sizeof(request) && privateKey[keySize - 1] == 0 && request[requestSize - 1] == 0 &&
        mbedtls_pk_parse_key(&parsedKey, privateKey, keySize, nullptr, 0, randomBytes, &seed) == 0 &&
        mbedtls_x509_csr_parse(&parsedRequest, request, requestSize) == 0 &&
        parsedRequest.subject.next == nullptr && parsedRequest.subject.oid.len == MBEDTLS_OID_SIZE(MBEDTLS_OID_AT_CN) &&
        std::memcmp(parsedRequest.subject.oid.p, MBEDTLS_OID_AT_CN, parsedRequest.subject.oid.len) == 0 &&
        parsedRequest.subject.val.len == sizeof(validName) && std::memcmp(parsedRequest.subject.val.p, validName, sizeof(validName)) == 0 &&
        mbedtls_pk_check_pair(&parsedRequest.pk, &parsedKey, randomBytes, &seed) == 0;
    mbedtls_x509_csr_free(&parsedRequest);
    mbedtls_pk_free(&parsedKey);
    NetInterface::wipe(privateKey, sizeof(privateKey));
    CHECK_TEST(pairMatches);
    return 0;
}

/** 核对请求双向索引、端点注册及通道初始化前的边界
@return 全部符合预期时返回0
*/
static int RoutingBoundaries()
{
    NetInterfaceRequests requests;
    CHECK_TEST(requests.status() == NI_OK && requests.addEndpoint(10, false, 0, 2) == NI_OK &&
        requests.addEndpoint(20, true, 7, 2) == NI_OK);
    NetInterfaceRequest created = {}, actual = {};
    CHECK_TEST(requests.begin(10, 41, 20, 7, 11, 0, 50, 5, created) == NI_OK && created.m_route &&
        created.m_source == 10 && created.m_original == 41 && created.m_deadline == 55);
    CHECK_TEST(requests.addEndpoint(30, true, 7, 1) == NI_OK);
    NetInterfaceRequest child = {};
    CHECK_TEST(requests.begin(20, 100, 30, 7, 8, created.m_route, 50, 15, child) == NI_OK &&
        child.m_origin == 10 && child.m_parent == created.m_route && child.m_deadline == created.m_deadline &&
        requests.takeChild(created.m_route, actual) == NI_READY && actual.m_original == 100);
    CHECK_TEST(requests.parentContext(20, created.m_route, 8, actual) == NI_NOT_FOUND && !actual.m_route &&
        requests.parentContext(20, created.m_route, 7, actual) == NI_READY && actual.m_origin == 10);
    CHECK_TEST(requests.completeOriginal(10, 41, 20, 7, 12, 51, actual) == NI_PROTOCOL_ERROR && !actual.m_route &&
        requests.completeOriginal(10, 41, 20, 7, 11, 52, actual) == NI_READY && actual.m_route == created.m_route &&
        requests.completeOriginal(10, 41, 20, 7, 11, 53, actual) == NI_NOT_FOUND && !actual.m_source);
    RouterFileSink sink;
    NetEndpointRouter router(&sink);
    uint64_t endpoint = 42;
    CHECK_TEST(!router.localPort() && router.addHandler(0, 0, endpoint) == NI_INVALID_ARGUMENT && !endpoint &&
        router.addHandler(0, 1, endpoint) == NI_OK && endpoint);
    uint64_t duplicate = 99;
    CHECK_TEST(router.addHandler(0, 1, duplicate) == NI_INVALID_ARGUMENT && !duplicate);
    router.shutdown();
    NetGatewayClientChannel gateway(7);
    CiphertextProbeHost host;
    CiphertextProbeTransport transport(host);
    NetConnectionPoolConfig pools;
    CHECK_TEST(transport.NetGatewayTransport::outputState(1) == NI_NOT_SUPPORTED &&
        gateway.NetClientChannel::configureCompression(1, nullptr, 0, false) == NI_NOT_SUPPORTED &&
        gateway.NetClientChannel::outputState(1) == NI_NOT_SUPPORTED &&
        gateway.NetClientChannel::configureConnectionPools(pools) == NI_NOT_SUPPORTED);
    CHECK_TEST(gateway.configureConnectionPools(pools) == NI_OK);
    pools.m_controlBurst = 0;
    CHECK_TEST(gateway.configureConnectionPools(pools) == NI_INVALID_ARGUMENT);
    const NetInterfaceFrame frame = {};
    CHECK_TEST(gateway.send(1, frame) == NI_INVALID_STATE && gateway.outputState(1) == NI_INVALID_STATE &&
        gateway.configureCompact(1, true) == NI_INVALID_STATE && gateway.configureCompression(1, nullptr, 0, false) == NI_INVALID_STATE);
    TestAttachedSession session;
    CHECK_TEST(!session.hasBoundChannel() && !session.boundGatewayFactory() &&
        session.bindGateway(&host, nullptr) == NI_OK && session.boundGatewayFactory() == &host &&
        session.bindChannel(&gateway) == NI_OK && session.hasBoundChannel());
    CHECK_TEST(!session.serviceMode() && !session.generation() && session.beginRequestAt(0, 7, 3, 0, 1) == NI_INVALID_ARGUMENT &&
        session.beginRequestAt(1, 7, 3, 0, 1) == NI_INVALID_STATE);
    return 0;
}

/** 反复启动两个代次，只有代号1完成握手；线程宿主另从读者观察就绪快照
@return 全部快照与真实握手代次相符时为0
*/
static int SubmissionSnapshots()
{
    NetSenderSubmission submission;
    CHECK_TEST(submission.start(1, 7) == NI_OK && submission.ready(1, 2, 1024) == NI_OK);
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    std::atomic<bool> reading(false), stop(false), invalid(false);
    std::thread reader([&]() {
        do
        {
            uint64_t generation = 0;
            bool ready = false;
            submission.state(generation, ready);
            if (generation == 0 || generation > 2 || (ready && generation != 1)) { invalid.store(true); }
            reading.store(true);
        } while (!stop.load());
    });
    while (!reading.load()) { std::this_thread::yield(); }
#endif
    bool ok = true;
    try
    {
        for (unsigned round = 0; ok && round < 128; ++round)
        {
            uint64_t generation = 0;
            bool ready = true;
            ok = submission.start(2, 7) == NI_OK;
            submission.state(generation, ready);
            ok = ok && generation == 2 && !ready && submission.start(1, 7) == NI_OK &&
                submission.ready(1, 2, 1024) == NI_OK;
            submission.state(generation, ready);
            ok = ok && generation == 1 && ready;
        }
    }
    catch (...) { ok = false; }
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    stop.store(true);
    reader.join();
    ok = ok && !invalid.load();
#endif
    CHECK_TEST(ok);
    return 0;
}

/** 核对多生产者同时预留的请求号、唯一性和关闭后的输出
@return 全部符合预期时返回0
*/
static int SubmissionProducers()
{
    CHECK_TEST(SubmissionSnapshots() == 0);
#if defined(NET_INTERFACE_SDK_STATIC_LINK) || defined(__EMSCRIPTEN__)
    {
        NetSenderSubmissionPools pools;
        pools.m_requestIndex.setProtectionRecordCount(1);
        std::unique_ptr<NetSenderSubmissionState> owner(new NetSenderSubmissionState(pools, NetSenderSubmissionOptions()));
        NetSenderSubmissionState& state = *owner;
        NetObjectReference<NetSenderSubmissionFront> front;
        NetSenderSubmissionItem begin, send;
        CHECK_TEST(state.m_initializationResult == NI_OK && state.m_frontsPool.create(front) == NI_OK &&
            state.m_inputsPool.create(begin.m_input, state.m_inputCount) == NI_OK &&
            state.m_requestsPool.create(begin.m_input->m_entry) == NI_OK &&
            state.m_inputsPool.create(send.m_input, state.m_inputCount) == NI_OK);
        const uint8_t body[] = {0x12, 0, 0xff};
        CHECK_TEST(state.m_payloadsPool.create(send.m_input->m_payload, state.m_bodies, state.m_bytes,
            body, sizeof(body)) == NI_OK && send.m_input->m_payload->m_status == NI_OK);
        NetSenderSubmissionEntry* entry = begin.m_input->m_entry.get();
        entry->m_storage = begin.m_input->m_entry.storage();
        entry->m_pool = &state.m_requestsPool;
        entry->m_command.m_kind = NET_SUBMIT_BEGIN;
        entry->m_command.m_id = 7;
        entry->m_front = front;
        front->m_command = entry->m_command;
        front->m_published.store(true);
        CHECK_TEST(state.m_fronts.set(7, front));
        ++state.m_requestCount;
        begin.m_input->m_kind = NET_SUBMIT_BEGIN;
        state.importOwned(begin);
        send.m_input->m_kind = NET_SUBMIT_SEND;
        send.m_input->m_front = front;
        // 遍历回调占用唯一保护记录；无须依赖两个线程恰好同时访问。
        bool accepted = false;
        CHECK_TEST(state.m_fronts.forEach([&](const uint64_t&, const NetObjectReference<NetSenderSubmissionFront>&) {
            NetObjectReference<NetSenderSubmissionFront> result;
            const int32_t lookup = state.lookupFront(7, result);
            state.importOwned(send);
            accepted = lookup == NI_LIMIT_EXCEEDED && !result && state.find(7) == entry &&
                state.m_pending == entry && entry->m_size == sizeof(body) && entry->m_input &&
                std::memcmp(entry->m_input, body, sizeof(body)) == 0;
        }) == 1 && accepted);
        NetSenderSubmissionCommand command = {};
        state.take(entry, command);
        CHECK_TEST(!front->m_entry && command.m_id == 7 && !state.m_requests && !state.m_pending &&
            state.m_bytes.load() == 0);
        // 真实并发不保证 CAS 必然冲突，使用同一实例核对默认竞争让步周期。
        uint32_t attempts = 0;
        for (uint32_t i = 1; i <= 65; ++i)
        {
            state.pauseContention(attempts);
            CHECK_TEST(attempts == i % 64);
        }
    }
#endif
    const unsigned kWorkers = 4;
    const unsigned kRequests = 64;
    NetSenderSubmission submission;
    CHECK_TEST(submission.start(7, 11) == NI_OK && submission.ready(7, kWorkers * kRequests, 1024) == NI_OK);
    uint64_t ids[kWorkers * kRequests] = {};
    bool ok[kWorkers] = {};
    const auto run = [&](unsigned worker) {
        try
        {
            for (unsigned i = 0; i < kRequests; ++i)
            {
                if (submission.begin(11, worker + 1, 60000, ids[worker * kRequests + i]) != NI_OK) { return; }
            }
            ok[worker] = true;
        }
        catch (...) { ok[worker] = false; }
    };
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    for (unsigned i = 0; i < kWorkers; ++i) { run(i); }
#else
    std::thread threads[kWorkers];
    std::atomic<bool> start(false);
    try
    {
        for (unsigned i = 0; i < kWorkers; ++i)
        {
            threads[i] = std::thread([&, i]() {
                while (!start.load(std::memory_order_acquire)) { std::this_thread::yield(); }
                run(i);
            });
        }
    }
    catch (...) { /* 已经启动的生产者仍须放行并回收。 */ }
    start.store(true, std::memory_order_release);
    for (unsigned i = 0; i < kWorkers; ++i) { if (threads[i].joinable()) { threads[i].join(); } }
#endif
    for (unsigned i = 0; i < kWorkers; ++i) { CHECK_TEST(ok[i]); }
    std::sort(ids, ids + kWorkers * kRequests);
    for (unsigned i = 0; i < kWorkers * kRequests; ++i) { CHECK_TEST(ids[i] == i + 1); }
    submission.close(NI_CANCELLED);
    uint64_t denied = 99;
    CHECK_TEST(submission.begin(11, 1, 1, denied) == NI_INVALID_STATE && !denied);
    return 0;
}

/** 从平台参考时钟直接取样，Wasm沿用本测试已有的单调时钟链接包装
@param [out] milliseconds 成功时为与被测平台匹配的单调毫秒
@return 平台取样成功返回true
*/
static bool ReferenceMonotonicMilliseconds(uint64_t& milliseconds)
{
#if defined(_WIN32) && defined(_MSC_VER) && _MSC_VER <= 1800
    milliseconds = ::GetTickCount64();
#elif defined(__EMSCRIPTEN__)
    struct timespec value = {};
    if (::clock_gettime(CLOCK_MONOTONIC, &value) != 0) { return false; }
    milliseconds = static_cast<uint64_t>(value.tv_sec) * 1000 + static_cast<uint64_t>(value.tv_nsec / 1000000);
#else
    milliseconds = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
    return true;
}

/** 夹逼实际公开取时及提交端取时，并核对连续调用非递减，不修改系统时间
@return 256组平台参考及两个公开入口全部一致返回0
*/
static int MonotonicClock()
{
    uint64_t previous = 0;
    for (uint32_t sample = 0; sample < 256; ++sample)
    {
        uint64_t before = 0, after = 0;
        CHECK_TEST(ReferenceMonotonicMilliseconds(before));
        const uint64_t current = NetInterface::monotonicMilliseconds();
        const uint64_t submission = NetSenderSubmission::now();
        CHECK_TEST(ReferenceMonotonicMilliseconds(after));
        CHECK_TEST(before <= current && current <= submission && submission <= after && previous <= current);
        previous = submission;
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
    if (argc == 3 && std::strcmp(argv[1], "--link-timers") == 0)
    {
        return LinkTimers(argv[2]);
    }
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
        {CaseCreditNegotiation, false, CreditNegotiation},
        {CaseHandlePools, false, HandlePools},
        {CaseSubmissionConfiguration, false, SubmissionConfiguration},
        {CaseOwnerPools, false, OwnerPools},
        {CaseCiphertextConfiguration, false, CiphertextConfiguration},
        {CaseServiceCiphertextConfiguration, false, ServiceCiphertextConfiguration},
        {CaseDecoderRetention, false, DecoderRetention},
        {CaseMessageLimits, false, MessageLimits},
        {CaseExecutionConfiguration, false, ExecutionConfiguration},
        {CaseRouterHandshake, false, RouterHandshake},
        {CaseRouterStorage, false, RouterStorage},
        {CaseOutboundCompression, false, OutboundCompression},
        {CaseSessionOptions, false, SessionOptions},
        {CaseRecoveryWait, false, RecoveryWait},
        {CaseTunnelBuffers, false, TunnelBuffers},
        {CaseTlsScheduling, false, TlsScheduling},
        {CaseOwnerBudgets, false, OwnerBudgets},
        {CaseRouterPollBudgets, true, RouterPollBudgets},
        {CaseEventEnvelope, false, EventEnvelope},
        {CaseEntropyOutput, false, EntropyOutput},
        {CasePasswordDerivation, false, PasswordDerivation},
        {CaseExecutionExceptions, false, ExecutionExceptions},
        {CaseFileProtocolFacade, false, FileProtocolFacade},
        {CaseTlsHandles, false, TlsHandles},
        {CaseDecoderHandleFields, false, DecoderHandleFields},
        {CaseDirectoryHandles, false, DirectoryHandles},
        {CaseRoutingBoundaries, false, RoutingBoundaries},
        {CaseSubmissionProducers, true, SubmissionProducers},
        {CaseObjectReferences, false, ObjectReferences},
        {CaseMonotonicClock, false, MonotonicClock}
    };
    if (argc == 1 || (argc == 2 && std::strcmp(argv[1], "--all") == 0))
    {
        char instant[] = "--instant";
        char stress[] = "--stress";
        char* selected[] = {argv[0], instant, nullptr};
        const int instantResult = LibraryCaseSelection::run(2, selected, cases);
        selected[1] = stress;
        const int stressResult = LibraryCaseSelection::run(2, selected, cases);
        return instantResult != 0 ? instantResult : stressResult;
    }
    return LibraryCaseSelection::run(argc, argv, cases);
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}