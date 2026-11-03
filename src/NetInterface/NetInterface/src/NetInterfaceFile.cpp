#include "NetInterfaceFile.h"
#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceFileGuard.h"
#include <cstring>

bool NetInterfaceFile::isControl(uint32_t type)
{
    return type >= NI_FILE_OPEN_UPLOAD && type <= NI_FILE_CANCEL;
}

bool NetInterfaceFile::zero(const uint8_t* bytes, uint32_t size)
{
    uint8_t combined = 0;
    for (uint32_t index = 0; index < size; ++index)
    {
        combined |= bytes[index];
    }
    return combined == 0;
}

bool NetInterfaceFile::equal(const uint8_t* left, const uint8_t* right, uint32_t size)
{
    if (left == nullptr || right == nullptr || size > 32)
    {
        return false;
    }
    volatile uint8_t difference = 0;
    for (uint32_t index = 0; index < size; ++index)
    {
        difference |= left[index] ^ right[index];
    }
    return difference == 0;
}

uint32_t NetInterfaceFile::read32(const uint8_t* bytes)
{
    uint32_t value = 0;
    for (uint32_t index = 0; index < 4; ++index)
    {
        value = (value << 8) | bytes[index];
    }
    return value;
}

uint64_t NetInterfaceFile::read64(const uint8_t* bytes)
{
    return (static_cast<uint64_t>(read32(bytes)) << 32) | read32(bytes + 4);
}

void NetInterfaceFile::write32(uint8_t* bytes, uint32_t value)
{
    for (uint32_t index = 0; index < 4; ++index)
    {
        bytes[index] = static_cast<uint8_t>(value >> ((3 - index) * 8));
    }
}

void NetInterfaceFile::write64(uint8_t* bytes, uint64_t value)
{
    write32(bytes, static_cast<uint32_t>(value >> 32));
    write32(bytes + 4, static_cast<uint32_t>(value));
}

bool NetInterfaceFile::validInfo(const NetInterfaceFileInfo& info)
{
    return (info.direction == 1 || info.direction == 2) && info.state >= NI_FILE_PREPARING && info.state <= NI_FILE_FAILED &&
           info.chunk >= 1024 && info.chunk <= 65536 && info.window >= 1 && info.window <= 8 &&
           info.total <= UINT64_C(0x1fffffffffffffff) && info.offset <= info.total &&
           (info.offset == info.total || info.offset % info.chunk == 0) && info.expiresUtc != 0 && info.expiresUtc <= INT64_MAX &&
           !zero(info.transfer, 16) && !zero(info.token, 32) &&
           (info.state == NI_FILE_TRANSFERRING ? info.stream != 0 : info.stream == 0) &&
           (info.state != NI_FILE_COMPLETED || info.offset == info.total);
}

int32_t NetInterfaceFile::encodeInfo(const NetInterfaceFileInfo& info, uint8_t* output, uint32_t capacity)
{
    if (output == nullptr || !validInfo(info))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (capacity < NET_INTERFACE_FILE_INFO_SIZE)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    uint8_t bytes[NET_INTERFACE_FILE_INFO_SIZE] = {};
    NetInterfaceFileGuard wipe(bytes, sizeof(bytes));
    std::memcpy(bytes, "NFI2", 4);
    write32(bytes + 4, 2);
    write32(bytes + 8, info.direction);
    write32(bytes + 12, info.state);
    write32(bytes + 16, info.chunk);
    write32(bytes + 20, info.window);
    write64(bytes + 24, info.total);
    write64(bytes + 32, info.offset);
    write64(bytes + 40, info.stream);
    write64(bytes + 48, info.expiresUtc);
    std::memcpy(bytes + 56, info.transfer, 16);
    std::memcpy(bytes + 72, info.token, 32);
    std::memcpy(bytes + 104, info.hash, 32);
    std::memcpy(bytes + 136, info.scope, 16);
    std::memcpy(output, bytes, sizeof(bytes));
    return NI_OK;
}

int32_t NetInterfaceFile::parseInfo(const uint8_t* bytes, uint32_t size, NetInterfaceFileInfo& info)
{
    if (NetInterfaceFileGuard::overlap(bytes, size, &info, sizeof(info)))
    {
        return NI_INVALID_ARGUMENT;
    }
    std::memset(&info, 0, sizeof(info));
    if (bytes == nullptr || size != NET_INTERFACE_FILE_INFO_SIZE || std::memcmp(bytes, "NFI2", 4) != 0 || read32(bytes + 4) != 2 ||
        !zero(bytes + 152, 8))
    {
        return NI_PROTOCOL_ERROR;
    }
    NetInterfaceFileInfo candidate = {};
    NetInterfaceFileGuard wipe(&candidate, sizeof(candidate));
    candidate.direction = read32(bytes + 8);
    candidate.state = read32(bytes + 12);
    candidate.chunk = read32(bytes + 16);
    candidate.window = read32(bytes + 20);
    candidate.total = read64(bytes + 24);
    candidate.offset = read64(bytes + 32);
    candidate.stream = read64(bytes + 40);
    candidate.expiresUtc = read64(bytes + 48);
    std::memcpy(candidate.transfer, bytes + 56, 16);
    std::memcpy(candidate.token, bytes + 72, 32);
    std::memcpy(candidate.hash, bytes + 104, 32);
    std::memcpy(candidate.scope, bytes + 136, 16);
    if (!validInfo(candidate))
    {
        return NI_PROTOCOL_ERROR;
    }
    info = candidate;
    return NI_OK;
}

int32_t NetInterfaceFile::encodeData(const NetInterfaceFileData& data, uint8_t* output, uint32_t capacity, uint32_t& required)
{
    if (NetInterfaceFileGuard::overlap(&required, sizeof(required), &data, sizeof(data)) ||
        NetInterfaceFileGuard::overlap(&required, sizeof(required), data.bytes, data.size) ||
        NetInterfaceFileGuard::overlap(&required, sizeof(required), output, capacity))
    {
        return NI_INVALID_ARGUMENT;
    }
    required = 0;
    if (data.flags > 1 || data.size > 65536 || (data.size != 0 && data.bytes == nullptr) || (data.flags == 1 && data.size != 0) ||
        (data.flags == 0 && data.size == 0) || zero(data.token, 32) || data.offset > UINT64_C(0x1fffffffffffffff) - data.size)
    {
        return NI_INVALID_ARGUMENT;
    }
    required = NET_INTERFACE_FILE_DATA_HEADER_SIZE + data.size;
    if (output == nullptr || capacity < required)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    if (NetInterfaceFileGuard::overlap(output, required, &data, sizeof(data)) || NetInterfaceFileGuard::overlap(output, required, data.bytes, data.size))
    {
        required = 0;
        return NI_INVALID_ARGUMENT;
    }
    std::memset(output, 0, NET_INTERFACE_FILE_DATA_HEADER_SIZE);
    std::memcpy(output, "NFD2", 4);
    write32(output + 4, data.flags);
    write64(output + 8, data.offset);
    write32(output + 16, data.size);
    std::memcpy(output + 24, data.token, 32);
    std::memcpy(output + 56, data.hash, 32);
    if (data.size != 0)
    {
        std::memcpy(output + NET_INTERFACE_FILE_DATA_HEADER_SIZE, data.bytes, data.size);
    }
    return NI_OK;
}

int32_t NetInterfaceFile::parseData(const NetInterfaceFrame& frame, NetInterfaceFileData& data)
{
    if (NetInterfaceFileGuard::overlap(&frame, sizeof(frame), &data, sizeof(data)) ||
        NetInterfaceFileGuard::overlap(frame.m_payload, frame.m_payloadSize, &data, sizeof(data)))
    {
        return NI_INVALID_ARGUMENT;
    }
    std::memset(&data, 0, sizeof(data));
    if (frame.m_kind != NI_DATA || frame.m_messageType != NI_FILE_DATA_TYPE || frame.m_productId == 0 || frame.m_requestId == 0 ||
        frame.m_payload == nullptr || frame.m_payloadSize < NET_INTERFACE_FILE_DATA_HEADER_SIZE ||
        frame.m_payloadSize > NET_INTERFACE_FILE_DATA_HEADER_SIZE + 65536 || std::memcmp(frame.m_payload, "NFD2", 4) != 0 ||
        read32(frame.m_payload + 20) != 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    NetInterfaceFileData candidate = {};
    NetInterfaceFileGuard wipe(&candidate, sizeof(candidate));
    candidate.flags = read32(frame.m_payload + 4);
    candidate.offset = read64(frame.m_payload + 8);
    candidate.size = read32(frame.m_payload + 16);
    if (candidate.flags > 1 || candidate.size != frame.m_payloadSize - NET_INTERFACE_FILE_DATA_HEADER_SIZE ||
        (candidate.flags == 1 ? candidate.size != 0 : candidate.size == 0) ||
        candidate.offset > UINT64_C(0x1fffffffffffffff) - candidate.size || zero(frame.m_payload + 24, 32))
    {
        return NI_PROTOCOL_ERROR;
    }
    std::memcpy(candidate.token, frame.m_payload + 24, 32);
    std::memcpy(candidate.hash, frame.m_payload + 56, 32);
    candidate.bytes = candidate.size != 0 ? frame.m_payload + NET_INTERFACE_FILE_DATA_HEADER_SIZE : nullptr;
    data = candidate;
    return NI_OK;
}

int32_t NetInterfaceFile::encodeAck(const NetInterfaceFileAck& ack, uint8_t* output, uint32_t capacity)
{
    if (output == nullptr || ack.flags > 2 || (ack.flags == 2 ? ack.result >= 0 : ack.result != NI_OK) ||
        ack.offset > UINT64_C(0x1fffffffffffffff) || zero(ack.token, 32))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (capacity < NET_INTERFACE_FILE_ACK_SIZE)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    uint8_t bytes[NET_INTERFACE_FILE_ACK_SIZE] = {};
    NetInterfaceFileGuard wipe(bytes, sizeof(bytes));
    std::memcpy(bytes, "NFA2", 4);
    write32(bytes + 4, ack.flags);
    write32(bytes + 8, static_cast<uint32_t>(ack.result));
    write64(bytes + 16, ack.offset);
    std::memcpy(bytes + 24, ack.token, 32);
    std::memcpy(bytes + 56, ack.hash, 32);
    std::memcpy(output, bytes, sizeof(bytes));
    return NI_OK;
}

int32_t NetInterfaceFile::parseAck(const NetInterfaceFrame& frame, NetInterfaceFileAck& ack)
{
    if (NetInterfaceFileGuard::overlap(&frame, sizeof(frame), &ack, sizeof(ack)) ||
        NetInterfaceFileGuard::overlap(frame.m_payload, frame.m_payloadSize, &ack, sizeof(ack)))
    {
        return NI_INVALID_ARGUMENT;
    }
    std::memset(&ack, 0, sizeof(ack));
    if (frame.m_kind != NI_DATA_ACK || frame.m_messageType != NI_FILE_DATA_TYPE || frame.m_productId == 0 || frame.m_requestId == 0 ||
        frame.m_payload == nullptr || frame.m_payloadSize != NET_INTERFACE_FILE_ACK_SIZE || std::memcmp(frame.m_payload, "NFA2", 4) != 0 ||
        read32(frame.m_payload + 12) != 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    NetInterfaceFileAck candidate = {};
    NetInterfaceFileGuard wipe(&candidate, sizeof(candidate));
    candidate.flags = read32(frame.m_payload + 4);
    const uint32_t result = read32(frame.m_payload + 8);
    candidate.result =
        result <= INT32_MAX ? static_cast<int32_t>(result) : static_cast<int32_t>(-1 - static_cast<int64_t>(UINT32_MAX - result));
    candidate.offset = read64(frame.m_payload + 16);
    if (candidate.flags > 2 || (candidate.flags == 2 ? candidate.result >= 0 : candidate.result != NI_OK) ||
        candidate.offset > UINT64_C(0x1fffffffffffffff) || zero(frame.m_payload + 24, 32))
    {
        return NI_PROTOCOL_ERROR;
    }
    std::memcpy(candidate.token, frame.m_payload + 24, 32);
    std::memcpy(candidate.hash, frame.m_payload + 56, 32);
    ack = candidate;
    return NI_OK;
}

int32_t NetInterfaceFile::encodeNotice(const NetInterfaceFileNotice& notice, uint8_t* output, uint32_t capacity)
{
    if (notice.result >= 0 || output == nullptr || zero(notice.transfer, 16) || zero(notice.token, 32))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (capacity < NET_INTERFACE_FILE_NOTICE_SIZE)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    uint8_t bytes[NET_INTERFACE_FILE_NOTICE_SIZE] = {};
    NetInterfaceFileGuard wipe(bytes, sizeof(bytes));
    std::memcpy(bytes, "NFN2", 4);
    write32(bytes + 4, static_cast<uint32_t>(notice.result));
    std::memcpy(bytes + 8, notice.transfer, 16);
    std::memcpy(bytes + 24, notice.token, 32);
    std::memcpy(output, bytes, sizeof(bytes));
    return NI_OK;
}

int32_t NetInterfaceFile::parseNotice(const NetInterfaceFrame& frame, NetInterfaceFileNotice& notice)
{
    if (NetInterfaceFileGuard::overlap(&frame, sizeof(frame), &notice, sizeof(notice)) ||
        NetInterfaceFileGuard::overlap(frame.m_payload, frame.m_payloadSize, &notice, sizeof(notice)))
    {
        return NI_INVALID_ARGUMENT;
    }
    std::memset(&notice, 0, sizeof(notice));
    if (frame.m_kind != NI_EVENT || frame.m_productId == 0 || frame.m_requestId == 0 || frame.m_messageType != NI_FILE_CANCEL ||
        frame.m_payload == nullptr || frame.m_payloadSize != NET_INTERFACE_FILE_NOTICE_SIZE ||
        std::memcmp(frame.m_payload, "NFN2", 4) != 0 || read32(frame.m_payload + 4) <= INT32_MAX || zero(frame.m_payload + 8, 16) ||
        zero(frame.m_payload + 24, 32))
    {
        return NI_PROTOCOL_ERROR;
    }
    notice.result = static_cast<int32_t>(-1 - static_cast<int64_t>(UINT32_MAX - read32(frame.m_payload + 4)));
    std::memcpy(notice.transfer, frame.m_payload + 8, 16);
    std::memcpy(notice.token, frame.m_payload + 24, 32);
    return NI_OK;
}

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