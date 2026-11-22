#include "RsaAlgorithm.h"
#include "CEncodeDecode.h"
#include <mbedtls/rsa.h>
#include <mbedtls/platform_util.h>
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#elif defined(__APPLE__)
#include <stdlib.h>
#elif defined(__linux__) && !defined(__EMSCRIPTEN__)
#include <cerrno>
#include <linux/random.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#if !defined(__EMSCRIPTEN__)
/** 用操作系统的密码学随机源为原生 RSA 操作提供种子
@param [in] context 保留的回调参数
@param [out] output 本次借用的缓冲区，由调用方在失败时擦除
@param [in] size 需要完整填充的字节数
@return 全部写入返回 0，系统随机源不可用或尚未就绪时返回 -1
*/
static int32_t NativeRsaEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
#if defined(_WIN32)
    return BCryptGenRandom(nullptr, output, size, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0 ? 0 : -1;
#elif defined(__APPLE__)
    arc4random_buf(output, size);
    return 0;
#elif defined(__linux__) && defined(SYS_getrandom)
    // 直接调用内核以兼容仍未通过 libc 导出 getrandom 的 Android 版本
    unsigned int interruptions = 0;
    while (size != 0)
    {
        const long bytes = syscall(SYS_getrandom, output, size, GRND_NONBLOCK);
        if (bytes < 0 && errno == EINTR && interruptions < 8)
        {
            ++interruptions;
            continue;
        }
        if (bytes <= 0 || static_cast<unsigned long>(bytes) > size)
        {
            return -1;
        }
        output += bytes;
        size -= static_cast<uint32_t>(bytes);
    }
    return 0;
#else
    return -1;
#endif
}
#endif

RsaAlgorithm::RsaAlgorithm(CEncodeDecodeEntropyFunction entropy, void* context) :
m_entropy(entropy),
m_entropyContext(context)
{
    mbedtls_pk_init(&m_key);
    mbedtls_ctr_drbg_init(&m_random);
#if !defined(__EMSCRIPTEN__)
    if (!m_entropy)
    {
        m_entropy = NativeRsaEntropy;
    }
#endif
}

RsaAlgorithm::~RsaAlgorithm()
{
    mbedtls_pk_free(&m_key);
    mbedtls_ctr_drbg_free(&m_random);
}

int RsaAlgorithm::entropyBytes(void* context, unsigned char* output, size_t size)
{
    RsaAlgorithm* self = static_cast<RsaAlgorithm*>(context);
    if (self && output && size <= UINT32_MAX && CEncodeDecode::entropyBytes(self->m_entropy, self->m_entropyContext,
        output, static_cast<uint32_t>(size)))
    {
        return 0;
    }
    return MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED;
}

bool RsaAlgorithm::seed()
{
    const unsigned char purpose[] = "CEncodeDecode RSA OAEP SHA256";
    return mbedtls_ctr_drbg_seed(&m_random, entropyBytes, this, purpose, sizeof(purpose) - 1) == 0;
}

bool RsaAlgorithm::parse(const std::string& pem, bool privateKey)
{
    if (pem.empty() || pem.size() > 16384 || pem.find('\0') != std::string::npos ||
        pem.compare(0, 11, "-----BEGIN ") != 0)
    {
        return false;
    }
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(pem.c_str());
    int result = privateKey ? mbedtls_pk_parse_key(&m_key, bytes, pem.size() + 1, nullptr, 0, mbedtls_ctr_drbg_random, &m_random) :
        mbedtls_pk_parse_public_key(&m_key, bytes, pem.size() + 1);
    if (result != 0 || mbedtls_pk_get_type(&m_key) != MBEDTLS_PK_RSA || mbedtls_pk_get_bitlen(&m_key) != 2048)
    {
        return false;
    }
    mbedtls_rsa_context* rsa = mbedtls_pk_rsa(m_key);
    if ((privateKey ? mbedtls_rsa_check_privkey(rsa) : mbedtls_rsa_check_pubkey(rsa)) != 0)
    {
        return false;
    }
    return mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256) == 0;
}

bool RsaAlgorithm::generate(std::string& publicKey, std::string& privateKey)
{
    if (!privateKey.empty())
    {
        mbedtls_platform_zeroize(&privateKey[0], privateKey.size());
    }
    publicKey.clear();
    privateKey.clear();
    if (&publicKey == &privateKey || !seed() || mbedtls_pk_setup(&m_key, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA)) != 0 ||
        mbedtls_rsa_gen_key(mbedtls_pk_rsa(m_key), mbedtls_ctr_drbg_random, &m_random, 2048, 65537) != 0)
    {
        return false;
    }
    unsigned char publicBytes[1024] = {};
    unsigned char privateBytes[4096] = {};
    bool success = false;
    if (mbedtls_pk_write_pubkey_pem(&m_key, publicBytes, sizeof(publicBytes)) == 0 &&
        mbedtls_pk_write_key_pem(&m_key, privateBytes, sizeof(privateBytes)) == 0)
    {
        try
        {
            publicKey.assign(reinterpret_cast<char*>(publicBytes));
            privateKey.assign(reinterpret_cast<char*>(privateBytes));
            success = true;
        }
        catch (...)
        {
            publicKey.clear();
            if (!privateKey.empty())
            {
                mbedtls_platform_zeroize(&privateKey[0], privateKey.size());
            }
            privateKey.clear();
        }
    }
    mbedtls_platform_zeroize(privateBytes, sizeof(privateBytes));
    return success;
}

std::string RsaAlgorithm::encrypt(const std::string& publicKey, const char* text)
{
    if (!text)
    {
        return "";
    }
    // OAEP-SHA256上限为256 - 2*32 - 2；有界扫描不对超长文本反复strlen
    size_t size = 0;
    while (size <= 190 && text[size] != 0)
    {
        ++size;
    }
    if (size > 190 || !seed() || !parse(publicKey, false))
    {
        return "";
    }
    unsigned char ciphertext[256] = {};
    if (mbedtls_rsa_rsaes_oaep_encrypt(mbedtls_pk_rsa(m_key), mbedtls_ctr_drbg_random, &m_random, nullptr, 0,
        size, reinterpret_cast<const unsigned char*>(text), ciphertext) != 0)
    {
        return "";
    }
    try
    {
        return std::string(reinterpret_cast<char*>(ciphertext), sizeof(ciphertext));
    }
    catch (...)
    {
        return "";
    }
}

std::string RsaAlgorithm::decrypt(const std::string& privateKey, const std::string& ciphertext)
{
    if (ciphertext.size() != 256 || !seed() || !parse(privateKey, true))
    {
        return "";
    }
    unsigned char plaintext[256] = {};
    size_t size = 0;
    std::string result;
    int status = mbedtls_rsa_rsaes_oaep_decrypt(mbedtls_pk_rsa(m_key), mbedtls_ctr_drbg_random, &m_random, nullptr, 0,
        &size, reinterpret_cast<const unsigned char*>(ciphertext.data()), plaintext, sizeof(plaintext));
    if (status == 0 && size <= 190)
    {
        try
        {
            result.assign(reinterpret_cast<char*>(plaintext), size);
        }
        catch (...)
        {
            result.clear();
        }
    }
    mbedtls_platform_zeroize(plaintext, sizeof(plaintext));
    return result;
}