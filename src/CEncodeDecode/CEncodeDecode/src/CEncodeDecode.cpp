#include "CEncodeDecode.h"
#include "CEncodeDecodeCertificate.h"
#include "CEncodeDecodeTls.h"
#include "CEncodeDecodeMbedConfig.h"
#include "CEncodeDecodeHash.h"
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/platform_util.h>
#include <new>
#include "MD5.h"
#include "aes_encryptor.h"
#include <vector>
#include <time.h>
#include <string.h>
#include "RsaAlgorithm.h"
#include <stdint.h>
#include <memory>
#include "sm2_cipher_error_codes.h"
#include "sm2_create_key_pair.h"
#include "sm2_encrypt_and_decrypt.h"
#include "sm4.h"
#ifdef USE_OPENSSL
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <algorithm>
#if (_MSC_VER == 1900)
#include <iterator>
#endif
#ifdef _WIN64
#pragma comment(lib, "libcrypto64.lib")
#pragma comment(lib, "libssl64.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#elif _WIN32
#pragma comment(lib, "libcrypto32.lib")
#pragma comment(lib, "libssl32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#endif
#endif

int32_t CEncodeDecode::p256IssueCertificate(const CEncodeDecodeCertificateIssue& options, uint8_t* certificate,
    uint32_t capacity, uint32_t* required)
{
    return CEncodeDecodeCertificate::issue(options, certificate, capacity, required);
}

int32_t CEncodeDecode::p256CreateRequest(const uint8_t* name, uint32_t nameSize, CEncodeDecodeEntropyFunction entropy,
    void* context, uint8_t* privateKey, uint32_t keyCapacity, uint32_t* keySize,
    uint8_t* request, uint32_t requestCapacity, uint32_t* requestSize)
{
    return CEncodeDecodeCertificate::createRequest(name, nameSize, entropy, context, privateKey, keyCapacity,
        keySize, request, requestCapacity, requestSize);
}

int32_t CEncodeDecode::p256SignDigest(const std::string& privateKey, const uint8_t* digest,
    CEncodeDecodeEntropyFunction entropy, void* context, uint8_t* signature, uint32_t capacity, uint32_t* required)
{
    return CEncodeDecodeCertificate::signDigest(privateKey, digest, entropy, context, signature, capacity, required);
}

int32_t CEncodeDecode::p256VerifyCertificate(const std::string& certificate, const uint8_t* authority, uint32_t authoritySize,
    const std::string& name, const uint8_t* digest, const uint8_t* signature, uint32_t signatureSize)
{
    return CEncodeDecodeCertificate::verify(certificate, authority, authoritySize, name, digest, signature, signatureSize);
}

int32_t CEncodeDecode::p256VerifyPair(const std::string& certificate, const std::string& privateKey,
    CEncodeDecodeEntropyFunction entropy, void* context)
{
    return CEncodeDecodeCertificate::verifyPair(certificate, privateKey, entropy, context);
}

int32_t CEncodeDecode::hashCreate(CEncodeDecodeHash** hash)
{
    if (hash == nullptr)
    {
        return CENCODE_ARGUMENT;
    }
    *hash = new (std::nothrow) CEncodeDecodeHash();
    if (*hash == nullptr)
    {
        return CENCODE_MEMORY;
    }
    if ((*hash)->m_status != CENCODE_OK)
    {
        delete *hash;
        *hash = nullptr;
        return CENCODE_STATE;
    }
    return CENCODE_OK;
}

int32_t CEncodeDecode::hashUpdate(CEncodeDecodeHash* hash, const uint8_t* input, uint32_t size)
{
    if (hash == nullptr || (input == nullptr && size != 0) || size > 16777216)
    {
        return CENCODE_ARGUMENT;
    }
    if (hash->m_finished)
    {
        return CENCODE_STATE;
    }
    if (hash->m_status != CENCODE_OK)
    {
        return hash->m_status;
    }
    if (hash->m_size > UINT64_C(0x1fffffffffffffff) - size)
    {
        hash->m_status = CENCODE_LIMIT;
        return hash->m_status;
    }
    if (size != 0 && mbedtls_sha256_update(&hash->m_context, input, size) != 0)
    {
        hash->m_status = CENCODE_STATE;
        return hash->m_status;
    }
    hash->m_size += size;
    return CENCODE_OK;
}

int32_t CEncodeDecode::hashFinish(CEncodeDecodeHash* hash, uint8_t* output, uint32_t capacity)
{
    if (hash == nullptr || output == nullptr)
    {
        return CENCODE_ARGUMENT;
    }
    if (hash->m_finished)
    {
        return CENCODE_STATE;
    }
    if (hash->m_status != CENCODE_OK)
    {
        return hash->m_status;
    }
    if (capacity < 32)
    {
        return CENCODE_BUFFER;
    }
    uint8_t digest[32] = {};
    const int result = mbedtls_sha256_finish(&hash->m_context, digest);
    if (result == 0)
    {
        memcpy(output, digest, sizeof(digest));
    }
    mbedtls_platform_zeroize(digest, sizeof(digest));
    hash->m_finished = true;
    hash->m_status = result == 0 ? CENCODE_OK : CENCODE_STATE;
    return hash->m_status;
}

void CEncodeDecode::hashDestroy(CEncodeDecodeHash* hash)
{
    delete hash;
}

int32_t CEncodeDecode::sha256(const uint8_t* input, uint32_t size, uint8_t* output, uint32_t capacity)
{
    if ((input == nullptr && size != 0) || output == nullptr || size > 16777216)
    {
        return CENCODE_ARGUMENT;
    }
    if (capacity < 32)
    {
        return CENCODE_BUFFER;
    }
    const uint8_t empty = 0;
    uint8_t digest[32] = {};
    const int result = mbedtls_sha256(input != nullptr ? input : &empty, size, digest, 0);
    if (result == 0)
    {
        memcpy(output, digest, sizeof(digest));
    }
    mbedtls_platform_zeroize(digest, sizeof(digest));
    return result == 0 ? CENCODE_OK : CENCODE_STATE;
}

int32_t CEncodeDecode::hmacSha256(const uint8_t* key, uint32_t keySize, const uint8_t* input, uint32_t size,
    uint8_t* output, uint32_t capacity)
{
    if ((key == nullptr && keySize != 0) || keySize > 16384 || (input == nullptr && size != 0) ||
        size > 16777216 || output == nullptr)
    {
        return CENCODE_ARGUMENT;
    }
    if (capacity < 32)
    {
        return CENCODE_BUFFER;
    }
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (info == nullptr)
    {
        return CENCODE_STATE;
    }
    const uint8_t empty = 0;
    uint8_t digest[32] = {};
    const int result = mbedtls_md_hmac(info, key != nullptr ? key : &empty, keySize,
        input != nullptr ? input : &empty, size, digest);
    if (result == 0)
    {
        memcpy(output, digest, sizeof(digest));
    }
    mbedtls_platform_zeroize(digest, sizeof(digest));
    return result == 0 ? CENCODE_OK : CENCODE_STATE;
}

/** 检查借用范围的空值与地址加法，不访问所指内存
@param [in] bytes 借用起始位置
@param [in] size 将访问的字节数
@return 空范围或地址加法可表示；不替代调用方保证内存可读写的责任
*/
static bool EncodeByteRange(const void* bytes, uint32_t size)
{
    return size == 0 || (bytes != nullptr && reinterpret_cast<uintptr_t>(bytes) <= UINTPTR_MAX - (size - 1));
}

/** 验证分片上限及累计正文长度，不在无效参数上启动密码运算
@param [in] parts 借用片段数组
@param [in] count 片段数
@return 参数有效且累计最多16MiB
*/
static bool EncodePartsValid(const CEncodeDecodeBytes* parts, uint32_t count)
{
    if (count > 16 || !EncodeByteRange(parts, count * static_cast<uint32_t>(sizeof(CEncodeDecodeBytes))))
    {
        return false;
    }
    uint32_t total = 0;
    for (uint32_t index = 0; index < count; ++index)
    {
        if (parts[index].m_size > 16777216 - total || !EncodeByteRange(parts[index].m_bytes, parts[index].m_size))
        {
            return false;
        }
        total += parts[index].m_size;
    }
    return true;
}

int32_t CEncodeDecode::sha256Parts(const CEncodeDecodeBytes* parts, uint32_t count, uint8_t* output, uint32_t capacity)
{
    if (!EncodeByteRange(output, 32) || !EncodePartsValid(parts, count))
    {
        return CENCODE_ARGUMENT;
    }
    if (capacity < 32)
    {
        return CENCODE_BUFFER;
    }
    uint8_t digest[32] = {};
    mbedtls_sha256_context hash;
    mbedtls_sha256_init(&hash);
    int result = mbedtls_sha256_starts(&hash, 0);
    for (uint32_t index = 0; result == 0 && index < count; ++index)
    {
        if (parts[index].m_size != 0)
        {
            result = mbedtls_sha256_update(&hash, parts[index].m_bytes, parts[index].m_size);
        }
    }
    if (result == 0)
    {
        result = mbedtls_sha256_finish(&hash, digest);
    }
    mbedtls_sha256_free(&hash);
    if (result == 0)
    {
        memcpy(output, digest, sizeof(digest));
    }
    mbedtls_platform_zeroize(digest, sizeof(digest));
    return result == 0 ? CENCODE_OK : CENCODE_STATE;
}

int32_t CEncodeDecode::hmacSha256Parts(const uint8_t* key, uint32_t keySize, const CEncodeDecodeBytes* parts,
    uint32_t count, uint8_t* output, uint32_t capacity)
{
    if (keySize > 16384 || !EncodeByteRange(key, keySize) || !EncodeByteRange(output, 32) || !EncodePartsValid(parts, count))
    {
        return CENCODE_ARGUMENT;
    }
    if (capacity < 32)
    {
        return CENCODE_BUFFER;
    }
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (info == nullptr)
    {
        return CENCODE_STATE;
    }
    const uint8_t empty = 0;
    uint8_t digest[32] = {};
    mbedtls_md_context_t hash;
    mbedtls_md_init(&hash);
    int result = mbedtls_md_setup(&hash, info, 1);
    if (result == 0)
    {
        result = mbedtls_md_hmac_starts(&hash, key != nullptr ? key : &empty, keySize);
    }
    for (uint32_t index = 0; result == 0 && index < count; ++index)
    {
        if (parts[index].m_size != 0)
        {
            result = mbedtls_md_hmac_update(&hash, parts[index].m_bytes, parts[index].m_size);
        }
    }
    if (result == 0)
    {
        result = mbedtls_md_hmac_finish(&hash, digest);
    }
    mbedtls_md_free(&hash);
    if (result == 0)
    {
        memcpy(output, digest, sizeof(digest));
    }
    mbedtls_platform_zeroize(digest, sizeof(digest));
    return result == 0 ? CENCODE_OK : CENCODE_STATE;
}

int32_t CEncodeDecode::pbkdf2Sha256(const uint8_t* password, uint32_t passwordSize, const uint8_t* salt, uint32_t saltSize,
    uint32_t iterations, uint8_t* output, uint32_t capacity)
{
    if (password == nullptr || passwordSize == 0 || passwordSize > 1024 || salt == nullptr || saltSize == 0 ||
        saltSize > 128 || iterations == 0 || iterations > 2000000 || output == nullptr)
    {
        return CENCODE_ARGUMENT;
    }
    if (capacity < 32)
    {
        return CENCODE_BUFFER;
    }
    uint8_t digest[32] = {};
    const int result = mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256, password, passwordSize, salt, saltSize,
        iterations, sizeof(digest), digest);
    if (result == 0)
    {
        memcpy(output, digest, sizeof(digest));
    }
    mbedtls_platform_zeroize(digest, sizeof(digest));
    return result == 0 ? CENCODE_OK : CENCODE_STATE;
}

void CEncodeDecode::wipe(uint8_t* bytes, uint32_t size)
{
    if (bytes != nullptr && size != 0)
    {
        mbedtls_platform_zeroize(bytes, size);
    }
}

CEncodeDecodeTls* CEncodeDecode::createTls(const CEncodeDecodeTlsOptions& options)
{
    return new (std::nothrow) CEncodeDecodeTls(options);
}

CEncodeDecodeTls* CEncodeDecode::createTls(const CEncodeDecodeTlsOptions& options, const CEncodeDecodeTlsQueueLimits& limits)
{
    return new (std::nothrow) CEncodeDecodeTls(options, limits);
}

void CEncodeDecode::destroyTls(CEncodeDecodeTls* tls)
{
    delete tls;
}

bool CEncodeDecode::entropyBytes(CEncodeDecodeEntropyFunction entropy, void* context, uint8_t* output, uint32_t size)
{
    if ((!output && size != 0) || size > 8192)
    {
        return false;
    }
    if (size == 0)
    {
        return true;
    }
    try
    {
        if (entropy && entropy(context, output, size) == 0)
        {
            return true;
        }
    }
    catch (...)
    {
        // 部分随机输出或回调异常必须失败，不越过第三方C调用栈
    }
    mbedtls_platform_zeroize(output, size);
    return false;
}

std::string CEncodeDecode::MD5Encode(const std::string& src)
{
	try
    {
        return MD5(src).toString();
    }
    catch (...)
    {
        return "";
    }
}

std::string CEncodeDecode::AESEncode(const std::string& key, const std::string& src)
{
    unsigned char normalized[16] = {};
    size_t size = key.size() < sizeof(normalized) ? key.size() : sizeof(normalized);
    memcpy(normalized, key.data(), size);
    AesEncryptor aes(normalized);
    mbedtls_platform_zeroize(normalized, sizeof(normalized));
    return aes.EncryptString(src);
}

std::string CEncodeDecode::AESDecode(const std::string& key, const std::string& src)
{
    unsigned char normalized[16] = {};
    size_t size = key.size() < sizeof(normalized) ? key.size() : sizeof(normalized);
    memcpy(normalized, key.data(), size);
    AesEncryptor aes(normalized);
    mbedtls_platform_zeroize(normalized, sizeof(normalized));
    return aes.DecryptString(src);
}

bool CEncodeDecode::RSAKey(std::string& publicKey, std::string& privateKey,
    CEncodeDecodeEntropyFunction entropy, void* context)
{
    RsaAlgorithm rsa(entropy, context);
    return rsa.generate(publicKey, privateKey);
}

std::string CEncodeDecode::RSAEncode(const std::string& publicKey, const char* src,
    CEncodeDecodeEntropyFunction entropy, void* context)
{
    RsaAlgorithm rsa(entropy, context);
    return rsa.encrypt(publicKey, src);
}

std::string CEncodeDecode::RSADecode(const std::string& privateKey, const std::string& src,
    CEncodeDecodeEntropyFunction entropy, void* context)
{
    RsaAlgorithm rsa(entropy, context);
    return rsa.decrypt(privateKey, src);
}

bool CEncodeDecode::SM2Key(std::string& publicKeyX, std::string& publicKeyY, std::string& privateKey, bool hasPrefix)
{
    if (!privateKey.empty())
    {
        mbedtls_platform_zeroize(&privateKey[0], privateKey.size());
    }
    publicKeyX.clear();
    publicKeyY.clear();
    privateKey.clear();
#ifdef USE_OPENSSL
    if (&publicKeyX == &publicKeyY || &publicKeyX == &privateKey || &publicKeyY == &privateKey)
    {
        return false;
    }
    SM2_KEY_PAIR key = {};
    bool success = false;
    if (sm2_create_key_pair(&key) == 0)
    {
        try
        {
            publicKeyX.assign(reinterpret_cast<char*>(key.pub_key) + (hasPrefix ? 0 : 1), hasPrefix ? 33 : 32);
            publicKeyY.assign(reinterpret_cast<char*>(key.pub_key) + 33, 32);
            privateKey.assign(reinterpret_cast<char*>(key.pri_key), 32);
            success = true;
        }
        catch (...)
        {
            publicKeyX.clear();
            publicKeyY.clear();
            privateKey.clear();
        }
    }
    OPENSSL_cleanse(&key, sizeof(key));
    return success;
#else
    return false;
#endif
}

bool CEncodeDecode::SM2Encode(const std::string& publicKeyX, const std::string& publicKeyY,
    const std::string& src, std::string& c1, std::string& c2, std::string& c3)
{
    // 输出别名会在清空时破坏输入，因此在读取或写入之前拒绝所有别名
    if (&c1 == &c2 || &c1 == &c3 || &c2 == &c3 ||
        &c1 == &publicKeyX || &c2 == &publicKeyX || &c3 == &publicKeyX ||
        &c1 == &publicKeyY || &c2 == &publicKeyY || &c3 == &publicKeyY ||
        &c1 == &src || &c2 == &src || &c3 == &src)
    {
        return false;
    }
    c1.clear();
    c2.clear();
    c3.clear();
#ifdef USE_OPENSSL
    if ((publicKeyX.size() != 32 && publicKeyX.size() != 33) || publicKeyY.size() != 32 ||
        (publicKeyX.size() == 33 && publicKeyX[0] != 4) || src.empty() || src.size() > INT32_MAX)
    {
        return false;
    }
    try
    {
        unsigned char key[65] = {4};
        memcpy(key + 1, publicKeyX.data() + (publicKeyX.size() == 33 ? 1 : 0), 32);
        memcpy(key + 33, publicKeyY.data(), 32);
        c1.resize(65);
        c2.resize(src.size());
        c3.resize(32);
        if (sm2_encrypt_code(reinterpret_cast<const unsigned char*>(src.data()), static_cast<int>(src.size()),
            key, reinterpret_cast<unsigned char*>(&c1[0]), reinterpret_cast<unsigned char*>(&c3[0]),
            reinterpret_cast<unsigned char*>(&c2[0])) == 0)
        {
            if (publicKeyX.size() == 32)
            {
                c1.erase(0, 1);
            }
            return true;
        }
    }
    catch (...)
    {
        // 失败不能把半成品作为完整密文返回
    }
    c1.clear();
    c2.clear();
    c3.clear();
#endif
    return false;
}

std::string CEncodeDecode::SM2Decode(const std::string& privateKey, const std::string& c1,
    const std::string& c2, const std::string& c3)
{
#ifdef USE_OPENSSL
    if ((c1.size() != 65 && c1.size() != 64) || (c1.size() == 65 && c1[0] != 4) ||
        c3.size() != 32 || c2.empty() || c2.size() > INT32_MAX || privateKey.size() != 32)
    {
        return "";
    }
    try
    {
        unsigned char point[65] = {4};
        memcpy(point + 1, c1.data() + (c1.size() == 65 ? 1 : 0), 64);
        std::string result(c2.size(), '\0');
        if (sm2_decrypt_code(point, reinterpret_cast<const unsigned char*>(c3.data()),
            reinterpret_cast<const unsigned char*>(c2.data()), static_cast<int>(c2.size()),
            reinterpret_cast<const unsigned char*>(privateKey.data()), reinterpret_cast<unsigned char*>(&result[0])) == 0)
        {
            return result;
        }
        OPENSSL_cleanse(&result[0], result.size());
    }
    catch (...)
    {
        // 认证或分配失败不返回部分明文
    }
#endif
    return "";
}

std::string CEncodeDecode::SM4Encode(const std::string& key, const std::string& src)
{
    if (key.size() != 16 || src.empty() || src.size() % 16 != 0)
    {
        return "";
    }
    try
    {
        std::string result(src.size(), '\0');
        sm4_context context;
        sm4_setkey_enc(&context, reinterpret_cast<unsigned char*>(const_cast<char*>(key.data())));
        for (size_t i = 0; i < src.size(); i += 16)
        {
            sm4_crypt_ecb(&context, SM4_ENCRYPT, 16, reinterpret_cast<unsigned char*>(const_cast<char*>(src.data()) + i),
                reinterpret_cast<unsigned char*>(&result[i]));
        }
        mbedtls_platform_zeroize(&context, sizeof(context));
        return result;
    }
    catch (...)
    {
        return "";
    }
}

std::string CEncodeDecode::SM4Decode(const std::string& key, const std::string& ciphertext)
{
    if (key.size() != 16 || ciphertext.empty() || ciphertext.size() % 16 != 0)
    {
        return "";
    }
    try
    {
        std::string result(ciphertext.size(), '\0');
        sm4_context context;
        sm4_setkey_dec(&context, reinterpret_cast<unsigned char*>(const_cast<char*>(key.data())));
        for (size_t i = 0; i < ciphertext.size(); i += 16)
        {
            sm4_crypt_ecb(&context, SM4_DECRYPT, 16, reinterpret_cast<unsigned char*>(const_cast<char*>(ciphertext.data()) + i),
                reinterpret_cast<unsigned char*>(&result[i]));
        }
        mbedtls_platform_zeroize(&context, sizeof(context));
        return result;
    }
    catch (...)
    {
        return "";
    }
}

X509* CEncodeDecode::getCert(const std::string& path)
{
#ifdef USE_OPENSSL
    if (path.empty() || path.find('\0') != std::string::npos)
    {
        return nullptr;
    }
    BIO* input = BIO_new_file(path.c_str(), "rb");
    if (!input)
    {
        return nullptr;
    }
    X509* certificate = d2i_X509_bio(input, nullptr);
    if (!certificate && BIO_reset(input) == 0)
    {
        certificate = PEM_read_bio_X509(input, nullptr, nullptr, nullptr);
    }
    BIO_free(input);
    return certificate;
#else
    return nullptr;
#endif
}

void CEncodeDecode::releaseCert(X509* cert)
{
#ifdef USE_OPENSSL
    X509_free(cert);
#endif
}

std::string CEncodeDecode::getPublicKey(X509* cert)
{
#ifdef USE_OPENSSL
    if (!cert)
    {
        return "";
    }
    EVP_PKEY* key = X509_get_pubkey(cert);
    if (!key)
    {
        return "";
    }
    const EC_KEY* ec = EVP_PKEY_get0_EC_KEY(key);
    unsigned char* bytes = nullptr;
    size_t size = ec ? EC_KEY_key2buf(ec, EC_KEY_get_conv_form(ec), &bytes, nullptr) : 0;
    std::string result;
    if (bytes && size != 0)
    {
        try
        {
            result.assign(reinterpret_cast<char*>(bytes), size);
        }
        catch (...)
        {
            result.clear();
        }
    }
    OPENSSL_free(bytes);
    EVP_PKEY_free(key);
    return result;
#else
    return "";
#endif
}

// 下列示例已用 Windows x64 Release 单独编译运行。
// #include "CEncodeDecode/CEncodeDecodeAPI.h"
// #ifdef __EMSCRIPTEN__
// #include <unistd.h>
// #endif
// #include <cstring>
// #include <memory>
// #include <string>
//
// int main()
// {
//     const uint8_t message[] = {'a', 'b', 'c'};
//     const CEncodeDecodeBytes parts[] = {{message, 1}, {message + 1, 2}};
//     uint8_t digest[32], partial[32];
//     // 固定数据仅用于演示 HMAC 和 PBKDF2，应用应提供自己的密钥和盐
//     uint8_t sampleKey[32] = {1, 2, 3, 4};
//     if (CEncodeDecode::sha256(message, 3, digest, 32) != CENCODE_OK ||
//         CEncodeDecode::sha256Parts(parts, 2, partial, 32) != CENCODE_OK || std::memcmp(digest, partial, 32)) { return 1; }
//     CEncodeDecodeHash* raw = nullptr;
//     if (CEncodeDecode::hashCreate(&raw) != CENCODE_OK) { return 1; }
//     std::unique_ptr<CEncodeDecodeHash, void (*)(CEncodeDecodeHash*)> stream(raw, CEncodeDecode::hashDestroy);
//     if (CEncodeDecode::hashUpdate(raw, message, 3) != CENCODE_OK ||
//         CEncodeDecode::hashFinish(raw, partial, 32) != CENCODE_OK || std::memcmp(digest, partial, 32)) { return 1; }
//     // 此调用在原生端使用系统随机源，Wasm 应传入宿主强随机回调。
// #ifdef __EMSCRIPTEN__
//     // Emscripten 的 getentropy 从宿主获取强随机字节，单次调用最多填充 256 字节。
//     const CEncodeDecodeEntropyFunction entropy = [](void*, uint8_t* out, uint32_t count) -> int32_t {
//         while (count) {
//             const uint32_t chunk = count > 256 ? 256 : count;
//             if (getentropy(out, chunk) != 0) { return -1; }
//             out += chunk;
//             count -= chunk;
//         }
//         return 0;
//     };
// #else
//     const CEncodeDecodeEntropyFunction entropy = nullptr;
// #endif
//     if (CEncodeDecode::hmacSha256(sampleKey, 32, message, 3, digest, 32) != CENCODE_OK ||
//         CEncodeDecode::hmacSha256Parts(sampleKey, 32, parts, 2, partial, 32) != CENCODE_OK || std::memcmp(digest, partial, 32)) { return 1; }
//     if (CEncodeDecode::pbkdf2Sha256(message, 3, sampleKey, 16, 10000, partial, 32) != CENCODE_OK) { return 1; }
//     std::string publicKey, privateKey;
//     if (!CEncodeDecode::RSAKey(publicKey, privateKey, entropy, nullptr)) { return 1; }
//     const std::string cipher = CEncodeDecode::RSAEncode(publicKey, "sample", entropy, nullptr);
//     const bool verified = CEncodeDecode::RSADecode(privateKey, cipher, entropy, nullptr) == "sample";
//     CEncodeDecode::wipe(reinterpret_cast<uint8_t*>(&privateKey[0]), static_cast<uint32_t>(privateKey.size()));
//     // 历史格式的往返检查。网关通信使用有证书验证的 TLS 通道。
//     std::string legacyKey(reinterpret_cast<const char*>(sampleKey), 16);
//     const std::string block("0123456789abcdef");
//     const bool compatible = CEncodeDecode::MD5Encode("abc") == "900150983cd24fb0d6963f7d28e17f72" &&
//         CEncodeDecode::AESDecode(legacyKey, CEncodeDecode::AESEncode(legacyKey, block)) == block &&
//         CEncodeDecode::SM4Decode(legacyKey, CEncodeDecode::SM4Encode(legacyKey, block)) == block;
//     CEncodeDecode::wipe(sampleKey, sizeof(sampleKey));
//     CEncodeDecode::wipe(reinterpret_cast<uint8_t*>(&legacyKey[0]), 16);
//     CEncodeDecode::wipe(partial, sizeof(partial));
//     return verified && compatible ? 0 : 1;
// }