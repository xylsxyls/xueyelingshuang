#include "NetCertificateBufferTest.h"
#include "NetWorkTestCases.h"
#include <stdexcept>
#include "NetInterface/NetInterfaceAPI.h"
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
int NetCertificateBufferTest::check()
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


void NetCertificateBufferTest::run()
{
    netTestCheckpoint();
    if (check() != 0)
    {
        throw std::runtime_error("Certificate fixed buffers and entropy failures failed");
    }
}