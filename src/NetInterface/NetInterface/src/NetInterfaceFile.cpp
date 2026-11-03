#include "NetInterfaceMacro.h"
#include "NetInterfaceFile.h"
#include "NetInterfaceFileGuard.h"
#include <cstring>

NetInterfaceFileGuard::NetInterfaceFileGuard(void* bytes, uint32_t size) :
m_bytes(static_cast<uint8_t*>(bytes)),
m_size(size)
{

}

NetInterfaceFileGuard::~NetInterfaceFileGuard()
{
    NetInterface::wipe(m_bytes, m_size);
}

bool NetInterfaceFileGuard::overlap(const void* left, uint32_t leftSize, const void* right, uint32_t rightSize)
{
    if (left == nullptr || right == nullptr || leftSize == 0 || rightSize == 0)
    {
        return false;
    }
    const uintptr_t first = reinterpret_cast<uintptr_t>(left);
    const uintptr_t second = reinterpret_cast<uintptr_t>(right);
    return first > UINTPTR_MAX - leftSize || second > UINTPTR_MAX - rightSize ||
        (first < second + rightSize && second < first + leftSize);
}

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