#include "NetWorkTestCases.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <vector>

/** 检查独立预期，生成带源码行号的失败
@param [in] pass 判定是否满足
@param [in] expression 断言原文
@param [in] line 测试源码行号
*/
static void netTestCheck(bool pass, const char* expression, int line)
{
    netTestCheckpoint();
    if (!pass)
    {
        std::ostringstream detail;
        detail << "line " << line << ": " << expression;
        throw std::runtime_error(detail.str());
    }
}

#define NET_CHECK(expression) netTestCheck((expression), #expression, __LINE__)

// 固定预期直接来自协议文档，不能用产品encode生成
static const uint8_t g_wire[] = {
    'N', 'I', 'F', '2', 0, 2, 0, 3, 1, 2, 3, 4,
    1, 2, 3, 4, 5, 6, 7, 8, 0, 0, 0, 3,
    0x11, 0x22, 0x33, 0x44, 0, 0, 0, 0, 'A', 0, 'Z'
};

/** 核对所有固定字段及嵌入零字节，不能只看READY
@param [in] decoder 已收完固定向量的真实实例
*/
static void checkWireFrame(NetInterface& decoder)
{
    NetInterfaceFrame frame;
    NET_CHECK(decoder.peek(frame) == NI_READY);
    NET_CHECK(frame.m_kind == NI_REQUEST);
    NET_CHECK(frame.m_productId == 0x01020304U);
    NET_CHECK(frame.m_requestId == UINT64_C(0x0102030405060708));
    NET_CHECK(frame.m_messageType == 0x11223344U);
    NET_CHECK(frame.m_payloadSize == 3);
    NET_CHECK(frame.m_payload != nullptr);
    NET_CHECK(std::memcmp(frame.m_payload, "A\0Z", 3) == 0);
}

void NetWorkTestCases::wireVector()
{
    NetInterfaceFrame source = {};
    source.m_kind = NI_REQUEST;
    source.m_productId = 0x01020304U;
    source.m_requestId = UINT64_C(0x0102030405060708);
    source.m_messageType = 0x11223344U;
    source.m_payloadSize = 3;
    source.m_payload = reinterpret_cast<const uint8_t*>("A\0Z");
    uint8_t output[sizeof(g_wire)] = {};
    uint32_t required = 0;
    NET_CHECK(NetInterface::encode(&source, output, sizeof(output), &required) == NI_OK);
    NET_CHECK(required == sizeof(g_wire));
    NET_CHECK(std::memcmp(output, g_wire, sizeof(g_wire)) == 0);
    NetInterface decoder;
    uint32_t used = 0;
    NET_CHECK(decoder.feed(g_wire, sizeof(g_wire), used) == NI_READY);
    NET_CHECK(used == sizeof(g_wire));
    checkWireFrame(decoder);
}

void NetWorkTestCases::fragmentation()
{
    for (uint32_t split = 0; split <= sizeof(g_wire); ++split)
    {
        NetInterface decoder;
        uint32_t used = 999;
        NET_CHECK(decoder.feed(g_wire, split, used) ==
            (split == sizeof(g_wire) ? NI_READY : NI_NEED_MORE));
        NET_CHECK(used == split);
        NET_CHECK(decoder.feed(g_wire + split, sizeof(g_wire) - split, used) == NI_READY);
        NET_CHECK(used == sizeof(g_wire) - split);
        checkWireFrame(decoder);
    }
    NetInterface decoder;
    for (uint32_t index = 0; index < sizeof(g_wire); ++index)
    {
        uint32_t used = 0;
        NET_CHECK(decoder.feed(g_wire + index, 1, used) ==
            (index + 1 == sizeof(g_wire) ? NI_READY : NI_NEED_MORE));
        NET_CHECK(used == 1);
    }
    checkWireFrame(decoder);
}

void NetWorkTestCases::backpressure()
{
    std::vector<uint8_t> wire(g_wire, g_wire + sizeof(g_wire));
    wire.insert(wire.end(), g_wire, g_wire + sizeof(g_wire));
    wire[sizeof(g_wire) + 19] = 9;
    NetInterface decoder;
    uint32_t used = 0;
    NET_CHECK(decoder.feed(&wire[0], static_cast<uint32_t>(wire.size()), used) == NI_READY);
    NET_CHECK(used == sizeof(g_wire));
    NetInterfaceFrame first;
    NET_CHECK(decoder.peek(first) == NI_READY);
    NET_CHECK(decoder.feed(&wire[sizeof(g_wire)], sizeof(g_wire), used) == NI_READY);
    NET_CHECK(used == 0);
    checkWireFrame(decoder);
    NET_CHECK(std::memcmp(first.m_payload, "A\0Z", 3) == 0);
    NET_CHECK(decoder.consume() == NI_OK);
    NET_CHECK(decoder.feed(&wire[sizeof(g_wire)], sizeof(g_wire), used) == NI_READY);
    NetInterfaceFrame second;
    NET_CHECK(decoder.peek(second) == NI_READY);
    NET_CHECK(second.m_requestId == UINT64_C(0x0102030405060709));
    NET_CHECK(used == sizeof(g_wire));
}

void NetWorkTestCases::invalidHeaders()
{
    const uint32_t positions[] = {0, 4, 5, 6, 7, 28, 31};
    for (uint32_t index = 0; index < sizeof(positions) / sizeof(positions[0]); ++index)
    {
        std::vector<uint8_t> wire(g_wire, g_wire + sizeof(g_wire));
        wire[positions[index]] = 0xff;
        NetInterface decoder;
        uint32_t used = 0;
        NET_CHECK(decoder.feed(&wire[0], 32, used) == NI_PROTOCOL_ERROR);
        NET_CHECK(used == 32);
        NET_CHECK(decoder.feed(g_wire, sizeof(g_wire), used) == NI_PROTOCOL_ERROR);
        NET_CHECK(used == 0);
        NetInterfaceFrame frame;
        std::memset(&frame, 0xff, sizeof(frame));
        NET_CHECK(decoder.peek(frame) == NI_PROTOCOL_ERROR);
        NET_CHECK(frame.m_payload == nullptr && frame.m_payloadSize == 0);
        decoder.reset();
        NET_CHECK(decoder.feed(g_wire, sizeof(g_wire), used) == NI_READY);
        checkWireFrame(decoder);
    }
    uint8_t wire[32];
    std::memcpy(wire, g_wire, 32);
    wire[7] = 0;
    NetInterface decoder;
    uint32_t used = 0;
    NET_CHECK(decoder.feed(wire, 32, used) == NI_PROTOCOL_ERROR);
}

void NetWorkTestCases::sizeLimits()
{
    NetInterface decoder(8);
    uint8_t wire[40] = {};
    std::memcpy(wire, g_wire, 32);
    wire[23] = 8;
    uint32_t used = 0;
    NET_CHECK(decoder.feed(wire, 40, used) == NI_READY);
    NET_CHECK(used == 40);
    NET_CHECK(decoder.consume() == NI_OK);
    wire[23] = 9;
    NET_CHECK(decoder.feed(wire, 32, used) == NI_LIMIT_EXCEEDED);
    decoder.reset();
    std::memset(wire + 20, 0xff, 4);
    NET_CHECK(decoder.feed(wire, 32, used) == NI_LIMIT_EXCEEDED);
    NET_CHECK(decoder.consume() == NI_LIMIT_EXCEEDED);
    NET_CHECK(decoder.finish() == NI_LIMIT_EXCEEDED);
    NetInterfaceHandle handle = nullptr;
    NET_CHECK(NetInterface::create(0, &handle) == NI_INVALID_ARGUMENT && handle == nullptr);
    NET_CHECK(NetInterface::create(NET_INTERFACE_MAX_PAYLOAD_LIMIT + 1, &handle) ==
        NI_INVALID_ARGUMENT && handle == nullptr);
    NetInterface invalid(0);
    NET_CHECK(invalid.status() == NI_INVALID_ARGUMENT);
}

void NetWorkTestCases::endOfInput()
{
    NetInterface decoder;
    uint32_t used = 99;
    NET_CHECK(decoder.feed(nullptr, 0, used) == NI_NEED_MORE && used == 0);
    NET_CHECK(decoder.finish() == NI_OK);
    for (uint32_t length = 1; length < sizeof(g_wire); ++length)
    {
        decoder.reset();
        NET_CHECK(decoder.feed(g_wire, length, used) == NI_NEED_MORE);
        NET_CHECK(decoder.consume() == NI_INVALID_STATE);
        NET_CHECK(decoder.finish() == NI_TRUNCATED);
        NET_CHECK(decoder.feed(g_wire, sizeof(g_wire), used) == NI_TRUNCATED);
    }
    decoder.reset();
    uint8_t empty[32];
    std::memcpy(empty, g_wire, 32);
    empty[23] = 0;
    NET_CHECK(decoder.feed(empty, 32, used) == NI_READY);
    NET_CHECK(decoder.finish() == NI_READY);
    NetInterfaceFrame frame;
    NET_CHECK(decoder.peek(frame) == NI_READY && frame.m_payload == nullptr);
    NET_CHECK(frame.m_payloadSize == 0);
    NET_CHECK(decoder.consume() == NI_OK && decoder.finish() == NI_OK);
}

void NetWorkTestCases::arguments()
{
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_PING;
    uint8_t output[32];
    std::memset(output, 0xab, sizeof(output));
    uint32_t required = 999;
    NET_CHECK(NetInterface::encode(&frame, output, 31, &required) == NI_BUFFER_TOO_SMALL);
    NET_CHECK(required == 32);
    for (uint32_t index = 0; index < sizeof(output); ++index)
    {
        NET_CHECK(output[index] == 0xab);
    }
    NET_CHECK(NetInterface::encode(&frame, nullptr, 0, &required) == NI_BUFFER_TOO_SMALL);
    NET_CHECK(required == 32);
    NET_CHECK(NetInterface::encode(&frame, nullptr, 32, &required) == NI_INVALID_ARGUMENT);
    NET_CHECK(NetInterface::encode(nullptr, output, 32, &required) == NI_INVALID_ARGUMENT);
    NET_CHECK(required == 0);
    NET_CHECK(NetInterface::encode(&frame, output, 32, nullptr) == NI_INVALID_ARGUMENT);
    frame.m_payloadSize = 1;
    NET_CHECK(NetInterface::encode(&frame, output, 32, &required) == NI_INVALID_ARGUMENT);
    frame.m_payloadSize = 0;
    frame.m_kind = 0;
    NET_CHECK(NetInterface::encode(&frame, output, 32, &required) == NI_INVALID_ARGUMENT);
    uint32_t used = 99;
    NET_CHECK(NetInterface::feed(nullptr, g_wire, sizeof(g_wire), &used) == NI_INVALID_ARGUMENT);
    NET_CHECK(used == 0);
    NET_CHECK(NetInterface::feed(nullptr, nullptr, 0, nullptr) == NI_INVALID_ARGUMENT);
    NET_CHECK(NetInterface::peek(nullptr, &frame) == NI_INVALID_ARGUMENT && frame.m_payload == nullptr);
    NET_CHECK(NetInterface::peek(nullptr, nullptr) == NI_INVALID_ARGUMENT);
    NET_CHECK(NetInterface::consume(nullptr) == NI_INVALID_ARGUMENT);
    NET_CHECK(NetInterface::finish(nullptr) == NI_INVALID_ARGUMENT);
    NET_CHECK(NetInterface::reset(nullptr) == NI_INVALID_ARGUMENT);
    NET_CHECK(NetInterface::create(8, nullptr) == NI_INVALID_ARGUMENT);
    NetInterface::destroy(nullptr);
    NetInterface decoder;
    NET_CHECK(decoder.feed(nullptr, 1, used) == NI_INVALID_ARGUMENT && used == 0);
    NET_CHECK(decoder.feed(g_wire, sizeof(g_wire), used) == NI_READY);
}

/** 运行确定性内容序列并核对每个字段
@param [in] count 有界消息数，循环中观察取消与期限
*/
static void checkSeries(uint32_t count)
{
    NetInterface decoder(512);
    uint32_t random = 0x348a92U;
    for (uint32_t sequence = 0; sequence < count; ++sequence)
    {
        netTestCheckpoint();
        uint32_t bodySize = sequence % 257U;
        std::vector<uint8_t> body(bodySize);
        for (uint32_t index = 0; index < bodySize; ++index)
        {
            body[index] = static_cast<uint8_t>((sequence + index * 17U) & 255U);
        }
        NetInterfaceFrame source = {};
        source.m_kind = NI_DATA;
        source.m_productId = 11;
        source.m_requestId = UINT64_C(0xffffffff00000000) + sequence;
        source.m_messageType = 77;
        source.m_payloadSize = bodySize;
        source.m_payload = body.empty() ? nullptr : &body[0];
        std::vector<uint8_t> wire(32 + bodySize);
        uint32_t required = 0;
        NET_CHECK(NetInterface::encode(&source, &wire[0], static_cast<uint32_t>(wire.size()), &required) == NI_OK);
        uint32_t offset = 0;
        while (offset < wire.size())
        {
            random = random * 1664525U + 1013904223U;
            uint32_t countBytes = (std::min)(static_cast<uint32_t>(wire.size()) - offset, 1U + random % 61U);
            uint32_t used = 0;
            int32_t state = decoder.feed(&wire[offset], countBytes, used);
            NET_CHECK(used == countBytes);
            offset += used;
            NET_CHECK(state == (offset == wire.size() ? NI_READY : NI_NEED_MORE));
        }
        NetInterfaceFrame result;
        NET_CHECK(decoder.peek(result) == NI_READY);
        NET_CHECK(result.m_requestId == source.m_requestId && result.m_productId == 11);
        NET_CHECK(result.m_kind == NI_DATA && result.m_messageType == 77);
        NET_CHECK(result.m_payloadSize == bodySize);
        NET_CHECK(bodySize == 0 || std::memcmp(result.m_payload, &body[0], bodySize) == 0);
        NET_CHECK(decoder.consume() == NI_OK);
    }
    NET_CHECK(decoder.finish() == NI_OK);
}

void NetWorkTestCases::cLanguage()
{
    NET_CHECK(netInterfaceCppProbe() == 0);
    NET_CHECK(netSenderProviderCProbe() == 0);
}

void NetWorkTestCases::deterministic()
{
    checkSeries(513);
}

void NetWorkTestCases::stress()
{
    checkSeries(100000);
}

void NetWorkTestCases::streamingHash()
{
    const uint8_t expected[] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};
    NetInterfaceHashHandle hash = nullptr;
    NET_CHECK(NetInterface::hashCreate(&hash) == NI_OK && hash != nullptr);
    try
    {
        uint8_t output[32];
        std::memset(output, 0x5a, sizeof(output));
        NET_CHECK(NetInterface::hashUpdate(hash, reinterpret_cast<const uint8_t*>("a"), 1) == NI_OK);
        NET_CHECK(NetInterface::hashUpdate(hash, nullptr, 0) == NI_OK);
        NET_CHECK(NetInterface::hashFinish(hash, output, 31) == NI_BUFFER_TOO_SMALL && output[0] == 0x5a && output[31] == 0x5a);
        NET_CHECK(NetInterface::hashUpdate(hash, reinterpret_cast<const uint8_t*>("bc"), 2) == NI_OK);
        NET_CHECK(NetInterface::hashFinish(hash, output, sizeof(output)) == NI_OK && std::memcmp(output, expected, 32) == 0);
        NET_CHECK(NetInterface::hashFinish(hash, output, sizeof(output)) == NI_INVALID_STATE &&
            NetInterface::hashUpdate(hash, nullptr, 0) == NI_INVALID_STATE);
    }
    catch (...)
    {
        NetInterface::hashDestroy(hash);
        throw;
    }
    NetInterface::hashDestroy(hash);
    NetInterface::hashDestroy(nullptr);
}

/** 用公开帧字段组成协议适配数组输入，不复制协议编码逻辑
@param [in] frame 原帧
@param [out] fields 六项ABI字段
*/
static void FileFrameFields(const NetInterfaceFrame& frame, uint32_t fields[6])
{
    fields[0] = frame.m_kind;
    fields[1] = frame.m_productId;
    fields[2] = static_cast<uint32_t>(frame.m_requestId);
    fields[3] = static_cast<uint32_t>(frame.m_requestId >> 32);
    fields[4] = frame.m_messageType;
    fields[5] = frame.m_payloadSize;
}

/** 将公开C输出转为C++视图，保留完整64位编号
@param [in] fields 六项ABI字段
@param [in] payload 借用体
@return 仅测试使用的视图
*/
static NetInterfaceFrame FileFieldsFrame(const uint32_t fields[6], const uint8_t* payload)
{
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(fields[0]);
    frame.m_productId = fields[1];
    frame.m_requestId = (static_cast<uint64_t>(fields[3]) << 32) | fields[2];
    frame.m_messageType = fields[4];
    frame.m_payloadSize = fields[5];
    frame.m_payload = payload;
    return frame;
}

void NetWorkTestCases::fileFlowBasic()
{
    const uint8_t expected[] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};
    NetInterfaceFileInfo info = {};
    info.direction = 1;
    info.state = NI_FILE_TRANSFERRING;
    info.chunk = 1024;
    info.window = 2;
    info.total = 3;
    info.stream = UINT64_C(0x8877665544332211);
    info.expiresUtc = 1700001000;
    std::memset(info.transfer, 0x42, sizeof(info.transfer));
    std::memset(info.token, 0x13, sizeof(info.token));
    std::memcpy(info.hash, expected, sizeof(expected));
    NetInterfaceFileControl open = {};
    open.operation = NI_FILE_OPEN_UPLOAD;
    open.chunk = 1024;
    open.window = 2;
    open.total = 3;
    std::memcpy(open.transfer, info.transfer, 16);
    std::memcpy(open.hash, expected, 32);
    open.key = reinterpret_cast<const uint8_t*>("fixture");
    open.keySize = 7;
    uint8_t control[2680] = {};
    uint32_t requiredControl = 0;
    NET_CHECK(NetInterfaceFile::encodeControl(open, control, sizeof(control), requiredControl) == NI_OK && requiredControl == 127);
    NetInterfaceFileControl parsed = {};
    NET_CHECK(NetInterfaceFile::parseControl(NI_FILE_OPEN_UPLOAD, control, requiredControl, parsed) == NI_OK &&
        parsed.total == 3 && parsed.keySize == 7 && std::memcmp(parsed.key, "fixture", 7) == 0);
    uint8_t wire[NET_INTERFACE_FILE_INFO_SIZE] = {};
    NET_CHECK(NetInterfaceFile::encodeInfo(info, wire, sizeof(wire)) == NI_OK);
    uint32_t metadata[12] = {};
    uint8_t transfer[16] = {};
    uint8_t token[32] = {};
    uint8_t hash[32] = {};
    NET_CHECK(NetInterface::fileInfoFields(wire, sizeof(wire), metadata, transfer, token, hash) == NI_OK &&
        metadata[8] == 0x44332211 && metadata[9] == 0x88776655 && std::memcmp(hash, expected, sizeof(hash)) == 0);
    NetInterfaceFileFlow sender(info, 9, true, 1000);
    NetInterfaceFileFlowHandle receiver = nullptr;
    NET_CHECK(NetInterface::fileFlowCreateWire(wire, sizeof(wire), 9, 0, 1000, 0, 15000, &receiver) == NI_OK);
    try
    {
        uint64_t offset = 99;
        uint32_t size = 99;
        NET_CHECK(sender.readRequest(offset, size) == NI_READY && offset == 0 && size == 3);
        NET_CHECK(sender.produced(1, reinterpret_cast<const uint8_t*>("abc"), 3) == NI_INVALID_ARGUMENT);
        NET_CHECK(sender.produced(0, reinterpret_cast<const uint8_t*>("abc"), 3) == NI_OK);
        NetInterfaceFrame data = {};
        NET_CHECK(sender.peekOutput(data) == NI_READY && data.m_kind == NI_DATA);
        uint32_t fields[6] = {};
        FileFrameFields(data, fields);
        NET_CHECK(NetInterface::fileFlowAccept(receiver, fields, data.m_payload) == NI_OK);
        NET_CHECK(sender.consumeOutput() == NI_OK);
        const uint8_t* bytes = nullptr;
        uint32_t work[3] = {};
        NET_CHECK(NetInterface::fileFlowWrite(receiver, work, &bytes) == NI_READY && work[0] == 0 && work[1] == 0 &&
            work[2] == 3 && std::memcmp(bytes, "abc", 3) == 0);
        NET_CHECK(NetInterface::fileFlowTakeCommit(receiver) == NI_NEED_MORE);
        NET_CHECK(NetInterface::fileFlowWritten(receiver, 0, 0, 3, NI_OK) == NI_OK);
        NET_CHECK(NetInterface::fileFlowPeek(receiver, fields, &bytes) == NI_READY);
        NET_CHECK(sender.accept(FileFieldsFrame(fields, bytes)) == NI_OK && NetInterface::fileFlowConsume(receiver) == NI_OK);
        NET_CHECK(sender.peekOutput(data) == NI_READY);
        NetInterfaceFileData end = {};
        NET_CHECK(NetInterfaceFile::parseData(data, end) == NI_OK && end.flags == 1 && end.offset == 3 && end.size == 0);
        FileFrameFields(data, fields);
        NET_CHECK(NetInterface::fileFlowAccept(receiver, fields, data.m_payload) == NI_OK && sender.consumeOutput() == NI_OK);
        NET_CHECK(NetInterface::fileFlowPeek(receiver, fields, &bytes) == NI_NEED_MORE);
        NET_CHECK(NetInterface::fileFlowTakeCommit(receiver) == NI_READY && NetInterface::fileFlowTakeCommit(receiver) == NI_NEED_MORE);
        NET_CHECK(NetInterface::fileFlowProgress(receiver, 1001, 0) == NI_NEED_MORE);
        NET_CHECK(NetInterface::fileFlowCommitted(receiver, NI_OK) == NI_READY);
        NET_CHECK(NetInterface::fileFlowPeek(receiver, fields, &bytes) == NI_READY &&
            sender.accept(FileFieldsFrame(fields, bytes)) == NI_OK && NetInterface::fileFlowConsume(receiver) == NI_OK);
        int32_t result = 99;
        sender.snapshot(info, result);
        NET_CHECK(result == NI_READY && info.state == NI_FILE_COMPLETED && info.offset == 3 && info.stream == 0);
        NET_CHECK(NetInterface::fileFlowSnapshot(receiver, &info, &result) == NI_OK && result == NI_READY && info.state == NI_FILE_COMPLETED);
    }
    catch (...)
    {
        NetInterface::fileFlowDestroy(receiver);
        throw;
    }
    NetInterface::fileFlowDestroy(receiver);
}

void NetWorkTestCases::fileFlowSnapshot()
{
    std::vector<uint8_t> content(3072);
    for (uint32_t index = 0; index < content.size(); ++index) { content[index] = static_cast<uint8_t>(index * 17); }
    for (uint32_t resume = 0; resume <= 1024; resume += 1024)
    {
        NetInterfaceFileInfo info = {};
        info.direction = 1; info.state = NI_FILE_TRANSFERRING; info.chunk = 1024; info.window = 2;
        info.total = content.size(); info.offset = resume; info.stream = 913; info.expiresUtc = 1700001000;
        std::memset(info.transfer, 0x42, sizeof(info.transfer)); std::memset(info.token, 0x13, sizeof(info.token));
        NET_CHECK(NetInterface::sha256(content.data(), static_cast<uint32_t>(content.size()), info.hash, 32) == NI_OK);
        NetInterfaceFileFlow sender(info, 9, true, 1000);
        NetInterfaceFileFlow receiver(info, 9, false, 1000);
        NetInterfaceFileInfo snapshot = {}; int32_t result = 99;
        if (resume != 0)
        {
            sender.snapshot(snapshot, result);
            NET_CHECK(snapshot.state == NI_FILE_PREPARING && snapshot.stream == 0 && snapshot.offset == resume);
            NET_CHECK(sender.prime(0, content.data(), resume) == NI_OK && receiver.prime(0, content.data(), resume) == NI_OK);
        }
        sender.snapshot(snapshot, result);
        NET_CHECK(snapshot.state == NI_FILE_TRANSFERRING && snapshot.stream == 913 && snapshot.offset == resume);
        NET_CHECK(sender.produced(resume, content.data() + resume, 1024) == NI_OK);
        NetInterfaceFrame frame = {};
        NET_CHECK(sender.peekOutput(frame) == NI_READY && receiver.accept(frame) == NI_OK && sender.consumeOutput() == NI_OK);
        uint64_t offset = 0; uint32_t size = 0; const uint8_t* bytes = nullptr;
        NET_CHECK(receiver.writeRequest(offset, bytes, size) == NI_READY && offset == resume && size == 1024);
        NET_CHECK(receiver.written(offset, size, NI_OK) == NI_OK);
        receiver.snapshot(snapshot, result);
        NET_CHECK(snapshot.state == NI_FILE_TRANSFERRING && snapshot.stream == 913 && snapshot.offset == resume + 1024);
        NET_CHECK(receiver.peekOutput(frame) == NI_READY && sender.accept(frame) == NI_OK && receiver.consumeOutput() == NI_OK);
        sender.snapshot(snapshot, result);
        NET_CHECK(snapshot.state == NI_FILE_TRANSFERRING && snapshot.stream == 913 && snapshot.offset == resume + 1024);
        sender.cancel(NI_CANCELLED); sender.snapshot(snapshot, result);
        NET_CHECK(snapshot.state == NI_FILE_CANCELLED && snapshot.stream == 0 && result == NI_CANCELLED);
    }
}

void NetWorkTestCases::encodingAliases()
{
    uint8_t output[96];
    std::memset(output, 0xa7, sizeof(output));
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_REQUEST;
    frame.m_productId = 0x01020304U;
    frame.m_requestId = UINT64_C(0x0102030405060708);
    frame.m_messageType = 0x11223344U;
    frame.m_payload = output + 4;
    frame.m_payloadSize = 3;
    uint32_t required = 99;
    NET_CHECK(NetInterface::encode(&frame, output, sizeof(output), &required) == NI_INVALID_ARGUMENT && required == 0);
    for (uint32_t index = 0; index < sizeof(output); ++index)
    {
        NET_CHECK(output[index] == 0xa7);
    }
    // capacity包括后方源数据，但实际35字节输出未覆盖它，不能误判为重叠。
    std::memcpy(output + sizeof(g_wire), g_wire + 32, 3);
    frame.m_payload = output + sizeof(g_wire);
    NET_CHECK(NetInterface::encode(&frame, output, sizeof(output), &required) == NI_OK && required == sizeof(g_wire));
    NET_CHECK(std::memcmp(output, g_wire, sizeof(g_wire)) == 0);
    NET_CHECK(std::memcmp(output + sizeof(g_wire), g_wire + 32, 3) == 0);
    frame.m_payload = g_wire + 32;
    std::vector<NetInterfaceFrame> aliased(3);
    aliased[0] = frame;
    NET_CHECK(NetInterface::encode(&aliased[0], reinterpret_cast<uint8_t*>(aliased.data()),
        static_cast<uint32_t>(aliased.size() * sizeof(NetInterfaceFrame)), &required) == NI_OK);
    NET_CHECK(required == sizeof(g_wire) && std::memcmp(aliased.data(), g_wire, sizeof(g_wire)) == 0);
}