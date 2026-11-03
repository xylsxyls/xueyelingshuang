#include "NetServerEnrollment.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "mbedtls/md.h"

int32_t NetServerEnrollment::proof(void* context, const uint8_t* statement, uint32_t size, uint8_t* output, uint32_t capacity)
{
    (void)context;
    if (statement == nullptr || size == 0 || size > 16384 || output == nullptr || capacity < 32) { return NI_INVALID_ARGUMENT; }
    const uint8_t credential[32] = { 0xe2, 0xf1, 0x40, 0xa3, 0xfa, 0xb9, 0x54, 0x26, 0xe9, 0x13, 0x68, 0x05, 0x12, 0x8e, 0xeb, 0xfd, 0xd7, 0x88, 0xd0, 0xcb, 0xbb, 0x20, 0x75, 0x0c, 0x41, 0x45, 0x74, 0xb7, 0x74, 0x43, 0x5d, 0xa1 };
    const unsigned char domain[] = "NetInterface service enrollment v3";
    mbedtls_md_context_t hash;
    mbedtls_md_init(&hash);
    int result = mbedtls_md_setup(&hash, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    if (result == 0) { result = mbedtls_md_hmac_starts(&hash, credential, sizeof(credential)); }
    if (result == 0) { result = mbedtls_md_hmac_update(&hash, domain, sizeof(domain)); }
    if (result == 0) { result = mbedtls_md_hmac_update(&hash, statement, size); }
    if (result == 0) { result = mbedtls_md_hmac_finish(&hash, output); }
    mbedtls_md_free(&hash);
    return result == 0 ? NI_OK : NI_PROTOCOL_ERROR;
}

int32_t NetServerEnrollment::verify(const uint8_t* statement, uint32_t size, const uint8_t* received, uint32_t proofSize)
{
    uint8_t expected[32] = {};
    if (received == nullptr || proofSize != sizeof(expected) || proof(nullptr, statement, size, expected, sizeof(expected)) != NI_OK)
    {
        return NI_PROTOCOL_ERROR;
    }
    uint8_t difference = 0;
    for (uint32_t index = 0; index < sizeof(expected); ++index) { difference |= expected[index] ^ received[index]; }
    NetInterface::wipe(expected, sizeof(expected));
    return difference == 0 ? NI_OK : NI_PROTOCOL_ERROR;
}