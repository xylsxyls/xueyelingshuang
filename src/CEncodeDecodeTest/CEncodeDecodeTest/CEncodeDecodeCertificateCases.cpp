#include "CEncodeDecodeCertificateCases.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include "CEncodeDecode/CEncodeDecodeMbedConfig.h"
#include "CEncodeDecodeTest.h"
// 第三方只作为测试的独立验证器，独立检查本库生成的证书。
#include <mbedtls/pk.h>
#include <mbedtls/oid.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/x509_csr.h>
#include <mbedtls/ecp.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>

/** 固定错误信息的断言，避免向日志输出私钥
@param [in] condition 预期条件
@param [in] step 固定步骤名
*/
static void CertificateCryptoCheck(bool condition, const char* step)
{
    if (!condition)
    {
        throw std::runtime_error(step);
    }
}

/** 模拟随机源失败/异常，正常分支使用可复现的测试字节
@param [in] context 可空int指针，1失败，2抛异常
@param [out] output 随机字节
@param [in] size 字节数
@return 0完整成功
*/
static int32_t CertificateCryptoEntropy(void* context, uint8_t* output, uint32_t size)
{
    if (context != nullptr && *static_cast<int*>(context) == 2)
    {
        throw std::runtime_error("controlled entropy failure");
    }
    if (context != nullptr && *static_cast<int*>(context) == 1)
    {
        return -1;
    }
    return cEncodeTestEntropy(nullptr, output, size);
}

/** 第三方测试验证器的随机适配
@param [in] context 随机失败模式
@param [out] output 随机缓冲
@param [in] size 字节数
@return 0完整成功
*/
static int CertificateCryptoRandom(void* context, unsigned char* output, size_t size)
{
    return size <= UINT32_MAX ? CertificateCryptoEntropy(context, output, static_cast<uint32_t>(size)) : -1;
}

/** 本次测试临时生成CA证书，第三方直接签发以独立检查被测库
@param [in] keyPem 已生成的P-256测试私钥PEM
@param [in] keySize 包括NUL的长度
@param [out] output 4096字节CA PEM
@return 第三方操作成功
*/
static bool CertificateCryptoRoot(const uint8_t* keyPem, uint32_t keySize, uint8_t* output)
{
    mbedtls_pk_context key;
    mbedtls_x509write_cert writer;
    mbedtls_pk_init(&key);
    mbedtls_x509write_crt_init(&writer);
    int result = mbedtls_pk_parse_key(&key, keyPem, keySize, nullptr, 0, CertificateCryptoRandom, nullptr);
    mbedtls_x509write_crt_set_version(&writer, MBEDTLS_X509_CRT_VERSION_3);
    mbedtls_x509write_crt_set_md_alg(&writer, MBEDTLS_MD_SHA256);
    mbedtls_x509write_crt_set_subject_key(&writer, &key);
    mbedtls_x509write_crt_set_issuer_key(&writer, &key);
    if (result == 0)
    {
        result = mbedtls_x509write_crt_set_subject_name(&writer, "CN=temporary.test.root");
    }
    if (result == 0)
    {
        result = mbedtls_x509write_crt_set_issuer_name(&writer, "CN=temporary.test.root");
    }
    if (result == 0)
    {
        result = mbedtls_x509write_crt_set_validity(&writer, "20200101000000", "20991231235959");
    }
    if (result == 0)
    {
        uint8_t serial[] = {1, 2, 3, 4};
        result = mbedtls_x509write_crt_set_serial_raw(&writer, serial, sizeof(serial));
    }
    if (result == 0)
    {
        result = mbedtls_x509write_crt_set_basic_constraints(&writer, 1, 0);
    }
    if (result == 0)
    {
        result = mbedtls_x509write_crt_set_key_usage(&writer, MBEDTLS_X509_KU_KEY_CERT_SIGN | MBEDTLS_X509_KU_CRL_SIGN);
    }
    if (result == 0)
    {
        result = mbedtls_x509write_crt_pem(&writer, output, 4096, CertificateCryptoRandom, nullptr);
    }
    mbedtls_x509write_crt_free(&writer);
    mbedtls_pk_free(&key);
    return result == 0;
}

/** 仅对借用的NUL文本生成测试描述符
@param [in] text 本次测试内有效的文本
@return 包括NUL的借用范围
*/
static CEncodeDecodeBytes CertificateCryptoText(const char* text)
{
    CEncodeDecodeBytes bytes = {reinterpret_cast<const uint8_t*>(text), static_cast<uint32_t>(std::strlen(text)) + 1};
    return bytes;
}

void CEncodeDecodeCertificateCases::run()
{
    uint8_t rootKey[4096] = {};
    uint8_t rootRequest[4096] = {};
    uint8_t root[4096] = {};
    uint8_t key[4096] = {};
    uint8_t request[4096] = {};
    uint8_t pem[8193];
    uint8_t signature[81];
    uint8_t digest[32] = {};
    const uint8_t name[] = "service.crypto.test";
    uint32_t rootKeySize = 0;
    uint32_t rootRequestSize = 0;
    uint32_t keySize = 0;
    uint32_t requestSize = 0;
    CertificateCryptoCheck(CEncodeDecode::p256CreateRequest(name, sizeof(name) - 1, CertificateCryptoEntropy, nullptr,
        rootKey, sizeof(rootKey), &rootKeySize, rootRequest, sizeof(rootRequest), &rootRequestSize) == CENCODE_OK,
        "temporary CA key generation");
    CertificateCryptoCheck(CertificateCryptoRoot(rootKey, rootKeySize, root), "independent temporary CA signing");
    CertificateCryptoCheck(CEncodeDecode::p256CreateRequest(name, sizeof(name) - 1, CertificateCryptoEntropy, nullptr,
        key, sizeof(key), &keySize, request, sizeof(request), &requestSize) == CENCODE_OK, "P256 leaf request");
    const std::string privateKey(reinterpret_cast<const char*>(key), keySize - 1);
    const CEncodeDecodeBytes alias = CertificateCryptoText("peer.crypto.test");
    CEncodeDecodeCertificateIssue options = {};
    options.m_name = CertificateCryptoText(reinterpret_cast<const char*>(name));
    options.m_request.m_bytes = request;
    options.m_request.m_size = requestSize;
    options.m_authority = CertificateCryptoText(reinterpret_cast<const char*>(root));
    options.m_privateKey.m_bytes = rootKey;
    options.m_privateKey.m_size = rootKeySize;
    options.m_notBefore = CertificateCryptoText("20200101000000");
    options.m_notAfter = CertificateCryptoText("20991231235959");
    options.m_dnsNames = &alias;
    options.m_dnsNameCount = 1;
    options.m_tlsServer = true;
    options.m_tlsClient = true;
    options.m_entropy = CertificateCryptoEntropy;
    std::memset(pem, 0x5a, sizeof(pem));
    uint32_t length = 999;
    CertificateCryptoCheck(CEncodeDecode::p256IssueCertificate(options, pem, 1, &length) == CENCODE_BUFFER &&
        length > 1 && length <= 8192 && pem[0] == 0x5a && pem[8192] == 0x5a, "issuer small buffer atomicity");
    CertificateCryptoCheck(CEncodeDecode::p256IssueCertificate(options, pem, 8192, &length) == CENCODE_OK &&
        length > 1 && pem[length - 1] == 0 && pem[8192] == 0x5a, "issuer bounded PEM result");
    const std::string certificate(reinterpret_cast<const char*>(pem), length - 1);
    CertificateCryptoCheck(CEncodeDecode::p256VerifyCertificate(certificate, options.m_authority.m_bytes,
        options.m_authority.m_size, reinterpret_cast<const char*>(name)) == CENCODE_OK, "leaf trust and name");
    CertificateCryptoCheck(CEncodeDecode::p256VerifyCertificate(certificate, options.m_authority.m_bytes,
        options.m_authority.m_size, "peer.crypto.test") == CENCODE_OK, "leaf additional SAN");
    CertificateCryptoCheck(CEncodeDecode::p256VerifyCertificate(certificate, options.m_authority.m_bytes,
        options.m_authority.m_size, "unknown.crypto.test") != CENCODE_OK, "wrong name accepted");
    CertificateCryptoCheck(CEncodeDecode::p256VerifyPair(certificate, privateKey, CertificateCryptoEntropy, nullptr) == CENCODE_OK &&
        CEncodeDecode::p256VerifyPair(certificate, reinterpret_cast<const char*>(rootKey), CertificateCryptoEntropy, nullptr) != CENCODE_OK,
        "certificate private key pair");
    CertificateCryptoCheck(CEncodeDecode::sha256(name, sizeof(name), digest, sizeof(digest)) == CENCODE_OK, "proof digest");
    std::memset(signature, 0x5a, sizeof(signature));
    uint32_t signatureSize = 999;
    CertificateCryptoCheck(CEncodeDecode::p256SignDigest(privateKey, digest, CertificateCryptoEntropy, nullptr,
        signature, 1, &signatureSize) == CENCODE_BUFFER && signatureSize > 1 && signature[0] == 0x5a, "signature short buffer");
    CertificateCryptoCheck(CEncodeDecode::p256SignDigest(privateKey, digest, CertificateCryptoEntropy, nullptr,
        signature, 80, &signatureSize) == CENCODE_OK && signatureSize <= 80 && signature[80] == 0x5a, "signature bounded result");
    CertificateCryptoCheck(CEncodeDecode::p256VerifyCertificate(certificate, options.m_authority.m_bytes,
        options.m_authority.m_size, reinterpret_cast<const char*>(name), digest, signature, signatureSize) == CENCODE_OK,
        "combined trust and proof");
    digest[0] ^= 1;
    CertificateCryptoCheck(CEncodeDecode::p256VerifyCertificate(certificate, options.m_authority.m_bytes,
        options.m_authority.m_size, reinterpret_cast<const char*>(name), digest, signature, signatureSize) != CENCODE_OK,
        "changed proof accepted");
    digest[0] ^= 1;
    for (uint32_t mode = 0; mode < 12; ++mode)
    {
        CEncodeDecodeCertificateIssue bad = options;
        int entropyMode = mode == 9 ? 2 : 1;
        if (mode == 0) { bad.m_name = CertificateCryptoText("CN=bad"); }
        if (mode == 1) { bad.m_name = CertificateCryptoText("other.crypto.test"); }
        if (mode == 2) { bad.m_notBefore = CertificateCryptoText("20260230000000"); }
        if (mode == 3) { bad.m_notBefore = CertificateCryptoText("20260001000000"); }
        if (mode == 4) { bad.m_notAfter = bad.m_notBefore; }
        if (mode == 5) { bad.m_dnsNameCount = 9; }
        if (mode == 6) { bad.m_dnsNames = nullptr; }
        if (mode == 7) { bad.m_entropy = nullptr; }
        if (mode == 8 || mode == 9) { bad.m_entropyContext = &entropyMode; }
        if (mode == 10) { bad.m_privateKey.m_bytes = key; bad.m_privateKey.m_size = keySize; }
        if (mode == 11) { bad.m_authority = CertificateCryptoText(certificate.c_str()); }
        std::memset(pem, 0x5a, sizeof(pem));
        length = 0;
        CertificateCryptoCheck(CEncodeDecode::p256IssueCertificate(bad, pem, 8192, &length) != CENCODE_OK &&
            length == 0 && pem[0] == 0x5a && pem[8192] == 0x5a, "issuer invalid input or entropy modified output");
    }
    for (int mode = 1; mode <= 2; ++mode)
    {
        signatureSize = 999;
        std::memset(signature, 0x5a, sizeof(signature));
        CertificateCryptoCheck(CEncodeDecode::p256SignDigest(privateKey, digest, CertificateCryptoEntropy, &mode,
            signature, 80, &signatureSize) == CENCODE_STATE && signatureSize == 0 && signature[0] == 0x5a,
            "signature entropy failure escaped");
    }
    length = 777;
    CertificateCryptoCheck(CEncodeDecode::p256IssueCertificate(options, reinterpret_cast<uint8_t*>(&length), sizeof(length),
        &length) == CENCODE_ARGUMENT && length == 777, "issuer length output alias");
    CertificateCryptoCheck(CEncodeDecode::p256IssueCertificate(options, request, sizeof(request), &length) == CENCODE_ARGUMENT &&
        length == 777, "issuer input/output alias");
    signatureSize = 777;
    CertificateCryptoCheck(CEncodeDecode::p256SignDigest(privateKey, digest, CertificateCryptoEntropy, nullptr,
        digest, sizeof(digest), &signatureSize) == CENCODE_ARGUMENT && signatureSize == 777, "signature digest alias");
    // 独立解析器确认被签发的是非CA、单叶、P-256、数字签名用途，且CSR签名有效。
    mbedtls_x509_crt parsed;
    mbedtls_x509_csr csr;
    mbedtls_x509_crt_init(&parsed);
    mbedtls_x509_csr_init(&csr);
    const bool parsedOkay = mbedtls_x509_crt_parse(&parsed, reinterpret_cast<const uint8_t*>(certificate.c_str()), certificate.size() + 1) == 0 &&
        parsed.next == nullptr && parsed.MBEDTLS_PRIVATE(ca_istrue) == 0 &&
        mbedtls_ecp_keypair_get_group_id(mbedtls_pk_ec(parsed.pk)) == MBEDTLS_ECP_DP_SECP256R1 &&
        mbedtls_x509_crt_check_key_usage(&parsed, MBEDTLS_X509_KU_DIGITAL_SIGNATURE) == 0 &&
        mbedtls_x509_crt_check_extended_key_usage(&parsed, MBEDTLS_OID_SERVER_AUTH, MBEDTLS_OID_SIZE(MBEDTLS_OID_SERVER_AUTH)) == 0 &&
        mbedtls_x509_crt_check_extended_key_usage(&parsed, MBEDTLS_OID_CLIENT_AUTH, MBEDTLS_OID_SIZE(MBEDTLS_OID_CLIENT_AUTH)) == 0 &&
        mbedtls_x509_csr_parse(&csr, request, requestSize) == 0;
    bool proofOkay = false;
    if (parsedOkay && CEncodeDecode::sha256(csr.cri.p, static_cast<uint32_t>(csr.cri.len), digest, sizeof(digest)) == CENCODE_OK)
    {
        proofOkay = mbedtls_pk_verify(&csr.pk, MBEDTLS_MD_SHA256, digest, sizeof(digest), csr.MBEDTLS_PRIVATE(sig).p, csr.MBEDTLS_PRIVATE(sig).len) == 0;
    }
    mbedtls_x509_csr_free(&csr);
    mbedtls_x509_crt_free(&parsed);
    CertificateCryptoCheck(parsedOkay && proofOkay, "independent certificate and CSR verifier");
    CEncodeDecode::wipe(key, sizeof(key));
    CEncodeDecode::wipe(rootKey, sizeof(rootKey));
    std::printf("P256 fresh CA, fixed outputs, proof, dates and entropy PASS\n");
}