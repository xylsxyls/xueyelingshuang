#include "NetConnectionQueueTest.h"
#include <stdexcept>
#include <algorithm>
#include "NetInterface/NetInterfaceAPI.h"
#include "NetTestTlsCertificateData.h"
#include <cstdio>
#include <cstring>
#include <vector>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

/** 独立启用本main；只用公开接口穿过实际TLS核对帧，不把确定性测试随机源用于业务
@return 0通过，1断言或异常失败
*/
int NetConnectionQueueTest::check()
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
#ifdef _DEBUG
        _CrtMemState before;
        _CrtMemState after;
        _CrtMemCheckpoint(&before);
#endif
        {
            NetInterfaceConnection client(Options(false, "localhost"));
            NetInterfaceConnection server(Options(true, nullptr));
            NetConnectionPoolConfig pools;
            pools.m_dataRecords.initialCapacity = pools.m_dataRecords.maximumCapacity = 64;
            pools.m_controlRecords.initialCapacity = pools.m_controlRecords.maximumCapacity = 16;
            Check(client.configurePools(pools) == NI_OK, "explicit physical queue pools");
            HandshakeConnection(client, server);
            Check(client.configurePools(NetConnectionPoolConfig()) == NI_INVALID_STATE, "pool reconfiguration after handshake");
            uint8_t payload[300];
            for (uint32_t round = 0; round < 64; ++round)
            {
                // 每轮填满两种预算，拒绝的第65/17项不能破坏已排队数据或后续空槽。
                for (uint32_t kind = 0; kind < 2; ++kind)
                {
                    const uint32_t count = kind == 0 ? 64 : 16;
                    for (uint32_t index = 0; index < count; ++index)
                    {
                        const uint64_t id = static_cast<uint64_t>(round) * 1000 + kind * 100 + index + 1;
                        std::memset(payload, static_cast<int>(id & 255), sizeof(payload));
                        NetInterfaceFrame frame = {};
                        frame.m_kind = static_cast<uint16_t>(kind == 0 ? NI_REQUEST : NI_PING);
                        frame.m_requestId = id;
                        frame.m_payload = payload;
                        frame.m_payloadSize = index % 3 == 0 ? 0 : static_cast<uint32_t>(sizeof(payload));
                        Check(client.send(frame) == NI_OK, "queue admission after recycled slots");
                        std::memset(payload, 0xef, sizeof(payload));
                        if (index + 1 == count)
                        {
                            Check(client.send(frame) == NI_LIMIT_EXCEEDED, "queue item budget lost");
                        }
                    }
                }
                uint32_t received = 0;
                for (uint32_t step = 0; step < 10000 && received < 80; ++step)
                {
                    
                    Check(client.progress(1000) >= 0, "queue sender progress");
                    TransferConnection(client, server);
                    Check(server.progress(1000) >= 0, "queue receiver progress");
                    NetInterfaceFrame frame = {};
                    if (server.peek(frame) != NI_READY)
                    {
                        continue;
                    }
                    const uint32_t kind = received < 16 ? 1 : 0;
                    const uint32_t index = received < 16 ? received : received - 16;
                    const uint64_t expected = static_cast<uint64_t>(round) * 1000 + kind * 100 + index + 1;
                    Check(frame.m_requestId == expected && frame.m_kind == (kind == 0 ? NI_REQUEST : NI_PING) &&
                        frame.m_payloadSize == (index % 3 == 0 ? 0 : sizeof(payload)), "queue identity or priority changed");
                    for (uint32_t offset = 0; offset < frame.m_payloadSize; ++offset)
                    {
                        Check(frame.m_payload[offset] == static_cast<uint8_t>(expected), "queue borrowed caller storage");
                    }
                    Check(server.consume() == NI_OK, "queue consumption failed");
                    ++received;
                }
                Check(received == 80 && client.outputState() == NI_OK, "queue failed to drain within bounded work");
                NetInterfaceFrame invalid = {};
                invalid.m_kind = 65535;
                Check(client.send(invalid) == NI_INVALID_ARGUMENT, "invalid frame changed queue state");
            }
            const uint32_t type = 991;
            Check(client.configureCompression(&type, 1, true) == NI_OK &&
                server.configureCompression(&type, 1, true) == NI_OK, "compression setup");
            std::vector<uint8_t> compressible(32768, 0x7b);
            NetInterfaceFrame compressed = {};
            compressed.m_kind = NI_REQUEST;
            compressed.m_messageType = type;
            compressed.m_requestId = 999999;
            compressed.m_payload = compressible.data();
            compressed.m_payloadSize = static_cast<uint32_t>(compressible.size());
            Check(client.send(compressed) == NI_OK, "compressed queue admission");
            bool complete = false;
            for (uint32_t step = 0; step < 10000 && !complete; ++step)
            {
                
                client.progress(1000);
                TransferConnection(client, server);
                Check(server.progress(1000) >= 0, "compressed queue receiver");
                NetInterfaceFrame frame = {};
                if (server.peek(frame) == NI_READY)
                {
                    Check(frame.m_requestId == compressed.m_requestId && frame.m_payloadSize == compressible.size() &&
                        std::memcmp(frame.m_payload, compressible.data(), compressible.size()) == 0, "compressed queue bytes");
                    server.consume();
                    complete = true;
                }
            }
            Check(complete && client.outputState() == NI_OK, "compressed queue retained budget");
        }
        {
            NetInterfaceConnection client(Options(false, "localhost"));
            NetInterfaceConnection server(Options(true, nullptr));
            NetConnectionPoolConfig invalid;
            invalid.m_dataRecords.initialCapacity = 2;
            invalid.m_dataRecords.maximumCapacity = 1;
            Check(client.configurePools(invalid) == NI_INVALID_ARGUMENT, "invalid pool configuration rollback");
            HandshakeConnection(client, server);
            // 同一个连接先扩容再复用；普通帧和控制帧都远超旧的64/16条上限。
            for (uint32_t round = 0; round < 2; ++round)
            {
                for (uint32_t kind = 0; kind < 2; ++kind)
                {
                    const uint32_t count = kind == 0 ? 2048 : 128;
                    for (uint32_t index = 0; index < count; ++index)
                    {
                        NetInterfaceFrame frame = {};
                        frame.m_kind = static_cast<uint16_t>(kind == 0 ? NI_DATA : NI_PING);
                        frame.m_requestId = index + 1;
                        frame.m_payload = reinterpret_cast<const uint8_t*>("123456");
                        frame.m_payloadSize = 6;
                        Check(client.send(frame) == NI_OK, "default pools retained fixed queue limits");
                    }
                }
                uint32_t received[2] = {};
                for (uint32_t step = 0; step < 40000 && received[0] + received[1] != 2176; ++step)
                {
                    Check(client.progress(1000) >= 0, "grown queue sender progress");
                    TransferConnection(client, server);
                    Check(server.progress(1000) >= 0, "grown queue receiver progress");
                    NetInterfaceFrame frame = {};
                    if (server.peek(frame) != NI_READY)
                    {
                        continue;
                    }
                    const uint32_t kind = frame.m_kind == NI_DATA ? 0 : 1;
                    Check(frame.m_kind == NI_DATA || frame.m_kind == NI_PING, "grown queue kind");
                    Check(frame.m_requestId == received[kind] + 1 && frame.m_payloadSize == 6 &&
                        std::memcmp(frame.m_payload, "123456", 6) == 0, "grown queue FIFO or plaintext");
                    if (kind == 0 && received[0] == 0)
                    {
                        Check(received[1] <= 32, "control stream starved ordinary frames");
                    }
                    ++received[kind];
                    Check(server.consume() == NI_OK, "grown queue consume");
                }
                Check(received[0] == 2048 && received[1] == 128 && client.outputState() == NI_OK,
                    "grown queue failed to drain all retained records");
            }
            // 最大合法正文连同帧头必须能分配，不能被正文大小类误拒绝。
            std::vector<uint8_t> body(1048576, 0x6d);
            NetInterfaceFrame largest = {};
            largest.m_kind = NI_DATA;
            largest.m_requestId = 9000;
            largest.m_payload = body.data();
            largest.m_payloadSize = static_cast<uint32_t>(body.size());
            Check(client.send(largest) == NI_OK, "maximum body plus header pool allocation");
            bool complete = false;
            for (uint32_t step = 0; step < 40000 && !complete; ++step)
            {
                Check(client.progress(1000) >= 0, "maximum body sender progress");
                TransferConnection(client, server);
                Check(server.progress(1000) >= 0, "maximum body receiver progress");
                NetInterfaceFrame frame = {};
                if (server.peek(frame) == NI_READY)
                {
                    Check(frame.m_requestId == largest.m_requestId && frame.m_payloadSize == body.size() &&
                        std::memcmp(frame.m_payload, body.data(), body.size()) == 0, "maximum body fragmented delivery");
                    Check(server.consume() == NI_OK, "maximum body consume");
                    complete = true;
                }
            }
            Check(complete && client.outputState() == NI_OK, "maximum body partial frame retained storage");
        }
#ifdef _DEBUG
        // 分配钩子只覆盖单次send；不把TLS加解密和测试正文容器的分配算入发送队列。
        static uint32_t s_allocations = 0;
        static bool s_reject = false;
        // CRT规定的参数；只统计普通分配，可拒绝本次send的分配以验证回滚。
        const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
        {
            if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK)
            {
                ++s_allocations;
                return s_reject ? 0 : 1;
            }
            return 1;
        };
        {
            NetInterfaceConnection client(Options(false, "localhost"));
            NetInterfaceConnection server(Options(true, nullptr));
            HandshakeConnection(client, server);
            NetInterfaceFrame frame = {};
            frame.m_kind = NI_REQUEST;
            uint8_t large[5000] = {};
            frame.m_payload = large;
            frame.m_payloadSize = sizeof(large);
            s_reject = true;
            _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
            const int32_t failed = client.send(frame);
            _CrtSetAllocHook(previous);
            s_reject = false;
            Check(failed == NI_NO_MEMORY && client.outputState() == NI_OK, "failed frame allocation committed a slot");
            frame.m_payloadSize = 0;
            uint32_t subsequentAllocations = 0;
            for (uint32_t index = 0; index < 1024; ++index)
            {
                frame.m_requestId = index + 1;
                s_allocations = 0;
                previous = _CrtSetAllocHook(Hook);
                const int32_t sent = client.send(frame);
                _CrtSetAllocHook(previous);
                Check(sent == NI_OK, "small message after allocation rollback");
                if (index != 0)
                {
                    subsequentAllocations += s_allocations;
                }
                bool received = false;
                for (uint32_t step = 0; step < 1000 && !received; ++step)
                {
                    client.progress(1000);
                    TransferConnection(client, server);
                    Check(server.progress(1000) >= 0, "small message progress");
                    NetInterfaceFrame incoming = {};
                    if (server.peek(incoming) == NI_READY)
                    {
                        Check(incoming.m_requestId == frame.m_requestId && incoming.m_payloadSize == 0,
                            "recycled small message identity");
                        server.consume();
                        received = true;
                    }
                }
                Check(received, "small message delivery bound");
            }
            Check(subsequentAllocations == 0, "warmed small send still allocates queue storage");
            // 保留未发送大帧直接析构，验证队列拥有者能完整回收。
            frame.m_payloadSize = sizeof(large);
            for (uint32_t index = 0; index < 64; ++index)
            {
                Check(client.send(frame) == NI_OK, "abandoned queue population");
            }
        }
        _CrtMemCheckpoint(&after);
        Check(after.lCounts[_NORMAL_BLOCK] == before.lCounts[_NORMAL_BLOCK] &&
            after.lSizes[_NORMAL_BLOCK] == before.lSizes[_NORMAL_BLOCK], "connection queue leaked memory");
#endif
        std::printf("NetInterface connection ownership checks=%u PASS\n", checks);
        return 0;
    }
    catch (...)
    {
        std::printf("NetInterface connection ownership checks=%u FAIL\n", checks);
        return 1;
    }
}

void NetConnectionQueueTest::run()
{
    if (check() != 0)
    {
        throw std::runtime_error("NetConnectionQueueTest failed");
    }
}