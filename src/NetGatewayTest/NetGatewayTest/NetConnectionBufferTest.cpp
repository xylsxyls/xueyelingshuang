#include "NetConnectionBufferTest.h"
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
int NetConnectionBufferTest::check()
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
            NetConnectionPoolConfig pools;
            pools.m_controlRecords.initialCapacity = pools.m_controlRecords.maximumCapacity = 16;
            Check(client.configurePools(pools) == NI_OK, "explicit control pool configuration");
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

void NetConnectionBufferTest::run()
{
    if (check() != 0)
    {
        throw std::runtime_error("NetConnectionBufferTest failed");
    }
}