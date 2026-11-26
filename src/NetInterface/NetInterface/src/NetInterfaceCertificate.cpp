#include "NetInterfaceCertificate.h"
#include "NetInterfaceMacro.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetInterface.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include <cstring>
#include <algorithm>

/** 固定域分隔防止身份挑战签名被当作其他协议签名使用
@param [in] data 挑战及声明
@param [in] size 字节数
@param [out] digest 32字节SHA256
@return 0成功，否则失败
*/
static int CertificateProofDigest(const uint8_t* data, uint32_t size, unsigned char* digest)
{
    const unsigned char domain[] = "NetInterface role proof v3";
    const CEncodeDecodeBytes parts[] = {{domain, sizeof(domain)}, {data, size}};
    return CEncodeDecode::sha256Parts(parts, 2, digest, 32);
}

/** 将密码库失败映射为网关握手失败，保留内存和参数错误
@param [in] result CEncodeDecodeResult
@return 网关状态
*/
static int32_t CertificateResult(int32_t result)
{
    switch (result)
    {
    case CENCODE_OK: return NI_OK;
    case CENCODE_ARGUMENT: return NI_INVALID_ARGUMENT;
    case CENCODE_MEMORY: return NI_NO_MEMORY;
    default: return NI_PROTOCOL_ERROR;
    }
}

/** 擦除已有输出的全部字节，包括超过单次擦除参数范围的文本
@param [in,out] text 独占字符串
*/
static void CertificateErase(std::string& text)
{
    size_t offset = 0;
    while (offset < text.size())
    {
        const uint32_t size = static_cast<uint32_t>((std::min)(text.size() - offset, static_cast<size_t>(UINT32_MAX)));
        CEncodeDecode::wipe(reinterpret_cast<uint8_t*>(&text[offset]), size);
        offset += size;
    }
    text.clear();
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
    CertificateErase(privateKey);
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
            CertificateErase(privateKey);
            request.clear();
            result = NI_NO_MEMORY;
        }
    }
    CEncodeDecode::wipe(keyBytes, sizeof(keyBytes));
    return result;
}

int32_t NetInterfaceCertificate::signProof(const std::string& privateKey, const uint8_t* data, uint32_t size,
    NetInterfaceEntropyFunction entropy, void* context, std::vector<uint8_t>& signature)
{
    if (privateKey.empty() || privateKey.size() > 4096 || CStringManager::Find(privateKey, '\0') != std::string::npos || entropy == nullptr ||
        data == nullptr || size == 0 || size > 16384)
    {
        signature.clear();
        return NI_INVALID_ARGUMENT;
    }
    unsigned char digest[32] = {};
    int32_t result = CertificateProofDigest(data, size, digest);
    // 挑战可以借用输出向量的元素，读取完毕后才能使这些元素失效。
    signature.clear();
    if (result == CENCODE_OK)
    {
        uint8_t bytes[80] = {};
        uint32_t length = 0;
        result = CEncodeDecode::p256SignDigest(privateKey, digest, entropy, context, bytes, sizeof(bytes), &length);
        if (result == CENCODE_OK)
        {
            try
            {
                signature.assign(bytes, bytes + length);
            }
            catch (...)
            {
                result = CENCODE_MEMORY;
            }
        }
    }
    CEncodeDecode::wipe(digest, sizeof(digest));
    return CertificateResult(result);
}

int32_t NetInterfaceCertificate::verifyProof(const std::string& certificate, const std::string& name, const uint8_t* data,
    uint32_t size, const uint8_t* signature, uint32_t signatureSize)
{
    if (data == nullptr || size == 0 || size > 16384 || signature == nullptr || signatureSize == 0 || signatureSize > 80)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (!validName(name) || certificate.empty() || certificate.size() > 16384 || CStringManager::Find(certificate, '\0') != std::string::npos)
    {
        return NI_INVALID_ARGUMENT;
    }
    uint32_t authoritySize = 0;
    const uint8_t* authority = NetInterface::trustAuthority(&authoritySize);
    unsigned char digest[32] = {};
    int32_t result = CertificateProofDigest(data, size, digest);
    if (result == CENCODE_OK)
    {
        result = CEncodeDecode::p256VerifyCertificate(certificate, authority, authoritySize, name, digest, signature, signatureSize);
    }
    CEncodeDecode::wipe(digest, sizeof(digest));
    return CertificateResult(result);
}

int32_t NetInterfaceCertificate::verify(const std::string& certificate, const std::string& name)
{
    if (!validName(name) || certificate.empty() || certificate.size() > 16384 || CStringManager::Find(certificate, '\0') != std::string::npos)
    {
        return NI_INVALID_ARGUMENT;
    }
    uint32_t authoritySize = 0;
    const uint8_t* authority = NetInterface::trustAuthority(&authoritySize);
    return CertificateResult(CEncodeDecode::p256VerifyCertificate(certificate, authority, authoritySize, name));
}

int32_t NetInterfaceCertificate::verifyPair(const std::string& certificate, const std::string& privateKey,
    const std::string& name, NetInterfaceEntropyFunction entropy, void* context)
{
    if (entropy == nullptr || privateKey.empty() || privateKey.size() > 4096 || CStringManager::Find(privateKey, '\0') != std::string::npos)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t verified = verify(certificate, name);
    if (verified != NI_OK)
    {
        return verified;
    }
    return CertificateResult(CEncodeDecode::p256VerifyPair(certificate, privateKey, entropy, context));
}