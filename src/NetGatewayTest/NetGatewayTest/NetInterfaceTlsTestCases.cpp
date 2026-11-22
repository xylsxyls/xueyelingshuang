#include "NetInterfaceTlsTestCases.h"
#include "NetGatewaySuiteCases.h"
#include "NetTestTlsCertificates.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#elif defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

#ifdef __EMSCRIPTEN__
// 测试宿主提供的强随机函数；正式微信桥接通过异步API预填随机池
EM_JS(int, NetGatewayTestRandom, (unsigned char* output, unsigned int size), {
    try {
        var bytes = new Uint8Array(size);
        if (typeof require === 'function') {
            require('crypto').randomFillSync(bytes);
        } else if (globalThis.crypto && globalThis.crypto.getRandomValues) {
            globalThis.crypto.getRandomValues(bytes);
        } else {
            return -1;
        }
        var address = Number(output);
        if (!Number.isSafeInteger(address) || address < 0 || size > HEAPU8.length - address) return -1;
        HEAPU8.set(bytes, address);
        return 0;
    } catch (error) {
        return -1;
    }
});
#endif

NetInterfaceTlsTestPair::NetInterfaceTlsTestPair(const char* hostname, bool wrongAuthority) :
m_client(new NetInterfaceTls(options(false, hostname, wrongAuthority))),
m_server(new NetInterfaceTls(options(true, nullptr)))
{
    require(m_client->status() == NET_INTERFACE_TLS_HANDSHAKE, "client configuration failed");
    require(m_server->status() == NET_INTERFACE_TLS_HANDSHAKE, "server configuration failed");
}

bool NetInterfaceTlsTestPair::handshake(uint32_t fragment)
{
    for (uint32_t step = 0; step < 20000; ++step)
    {
        netTestCheckpoint();
        m_client->progress(1000 + step / 64);
        m_server->progress(1000 + step / 64);
        if (m_client->status() < 0 || m_server->status() < 0)
        {
            return false;
        }
        transfer(*m_client, *m_server, fragment);
        transfer(*m_server, *m_client, fragment);
        if (m_client->status() == NET_INTERFACE_TLS_READY &&
            m_server->status() == NET_INTERFACE_TLS_READY)
        {
            return true;
        }
    }
    throw std::runtime_error("TLS handshake exceeded bounded steps");
}

uint32_t NetInterfaceTlsTestPair::transfer(NetInterfaceTls& source,
    NetInterfaceTls& destination, uint32_t fragment)
{
    const uint8_t* data = nullptr;
    uint32_t size = 0;
    source.peekOutput(data, size);
    if (size == 0 || destination.status() < 0)
    {
        return 0;
    }
    uint32_t consumed = 0;
    int32_t status = destination.feed(data, (std::min)(size, fragment), consumed);
    require(status >= 0, "peer rejected ciphertext input");
    require(source.consumeOutput(consumed) == NET_INTERFACE_TLS_OK,
        "ciphertext consumption failed");
    return consumed;
}

NetInterfaceTlsOptions NetInterfaceTlsTestPair::options(bool server,
    const char* hostname, bool wrongAuthority)
{
    NetInterfaceTlsOptions options = {};
    options.server = server ? 1 : 0;
    options.hostname = hostname;
    options.authority = reinterpret_cast<const uint8_t*>(wrongAuthority ?
        mbedtls_test_ca_crt_ec_pem : mbedtls_test_ca_crt_rsa_sha256_pem);
    options.authoritySize = static_cast<uint32_t>(wrongAuthority ?
        mbedtls_test_ca_crt_ec_pem_len : mbedtls_test_ca_crt_rsa_sha256_pem_len);
    options.certificate = reinterpret_cast<const uint8_t*>(mbedtls_test_srv_crt_rsa_sha256_pem);
    options.certificateSize = static_cast<uint32_t>(mbedtls_test_srv_crt_rsa_sha256_pem_len);
    options.privateKey = reinterpret_cast<const uint8_t*>(mbedtls_test_srv_key_rsa_pem);
    options.privateKeySize = static_cast<uint32_t>(mbedtls_test_srv_key_rsa_pem_len);
    options.entropy = &NetInterfaceTlsTestPair::strongRandom;
    options.handshakeTimeoutMs = 10000;
    return options;
}

int32_t NetInterfaceTlsTestPair::strongRandom(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
#ifdef __EMSCRIPTEN__
    return NetGatewayTestRandom(output, size);
#elif defined(_WIN32)
    return BCryptGenRandom(nullptr, output, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0 ? 0 : -1;
#else
    int descriptor = open("/dev/urandom", O_RDONLY);
    if (descriptor < 0)
    {
        return -1;
    }
    uint32_t offset = 0;
    while (offset < size)
    {
        ssize_t count = ::read(descriptor, output + offset, size - offset);
        if (count < 0 && errno == EINTR)
        {
            continue;
        }
        if (count <= 0)
        {
            close(descriptor);
            return -1;
        }
        offset += static_cast<uint32_t>(count);
    }
    close(descriptor);
    return 0;
#endif
}

int32_t NetInterfaceTlsTestPair::failedRandom(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    (void)output;
    (void)size;
    return -1;
}

void NetInterfaceTlsTestPair::require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void NetInterfaceTlsTestCases::handshakeAndClose()
{
    NetInterfaceTlsTestPair pair("localhost");
    NetInterfaceTlsTestPair::require(pair.handshake(1), "trusted fragmented handshake failed");
    const uint8_t payload[] = {0, 1, 2, 128, 255, 0, 36};
    uint32_t accepted = 0;
    NetInterfaceTlsTestPair::require(pair.m_client->write(payload, sizeof(payload), accepted) ==
        NET_INTERFACE_TLS_OK && accepted == sizeof(payload), "binary write failed");
    pair.m_client->progress(2000);
    NetInterfaceTlsTestPair::transfer(*pair.m_client, *pair.m_server, 32768);
    uint8_t output[64] = {};
    uint32_t received = 0;
    NetInterfaceTlsTestPair::require(pair.m_server->read(output, sizeof(output), received) ==
        NET_INTERFACE_TLS_OK && received == sizeof(payload) &&
        std::memcmp(payload, output, received) == 0, "authenticated binary payload differs");
    NetInterfaceTlsTestPair::require(pair.m_client->close() == NET_INTERFACE_TLS_OK,
        "authenticated shutdown failed");
    NetInterfaceTlsTestPair::transfer(*pair.m_client, *pair.m_server, 32768);
    pair.m_server->finishInput();
    NetInterfaceTlsTestPair::require(pair.m_server->read(output, sizeof(output), received) ==
        NET_INTERFACE_TLS_CLOSED && received == 0, "close_notify not recognized");
    NetInterfaceTlsTestPair::require(pair.m_client->write(payload, 1, accepted) ==
        NET_INTERFACE_TLS_STATE && accepted == 0, "write allowed after local close");
}

void NetInterfaceTlsTestCases::certificateRejection()
{
    NetInterfaceTlsTestPair hostname("unrelated.invalid");
    NetInterfaceTlsTestPair::require(!hostname.handshake(32768) &&
        hostname.m_client->status() == NET_INTERFACE_TLS_PROTOCOL &&
        hostname.m_client->detail() != 0, "wrong hostname accepted");
    NetInterfaceTlsTestPair authority("localhost", true);
    NetInterfaceTlsTestPair::require(!authority.handshake(32768) &&
        authority.m_client->status() == NET_INTERFACE_TLS_PROTOCOL,
        "untrusted certificate accepted");
}

void NetInterfaceTlsTestCases::tamperRejection()
{
    NetInterfaceTlsTestPair pair("localhost");
    NetInterfaceTlsTestPair::require(pair.handshake(32768), "handshake failed before tamper");
    const std::string secret = "private account message which must remain encrypted";
    uint32_t accepted = 0;
    pair.m_client->write(reinterpret_cast<const uint8_t*>(secret.data()),
        static_cast<uint32_t>(secret.size()), accepted);
    pair.m_client->progress(2000);
    const uint8_t* wire = nullptr;
    uint32_t size = 0;
    pair.m_client->peekOutput(wire, size);
    NetInterfaceTlsTestPair::require(size > secret.size() &&
        std::search(wire, wire + size, secret.begin(), secret.end()) == wire + size,
        "plaintext exposed in ciphertext output");
    std::vector<uint8_t> damaged(wire, wire + size);
    damaged.back() ^= 1;
    uint32_t consumed = 0;
    pair.m_server->feed(damaged.data(), size, consumed);
    uint8_t output[128] = {};
    uint32_t received = 0;
    NetInterfaceTlsTestPair::require(pair.m_server->read(output, sizeof(output), received) ==
        NET_INTERFACE_TLS_PROTOCOL && received == 0, "tampered record exposed plaintext");
    NetInterfaceTlsTestPair::require(pair.m_server->progress(3000) == NET_INTERFACE_TLS_PROTOCOL,
        "TLS error did not remain sticky");
}

void NetInterfaceTlsTestCases::replayRejection()
{
    NetInterfaceTlsTestPair pair("localhost");
    NetInterfaceTlsTestPair::require(pair.handshake(32768), "handshake failed before replay");
    uint32_t accepted = 0;
    const uint8_t message[] = {9, 8, 7};
    pair.m_client->write(message, sizeof(message), accepted);
    pair.m_client->progress(2000);
    const uint8_t* wire = nullptr;
    uint32_t size = 0;
    pair.m_client->peekOutput(wire, size);
    std::vector<uint8_t> replay(wire, wire + size);
    uint32_t consumed = 0;
    uint32_t received = 0;
    uint8_t output[32] = {};
    pair.m_server->feed(replay.data(), size, consumed);
    NetInterfaceTlsTestPair::require(pair.m_server->read(output, sizeof(output), received) ==
        NET_INTERFACE_TLS_OK && received == sizeof(message), "first record failed");
    pair.m_server->feed(replay.data(), size, consumed);
    NetInterfaceTlsTestPair::require(pair.m_server->read(output, sizeof(output), received) ==
        NET_INTERFACE_TLS_PROTOCOL && received == 0, "TLS sequence replay accepted");
}

void NetInterfaceTlsTestCases::failures()
{
    NetInterfaceTlsOptions options = NetInterfaceTlsTestPair::options(false, "localhost");
    options.handshakeTimeoutMs = 100;
    NetInterfaceTls timeout(options);
    NetInterfaceTlsTestPair::require(timeout.progress(0) == NET_INTERFACE_TLS_HANDSHAKE &&
        timeout.progress(99) == NET_INTERFACE_TLS_HANDSHAKE &&
        timeout.progress(100) == NET_INTERFACE_TLS_TIMEOUT &&
        timeout.progress(101) == NET_INTERFACE_TLS_TIMEOUT, "handshake deadline failed");
    NetInterfaceTls clock(options);
    clock.progress(123);
    NetInterfaceTlsTestPair::require(clock.progress(122) == NET_INTERFACE_TLS_ARGUMENT,
        "backward monotonic clock accepted");
    NetInterfaceTlsTestPair eof("localhost");
    NetInterfaceTlsTestPair::require(eof.handshake(32768), "EOF test handshake failed");
    eof.m_server->finishInput();
    uint8_t output[64] = {};
    uint32_t received = 0;
    NetInterfaceTlsTestPair::require(eof.m_server->read(output, sizeof(output), received) ==
        NET_INTERFACE_TLS_TRUNCATED && received == 0, "bare TCP EOF treated as clean close");
    NetInterfaceTlsTestPair truncated("localhost");
    NetInterfaceTlsTestPair::require(truncated.handshake(32768), "truncation handshake failed");
    uint32_t accepted = 0;
    truncated.m_client->write(output, sizeof(output), accepted);
    truncated.m_client->progress(2000);
    const uint8_t* wire = nullptr;
    uint32_t size = 0;
    truncated.m_client->peekOutput(wire, size);
    NetInterfaceTlsTestPair::require(size > 1, "missing ciphertext");
    uint32_t consumed = 0;
    truncated.m_server->feed(wire, size - 1, consumed);
    truncated.m_server->finishInput();
    NetInterfaceTlsTestPair::require(truncated.m_server->read(output, sizeof(output), received) ==
        NET_INTERFACE_TLS_TRUNCATED && received == 0, "partial TLS record accepted");
}

void NetInterfaceTlsTestCases::cApiAndEntropy()
{
    NetInterfaceTlsOptions options = NetInterfaceTlsTestPair::options(false, "localhost");
    NetInterfaceTlsHandle handle = nullptr;
    int32_t detail = 123;
    NetInterfaceTlsTestPair::require(NetInterface::tlsCreate(&options, &handle, &detail) ==
        NET_INTERFACE_TLS_HANDSHAKE && handle != nullptr && detail == 0, "TLS C create failed");
    uint32_t count = 123;
    NetInterfaceTlsTestPair::require(NetInterface::tlsFeed(handle, nullptr, 1, &count) ==
        NET_INTERFACE_TLS_ARGUMENT && count == 0, "null input not rejected");
    NetInterfaceTlsTestPair::require(NetInterface::tlsConsumeOutput(handle, 1) ==
        NET_INTERFACE_TLS_ARGUMENT, "output over-consumption accepted");
    NetInterfaceTlsTestPair::require(NetInterface::tlsProgress(handle, 0xfffffff0U, 1) ==
        NET_INTERFACE_TLS_HANDSHAKE && NetInterface::tlsProgress(handle, 4, 2) ==
        NET_INTERFACE_TLS_HANDSHAKE, "64-bit clock word rollover failed");
    NetInterface::tlsDestroy(handle);
    handle = nullptr;
    options.entropy = &NetInterfaceTlsTestPair::failedRandom;
    NetInterfaceTlsTestPair::require(NetInterface::tlsCreate(&options, &handle, &detail) ==
        NET_INTERFACE_TLS_PROTOCOL && handle == nullptr && detail != 0,
        "failed entropy source silently downgraded");
    NetInterfaceTls failed(options);
    NetInterfaceTlsTestPair::require(failed.status() == NET_INTERFACE_TLS_PROTOCOL &&
        failed.outputState() == NET_INTERFACE_TLS_PROTOCOL, "TLS output query lost persistent initialization error");
    options.entropy = nullptr;
    NetInterfaceTlsTestPair::require(NetInterface::tlsCreate(&options, &handle, &detail) ==
        NET_INTERFACE_TLS_ARGUMENT && handle == nullptr, "missing entropy accepted");
    options = NetInterfaceTlsTestPair::options(false, "");
    NetInterfaceTlsTestPair::require(NetInterface::tlsCreate(&options, &handle, &detail) ==
        NET_INTERFACE_TLS_ARGUMENT && handle == nullptr, "missing hostname accepted");
    NetInterface::tlsDestroy(nullptr);
}

void NetInterfaceTlsTestCases::boundedTransfer()
{
    NetInterfaceTlsTestPair pair("localhost");
    NetInterfaceTlsTestPair::require(pair.handshake(333), "bulk handshake failed");
    std::vector<uint8_t> expected(1024 * 1024);
    for (size_t index = 0; index < expected.size(); ++index)
    {
        expected[index] = static_cast<uint8_t>((index * 37 + index / 253) & 255);
    }
    std::vector<uint8_t> actual;
    size_t sent = 0;
    bool sawBackpressure = false;
    uint8_t output[4093];
    for (uint32_t step = 0; step < 30000 && actual.size() < expected.size(); ++step)
    {
        netTestCheckpoint();
        if (sent < expected.size())
        {
            uint32_t accepted = 0;
            int32_t status = pair.m_client->write(expected.data() + sent,
                static_cast<uint32_t>(expected.size() - sent), accepted);
            NetInterfaceTlsTestPair::require(status == NET_INTERFACE_TLS_OK ||
                status == NET_INTERFACE_TLS_WANT_IO, "bulk write failed");
            sawBackpressure = sawBackpressure || status == NET_INTERFACE_TLS_WANT_IO;
            sent += accepted;
        }
        pair.m_client->progress(2000 + step);
        // 首轮故意暂不转发，验证固定输出队列会阻止无限接受明文
        if (step > 8)
        {
            NetInterfaceTlsTestPair::transfer(*pair.m_client, *pair.m_server, 977);
            uint32_t received = 0;
            int32_t status = pair.m_server->read(output, sizeof(output), received);
            NetInterfaceTlsTestPair::require(status == NET_INTERFACE_TLS_OK ||
                status == NET_INTERFACE_TLS_WANT_IO, "bulk decrypt failed");
            actual.insert(actual.end(), output, output + received);
        }
    }
    NetInterfaceTlsTestPair::require(sawBackpressure, "bounded output did not apply backpressure");
    NetInterfaceTlsTestPair::require(sent == expected.size() && actual == expected,
        "fragmented bulk stream differs or did not finish");
}

void NetInterfaceTlsTestCases::entropyExceptionBoundary()
{
    const uint8_t authority[] = {1};
    NetInterfaceTlsOptions options = {};
    options.authority = authority;
    options.authoritySize = sizeof(authority);
    options.hostname = "example.test";
    options.handshakeTimeoutMs = 1000;
    // 回调先写部分随机数再失败，验证任何失败路径都不会继续使用这些字节。
    // context借用uint32_t模式：0返回失败，1抛异常；output为size字节可写内存。
    options.entropy = [](void* context, uint8_t* output, uint32_t size) -> int32_t
    {
        std::memset(output, 0xa7, size / 2);
        if (*static_cast<uint32_t*>(context) != 0)
        {
            throw 42;
        }
        return -1;
    };
    for (uint32_t mode = 0; mode < 2; ++mode)
    {
        options.entropyContext = &mode;
        for (uint32_t repeat = 0; repeat < 64; ++repeat)
        {
            uint8_t bytes[48];
            std::memset(bytes, 0xc3, sizeof(bytes));
            NetInterfaceTlsTestPair::require(NetInterface::entropyBytes(options.entropy, &mode, bytes, sizeof(bytes)) == NI_INVALID_STATE,
                "entropy callback failure did not fail closed");
            for (size_t index = 0; index < sizeof(bytes); ++index)
            {
                NetInterfaceTlsTestPair::require(bytes[index] == 0, "failed entropy retained partial output");
            }
            NetInterfaceTls tls(options);
            const int32_t failed = tls.status();
            NetInterfaceTlsTestPair::require(failed == NET_INTERFACE_TLS_PROTOCOL && tls.detail() != 0,
                "TLS constructor leaked entropy exception or lost failure");
            uint32_t consumed = 77;
            NetInterfaceTlsTestPair::require(tls.feed(bytes, sizeof(bytes), consumed) == failed && consumed == 0,
                "failed TLS accepted input");
            uint32_t accepted = 77;
            NetInterfaceTlsTestPair::require(tls.write(bytes, sizeof(bytes), accepted) == failed && accepted == 0,
                "failed TLS accepted plaintext");
            uint32_t received = 77;
            NetInterfaceTlsTestPair::require(tls.read(bytes, sizeof(bytes), received) == failed && received == 0,
                "failed TLS delivered plaintext");
            NetInterfaceTlsTestPair::require(tls.progress(1) == failed && tls.outputState() == failed && tls.finishInput() == failed && tls.close() == failed,
                "failed TLS state was revived");
            const uint8_t* output = reinterpret_cast<const uint8_t*>(1);
            uint32_t size = 77;
            NetInterfaceTlsTestPair::require(tls.peekOutput(output, size) == failed && output == nullptr && size == 0,
                "failed initialization generated output");
            NetInterfaceTlsHandle handle = reinterpret_cast<NetInterfaceTlsHandle>(1);
            int32_t detail = 77;
            NetInterfaceTlsTestPair::require(NetInterface::tlsCreate(&options, &handle, &detail) == failed && handle == nullptr && detail != 0,
                "TLS C factory leaked exception or partial handle");
            NetInterfaceConnection connection(options);
            NetInterfaceTlsTestPair::require(connection.status() == failed && connection.detail() != 0,
                "connection constructor lost entropy failure");
            NetInterfaceConnectionHandle connectionHandle = reinterpret_cast<NetInterfaceConnectionHandle>(1);
            detail = 77;
            NetInterfaceTlsTestPair::require(NetInterface::connectionCreate(&options, &connectionHandle, &detail) == failed && connectionHandle == nullptr && detail != 0,
                "connection C factory lost entropy failure");
        }
    }
    uint8_t bytes[8];
    std::memset(bytes, 0xa5, sizeof(bytes));
    NetInterfaceTlsTestPair::require(NetInterface::entropyBytes(nullptr, nullptr, bytes, sizeof(bytes)) == NI_INVALID_STATE,
        "missing entropy source accepted");
    for (size_t index = 0; index < sizeof(bytes); ++index)
    {
        NetInterfaceTlsTestPair::require(bytes[index] == 0, "missing entropy source retained output");
    }
    NetInterfaceTlsTestPair::require(NetInterface::entropyBytes(nullptr, nullptr, nullptr, 0) == NI_OK &&
        NetInterface::entropyBytes(nullptr, nullptr, nullptr, 1) == NI_INVALID_ARGUMENT &&
        NetInterface::entropyBytes(nullptr, nullptr, bytes, 8193) == NI_INVALID_ARGUMENT,
        "entropy argument boundaries changed");
}

void NetInterfaceTlsTestCases::appendRecords()
{
    const auto Check = &NetInterfaceTlsTestPair::require;
    NetInterfaceTlsTestPair pair("localhost");
    Check(pair.handshake(32768), "append handshake failed");
    uint8_t body[6] = { '1', '2', '3', '4', '5', '6' };
    uint32_t accepted = 77;
    Check(pair.m_client->append(nullptr, 1, accepted) == NET_INTERFACE_TLS_ARGUMENT && accepted == 0,
        "append accepted null nonempty input");
    Check(pair.m_client->append(nullptr, 0, accepted) == NET_INTERFACE_TLS_OK && accepted == 0,
        "empty append failed");
    for (uint32_t index = 0; index < 1024; ++index)
    {
        Check(pair.m_client->append(body, sizeof(body), accepted) == NET_INTERFACE_TLS_OK && accepted == sizeof(body),
            "append failed before record capacity");
    }
    Check(pair.m_client->write(body, sizeof(body), accepted) == NET_INTERFACE_TLS_WANT_IO && accepted == 0,
        "write overwrote unsubmitted append data");
    Check(pair.m_client->progress(2000) == NET_INTERFACE_TLS_READY, "aggregate progress failed");
    const uint8_t* wire = nullptr;
    uint32_t wireSize = 0;
    pair.m_client->peekOutput(wire, wireSize);
    Check(wireSize > 6144 && wireSize < 6244, "queued plaintext was not one compact TLS record");
    std::vector<uint8_t> output(16384);
    uint32_t total = 0;
    for (uint32_t step = 0; step < 20000 && total < 6144; ++step)
    {
        NetInterfaceTlsTestPair::transfer(*pair.m_client, *pair.m_server, 7);
        uint32_t received = 0;
        Check(pair.m_server->read(output.data(), static_cast<uint32_t>(output.size()), received) >= 0,
            "fragmented aggregate read failed");
        for (uint32_t index = 0; index < received; ++index)
        {
            Check(output[index] == body[(total + index) % 6], "aggregate content or ordering changed");
        }
        total += received;
    }
    Check(total == 6144, "aggregate plaintext incomplete");

    // 暂不消费密文，令未满的短记录触发WANT_WRITE，重试期间禁止追加明文。
    std::vector<uint8_t> expected;
    std::vector<uint8_t> block(16384, 0x5a);
    bool frozen = false;
    for (uint32_t step = 0; step < 4096; ++step)
    {
        const uint32_t size = step < 1 ? 16384 : 17;
        const int32_t result = pair.m_client->append(block.data(), size, accepted);
        Check(result >= 0, "append failed while saturating ciphertext");
        if (accepted == 0)
        {
            frozen = true;
            Check(pair.m_client->appendExact(body, 1) == NET_INTERFACE_TLS_WANT_IO,
                "exact append changed a frozen retry record");
            Check(pair.m_client->append(body, 1, accepted) == NET_INTERFACE_TLS_WANT_IO && accepted == 0,
                "frozen TLS retry data changed");
            break;
        }
        expected.insert(expected.end(), block.begin(), block.begin() + accepted);
        Check(pair.m_client->progress(2001) == NET_INTERFACE_TLS_READY, "backpressured progress failed");
    }
    Check(frozen, "ciphertext backpressure was not reached");
    total = 0;
    for (uint32_t step = 0; step < 100000 && (total < expected.size() || pair.m_client->outputState() != NET_INTERFACE_TLS_OK); ++step)
    {
        pair.m_client->progress(2002);
        NetInterfaceTlsTestPair::transfer(*pair.m_client, *pair.m_server, 127);
        uint32_t received = 0;
        Check(pair.m_server->read(output.data(), static_cast<uint32_t>(output.size()), received) >= 0,
            "frozen retry failed authentication");
        Check(received <= expected.size() - total && std::memcmp(output.data(), expected.data() + total, received) == 0,
            "frozen retry lost, duplicated or changed plaintext");
        total += received;
    }
    Check(total == expected.size(), "frozen retry failed to drain");
    Check(pair.m_client->append(block.data(), 16380, accepted) == NET_INTERFACE_TLS_OK && accepted == 16380,
        "append did not recover after backpressure");
    Check(pair.m_client->append(body, 6, accepted) == NET_INTERFACE_TLS_OK && accepted == 4,
        "append did not return exact record-tail prefix");
    Check(pair.m_client->append(body, 1, accepted) == NET_INTERFACE_TLS_WANT_IO && accepted == 0,
        "full plaintext record was overfilled");
    pair.m_client->progress(2003);
    NetInterfaceTlsTestPair::transfer(*pair.m_client, *pair.m_server, 32768);
    uint32_t received = 0;
    Check(pair.m_server->read(output.data(), static_cast<uint32_t>(output.size()), received) == NET_INTERFACE_TLS_OK &&
        received == 16384 && std::memcmp(output.data(), block.data(), 16380) == 0 &&
        std::memcmp(output.data() + 16380, body, 4) == 0, "partial append tail corrupted");
    Check(pair.m_client->appendExact(nullptr, 1) == NET_INTERFACE_TLS_ARGUMENT &&
        pair.m_client->appendExact(nullptr, 0) == NET_INTERFACE_TLS_OK &&
        pair.m_client->appendExact(block.data(), 16385) == NET_INTERFACE_TLS_WANT_IO &&
        pair.m_client->appendExact(body, 3, nullptr, 1) == NET_INTERFACE_TLS_ARGUMENT &&
        pair.m_client->appendExact(nullptr, 1, body, 3) == NET_INTERFACE_TLS_ARGUMENT &&
        pair.m_client->appendExact(block.data(), 16000, block.data(), 1000) == NET_INTERFACE_TLS_WANT_IO,
        "exact append argument or oversized atomicity");
    // 每轮及时排空本例32KiB密文池；跨轮复用同时覆盖环形缓冲回绕。
    uint32_t exactTotal = 0;
    for (uint32_t round = 0; round < 3; ++round)
    {
        Check(pair.m_client->appendExact(block.data(), 16380) == NET_INTERFACE_TLS_OK &&
            pair.m_client->appendExact(body, round, body + round, 6 - round) == NET_INTERFACE_TLS_OK,
            "exact append did not roll a full record without a partial new input");
        const uint32_t expectedTotal = (round + 1) * 16386;
        for (uint32_t step = 0; step < 10000 && exactTotal < expectedTotal; ++step)
        {
            pair.m_client->progress(2003);
            NetInterfaceTlsTestPair::transfer(*pair.m_client, *pair.m_server, 127);
            uint32_t exactReceived = 0;
            Check(pair.m_server->read(output.data(), static_cast<uint32_t>(output.size()), exactReceived) >= 0,
                "exact append rollover authentication");
            for (uint32_t index = 0; index < exactReceived; ++index)
            {
                const uint32_t offset = (exactTotal + index) % 16386;
                Check(output[index] == (offset < 16380 ? 0x5a : body[offset - 16380]),
                    "exact append rollover content/order");
            }
            exactTotal += exactReceived;
        }
        Check(exactTotal == expectedTotal, "exact append rollover missing bytes");
    }
    // 两段短正文覆盖所有0～32字节长度和非对齐地址；比较真实TLS解密结果而非内部存储。
    std::vector<uint8_t> fragments;
    uint8_t bytes[34];
    for (uint32_t index = 0; index < sizeof(bytes); ++index)
    {
        bytes[index] = static_cast<uint8_t>(index * 7 + 3);
    }
    for (uint32_t size = 0; size <= 32; ++size)
    {
        Check(pair.m_client->appendExact(bytes + 1, size, bytes + 2, 32 - size) == NET_INTERFACE_TLS_OK,
            "exact append short unaligned fragments");
        fragments.insert(fragments.end(), bytes + 1, bytes + 1 + size);
        fragments.insert(fragments.end(), bytes + 2, bytes + 34 - size);
    }
    pair.m_client->progress(2004);
    NetInterfaceTlsTestPair::transfer(*pair.m_client, *pair.m_server, 32768);
    Check(pair.m_server->read(output.data(), static_cast<uint32_t>(output.size()), received) == NET_INTERFACE_TLS_OK &&
        received == fragments.size() && std::memcmp(output.data(), fragments.data(), fragments.size()) == 0,
        "exact append small fragment content/size");
    Check(pair.m_client->write(body, 6, accepted) == NET_INTERFACE_TLS_OK && accepted == 6,
        "legacy write failed after append");
    pair.m_client->close();
    NetInterfaceTlsTestPair::transfer(*pair.m_client, *pair.m_server, 32768);
    Check(pair.m_server->read(output.data(), static_cast<uint32_t>(output.size()), received) == NET_INTERFACE_TLS_OK &&
        received == 6 && std::memcmp(output.data(), body, 6) == 0, "close lost pending plaintext");
    Check(pair.m_client->append(body, 6, accepted) == NET_INTERFACE_TLS_STATE && accepted == 0,
        "append succeeded after close");
}