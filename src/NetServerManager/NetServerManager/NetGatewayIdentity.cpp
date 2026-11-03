#include "NetGatewayIdentity.h"
#include "NetGatewayAuthority.h"
#include "NetWork/NetWorkAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cstring>
#include <memory>
#include <vector>

/** 使用操作系统强随机
@param [in] context 忽略
@param [out] output 输出
@param [in] size 字节数
@return 0成功，否则失败
*/
static int32_t IdentityEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return NetTransport::randomBytes(output, size) == NI_OK ? 0 : -1;
}

/** 摘要和随机标识的十六进制表示，不处理或输出私钥
@param [in] bytes 公开标识随机字节
@param [in] size 字节数
@return 小写十六进制
*/
static std::string IdentityHex(const uint8_t* bytes, uint32_t size)
{
    const char alphabet[] = "0123456789abcdef";
    std::string value;
    value.reserve(size * 2);
    for (uint32_t index = 0; index < size; ++index)
    {
        value += alphabet[bytes[index] >> 4];
        value += alphabet[bytes[index] & 15];
    }
    return value;
}

/** 读取固定大端长度
@param [in] data 至少4字节
@return uint32长度
*/
static uint32_t IdentityLength(const uint8_t* data)
{
    return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) |
        (static_cast<uint32_t>(data[2]) << 8) | data[3];
}

/** 写入固定大端长度
@param [in,out] output 头部缓冲
@param [in] offset 写入偏移
@param [in] value 长度
*/
static void IdentityWriteLength(std::vector<uint8_t>& output, uint32_t offset, uint32_t value)
{
    for (uint32_t index = 0; index < 4; ++index)
    {
        output[offset + 3 - index] = static_cast<uint8_t>(value);
        value >>= 8;
    }
}

NetGatewayIdentity::NetGatewayIdentity()
{

}

NetGatewayIdentity::~NetGatewayIdentity()
{
    if (!m_privateKey.empty())
    {
        NetInterface::wipe(reinterpret_cast<uint8_t*>(&m_privateKey[0]), static_cast<uint32_t>(m_privateKey.size()));
    }
}

int32_t NetGatewayIdentity::load(const std::string& path)
{
    std::vector<uint8_t> bytes;
    if (!CSystem::plainFileUtf8(path) || !CSystem::readBinaryFile(CStringManager::Utf8ToUnicode(path), 24576, bytes) || bytes.size() < 48 ||
        std::memcmp(bytes.data(), "NGI3", 4) != 0)
    {
        if (!bytes.empty()) { NetInterface::wipe(bytes.data(), static_cast<uint32_t>(bytes.size())); }
        return NI_PROTOCOL_ERROR;
    }
    const uint32_t nameSize = IdentityLength(bytes.data() + 4);
    const uint32_t keySize = IdentityLength(bytes.data() + 8);
    const uint32_t certSize = IdentityLength(bytes.data() + 12);
    if (nameSize != 37 || keySize == 0 || keySize > 4096 || certSize == 0 || certSize > 16384 ||
        static_cast<uint64_t>(nameSize) + keySize + certSize + 48 != bytes.size())
    {
        NetInterface::wipe(bytes.data(), static_cast<uint32_t>(bytes.size()));
        return NI_PROTOCOL_ERROR;
    }
    uint8_t digest[32] = {};
    const uint32_t payloadSize = static_cast<uint32_t>(bytes.size()) - 32;
    if (NetInterface::sha256(bytes.data(), payloadSize, digest, sizeof(digest)) != NI_OK ||
        std::memcmp(digest, bytes.data() + payloadSize, sizeof(digest)) != 0)
    {
        NetInterface::wipe(bytes.data(), static_cast<uint32_t>(bytes.size()));
        return NI_PROTOCOL_ERROR;
    }
    try
    {
        std::string name(reinterpret_cast<const char*>(bytes.data() + 16), nameSize);
        std::string key(reinterpret_cast<const char*>(bytes.data() + 16 + nameSize), keySize);
        std::string certificate(reinterpret_cast<const char*>(bytes.data() + 16 + nameSize + keySize), certSize);
        NetInterface::wipe(bytes.data(), static_cast<uint32_t>(bytes.size()));
        const int32_t verified = name.compare(0, 5, "node.") == 0 &&
            name.find_first_not_of("0123456789abcdef", 5) == std::string::npos ?
            NetInterfaceCertificate::verifyPair(certificate, key, name, IdentityEntropy, nullptr) : NI_PROTOCOL_ERROR;
        if (verified == NI_OK)
        {
            m_name.swap(name);
            m_privateKey.swap(key);
            m_certificate.swap(certificate);
        }
        if (!key.empty()) { NetInterface::wipe(reinterpret_cast<uint8_t*>(&key[0]), static_cast<uint32_t>(key.size())); }
        return verified;
    }
    catch (...)
    {
        NetInterface::wipe(bytes.data(), static_cast<uint32_t>(bytes.size()));
        return NI_NO_MEMORY;
    }
}

int32_t NetGatewayIdentity::loadOrCreate(const std::string& directory)
{
    if (!m_name.empty() || directory.empty() || !CSystem::createDirectoriesUtf8(directory))
    {
        return NI_INVALID_ARGUMENT;
    }
    const std::string canonical = CSystem::canonicalPathUtf8(directory);
    if (canonical.empty())
    {
        return NI_INVALID_ARGUMENT;
    }
    const std::string path = CSystem::joinPath(canonical, "identity.dat");
    const std::wstring wide = CStringManager::Utf8ToUnicode(path);
    if (CSystem::fileExists(wide))
    {
        return load(path);
    }
    uint8_t identity[16] = {};
    uint8_t temporaryId[16] = {};
    if (IdentityEntropy(nullptr, identity, sizeof(identity)) != 0 || IdentityEntropy(nullptr, temporaryId, sizeof(temporaryId)) != 0)
    {
        return NI_INVALID_STATE;
    }
    const std::string name = "node." + IdentityHex(identity, sizeof(identity));
    std::string key;
    std::string request;
    std::string certificate;
    int32_t result = NetInterfaceCertificate::createRequest(name, IdentityEntropy, nullptr, key, request);
    if (result == NI_OK)
    {
        result = NetGatewayAuthority::issueLocalIdentity(name, request, IdentityEntropy, nullptr, certificate);
    }
    if (result != NI_OK)
    {
        if (!key.empty()) { NetInterface::wipe(reinterpret_cast<uint8_t*>(&key[0]), static_cast<uint32_t>(key.size())); }
        return result;
    }
    const std::string temporary = CSystem::joinPath(canonical, "identity.tmp." + IdentityHex(temporaryId, sizeof(temporaryId)));
    const std::wstring temporaryWide = CStringManager::Utf8ToUnicode(temporary);
    std::vector<uint8_t> bytes(16);
    std::memcpy(bytes.data(), "NGI3", 4);
    IdentityWriteLength(bytes, 4, static_cast<uint32_t>(name.size()));
    IdentityWriteLength(bytes, 8, static_cast<uint32_t>(key.size()));
    IdentityWriteLength(bytes, 12, static_cast<uint32_t>(certificate.size()));
    bytes.insert(bytes.end(), name.begin(), name.end());
    bytes.insert(bytes.end(), key.begin(), key.end());
    bytes.insert(bytes.end(), certificate.begin(), certificate.end());
    uint8_t digest[32] = {};
    result = NetInterface::sha256(bytes.data(), static_cast<uint32_t>(bytes.size()), digest, sizeof(digest));
    bytes.insert(bytes.end(), digest, digest + sizeof(digest));
    NetInterface::wipe(reinterpret_cast<uint8_t*>(&key[0]), static_cast<uint32_t>(key.size()));
    std::unique_ptr<FILE, bool (*)(FILE*)> file(CSystem::openPrivateBinaryOutputFile(temporaryWide), CSystem::closeBinaryOutputFile);
    bool installed = false;
    if (file)
    {
        const bool written = result == NI_OK && CSystem::writeBinaryOutputFile(file.get(), bytes.data(), bytes.size()) &&
            CSystem::flushBinaryOutputFile(file.get());
        const bool closed = CSystem::closeBinaryOutputFile(file.release());
        if (written && closed)
        {
            installed = CSystem::installFileIfAbsent(temporaryWide, wide);
        }
        // 只清理本次排他创建、同可信目录的随机临时路径，绝不删除失败时已有的别人文件。
        if (CSystem::fileExists(temporaryWide))
        {
            CSystem::deleteFile(temporaryWide);
        }
    }
    NetInterface::wipe(bytes.data(), static_cast<uint32_t>(bytes.size()));
    if (!installed && !CSystem::fileExists(wide))
    {
        return NI_INVALID_STATE;
    }
    return load(path);
}