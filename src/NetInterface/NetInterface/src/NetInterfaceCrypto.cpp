#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/sha256.h"
#include "mbedtls/platform_util.h"
#include <cstring>
#include "NetInterfaceCryptoState.h"
#include <new>

NetInterfaceHashOpaque::NetInterfaceHashOpaque() :
m_size(0),
m_status(NI_OK),
m_finished(false)
{
    mbedtls_sha256_init(&m_context);
    if (mbedtls_sha256_starts(&m_context, 0) != 0)
    {
        m_status = NI_INVALID_STATE;
    }
}

NetInterfaceHashOpaque::~NetInterfaceHashOpaque()
{
    mbedtls_sha256_free(&m_context);
}

int32_t NetInterface::hashCreate(NetInterfaceHashHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = new (std::nothrow) NetInterfaceHashOpaque();
    if (*handle == nullptr)
    {
        return NI_NO_MEMORY;
    }
    if ((*handle)->m_status != NI_OK)
    {
        delete *handle;
        *handle = nullptr;
        return NI_INVALID_STATE;
    }
    return NI_OK;
}

int32_t NetInterface::hashUpdate(NetInterfaceHashHandle handle, const uint8_t* input, uint32_t size)
{
    if (handle == nullptr || (input == nullptr && size != 0) || size > 16777216)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_finished)
    {
        return NI_INVALID_STATE;
    }
    if (handle->m_status < 0)
    {
        return handle->m_status;
    }
    if (handle->m_size > UINT64_C(0x1fffffffffffffff) - size)
    {
        handle->m_status = NI_LIMIT_EXCEEDED;
        return handle->m_status;
    }
    if (size != 0 && mbedtls_sha256_update(&handle->m_context, input, size) != 0)
    {
        handle->m_status = NI_INVALID_STATE;
        return handle->m_status;
    }
    handle->m_size += size;
    return NI_OK;
}

int32_t NetInterface::hashFinish(NetInterfaceHashHandle handle, uint8_t* output, uint32_t capacity)
{
    if (handle == nullptr || output == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_finished)
    {
        return NI_INVALID_STATE;
    }
    if (handle->m_status < 0)
    {
        return handle->m_status;
    }
    if (capacity < 32)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    uint8_t hash[32] = {};
    const int result = mbedtls_sha256_finish(&handle->m_context, hash);
    if (result == 0)
    {
        std::memcpy(output, hash, sizeof(hash));
    }
    mbedtls_platform_zeroize(hash, sizeof(hash));
    handle->m_finished = true;
    handle->m_status = result == 0 ? NI_OK : NI_INVALID_STATE;
    return handle->m_status;
}

void NetInterface::hashDestroy(NetInterfaceHashHandle handle)
{
    delete handle;
}

int32_t NetInterface::sha256(const uint8_t* input, uint32_t size, uint8_t* output, uint32_t capacity)
{
    if ((input == nullptr && size != 0) || output == nullptr || size > 16777216)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (capacity < 32)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    uint8_t hash[32] = {};
    const int result = mbedtls_sha256(input, size, hash, 0);
    if (result == 0)
    {
        std::memcpy(output, hash, sizeof(hash));
    }
    mbedtls_platform_zeroize(hash, sizeof(hash));
    return result == 0 ? NI_OK : NI_INVALID_STATE;
}

int32_t NetInterface::pbkdf2Sha256(const uint8_t* password, uint32_t passwordSize, const uint8_t* salt, uint32_t saltSize,
                                 uint32_t iterations, uint8_t* output, uint32_t capacity)
{
    if (password == nullptr || passwordSize == 0 || passwordSize > 1024 || salt == nullptr || saltSize == 0 || saltSize > 128 ||
        iterations == 0 || iterations > 2000000 || output == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (capacity < 32)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    uint8_t hash[32] = {};
    const int result =
        mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256, password, passwordSize, salt, saltSize, iterations, sizeof(hash), hash);
    if (result == 0)
    {
        std::memcpy(output, hash, sizeof(hash));
    }
    mbedtls_platform_zeroize(hash, sizeof(hash));
    return result == 0 ? NI_OK : NI_INVALID_STATE;
}

void NetInterface::wipe(uint8_t* bytes, uint32_t size)
{
    if (bytes != nullptr && size != 0)
    {
        mbedtls_platform_zeroize(bytes, size);
    }
}

int32_t NetInterface::entropyBytes(NetInterfaceEntropyFunction entropy, void* context, uint8_t* output, uint32_t size)
{
    if ((output == nullptr && size != 0) || size > 8192)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (size == 0)
    {
        return NI_OK;
    }
    try
    {
        if (entropy != nullptr && entropy(context, output, size) == 0)
        {
            return NI_OK;
        }
    }
    catch (...)
    {
        // 部分输出或异常不能作为握手随机数继续使用
    }
    NetInterface::wipe(output, size);
    return NI_INVALID_STATE;
}