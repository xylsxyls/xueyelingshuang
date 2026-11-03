#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceFile.h"
#include "NetInterfaceFileGuard.h"
#include <cstring>

int32_t NetInterfaceFile::encodeControl(const NetInterfaceFileControl& control, uint8_t* output, uint32_t capacity, uint32_t& required)
{
    if (NetInterfaceFileGuard::overlap(&required, sizeof(required), &control, sizeof(control)) ||
        NetInterfaceFileGuard::overlap(&required, sizeof(required), control.key, control.keySize) ||
        NetInterfaceFileGuard::overlap(&required, sizeof(required), control.authorization, control.authorizationSize) ||
        NetInterfaceFileGuard::overlap(&required, sizeof(required), output, capacity))
    {
        return NI_INVALID_ARGUMENT;
    }
    required = 0;
    const bool open = control.operation == NI_FILE_OPEN_UPLOAD || control.operation == NI_FILE_OPEN_DOWNLOAD;
    if (!isControl(control.operation) || control.keySize > 512 || control.authorizationSize > 2048 || (control.keySize && !control.key) ||
        (control.authorizationSize && !control.authorization) || zero(control.transfer, 16) ||
        (open ? control.keySize == 0 || control.chunk < 1024 || control.chunk > 65536 || control.window < 1 || control.window > 8 ||
                    !zero(control.token, 32) || control.total > UINT64_C(0x1fffffffffffffff) ||
                    (control.operation == NI_FILE_OPEN_DOWNLOAD && (control.total != 0 || !zero(control.hash, 32)))
              : control.keySize != 0 || control.chunk != 0 || control.window != 0 || control.total != 0 || zero(control.token, 32) ||
                    !zero(control.hash, 32)))
    {
        return NI_INVALID_ARGUMENT;
    }
    required = NET_INTERFACE_FILE_CONTROL_HEADER_SIZE + control.keySize + control.authorizationSize;
    if (!output || capacity < required)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    if (NetInterfaceFileGuard::overlap(output, required, &control, sizeof(control)) ||
        NetInterfaceFileGuard::overlap(output, required, control.key, control.keySize) ||
        NetInterfaceFileGuard::overlap(output, required, control.authorization, control.authorizationSize))
    {
        required = 0;
        return NI_INVALID_ARGUMENT;
    }
    uint8_t header[NET_INTERFACE_FILE_CONTROL_HEADER_SIZE] = {};
    NetInterfaceFileGuard wipe(header, sizeof(header));
    std::memcpy(header, "NFC2", 4);
    write32(header + 4, 2);
    write32(header + 8, control.operation);
    write32(header + 12, control.chunk);
    write32(header + 16, control.window);
    write32(header + 20, control.keySize);
    write32(header + 24, control.authorizationSize);
    write64(header + 32, control.total);
    std::memcpy(header + 40, control.transfer, 16);
    std::memcpy(header + 56, control.token, 32);
    std::memcpy(header + 88, control.hash, 32);
    std::memcpy(output, header, sizeof(header));
    if (control.keySize)
    {
        std::memcpy(output + sizeof(header), control.key, control.keySize);
    }
    if (control.authorizationSize)
    {
        std::memcpy(output + sizeof(header) + control.keySize, control.authorization, control.authorizationSize);
    }
    return NI_OK;
}

int32_t NetInterfaceFile::parseControl(uint32_t operation, const uint8_t* bytes, uint32_t size, NetInterfaceFileControl& control)
{
    if (NetInterfaceFileGuard::overlap(bytes, size, &control, sizeof(control)))
    {
        return NI_INVALID_ARGUMENT;
    }
    std::memset(&control, 0, sizeof(control));
    if (!bytes || size < NET_INTERFACE_FILE_CONTROL_HEADER_SIZE || size > 2680 || std::memcmp(bytes, "NFC2", 4) != 0 ||
        read32(bytes + 4) != 2 || read32(bytes + 8) != operation || read32(bytes + 28) != 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    NetInterfaceFileControl candidate = {};
    NetInterfaceFileGuard wipe(&candidate, sizeof(candidate));
    candidate.operation = operation;
    candidate.chunk = read32(bytes + 12);
    candidate.window = read32(bytes + 16);
    candidate.keySize = read32(bytes + 20);
    candidate.authorizationSize = read32(bytes + 24);
    if (candidate.keySize > 512 || candidate.authorizationSize > 2048 ||
        size != NET_INTERFACE_FILE_CONTROL_HEADER_SIZE + candidate.keySize + candidate.authorizationSize)
    {
        return NI_PROTOCOL_ERROR;
    }
    candidate.total = read64(bytes + 32);
    std::memcpy(candidate.transfer, bytes + 40, 16);
    std::memcpy(candidate.token, bytes + 56, 32);
    std::memcpy(candidate.hash, bytes + 88, 32);
    candidate.key = candidate.keySize ? bytes + NET_INTERFACE_FILE_CONTROL_HEADER_SIZE : nullptr;
    candidate.authorization = candidate.authorizationSize ? bytes + NET_INTERFACE_FILE_CONTROL_HEADER_SIZE + candidate.keySize : nullptr;
    uint32_t required = 0;
    if (encodeControl(candidate, nullptr, 0, required) != NI_BUFFER_TOO_SMALL || required != size)
    {
        return NI_PROTOCOL_ERROR;
    }
    control = candidate;
    return NI_OK;
}

int32_t NetInterface::fileControlEncode(const uint32_t* fields, const uint8_t* transfer, const uint8_t* token, const uint8_t* hash,
                                      const uint8_t* key, uint32_t keySize, const uint8_t* authorization, uint32_t authorizationSize,
                                      uint8_t* output, uint32_t capacity, uint32_t* required)
{
    if (required != nullptr &&
        (NetInterfaceFileGuard::overlap(required, sizeof(*required), fields, 5 * sizeof(uint32_t)) ||
         NetInterfaceFileGuard::overlap(required, sizeof(*required), transfer, 16) ||
         NetInterfaceFileGuard::overlap(required, sizeof(*required), token, 32) ||
         NetInterfaceFileGuard::overlap(required, sizeof(*required), hash, 32) ||
         NetInterfaceFileGuard::overlap(required, sizeof(*required), key, keySize) ||
         NetInterfaceFileGuard::overlap(required, sizeof(*required), authorization, authorizationSize) ||
         NetInterfaceFileGuard::overlap(required, sizeof(*required), output, capacity)))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (required != nullptr)
    {
        *required = 0;
    }
    if (!fields || !transfer || !token || !hash || !required || (!output && capacity))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFileControl control = {};
    NetInterfaceFileGuard wipe(&control, sizeof(control));
    control.operation = fields[0];
    control.chunk = fields[1];
    control.window = fields[2];
    control.total = (static_cast<uint64_t>(fields[4]) << 32) | fields[3];
    std::memcpy(control.transfer, transfer, 16);
    std::memcpy(control.token, token, 32);
    std::memcpy(control.hash, hash, 32);
    control.key = key;
    control.keySize = keySize;
    control.authorization = authorization;
    control.authorizationSize = authorizationSize;
    return NetInterfaceFile::encodeControl(control, output, capacity, *required);
}