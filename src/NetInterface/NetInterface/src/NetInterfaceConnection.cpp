#include "NetInterfaceMacro.h"
#include "NetInterfaceConnection.h"
#include "NetInterfaceCompression.h"
#include <algorithm>
#include <cstring>
#include <new>

NetInterfaceConnection::NetInterfaceConnection(const NetInterfaceTlsOptions& options) :
NetInterfaceConnection(options, NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT)
{

}

NetInterfaceConnection::NetInterfaceConnection(const NetInterfaceTlsOptions& options, uint32_t payloadLimit) :
m_tls(options),
m_decoder(payloadLimit),
m_error(payloadLimit == 0 || payloadLimit > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT + 32 ? NI_INVALID_ARGUMENT : 0),
m_closed(false),
m_plainSize(0),
m_plainOffset(0),
m_active(kWriteSlots),
m_activeOffset(0),
m_free(0),
m_spare(nullptr),
m_spareCapacity(0),
m_compressionCount(0)
{
    for (uint32_t queue = 0; queue < 2; ++queue)
    {
        m_heads[queue] = m_tails[queue] = kWriteSlots;
        m_queueBytes[queue] = m_queueCounts[queue] = 0;
    }
    for (uint32_t index = 0; index < kWriteSlots; ++index)
    {
        NetInterfaceConnectionWrite& write = m_writes[index];
        write.m_data = nullptr;
        write.m_size = write.m_capacity = write.m_queue = 0;
        write.m_next = index + 1;
    }
}

NetInterfaceConnection::~NetInterfaceConnection()
{
    clearWrites();
    wipe(m_plain, sizeof(m_plain));
}

void NetInterfaceConnection::wipe(uint8_t* bytes, uint32_t size)
{
    volatile uint8_t* destination = bytes;
    for (uint32_t index = 0; index < size; ++index)
    {
        destination[index] = 0;
    }
}

void NetInterfaceConnection::recycle(NetInterfaceConnectionWrite& write)
{
    wipe(write.m_data, write.m_size);
    if (write.m_capacity <= 4096 && write.m_capacity > m_spareCapacity)
    {
        delete[] m_spare;
        m_spare = write.m_data;
        m_spareCapacity = write.m_capacity;
    }
    else
    {
        delete[] write.m_data;
    }
    write.m_data = nullptr;
    write.m_size = write.m_capacity = 0;
}

void NetInterfaceConnection::completeActive()
{
    NetInterfaceConnectionWrite& write = m_writes[m_active];
    m_queueBytes[write.m_queue] -= write.m_size;
    --m_queueCounts[write.m_queue];
    recycle(write);
    write.m_next = m_free;
    m_free = m_active;
    m_active = kWriteSlots;
    m_activeOffset = 0;
}

void NetInterfaceConnection::clearWrites()
{
    for (uint32_t index = 0; index < kWriteSlots; ++index)
    {
        NetInterfaceConnectionWrite& write = m_writes[index];
        wipe(write.m_data, write.m_size);
        delete[] write.m_data;
        write.m_data = nullptr;
        write.m_size = write.m_capacity = write.m_queue = 0;
        write.m_next = index + 1;
    }
    delete[] m_spare;
    m_spare = nullptr;
    m_spareCapacity = 0;
    m_free = 0;
    m_active = kWriteSlots;
    m_activeOffset = 0;
    for (uint32_t queue = 0; queue < 2; ++queue)
    {
        m_heads[queue] = m_tails[queue] = kWriteSlots;
        m_queueBytes[queue] = m_queueCounts[queue] = 0;
    }
}

int32_t NetInterfaceConnection::feed(const uint8_t* data, uint32_t size, uint32_t& consumed)
{
    consumed = 0;
    return m_error != 0 ? m_error : m_tls.feed(data, size, consumed);
}

uint32_t NetInterfaceConnection::decodePending()
{
    if (m_decoder.status() == NI_READY || m_plainOffset == m_plainSize)
    {
        return 0;
    }
    uint32_t consumed = 0;
    const int32_t result = m_decoder.feed(m_plain + m_plainOffset, m_plainSize - m_plainOffset, consumed);
    wipe(m_plain + m_plainOffset, consumed);
    m_plainOffset += consumed;
    if (m_plainOffset == m_plainSize)
    {
        m_plainOffset = m_plainSize = 0;
    }
    if (result < 0)
    {
        fail(result);
    }
    return consumed;
}

int32_t NetInterfaceConnection::progress(uint64_t monotonicMs)
{
    if (m_error != 0 || m_closed)
    {
        return status();
    }
    for (uint32_t step = 0; step < 64; ++step)
    {
        bool moved = false;
        const int32_t tlsStatus = m_tls.progress(monotonicMs);
        if (tlsStatus < 0)
        {
            return fail(tlsStatus);
        }
        if (tlsStatus != NET_INTERFACE_TLS_READY)
        {
            return tlsStatus;
        }
        if (m_active == kWriteSlots)
        {
            const uint32_t queue = m_heads[1] != kWriteSlots ? 1 : 0;
            m_active = m_heads[queue];
            if (m_active != kWriteSlots)
            {
                m_heads[queue] = m_writes[m_active].m_next;
                if (m_heads[queue] == kWriteSlots)
                {
                    m_tails[queue] = kWriteSlots;
                }
                m_writes[m_active].m_next = kWriteSlots;
            }
        }
        if (m_active != kWriteSlots)
        {
            NetInterfaceConnectionWrite& write = m_writes[m_active];
            uint32_t accepted = 0;
            const int32_t result = m_tls.write(write.m_data + m_activeOffset, write.m_size - m_activeOffset, accepted);
            if (result < 0)
            {
                return fail(result);
            }
            m_activeOffset += accepted;
            moved = accepted != 0;
            if (m_activeOffset == write.m_size)
            {
                completeActive();
            }
        }
        moved = decodePending() != 0 || moved;
        if (m_error != 0)
        {
            return m_error;
        }
        if (m_decoder.status() != NI_READY && m_plainSize == 0)
        {
            uint32_t received = 0;
            const int32_t result = m_tls.read(m_plain, sizeof(m_plain), received);
            if (result < 0)
            {
                return fail(result);
            }
            if (result == NET_INTERFACE_TLS_CLOSED)
            {
                const int32_t frameEnd = m_decoder.finish();
                if (frameEnd < 0)
                {
                    return fail(frameEnd);
                }
                m_closed = true;
                clearWrites();
                return NET_INTERFACE_TLS_CLOSED;
            }
            m_plainSize = received;
            moved = received != 0 || moved;
            moved = decodePending() != 0 || moved;
        }
        if (m_error != 0 || !moved)
        {
            break;
        }
    }
    return status();
}

int32_t NetInterfaceConnection::send(const NetInterfaceFrame& frame)
{
    const int32_t current = status();
    if (current != NET_INTERFACE_TLS_READY)
    {
        return current < 0 ? current : NI_INVALID_STATE;
    }
    if (frame.m_payloadSize > m_decoder.payloadLimit())
    {
        return NI_LIMIT_EXCEEDED;
    }
    uint32_t required = 0;
    const int32_t validation = NetInterface::encode(frame, nullptr, 0, required);
    if (validation != NI_BUFFER_TOO_SMALL)
    {
        return validation;
    }
    const bool control = frame.m_payloadSize <= 4096 &&
        (frame.m_kind == NI_HELLO || frame.m_kind == NI_HELLO_REPLY || frame.m_kind == NI_EVENT ||
        frame.m_kind == NI_CANCEL || frame.m_kind == NI_PING || frame.m_kind == NI_PONG || frame.m_kind == NI_DATA_ACK);
    const uint32_t queue = control ? 1 : 0;
    if (m_queueCounts[queue] >= (control ? 16U : 64U) ||
        required > (control ? 65536U : 2097152U) - m_queueBytes[queue] || m_free == kWriteSlots)
    {
        return NI_LIMIT_EXCEEDED;
    }
    // 先在空闲槽编码，最后才提交链和预算；失败不会留下半条帧或占用槽位。
    NetInterfaceConnectionWrite& write = m_writes[m_free];
    if (m_spareCapacity >= required)
    {
        write.m_data = m_spare;
        write.m_capacity = m_spareCapacity;
        m_spare = nullptr;
        m_spareCapacity = 0;
    }
    else
    {
        write.m_capacity = required <= 4096 ? 64 : required;
        while (write.m_capacity < required)
        {
            write.m_capacity *= 2;
        }
        write.m_data = new (std::nothrow) uint8_t[write.m_capacity];
        if (write.m_data == nullptr)
        {
            write.m_capacity = 0;
            return NI_NO_MEMORY;
        }
    }
    const bool compress = std::find(m_compressionTypes, m_compressionTypes + m_compressionCount, frame.m_messageType) !=
        m_compressionTypes + m_compressionCount;
    const int32_t result = compress ? NetInterfaceCompression::encode(frame, write.m_data, write.m_capacity, required) :
        NetInterface::encode(frame, write.m_data, write.m_capacity, required);
    if (result != NI_OK)
    {
        // 失败时编码器可能写过前缀；清除整块容量后才允许复用。
        write.m_size = write.m_capacity;
        recycle(write);
        return result;
    }
    write.m_size = required;
    write.m_queue = queue;
    const uint32_t index = m_free;
    m_free = write.m_next;
    write.m_next = kWriteSlots;
    if (m_tails[queue] == kWriteSlots)
    {
        m_heads[queue] = index;
    }
    else
    {
        m_writes[m_tails[queue]].m_next = index;
    }
    m_tails[queue] = index;
    m_queueBytes[queue] += required;
    ++m_queueCounts[queue];
    return NI_OK;
}

int32_t NetInterfaceConnection::configureCompression(const uint32_t* types, uint32_t count, bool negotiated)
{
    if (count > 64 || (count != 0 && types == nullptr))
    {
        return NI_INVALID_ARGUMENT;
    }
    for (uint32_t index = 0; index < count; ++index)
    {
        if (types[index] == 0)
        {
            return NI_INVALID_ARGUMENT;
        }
    }
    m_compressionCount = negotiated ? count : 0;
    if (m_compressionCount != 0)
    {
        std::memmove(m_compressionTypes, types, m_compressionCount * sizeof(uint32_t));
    }
    m_decoder.allowCompression(negotiated);
    return NI_OK;
}

int32_t NetInterfaceConnection::peek(NetInterfaceFrame& frame) const
{
    if (m_error != 0)
    {
        frame = NetInterfaceFrame();
        return m_error;
    }
    return m_decoder.peek(frame);
}

int32_t NetInterfaceConnection::consume()
{
    return m_error != 0 ? m_error : m_decoder.consume();
}

int32_t NetInterfaceConnection::peekOutput(const uint8_t*& data, uint32_t& size) const
{
    return m_tls.peekOutput(data, size);
}

int32_t NetInterfaceConnection::consumeOutput(uint32_t size)
{
    return m_tls.consumeOutput(size);
}

int32_t NetInterfaceConnection::finishInput()
{
    return m_error != 0 ? m_error : m_tls.finishInput();
}

int32_t NetInterfaceConnection::status() const
{
    return m_error != 0 ? m_error : (m_closed ? NET_INTERFACE_TLS_CLOSED : m_tls.status());
}

int32_t NetInterfaceConnection::detail() const
{
    return m_tls.detail();
}

int32_t NetInterfaceConnection::fail(int32_t error)
{
    if (m_error == 0)
    {
        m_error = error;
        clearWrites();
        wipe(m_plain, sizeof(m_plain));
        m_plainSize = m_plainOffset = 0;
    }
    return m_error;
}

int32_t NetInterfaceConnection::outputState() const
{
    if (m_error < 0)
    {
        return m_error;
    }
    if (m_queueCounts[0] != 0 || m_queueCounts[1] != 0)
    {
        return NI_READY;
    }
    return m_tls.outputState();
}

// NET_INTERFACE_CONNECTION_UNIT_BEGIN
//#include "NetInterface.h"
//#include "../../../NetWorkTest/testdata/tls/test/test_certs.h"
//#include <cstdio>
//#include <cstring>
//#include <vector>
//#ifdef _DEBUG
//#include <crtdbg.h>
//#endif
//
///** 独立启用本main；只用公开接口穿过实际TLS核对帧，不把确定性测试随机源用于业务
//@return 0通过，1断言或异常失败
//*/
//int main()
//{
//    uint32_t checks = 0;
//    // condition为期望结果，message为静态错误说明；异常仅由本main捕获。
//    const auto Check = [&checks](bool condition, const char* message)
//    {
//        ++checks;
//        if (!condition)
//        {
//            std::printf("FAIL %s\n", message);
//            throw 1;
//        }
//    };
//    try
//    {
//        const uint8_t authority[] = TEST_CA_CRT_RSA_SHA256_PEM;
//        const uint8_t certificate[] = TEST_SRV_CRT_RSA_SHA256_PEM;
//        const uint8_t key[] = TEST_SRV_KEY_RSA_PEM;
//        uint32_t randomState = 0x1234a8b7;
//        // server选择证书方向，hostname仅在create时借用；返回只用于隔离测试的配置。
//        const auto Options = [&](bool server, const char* hostname) -> NetInterfaceTlsOptions
//        {
//            NetInterfaceTlsOptions options = {};
//            options.server = server ? 1 : 0;
//            options.authority = authority;
//            options.authoritySize = sizeof(authority);
//            options.certificate = certificate;
//            options.certificateSize = sizeof(certificate);
//            options.privateKey = key;
//            options.privateKeySize = sizeof(key);
//            options.hostname = hostname;
//            options.handshakeTimeoutMs = 10000;
//            options.entropyContext = &randomState;
//            // context借用本main的种子，output为size字节；可预测数据仅用来稳定复现队列测试。
//            options.entropy = [](void* context, uint8_t* output, uint32_t size) -> int32_t
//            {
//                uint32_t& state = *static_cast<uint32_t*>(context);
//                for (uint32_t index = 0; index < size; ++index)
//                {
//                    state ^= state << 13;
//                    state ^= state >> 17;
//                    state ^= state << 5;
//                    output[index] = static_cast<uint8_t>(state);
//                }
//                return 0;
//            };
//            return options;
//        };
//        // 从source复制最多733字节密文到destination，按实际接纳量消费，双端均借用至返回。
//        const auto TransferConnection = [&Check](NetInterfaceConnection& source, NetInterfaceConnection& destination)
//        {
//            const uint8_t* data = nullptr;
//            uint32_t size = 0;
//            Check(source.peekOutput(data, size) >= 0, "ciphertext peek");
//            uint32_t consumed = 0;
//            Check(destination.feed(data, (std::min)(size, 733U), consumed) >= 0, "ciphertext feed");
//            Check(source.consumeOutput(consumed) == NI_OK, "ciphertext consume");
//        };
//        // client/server借用至返回，实际握手最多20000轮，不依赖网络或工作线程。
//        const auto HandshakeConnection = [&Check, &TransferConnection](NetInterfaceConnection& client, NetInterfaceConnection& server)
//        {
//            for (uint32_t step = 0; step < 20000; ++step)
//            {
//                Check(client.progress(1000) >= 0 && server.progress(1000) >= 0, "TLS handshake progress");
//                TransferConnection(client, server);
//                TransferConnection(server, client);
//                if (client.status() == NET_INTERFACE_TLS_READY && server.status() == NET_INTERFACE_TLS_READY)
//                {
//                    return;
//                }
//            }
//            Check(false, "TLS handshake bound");
//        };
//#ifdef _DEBUG
//        _CrtMemState before;
//        _CrtMemState after;
//        _CrtMemCheckpoint(&before);
//#endif
//        {
//            NetInterfaceConnection client(Options(false, "localhost"));
//            NetInterfaceConnection server(Options(true, nullptr));
//            HandshakeConnection(client, server);
//            uint8_t payload[300];
//            for (uint32_t round = 0; round < 64; ++round)
//            {
//                // 每轮填满两种预算，拒绝的第65/17项不能破坏已排队数据或后续空槽。
//                for (uint32_t kind = 0; kind < 2; ++kind)
//                {
//                    const uint32_t count = kind == 0 ? 64 : 16;
//                    for (uint32_t index = 0; index < count; ++index)
//                    {
//                        const uint64_t id = static_cast<uint64_t>(round) * 1000 + kind * 100 + index + 1;
//                        std::memset(payload, static_cast<int>(id & 255), sizeof(payload));
//                        NetInterfaceFrame frame = {};
//                        frame.m_kind = static_cast<uint16_t>(kind == 0 ? NI_REQUEST : NI_PING);
//                        frame.m_requestId = id;
//                        frame.m_payload = payload;
//                        frame.m_payloadSize = index % 3 == 0 ? 0 : static_cast<uint32_t>(sizeof(payload));
//                        Check(client.send(frame) == NI_OK, "queue admission after recycled slots");
//                        std::memset(payload, 0xef, sizeof(payload));
//                        if (index + 1 == count)
//                        {
//                            Check(client.send(frame) == NI_LIMIT_EXCEEDED, "queue item budget lost");
//                        }
//                    }
//                }
//                uint32_t received = 0;
//                for (uint32_t step = 0; step < 10000 && received < 80; ++step)
//                {
//                    
//                    Check(client.progress(1000) >= 0, "queue sender progress");
//                    TransferConnection(client, server);
//                    Check(server.progress(1000) >= 0, "queue receiver progress");
//                    NetInterfaceFrame frame = {};
//                    if (server.peek(frame) != NI_READY)
//                    {
//                        continue;
//                    }
//                    const uint32_t kind = received < 16 ? 1 : 0;
//                    const uint32_t index = received < 16 ? received : received - 16;
//                    const uint64_t expected = static_cast<uint64_t>(round) * 1000 + kind * 100 + index + 1;
//                    Check(frame.m_requestId == expected && frame.m_kind == (kind == 0 ? NI_REQUEST : NI_PING) &&
//                        frame.m_payloadSize == (index % 3 == 0 ? 0 : sizeof(payload)), "queue identity or priority changed");
//                    for (uint32_t offset = 0; offset < frame.m_payloadSize; ++offset)
//                    {
//                        Check(frame.m_payload[offset] == static_cast<uint8_t>(expected), "queue borrowed caller storage");
//                    }
//                    Check(server.consume() == NI_OK, "queue consumption failed");
//                    ++received;
//                }
//                Check(received == 80 && client.outputState() == NI_OK, "queue failed to drain within bounded work");
//                NetInterfaceFrame invalid = {};
//                invalid.m_kind = 65535;
//                Check(client.send(invalid) == NI_INVALID_ARGUMENT, "invalid frame changed queue state");
//            }
//            const uint32_t type = 991;
//            Check(client.configureCompression(&type, 1, true) == NI_OK &&
//                server.configureCompression(&type, 1, true) == NI_OK, "compression setup");
//            std::vector<uint8_t> compressible(32768, 0x7b);
//            NetInterfaceFrame compressed = {};
//            compressed.m_kind = NI_REQUEST;
//            compressed.m_messageType = type;
//            compressed.m_requestId = 999999;
//            compressed.m_payload = compressible.data();
//            compressed.m_payloadSize = static_cast<uint32_t>(compressible.size());
//            Check(client.send(compressed) == NI_OK, "compressed queue admission");
//            bool complete = false;
//            for (uint32_t step = 0; step < 10000 && !complete; ++step)
//            {
//                
//                client.progress(1000);
//                TransferConnection(client, server);
//                Check(server.progress(1000) >= 0, "compressed queue receiver");
//                NetInterfaceFrame frame = {};
//                if (server.peek(frame) == NI_READY)
//                {
//                    Check(frame.m_requestId == compressed.m_requestId && frame.m_payloadSize == compressible.size() &&
//                        std::memcmp(frame.m_payload, compressible.data(), compressible.size()) == 0, "compressed queue bytes");
//                    server.consume();
//                    complete = true;
//                }
//            }
//            Check(complete && client.outputState() == NI_OK, "compressed queue retained budget");
//        }
//#ifdef _DEBUG
//        // 分配钩子只覆盖单次send；不把TLS加解密和测试正文容器的分配算入发送队列。
//        static uint32_t s_allocations = 0;
//        static bool s_reject = false;
//        // CRT规定的参数；只统计普通分配，可拒绝本次send的分配以验证回滚。
//        const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
//        {
//            if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK)
//            {
//                ++s_allocations;
//                return s_reject ? 0 : 1;
//            }
//            return 1;
//        };
//        {
//            NetInterfaceConnection client(Options(false, "localhost"));
//            NetInterfaceConnection server(Options(true, nullptr));
//            HandshakeConnection(client, server);
//            NetInterfaceFrame frame = {};
//            frame.m_kind = NI_REQUEST;
//            uint8_t large[5000] = {};
//            frame.m_payload = large;
//            frame.m_payloadSize = sizeof(large);
//            s_reject = true;
//            _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
//            const int32_t failed = client.send(frame);
//            _CrtSetAllocHook(previous);
//            s_reject = false;
//            Check(failed == NI_NO_MEMORY && client.outputState() == NI_OK, "failed frame allocation committed a slot");
//            frame.m_payloadSize = 0;
//            uint32_t subsequentAllocations = 0;
//            for (uint32_t index = 0; index < 1024; ++index)
//            {
//                frame.m_requestId = index + 1;
//                s_allocations = 0;
//                previous = _CrtSetAllocHook(Hook);
//                const int32_t sent = client.send(frame);
//                _CrtSetAllocHook(previous);
//                Check(sent == NI_OK, "small message after allocation rollback");
//                if (index != 0)
//                {
//                    subsequentAllocations += s_allocations;
//                }
//                bool received = false;
//                for (uint32_t step = 0; step < 1000 && !received; ++step)
//                {
//                    client.progress(1000);
//                    TransferConnection(client, server);
//                    Check(server.progress(1000) >= 0, "small message progress");
//                    NetInterfaceFrame incoming = {};
//                    if (server.peek(incoming) == NI_READY)
//                    {
//                        Check(incoming.m_requestId == frame.m_requestId && incoming.m_payloadSize == 0,
//                            "recycled small message identity");
//                        server.consume();
//                        received = true;
//                    }
//                }
//                Check(received, "small message delivery bound");
//            }
//            Check(subsequentAllocations == 0, "warmed small send still allocates queue storage");
//            // 保留未发送大帧直接析构，验证队列拥有者能完整回收。
//            frame.m_payloadSize = sizeof(large);
//            for (uint32_t index = 0; index < 64; ++index)
//            {
//                Check(client.send(frame) == NI_OK, "abandoned queue population");
//            }
//        }
//        _CrtMemCheckpoint(&after);
//        Check(after.lCounts[_NORMAL_BLOCK] == before.lCounts[_NORMAL_BLOCK] &&
//            after.lSizes[_NORMAL_BLOCK] == before.lSizes[_NORMAL_BLOCK], "connection queue leaked memory");
//#endif
//        std::printf("NetInterface connection ownership checks=%u PASS\n", checks);
//        return 0;
//    }
//    catch (...)
//    {
//        std::printf("NetInterface connection ownership checks=%u FAIL\n", checks);
//        return 1;
//    }
//}
// NET_INTERFACE_CONNECTION_UNIT_END

#if 0 // CONNECTION_BOUNDARY_UNIT_BEGIN
#include <algorithm>
#include "NetInterface.h"
#include <test/test_certs.h>
#include <cstdio>
#include <cstring>
#include <vector>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

/** 独立启用本main；只用公开接口穿过实际TLS核对帧，不把确定性测试随机源用于业务
@return 0通过，1断言或异常失败
*/
int main()
{
    uint32_t checks = 0;
    // condition为期望结果，message为静态错误说明；异常仅由本main捕获。
    const auto Check = [&checks](bool condition, const char* message)
    {
        ++checks;
        if (!condition)
        {
            std::printf("FAIL %s\n", message);
            throw 1;
        }
    };
    try
    {
        const uint8_t authority[] = TEST_CA_CRT_RSA_SHA256_PEM;
        const uint8_t certificate[] = TEST_SRV_CRT_RSA_SHA256_PEM;
        const uint8_t key[] = TEST_SRV_KEY_RSA_PEM;
        uint32_t randomState = 0x1234a8b7;
        // server选择证书方向，hostname仅在create时借用；返回只用于隔离测试的配置。
        const auto Options = [&](bool server, const char* hostname) -> NetInterfaceTlsOptions
        {
            NetInterfaceTlsOptions options = {};
            options.server = server ? 1 : 0;
            options.authority = authority;
            options.authoritySize = sizeof(authority);
            options.certificate = certificate;
            options.certificateSize = sizeof(certificate);
            options.privateKey = key;
            options.privateKeySize = sizeof(key);
            options.hostname = hostname;
            options.handshakeTimeoutMs = 10000;
            options.entropyContext = &randomState;
            // context借用本main的种子，output为size字节；可预测数据仅用来稳定复现队列测试。
            options.entropy = [](void* context, uint8_t* output, uint32_t size) -> int32_t
            {
                uint32_t& state = *static_cast<uint32_t*>(context);
                for (uint32_t index = 0; index < size; ++index)
                {
                    state ^= state << 13;
                    state ^= state >> 17;
                    state ^= state << 5;
                    output[index] = static_cast<uint8_t>(state);
                }
                return 0;
            };
            return options;
        };
        // 从source复制最多733字节密文到destination，按实际接纳量消费，双端均借用至返回。
        const auto TransferConnection = [&Check](NetInterfaceConnection& source, NetInterfaceConnection& destination)
        {
            const uint8_t* data = nullptr;
            uint32_t size = 0;
            Check(source.peekOutput(data, size) >= 0, "ciphertext peek");
            uint32_t consumed = 0;
            Check(destination.feed(data, (std::min)(size, 733U), consumed) >= 0, "ciphertext feed");
            Check(source.consumeOutput(consumed) == NI_OK, "ciphertext consume");
        };
        // client/server借用至返回，实际握手最多20000轮，不依赖网络或工作线程。
        const auto HandshakeConnection = [&Check, &TransferConnection](NetInterfaceConnection& client, NetInterfaceConnection& server)
        {
            for (uint32_t step = 0; step < 20000; ++step)
            {
                Check(client.progress(1000) >= 0 && server.progress(1000) >= 0, "TLS handshake progress");
                TransferConnection(client, server);
                TransferConnection(server, client);
                if (client.status() == NET_INTERFACE_TLS_READY && server.status() == NET_INTERFACE_TLS_READY)
                {
                    return;
                }
            }
            Check(false, "TLS handshake bound");
        };
        {
            NetInterfaceConnection client(Options(false, "localhost"));
            NetInterfaceConnection server(Options(true, nullptr));
            HandshakeConnection(client, server);
            std::vector<uint8_t> payload(1048576);
            for (size_t index = 0; index < payload.size(); ++index)
            {
                payload[index] = static_cast<uint8_t>(index * 37 + 11);
            }
            NetInterfaceFrame outgoing = {};
            outgoing.m_kind = NI_REQUEST;
            outgoing.m_requestId = 1;
            outgoing.m_payload = payload.data();
            outgoing.m_payloadSize = static_cast<uint32_t>(payload.size());
            Check(client.send(outgoing) == NI_OK, "large frame admission");
            Check(client.progress(1000) >= 0 && client.outputState() == NI_READY, "partial large frame starts");
            const uint8_t* encrypted = nullptr;
            uint32_t encryptedSize = 0;
            Check(client.peekOutput(encrypted, encryptedSize) >= 0 && encrypted != nullptr &&
                encryptedSize > 0 && encryptedSize <= 32768, "bounded TLS output held");
            // 大帧已经交出前缀，后到控制帧只能等其完整结束，不能插进明文中间。
            outgoing.m_payload = nullptr;
            outgoing.m_payloadSize = 0;
            outgoing.m_kind = NI_PING;
            for (uint64_t id = 100; id < 116; ++id)
            {
                outgoing.m_requestId = id;
                Check(client.send(outgoing) == NI_OK, "control reserve during partial frame");
            }
            Check(client.send(outgoing) == NI_LIMIT_EXCEEDED, "full control reserve rejects transactionally");
            outgoing.m_kind = NI_REQUEST;
            outgoing.m_requestId = 2;
            Check(client.send(outgoing) == NI_OK, "ordinary successor admitted");
            uint32_t received = 0;
            for (uint32_t step = 0; step < 20000 && received < 18; ++step)
            {
                Check(client.progress(1000) >= 0, "partial sender progress");
                TransferConnection(client, server);
                Check(server.progress(1000) >= 0, "partial receiver progress");
                NetInterfaceFrame incoming = {};
                if (server.peek(incoming) != NI_READY)
                {
                    continue;
                }
                const uint64_t expectedId = received == 0 ? 1 : received == 17 ? 2 : 99 + received;
                const uint16_t expectedKind = static_cast<uint16_t>(received == 0 || received == 17 ? NI_REQUEST : NI_PING);
                Check(incoming.m_requestId == expectedId && incoming.m_kind == expectedKind, "active frame/control/successor ordering");
                if (received == 0)
                {
                    Check(incoming.m_payloadSize == payload.size() && std::memcmp(incoming.m_payload, payload.data(), payload.size()) == 0,
                        "partial payload preserved through control insertion");
                    const uint8_t* borrowed = incoming.m_payload;
                    // 接收者持续占用首帧，后到控制帧不得覆盖已借用数据。
                    for (uint32_t hold = 0; hold < 100; ++hold)
                    {
                        Check(client.progress(1000) >= 0, "held sender progress");
                        TransferConnection(client, server);
                        Check(server.progress(1000) >= 0 && server.peek(incoming) == NI_READY && incoming.m_payload == borrowed &&
                            incoming.m_requestId == 1 && std::memcmp(incoming.m_payload, payload.data(), payload.size()) == 0,
                            "borrowed large frame remains stable under control backlog");
                    }
                }
                else
                {
                    Check(incoming.m_payloadSize == 0, "control and successor payload boundary");
                }
                Check(server.consume() == NI_OK, "frame consumed once");
                ++received;
            }
            Check(received == 18, "bounded delivery of all accepted frames");
            for (uint32_t step = 0; step < 16; ++step)
            {
                Check(client.progress(1000) >= 0, "final sender progress");
                TransferConnection(client, server);
                Check(server.progress(1000) >= 0, "final receiver progress");
            }
            NetInterfaceFrame extra = {};
            Check(server.peek(extra) == NI_NEED_MORE && client.outputState() == NI_OK, "no duplicate and all budgets drained");
        }
        std::printf("Connection partial frame/control boundary checks=%u PASS\n", checks);
        return 0;
    }
    catch (...)
    {
        return 1;
    }
}

#endif // CONNECTION_BOUNDARY_UNIT_END