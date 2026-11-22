#include "CEncodeDecodeCertificate.h"
#include "CEncodeDecode.h"
#include "CEncodeDecodeMbedConfig.h"
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecp.h>
#include <mbedtls/oid.h>
#include <mbedtls/pk.h>
#include <mbedtls/md.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#include <cstring>

/** 将安全随机回调适配为第三方错误码，异常不跨密码库边界
@param [in] context 本次运算的回调与上下文
@param [out] output 待填充缓冲
@param [in] size 字节数
@return 0完整成功，其他失败
*/
static int CertificateEntropyBytes(void* context, unsigned char* output, size_t size)
{
    CEncodeDecodeCertificateEntropy* source = static_cast<CEncodeDecodeCertificateEntropy*>(context);
    return source != nullptr && size <= UINT32_MAX && CEncodeDecode::entropyBytes(source->m_function,
        source->m_context, output, static_cast<uint32_t>(size)) ? 0 : -1;
}

/** 检查确切曲线，不能只凭256位接受另一条曲线
@param [in] key 已解析的公钥或私钥
@return 确为secp256r1
*/
static bool CertificateP256(const mbedtls_pk_context* key)
{
    return mbedtls_pk_get_type(key) == MBEDTLS_PK_ECKEY &&
        mbedtls_ecp_keypair_get_group_id(mbedtls_pk_ec(*key)) == MBEDTLS_ECP_DP_SECP256R1;
}

/** 验证文本长度及嵌入NUL，不把二进制尾随内容交给PEM解析
@param [in] text PEM文本
@param [in] maximum 允许最大字节数
@return 非空且没有嵌入NUL
*/
static bool CertificateText(const std::string& text, size_t maximum)
{
    return !text.empty() && text.size() <= maximum && text.find('\0') == std::string::npos;
}

/** 检查输出范围重叠和地址加法溢出，空指针由调用点另行验证
@param [in] first 第一段范围
@param [in] firstSize 第一段长度
@param [in] second 第二段范围
@param [in] secondSize 第二段长度
@return 重叠或任意地址范围不可表示时为true
*/
static bool CertificateRangesOverlap(const void* first, size_t firstSize, const void* second, size_t secondSize)
{
    if ((first != nullptr && firstSize != 0 && reinterpret_cast<uintptr_t>(first) > UINTPTR_MAX - (firstSize - 1)) ||
        (second != nullptr && secondSize != 0 && reinterpret_cast<uintptr_t>(second) > UINTPTR_MAX - (secondSize - 1)))
    {
        return true;
    }
    if (first == nullptr || second == nullptr || firstSize == 0 || secondSize == 0)
    {
        return false;
    }
    const uintptr_t firstAddress = reinterpret_cast<uintptr_t>(first);
    const uintptr_t secondAddress = reinterpret_cast<uintptr_t>(second);
    return firstAddress <= secondAddress ? secondAddress - firstAddress < firstSize : firstAddress - secondAddress < secondSize;
}

bool CEncodeDecodeCertificate::validDnsName(const uint8_t* name, size_t size)
{
    if (name == nullptr || size == 0 || size > 253)
    {
        return false;
    }
    size_t start = 0;
    for (size_t index = 0; index <= size; ++index)
    {
        if (index == size || name[index] == '.')
        {
            if (index == start || index - start > 63 || name[start] == '-' || name[index - 1] == '-')
            {
                return false;
            }
            start = index + 1;
        }
        else if (!((name[index] >= 'a' && name[index] <= 'z') || (name[index] >= 'A' && name[index] <= 'Z') || (name[index] >= '0' && name[index] <= '9') || name[index] == '-'))
        {
            return false;
        }
    }
    return true;
}

int32_t CEncodeDecodeCertificate::createRequest(const uint8_t* name, uint32_t nameSize,
    CEncodeDecodeEntropyFunction entropy, void* context, uint8_t* privateKey, uint32_t keyCapacity,
    uint32_t* keySize, uint8_t* request, uint32_t requestCapacity, uint32_t* requestSize)
{
    if (CertificateRangesOverlap(keySize, sizeof(uint32_t), requestSize, sizeof(uint32_t)) ||
        CertificateRangesOverlap(keySize, sizeof(uint32_t), privateKey, keyCapacity) ||
        CertificateRangesOverlap(keySize, sizeof(uint32_t), request, requestCapacity) ||
        CertificateRangesOverlap(keySize, sizeof(uint32_t), name, nameSize) ||
        CertificateRangesOverlap(requestSize, sizeof(uint32_t), privateKey, keyCapacity) ||
        CertificateRangesOverlap(requestSize, sizeof(uint32_t), request, requestCapacity) ||
        CertificateRangesOverlap(requestSize, sizeof(uint32_t), name, nameSize))
    {
        return CENCODE_ARGUMENT;
    }
    if (keySize != nullptr)
    {
        *keySize = 0;
    }
    if (requestSize != nullptr)
    {
        *requestSize = 0;
    }
    if (keySize == nullptr || requestSize == nullptr || privateKey == nullptr || request == nullptr ||
        keyCapacity == 0 || keyCapacity > 4096 || requestCapacity == 0 ||
        requestCapacity > 4096 || entropy == nullptr ||
        CertificateRangesOverlap(privateKey, keyCapacity, request, requestCapacity) ||
        CertificateRangesOverlap(privateKey, keyCapacity, name, nameSize) ||
        CertificateRangesOverlap(request, requestCapacity, name, nameSize) || !validDnsName(name, nameSize))
    {
        return CENCODE_ARGUMENT;
    }
    char subject[257] = {'C', 'N', '=', 0};
    std::memcpy(subject + 3, name, nameSize);
    mbedtls_ctr_drbg_context random;
    mbedtls_pk_context key;
    mbedtls_x509write_csr writer;
    mbedtls_ctr_drbg_init(&random);
    mbedtls_pk_init(&key);
    mbedtls_x509write_csr_init(&writer);
    CEncodeDecodeCertificateEntropy source = {entropy, context};
    const unsigned char personalization[] = "CEncodeDecode P256 request v1";
    unsigned char keyBytes[4096] = {};
    unsigned char requestBytes[4096] = {};
    int result = mbedtls_ctr_drbg_seed(&random, CertificateEntropyBytes, &source, personalization, sizeof(personalization) - 1);
    if (result == 0)
    {
        result = mbedtls_pk_setup(&key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    }
    if (result == 0)
    {
        result = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, mbedtls_pk_ec(key), mbedtls_ctr_drbg_random, &random);
    }
    if (result == 0)
    {
        result = mbedtls_pk_write_key_pem(&key, keyBytes, sizeof(keyBytes));
    }
    mbedtls_x509write_csr_set_key(&writer, &key);
    mbedtls_x509write_csr_set_md_alg(&writer, MBEDTLS_MD_SHA256);
    if (result == 0)
    {
        result = mbedtls_x509write_csr_set_subject_name(&writer, subject);
    }
    if (result == 0)
    {
        result = mbedtls_x509write_csr_pem(&writer, requestBytes, sizeof(requestBytes), mbedtls_ctr_drbg_random, &random);
    }
    if (result == 0)
    {
        *keySize = static_cast<uint32_t>(std::strlen(reinterpret_cast<const char*>(keyBytes))) + 1;
        *requestSize = static_cast<uint32_t>(std::strlen(reinterpret_cast<const char*>(requestBytes))) + 1;
        if (keyCapacity >= *keySize && requestCapacity >= *requestSize)
        {
            std::memcpy(privateKey, keyBytes, *keySize);
            std::memcpy(request, requestBytes, *requestSize);
        }
    }
    mbedtls_platform_zeroize(keyBytes, sizeof(keyBytes));
    mbedtls_x509write_csr_free(&writer);
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&random);
    return result != 0 ? CENCODE_STATE : keyCapacity < *keySize || requestCapacity < *requestSize ? CENCODE_BUFFER : CENCODE_OK;
}

int32_t CEncodeDecodeCertificate::signDigest(const std::string& privateKey, const uint8_t* digest,
    CEncodeDecodeEntropyFunction entropy, void* context, uint8_t* signature, uint32_t capacity, uint32_t* required)
{
    const size_t keySize = privateKey.size() + 1;
    if (CertificateRangesOverlap(required, sizeof(uint32_t), signature, capacity) ||
        CertificateRangesOverlap(required, sizeof(uint32_t), privateKey.c_str(), keySize) ||
        CertificateRangesOverlap(required, sizeof(uint32_t), digest, 32) ||
        CertificateRangesOverlap(signature, capacity, privateKey.c_str(), keySize) ||
        CertificateRangesOverlap(signature, capacity, digest, 32))
    {
        return CENCODE_ARGUMENT;
    }
    if (required != nullptr)
    {
        *required = 0;
    }
    if (!CertificateText(privateKey, 4096) || entropy == nullptr || digest == nullptr || required == nullptr ||
        signature == nullptr || capacity == 0 || capacity > 80)
    {
        return CENCODE_ARGUMENT;
    }
    mbedtls_pk_context key;
    mbedtls_ctr_drbg_context random;
    mbedtls_pk_init(&key);
    mbedtls_ctr_drbg_init(&random);
    CEncodeDecodeCertificateEntropy source = {entropy, context};
    const unsigned char personalization[] = "CEncodeDecode P256 sign v1";
    unsigned char bytes[80] = {};
    size_t length = 0;
    int result = mbedtls_ctr_drbg_seed(&random, CertificateEntropyBytes, &source, personalization, sizeof(personalization) - 1);
    if (result == 0)
    {
        result = mbedtls_pk_parse_key(&key, reinterpret_cast<const uint8_t*>(privateKey.c_str()), privateKey.size() + 1,
            nullptr, 0, mbedtls_ctr_drbg_random, &random);
    }
    if (result == 0 && !CertificateP256(&key))
    {
        result = -1;
    }
    if (result == 0)
    {
        result = mbedtls_pk_sign(&key, MBEDTLS_MD_SHA256, digest, 32, bytes, sizeof(bytes), &length,
            mbedtls_ctr_drbg_random, &random);
    }
    int32_t status = result == 0 ? CENCODE_OK : CENCODE_STATE;
    if (result == 0)
    {
        *required = static_cast<uint32_t>(length);
        if (capacity < length)
        {
            status = CENCODE_BUFFER;
        }
        else
        {
            std::memcpy(signature, bytes, length);
        }
    }
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&random);
    return status;
}

/** 检查包含NUL的借用文本，先限长与地址检查再访问
@param [in] value 字节范围
@param [in] maximum 包括NUL的上限
@return 非空文本且仅末字节是NUL
*/
static bool CertificatePemBytes(const CEncodeDecodeBytes& value, uint32_t maximum)
{
    return value.m_bytes != nullptr && value.m_size >= 2 && value.m_size <= maximum &&
        reinterpret_cast<uintptr_t>(value.m_bytes) <= UINTPTR_MAX - (value.m_size - 1) &&
        value.m_bytes[value.m_size - 1] == 0 && std::memchr(value.m_bytes, 0, value.m_size - 1) == nullptr;
}

int32_t CEncodeDecodeCertificate::verify(const std::string& certificate, const uint8_t* authority, uint32_t authoritySize,
    const std::string& name, const uint8_t* digest, const uint8_t* signature, uint32_t signatureSize)
{
    const bool proof = digest != nullptr || signature != nullptr || signatureSize != 0;
    const CEncodeDecodeBytes authorityText = {authority, authoritySize};
    if (!CertificateText(certificate, 16384) || !validDnsName(reinterpret_cast<const uint8_t*>(name.data()), name.size()) ||
        !CertificatePemBytes(authorityText, 16384) ||
        (proof && (digest == nullptr || signature == nullptr || signatureSize == 0 || signatureSize > 80)))
    {
        return CENCODE_ARGUMENT;
    }
    mbedtls_x509_crt leaf;
    mbedtls_x509_crt root;
    mbedtls_x509_crt_init(&leaf);
    mbedtls_x509_crt_init(&root);
    int result = mbedtls_x509_crt_parse(&root, authority, authoritySize);
    if (result == 0)
    {
        result = mbedtls_x509_crt_parse(&leaf, reinterpret_cast<const uint8_t*>(certificate.c_str()), certificate.size() + 1);
    }
    uint32_t flags = 0;
    if (result == 0 && (leaf.next != nullptr || leaf.MBEDTLS_PRIVATE(ca_istrue) != 0 || !CertificateP256(&leaf.pk) ||
        (leaf.MBEDTLS_PRIVATE(ext_types) & MBEDTLS_X509_EXT_KEY_USAGE) == 0 ||
        mbedtls_x509_crt_check_key_usage(&leaf, MBEDTLS_X509_KU_DIGITAL_SIGNATURE) != 0))
    {
        result = -1;
    }
    if (result == 0)
    {
        result = mbedtls_x509_crt_verify(&leaf, &root, nullptr, name.c_str(), &flags, nullptr, nullptr);
    }
    if (result == 0 && flags == 0 && proof)
    {
        result = mbedtls_pk_verify(&leaf.pk, MBEDTLS_MD_SHA256, digest, 32, signature, signatureSize);
    }
    mbedtls_x509_crt_free(&leaf);
    mbedtls_x509_crt_free(&root);
    return result == 0 && flags == 0 ? CENCODE_OK : CENCODE_STATE;
}

int32_t CEncodeDecodeCertificate::verifyPair(const std::string& certificate, const std::string& privateKey,
    CEncodeDecodeEntropyFunction entropy, void* context)
{
    if (!CertificateText(certificate, 16384) || !CertificateText(privateKey, 4096) || entropy == nullptr)
    {
        return CENCODE_ARGUMENT;
    }
    mbedtls_x509_crt leaf;
    mbedtls_pk_context key;
    mbedtls_ctr_drbg_context random;
    mbedtls_x509_crt_init(&leaf);
    mbedtls_pk_init(&key);
    mbedtls_ctr_drbg_init(&random);
    CEncodeDecodeCertificateEntropy source = {entropy, context};
    const unsigned char personalization[] = "CEncodeDecode P256 pair v1";
    int result = mbedtls_ctr_drbg_seed(&random, CertificateEntropyBytes, &source, personalization, sizeof(personalization) - 1);
    if (result == 0)
    {
        result = mbedtls_x509_crt_parse(&leaf, reinterpret_cast<const uint8_t*>(certificate.c_str()), certificate.size() + 1);
    }
    if (result == 0)
    {
        result = mbedtls_pk_parse_key(&key, reinterpret_cast<const uint8_t*>(privateKey.c_str()), privateKey.size() + 1,
            nullptr, 0, mbedtls_ctr_drbg_random, &random);
    }
    if (result == 0 && (leaf.next != nullptr || !CertificateP256(&leaf.pk) || !CertificateP256(&key)))
    {
        result = -1;
    }
    if (result == 0)
    {
        result = mbedtls_pk_check_pair(&leaf.pk, &key, mbedtls_ctr_drbg_random, &random);
    }
    mbedtls_pk_free(&key);
    mbedtls_x509_crt_free(&leaf);
    mbedtls_ctr_drbg_free(&random);
    return result == 0 ? CENCODE_OK : CENCODE_STATE;
}

/** 校验公历UTC时间，拒绝不存在的日期和越界时分秒
@param [in] value 含NUL的14位UTC时间
@return 固定格式、年份1至9999且公历日期有效
*/
static bool CertificateDateBytes(const CEncodeDecodeBytes& value)
{
    if (value.m_size != 15 || !CertificatePemBytes(value, 15))
    {
        return false;
    }
    for (uint32_t index = 0; index < 14; ++index)
    {
        if (value.m_bytes[index] < '0' || value.m_bytes[index] > '9')
        {
            return false;
        }
    }
    const uint8_t* text = value.m_bytes;
    const uint32_t year = (text[0] - '0') * 1000 + (text[1] - '0') * 100 + (text[2] - '0') * 10 + text[3] - '0';
    const uint32_t month = (text[4] - '0') * 10 + text[5] - '0';
    const uint32_t day = (text[6] - '0') * 10 + text[7] - '0';
    const uint32_t hour = (text[8] - '0') * 10 + text[9] - '0';
    const uint32_t minute = (text[10] - '0') * 10 + text[11] - '0';
    const uint32_t second = (text[12] - '0') * 10 + text[13] - '0';
    if (year == 0 || month == 0 || month > 12 || hour > 23 || minute > 59 || second > 59)
    {
        return false;
    }
    const uint32_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    return day != 0 && day <= days[month - 1] + (month == 2 && leap ? 1 : 0);
}

int32_t CEncodeDecodeCertificate::issue(const CEncodeDecodeCertificateIssue& options, uint8_t* certificate,
    uint32_t capacity, uint32_t* required)
{
    if (options.m_dnsNameCount > 8 || (options.m_dnsNameCount != 0 && options.m_dnsNames == nullptr) ||
        CertificateRangesOverlap(&options, sizeof(options), certificate, capacity) ||
        CertificateRangesOverlap(&options, sizeof(options), required, sizeof(uint32_t)) ||
        CertificateRangesOverlap(required, sizeof(uint32_t), certificate, capacity) ||
        CertificateRangesOverlap(options.m_dnsNames, options.m_dnsNameCount * sizeof(CEncodeDecodeBytes), certificate, capacity) ||
        CertificateRangesOverlap(options.m_dnsNames, options.m_dnsNameCount * sizeof(CEncodeDecodeBytes), required, sizeof(uint32_t)))
    {
        return CENCODE_ARGUMENT;
    }
    const CEncodeDecodeBytes inputs[] = {options.m_name, options.m_request, options.m_authority,
        options.m_privateKey, options.m_notBefore, options.m_notAfter};
    for (size_t index = 0; index < sizeof(inputs) / sizeof(inputs[0]); ++index)
    {
        if (CertificateRangesOverlap(inputs[index].m_bytes, inputs[index].m_size, certificate, capacity) ||
            CertificateRangesOverlap(inputs[index].m_bytes, inputs[index].m_size, required, sizeof(uint32_t)))
        {
            return CENCODE_ARGUMENT;
        }
    }
    for (uint32_t index = 0; index < options.m_dnsNameCount; ++index)
    {
        if (CertificateRangesOverlap(options.m_dnsNames[index].m_bytes, options.m_dnsNames[index].m_size, certificate, capacity) ||
            CertificateRangesOverlap(options.m_dnsNames[index].m_bytes, options.m_dnsNames[index].m_size, required, sizeof(uint32_t)))
        {
            return CENCODE_ARGUMENT;
        }
    }
    if (required != nullptr)
    {
        *required = 0;
    }
    if (required == nullptr || certificate == nullptr || capacity == 0 || capacity > 8192)
    {
        return CENCODE_ARGUMENT;
    }
    if (!CertificatePemBytes(options.m_name, 254) || !CertificatePemBytes(options.m_request, 4096) ||
        !CertificatePemBytes(options.m_authority, 16384) || !CertificatePemBytes(options.m_privateKey, 16384) ||
        !CertificateDateBytes(options.m_notBefore) || !CertificateDateBytes(options.m_notAfter) ||
        std::memcmp(options.m_notBefore.m_bytes, options.m_notAfter.m_bytes, 14) >= 0 || options.m_entropy == nullptr)
    {
        return CENCODE_ARGUMENT;
    }
    if (!validDnsName(options.m_name.m_bytes, options.m_name.m_size - 1))
    {
        return CENCODE_ARGUMENT;
    }
    for (uint32_t index = 0; index < options.m_dnsNameCount; ++index)
    {
        const CEncodeDecodeBytes& dns = options.m_dnsNames[index];
        if (!CertificatePemBytes(dns, 254) || !validDnsName(dns.m_bytes, dns.m_size - 1))
        {
            return CENCODE_ARGUMENT;
        }
    }
    char subject[257] = {'C', 'N', '=', 0};
    std::memcpy(subject + 3, options.m_name.m_bytes, options.m_name.m_size);
    mbedtls_ctr_drbg_context random;
    mbedtls_pk_context signer;
    mbedtls_x509_crt root;
    mbedtls_x509_csr csr;
    mbedtls_x509write_cert writer;
    mbedtls_ctr_drbg_init(&random);
    mbedtls_pk_init(&signer);
    mbedtls_x509_crt_init(&root);
    mbedtls_x509_csr_init(&csr);
    mbedtls_x509write_crt_init(&writer);
    CEncodeDecodeCertificateEntropy source = {options.m_entropy, options.m_entropyContext};
    const unsigned char personalization[] = "CEncodeDecode P256 issuer v1";

    unsigned char digest[32] = {};
    unsigned char serial[20] = {};
    unsigned char pem[8192] = {};
    char issuer[256] = {};
    mbedtls_x509_san_list names[9] = {};
    names[0].node.type = MBEDTLS_X509_SAN_DNS_NAME;
    names[0].node.san.unstructured_name.p = const_cast<unsigned char*>(options.m_name.m_bytes);
    names[0].node.san.unstructured_name.len = options.m_name.m_size - 1;
    for (uint32_t index = 0; index < options.m_dnsNameCount; ++index)
    {
        names[index].next = &names[index + 1];
        names[index + 1].node.type = MBEDTLS_X509_SAN_DNS_NAME;
        names[index + 1].node.san.unstructured_name.p = const_cast<unsigned char*>(options.m_dnsNames[index].m_bytes);
        names[index + 1].node.san.unstructured_name.len = options.m_dnsNames[index].m_size - 1;
    }
    const unsigned char serverUsage[] = {0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01};
    const unsigned char clientUsage[] = {0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x02};
    unsigned char usage[22] = {0x30, 0};
    if (options.m_tlsServer)
    {
        std::memcpy(usage + 2 + usage[1], serverUsage, sizeof(serverUsage));
        usage[1] += sizeof(serverUsage);
    }
    if (options.m_tlsClient)
    {
        std::memcpy(usage + 2 + usage[1], clientUsage, sizeof(clientUsage));
        usage[1] += sizeof(clientUsage);
    }
    int result = mbedtls_ctr_drbg_seed(&random, CertificateEntropyBytes, &source, personalization, sizeof(personalization) - 1);
    if (result == 0)
    {
        result = mbedtls_x509_csr_parse(&csr, options.m_request.m_bytes, options.m_request.m_size);
    }
    if (result == 0 && (!CertificateP256(&csr.pk) ||
        csr.MBEDTLS_PRIVATE(sig_md) != MBEDTLS_MD_SHA256 || csr.MBEDTLS_PRIVATE(sig_pk) != MBEDTLS_PK_ECDSA ||
        csr.subject.next != nullptr || csr.subject.oid.len != MBEDTLS_OID_SIZE(MBEDTLS_OID_AT_CN) ||
        std::memcmp(csr.subject.oid.p, MBEDTLS_OID_AT_CN, csr.subject.oid.len) != 0 || csr.subject.val.len != options.m_name.m_size - 1 ||
        std::memcmp(csr.subject.val.p, options.m_name.m_bytes, options.m_name.m_size - 1) != 0))
    {
        result = -1;
    }
    // CSR解析不验证签名，必须显式验证请求私钥持有证明。
    if (result == 0)
    {
        result = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), csr.cri.p, csr.cri.len, digest);
    }
    if (result == 0)
    {
        result = mbedtls_pk_verify(&csr.pk, MBEDTLS_MD_SHA256, digest, sizeof(digest),
            csr.MBEDTLS_PRIVATE(sig).p, csr.MBEDTLS_PRIVATE(sig).len);
    }
    if (result == 0)
    {
        result = mbedtls_x509_crt_parse(&root, options.m_authority.m_bytes, options.m_authority.m_size);
    }
    if (result == 0)
    {
        result = mbedtls_pk_parse_key(&signer, options.m_privateKey.m_bytes, options.m_privateKey.m_size, nullptr, 0, mbedtls_ctr_drbg_random, &random);
    }
    if (result == 0 && (root.next != nullptr || root.MBEDTLS_PRIVATE(ca_istrue) == 0 ||
        mbedtls_x509_crt_check_key_usage(&root, MBEDTLS_X509_KU_KEY_CERT_SIGN) != 0))
    {
        result = -1;
    }
    if (result == 0)
    {
        result = mbedtls_pk_check_pair(&root.pk, &signer, mbedtls_ctr_drbg_random, &random);
    }
    if (result == 0 && mbedtls_x509_dn_gets(issuer, sizeof(issuer), &root.subject) < 0)
    {
        result = -1;
    }
    if (result == 0)
    {
        result = mbedtls_ctr_drbg_random(&random, serial, sizeof(serial));
        serial[0] &= 0x7f;
        serial[0] |= 1;
    }
    mbedtls_x509write_crt_set_version(&writer, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&writer, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_subject_key(&writer, &csr.pk);
    mbedtls_x509write_crt_set_issuer_key(&writer, &signer);
    if (result == 0)
    {
        result = mbedtls_x509write_crt_set_subject_name(&writer, subject);
    }
    if (result == 0)
    {
        result = mbedtls_x509write_crt_set_issuer_name(&writer, issuer);
    }
    if (result == 0)
    {
        result = mbedtls_x509write_crt_set_validity(&writer, reinterpret_cast<const char*>(options.m_notBefore.m_bytes), reinterpret_cast<const char*>(options.m_notAfter.m_bytes));
    }
    if (result == 0)
    {
        result = mbedtls_x509write_crt_set_serial_raw(&writer, serial, sizeof(serial));
    }
    if (result == 0)
    {
        result = mbedtls_x509write_crt_set_basic_constraints(&writer, 0, -1);
    }
    if (result == 0)
    {
        result = mbedtls_x509write_crt_set_key_usage(&writer, MBEDTLS_X509_KU_DIGITAL_SIGNATURE);
    }
    if (result == 0)
    {
        result = mbedtls_x509write_crt_set_subject_alternative_name(&writer, names);
    }
    if (result == 0 && usage[1] != 0)
    {
        result = mbedtls_x509write_crt_set_extension(&writer, MBEDTLS_OID_EXTENDED_KEY_USAGE,
            MBEDTLS_OID_SIZE(MBEDTLS_OID_EXTENDED_KEY_USAGE), 0, usage, 2 + usage[1]);
    }
    if (result == 0)
    {
        result = mbedtls_x509write_crt_pem(&writer, pem, sizeof(pem), mbedtls_ctr_drbg_random, &random);
    }
    int32_t status = result == 0 ? CENCODE_OK : CENCODE_STATE;
    if (result == 0)
    {
        *required = static_cast<uint32_t>(std::strlen(reinterpret_cast<const char*>(pem))) + 1;
        if (capacity < *required)
        {
            status = CENCODE_BUFFER;
        }
        else
        {
            std::memcpy(certificate, pem, *required);
        }
    }
    mbedtls_platform_zeroize(digest, sizeof(digest));
    mbedtls_x509write_crt_free(&writer);
    mbedtls_x509_csr_free(&csr);
    mbedtls_x509_crt_free(&root);
    mbedtls_pk_free(&signer);
    mbedtls_ctr_drbg_free(&random);
    return status;
}