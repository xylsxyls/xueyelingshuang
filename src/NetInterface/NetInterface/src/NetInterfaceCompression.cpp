#include "NetInterfaceMacro.h"
#include "NetInterfaceCompression.h"
#include "NetInterface.h"
#include "NetInterfaceFileTypes.h"
#include "Compress/CompressAPI.h"
#include <cstring>
#include <new>
#include <vector>

int32_t NetInterfaceCompression::encode(const NetInterfaceFrame& frame, uint8_t* output, uint32_t capacity, uint32_t& required)
{
    required = 0;
    uint32_t raw = 0;
    const int32_t validation = NetInterface::encode(frame, nullptr, 0, raw);
    if (validation != NI_BUFFER_TOO_SMALL) { return validation; }
    if ((frame.m_kind != NI_REQUEST && frame.m_kind != NI_RESPONSE) || frame.m_payloadSize < 1024 ||
        (frame.m_messageType >= NI_FILE_OPEN_UPLOAD && frame.m_messageType <= NI_FILE_REVOKE_SCOPE))
    {
        return NetInterface::encode(frame, output, capacity, required);
    }
    try
    {
        uint32_t size = Compress::zlibRawCompressBound(frame.m_payloadSize);
        if (size == 0)
        {
            return NI_LIMIT_EXCEEDED;
        }
        std::vector<uint8_t> zipped(static_cast<size_t>(size) + 8);
        std::memcpy(zipped.data(), "NZC1", 4);
        for (uint32_t index = 0; index < 4; ++index) { zipped[4 + index] = static_cast<uint8_t>(frame.m_payloadSize >> (24 - index * 8)); }
        const int32_t status = Compress::zlibRawCompress(zipped.data() + 8, size, frame.m_payload, frame.m_payloadSize, 1);
        if (status != COMPRESS_OK) { return status == COMPRESS_NO_MEMORY ? NI_NO_MEMORY : NI_PROTOCOL_ERROR; }
        if (size + 8 > frame.m_payloadSize - frame.m_payloadSize / 12)
        {
            return NetInterface::encode(frame, output, capacity, required);
        }
        NetInterfaceFrame compressed = frame;
        compressed.m_payload = zipped.data();
        compressed.m_payloadSize = static_cast<uint32_t>(size + 8);
        const int32_t result = NetInterface::encode(compressed, output, capacity, required);
        if (result == NI_OK) { output[31] = 1; }
        return result;
    }
    catch (...) { return NI_NO_MEMORY; }
}

int32_t NetInterfaceCompression::decode(const uint8_t* input, uint32_t size, uint32_t limit, uint8_t*& output, uint32_t& length)
{
    output = nullptr;
    length = 0;
    if (input == nullptr || size < 9 || std::memcmp(input, "NZC1", 4) != 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    uint32_t original = 0;
    for (uint32_t index = 0; index < 4; ++index) { original = (original << 8) | input[4 + index]; }
    if (original == 0 || original > limit) { return NI_LIMIT_EXCEEDED; }
    uint8_t* bytes = new (std::nothrow) uint8_t[original];
    if (bytes == nullptr) { return NI_NO_MEMORY; }
    uint32_t actual = original;
    uint32_t source = size - 8;
    const int32_t status = Compress::zlibRawUnCompress(bytes, actual, input + 8, source);
    if (status != COMPRESS_OK || actual != original || source != size - 8)
    {
        volatile uint8_t* wipe = bytes;
        for (uint32_t index = 0; index < original; ++index) { wipe[index] = 0; }
        delete[] bytes;
        return status == COMPRESS_NO_MEMORY ? NI_NO_MEMORY : NI_PROTOCOL_ERROR;
    }
    output = bytes;
    length = original;
    return NI_OK;
}