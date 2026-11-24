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
#ifdef USE_OPENSSL
#include <openssl/opensslconf.h>
#endif

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

/** 提供本测试进程串行使用的字节生成器
@return 在每组功能用例开始时重新设定种子的生成器
*/
static std::mt19937& TestEntropyGenerator()
{
    static std::mt19937 s_generator;
    return s_generator;
}

int32_t cEncodeTestEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    std::mt19937& generator = TestEntropyGenerator();
    for (uint32_t i = 0; i < size; ++i)
    {
        output[i] = static_cast<uint8_t>(generator());
    }
    return 0;
}

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

/** 验证公共熵源适配器的调用条件、长度上限及失败后的写入边界
@return 通过返回 0，失败抛出带步骤的测试异常
*/
static int CheckEntropyBoundaries()
{
    uint32_t calls = 0;
    // context 记录调用次数；将 size 字节全部填为可核对的标记，参数有效时返回 0。
    const CEncodeDecodeEntropyFunction fill = [](void* context, uint8_t* output, uint32_t size) -> int32_t
    {
        if (context == nullptr)
        {
            return -1;
        }
        ++*static_cast<uint32_t*>(context);
        if (output == nullptr)
        {
            return -1;
        }
        std::memset(output, 0xa6, size);
        return 0;
    };
    const std::vector<uint8_t> untouched(8195, 0x5a);
    std::vector<uint8_t> bytes = untouched;
    uint8_t* output = bytes.data() + 1;
    Check(CEncodeDecode::entropyBytes(nullptr, nullptr, nullptr, 0) &&
        CEncodeDecode::entropyBytes(fill, &calls, output, 0) && calls == 0 && bytes == untouched, "empty entropy request");
    Check(!CEncodeDecode::entropyBytes(fill, &calls, nullptr, 1) &&
        !CEncodeDecode::entropyBytes(fill, &calls, output, 8193) && calls == 0 && bytes == untouched, "invalid entropy request");
    Check(CEncodeDecode::entropyBytes(fill, &calls, output, 8192) && calls == 1 &&
        std::count(output, output + 8192, 0xa6) == 8192 && bytes.front() == 0x5a &&
        bytes[8193] == 0x5a && bytes.back() == 0x5a, "maximum entropy request");
    Check(!CEncodeDecode::entropyBytes(nullptr, nullptr, output, 8192) &&
        std::count(output, output + 8192, 0) == 8192 && bytes.front() == 0x5a &&
        bytes[8193] == 0x5a && bytes.back() == 0x5a, "missing entropy callback clears only the requested range");
    return 0;
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
    Check(CEncodeDecode::AESDecode(key, "xyz").empty(), "AES invalid length");
    std::string malformed = aes;
    malformed[malformed.size() - 1] = 'G';
    Check(CEncodeDecode::AESDecode(key, malformed).empty(), "AES invalid hex digit");
    // 两个固定答案分别用独立 AES-128-ECB 实现对补零后的 abc 分组生成。
    Check(CEncodeDecode::AESEncode("Key", "abc") == "F0D87F23F873148DC86D4803D7456AFF" &&
        CEncodeDecode::AESDecode("Key", "f0d87f23f873148dc86d4803d7456aff") == "abc", "AES short key zero padding");
    const std::string longKey = key + "-ignored";
    Check(CEncodeDecode::AESEncode(longKey, "abc") == "58E6250FB71A1DDC21A07145D13CFF91" &&
        CEncodeDecode::AESDecode(longKey, "58e6250fb71a1ddc21a07145d13cff91") == "abc", "AES long key prefix");
    // AES-128 的零密钥和零分组固定向量，同时覆盖空文本填充。
    Check(CEncodeDecode::AESEncode("", "") == "66E94BD4EF8A2C3B884CFA59CA342B2E", "AES zero block vector");

    const std::string block("\x01\x23\x45\x67\x89\xab\xcd\xef\xfe\xdc\xba\x98\x76\x54\x32\x10", 16);
    const std::string expected("\x68\x1e\xdf\x34\xd2\x06\x96\x5e\x86\xb3\xe9\x4f\x53\x6e\x42\x46", 16);
    Check(CEncodeDecode::SM4Encode(block, block) == expected &&
        CEncodeDecode::SM4Decode(block, expected) == block, "SM4 known answer");
    // 使用 OpenSSL 对相同密钥下的标准分组及全零分组独立计算固定密文。
    const std::string twoBlocks = block + std::string(16, '\0');
    const std::string twoCipherBlocks = expected + std::string("\x26\x77\xf4\x6b\x09\xc1\x22\xcc\x97\x55\x33\x10\x5b\xd4\xa2\x2a", 16);
    Check(CEncodeDecode::SM4Encode(block, twoBlocks) == twoCipherBlocks &&
        CEncodeDecode::SM4Decode(block, twoCipherBlocks) == twoBlocks, "SM4 distinct ECB blocks");
    const std::string shortBlock = key.substr(1);
    Check(CEncodeDecode::SM4Encode(block, shortBlock).empty() && CEncodeDecode::SM4Decode(block, shortBlock).empty() &&
        CEncodeDecode::SM4Encode(shortBlock, block).empty() && CEncodeDecode::SM4Decode(shortBlock, block).empty() &&
        CEncodeDecode::SM4Encode(block, "").empty() && CEncodeDecode::SM4Decode(block, "").empty(), "SM4 key and input lengths");

    std::string repeated = block;
    for (uint32_t round = 0; round < 1000000; ++round)
    {
        repeated = CEncodeDecode::SM4Encode(block, repeated);
    }
    Check(repeated == std::string("\x59\x52\x98\xc7\xc6\xfd\x27\x1f\x04\x02\xf8\x04\xc3\x3d\x3f\x66", 16),
        "SM4 million-iteration known answer");

    std::string privateKey("previous");
    std::string x("previous"), y("previous"), c1("previous"), c2("previous"), c3("previous");
#if defined(USE_OPENSSL) && !defined(OPENSSL_NO_SM2)
    Check(CEncodeDecode::SM2Key(x, y, privateKey, false) && x.size() == 32 && y.size() == 32 && privateKey.size() == 32,
        "SM2 enabled key generation");
    Check(CEncodeDecode::SM2Encode(x, y, message, c1, c2, c3) && c1.size() == 64 && c2.size() == message.size() &&
        c3.size() == 32 && CEncodeDecode::SM2Decode(privateKey, c1, c2, c3) == message, "SM2 enabled round trip");
    c3[0] ^= 1;
    Check(CEncodeDecode::SM2Decode(privateKey, c1, c2, c3).empty(), "SM2 integrity check");
    Check(CEncodeDecode::SM2Key(x, y, privateKey) && x.size() == 33 && x[0] == 4 && y.size() == 32 &&
        privateKey.size() == 32 && CEncodeDecode::SM2Encode(x, y, message, c1, c2, c3) && c1.size() == 65 &&
        c1[0] == 4 && CEncodeDecode::SM2Decode(privateKey, c1, c2, c3) == message, "SM2 prefixed key round trip");
#else
    Check(!CEncodeDecode::SM2Key(x, y, privateKey, false) && x.empty() && y.empty() && privateKey.empty() &&
        !CEncodeDecode::SM2Encode(x, y, message, c1, c2, c3) && c1.empty() && c2.empty() && c3.empty() &&
        CEncodeDecode::SM2Decode(privateKey, c1, c2, c3).empty(), "SM2 unavailable fails closed");
#endif
    Check(CEncodeDecode::getCert("") == nullptr, "empty certificate path");
    Check(CEncodeDecode::getPublicKey(nullptr).empty(), "null certificate has no public key");
    CEncodeDecode::releaseCert(nullptr);
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
    const int32_t status = to.feed(ciphertext, part, accepted);
    Check(accepted <= part && status == (accepted == part ? CENCODE_TLS_OK : CENCODE_TLS_WANT_IO), "TLS fragment feed");
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

/** 在对端处理之前交付本次产生的全部密文，限一个默认队列容量
@param [in,out] from 已完成本次 progress 的发送端
@param [in,out] to 输入队列为空的接收端
*/
static void TransferFlight(CEncodeDecodeTls& from, CEncodeDecodeTls& to)
{
    uint32_t total = 0;
    for (;;)
    {
        const uint8_t* data = nullptr;
        uint32_t size = 0;
        Check(from.peekOutput(data, size) == CENCODE_TLS_OK, "TLS handshake flight peek");
        if (size == 0)
        {
            Check(total != 0, "TLS handshake flight empty");
            return;
        }
        uint32_t accepted = 0;
        Check(size <= 32768 - total && to.feed(data, size, accepted) == CENCODE_TLS_OK &&
            accepted == size && from.consumeOutput(size) == CENCODE_TLS_OK, "TLS handshake flight transfer");
        total += size;
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
        static const uint8_t authority[] = TEST_CA_CRT_RSA_SHA256_PEM;
        CEncodeDecodeTlsOptions options = {};
        options.authority = authority;
        options.authoritySize = sizeof(authority);
        options.entropy = cEncodeTestEntropy;
        const std::string label(63, 'a');
        const std::string longest = label + "." + label + "." + label + "." + std::string(61, 'b');
        const std::string invalid[] = {".", "a..b", ".example", "-host.example", "a-.example", std::string(64, 'a'),
            longest + ".", longest + "x", "two.."};
        for (const std::string& hostname : invalid)
        {
            options.hostname = hostname.c_str();
            TlsHandle endpoint(CEncodeDecode::createTls(options), CEncodeDecode::destroyTls);
            Check(endpoint && endpoint->status() == CENCODE_TLS_ARGUMENT, "TLS invalid DNS label accepted");
        }
        CreateEndpoint(false, "localhost.");
        CreateEndpoint(false, label.c_str());
        CreateEndpoint(false, longest.c_str());
        const int32_t verificationDetail = client->detail();
        client = CreateEndpoint(false, "unrelated.invalid");
        server = CreateEndpoint(true, nullptr);
        Check(client->progress(1000) == CENCODE_TLS_HANDSHAKE, "TLS client hello");
        TransferFlight(*client, *server);
        Check(server->progress(1000) == CENCODE_TLS_HANDSHAKE, "TLS server certificate flight");
        TransferFlight(*server, *client);
        Check(client->finishInput() == CENCODE_TLS_OK && client->progress(1001) == CENCODE_TLS_PROTOCOL &&
            client->detail() == verificationDetail, "TLS buffered hostname verification at EOF");
        client = CreateEndpoint(false, "localhost");
        Check(client->finishInput() == CENCODE_TLS_OK && client->progress(1000) == CENCODE_TLS_TRUNCATED &&
            client->detail() != 0, "TLS handshake missing peer input at EOF");
        return;
    }
    Check(client->status() == CENCODE_TLS_READY && server->status() == CENCODE_TLS_READY, "TLS handshake");
    const uint8_t message[] = {0x81, 0, 0x5e, 0xff};
    Check(client->appendExact(message, 2, nullptr, 1) == CENCODE_TLS_ARGUMENT &&
        client->outputState() == CENCODE_TLS_OK, "TLS invalid second part left output queued");
    Check(client->appendExact(message, 2, message + 2, 2) == CENCODE_TLS_OK, "TLS two-part plaintext");
    uint8_t result[sizeof(message)] = {};
    uint32_t total = 0;
    for (unsigned step = 0; step < 128 && total < sizeof(result); ++step)
    {
        Check(client->progress(1400) == CENCODE_TLS_READY, "TLS write progress");
        Transfer(*client, *server);
        uint32_t received = 0;
        const int32_t status = server->read(result + total, 1, received);
        Check(received <= 1 && status == (received != 0 ? CENCODE_TLS_OK : CENCODE_TLS_WANT_IO), "TLS read progress");
        total += received;
    }
    Check(total == sizeof(message) && std::memcmp(result, message, total) == 0, "TLS payload");
    Check(client->outputState() == CENCODE_TLS_OK && client->append(message, 1, total) == CENCODE_TLS_OK &&
        total == 1 && client->appendExact(message + 1, 3) == CENCODE_TLS_OK &&
        client->outputState() == CENCODE_TLS_READY, "TLS appended plaintext pending");
    Check(client->progress(1401) == CENCODE_TLS_READY, "TLS flush appended plaintext");
    std::memset(result, 0x5a, sizeof(result));
    total = 0;
    for (unsigned step = 0; total < sizeof(result) && step < 128; ++step)
    {
        Transfer(*client, *server);
        uint32_t received = 0;
        const int32_t status = server->read(result + total, sizeof(result) - total, received);
        Check(received <= sizeof(result) - total && status == (received != 0 ? CENCODE_TLS_OK : CENCODE_TLS_WANT_IO),
            "TLS appended read");
        total += received;
    }
    Check(total == sizeof(result) && std::memcmp(result, message, total) == 0 &&
        client->outputState() == CENCODE_TLS_OK, "TLS appended payload");
    Check(client->close() == CENCODE_TLS_OK, "TLS orderly closure");
    for (unsigned step = 0; step < 128 && server->status() != CENCODE_TLS_CLOSED; ++step)
    {
        Transfer(*client, *server);
        if (client->outputState() == CENCODE_TLS_OK)
        {
            Check(server->finishInput() == CENCODE_TLS_OK, "TLS transport EOF after close notification");
        }
        uint32_t received = 0;
        const int32_t status = server->read(result, 1, received);
        Check(received == 0 && ((status == CENCODE_TLS_WANT_IO && server->status() == CENCODE_TLS_READY) ||
            (status == CENCODE_TLS_CLOSED && server->status() == CENCODE_TLS_CLOSED)), "TLS close notification");
    }
    Check(server->status() == CENCODE_TLS_CLOSED, "TLS authenticated end");
    Check(client->finishInput() == CENCODE_TLS_OK && client->read(result, sizeof(result), total) == CENCODE_TLS_TRUNCATED &&
        total == 0, "TLS transport EOF without peer close notification");
}

/** 在固定输入上采样密码接口，每次调用校验成功并将实际输出计入校验和
*/
static void MeasureCrypto()
{
    const std::string shortInput(32, 'M');
    const std::string block(4096, 'H');
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(block.data());
    uint8_t digest[32] = {};
    // 先核验本次工作负载的完整固定答案，计时区间内只读取一个结果字节形成校验和。
    const char expectedHash[] =
        "\x08\x80\xf4\xf8\x0c\x54\xd9\x45\xfa\x5e\x8c\x63\xc0\x39\x6b\xbf"
        "\x98\x97\x37\x61\x80\x9e\x93\x79\x07\x88\x53\x1c\x83\xb1\x31\x4c";
    const char expectedMac[] =
        "\xcc\xb5\x34\xb6\xb2\x0f\x97\x26\x1a\x16\xf6\xf4\xe4\x13\xe0\x95"
        "\x64\xf9\xaf\xa6\x57\x5a\x06\x30\x68\x27\x17\x7c\x0f\xde\xc2\x82";
    Check(CEncodeDecode::MD5Encode(shortInput) == "bff60fab237dc95adbedfb6ced30faa9", "MD5 benchmark input vector");
    Check(CEncodeDecode::sha256(bytes, 4096, digest, 32) == CENCODE_OK &&
        std::memcmp(digest, expectedHash, 32) == 0, "SHA256 benchmark input vector");
    Check(CEncodeDecode::hmacSha256(bytes, 32, bytes, 4096, digest, 32) == CENCODE_OK &&
        std::memcmp(digest, expectedMac, 32) == 0, "HMAC benchmark input vector");
    LibraryBenchmark::run("CEncodeDecode.MD5.32B", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            const std::string result = CEncodeDecode::MD5Encode(shortInput);
            Check(result.size() == 32, "MD5 benchmark digest");
            sum += result[i % 32];
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

/** 核对默认及显式设置的握手截止时间
@return 通过返回 0，失败抛出带步骤的测试异常
*/
static int CheckTlsDeadline()
{
    TlsHandle client = CreateEndpoint(false, "localhost");
    Check(client->progress(1000) == CENCODE_TLS_HANDSHAKE &&
        client->progress(10999) == CENCODE_TLS_HANDSHAKE &&
        client->progress(11000) == CENCODE_TLS_TIMEOUT, "TLS handshake deadline");
    TlsHandle shortDeadline = CreateEndpoint(false, "localhost", 100);
    Check(shortDeadline->progress(5000) == CENCODE_TLS_HANDSHAKE &&
        shortDeadline->progress(5099) == CENCODE_TLS_HANDSHAKE &&
        shortDeadline->progress(5100) == CENCODE_TLS_TIMEOUT, "TLS configured handshake deadline");
    TlsHandle maximum = CreateEndpoint(false, "localhost", 60000);
    Check(maximum->progress(600) == CENCODE_TLS_HANDSHAKE &&
        maximum->progress(60599) == CENCODE_TLS_HANDSHAKE &&
        maximum->progress(60600) == CENCODE_TLS_TIMEOUT, "TLS maximum handshake deadline");
    TlsHandle clock = CreateEndpoint(false, "localhost");
    Check(clock->progress(600) == CENCODE_TLS_HANDSHAKE && clock->progress(599) == CENCODE_TLS_ARGUMENT &&
        clock->progress(601) == CENCODE_TLS_ARGUMENT, "TLS decreasing clock stays failed");
    return 0;
}

/** 核对 TLS 两个方向的默认、固定和按需增长限额，以及跨块的字节顺序与借用期
@return 通过返回 0，失败抛出带步骤的测试异常
*/
static int CheckTlsQueueLimits()
{
    CEncodeDecodeTlsQueueLimits limits;
    Check(limits.inputBytes() == 32768 && limits.outputBytes() == 32768, "TLS default queue settings");
    std::vector<uint8_t> sent(4 * 16384 + 13);
    for (size_t i = 0; i < sent.size(); ++i) { sent[i] = static_cast<uint8_t>(i * 37 + i / 16384); }
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
    TlsHandle defaultPeer = CreateEndpoint(true, nullptr, 0, &limits);
    Handshake(defaultSender, defaultPeer);
    for (int record = 0; record < 2; ++record)
    {
        Check(defaultSender->write(sent.data() + record * 16384, 16384, count) == CENCODE_TLS_OK && count == 16384 &&
            defaultSender->progress(1400) == CENCODE_TLS_READY, "TLS default output record");
    }
    const uint8_t suffix[] = {0xa1, 0xb2, 0xc3};
    Check(defaultSender->write(suffix, sizeof(suffix), count) == CENCODE_TLS_WANT_IO && count == 0 &&
        defaultSender->append(suffix, sizeof(suffix), count) == CENCODE_TLS_WANT_IO && count == 0 &&
        defaultSender->appendExact(suffix, 1, suffix + 1, 2) == CENCODE_TLS_WANT_IO, "TLS frozen record rejects new data");
    size_t defaultPending = 0;
    for (;;)
    {
        Check(defaultSender->peekOutput(first, length) == CENCODE_TLS_OK, "TLS default output peek");
        if (length == 0)
        {
            break;
        }
        Check(first && length <= 32768 - defaultPending, "TLS default output limit");
        Check(defaultPeer->feed(first, length, count) == CENCODE_TLS_OK && count == length, "TLS bounded sender transfer");
        defaultPending += length;
        Check(defaultSender->consumeOutput(length) == CENCODE_TLS_OK, "TLS default output consume");
    }
    Check(defaultPending == 32768, "TLS default output backpressure");
    Check(defaultSender->progress(1401) == CENCODE_TLS_READY &&
        defaultSender->appendExact(suffix, 1, suffix + 1, 2) == CENCODE_TLS_OK &&
        defaultSender->progress(1402) == CENCODE_TLS_READY, "TLS retry after draining ciphertext");
    TransferFlight(*defaultSender, *defaultPeer);
    std::vector<uint8_t> record(16384);
    for (int index = 0; index < 2; ++index)
    {
        Check(defaultPeer->read(record.data(), 16384, count) == CENCODE_TLS_OK && count == 16384 &&
            std::memcmp(record.data(), sent.data() + index * 16384, count) == 0, "TLS frozen plaintext changed");
    }
    Check(defaultPeer->read(record.data(), 16384, count) == CENCODE_TLS_OK && count == sizeof(suffix) &&
        std::memcmp(record.data(), suffix, count) == 0 && defaultPeer->read(record.data(), 16384, count) == CENCODE_TLS_WANT_IO &&
        count == 0, "TLS retry delivered suffix exactly once");
    TlsHandle client = CreateEndpoint(false, "localhost", 0, &limits);
    TlsHandle server = CreateEndpoint(true, nullptr, 0, &limits);
    Handshake(client, server);
    Check(client->status() == CENCODE_TLS_READY && server->status() == CENCODE_TLS_READY, "TLS queue handshake");
    std::vector<uint8_t> prefix;
    size_t offset = 0;
    while (offset < sent.size())
    {
        Check(client->write(sent.data() + offset, static_cast<uint32_t>(sent.size() - offset), count) == CENCODE_TLS_OK &&
            count != 0 && count <= sent.size() - offset && client->progress(1400) == CENCODE_TLS_READY, "TLS queue record write");
        offset += count;
        if (prefix.empty())
        {
            Check(client->peekOutput(first, length) == CENCODE_TLS_OK && first && length != 0 && length <= 32768, "TLS borrow first output segment");
            prefix.assign(first, first + length);
        }
    }
    Check(std::memcmp(first, prefix.data(), prefix.size()) == 0, "TLS borrowed prefix stable across queue growth");
    size_t cipherSize = 0;
    for (;;)
    {
        Check(client->peekOutput(first, length) == CENCODE_TLS_OK, "TLS queued output peek");
        if (length == 0)
        {
            break;
        }
        // 五条记录的密文含记录开销；保留宽裕上界，使消费端停滞时能够报告失败。
        Check(first && length <= 2 * sent.size() - cipherSize, "TLS queued output exceeds test budget");
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
            count != 0 && count <= received.size() - offset, "TLS queued record read");
        offset += count;
    }
    Check(received == sent, "TLS cross-block plaintext integrity");
    return 0;
}

int main(int argc, char** argv)
{
    try
    {
        if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
        {
            MeasureCrypto();
            return 0;
        }
        const LibraryTestCase cases[] = {
            {kCryptoHash, false, []() -> int { CEncodeDecodeHashCases::hashBoundary(); return 0; }},
            {kCryptoDerivation, false, []() -> int { CEncodeDecodeHashCases::derivationBoundary(); return 0; }},
            {kCryptoParts, false, []() -> int { CEncodeDecodeHashCases::partsBoundary(); return 0; }},
            {kCryptoCertificate, false, []() -> int { CEncodeDecodeCertificateCases::run(); return 0; }},
            {kCryptoLegacy, true, []() -> int { CheckLegacy(); return 0; }},
            {kCryptoTlsTrusted, false, []() -> int { CheckTls(true); return 0; }},
            {kCryptoTlsWrongHost, false, []() -> int { CheckTls(false); return 0; }},
            {kCryptoTlsDeadline, false, CheckTlsDeadline},
            {kCryptoTlsQueueLimits, false, CheckTlsQueueLimits},
            {kCryptoEntropy, false, CheckEntropyBoundaries},
#ifdef CENCODE_TEST_PRIVATE_CASES
            {11, false, []() -> int { return cEncodeCompatibilityCase(1); }},
            {12, false, []() -> int { return cEncodeCompatibilityCase(2); }},
            {13, false, []() -> int { return cEncodeCompatibilityCase(3); }},
            {14, false, []() -> int { return cEncodeCompatibilityCase(4); }},
            {15, true, []() -> int { return cEncodeCompatibilityCase(5); }},
            {16, true, []() -> int { return cEncodeCompatibilityCase(6); }},
            {17, true, []() -> int { return cEncodeCompatibilityCase(7); }},
            {18, true, []() -> int { return cEncodeCompatibilityCase(8); }},
            {19, false, []() -> int { return cEncodeCompatibilityCase(9); }},
#ifdef USE_OPENSSL
            {20, false, []() -> int { return cEncodeCompatibilityCase(10); }},
#endif
            {21, false, []() -> int { return cEncodeCompatibilityCase(11); }},
#endif
        };
        const int result = LibraryCaseSelection::run(argc, argv, cases, [](const LibraryTestCase& item) -> int
        {
            try
            {
                TestEntropyGenerator().seed(0x8fa29d19U + item.id);
                return item.run();
            }
            catch (const std::exception& error)
            {
                std::fprintf(stderr, "CEncodeDecodeTest case %u: %s\n", item.id, error.what());
                return 1;
            }
        });
        if (result == 0) { std::puts("CEncodeDecodeTest PASS"); }
        return result;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "CEncodeDecodeTest FAIL: %s\n", error.what());
        return 1;
    }
}