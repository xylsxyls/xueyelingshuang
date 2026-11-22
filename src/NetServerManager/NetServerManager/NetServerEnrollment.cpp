#include "NetServerEnrollment.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"

int32_t NetServerEnrollment::proof(void* context, const uint8_t* statement, uint32_t size, uint8_t* output, uint32_t capacity)
{
    (void)context;
    if (statement == nullptr || size == 0 || size > 16384 || output == nullptr || capacity < 32) { return NI_INVALID_ARGUMENT; }
    const uint8_t credential[32] = { 0xe2, 0xf1, 0x40, 0xa3, 0xfa, 0xb9, 0x54, 0x26, 0xe9, 0x13, 0x68, 0x05, 0x12, 0x8e, 0xeb, 0xfd, 0xd7, 0x88, 0xd0, 0xcb, 0xbb, 0x20, 0x75, 0x0c, 0x41, 0x45, 0x74, 0xb7, 0x74, 0x43, 0x5d, 0xa1 };
    const unsigned char domain[] = "NetInterface service enrollment v3";
    const CEncodeDecodeBytes parts[] = {{domain, sizeof(domain)}, {statement, size}};
    const int32_t result = CEncodeDecode::hmacSha256Parts(credential, sizeof(credential), parts, 2, output, capacity);
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