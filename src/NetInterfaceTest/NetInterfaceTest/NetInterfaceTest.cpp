#include "NetInterfaceTest.h"
#include "NetInterface/NetGatewayClientTiming.h"
#include "PlatformTest/PlatformTestAPI.h"
#include "Compress/CompressAPI.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
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
#include <stdexcept>
#include <vector>
#include <memory>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif
#include "../../NetGatewayTest/NetGatewayTest/NetTestTlsCertificateData.h"

#define CHECK_TEST(expr) do { if (!(expr)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; } } while (false)

// 手工构造的 NIF2 网络字节序样本，正文含零字节。
static const uint8_t kFrame[] = {'N','I','F','2',0,2,0,3,0,0,0,7,1,2,3,4,5,6,7,8,0,0,0,3,0,0,0,9,0,0,0,0,0,255,17};

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
    struct BorrowedView
    {
        const uint8_t* pointer;
        uint32_t padding[6];
    } shared = {bytes, {11,22,33,44,55,66}};
    uint32_t* overlapping = reinterpret_cast<uint32_t*>(&shared);
    CHECK_TEST(NetInterface::tunnelPeek(tunnel, overlapping, &shared.pointer) == NI_INVALID_ARGUMENT &&
        shared.pointer == bytes && shared.padding[0] == 11 && shared.padding[1] == 22);
    CHECK_TEST(NetInterface::connectionPeekOutput(nullptr, &shared.pointer, overlapping) == NI_INVALID_ARGUMENT &&
        shared.pointer == bytes);
    uint32_t identityFields[8] = {37,38,39,40,41,42,43,44};
    const uint8_t* certificate = bytes;
    CHECK_TEST(NetInterface::linkPeerInfo(nullptr, identityFields, &shared.pointer, &certificate, &shared.pointer) ==
        NI_INVALID_ARGUMENT && certificate == bytes && shared.pointer == bytes && identityFields[0] == 37);
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
    const NetInterfaceEntropyFunction failures[] = {partial, nullptr};
    for (size_t i = 0; i < 2; ++i)
    {
        uint8_t guarded[] = {1, 2, 3, 4, 5};
        const uint8_t expected[] = {1, 0, 0, 0, 5};
        CHECK_TEST(NetInterface::entropyBytes(failures[i], &calls, guarded + 1, 3) == NI_INVALID_STATE &&
            !std::memcmp(guarded, expected, sizeof(expected)) && calls == 2);
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
    for (uint32_t i = 0; i < 3; ++i)
    {
        if (i == 2) { client->configureCompact(false); }
        client->configureCompact(true);
        const uint32_t id = 40 + i;
        NetInterfaceFrame data = {7, id, type, NI_DATA, 1, &marker};
        const uint8_t full[] = {0xe3, NI_DATA, 7, 9, 1, 0, 0, 0, 0, 0, 0, 0, static_cast<uint8_t>(id), marker};
        const uint8_t delta[] = {0x81, marker};
        const uint32_t expected = i == 1 ? sizeof(delta) : sizeof(full);
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
        CHECK_TEST(total == expected && !std::memcmp(actual, i == 1 ? delta : full, expected));
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

/** 验证三个句柄工厂在交付所有权前实际采用指定池
@return 成功为0，任一配置被忽略时失败
*/
static int HandlePools()
{
    NetRequestTablePools tables;
    tables.m_endpoints.initialCapacity = 1;
    tables.m_endpoints.maximumCapacity = 1;
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
    CHECK_TEST(submission.begin(7, 8, 60000, pending) == NI_OK && pending &&
        submission.sendSynchronous(pending, &value, 1, ticket) == NI_OK && ticket);
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
    pools.m_queue.initialCapacity = pools.m_queue.maximumCapacity = 16;
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

/** 检查提交层的每个物理池和索引均能独立拒绝错误配置，并观察入口真实扩容、按序交付和归还
@return 成功为0
*/
static int SubmissionConfiguration()
{
    CHECK_TEST(SubmissionPrefetch(1, 1) == 0 && SubmissionPrefetch(2, 3) == 0 &&
        SubmissionPrefetch(2, 4) == 0 && SubmissionPrefetch(8, 32) == 0);
    CHECK_TEST(SubmissionOwnerLifetime() == 0);
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
    CHECK_TEST(submission.configurePools(pools, options) == NI_INVALID_STATE);
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
    const NetInterfaceFrame synchronous = {7, 33, 0, NI_DATA, 0, nullptr};
    uint64_t ticket = 0;
    CHECK_TEST(submission.dataSynchronous(synchronous, ticket) == NI_OK && ticket);
    struct Observer : NetSenderSubmissionSignal
    {
        unsigned count;
        Observer() : count(0) {}
        void notify() override { ++count; }
    };
    std::shared_ptr<Observer> observer(new Observer);
    CHECK_TEST(submission.observeSynchronous(ticket, std::shared_ptr<NetSenderSubmissionSignal>()) == NI_INVALID_ARGUMENT &&
        submission.observeSynchronous(ticket, observer) == NI_OK && submission.observeSynchronous(ticket, observer) == NI_INVALID_STATE);
    submission.advanceSynchronous(NI_READY, NetSenderSubmission::now() + 1);
    CHECK_TEST(observer->count == 1 && submission.synchronousResult(ticket, true) == NI_TIMEOUT &&
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

int32_t PoolTestSink::send(uint64_t, const NetInterfaceFrame&, const NetEndpointOrigin&)
{
    return NI_NOT_SUPPORTED;
}

void PoolTestSink::close(uint64_t)
{

}

/** 使路由端点池实际耗尽、回收并重用，验证两层会话转交各自的请求池设置
@return 成功为0
*/
static int OwnerPools()
{
    PoolTestSink sink;
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
    config.m_endpoints.initialCapacity = config.m_endpoints.maximumCapacity = 1;
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
    CHECK_TEST(link.peekOutput(record) == NI_READY && record.m_kind == NI_TUNNEL_HELLO && link.consumeOutput() == NI_OK);
    CHECK_TEST(link.progress(50) >= 0 && link.peekOutput(record) == NI_READY && record.m_kind == NI_TUNNEL_CONTROL &&
        record.m_size == 11 && record.m_data);
    CHECK_TEST(link.progress(10049) >= 0 && link.progress(10050) == NI_TIMEOUT);
    return 0;
}

CiphertextProbeTransport::CiphertextProbeTransport(CiphertextProbeHost& host) : m_host(host), m_listener(nullptr), m_ready(false)
{

}

int32_t CiphertextProbeTransport::connect(const char*, uint32_t, const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener,
    uint64_t& peer)
{
    peer = 0;
    if (m_host.m_refuseConnect) { return NI_PROTOCOL_ERROR; }
    if (!listener || options.m_tls.handshakeTimeoutMs != 10000)
    {
        return NI_INVALID_ARGUMENT;
    }
    m_listener = listener;
    peer = 1;
    return NI_OK;
}

bool CiphertextProbeTransport::poll()
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
    else if (!m_response.empty())
    {
        const NetInterfaceFrame response = {0, 1, NI_GATEWAY_OPEN, NI_RESPONSE,
            static_cast<uint32_t>(m_response.size()), reinterpret_cast<const uint8_t*>(m_response.data())};
        m_listener->onLinkControl(1, response);
        m_response.clear();
    }
    return true;
}

int32_t CiphertextProbeTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != 1 || frame.m_kind != NI_REQUEST || frame.m_messageType != NI_GATEWAY_OPEN || frame.m_requestId != 1)
    {
        return NI_PROTOCOL_ERROR;
    }
    ProtoMessage open;
    if (frame.m_productId != 0 || !frame.m_payload || !frame.m_payloadSize ||
        !open.from(frame.m_payload, frame.m_payloadSize)) { return NI_PROTOCOL_ERROR; }
    const Variant* target = open.findValue(NI_GATEWAY_TARGET_PRODUCT);
    const Variant* channel = open.findValue(NI_GATEWAY_CHANNEL);
    uint32_t window = 0;
    if (!target || target->type() != Variant::UINT64 || static_cast<uint64_t>(*target) != 7 ||
        !channel || channel->type() != Variant::UINT64 || static_cast<uint64_t>(*channel) != 1 ||
        !open.findValue(NI_GATEWAY_WINDOW) || NetInterfaceCredit::window(open, window) != NI_OK) { return NI_PROTOCOL_ERROR; }
    m_host.m_offered = window;
    ProtoMessage answer;
    if (m_host.m_reply)
    {
        answer.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(m_host.m_reply)));
    }
    answer.setKeyValue(NI_GATEWAY_CHANNEL, Variant(static_cast<uint64_t>(1)));
    return answer.toString(m_response) ? NI_OK : NI_NO_MEMORY;
}

int32_t CiphertextProbeTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (peer != 1 || record.m_kind != NI_TUNNEL_BYTES || record.m_product != 7 || record.m_channel != 1 ||
        !record.m_data || record.m_size == 0 || record.m_data[0] != 22) {
        return NI_PROTOCOL_ERROR;
    }
    m_host.m_first = record.m_size;
    return NI_LIMIT_EXCEEDED;
}

void CiphertextProbeTransport::shutdown()
{
    m_listener = nullptr;
}

CiphertextProbeHost::CiphertextProbeHost() : m_first(0), m_offered(0), m_reply(0), m_closed(NI_OK)
{

}

NetGatewayTransport* CiphertextProbeHost::create()
{
    return m_fault == 1 ? nullptr : new CiphertextProbeTransport(*this);
}

void CiphertextProbeHost::onSessionReady(uint64_t, uint32_t, uint32_t)
{

}

void CiphertextProbeHost::onSessionResult(uint64_t, uint64_t, uint32_t, uint32_t, uint32_t status, const uint8_t*, uint32_t)
{
    if (status == NI_RPC_TIMEOUT) { ++m_timeouts; }
}

bool CiphertextProbeHost::onSessionFrame(uint64_t, const NetInterfaceFrame& frame)
{
    if (frame.m_kind == NI_EVENT && frame.m_requestId == 99 && frame.m_payloadSize == 2 && frame.m_payload &&
        frame.m_payload[0] == 19 && frame.m_payload[1] == 31) { ++m_frames; }
    if (m_throwFrame) { throw std::runtime_error("frame"); }
    return false;
}

void CiphertextProbeHost::onSessionClosed(uint64_t, int32_t reason, int32_t)
{
    ++m_closes;
    m_closed = reason;
}

/** 单独创建两个客户端会话，在运输边界核对各自的 TLS 输出预算
@return 成功为0
*/
static int ServiceCiphertextConfiguration()
{
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
    const uint32_t windows[] = {1048576, 131072, 65536};
    const uint32_t replies[] = {0, 131072, 81920};
    for (size_t i = 0; i < 3; ++i)
    {
        CiphertextProbeHost host;
        host.m_reply = replies[i];
        NetClientSession session;
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
        CHECK_TEST(session.connectService(7, "127.0.0.1", 8000, tls, hello, &host, &host) == NI_OK);
        CHECK_TEST(session.configureServiceTlsQueueLimits(limits) == NI_INVALID_STATE);
        CHECK_TEST(session.configureServiceWindow(65536) == NI_INVALID_STATE &&
            session.configureServicePollBudgets(2, 3) == NI_INVALID_STATE);
        for (unsigned step = 0; step != 16 && !host.m_first && !host.m_closed; ++step) { session.poll(); }
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
    for (unsigned fault = 1; fault <= 3; ++fault)
    {
        CiphertextProbeHost host;
        host.m_fault = fault;
        NetClientSession session;
        CHECK_TEST(session.connectService(7, "127.0.0.1", 8000, tls, hello, &host, &host) == NI_OK);
        for (unsigned i = 0; i < 16 && !host.m_closes; ++i) { session.poll(); }
        CHECK_TEST(host.m_closes == 1 && host.m_closed == (fault == 1 ? NI_NO_MEMORY : fault == 2 ? NI_TRUNCATED : NI_PROTOCOL_ERROR));
        session.shutdown();
    }
    return 0;
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
        return 0;
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
    for (fault = 1; fault <= 7; ++fault)
    {
        destroys = binds = waits = 0;
        active = &external;
        {
            NetExecution execution;
            if (fault == 1)
            {
                void* previousHost = &external;
                NetExecutionScope rejected(execution);
                valid = valid && !NetExecution::exchange(&execution, previousHost) && !previousHost &&
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
            valid = valid && waits == (fault == 4 || fault >= 6 ? 1U : 0U);
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
m_client(client), m_listener(nullptr), m_events(0), m_result(NI_NEED_MORE)
{

}

int32_t RouterHandshakeTransport::connect(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener*, uint64_t& peer)
{
    peer = 0;
    return NI_NOT_SUPPORTED;
}

int32_t RouterHandshakeTransport::listen(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener* listener, uint32_t)
{
    m_listener = listener;
    return NI_OK;
}

bool RouterHandshakeTransport::poll()
{
    if (m_listener == nullptr) { return false; }
    if (m_endInput) { m_endInput = false; m_listener->onClosed(9, NI_CANCELLED, 0); return true; }
    if (m_events == 0)
    {
        ++m_events;
        m_listener->onReady(9);
    }
    else if (m_events == 1)
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
    if (peer != 9 || frame.m_kind != NI_HELLO_REPLY) { return NI_PROTOCOL_ERROR; }
    m_result = m_client.accept(frame, 1);
    return m_result == NI_READY ? NI_OK : NI_PROTOCOL_ERROR;
}

void RouterHandshakeTransport::close(uint64_t, int32_t reason)
{
    m_result = reason;
}

void RouterHandshakeTransport::shutdown()
{
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
    CHECK_TEST(router.bindTransport(&transport) == NI_OK);
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
    CHECK_TEST(router.listen("127.0.0.1", 8081, tls, 1) == NI_OK &&
        router.configureClientHandshake(1, 1, 1024, 100) == NI_INVALID_STATE);
    NetExecution owner;
    NetExecutionScope scope(owner);
    owner.begin();
    const auto start = std::chrono::steady_clock::now();
    CHECK_TEST(router.poll() && owner.wait(60000) == NI_OK);
    const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count());
    const uint32_t timeout = custom ? 24000 : 5000;
    CHECK_TEST(waited <= timeout && waited + elapsed + 2 >= timeout);
    CHECK_TEST(router.poll() && transport.m_result == NI_READY);
    owner.begin();
    CHECK_TEST(router.poll() && owner.wait(60000) == NI_OK);
    const uint64_t sinceReady = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count());
    const uint32_t ping = custom ? 12345 : 30000;
    CHECK_TEST(waited <= ping && waited + sinceReady + 2 >= ping);
    uint32_t fields[4] = {};
    uint8_t nonce[16] = {}, device[16] = {};
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

SessionProbeChannel::SessionProbeChannel(NetInterfaceHandshake& server) :
m_server(server), m_listener(nullptr), m_announced(false)
{

}

int32_t SessionProbeChannel::connect(const char*, uint32_t, const NetInterfaceTlsOptions&, NetTransportListener* listener, uint64_t& peer)
{
    peer = 7;
    m_listener = listener;
    return NI_OK;
}

bool SessionProbeChannel::poll()
{
    if (m_listener == nullptr) { return false; }
    if (!m_announced)
    {
        m_announced = true;
        m_listener->onReady(7);
    }
    NetInterfaceFrame frame = {};
    if (m_event && m_listener->onFrame(7, *m_event)) { m_event = nullptr; }
    if (m_server.peekOutput(frame) == NI_READY && m_listener->onFrame(7, frame))
    {
        m_server.consumeOutput();
    }
    return true;
}

int32_t SessionProbeChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != 7) { return NI_INVALID_ARGUMENT; }
    if (frame.m_kind == NI_HELLO) { return m_server.accept(frame, 1) < 0 ? NI_PROTOCOL_ERROR : NI_OK; }
    if (frame.m_kind == NI_PING) { ++m_pings; return NI_OK; }
    if (frame.m_kind == NI_DATA)
    {
        if (frame.m_requestId != m_dataIds.size() + 1 || frame.m_payloadSize != 2 ||
            !frame.m_payload || frame.m_payload[0] != 19 || frame.m_payload[1] != 31) { return NI_PROTOCOL_ERROR; }
        m_dataIds.push_back(frame.m_requestId);
        return NI_OK;
    }
    if (frame.m_kind == NI_CANCEL) { return NI_OK; }
    NetInterfaceRpcView rpc = {};
    if (frame.m_kind != NI_REQUEST || NetInterfaceRpc::parse(frame, rpc) != NI_OK) { return NI_PROTOCOL_ERROR; }
    if (rpc.m_size == 0)
    {
        m_body.clear();
    }
    else
    {
        m_body.assign(rpc.m_data, rpc.m_data + rpc.m_size);
    }
    return NI_OK;
}

int32_t SessionProbeChannel::outputState(uint64_t peer) const
{
    return peer == 7 ? NI_OK : NI_INVALID_ARGUMENT;
}

void SessionProbeChannel::close(uint64_t, int32_t)
{
    m_listener = nullptr;
}

void SessionProbeChannel::shutdown()
{
    m_listener = nullptr;
}

/** 给原生事件线程一次重新调度机会；单线程Wasm只在本用例的短时钟窗口内推进 */
static void YieldTimerProbe()
{
#if !defined(__EMSCRIPTEN__)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
}

/** 对真实业务请求检查会话缓存的保留和释放，同时核对实际传给宿主的等待上限
@param [in] retention 请求编码缓存预算，-1保留当前容量
@param [in] heartbeatMode 0检查空闲期限，1、2分别让总无输入或PONG期限先到
@param [in] waited 最近一次事件等待回调收到的预算
@return 正文、容量和期限均符合预期时为0
*/
static int CheckSessionOptions(int64_t retention, const uint32_t& waited, unsigned heartbeatMode = 0)
{
    NetInterfaceHandshakeOptions options = {};
    options.m_server = 1;
    options.m_capabilities = 1;
    options.m_maxRequests = 4;
    options.m_maxChunk = 1024;
    options.m_timeoutMs = 5000;
    options.m_nonce[0] = 19;
    NetInterfaceHandshake server(options, 0);
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
            heartbeat.m_pingIntervalMs = 1;
            heartbeat.m_receiveTimeoutMs = heartbeatMode == 1 ? 3200 : 7800;
            heartbeat.m_pongTimeoutMs = heartbeatMode == 1 ? 5700 : 2300;
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
    CHECK_TEST(client.configureExpiryBudget(0) == NI_INVALID_ARGUMENT &&
        client.bindChannel(&channel) == NI_OK && client.connect("127.0.0.1", 9001, tls, options, &events) == NI_OK);
    CHECK_TEST(client.configureExpiryBudget(2) == NI_INVALID_STATE);
    NetExecution owner;
    NetExecutionScope scope(owner);
    const auto start = std::chrono::steady_clock::now();
    CHECK_TEST(client.poll() && client.ready());
    owner.begin();
    CHECK_TEST(client.poll() && owner.wait(120000) == NI_OK);
    const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count());
    if (heartbeatMode != 0)
    {
        const auto watchdog = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!channel.m_pings && client.ready() && std::chrono::steady_clock::now() < watchdog)
        {
            YieldTimerProbe();
            client.poll();
        }
        CHECK_TEST(channel.m_pings == 1 && client.ready());
        owner.begin();
        CHECK_TEST(client.poll() && owner.wait(120000) == NI_OK);
        const uint64_t sinceReady = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count());
        const uint32_t next = heartbeatMode == 1 ? 3200 : 2300;
        CHECK_TEST(waited <= next && waited + sinceReady + 2 >= next && channel.m_pings == 1);
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
    for (unsigned i = 0; i < 3; ++i)
    {
        uint64_t id = 0;
        CHECK_TEST(client.beginRequest(7, 9, 25, id) == NI_OK && id != 0);
    }
    const auto expired = std::chrono::steady_clock::now() + std::chrono::milliseconds(27);
    while (std::chrono::steady_clock::now() < expired) { YieldTimerProbe(); }
    CHECK_TEST(client.poll() && events.m_timeouts == (custom ? 1U : 3U));
    if (custom) { CHECK_TEST(client.poll() && events.m_timeouts == 2 && client.poll() && events.m_timeouts == 3); }
    client.shutdown();
    CHECK_TEST(client.payloadCapacity() == 0);
    return 0;
}

/** 核对默认、丢弃及无限保留三种会话，并验证心跳在无待办时仍能唤醒宿主
@return 全部通过为0
*/
static int SessionOptions()
{
    return WithWaitProbe([](const uint32_t& waited) -> int {
        if (CheckSessionOptions(4096, waited) || CheckSessionOptions(0, waited)) { return 1; }
        if (CheckSessionOptions(-1, waited) || CheckSessionOptions(-1, waited, 1)) { return 1; }
        return CheckSessionOptions(-1, waited, 2);
    });
}

int32_t PendingDiscovery::start(const std::vector<NetGatewayAddress>&, const std::string&, const NetInterfaceTlsOptions&)
{
    m_running = true;
    return NI_OK;
}

bool PendingDiscovery::poll()
{
    return m_running;
}

void PendingDiscovery::shutdown()
{
    m_running = false;
}

bool PendingDiscovery::stopped() const
{
    return !m_running;
}

bool PendingDiscovery::choose(NetClientDiscoveryPoint&)
{
    ++m_attempts;
    return false;
}

void PendingDiscovery::failed(const NetClientDiscoveryPoint&)
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

void FrameDeliveryProbe::onClosed(uint64_t, int32_t, int32_t)
{

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
        const auto start = std::chrono::steady_clock::now();
        CHECK_TEST(timing.deliver(listener, frame, 0, deadline) == NI_LIMIT_EXCEEDED && deadline == expected &&
            owner.wait(60000) == NI_OK);
        const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count());
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
        const auto watchdog = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (host.m_closes < step && std::chrono::steady_clock::now() < watchdog)
        {
            owner.begin();
            const auto start = std::chrono::steady_clock::now();
            CHECK_TEST(session.poll());
            if (host.m_closes != step)
            {
                YieldTimerProbe();
                continue;
            }
            CHECK_TEST(host.m_closed == NI_PROTOCOL_ERROR && owner.wait(120000) == NI_OK);
            const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count());
            const uint32_t expected = step == 1 ? 16 : 20;
            CHECK_TEST(waited <= expected && waited + elapsed + 2 >= expected);
        }
        CHECK_TEST(host.m_closes == step);
    }
    session.shutdown();
    return 0;
}

/** 无候选时核对实际宿主等待预算，分别让重试和总期限先到
@return 全部通过为0
*/
static int RecoveryWait()
{
    return WithWaitProbe([](const uint32_t& waited) -> int {
        CHECK_TEST(FrameBackpressure(waited) == 0 && RecoveryBackoff(waited) == 0);
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
            uint32_t seed = 17;
            tls.entropy = HelloEntropy;
            tls.entropyContext = &seed;
            NetInterfaceHandshakeOptions options = {};
            options.m_platform = options.m_capabilities = options.m_maxRequests = 1;
            options.m_maxChunk = 1024;
            options.m_timeoutMs = 1000;
            options.m_device[0] = 1;
            CHECK_TEST(session.bindGateway(&host, &discovery) == NI_OK);
            NetExecution owner;
            NetExecutionScope scope(owner);
            owner.begin();
            const auto start = std::chrono::steady_clock::now();
            CHECK_TEST(session.connectService(7, "127.0.0.1", 8000, tls, options, &host) == NI_OK);
            CHECK_TEST(session.poll() && discovery.m_attempts == 1 && owner.wait(120000) == NI_OK);
            const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count());
            CHECK_TEST(waited <= expected[i] && elapsed + waited + 2 >= expected[i]);
            CHECK_TEST(session.configureRecovery(50, 1, 2) == NI_INVALID_STATE);
            session.shutdown();
            CHECK_TEST(discovery.stopped());
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
            const auto start = std::chrono::steady_clock::now();
            CHECK_TEST(session.connectService(7, "127.0.0.1", 8000, tls, hello, &host, &host) == NI_OK);
            owner.begin();
            CHECK_TEST(session.poll() && owner.wait(120000) == NI_OK && waited == 0);
            owner.begin();
            CHECK_TEST(session.poll() && host.m_offered != 0 && owner.wait(120000) == NI_OK);
            const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count());
            const uint32_t limit = i ? 2100 : 30000;
            CHECK_TEST(waited <= limit && waited + elapsed + 2 >= limit &&
                session.configureServiceTimeouts(100, 500) == NI_INVALID_STATE);
            session.shutdown();
        }
        return 0;
    });
}

/** 用真实路由器分别协商默认及自定义值，不等待墙钟流逝
@return 默认、独立配置和外部附加模式均符合契约时为0
*/
static int RouterHandshake()
{
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
m_request(0), m_stream(0)
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
    else if (frame.m_kind == NI_RESPONSE && frame.m_messageType == NI_FILE_OPEN_DOWNLOAD)
    {
        NetInterfaceRpcView rpc = {};
        NetInterfaceFileInfo info = {};
        if (NetInterfaceRpc::parse(frame, rpc) != NI_OK || rpc.m_value != NI_RPC_SUCCESS ||
            NetInterfaceFile::parseInfo(rpc.m_data, rpc.m_size, info) != NI_OK) { return NI_PROTOCOL_ERROR; }
        m_stream = info.stream;
    }
    else if (frame.m_kind == NI_REQUEST)
    {
        m_request = frame.m_requestId;
    }
    return NI_OK;
}

void RouterFileSink::close(uint64_t)
{

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
    if (expireTogether)
    {
        pools.m_expiredRequestBudget = 2;
        pools.m_closingEndpointBudget = 1;
        pools.m_expiredFileBudget = 1;
        pools.m_endpoints.initialCapacity = pools.m_endpoints.maximumCapacity = 2;
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
    const auto created = std::chrono::steady_clock::now();
    uint8_t payload[NET_INTERFACE_FILE_INFO_SIZE];
    uint64_t ids[3] = {};
    const uint32_t durations[] = {90, 30, 60};
    for (unsigned iteration = 0; iteration < 4; ++iteration)
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
            NetInterfaceRpc::encode(NI_REQUEST, 30000, 0, payload, count, wire.data(), static_cast<uint32_t>(wire.size()), length) == NI_OK);
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
        info.expiresUtc = static_cast<uint64_t>(std::time(nullptr)) + (expireTogether ? 2 : durations[i]);
        info.transfer[0] = open.transfer[0];
        info.hash[0] = info.token[0] = 1;
        CHECK_TEST(NetInterfaceFile::encodeInfo(info, payload, sizeof(payload)) == NI_OK &&
            NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, payload, sizeof(payload),
                wire.data(), static_cast<uint32_t>(wire.size()), length) == NI_OK);
        frame.m_requestId = service.m_request;
        frame.m_kind = NI_RESPONSE;
        frame.m_payloadSize = length;
        CHECK_TEST(router.receiveService(server, frame) == NI_OK && client.m_stream != 0);
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
        const auto due = std::chrono::steady_clock::now() + std::chrono::milliseconds(2050);
        while (std::chrono::steady_clock::now() < due) { YieldTimerProbe(); }
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
        uint64_t first = 0, second = 0;
        CHECK_TEST(router.poll() && router.addService(8, false, 1, first) == NI_OK && first != 0 &&
            router.addService(9, false, 1, second) == NI_LIMIT_EXCEEDED && !second);
        CHECK_TEST(router.poll() && router.addService(9, false, 1, second) == NI_OK && second != first);
        router.shutdown();
        return 0;
    }
    const unsigned order[] = {1, 0, 2};
    const uint32_t nextDeadline[] = {30000, 60000, 60000};
    NetExecution owner;
    NetExecutionScope scope(owner);
    for (unsigned i = 0; i < 3; ++i)
    {
        owner.begin();
        CHECK_TEST(router.poll() && owner.wait(120000) == NI_OK);
        const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - created).count());
        // 文件授权按整UTC秒编码，转成单调期限时可能跨过一秒边界。
        CHECK_TEST(waited <= nextDeadline[i] && waited + elapsed + 1002 >= nextDeadline[i]);
        const unsigned index = order[i];
        NetInterfaceFrame cancel = {7, ids[index], NI_FILE_DATA_TYPE, NI_CANCEL, 0, nullptr};
        CHECK_TEST(router.receiveClient(source, cancel) == NI_OK && router.receiveClient(source, cancel) == NI_OK &&
            service.m_notices.size() == i + 1 && service.m_notices[i] == 100 + index);
    }
    router.shutdown();
    CHECK_TEST(router.payloadCapacity() == 0);
    return 0;
}

/** 验证默认、零及无限暂存预算和文件期限堆摘除后的路由行为
@return 全部通过时为0
*/
static int RouterStorage()
{
    return WithWaitProbe([](const uint32_t& waited) -> int {
        const int defaults = CheckRouterStorage(4096, waited);
        if (defaults != 0) { return defaults; }
        const int discard = CheckRouterStorage(0, waited);
        if (discard != 0) { return discard; }
        return CheckRouterStorage(-1, waited);
    });
}

/** 使已接纳的三条文件流与三条请求同时到期，逐轮检查两种预算并回收端点
@return 真实回调、在册请求数及归还槽数符合各自预算时为0
*/
static int RouterPollBudgets()
{
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
    for (unsigned round = 0; round < 2; ++round)
    {
        CHECK_TEST(client.sendRecord(smallRecord) == NI_OK && client.sendRecord(smallRecord) == NI_LIMIT_EXCEEDED &&
            client.sendRecord(mediumRecord) == NI_OK && client.sendRecord(largeRecord) == NI_LIMIT_EXCEEDED);
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
    NetExecution owner;
    NetExecutionScope scope(owner);
    auto checkWait = [&](NetInterfaceLink& link, uint64_t now, uint32_t expected) -> bool {
        owner.begin();
        const auto start = std::chrono::steady_clock::now();
        if (link.progress(now) != NI_READY || owner.wait(60000) != NI_OK) { return false; }
        const uint64_t elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count());
        return waited <= expected && waited + elapsed + 2 >= expected;
    };
    const uint32_t idle = custom ? 9000 : 15000;
    const uint32_t pong = custom ? 7000 : 10000;
    const uint32_t drain = custom ? 3000 : 5000;
    const uint64_t pingAt = 1000 + idle;
    CHECK_TEST(checkWait(client, 1500, idle - 500));
    CHECK_TEST(client.progress(pingAt) == NI_READY && checkWait(client, pingAt + 500, pong - 500));
    const uint8_t body[] = {0, 17, 41};
    const NetInterfaceFrame event = {0, 17, 42, NI_EVENT, sizeof(body), body};
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
    CHECK_TEST(client.progress(pingAt + pong - 1) == NI_READY && client.progress(pingAt + pong) == NI_TIMEOUT);
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
    CHECK_TEST(NetInterface::linkCreate(&clientOptions, &client) >= 0 && client);
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
        !std::memcmp(name, nodeOptions.m_name, fields[1]) && fields[2] && certificate);
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
    enum { IdentityCapacity = 24577 };
    uint8_t bytes[IdentityCapacity] = {};
    const auto wipe = [](uint8_t* data) { NetInterface::wipe(data, IdentityCapacity); };
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
    char unterminated[97];
    std::memset(unterminated, 'n', sizeof(unterminated));
    NetInterfaceLinkOptions invalidName = options;
    invalidName.m_name = unterminated;
    NetInterfaceLink invalidLink(invalidName);
    CHECK_TEST(invalidLink.status() == NI_INVALID_ARGUMENT);
    CHECK_TEST(AuthenticatedLinkHandles(clientOptions, options) == 0);
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
    const auto stop = [](NetSenderDispatch* active) { active->shutdown(); };
    std::unique_ptr<NetSenderDispatch, decltype(stop)> lifetime(&dispatch, stop);
    CHECK_TEST(dispatch.bindSubmission(nullptr) == NI_INVALID_ARGUMENT && dispatch.bindSubmission(&submission) == NI_OK &&
        &dispatch.submission() == &submission);
    CHECK_TEST(dispatch.configurePollBudgets(commands, bytes) == NI_OK &&
        dispatch.configurePollBudgets(0, 1) == NI_INVALID_ARGUMENT &&
        dispatch.configurePollBudgets(1, 0) == NI_INVALID_ARGUMENT && dispatch.prepare(&events) == NI_OK &&
        dispatch.configurePollBudgets(1, 1) == NI_INVALID_STATE);
    hello.m_server = 0;
    hello.m_platform = 2;
    hello.m_device[0] = 2;
    uint32_t entropy = 37;
    NetInterfaceTlsOptions tls = {};
    tls.entropy = HelloEntropy;
    tls.entropyContext = &entropy;
    CHECK_TEST(client.bindChannel(&channel) == NI_OK &&
        client.connect("127.0.0.1", 9001, tls, hello, dispatch.listener()) == NI_OK &&
        dispatch.started(client.generation(), 7) == NI_OK && dispatch.poll() && client.ready());
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

/** 对比命令预算与字节预算，两者先耗尽的决定交付边界
@return 各个独立实例通过时为0
*/
static int OwnerBudgets()
{
    return WithWaitProbe([](const uint32_t& waited) -> int {
        return CheckDispatchBudget(1, 100, 1, waited) || CheckDispatchBudget(10, 3, 2, waited) ||
            CheckDispatchBudget(10, 1, 1, waited);
    });
}

/** 文件协议门面核对线格式、恢复前缀、宿主读长度和取消终态
@return 全部符合预期时返回0
*/
static int FileProtocolFacade()
{
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
    CHECK_TEST(protocol.fileFlowPrime(handle, 0, 0, payload, 1024) == NI_OK && protocol.fileFlowProgress(handle, 2, 0) == NI_OK);
    CHECK_TEST(protocol.fileFlowRead(handle, fields) == NI_READY && fields[0] == 1024 && fields[1] == 0 && fields[2] == 2);
    CHECK_TEST(protocol.fileFlowProduced(handle, 1024, 0, payload + 1024, 1) == NI_INVALID_ARGUMENT);
    CHECK_TEST(protocol.fileFlowProduced(handle, 1024, 0, payload + 1024, 2) == NI_OK);
    CHECK_TEST(protocol.fileFlowCancel(handle, NI_CANCELLED) == NI_OK);
    int32_t status = 0;
    CHECK_TEST(protocol.fileFlowSnapshotWire(handle, wire, sizeof(wire), &status) == NI_OK && status == NI_CANCELLED);
    CHECK_TEST(protocol.fileParseInfo(wire, sizeof(wire), &parsed) == NI_OK && parsed.state == NI_FILE_CANCELLED && parsed.stream == 0);
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
    CHECK_TEST(NetInterface::tlsClose(tls) >= 0 && transfer(tls, listener) && NetInterface::tlsFinishInput(listener) >= 0);
    return 0;
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
        spare.adopt(pool, storage);
        root.reset();
        CHECK_TEST(spare.get() == child && destroyed == 1);
        spare.reset();
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
    std::string privateText("old"), requestText("old");
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
    CHECK_TEST(NetInterface::certificateCreateRequest(validName, sizeof(validName), nullptr, &seed, privateKey, sizeof(privateKey),
        &keySize, request, sizeof(request), &requestSize) == NI_INVALID_ARGUMENT && !keySize && !requestSize);
    const int32_t result = NetInterface::certificateCreateRequest(validName, sizeof(validName), HelloEntropy, &seed,
        privateKey, sizeof(privateKey), &keySize, request, sizeof(request), &requestSize);
    const bool pem = result == NI_OK && keySize >= 11 && requestSize >= 11 && !std::memcmp(privateKey, "-----BEGIN ", 11) &&
        !std::memcmp(request, "-----BEGIN ", 11);
    NetInterface::wipe(privateKey, sizeof(privateKey));
    CHECK_TEST(pem);
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
    struct AttachedSession : NetClientSession
    {
        using NetClientSession::hasBoundChannel;
        using NetClientSession::boundGatewayFactory;
        ~AttachedSession() { detachBindingsForDestruction(); }
    } session;
    CHECK_TEST(!session.hasBoundChannel() && !session.boundGatewayFactory() &&
        session.bindGateway(&host, nullptr) == NI_OK && session.boundGatewayFactory() == &host &&
        session.bindChannel(&gateway) == NI_OK && session.hasBoundChannel());
    CHECK_TEST(!session.serviceMode() && !session.generation() && session.beginRequestAt(0, 7, 3, 0, 1) == NI_INVALID_ARGUMENT &&
        session.beginRequestAt(1, 7, 3, 0, 1) == NI_INVALID_STATE);
    return 0;
}

/** 核对多生产者同时预留的请求号、唯一性和关闭后的输出
@return 全部符合预期时返回0
*/
static int SubmissionProducers()
{
    enum { Workers = 4, Requests = 64 };
    NetSenderSubmission submission;
    CHECK_TEST(submission.start(7, 11) == NI_OK && submission.ready(7, Workers * Requests, 1024) == NI_OK);
    uint64_t ids[Workers * Requests] = {};
    bool ok[Workers] = {};
    const auto run = [&](unsigned worker) {
        try
        {
            for (unsigned i = 0; i < Requests; ++i)
            {
                if (submission.begin(11, worker + 1, 60000, ids[worker * Requests + i]) != NI_OK) { return; }
            }
            ok[worker] = true;
        }
        catch (...) { ok[worker] = false; }
    };
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    for (unsigned i = 0; i < Workers; ++i) { run(i); }
#else
    std::thread threads[Workers];
    std::atomic<bool> start(false);
    try
    {
        for (unsigned i = 0; i < Workers; ++i)
        {
            threads[i] = std::thread([&, i]() {
                while (!start.load(std::memory_order_acquire)) { std::this_thread::yield(); }
                run(i);
            });
        }
    }
    catch (...) { /* 已经启动的生产者仍须放行并回收。 */ }
    start.store(true, std::memory_order_release);
    for (unsigned i = 0; i < Workers; ++i) { if (threads[i].joinable()) { threads[i].join(); } }
#endif
    for (unsigned i = 0; i < Workers; ++i) { CHECK_TEST(ok[i]); }
    std::sort(ids, ids + Workers * Requests);
    for (unsigned i = 0; i < Workers * Requests; ++i) { CHECK_TEST(ids[i] == i + 1); }
    submission.close(NI_CANCELLED);
    uint64_t denied = 99;
    CHECK_TEST(submission.begin(11, 1, 1, denied) == NI_INVALID_STATE && !denied);
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
        {CaseObjectReferences, false, ObjectReferences}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
}