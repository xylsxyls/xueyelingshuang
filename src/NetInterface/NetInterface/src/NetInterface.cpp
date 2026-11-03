#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceCompression.h"
#include <algorithm>
#include <cstring>
#include <new>

NetInterface::NetInterface(uint32_t payloadLimit) :
m_payloadLimit(payloadLimit),
m_compressionAllowed(false),
m_compressed(false),
m_headerSize(0),
m_wireSize(0),
m_body(nullptr),
m_capacity(0),
m_bodySize(0),
m_frame(),
m_status(payloadLimit == 0 || payloadLimit > NET_INTERFACE_MAX_PAYLOAD_LIMIT ? NI_INVALID_ARGUMENT : NI_NEED_MORE)
{

}

NetInterface::~NetInterface()
{
    delete[] m_body;
}

uint64_t NetInterface::readNumber(const uint8_t* bytes, uint32_t width)
{
    uint64_t value = 0;
    for (uint32_t index = 0; index < width; ++index)
    {
        value = (value << 8) | bytes[index];
    }
    return value;
}

void NetInterface::writeNumber(uint8_t* bytes, uint64_t value, uint32_t width)
{
    for (uint32_t index = width; index != 0; --index)
    {
        bytes[index - 1] = static_cast<uint8_t>(value);
        value >>= 8;
    }
}

int32_t NetInterface::prepareHeader()
{
    const uint32_t flags = static_cast<uint32_t>(readNumber(m_header + 28, 4));
    if (std::memcmp(m_header, "NIF2", 4) != 0 || readNumber(m_header + 4, 2) != 2 ||
        flags > 1 || (flags == 1 && !m_compressionAllowed))
    {
        return NI_PROTOCOL_ERROR;
    }
    m_frame.m_kind = static_cast<uint16_t>(readNumber(m_header + 6, 2));
    m_compressed = flags == 1;
    if (m_frame.m_kind < NI_HELLO || m_frame.m_kind > NI_DATA_ACK ||
        (m_compressed && m_frame.m_kind != NI_REQUEST && m_frame.m_kind != NI_RESPONSE))
    {
        return NI_PROTOCOL_ERROR;
    }
    m_frame.m_productId = static_cast<uint32_t>(readNumber(m_header + 8, 4));
    m_frame.m_requestId = readNumber(m_header + 12, 8);
    m_wireSize = static_cast<uint32_t>(readNumber(m_header + 20, 4));
    m_frame.m_messageType = static_cast<uint32_t>(readNumber(m_header + 24, 4));
    if (m_wireSize > m_payloadLimit)
    {
        return NI_LIMIT_EXCEEDED;
    }
    if (m_compressed && m_wireSize == 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (m_wireSize > m_capacity)
    {
        // 常用小消息按几何容量复用；大消息只占本条所需空间，consume立即释放。
        uint32_t capacity = m_wireSize;
        if (capacity <= 4096)
        {
            capacity = 256;
            while (capacity < m_wireSize)
            {
                capacity *= 2;
            }
        }
        delete[] m_body;
        m_body = nullptr;
        m_capacity = 0;
        m_body = new (std::nothrow) uint8_t[capacity];
        if (m_body == nullptr)
        {
            return NI_NO_MEMORY;
        }
        m_capacity = capacity;
    }
    return NI_NEED_MORE;
}

int32_t NetInterface::feed(const uint8_t* data, uint32_t size, uint32_t& consumed)
{
    consumed = 0;
    if (size != 0 && (data == nullptr || reinterpret_cast<uintptr_t>(data) > UINTPTR_MAX - size))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_status != NI_NEED_MORE)
    {
        return m_status;
    }
    if (m_headerSize != NET_INTERFACE_HEADER_SIZE)
    {
        const uint32_t count = (std::min)(size, NET_INTERFACE_HEADER_SIZE - m_headerSize);
        if (count != 0)
        {
            std::memcpy(m_header + m_headerSize, data, count);
            m_headerSize += count;
            consumed = count;
        }
        if (m_headerSize != NET_INTERFACE_HEADER_SIZE)
        {
            return NI_NEED_MORE;
        }
        const int32_t checked = prepareHeader();
        if (checked < 0)
        {
            return fail(checked);
        }
    }
    const uint32_t count = (std::min)(size - consumed, m_wireSize - m_bodySize);
    if (count != 0)
    {
        std::memcpy(m_body + m_bodySize, data + consumed, count);
        m_bodySize += count;
        consumed += count;
    }
    if (m_bodySize != m_wireSize)
    {
        return NI_NEED_MORE;
    }
    m_frame.m_payloadSize = m_wireSize;
    if (m_compressed)
    {
        uint8_t* decoded = nullptr;
        uint32_t length = 0;
        const int32_t result = NetInterfaceCompression::decode(m_body, m_wireSize, m_payloadLimit, decoded, length);
        if (result != NI_OK)
        {
            return fail(result);
        }
        delete[] m_body;
        m_body = decoded;
        m_capacity = length;
        m_frame.m_payloadSize = length;
    }
    m_frame.m_payload = m_frame.m_payloadSize == 0 ? nullptr : m_body;
    m_status = NI_READY;
    return m_status;
}

int32_t NetInterface::peek(NetInterfaceFrame& frame) const
{
    frame = m_status == NI_READY ? m_frame : NetInterfaceFrame();
    return m_status;
}

void NetInterface::clear(bool release)
{
    if (release)
    {
        delete[] m_body;
        m_body = nullptr;
        m_capacity = 0;
    }
    m_headerSize = 0;
    m_wireSize = 0;
    m_bodySize = 0;
    m_compressed = false;
    m_frame = NetInterfaceFrame();
    m_status = m_payloadLimit == 0 || m_payloadLimit > NET_INTERFACE_MAX_PAYLOAD_LIMIT ? NI_INVALID_ARGUMENT : NI_NEED_MORE;
}

int32_t NetInterface::consume()
{
    if (m_status != NI_READY)
    {
        return m_status < 0 ? m_status : NI_INVALID_STATE;
    }
    clear(m_capacity > 4096);
    return NI_OK;
}

int32_t NetInterface::finish()
{
    if (m_status != NI_NEED_MORE)
    {
        return m_status;
    }
    return m_headerSize == 0 ? NI_OK : fail(NI_TRUNCATED);
}

void NetInterface::reset()
{
    clear(true);
}

int32_t NetInterface::status() const
{
    return m_status;
}

uint32_t NetInterface::payloadLimit() const
{
    return m_payloadLimit;
}

int32_t NetInterface::fail(int32_t error)
{
    clear(true);
    m_status = error;
    return error;
}

void NetInterface::allowCompression(bool allowed)
{
    m_compressionAllowed = allowed;
}

int32_t NetInterface::encode(const NetInterfaceFrame& frame, uint8_t* output, uint32_t capacity, uint32_t& required)
{
    // 先固定元数据，避免输出写入调用方frame存储时覆盖后续读取的字段。
    const NetInterfaceFrame input = frame;
    required = 0;
    if (input.m_kind < NI_HELLO || input.m_kind > NI_DATA_ACK ||
        (input.m_payload == nullptr && input.m_payloadSize != 0) || (output == nullptr && capacity != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (input.m_payloadSize > NET_INTERFACE_MAX_PAYLOAD_LIMIT)
    {
        return NI_LIMIT_EXCEEDED;
    }
    const uintptr_t source = reinterpret_cast<uintptr_t>(input.m_payload);
    if (input.m_payloadSize != 0 && source > UINTPTR_MAX - input.m_payloadSize)
    {
        return NI_INVALID_ARGUMENT;
    }
    required = NET_INTERFACE_HEADER_SIZE + input.m_payloadSize;
    if (capacity < required)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    const uintptr_t destination = reinterpret_cast<uintptr_t>(output);
    if (destination > UINTPTR_MAX - required || (input.m_payloadSize != 0 &&
        source < destination + required && destination < source + input.m_payloadSize))
    {
        required = 0;
        return NI_INVALID_ARGUMENT;
    }
    std::memcpy(output, "NIF2", 4);
    writeNumber(output + 4, 2, 2);
    writeNumber(output + 6, input.m_kind, 2);
    writeNumber(output + 8, input.m_productId, 4);
    writeNumber(output + 12, input.m_requestId, 8);
    writeNumber(output + 20, input.m_payloadSize, 4);
    writeNumber(output + 24, input.m_messageType, 4);
    writeNumber(output + 28, 0, 4);
    if (input.m_payloadSize != 0)
    {
        std::memcpy(output + NET_INTERFACE_HEADER_SIZE, input.m_payload, input.m_payloadSize);
    }
    return NI_OK;
}

// NET_INTERFACE_UNIT_TEST_BEGIN
//#include "NetInterface.h"
//#include "NetInterfaceFileFlow.h"
//#include "NetInterface.h"
//#include "NetInterfaceDirectory.h"
//#include <vector>
//#include <cstdio>
//#include <chrono>
//
///** 测试时放开本段逐行注释，测试后注释回去；普通库不包含main
//@param [in] argc 参数个数
//@param [in] argv 可选--benchmark只运行完整正文校验的有界性能批次
//@return 0通过，失败输出源码行号并返回1
//*/
//int main(int argc, const char* argv[])
//{
//#define NET_INTERFACE_CHECK(value) do { if (!(value)) { std::fprintf(stderr, "NetInterface self-test failed at %d\n", __LINE__); return 1; } } while (0)
//    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
//    {
//        uint8_t payload[1024];
//        for (uint32_t index = 0; index < sizeof(payload); ++index)
//        {
//            payload[index] = static_cast<uint8_t>(index * 37U);
//        }
//        NetInterfaceFrame source = {};
//        source.m_kind = NI_REQUEST;
//        source.m_productId = 4;
//        source.m_requestId = UINT64_C(0x8123456789abcdef);
//        source.m_messageType = 23;
//        source.m_payload = payload;
//        source.m_payloadSize = sizeof(payload);
//        uint8_t wire[1056];
//        uint32_t required = 0;
//        NET_INTERFACE_CHECK(NetInterface::encode(source, wire, sizeof(wire), required) == NI_OK && required == sizeof(wire));
//        NetInterface decoder;
//        const auto begin = std::chrono::steady_clock::now();
//        for (uint32_t index = 0; index < 500000; ++index)
//        {
//            uint32_t used = 0;
//            NET_INTERFACE_CHECK(decoder.feed(wire, sizeof(wire), used) == NI_READY && used == sizeof(wire));
//            NetInterfaceFrame frame = {};
//            NET_INTERFACE_CHECK(decoder.peek(frame) == NI_READY && frame.m_payloadSize == sizeof(payload) &&
//                frame.m_requestId == source.m_requestId && std::memcmp(frame.m_payload, payload, sizeof(payload)) == 0);
//            NET_INTERFACE_CHECK(decoder.consume() == NI_OK);
//        }
//        const uint64_t microseconds = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
//            std::chrono::steady_clock::now() - begin).count());
//        decoder.reset();
//        std::printf("NIF2_BENCH frames=500000 body_bytes=512000000 microseconds=%llu PASS\n", static_cast<unsigned long long>(microseconds));
//        return 0;
//    }
//
//    const uint8_t golden[] = {'N', 'I', 'F', '2', 0, 2, 0, 3, 1, 2, 3, 4,
//        1, 2, 3, 4, 5, 6, 7, 8, 0, 0, 0, 3, 0x11, 0x22, 0x33, 0x44, 0, 0, 0, 0, 'A', 0, 'Z'};
//    uint8_t target[80];
//    NetInterfaceFrame fixed = {};
//    fixed.m_kind = NI_REQUEST;
//    fixed.m_productId = 0x01020304U;
//    fixed.m_requestId = UINT64_C(0x0102030405060708);
//    fixed.m_messageType = 0x11223344U;
//    fixed.m_payload = golden + 32;
//    fixed.m_payloadSize = 3;
//    uint32_t wanted = 0;
//    NET_INTERFACE_CHECK(NetInterface::encode(fixed, target, sizeof(target), wanted) == NI_OK && wanted == sizeof(golden));
//    NET_INTERFACE_CHECK(std::memcmp(target, golden, wanted) == 0);
//    for (uint32_t split = 0; split <= sizeof(golden); ++split)
//    {
//        NetInterface decoder;
//        uint32_t used = 99;
//        NET_INTERFACE_CHECK(decoder.feed(golden, split, used) == (split == sizeof(golden) ? NI_READY : NI_NEED_MORE) && used == split);
//        NET_INTERFACE_CHECK(decoder.feed(golden + split, static_cast<uint32_t>(sizeof(golden)) - split, used) == NI_READY);
//        NetInterfaceFrame decoded = {};
//        NET_INTERFACE_CHECK(decoder.peek(decoded) == NI_READY && decoded.m_productId == fixed.m_productId &&
//            decoded.m_kind == fixed.m_kind && decoded.m_requestId == fixed.m_requestId &&
//            decoded.m_messageType == fixed.m_messageType && decoded.m_payloadSize == 3 &&
//            std::memcmp(decoded.m_payload, golden + 32, 3) == 0);
//        NET_INTERFACE_CHECK(decoder.consume() == NI_OK && decoder.finish() == NI_OK);
//    }
//    NetInterface reused;
//    for (uint32_t round = 0; round < 10000; ++round)
//    {
//        uint32_t used = 0;
//        NET_INTERFACE_CHECK(reused.feed(golden, sizeof(golden), used) == NI_READY && used == sizeof(golden));
//        NetInterfaceFrame decoded = {};
//        NET_INTERFACE_CHECK(reused.peek(decoded) == NI_READY && decoded.m_payloadSize == 3 && std::memcmp(decoded.m_payload, golden + 32, 3) == 0);
//        NET_INTERFACE_CHECK(reused.consume() == NI_OK);
//    }
//    std::memset(target, 0x5a, sizeof(target));
//    fixed.m_payload = target + 4;
//    NET_INTERFACE_CHECK(NetInterface::encode(fixed, target, sizeof(target), wanted) == NI_INVALID_ARGUMENT && wanted == 0);
//    for (uint32_t index = 0; index < sizeof(target); ++index)
//    {
//        NET_INTERFACE_CHECK(target[index] == 0x5a);
//    }
//    fixed.m_payload = golden + 32;
//    std::vector<NetInterfaceFrame> alias(3);
//    alias[0] = fixed;
//    NET_INTERFACE_CHECK(NetInterface::encode(alias[0], reinterpret_cast<uint8_t*>(alias.data()),
//        static_cast<uint32_t>(alias.size() * sizeof(NetInterfaceFrame)), wanted) == NI_OK);
//    NET_INTERFACE_CHECK(std::memcmp(alias.data(), golden, sizeof(golden)) == 0);
//    uint32_t usedAddress = 99;
//    NET_INTERFACE_CHECK(reused.feed(reinterpret_cast<const uint8_t*>(UINTPTR_MAX - 1), 4, usedAddress) == NI_INVALID_ARGUMENT && usedAddress == 0);
//    NET_INTERFACE_CHECK(reused.finish() == NI_OK);
//
//    NetInterfaceDirectoryPoint point;
//    point.m_name = "node.0123456789abcdef0123456789abcdef";
//    point.m_address = "127.0.0.1";
//    point.m_port = 5203;
//    point.m_capacity = 32;
//    point.m_boot = UINT64_C(0x8123456789abcdef);
//    const uint64_t now = UINT64_C(1792000000000);
//    std::vector<NetInterfaceDirectoryPoint> points(1, point);
//    std::string wire;
//    NET_INTERFACE_CHECK(NetInterfaceDirectory::encode(points, wire) == NI_OK);
//    NetInterfaceDirectoryHandle handle = nullptr;
//    NET_INTERFACE_CHECK(NetInterface::directoryCreate(reinterpret_cast<const uint8_t*>(wire.data()), static_cast<uint32_t>(wire.size()), &handle) == NI_OK);
//    uint32_t count = 0;
//    NET_INTERFACE_CHECK(NetInterface::directoryCount(handle, &count) == NI_OK && count == 1);
//    char name[38] = {};
//    char address[40] = {};
//    uint32_t fields[4] = {};
//    NET_INTERFACE_CHECK(NetInterface::directoryNode(handle, 0, name, sizeof(name), address, sizeof(address), fields) == NI_OK &&
//        name == point.m_name && address == point.m_address && fields[0] == 5203 && fields[1] == 32 &&
//        fields[2] == UINT32_C(0x89abcdef) && fields[3] == UINT32_C(0x81234567));
//    name[0] = 'X';
//    NET_INTERFACE_CHECK(NetInterface::directoryNode(handle, 0, name, 1, address, sizeof(address), fields) == NI_BUFFER_TOO_SMALL &&
//        name[0] == 'X' && fields[0] == 0 && fields[3] == 0);
//    uint32_t required = 0;
//    NET_INTERFACE_CHECK(NetInterface::directoryCache(handle, static_cast<uint32_t>(now), static_cast<uint32_t>(now >> 32), nullptr, 0, &required) == NI_BUFFER_TOO_SMALL &&
//        required == wire.size() + 48);
//    std::vector<uint8_t> cache(required);
//    NET_INTERFACE_CHECK(NetInterface::directoryCache(handle, static_cast<uint32_t>(now), static_cast<uint32_t>(now >> 32), cache.data(), required, &required) == NI_OK);
//    NetInterface::directoryDestroy(handle);
//    handle = nullptr;
//    NET_INTERFACE_CHECK(NetInterface::directoryCreateCache(cache.data(), required, static_cast<uint32_t>(now + 1000),
//        static_cast<uint32_t>((now + 1000) >> 32), &handle) == NI_OK);
//    NetInterface::directoryDestroy(handle);
//    handle = nullptr;
//    std::vector<NetInterfaceDirectoryPoint> decoded;
//    NET_INTERFACE_CHECK(NetInterfaceDirectory::parseCache(cache.data(), required, now + UINT64_C(2592000001), decoded) == NI_TIMEOUT && decoded.empty());
//    NET_INTERFACE_CHECK(NetInterfaceDirectory::parseCache(cache.data(), required, now - 300001, decoded) == NI_TIMEOUT && decoded.empty());
//    NET_INTERFACE_CHECK(NetInterfaceDirectory::parseCache(cache.data(), required - 1, now, decoded) == NI_PROTOCOL_ERROR && decoded.empty());
//    cache[17] ^= 1;
//    NET_INTERFACE_CHECK(NetInterfaceDirectory::parseCache(cache.data(), required, now, decoded) == NI_PROTOCOL_ERROR && decoded.empty());
//    NET_INTERFACE_CHECK(NetInterface::directoryCreateCache(cache.data(), required, static_cast<uint32_t>(now), static_cast<uint32_t>(now >> 32), &handle) == NI_PROTOCOL_ERROR && !handle);
//    points.push_back(point);
//    NET_INTERFACE_CHECK(NetInterfaceDirectory::encode(points, wire) == NI_INVALID_ARGUMENT);
//    NET_INTERFACE_CHECK(NetInterfaceDirectory::address("::1") && NetInterfaceDirectory::address("2001:db8::1234") &&
//        !NetInterfaceDirectory::address("0.0.0.0") && !NetInterfaceDirectory::address("127.00.0.1") &&
//        !NetInterfaceDirectory::address("::") && !NetInterfaceDirectory::address("2001:::1") && !NetInterfaceDirectory::address("example.com"));
//    std::vector<uint8_t> plain(65537, 0x41);
//    NetInterfaceFrame outgoing = {};
//    outgoing.m_kind = NI_REQUEST; outgoing.m_productId = 1; outgoing.m_requestId = 1; outgoing.m_messageType = 123;
//    outgoing.m_payload = plain.data(); outgoing.m_payloadSize = static_cast<uint32_t>(plain.size());
//    std::vector<uint8_t> packed(NET_INTERFACE_HEADER_SIZE + plain.size());
//    uint32_t packedSize = 0;
//    NET_INTERFACE_CHECK(NetInterfaceCompression::encode(outgoing, packed.data(), static_cast<uint32_t>(packed.size()), packedSize) == NI_OK);
//    NET_INTERFACE_CHECK(packedSize < 2048 && packed[31] == 1);
//    NetInterface denied;
//    uint32_t used = 0;
//    NET_INTERFACE_CHECK(denied.feed(packed.data(), packedSize, used) == NI_PROTOCOL_ERROR);
//    NetInterface accepted;
//    accepted.allowCompression(true);
//    for (uint32_t offset = 0; offset < packedSize; ++offset)
//    {
//        NET_INTERFACE_CHECK(accepted.feed(packed.data() + offset, 1, used) >= 0 && used == 1);
//    }
//    NetInterfaceFrame incoming = {};
//    NET_INTERFACE_CHECK(accepted.peek(incoming) == NI_READY && incoming.m_payloadSize == plain.size() &&
//        std::memcmp(incoming.m_payload, plain.data(), plain.size()) == 0);
//    NetInterface bounded(65536);
//    bounded.allowCompression(true);
//    NET_INTERFACE_CHECK(bounded.feed(packed.data(), packedSize, used) == NI_LIMIT_EXCEEDED);
//    packed[packedSize - 1] ^= 1;
//    NetInterface broken;
//    broken.allowCompression(true);
//    NET_INTERFACE_CHECK(broken.feed(packed.data(), packedSize, used) == NI_PROTOCOL_ERROR);
//    uint32_t noise = 0x3e251dc7;
//    for (size_t index = 0; index < plain.size(); ++index)
//    {
//        noise ^= noise << 13; noise ^= noise >> 17; noise ^= noise << 5;
//        plain[index] = static_cast<uint8_t>(noise);
//    }
//    NET_INTERFACE_CHECK(NetInterfaceCompression::encode(outgoing, packed.data(), static_cast<uint32_t>(packed.size()), packedSize) == NI_OK &&
//        packedSize == packed.size() && packed[31] == 0);
//    NetInterfaceFileInfo info = {};
//    info.direction = 2; info.state = NI_FILE_TRANSFERRING; info.chunk = 1024; info.window = 4;
//    info.total = 4233; info.stream = 123; info.expiresUtc = 1792000060;
//    info.transfer[0] = 1; info.token[0] = 2;
//    NET_INTERFACE_CHECK(NetInterface::sha256(plain.data(), static_cast<uint32_t>(info.total), info.hash, 32) == NI_OK);
//    NetInterfaceFileFlow receiver(info, 100, false, now), legacy(info, 100, false, now), corrupt(info, 100, false, now);
//    NET_INTERFACE_CHECK(receiver.setWriteBatchChunks(0) == NI_INVALID_ARGUMENT && receiver.setWriteBatchChunks(5) == NI_INVALID_ARGUMENT);
//    NET_INTERFACE_CHECK(receiver.setWriteBatchChunks(4) == NI_OK && corrupt.setWriteBatchChunks(4) == NI_OK);
//    NetInterfaceFrame dataFrame = {};
//    dataFrame.m_kind = NI_DATA; dataFrame.m_productId = 100; dataFrame.m_requestId = info.stream;
//    dataFrame.m_messageType = NI_FILE_DATA_TYPE;
//    std::vector<uint8_t> dataWire(1200);
//    for (uint32_t block = 0; block < 5; ++block)
//    {
//        NetInterfaceFileData data = {};
//        data.offset = block * 1024; data.size = block == 4 ? 137 : 1024; data.bytes = plain.data() + data.offset;
//        std::memcpy(data.token, info.token, 32);
//        NET_INTERFACE_CHECK(NetInterface::sha256(data.bytes, data.size, data.hash, 32) == NI_OK);
//        NET_INTERFACE_CHECK(NetInterfaceFile::encodeData(data, dataWire.data(), static_cast<uint32_t>(dataWire.size()), dataFrame.m_payloadSize) == NI_OK);
//        dataFrame.m_payload = dataWire.data();
//        NET_INTERFACE_CHECK(receiver.accept(dataFrame) == NI_OK);
//        NET_INTERFACE_CHECK(receiver.setWriteBatchChunks(1) == NI_INVALID_STATE);
//        if (block == 0) { NET_INTERFACE_CHECK(legacy.accept(dataFrame) == NI_OK); }
//        if (block == 1) { NET_INTERFACE_CHECK(legacy.accept(dataFrame) == NI_LIMIT_EXCEEDED); }
//        if (block == 0)
//        {
//            dataWire[dataFrame.m_payloadSize - 1] ^= 1;
//            NET_INTERFACE_CHECK(corrupt.accept(dataFrame) == NI_PROTOCOL_ERROR);
//            dataWire[dataFrame.m_payloadSize - 1] ^= 1;
//        }
//        NetInterfaceFrame acknowledgement = {};
//        NET_INTERFACE_CHECK(receiver.peekOutput(acknowledgement) == NI_NEED_MORE);
//        if (block == 3 || block == 4)
//        {
//            uint64_t offset = 0; uint32_t size = 0; const uint8_t* bytes = nullptr;
//            NET_INTERFACE_CHECK(receiver.writeRequest(offset, bytes, size) == NI_READY &&
//                offset == (block == 3 ? 0 : 4096) && size == (block == 3 ? 4096 : 137));
//            NET_INTERFACE_CHECK(std::memcmp(bytes, plain.data() + offset, size) == 0);
//            // 相同重复块只确认内容一致，不能改变借出的写盘缓冲区，也不能提前ACK。
//            NET_INTERFACE_CHECK(receiver.accept(dataFrame) == NI_OK && std::memcmp(bytes, plain.data() + offset, size) == 0);
//            NET_INTERFACE_CHECK(receiver.peekOutput(acknowledgement) == NI_NEED_MORE);
//            NetInterfaceFileData pending = {};
//            pending.offset = block == 3 ? 4096 : info.total;
//            pending.size = block == 3 ? 137 : 0;
//            pending.flags = block == 3 ? 0 : 1;
//            pending.bytes = block == 3 ? plain.data() + 4096 : nullptr;
//            std::memcpy(pending.token, info.token, 32);
//            if (block == 3)
//            {
//                NET_INTERFACE_CHECK(NetInterface::sha256(pending.bytes, pending.size, pending.hash, 32) == NI_OK);
//            }
//            else
//            {
//                std::memcpy(pending.hash, info.hash, 32);
//            }
//            std::vector<uint8_t> pendingWire(1200);
//            NetInterfaceFrame pendingFrame = dataFrame;
//            pendingFrame.m_payload = pendingWire.data();
//            NET_INTERFACE_CHECK(NetInterfaceFile::encodeData(pending, pendingWire.data(),
//                static_cast<uint32_t>(pendingWire.size()), pendingFrame.m_payloadSize) == NI_OK);
//            // 新数据和END必须等写盘确认；背压期间原缓冲区的地址与内容保持有效。
//            NET_INTERFACE_CHECK(receiver.accept(pendingFrame) == NI_LIMIT_EXCEEDED &&
//                std::memcmp(bytes, plain.data() + offset, size) == 0);
//            NET_INTERFACE_CHECK(receiver.written(offset + 1, size, NI_OK) == NI_INVALID_STATE);
//            NET_INTERFACE_CHECK(receiver.written(offset, size, NI_OK) == NI_OK);
//            NET_INTERFACE_CHECK(receiver.peekOutput(acknowledgement) == NI_READY);
//            NetInterfaceFileAck ack = {};
//            NET_INTERFACE_CHECK(NetInterfaceFile::parseAck(acknowledgement, ack) == NI_OK && ack.offset == offset + size && ack.flags == 0);
//            NET_INTERFACE_CHECK(receiver.consumeOutput() == NI_OK);
//        }
//    }
//    NetInterfaceFileData end = {};
//    end.offset = info.total; end.flags = 1;
//    std::memcpy(end.token, info.token, 32); std::memcpy(end.hash, info.hash, 32);
//    NET_INTERFACE_CHECK(NetInterfaceFile::encodeData(end, dataWire.data(), static_cast<uint32_t>(dataWire.size()), dataFrame.m_payloadSize) == NI_OK);
//    NET_INTERFACE_CHECK(receiver.accept(dataFrame) == NI_OK && receiver.takeCommit() == NI_READY);
//    NetInterfaceFrame finalAck = {};
//    NET_INTERFACE_CHECK(receiver.peekOutput(finalAck) == NI_NEED_MORE);
//    NET_INTERFACE_CHECK(receiver.committed(NI_OK) == NI_READY && receiver.peekOutput(finalAck) == NI_READY);
//    NetInterfaceFileAck committedAck = {};
//    NET_INTERFACE_CHECK(NetInterfaceFile::parseAck(finalAck, committedAck) == NI_OK && committedAck.flags == 1 && committedAck.offset == info.total);
//    std::puts("NetInterface directory/cache, compression and bounded durable file batching self-test PASS");
//#undef NET_INTERFACE_CHECK
//    return 0;
//}
// NET_INTERFACE_UNIT_TEST_END