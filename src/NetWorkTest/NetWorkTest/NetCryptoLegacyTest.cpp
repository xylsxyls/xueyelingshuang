#include "NetCryptoLegacyTest.h"
#include "NetWorkTest.h"
#include "NetTestTlsCertificates.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include "CEncodeDecode/CEncodeDecodeTls.h"
#include "CEncodeDecode/MD5.h"
#include "CEncodeDecode/sm2_create_key_pair.h"
#include "CEncodeDecode/sm2_encrypt_and_decrypt.h"
#include <cstring>
#include <stdexcept>
#define CENCODE_BENCH_MBEDTLS
#ifdef USE_OPENSSL
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#endif

#include "CEncodeDecode/AES.h"
#include "CEncodeDecode/sm4.h"
#include "CEncodeDecode/aes_encryptor.h"
#include <fstream>
#include <iterator>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
// 测试宿主提供强随机字节；正式小程序仍使用业务宿主随机池
EM_JS(int, CEncodeTestHostRandom, (uint8_t* output, uint32_t size), {
    try {
        var bytes = new Uint8Array(size);
        require('crypto').randomFillSync(bytes);
        HEAPU8.set(bytes, Number(output));
        return 0;
    } catch (error) { return -1; }
});
#else
#include "CSystem/CSystemAPI.h"
#endif

/** 单测随机源，原生转交CSystem，Wasm转交Node宿主
@param [in] context 不使用
@param [out] output size字节
@param [in] size 随机字节数
@return 0成功，否则失败
*/
static int32_t CEncodeTestRandom(void* context, uint8_t* output, uint32_t size)
{
#ifdef __EMSCRIPTEN__
    (void)context;
    return CEncodeTestHostRandom(output, size);
#else
    return CSystem::secureRandomBytesCallback(context, output, size);
#endif
}
#ifdef CENCODE_TEST_ADAPTER
#include "NetInterface/NetInterfaceAPI.h"
#endif
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <algorithm>
#include <thread>
#include <atomic>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
#ifdef CENCODE_BENCH_MBEDTLS
#include <mbedtls/aes.h>
#endif

/** 执行指定的独立回归或性能用例，非零退出表示失败
@param [in] id 用例ID：1散列、2兼容AES、3畸形AES、4SM4、5RSA、6性能、7并发、8TLS、9SM2与证书、10RSA互通、11文件与分组边界
@return 0通过，1断言失败，2参数错误
*/
int NetCryptoLegacyTest::check(int id)
{
#ifdef _MSC_VER
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif

    int failed = 0;
    int checks = 0;
    // 比较独立预期；条件失败时保留场景名称并累加失败，不中断后续检查
    const auto check = [&failed, &checks](bool condition, const char* name)
    {
        ++checks;
        if (!condition)
        {
            ++failed;
            printf("FAIL %s\n", name);
        }
    };
    if (id == 1)
    {
        check(CEncodeDecode::MD5Encode("") == "d41d8cd98f00b204e9800998ecf8427e", "MD5 empty RFC1321");
        check(CEncodeDecode::MD5Encode("abc") == "900150983cd24fb0d6963f7d28e17f72", "MD5 abc RFC1321");
        check(CEncodeDecode::MD5Encode(std::string(1000000, 'a')) == "7707d6ae4e027c70eea2a935c2296f21", "MD5 million a");
        MD5 stream("a");
        check(stream.toString() == "0cc175b9c0f1b6a831c399e269772661", "MD5 first snapshot");
        check(stream.toString() == "0cc175b9c0f1b6a831c399e269772661", "MD5 repeated snapshot");
        stream.update("bc");
        check(stream.toString() == "900150983cd24fb0d6963f7d28e17f72", "MD5 continue after snapshot");
        for (size_t size = 0; size < 260; ++size)
        {
            std::string input(size, 'x');
            MD5 chunks;
            for (size_t i = 0; i < size; ++i)
            {
                chunks.update(input.data() + i, 1);
                chunks.toString();
            }
            check(chunks.toString() == CEncodeDecode::MD5Encode(input), "MD5 boundary streaming");
        }
    }
    else if (id == 2)
    {
        unsigned char key[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        unsigned char block[16] = {0,17,34,51,68,85,102,119,136,153,170,187,204,221,238,255};
        const unsigned char expected[16] = {0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a};
        unsigned char original[16];
        memcpy(original, block, sizeof(block));
        AES aes(key);
        aes.Cipher(block);
        check(memcmp(block, expected, 16) == 0, "AES128 FIPS197 encrypt");
        aes.InvCipher(block);
        check(memcmp(block, original, 16) == 0, "AES128 FIPS197 decrypt");
        check(CEncodeDecode::AESEncode("", "") == "66E94BD4EF8A2C3B884CFA59CA342B2E", "legacy AES zero key and zero padding");
        const size_t lengths[] = {0,1,15,16,17,31,32,63,64,65,999,1000,1001,16383,16384,16385};
        for (size_t k = 0; k < 33; ++k)
        {
            std::string password(k, 'K');
            for (size_t n = 0; n < sizeof(lengths)/sizeof(lengths[0]); ++n)
            {
                std::string input(lengths[n], 'x');
                for (size_t i = 0; i < input.size(); ++i)
                {
                    input[i] = static_cast<char>(1 + i % 255);
                }
                std::string cipher = CEncodeDecode::AESEncode(password, input);
                check(cipher.size() == (input.size()/16 + 1)*32, "legacy AES padding length");
                check(CEncodeDecode::AESDecode(password, cipher) == input, "legacy AES roundtrip");
            }
        }
        std::string encoded = CEncodeDecode::AESEncode("Yangnan8", "Package compatibility");
        printf("COMPAT %s\n", encoded.c_str());
        std::string lower = encoded;
        for (size_t i = 0; i < lower.size(); ++i)
        {
            if (lower[i] >= 'A' && lower[i] <= 'F')
            {
                lower[i] += 'a' - 'A';
            }
        }
        check(CEncodeDecode::AESDecode("Yangnan8", lower) == "Package compatibility", "lowercase hex");
        check(CEncodeDecode::AESEncode(std::string(17, 'K'), "x") == CEncodeDecode::AESEncode(std::string(16, 'K'), "x"), "legacy long key truncation");
    }
    else if (id == 3)
    {
        for (size_t n = 1; n < 128; ++n)
        {
            if (n % 32 != 0)
            {
                check(CEncodeDecode::AESDecode("k", std::string(n, '0')).empty(), "AES truncated block");
            }
        }
        for (int value = 0; value < 256; ++value)
        {
            char c = static_cast<char>(value);
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
            {
                continue;
            }
            std::string bad(32, '0');
            bad[17] = c;
            check(CEncodeDecode::AESDecode("k", bad).empty(), "AES invalid hex");
        }
        unsigned char key[16] = {};
        AES aes(key);
        unsigned char guarded[48];
        memset(guarded, 0x5a, sizeof(guarded));
        check(aes.Cipher(static_cast<void*>(guarded + 16), 15) == nullptr, "AES primitive rejects partial block");
        check(aes.InvCipher(static_cast<void*>(guarded + 16), -16) == nullptr, "AES primitive rejects negative size");
        check(aes.Cipher(static_cast<void*>(nullptr), 16) == nullptr, "AES primitive rejects null");
        for (size_t i = 0; i < sizeof(guarded); ++i)
        {
            check(guarded[i] == 0x5a, "AES rejected input is unchanged");
        }
    }
    else if (id == 4)
    {
        const unsigned char key[16] = {1,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10};
        const unsigned char expected[16] = {0x68,0x1e,0xdf,0x34,0xd2,6,0x96,0x5e,0x86,0xb3,0xe9,0x4f,0x53,0x6e,0x42,0x46};
        std::string k(reinterpret_cast<const char*>(key), 16);
        std::string e(reinterpret_cast<const char*>(expected), 16);
        check(CEncodeDecode::SM4Encode(k, k) == e, "SM4 standard encrypt");
        check(CEncodeDecode::SM4Decode(k, e) == k, "SM4 standard decrypt");
        for (size_t n = 0; n < 128; ++n)
        {
            std::string data(n, '\0');
            std::string encrypted = CEncodeDecode::SM4Encode(k, data);
            check(n % 16 ? encrypted.empty() : CEncodeDecode::SM4Decode(k, encrypted) == data, "SM4 boundaries and binary zero");
        }
        for (size_t n = 0; n < 33; ++n)
        {
            if (n != 16)
            {
                check(CEncodeDecode::SM4Encode(std::string(n, 'k'), k).empty(), "SM4 invalid key");
            }
        }
    }
    else if (id == 5)
    {
        std::string publicKey, privateKey;
        check(CEncodeDecode::RSAKey(publicKey, privateKey, CEncodeTestRandom, nullptr), "RSA2048 key generation");
        check(publicKey.find("-----BEGIN PUBLIC KEY-----") == 0, "RSA public PEM");
        check(privateKey.find("-----BEGIN RSA PRIVATE KEY-----") == 0, "RSA private PEM");
        std::string encrypted = CEncodeDecode::RSAEncode(publicKey, "RSA OAEP-SHA256", CEncodeTestRandom, nullptr);
        check(encrypted.size() == 256, "RSA ciphertext exact modulus width");
        check(CEncodeDecode::RSADecode(privateKey, encrypted, CEncodeTestRandom, nullptr) == "RSA OAEP-SHA256", "RSA OAEP decrypt");
        check(encrypted != CEncodeDecode::RSAEncode(publicKey, "RSA OAEP-SHA256", CEncodeTestRandom, nullptr), "RSA probabilistic padding");
        std::string largest(190, 'z');
        check(CEncodeDecode::RSADecode(privateKey, CEncodeDecode::RSAEncode(publicKey, largest.c_str(), CEncodeTestRandom, nullptr), CEncodeTestRandom, nullptr) == largest, "OAEP 190 byte limit");
        check(CEncodeDecode::RSAEncode(publicKey, std::string(191, 'z').c_str(), CEncodeTestRandom, nullptr).empty(), "OAEP rejects 191 bytes");
        check(CEncodeDecode::RSAEncode(publicKey, nullptr, CEncodeTestRandom, nullptr).empty(), "RSA null plaintext");
        check(CEncodeDecode::RSAEncode(publicKey, "", CEncodeTestRandom, nullptr).size() == 256, "RSA valid empty plaintext");
        const char* malformed[] = {"", "0;0", "1,2;3,4", "-----BEGIN PUBLIC KEY-----invalid"};
        for (size_t i = 0; i < sizeof(malformed)/sizeof(malformed[0]); ++i)
        {
            check(CEncodeDecode::RSAEncode(malformed[i], "x", CEncodeTestRandom, nullptr).empty(), "RSA rejects malformed and obsolete keys");
            check(CEncodeDecode::RSADecode(malformed[i], encrypted, CEncodeTestRandom, nullptr).empty(), "RSA private key validation");
        }
        check(CEncodeDecode::RSAEncode(std::string(16385, 'x'), "x", CEncodeTestRandom, nullptr).empty(), "RSA oversized key");
        check(CEncodeDecode::RSAEncode(publicKey + std::string(1, '\0'), "x", CEncodeTestRandom, nullptr).empty(), "RSA embedded null key");
        for (size_t i = 0; i < encrypted.size(); i += 17)
        {
            std::string corrupt(encrypted);
            corrupt[i] ^= 1;
            check(CEncodeDecode::RSADecode(privateKey, corrupt, CEncodeTestRandom, nullptr).empty(), "RSA rejects corrupted ciphertext");
        }
        check(CEncodeDecode::RSADecode(privateKey, std::string(255, 'x'), CEncodeTestRandom, nullptr).empty(), "RSA truncated ciphertext");
        check(CEncodeDecode::RSADecode(privateKey, std::string(257, 'x'), CEncodeTestRandom, nullptr).empty(), "RSA oversized ciphertext");
        std::string otherPublic, otherPrivate;
        check(CEncodeDecode::RSAKey(otherPublic, otherPrivate, CEncodeTestRandom, nullptr), "RSA independent key");
        check(otherPublic != publicKey && otherPrivate != privateKey, "RSA key generation uniqueness");
        check(CEncodeDecode::RSADecode(otherPrivate, encrypted, CEncodeTestRandom, nullptr).empty(), "RSA wrong private key");
        check(!CEncodeDecode::RSAKey(otherPublic, otherPublic, CEncodeTestRandom, nullptr) && otherPublic.empty(), "RSA output alias rejection");
        // 模拟随机源部分输出后失败；不能降级到伪随机源
        CEncodeDecodeEntropyFunction broken = [](void* context, uint8_t* output, uint32_t size) -> int32_t
        {
            memset(output, 0xa5, size / 2);
            if (context)
            {
                throw 1;
            }
            return -1;
        };
        for (int mode = 0; mode < 2; ++mode)
        {
            void* context = mode ? &mode : nullptr;
            otherPublic = "old";
            otherPrivate = "old";
            check(!CEncodeDecode::RSAKey(otherPublic, otherPrivate, broken, context) && otherPublic.empty() && otherPrivate.empty(), "RSA random failure clears outputs");
            check(CEncodeDecode::RSAEncode(publicKey, "x", broken, context).empty(), "RSA random failure encryption");
            check(CEncodeDecode::RSADecode(privateKey, encrypted, broken, context).empty(), "RSA random failure blinding");
        }
    }
    else if (id == 6)
    {
#ifdef CENCODE_BENCH_MBEDTLS
        unsigned char key[16] = {};
        AES legacy(key);
        mbedtls_aes_context reference;
        mbedtls_aes_init(&reference);
        const size_t sizes[] = {16,64,1024,16384,65536};
        for (int decrypt = 0; decrypt < 2; ++decrypt)
        {
            check((decrypt ? mbedtls_aes_setkey_dec(&reference, key, 128) : mbedtls_aes_setkey_enc(&reference, key, 128)) == 0, "mbedTLS AES key");
            for (size_t i = 0; i < sizeof(sizes)/sizeof(sizes[0]); ++i)
            {
                std::string left(sizes[i], 'A'), right(left);
                if (decrypt)
                {
                    legacy.InvCipher(&left[0], static_cast<int>(left.size()));
                }
                else
                {
                    legacy.Cipher(&left[0], static_cast<int>(left.size()));
                }
                for (size_t n = 0; n < right.size(); n += 16)
                {
                    check(mbedtls_aes_crypt_ecb(&reference, decrypt ? MBEDTLS_AES_DECRYPT : MBEDTLS_AES_ENCRYPT,
                        reinterpret_cast<unsigned char*>(&right[n]), reinterpret_cast<unsigned char*>(&right[n])) == 0, "reference AES");
                }
                check(left == right, "same mode key and direction equality");
                for (int backend = 0; backend < 2; ++backend)
                {
                    double samples[5];
                    for (int sample = 0; sample < 5; ++sample)
                    {
                        size_t count = 0;
                        double seconds = 0;
                        const auto start = std::chrono::steady_clock::now();
                        // 每个样本至少运行100毫秒，避免高速实现只测到时钟抖动
                        do
                        {
                            for (size_t batch = 0; batch < 16; ++batch)
                            {
                                if (backend == 0)
                                {
                                    if (decrypt)
                                    {
                                        legacy.InvCipher(&left[0], static_cast<int>(left.size()));
                                    }
                                    else
                                    {
                                        legacy.Cipher(&left[0], static_cast<int>(left.size()));
                                    }
                                }
                                else
                                {
                                    for (size_t n = 0; n < right.size(); n += 16)
                                    {
                                        mbedtls_aes_crypt_ecb(&reference, decrypt ? MBEDTLS_AES_DECRYPT : MBEDTLS_AES_ENCRYPT,
                                            reinterpret_cast<unsigned char*>(&right[n]), reinterpret_cast<unsigned char*>(&right[n]));
                                    }
                                }
                            }
                            count += 16;
                            seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                        } while (seconds < 0.1);
                        samples[sample] = count * sizes[i] / 1048576.0 / seconds;
                    }
                    std::sort(samples, samples + 5);
                    printf("BENCH AES128_ECB direction=%s bytes=%u backend=%s median_MiB_s=%.3f\n",
                        decrypt ? "decrypt" : "encrypt", static_cast<unsigned>(sizes[i]), backend ? "mbedTLS" : "legacy", samples[2]);
                }
            }
        }
        mbedtls_aes_free(&reference);
        std::string publicKey, privateKey;
        double generation[5];
        for (int sample = 0; sample < 5; ++sample)
        {
            const auto start = std::chrono::steady_clock::now();
            check(CEncodeDecode::RSAKey(publicKey, privateKey, CEncodeTestRandom, nullptr), "RSA benchmark generation");
            generation[sample] = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        }
        std::sort(generation, generation + 5);
        printf("BENCH RSA2048 operation=keygen median_ms=%.3f\n", generation[2]);
        std::string input(190, 'r');
        std::string ciphertext = CEncodeDecode::RSAEncode(publicKey, input.c_str(), CEncodeTestRandom, nullptr);
        for (int decrypt = 0; decrypt < 2; ++decrypt)
        {
            double samples[21];
            for (int sample = 0; sample < 21; ++sample)
            {
                const auto start = std::chrono::steady_clock::now();
                std::string result;
                int batchCount = decrypt ? 5 : 50;
                for (int n = 0; n < batchCount; ++n)
                {
                    result = decrypt ? CEncodeDecode::RSADecode(privateKey, ciphertext, CEncodeTestRandom, nullptr) :
                        CEncodeDecode::RSAEncode(publicKey, input.c_str(), CEncodeTestRandom, nullptr);
                }
                samples[sample] = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / batchCount;
                check(decrypt ? result == input : result.size() == 256, "RSA benchmark correctness");
            }
            std::sort(samples, samples + 21);
            printf("BENCH RSA2048_OAEP_SHA256 operation=%s median_ms=%.3f\n", decrypt ? "decrypt" : "encrypt", samples[10]);
        }
#else
        return 2;
#endif
    }
    else if (id == 7)
    {
#ifndef __EMSCRIPTEN__
        std::atomic<int> errors(0);
        std::thread workers[4];
        for (size_t i = 0; i < 4; ++i)
        {
            // 每线程独立输入和密码，验证公开无状态入口并发不会串用上下文
            workers[i] = std::thread([&errors, i]()
            {
                std::string key(16, static_cast<char>('A' + i));
                for (size_t n = 0; n < 2000; ++n)
                {
                    std::string data(n % 257, static_cast<char>('a' + i));
                    if (CEncodeDecode::AESDecode(key, CEncodeDecode::AESEncode(key, data)) != data || CEncodeDecode::MD5Encode("abc") != "900150983cd24fb0d6963f7d28e17f72")
                    {
                        ++errors;
                    }
                }
            });
        }
        for (size_t i = 0; i < 4; ++i)
        {
            workers[i].join();
        }
        check(errors.load() == 0, "four threads 8000 roundtrips");
#else
        return 2;
#endif
    }
    else if (id == 8)
    {
        // 复用仓库公开测试证书；逐字节输送模拟任意TCP拆包，不打开监听端口
        // 使用Test工程已编入的公开证书，测试不依赖当前目录或构建机源码路径
        std::string ca(mbedtls_test_ca_crt_rsa_sha256_pem, mbedtls_test_ca_crt_rsa_sha256_pem_len);
        std::string wrongCa(mbedtls_test_ca_crt_ec_pem, mbedtls_test_ca_crt_ec_pem_len);
        std::string cert(mbedtls_test_srv_crt_rsa_sha256_pem, mbedtls_test_srv_crt_rsa_sha256_pem_len);
        std::string key(mbedtls_test_srv_key_rsa_pem, mbedtls_test_srv_key_rsa_pem_len);
        check(ca.size() > 100 && cert.size() > 100 && key.size() > 100, "TLS fixtures present");
        if (failed)
        {
            return 1;
        }
#ifdef CENCODE_TEST_ADAPTER
        typedef NetInterfaceTls Channel;
        typedef NetInterfaceTlsOptions Options;
#else
        typedef CEncodeDecodeTls Channel;
        typedef CEncodeDecodeTlsOptions Options;
#endif
        Options client = {};
        client.authority = reinterpret_cast<const uint8_t*>(ca.data());
        client.authoritySize = static_cast<uint32_t>(ca.size());
        client.hostname = "localhost";
        client.entropy = CEncodeTestRandom;
        client.handshakeTimeoutMs = 10000;
        Options server = client;
        server.server = 1;
        server.certificate = reinterpret_cast<const uint8_t*>(cert.data());
        server.certificateSize = static_cast<uint32_t>(cert.size());
        server.privateKey = reinterpret_cast<const uint8_t*>(key.data());
        server.privateKeySize = static_cast<uint32_t>(key.size());
        // 最多搬运一个连续片段；返回实际消费数，输入满时保留发送端数据
        const auto transfer = [&check](Channel& from, Channel& to, uint32_t fragment) -> uint32_t
        {
            const uint8_t* bytes = nullptr;
            uint32_t size = 0, used = 0;
            from.peekOutput(bytes, size);
            if (size && to.status() >= 0)
            {
                check(to.feed(bytes, (std::min)(size, fragment), used) >= 0, "TLS feed");
                check(from.consumeOutput(used) == 0, "TLS consume");
            }
            return used;
        };
        // 有界循环防止测试本身挂死；计时与搬运颗粒分离，逐字节握手仍可完成
        const auto handshake = [&transfer](Channel& left, Channel& right, uint32_t fragment) -> bool
        {
            for (uint32_t step = 0; step < 20000; ++step)
            {
                if (left.progress(1000 + step / 64) < 0 || right.progress(1000 + step / 64) < 0)
                {
                    return false;
                }
                transfer(left, right, fragment);
                transfer(right, left, fragment);
                if (left.status() == 2 && right.status() == 2)
                {
                    return true;
                }
            }
            return false;
        };
        const uint32_t fragments[] = {1, 7, 16384, 65536};
        for (size_t i = 0; i < sizeof(fragments)/sizeof(fragments[0]); ++i)
        {
            Channel left(client), right(server);
            check(handshake(left, right, fragments[i]), "TLS fragmented authenticated handshake");
            if (left.status() != 2 || right.status() != 2)
            {
                printf("TLS detail client=%d server=%d\n", left.detail(), right.detail());
                continue;
            }
            uint32_t used = 0;
            check(left.write(nullptr, 1, used) == -101 && used == 0, "TLS null write");
            check(left.consumeOutput(1) == -101, "TLS excessive consume");
            std::string input(16384, 'x'), output;
            input[0] = 0; input[1000] = 0;
            check(left.write(reinterpret_cast<const uint8_t*>(input.data()), 16384, used) == 0 && used == 16384, "TLS copied write");
            check(left.write(reinterpret_cast<const uint8_t*>(input.data()), 1, used) == 4 && used == 0, "TLS plaintext bounded queue");
            input[1] = 'y';
            for (uint32_t step = 0; step < 20000 && output.size() < 16384; ++step)
            {
                left.progress(2000 + step / 64);
                transfer(left, right, fragments[i]);
                uint8_t bytes[997]; uint32_t count = 0;
                int32_t result = right.read(bytes, sizeof(bytes), count);
                check(result == 0 || result == 4, "TLS application read");
                output.append(reinterpret_cast<char*>(bytes), count);
            }
            input[1] = 'x';
            check(output == input, "TLS exact binary payload after copy");
            check(left.outputState() == 0, "TLS drained output");
            check(left.close() == 0, "TLS close notify");
            for (int step = 0; step < 100 && right.status() >= 0 && right.status() != 3; ++step)
            {
                transfer(left, right, 1024);
                uint8_t byte; right.read(&byte, 1, used);
            }
            check(right.status() == 3, "TLS authenticated close");
            check(left.close() == 0, "TLS close idempotent");
        }
        for (int mode = 0; mode < 2; ++mode)
        {
            Options bad = client;
            if (mode == 0)
            {
                bad.hostname = "wrong.example";
            }
            else
            {
                bad.authority = reinterpret_cast<const uint8_t*>(wrongCa.data());
                bad.authoritySize = static_cast<uint32_t>(wrongCa.size());
            }
            Channel left(bad), right(server);
            check(!handshake(left, right, 65536) && left.status() < 0, "TLS rejects untrusted peer");
        }
        {
            Channel left(client), right(server);
            check(handshake(left, right, 65536), "TLS truncation setup");
            right.finishInput();
            uint8_t byte; uint32_t count;
            check(right.read(&byte, 1, count) == -105 && count == 0, "TLS rejects unauthenticated EOF");
        }
        {
            Channel left(client), right(server);
            check(handshake(left, right, 65536), "TLS tamper setup");
            uint32_t count;
            left.write(reinterpret_cast<const uint8_t*>("secret"), 6, count);
            left.progress(2000);
            const uint8_t* bytes; uint32_t size;
            left.peekOutput(bytes, size);
            check(size > 10, "TLS encrypted record");
            if (size > 10)
            {
                std::string record(reinterpret_cast<const char*>(bytes), size);
                record.back() ^= 1;
                right.feed(reinterpret_cast<const uint8_t*>(record.data()), size, count);
                uint8_t plain[64] = {};
                check(right.read(plain, sizeof(plain), count) == -103 && count == 0, "TLS rejects tampered record");
            }
        }
        {
            Channel timeout(client), backwards(client);
            check(timeout.progress(100) == 1 && timeout.progress(10100) == -104, "TLS timeout");
            check(backwards.progress(100) == 1 && backwards.progress(99) == -101, "TLS monotonic clock");
            Options invalid = {};
            Channel rejected(invalid);
            check(rejected.status() == -101, "TLS invalid config");
        }
        for (int i = 0; i < 100; ++i)
        {
            Channel owned(client);
            check(owned.status() == 1, "TLS repeated context lifecycle");
        }
    }
    else if (id == 9)
    {
#ifdef USE_OPENSSL
        const size_t lengths[] = {1, 16, 32, 255, 4096};
        for (int prefix = 0; prefix < 2; ++prefix)
        {
            std::string x, y, privateKey;
            check(CEncodeDecode::SM2Key(x, y, privateKey, prefix != 0), "SM2 key generation");
            check(x.size() == (prefix ? 33 : 32) && y.size() == 32 && privateKey.size() == 32, "SM2 key widths");
            if (failed)
            {
                return 1;
            }
            for (size_t i = 0; i < sizeof(lengths)/sizeof(lengths[0]); ++i)
            {
                std::string input(lengths[i], '\0'), c1, c2, c3;
                for (size_t n = 0; n < input.size(); ++n)
                {
                    input[n] = static_cast<char>(n);
                }
                check(CEncodeDecode::SM2Encode(x, y, input, c1, c2, c3), "SM2 binary encrypt");
                check(c1.size() == (prefix ? 65 : 64) && c2.size() == input.size() && c3.size() == 32, "SM2 segment widths");
                check(CEncodeDecode::SM2Decode(privateKey, c1, c2, c3) == input, "SM2 roundtrip");
                std::string alt = prefix ? c1.substr(1) : std::string(1, 4) + c1;
                check(CEncodeDecode::SM2Decode(privateKey, alt, c2, c3) == input, "SM2 both point formats");
                for (size_t n = 0; n < c3.size(); ++n)
                {
                    std::string bad = c3; bad[n] ^= 1;
                    check(CEncodeDecode::SM2Decode(privateKey, c1, c2, bad).empty(), "SM2 digest tamper");
                }
                std::string corrupted = c2; corrupted[0] ^= 1;
                check(CEncodeDecode::SM2Decode(privateKey, c1, corrupted, c3).empty(), "SM2 payload tamper");
                check(CEncodeDecode::SM2Decode(std::string(32, '\0'), c1, c2, c3).empty(), "SM2 zero scalar");
                check(CEncodeDecode::SM2Decode(std::string(32, static_cast<char>(255)), c1, c2, c3).empty(), "SM2 scalar above order");
                check(CEncodeDecode::SM2Decode(privateKey, std::string(65, '\0'), c2, c3).empty(), "SM2 invalid point");
                check(!CEncodeDecode::SM2Encode(x, y, input, c1, c1, c3), "SM2 aliased outputs");
                const std::string oldX = x;
                check(!CEncodeDecode::SM2Encode(x, y, input, x, c2, c3) && x == oldX, "SM2 aliased input preserved");
            }
        }
        std::string x = "x", y = "y", z = "z";
        check(!CEncodeDecode::SM2Encode("bad", "bad", "x", x, y, z) && x.empty() && y.empty() && z.empty(), "SM2 failed output cleanup");
        check(sm2_create_key_pair(nullptr) != 0, "SM2 null key output");
        check(sm2_encrypt_code(nullptr, 0, nullptr, nullptr, nullptr, nullptr) != 0, "SM2 raw null input");
        check(sm2_decrypt_code(nullptr, nullptr, nullptr, -1, nullptr, nullptr) != 0, "SM2 raw negative length");
        check(CEncodeDecode::getCert("file-does-not-exist.crt") == nullptr, "X509 missing file");
        check(CEncodeDecode::getPublicKey(nullptr).empty(), "X509 null certificate");
        X509* ecCert = CEncodeDecode::getCert("ec.crt");
        check(ecCert != nullptr, "X509 EC fixture read");
        std::string expectedKey = CEncodeDecode::getPublicKey(ecCert);
        check(expectedKey.size() == 65 && expectedKey[0] == 4, "X509 EC point encoding");
        for (int i = 0; i < 10000; ++i)
        {
            check(CEncodeDecode::getPublicKey(ecCert) == expectedKey, "X509 repeated owned key extraction");
        }
        CEncodeDecode::releaseCert(ecCert);
        for (int i = 0; i < 1000; ++i)
        {
            X509* cert = CEncodeDecode::getCert("server2-sha256.crt");
            check(cert != nullptr, "X509 RSA PEM read");
            check(CEncodeDecode::getPublicKey(cert).empty(), "X509 non-EC key rejected");
            CEncodeDecode::releaseCert(cert);
        }
#else
        std::string x = "x", y = "y", z = "z";
        check(!CEncodeDecode::SM2Key(x, y, z) && x.empty() && y.empty() && z.empty(), "disabled SM2 clears key outputs");
        x = y = z = "old";
        check(!CEncodeDecode::SM2Encode("x", "y", "data", x, y, z) && x.empty() && y.empty() && z.empty(), "disabled SM2 clears cipher outputs");
        check(CEncodeDecode::SM2Decode("x", "y", "z", "digest").empty(), "disabled SM2 decrypt");
#endif
    }
    else if (id == 10)
    {
#ifdef USE_OPENSSL
        std::string publicKey, privateKey;
        check(CEncodeDecode::RSAKey(publicKey, privateKey, CEncodeTestRandom, nullptr), "RSA interoperability key");
        BIO* publicBio = BIO_new_mem_buf(publicKey.data(), static_cast<int>(publicKey.size()));
        BIO* privateBio = BIO_new_mem_buf(privateKey.data(), static_cast<int>(privateKey.size()));
        EVP_PKEY* publicParsed = PEM_read_bio_PUBKEY(publicBio, nullptr, nullptr, nullptr);
        EVP_PKEY* privateParsed = PEM_read_bio_PrivateKey(privateBio, nullptr, nullptr, nullptr);
        BIO_free(publicBio); BIO_free(privateBio);
        check(publicParsed && privateParsed, "OpenSSL parses generated PEM");
        if (!publicParsed || !privateParsed)
        {
            EVP_PKEY_free(publicParsed); EVP_PKEY_free(privateParsed);
            return 1;
        }
        EVP_PKEY_CTX* encrypt = EVP_PKEY_CTX_new(publicParsed, nullptr);
        EVP_PKEY_CTX* decrypt = EVP_PKEY_CTX_new(privateParsed, nullptr);
        check(encrypt && decrypt, "OpenSSL RSA contexts");
        if (!encrypt || !decrypt)
        {
            EVP_PKEY_CTX_free(encrypt); EVP_PKEY_CTX_free(decrypt);
            EVP_PKEY_free(publicParsed); EVP_PKEY_free(privateParsed);
            return 1;
        }
        check(EVP_PKEY_encrypt_init(encrypt) == 1 && EVP_PKEY_CTX_set_rsa_padding(encrypt, RSA_PKCS1_OAEP_PADDING) == 1 &&
            EVP_PKEY_CTX_set_rsa_oaep_md(encrypt, EVP_sha256()) == 1 && EVP_PKEY_CTX_set_rsa_mgf1_md(encrypt, EVP_sha256()) == 1, "OpenSSL encrypt OAEP SHA256");
        check(EVP_PKEY_decrypt_init(decrypt) == 1 && EVP_PKEY_CTX_set_rsa_padding(decrypt, RSA_PKCS1_OAEP_PADDING) == 1 &&
            EVP_PKEY_CTX_set_rsa_oaep_md(decrypt, EVP_sha256()) == 1 && EVP_PKEY_CTX_set_rsa_mgf1_md(decrypt, EVP_sha256()) == 1, "OpenSSL decrypt OAEP SHA256");
        for (size_t n = 0; n <= 190; n += 19)
        {
            std::string input(n, 'a');
            std::string encrypted = CEncodeDecode::RSAEncode(publicKey, input.c_str(), CEncodeTestRandom, nullptr);
            unsigned char bytes[256] = {}; size_t size = sizeof(bytes);
            check(EVP_PKEY_decrypt(decrypt, bytes, &size, reinterpret_cast<const unsigned char*>(encrypted.data()), encrypted.size()) == 1 &&
                std::string(reinterpret_cast<char*>(bytes), size) == input, "mbedTLS encrypt to OpenSSL decrypt");
            size = sizeof(bytes);
            check(EVP_PKEY_encrypt(encrypt, bytes, &size, reinterpret_cast<const unsigned char*>(input.data()), input.size()) == 1, "OpenSSL OAEP encryption");
            check(CEncodeDecode::RSADecode(privateKey, std::string(reinterpret_cast<char*>(bytes), size), CEncodeTestRandom, nullptr) == input, "OpenSSL encrypt to mbedTLS decrypt");
        }
        EVP_PKEY_CTX_free(encrypt); EVP_PKEY_CTX_free(decrypt);
        EVP_PKEY_free(publicParsed); EVP_PKEY_free(privateParsed);
#else
        return 2;
#endif
    }
    else if (id == 11)
    {
        const std::string executable = CSystem::currentExecutablePathUtf8();
        const std::string directory = executable.substr(0, executable.find_last_of("/\\")) + "/crypto-file-test";
        if (!CSystem::createDirectoriesUtf8(directory))
        {
            return 1;
        }
        unsigned char key[16] = {};
        AesEncryptor aes(key);
        // 本用例在独立工作目录读写；检查失败路径不会覆盖已有目标文件
        const auto put = [](const char* path, const std::string& data)
        {
            std::ofstream out(path, std::ios::binary);
            out.write(data.data(), static_cast<std::streamsize>(data.size()));
        };
        const auto get = [](const char* path) -> std::string
        {
            std::ifstream in(path, std::ios::binary);
            return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        };
        put((directory + "/plain.txt").c_str(), "file compatibility");
        aes.EncryptTxtFile((directory + "/plain.txt").c_str(), (directory + "/encoded.txt").c_str());
        check(get((directory + "/encoded.txt").c_str()) == aes.EncryptString("file compatibility"), "AES text file encryption");
        aes.DecryptTxtFile((directory + "/encoded.txt").c_str(), (directory + "/decoded.txt").c_str());
        check(get((directory + "/decoded.txt").c_str()) == "file compatibility", "AES text file decryption");
        put((directory + "/decoded.txt").c_str(), "keep");
        put((directory + "/invalid.txt").c_str(), "A");
        aes.DecryptTxtFile((directory + "/invalid.txt").c_str(), (directory + "/decoded.txt").c_str());
        check(get((directory + "/decoded.txt").c_str()) == "keep", "AES malformed file preserves destination");
        aes.DecryptTxtFile((directory + "/missing.txt").c_str(), (directory + "/decoded.txt").c_str());
        check(get((directory + "/decoded.txt").c_str()) == "keep", "AES missing input preserves destination");
        AesEncryptor invalid(nullptr);
        invalid.DecryptTxtFile((directory + "/encoded.txt").c_str(), (directory + "/decoded.txt").c_str());
        check(get((directory + "/decoded.txt").c_str()) == "keep", "AES invalid key preserves destination");
        put((directory + "/encoded.txt").c_str(), aes.EncryptString(""));
        aes.DecryptTxtFile((directory + "/encoded.txt").c_str(), (directory + "/decoded.txt").c_str());
        check(get((directory + "/decoded.txt").c_str()).empty(), "AES valid empty file replaces destination");
        put((directory + "/plain.txt").c_str(), "in place");
        aes.EncryptTxtFile((directory + "/plain.txt").c_str(), (directory + "/plain.txt").c_str());
        aes.DecryptTxtFile((directory + "/plain.txt").c_str(), (directory + "/plain.txt").c_str());
        check(get((directory + "/plain.txt").c_str()) == "in place", "AES in place file operation");
        sm4_context context = {};
        unsigned char guarded[48]; memset(guarded, 0xa5, sizeof(guarded));
        unsigned char iv[16] = {};
        sm4_setkey_enc(nullptr, key); sm4_setkey_dec(nullptr, key);
        sm4_setkey_enc(&context, key);
        check(context.mode == SM4_ENCRYPT, "SM4 encrypt mode");
        for (int length = -1; length < 32; ++length)
        {
            if (length > 0 && length % 16 == 0)
            {
                continue;
            }
            sm4_crypt_ecb(&context, SM4_ENCRYPT, length, guarded + 16, guarded + 16);
            sm4_crypt_cbc(&context, SM4_ENCRYPT, length, iv, guarded + 16, guarded + 16);
            check(std::string(reinterpret_cast<char*>(guarded), sizeof(guarded)) == std::string(48, static_cast<char>(0xa5)), "SM4 incomplete block untouched");
        }
        sm4_setkey_dec(&context, key);
        check(context.mode == SM4_DECRYPT, "SM4 decrypt mode");
    }
    else
    {
        return 2;
    }
    printf("CASE %d checks=%d failed=%d\n", id, checks, failed);
    return failed ? 1 : 0;
}

void NetCryptoLegacyTest::regression()
{
    const int cases[] = {1, 2, 3, 4, 5, 7, 9};
    for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); ++index)
    {
        netTestCheckpoint();
        if (check(cases[index]) != 0)
        {
            throw std::runtime_error("legacy crypto regression");
        }
    }
}

void NetCryptoLegacyTest::benchmark()
{
    netTestCheckpoint();
    if (check(6) != 0)
    {
        throw std::runtime_error("legacy crypto benchmark");
    }
}

void NetCryptoLegacyTest::tls()
{
    netTestCheckpoint();
    if (check(8) != 0)
    {
        throw std::runtime_error("legacy crypto TLS");
    }
}

void NetCryptoLegacyTest::fileBoundary()
{
    netTestCheckpoint();
    if (check(11) != 0)
    {
        throw std::runtime_error("legacy crypto file boundary");
    }
}