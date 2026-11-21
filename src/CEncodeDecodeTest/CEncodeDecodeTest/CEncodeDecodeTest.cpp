#include "CEncodeDecodeTest.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include "CEncodeDecodeCertificateCases.h"
#include "CEncodeDecodeHashCases.h"
#include "CEncodeDecodeTlsTestData.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

/** 检查当前步骤，失败时抛出固定提示。
@param [in] condition 预期条件
@param [in] step 步骤标识
*/
static void Check(bool condition, const char* step)
{
    if (!condition)
    {
        throw std::runtime_error(step);
    }
}

int32_t cEncodeTestEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    static std::mt19937 s_generator(0x8fa29d19U);
    for (uint32_t i = 0; i < size; ++i)
    {
        output[i] = static_cast<uint8_t>(s_generator());
    }
    return 0;
}

// Test vector 1
// plain: 01 23 45 67 89 ab cd ef fe dc ba 98 76 54 32 10
// key:   01 23 45 67 89 ab cd ef fe dc ba 98 76 54 32 10
// 	   round key and temp computing result:
// 	   rk[ 0] = f12186f9 X[ 0] = 27fad345
// 		   rk[ 1] = 41662b61 X[ 1] = a18b4cb2
// 		   rk[ 2] = 5a6ab19a X[ 2] = 11c1e22a
// 		   rk[ 3] = 7ba92077 X[ 3] = cc13e2ee
// 		   rk[ 4] = 367360f4 X[ 4] = f87c5bd5
// 		   rk[ 5] = 776a0c61 X[ 5] = 33220757
// 		   rk[ 6] = b6bb89b3 X[ 6] = 77f4c297
// 		   rk[ 7] = 24763151 X[ 7] = 7a96f2eb
// 		   rk[ 8] = a520307c X[ 8] = 27dac07f
// 		   rk[ 9] = b7584dbd X[ 9] = 42dd0f19
// 		   rk[10] = c30753ed X[10] = b8a5da02
// 		   rk[11] = 7ee55b57 X[11] = 907127fa
// 		   rk[12] = 6988608c X[12] = 8b952b83
// 		   rk[13] = 30d895b7 X[13] = d42b7c59
// 		   rk[14] = 44ba14af X[14] = 2ffc5831
// 		   rk[15] = 104495a1 X[15] = f69e6888
// 		   rk[16] = d120b428 X[16] = af2432c4
// 		   rk[17] = 73b55fa3 X[17] = ed1ec85e
// 		   rk[18] = cc874966 X[18] = 55a3ba22
// 		   rk[19] = 92244439 X[19] = 124b18aa
// 		   rk[20] = e89e641f X[20] = 6ae7725f
// 		   rk[21] = 98ca015a X[21] = f4cba1f9
// 		   rk[22] = c7159060 X[22] = 1dcdfa10
// 		   rk[23] = 99e1fd2e X[23] = 2ff60603
// 		   rk[24] = b79bd80c X[24] = eff24fdc
// 		   rk[25] = 1d2115b0 X[25] = 6fe46b75
// 		   rk[26] = 0e228aeb X[26] = 893450ad
// 		   rk[27] = f1780c81 X[27] = 7b938f4c
// 		   rk[28] = 428d3654 X[28] = 536e4246
// 		   rk[29] = 62293496 X[29] = 86b3e94f
// 		   rk[30] = 01cf72e5 X[30] = d206965e
// 		   rk[31] = 9124a012 X[31] = 681edf34
// cypher: 68 1e df 34 d2 06 96 5e 86 b3 e9 4f 53 6e 42 46
// 		
// test vector 2
// the same key and plain 1000000 times coumpting 
// plain:  01 23 45 67 89 ab cd ef fe dc ba 98 76 54 32 10
// key:    01 23 45 67 89 ab cd ef fe dc ba 98 76 54 32 10
// cypher: 59 52 98 c7 c6 fd 27 1f 04 02 f8 04 c3 3d 3f 66

/** 记录 RSA 取熵请求，并提供成功、部分填充后报错或抛异常的测试回调
@param [in,out] context 本次用例的熵源模式和计数器
@param [out] output 仅在回调期间使用的缓冲
@param [in] size 请求的字节数
@return 完整填充返回 0，拒绝模式返回非零，异常模式抛出测试异常
*/
static int32_t RsaTestEntropy(void* context, uint8_t* output, uint32_t size)
{
    CEncodeDecodeTestRsaEntropy& state = *static_cast<CEncodeDecodeTestRsaEntropy*>(context);
    ++state.m_calls;
    if (state.m_mode == CEncodeDecodeTestRsaEntropyMode::Fill)
    {
        return cEncodeTestEntropy(nullptr, output, size);
    }
    if (size != 0)
    {
        output[0] = 0xa5;
    }
    if (state.m_mode == CEncodeDecodeTestRsaEntropyMode::Throw)
    {
        throw std::runtime_error("controlled RSA entropy exception");
    }
    return -1;
}

/** 检查 RSA 的显式熵源、各端默认行为、密钥输出清理和 OAEP 长度边界
*/
static void CheckRsa()
{
    CEncodeDecodeTestRsaEntropy source = { CEncodeDecodeTestRsaEntropyMode::Fill, 0 };
    std::string publicKey, privateKey;
    Check(CEncodeDecode::RSAKey(publicKey, privateKey, RsaTestEntropy, &source) &&
        source.m_calls > 0, "RSA generation with supplied entropy");
    const std::string longest(190, 'r');
    source.m_calls = 0;
    const std::string encrypted = CEncodeDecode::RSAEncode(publicKey, longest.c_str(), RsaTestEntropy, &source);
    Check(encrypted.size() == 256 && source.m_calls > 0, "RSA OAEP encryption uses supplied entropy");
    source.m_calls = 0;
    Check(CEncodeDecode::RSADecode(privateKey, encrypted, RsaTestEntropy, &source) == longest &&
        source.m_calls > 0, "RSA OAEP 190 byte round trip uses supplied entropy");
    Check(CEncodeDecode::RSAEncode(publicKey, (longest + "x").c_str(), RsaTestEntropy, &source).empty(),
        "RSA OAEP maximum");

    const CEncodeDecodeTestRsaEntropyMode failures[] = { CEncodeDecodeTestRsaEntropyMode::Reject, CEncodeDecodeTestRsaEntropyMode::Throw };
    for (size_t i = 0; i < sizeof(failures) / sizeof(failures[0]); ++i)
    {
        source.m_mode = failures[i];
        uint8_t guarded[] = { 0x11, 0x5a, 0x5a, 0x5a, 0x22 };
        const uint8_t cleared[] = { 0x11, 0, 0, 0, 0x22 };
        source.m_calls = 0;
        Check(!CEncodeDecode::entropyBytes(RsaTestEntropy, &source, guarded + 1, 3) && source.m_calls == 1 &&
            std::memcmp(guarded, cleared, sizeof(guarded)) == 0, "failed entropy wipes requested bytes only");
        std::string failedPublic("previous public"), failedPrivate("previous private");
        source.m_calls = 0;
        Check(!CEncodeDecode::RSAKey(failedPublic, failedPrivate, RsaTestEntropy, &source) &&
            failedPublic.empty() && failedPrivate.empty() && source.m_calls > 0, "RSA rejected entropy clears key outputs");
        source.m_calls = 0;
        Check(CEncodeDecode::RSAEncode(publicKey, longest.c_str(), RsaTestEntropy, &source).empty() && source.m_calls > 0,
            "RSA rejected entropy yields no ciphertext");
        source.m_calls = 0;
        Check(CEncodeDecode::RSADecode(privateKey, encrypted, RsaTestEntropy, &source).empty() && source.m_calls > 0,
            "RSA rejected entropy yields no plaintext");
    }
#if defined(__EMSCRIPTEN__)
    Check(CEncodeDecode::RSAEncode(publicKey, longest.c_str()).empty() &&
        CEncodeDecode::RSADecode(privateKey, encrypted).empty(), "Wasm RSA requires host entropy for encrypt and decrypt");
    Check(!CEncodeDecode::RSAKey(publicKey, privateKey) && publicKey.empty() && privateKey.empty(),
        "Wasm RSA generation requires host entropy and clears key outputs");
#else
    Check(CEncodeDecode::RSAKey(publicKey, privateKey), "native RSA generation with default entropy");
    const std::string nativeCipher = CEncodeDecode::RSAEncode(publicKey, longest.c_str());
    Check(nativeCipher.size() == 256 && CEncodeDecode::RSADecode(privateKey, nativeCipher) == longest,
        "native RSA OAEP round trip with default entropy");
    CEncodeDecode::wipe(reinterpret_cast<uint8_t*>(&privateKey[0]), static_cast<uint32_t>(privateKey.size()));
#endif
}

/** 验证旧格式固定向量、往返及长度边界
*/
static void CheckLegacy()
{
    Check(CEncodeDecode::MD5Encode("abc") == "900150983cd24fb0d6963f7d28e17f72", "MD5 known answer");
    const std::string key("0123456789ABCDEF");
    const std::string message("Plain text crossing a block");
    const std::string aes = CEncodeDecode::AESEncode(key, message);
    Check(aes.size() == 64 && CEncodeDecode::AESDecode(key, aes) == message, "AES two blocks");
    Check(CEncodeDecode::AESDecode(key, "xyz").empty(), "AES invalid hex");

    const std::string block("\x01\x23\x45\x67\x89\xab\xcd\xef\xfe\xdc\xba\x98\x76\x54\x32\x10", 16);
    const std::string expected("\x68\x1e\xdf\x34\xd2\x06\x96\x5e\x86\xb3\xe9\x4f\x53\x6e\x42\x46", 16);
    Check(CEncodeDecode::SM4Encode(block, block) == expected &&
        CEncodeDecode::SM4Decode(block, expected) == block, "SM4 known answer");
    Check(CEncodeDecode::SM4Encode(block, key.substr(1)).empty(), "SM4 incomplete block");

    std::string repeated = block;
    for (uint32_t round = 0; round < 1000000; ++round)
    {
        repeated = CEncodeDecode::SM4Encode(block, repeated);
    }
    Check(repeated == std::string("\x59\x52\x98\xc7\xc6\xfd\x27\x1f\x04\x02\xf8\x04\xc3\x3d\x3f\x66", 16),
        "SM4 million-iteration known answer");

    std::string privateKey;
    std::string x, y, c1("previous"), c2("previous"), c3("previous");
    if (CEncodeDecode::SM2Key(x, y, privateKey, false))
    {
        Check(CEncodeDecode::SM2Encode(x, y, message, c1, c2, c3) &&
            CEncodeDecode::SM2Decode(privateKey, c1, c2, c3) == message, "SM2 enabled round trip");
    }
    else
    {
        Check(x.empty() && y.empty() && privateKey.empty() &&
            !CEncodeDecode::SM2Encode(x, y, message, c1, c2, c3) && c1.empty() && c2.empty() && c3.empty() &&
            CEncodeDecode::SM2Decode(privateKey, c1, c2, c3).empty(), "SM2 unavailable fails closed");
    }
    CheckRsa();
}

typedef std::unique_ptr<CEncodeDecodeTls, void (*)(CEncodeDecodeTls*)> TlsHandle;

/** 以内置公开测试证书创建独立 TLS 端点。
@param [in] server 是否创建服务端
@param [in] hostname 客户端预期域名；服务端可空
@param [in] timeout 握手毫秒数，0 测试内部默认值
@param [in] limits 可选的双向密文队列配置
@return 独占端点
*/
static TlsHandle CreateEndpoint(bool server, const char* hostname, uint32_t timeout = 0,
    const CEncodeDecodeTlsQueueLimits* limits = nullptr)
{
    static const uint8_t authority[] = TEST_CA_CRT_RSA_SHA256_PEM;
    static const uint8_t certificate[] = TEST_SRV_CRT_RSA_SHA256_PEM;
    static const uint8_t key[] = TEST_SRV_KEY_RSA_PEM;
    CEncodeDecodeTlsOptions options = {};
    options.server = server ? 1 : 0;
    options.hostname = hostname;
    options.entropy = cEncodeTestEntropy;
    Check(options.setHandshakeTimeout(timeout) && !options.setHandshakeTimeout(99) &&
        !options.setHandshakeTimeout(60001) && options.handshakeTimeoutMs == timeout, "TLS deadline setter");
    if (server)
    {
        options.certificate = certificate;
        options.certificateSize = sizeof(certificate);
        options.privateKey = key;
        options.privateKeySize = sizeof(key);
    }
    else
    {
        options.authority = authority;
        options.authoritySize = sizeof(authority);
    }
    TlsHandle endpoint(limits ? CEncodeDecode::createTls(options, *limits) : CEncodeDecode::createTls(options), CEncodeDecode::destroyTls);
    Check(endpoint && endpoint->status() == CENCODE_TLS_HANDSHAKE, "TLS endpoint initialization");
    return endpoint;
}

/** 转移至多 17 字节的密文，并仅消费对端已接纳的部分。
@param [in,out] from 发送方
@param [in,out] to 接收方
*/
static void Transfer(CEncodeDecodeTls& from, CEncodeDecodeTls& to)
{
    const uint8_t* ciphertext = nullptr;
    uint32_t size = 0;
    Check(from.peekOutput(ciphertext, size) == CENCODE_TLS_OK, "TLS peek");
    if (size == 0)
    {
        return;
    }
    uint32_t accepted = 0;
    const uint32_t part = (std::min)(size, 17U);
    Check(to.feed(ciphertext, part, accepted) >= 0 && accepted <= part, "TLS fragment feed");
    Check(from.consumeOutput(accepted) == CENCODE_TLS_OK, "TLS consume");
}

/** 在两个独立端点间按 17 字节分段推进握手。
@param [in,out] client 客户端
@param [in,out] server 服务端
*/
static void Handshake(TlsHandle& client, TlsHandle& server)
{
    for (uint32_t step = 0; step < 16000; ++step)
    {
        client->progress(1000 + step / 64);
        server->progress(1000 + step / 64);
        if (client->status() < 0 || server->status() < 0)
        {
            break;
        }
        Transfer(*client, *server);
        Transfer(*server, *client);
        if (client->status() == CENCODE_TLS_READY && server->status() == CENCODE_TLS_READY)
        {
            break;
        }
    }
}

/** 使用正确或不匹配的域名驱动握手。
@param [in] validHost 是否应建立连接并验证数据及关闭通知
*/
static void CheckTls(bool validHost)
{
    TlsHandle client = CreateEndpoint(false, validHost ? "localhost" : "unrelated.invalid");
    TlsHandle server = CreateEndpoint(true, nullptr);
    Handshake(client, server);
    if (!validHost)
    {
        Check(client->status() == CENCODE_TLS_PROTOCOL && client->detail() != 0, "TLS hostname validation");
        return;
    }
    Check(client->status() == CENCODE_TLS_READY && server->status() == CENCODE_TLS_READY, "TLS handshake");
    const uint8_t message[] = {0x81, 0, 0x5e, 0xff};
    Check(client->appendExact(message, 2, message + 2, 2) == CENCODE_TLS_OK, "TLS two-part plaintext");
    uint8_t result[sizeof(message)] = {};
    uint32_t total = 0;
    for (unsigned step = 0; step < 128 && total < sizeof(result); ++step)
    {
        Check(client->progress(1400) >= 0, "TLS write progress");
        Transfer(*client, *server);
        uint32_t received = 0;
        Check(server->read(result + total, 1, received) >= 0 && received <= 1, "TLS read progress");
        total += received;
    }
    Check(total == sizeof(message) && std::memcmp(result, message, total) == 0, "TLS payload");
    Check(client->close() == CENCODE_TLS_OK, "TLS orderly closure");
    for (unsigned step = 0; step < 128 && server->status() != CENCODE_TLS_CLOSED; ++step)
    {
        Transfer(*client, *server);
        uint32_t received = 0;
        Check(server->read(result, 1, received) >= 0 && received == 0, "TLS close notification");
    }
    Check(server->status() == CENCODE_TLS_CLOSED, "TLS authenticated end");
}

/** 在固定输入上采样密码接口，每次调用校验成功并将实际输出计入校验和 */
static void MeasureCrypto()
{
    const std::string shortInput(32, 'M');
    const std::string block(4096, 'H');
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(block.data());
    uint8_t digest[32] = {};
    LibraryBenchmark::run("CEncodeDecode.MD5.32B", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            const std::string result = CEncodeDecode::MD5Encode(shortInput);
            Check(result.size() == 32, "MD5 benchmark digest");
            sum += result[i % result.size()];
        }
        return sum;
    });
    LibraryBenchmark::run("CEncodeDecode.SHA256.4096B", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            Check(CEncodeDecode::sha256(bytes, 4096, digest, 32) == CENCODE_OK, "SHA256 benchmark");
            sum += digest[i % 32];
        }
        return sum;
    });
    LibraryBenchmark::run("CEncodeDecode.HMACSHA256.4096B", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            Check(CEncodeDecode::hmacSha256(bytes, 32, bytes, 4096, digest, 32) == CENCODE_OK, "HMAC benchmark");
            sum += digest[i % 32];
        }
        return sum;
    });
}

/** 核对未设置及显式设置的握手截止时间。 */
static int CheckTlsDeadline()
{
    TlsHandle client = CreateEndpoint(false, "localhost");
    Check(client->progress(1000) == CENCODE_TLS_HANDSHAKE &&
        client->progress(11001) == CENCODE_TLS_TIMEOUT, "TLS handshake deadline");
    TlsHandle shortDeadline = CreateEndpoint(false, "localhost", 100);
    Check(shortDeadline->progress(5000) == CENCODE_TLS_HANDSHAKE &&
        shortDeadline->progress(5099) == CENCODE_TLS_HANDSHAKE &&
        shortDeadline->progress(5100) == CENCODE_TLS_TIMEOUT, "TLS configured handshake deadline");
    return 0;
}

/** 从真实 TLS 入口核对两个方向的默认、固定和按需增长限额，以及跨块的字节顺序与借用期。 */
static int CheckTlsQueueLimits()
{
    CEncodeDecodeTlsQueueLimits limits;
    Check(limits.inputBytes() == 32768 && limits.outputBytes() == 32768, "TLS default queue settings");
    std::vector<uint8_t> sent(4 * 16384 + 13);
    for (size_t i = 0; i < sent.size(); ++i) { sent[i] = static_cast<uint8_t>(i * 37); }
    uint32_t count = 0;
    TlsHandle bounded = CreateEndpoint(false, "localhost");
    Check(bounded->feed(sent.data(), static_cast<uint32_t>(sent.size()), count) == CENCODE_TLS_WANT_IO && count == 32768,
        "TLS default input backpressure");
    Check(limits.set(41000, 7) && !limits.set(-2, 10) && !limits.set(1, 0) &&
        limits.inputBytes() == 41000 && limits.outputBytes() == 7, "TLS queue setters preserve both previous values");
    bounded = CreateEndpoint(false, "localhost", 0, &limits);
    const uint8_t* first = nullptr;
    uint32_t length = 0;
    Check(bounded->progress(1000) == CENCODE_TLS_HANDSHAKE && bounded->peekOutput(first, length) == CENCODE_TLS_OK &&
        first && length == 7, "TLS configured output backpressure");
    Check(bounded->feed(sent.data(), static_cast<uint32_t>(sent.size()), count) == CENCODE_TLS_WANT_IO && count == 41000 &&
        bounded->feed(sent.data(), 1, count) == CENCODE_TLS_WANT_IO && count == 0, "TLS configured input boundary");
    Check(limits.set(-1, -1), "TLS unbounded queues");
    TlsHandle defaultSender = CreateEndpoint(false, "localhost");
    TlsHandle defaultPeer = CreateEndpoint(true, nullptr);
    Handshake(defaultSender, defaultPeer);
    for (int record = 0; record < 2; ++record)
    {
        Check(defaultSender->write(sent.data(), 16384, count) == CENCODE_TLS_OK && count == 16384 &&
            defaultSender->progress(1400) == CENCODE_TLS_READY, "TLS default output record");
    }
    size_t defaultPending = 0;
    while (defaultSender->peekOutput(first, length) == CENCODE_TLS_OK && length != 0)
    {
        defaultPending += length;
        Check(defaultSender->consumeOutput(length) == CENCODE_TLS_OK, "TLS default output consume");
    }
    Check(defaultPending == 32768, "TLS default output backpressure");
    TlsHandle client = CreateEndpoint(false, "localhost", 0, &limits);
    TlsHandle server = CreateEndpoint(true, nullptr, 0, &limits);
    Handshake(client, server);
    Check(client->status() == CENCODE_TLS_READY && server->status() == CENCODE_TLS_READY, "TLS queue handshake");
    std::vector<uint8_t> prefix;
    size_t offset = 0;
    while (offset < sent.size())
    {
        Check(client->write(sent.data() + offset, static_cast<uint32_t>(sent.size() - offset), count) == CENCODE_TLS_OK &&
            count != 0 && client->progress(1400) == CENCODE_TLS_READY, "TLS queue record write");
        offset += count;
        if (prefix.empty())
        {
            Check(client->peekOutput(first, length) == CENCODE_TLS_OK && length != 0, "TLS borrow first output segment");
            prefix.assign(first, first + length);
        }
    }
    Check(std::memcmp(first, prefix.data(), prefix.size()) == 0, "TLS borrowed prefix stable across queue growth");
    size_t cipherSize = 0;
    while (client->peekOutput(first, length) == CENCODE_TLS_OK && length != 0)
    {
        Check(server->feed(first, length, count) == CENCODE_TLS_OK && count == length &&
            client->consumeOutput(length) == CENCODE_TLS_OK, "TLS queued ciphertext transfer");
        cipherSize += length;
    }
    Check(cipherSize > sent.size(), "TLS multiple queued records");
    std::vector<uint8_t> received(sent.size());
    offset = 0;
    while (offset < received.size())
    {
        Check(server->read(received.data() + offset, static_cast<uint32_t>(received.size() - offset), count) == CENCODE_TLS_OK &&
            count != 0, "TLS queued record read");
        offset += count;
    }
    Check(received == sent, "TLS cross-block plaintext integrity");
    return 0;
}

int main(int argc, char** argv)
{
    try
    {
        const bool benchmark = argc == 2 && std::string(argv[1]) == "--benchmark";
        const LibraryTestCase cases[] = {
            {kCryptoHash, false, []() -> int { CEncodeDecodeHashCases::hashBoundary(); return 0; }},
            {kCryptoDerivation, false, []() -> int { CEncodeDecodeHashCases::derivationBoundary(); return 0; }},
            {kCryptoParts, false, []() -> int { CEncodeDecodeHashCases::partsBoundary(); return 0; }},
            {kCryptoCertificate, false, []() -> int { CEncodeDecodeCertificateCases::run(); return 0; }},
            {kCryptoLegacy, true, []() -> int { CheckLegacy(); return 0; }},
            {kCryptoTlsTrusted, false, []() -> int { CheckTls(true); return 0; }},
            {kCryptoTlsWrongHost, false, []() -> int { CheckTls(false); return 0; }},
            {kCryptoTlsDeadline, false, CheckTlsDeadline},
            {kCryptoTlsQueueLimits, false, CheckTlsQueueLimits}
        };
        const int result = LibraryCaseSelection::run(benchmark ? 1 : argc, argv, cases);
        if (result == 0 && benchmark) { MeasureCrypto(); }
        if (result == 0) { std::puts("CEncodeDecodeTest PASS"); }
        return result;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "CEncodeDecodeTest FAIL: %s\n", error.what());
        return 1;
    }
}