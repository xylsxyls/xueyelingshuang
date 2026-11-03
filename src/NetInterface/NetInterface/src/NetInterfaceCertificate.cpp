#include "NetInterfaceCertificate.h"
#include "NetInterfaceCertificateHelper.h"
#include "NetInterfaceMacro.h"
#include "NetInterfaceCertificateEntropy.h"
#include "NetInterface.h"
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecp.h>
#include <mbedtls/oid.h>
#include <mbedtls/pk.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/sha256.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#include <cstring>

/** 固定域分隔防止身份挑战签名被当作其他协议签名使用
@param [in] data 挑战及声明
@param [in] size 字节数
@param [out] digest 32字节SHA256
@return 0成功，否则失败
*/
static int CertificateProofDigest(const uint8_t* data, uint32_t size, unsigned char* digest)
{
    const unsigned char domain[] = "NetInterface role proof v3";
    mbedtls_sha256_context hash;
    mbedtls_sha256_init(&hash);
    int result = mbedtls_sha256_starts(&hash, 0);
    if (result == 0)
    {
        result = mbedtls_sha256_update(&hash, domain, sizeof(domain));
    }
    if (result == 0)
    {
        result = mbedtls_sha256_update(&hash, data, size);
    }
    if (result == 0)
    {
        result = mbedtls_sha256_finish(&hash, digest);
    }
    mbedtls_sha256_free(&hash);
    return result;
}

bool NetInterfaceCertificate::validName(const std::string& name)
{
    if (name.empty() || name.size() > 96)
    {
        return false;
    }
    size_t start = 0;
    for (size_t index = 0; index <= name.size(); ++index)
    {
        if (index == name.size() || name[index] == '.')
        {
            if (index == start || index - start > 63 || name[start] == '-' || name[index - 1] == '-')
            {
                return false;
            }
            start = index + 1;
        }
        else if (!((name[index] >= 'a' && name[index] <= 'z') || (name[index] >= '0' && name[index] <= '9') || name[index] == '-'))
        {
            return false;
        }
    }
    return true;
}

int32_t NetInterfaceCertificate::createRequest(const std::string& name, NetInterfaceEntropyFunction entropy, void* context,
    std::string& privateKey, std::string& request)
{
    if (&privateKey == &request || &name == &privateKey || &name == &request)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (!privateKey.empty())
    {
        mbedtls_platform_zeroize(&privateKey[0], privateKey.size());
    }
    privateKey.clear();
    request.clear();
    if (name.size() > 96)
    {
        return NI_INVALID_ARGUMENT;
    }
    uint8_t keyBytes[NET_INTERFACE_IDENTITY_PEM_BYTES] = {};
    uint8_t requestBytes[NET_INTERFACE_IDENTITY_PEM_BYTES] = {};
    uint32_t keySize = 0;
    uint32_t requestSize = 0;
    int32_t result = NetInterface::certificateCreateRequest(reinterpret_cast<const uint8_t*>(name.data()),
        static_cast<uint32_t>(name.size()), entropy, context, keyBytes, sizeof(keyBytes), &keySize,
        requestBytes, sizeof(requestBytes), &requestSize);
    if (result == NI_OK)
    {
        try
        {
            privateKey.assign(reinterpret_cast<const char*>(keyBytes), keySize - 1);
            request.assign(reinterpret_cast<const char*>(requestBytes), requestSize - 1);
        }
        catch (...)
        {
            if (!privateKey.empty())
            {
                mbedtls_platform_zeroize(&privateKey[0], privateKey.size());
            }
            privateKey.clear();
            request.clear();
            result = NI_NO_MEMORY;
        }
    }
    mbedtls_platform_zeroize(keyBytes, sizeof(keyBytes));
    return result;
}

int32_t NetInterfaceCertificate::signProof(const std::string& privateKey, const uint8_t* data, uint32_t size,
    NetInterfaceEntropyFunction entropy, void* context, std::vector<uint8_t>& signature)
{
    signature.clear();
    if (privateKey.empty() || privateKey.size() > 4096 || privateKey.find('\0') != std::string::npos || entropy == nullptr ||
        data == nullptr || size == 0 || size > 16384)
    {
        return NI_INVALID_ARGUMENT;
    }
    mbedtls_pk_context key;
    mbedtls_ctr_drbg_context random;
    mbedtls_pk_init(&key);
    mbedtls_ctr_drbg_init(&random);
    NetInterfaceCertificateEntropy source = {entropy, context};
    const unsigned char personalization[] = "NetInterface role proof v3";
    unsigned char digest[32] = {};
    unsigned char bytes[80] = {};
    size_t length = 0;
    int result = mbedtls_ctr_drbg_seed(&random, NetInterfaceCertificateHelper::certificateEntropyBytes, &source, personalization, sizeof(personalization) - 1);
    if (result == 0)
    {
        result = mbedtls_pk_parse_key(&key, reinterpret_cast<const uint8_t*>(privateKey.c_str()), privateKey.size() + 1,
            nullptr, 0, mbedtls_ctr_drbg_random, &random);
    }
    if (result == 0 && (mbedtls_pk_get_type(&key) != MBEDTLS_PK_ECKEY || mbedtls_pk_get_bitlen(&key) != 256))
    {
        result = -1;
    }
    if (result == 0)
    {
        result = CertificateProofDigest(data, size, digest);
    }
    if (result == 0)
    {
        result = mbedtls_pk_sign(&key, MBEDTLS_MD_SHA256, digest, sizeof(digest), bytes, sizeof(bytes), &length,
            mbedtls_ctr_drbg_random, &random);
    }
    if (result == 0)
    {
        try
        {
            signature.assign(bytes, bytes + length);
        }
        catch (...)
        {
            result = -1;
        }
    }
    mbedtls_platform_zeroize(digest, sizeof(digest));
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&random);
    return result == 0 ? NI_OK : NI_PROTOCOL_ERROR;
}

#if 0 // CERTIFICATE_BUFFERS_UNIT_TEST_BEGIN
#include "NetInterface.h"
#include <cstdio>
#include <cstring>

/** 只给本地CSR单元提供确定性字节，不签发证书或连接网络
@param [in] context 三个uint32：调用数、失败模式、伪随机状态
@param [out] output 测试输入
@param [in] size 输入长度
@return 0，模式1返回失败，模式2抛异常
*/
static int32_t CertificateTestEntropy(void* context, uint8_t* output, uint32_t size)
{
    uint32_t* state = static_cast<uint32_t*>(context);
    ++state[0];
    for (uint32_t index = 0; index < size; ++index)
    {
        state[2] ^= state[2] << 13;
        state[2] ^= state[2] >> 17;
        state[2] ^= state[2] << 5;
        output[index] = static_cast<uint8_t>(state[2]);
    }
    if (state[1] == 2)
    {
        throw 1;
    }
    return state[1] == 1 ? -1 : 0;
}

/** 固定缓冲及失败输出；不跨DLL转交字符串的分配/释放权
@return 0通过，1失败
*/
int main()
{
    uint32_t assertions = 0;
#define CERTIFICATE_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    uint32_t entropy[3] = {0, 0, 0x72bc51a9};
    const uint8_t name[] = "service.101";
    uint8_t key[NET_INTERFACE_IDENTITY_PEM_BYTES + 1];
    uint8_t request[NET_INTERFACE_IDENTITY_PEM_BYTES + 1];
    std::memset(key, 0x5a, sizeof(key));
    std::memset(request, 0x6b, sizeof(request));
    uint32_t keySize = 99;
    uint32_t requestSize = 99;
    CERTIFICATE_CHECK(NetInterface::certificateCreateRequest(name, sizeof(name) - 1, CertificateTestEntropy, entropy,
        key, sizeof(key) - 1, &keySize, request, sizeof(request) - 1, &requestSize) == NI_OK);
    CERTIFICATE_CHECK(keySize > 1 && keySize <= NET_INTERFACE_IDENTITY_PEM_BYTES && key[keySize - 1] == 0 &&
        requestSize > 1 && requestSize <= NET_INTERFACE_IDENTITY_PEM_BYTES && request[requestSize - 1] == 0);
    CERTIFICATE_CHECK(std::strstr(reinterpret_cast<const char*>(key), "PRIVATE KEY-----") != nullptr &&
        std::strstr(reinterpret_cast<const char*>(request), "-----BEGIN CERTIFICATE REQUEST-----") != nullptr);
    CERTIFICATE_CHECK(key[NET_INTERFACE_IDENTITY_PEM_BYTES] == 0x5a && request[NET_INTERFACE_IDENTITY_PEM_BYTES] == 0x6b && entropy[0] != 0);
    NetInterface::wipe(key, sizeof(key));
    uint8_t overlongName[97];
    std::memset(overlongName, 'a', sizeof(overlongName));
    for (uint32_t mode = 0; mode < 9; ++mode)
    {
        std::memset(key, 0x5a, sizeof(key));
        std::memset(request, 0x6b, sizeof(request));
        keySize = requestSize = 99;
        const uint32_t before = entropy[0];
        const uint8_t invalid[] = "service..101";
        const int32_t result = NetInterface::certificateCreateRequest(mode == 0 ? nullptr : mode == 1 ? invalid : mode == 2 ? overlongName : name,
            mode == 1 ? sizeof(invalid) - 1 : mode == 2 ? 97 : sizeof(name) - 1,
            mode == 3 ? nullptr : CertificateTestEntropy, entropy, mode == 4 ? nullptr : key,
            mode == 5 ? 0 : mode == 6 ? sizeof(key) : sizeof(key) - 1, mode == 7 ? nullptr : &keySize,
            request, sizeof(request) - 1, mode == 8 ? nullptr : &requestSize);
        CERTIFICATE_CHECK(result == NI_INVALID_ARGUMENT && entropy[0] == before);
        CERTIFICATE_CHECK(key[0] == 0x5a && key[sizeof(key) - 1] == 0x5a && request[0] == 0x6b);
        CERTIFICATE_CHECK((mode == 7 || keySize == 0) && (mode == 8 || requestSize == 0));
    }
    for (uint32_t mode = 1; mode <= 2; ++mode)
    {
        entropy[1] = mode;
        keySize = requestSize = 99;
        CERTIFICATE_CHECK(NetInterface::certificateCreateRequest(name, sizeof(name) - 1, CertificateTestEntropy, entropy,
            key, sizeof(key) - 1, &keySize, request, sizeof(request) - 1, &requestSize) == NI_PROTOCOL_ERROR);
        CERTIFICATE_CHECK(keySize == 0 && requestSize == 0 && key[0] == 0x5a && request[0] == 0x6b);
    }
    entropy[1] = 0;
    CERTIFICATE_CHECK(NetInterface::certificateCreateRequest(name, sizeof(name) - 1, CertificateTestEntropy, entropy,
        key, 1, &keySize, request, sizeof(request) - 1, &requestSize) == NI_BUFFER_TOO_SMALL);
    CERTIFICATE_CHECK(keySize > 1 && requestSize > 1 && key[0] == 0x5a && request[0] == 0x6b);
    const uint32_t before = entropy[0];
    CERTIFICATE_CHECK(NetInterface::certificateCreateRequest(name, sizeof(name) - 1, CertificateTestEntropy, entropy,
        key, sizeof(key) - 1, &keySize, key, sizeof(key) - 1, &requestSize) == NI_INVALID_ARGUMENT && entropy[0] == before);
    keySize = 777;
    CERTIFICATE_CHECK(NetInterface::certificateCreateRequest(name, sizeof(name) - 1, CertificateTestEntropy, entropy,
        key, sizeof(key) - 1, &keySize, request, sizeof(request) - 1, &keySize) == NI_INVALID_ARGUMENT && keySize == 777);
    uint32_t overlap[NET_INTERFACE_IDENTITY_PEM_BYTES / sizeof(uint32_t)] = {};
    overlap[0] = 777;
    CERTIFICATE_CHECK(NetInterface::certificateCreateRequest(name, sizeof(name) - 1, CertificateTestEntropy, entropy,
        reinterpret_cast<uint8_t*>(overlap), sizeof(overlap), overlap, request, sizeof(request) - 1,
        &requestSize) == NI_INVALID_ARGUMENT && overlap[0] == 777);
    CERTIFICATE_CHECK(NetInterface::certificateCreateRequest(name, sizeof(name) - 1, CertificateTestEntropy, entropy,
        const_cast<uint8_t*>(name), sizeof(name), &keySize, request, sizeof(request) - 1,
        &requestSize) == NI_INVALID_ARGUMENT && entropy[0] == before);
    const std::string longName = std::string(63, 'a') + "." + std::string(32, 'b');
    CERTIFICATE_CHECK(NetInterface::certificateCreateRequest(reinterpret_cast<const uint8_t*>(longName.data()),
        static_cast<uint32_t>(longName.size()), CertificateTestEntropy, entropy, key, sizeof(key) - 1, &keySize,
        request, sizeof(request) - 1, &requestSize) == NI_OK);
    CERTIFICATE_CHECK(keySize > 1 && requestSize > 1 && key[keySize - 1] == 0 && request[requestSize - 1] == 0);
    NetInterface::wipe(key, sizeof(key));
    std::printf("Certificate fixed buffers assertions=%u PASS\n", assertions);
    return 0;
#undef CERTIFICATE_CHECK
}

#endif // CERTIFICATE_BUFFERS_UNIT_TEST_END

int32_t NetInterfaceCertificate::verifyProof(const std::string& certificate, const std::string& name, const uint8_t* data,
    uint32_t size, const uint8_t* signature, uint32_t signatureSize)
{
    if (data == nullptr || size == 0 || size > 16384 || signature == nullptr || signatureSize == 0 || signatureSize > 80)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t verified = verify(certificate, name);
    if (verified != NI_OK)
    {
        return verified;
    }
    mbedtls_x509_crt leaf;
    mbedtls_x509_crt_init(&leaf);
    unsigned char digest[32] = {};
    int result = mbedtls_x509_crt_parse(&leaf, reinterpret_cast<const uint8_t*>(certificate.c_str()), certificate.size() + 1);
    if (result == 0)
    {
        result = CertificateProofDigest(data, size, digest);
    }
    if (result == 0)
    {
        result = mbedtls_pk_verify(&leaf.pk, MBEDTLS_MD_SHA256, digest, sizeof(digest), signature, signatureSize);
    }
    mbedtls_platform_zeroize(digest, sizeof(digest));
    mbedtls_x509_crt_free(&leaf);
    return result == 0 ? NI_OK : NI_PROTOCOL_ERROR;
}

int32_t NetInterfaceCertificate::verify(const std::string& certificate, const std::string& name)
{
    if (!validName(name) || certificate.empty() || certificate.size() > 16384 || certificate.find('\0') != std::string::npos)
    {
        return NI_INVALID_ARGUMENT;
    }
    mbedtls_x509_crt leaf;
    mbedtls_x509_crt authority;
    mbedtls_x509_crt_init(&leaf);
    mbedtls_x509_crt_init(&authority);
    uint32_t size = 0;
    const uint8_t* pem = NetInterface::trustAuthority(&size);
    int result = mbedtls_x509_crt_parse(&authority, pem, size);
    if (result == 0)
    {
        result = mbedtls_x509_crt_parse(&leaf, reinterpret_cast<const uint8_t*>(certificate.c_str()), certificate.size() + 1);
    }
    uint32_t flags = 0;
    if (result == 0 && (leaf.next != nullptr || leaf.MBEDTLS_PRIVATE(ca_istrue) != 0 || mbedtls_pk_get_type(&leaf.pk) != MBEDTLS_PK_ECKEY ||
        mbedtls_pk_get_bitlen(&leaf.pk) != 256 || (leaf.MBEDTLS_PRIVATE(ext_types) & MBEDTLS_X509_EXT_KEY_USAGE) == 0 ||
        mbedtls_x509_crt_check_key_usage(&leaf, MBEDTLS_X509_KU_DIGITAL_SIGNATURE) != 0))
    {
        result = -1;
    }
    if (result == 0)
    {
        result = mbedtls_x509_crt_verify(&leaf, &authority, nullptr, name.c_str(), &flags, nullptr, nullptr);
    }
    mbedtls_x509_crt_free(&leaf);
    mbedtls_x509_crt_free(&authority);
    return result == 0 && flags == 0 ? NI_OK : NI_PROTOCOL_ERROR;
}

int32_t NetInterfaceCertificate::verifyPair(const std::string& certificate, const std::string& privateKey,
    const std::string& name, NetInterfaceEntropyFunction entropy, void* context)
{
    if (entropy == nullptr || privateKey.empty() || privateKey.size() > 4096 || privateKey.find('\0') != std::string::npos)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t verified = verify(certificate, name);
    if (verified != NI_OK)
    {
        return verified;
    }
    mbedtls_x509_crt leaf;
    mbedtls_pk_context key;
    mbedtls_ctr_drbg_context random;
    mbedtls_x509_crt_init(&leaf);
    mbedtls_pk_init(&key);
    mbedtls_ctr_drbg_init(&random);
    NetInterfaceCertificateEntropy source = {entropy, context};
    const unsigned char personalization[] = "NetInterface identity check v2";
    int result = mbedtls_ctr_drbg_seed(&random, NetInterfaceCertificateHelper::certificateEntropyBytes, &source, personalization, sizeof(personalization) - 1);
    if (result == 0)
    {
        result = mbedtls_x509_crt_parse(&leaf, reinterpret_cast<const uint8_t*>(certificate.c_str()), certificate.size() + 1);
    }
    if (result == 0)
    {
        result = mbedtls_pk_parse_key(&key, reinterpret_cast<const uint8_t*>(privateKey.c_str()), privateKey.size() + 1,
            nullptr, 0, mbedtls_ctr_drbg_random, &random);
    }
    if (result == 0)
    {
        result = mbedtls_pk_check_pair(&leaf.pk, &key, mbedtls_ctr_drbg_random, &random);
    }
    mbedtls_pk_free(&key);
    mbedtls_x509_crt_free(&leaf);
    mbedtls_ctr_drbg_free(&random);
    return result == 0 ? NI_OK : NI_PROTOCOL_ERROR;
}