#include "NetInterfaceCompact.h"
#include "NetInterfaceFileGuard.h"
#include <cstring>

uint32_t NetInterfaceCompact::write(uint8_t* output, uint32_t value)
{
    uint32_t count = 0;
    while (value >= 128)
    {
        output[count++] = static_cast<uint8_t>(value | 128);
        value >>= 7;
    }
    output[count++] = static_cast<uint8_t>(value);
    return count;
}

int32_t NetInterfaceCompact::read(const uint8_t* input, uint32_t size, uint32_t& offset, uint32_t& value)
{
    value = 0;
    for (uint32_t index = 0; index < 5; ++index)
    {
        if (offset == size)
        {
            return NI_NEED_MORE;
        }
        const uint8_t byte = input[offset++];
        if (index == 4 && byte > 15)
        {
            return NI_PROTOCOL_ERROR;
        }
        value |= static_cast<uint32_t>(byte & 127) << (index * 7);
        if (byte < 128)
        {
            return index != 0 && byte == 0 ? NI_PROTOCOL_ERROR : NI_READY;
        }
    }
    return NI_PROTOCOL_ERROR;
}

int32_t NetInterfaceCompact::header(const uint8_t* input, uint32_t size, NetInterfaceFrame& frame, uint32_t& headerSize,
    const NetInterfaceFrame* previous)
{
    frame = NetInterfaceFrame();
    headerSize = 0;
    if (size == 0)
    {
        return NI_NEED_MORE;
    }
    if (input == nullptr || reinterpret_cast<uintptr_t>(input) > UINTPTR_MAX - size || (input[0] != 0xe3 && input[0] != 0xe4 && (input[0] & 0xc0) != 0x80))
    {
        return NI_PROTOCOL_ERROR;
    }
    // 0x80～0xbf将短正文长度放入首字节；同产品/类型的下一号DATA不重复写固定增量1。
    if ((input[0] & 0xc0) == 0x80)
    {
        if (previous == nullptr || previous->m_kind != NI_DATA || previous->m_requestId == UINT64_MAX)
        {
            return NI_PROTOCOL_ERROR;
        }
        frame = *previous;
        ++frame.m_requestId;
        frame.m_payload = nullptr;
        frame.m_payloadSize = input[0] & 63;
        headerSize = 1;
        return NI_READY;
    }
    if (input[0] == 0xe4)
    {
        if (previous == nullptr || previous->m_kind != NI_DATA)
        {
            return NI_PROTOCOL_ERROR;
        }
        uint32_t offset = 1;
        uint32_t length = 0;
        uint32_t delta = 0;
        int32_t result = read(input, size, offset, length);
        if (result != NI_READY)
        {
            return result;
        }
        result = read(input, size, offset, delta);
        if (result != NI_READY)
        {
            return result;
        }
        if (delta == 0 || previous->m_requestId > UINT64_MAX - delta)
        {
            return NI_PROTOCOL_ERROR;
        }
        frame = *previous;
        frame.m_requestId += delta;
        frame.m_payload = nullptr;
        frame.m_payloadSize = length;
        headerSize = offset;
        return NI_READY;
    }
    if (size < 2)
    {
        return NI_NEED_MORE;
    }
    if (input[1] < NI_HELLO || input[1] > NI_DATA_ACK)
    {
        return NI_PROTOCOL_ERROR;
    }
    NetInterfaceFrame decoded = {};
    decoded.m_kind = input[1];
    uint32_t offset = 2;
    int32_t result = read(input, size, offset, decoded.m_productId);
    if (result != NI_READY)
    {
        return result;
    }
    result = read(input, size, offset, decoded.m_messageType);
    if (result != NI_READY)
    {
        return result;
    }
    result = read(input, size, offset, decoded.m_payloadSize);
    if (result != NI_READY)
    {
        return result;
    }
    if (size - offset < 8)
    {
        return NI_NEED_MORE;
    }
    for (uint32_t index = 0; index < 8; ++index)
    {
        decoded.m_requestId = (decoded.m_requestId << 8) | input[offset++];
    }
    frame = decoded;
    headerSize = offset;
    return NI_READY;
}

int32_t NetInterfaceCompact::encode(const NetInterfaceFrame& frame, uint8_t* output, uint32_t capacity, uint32_t& required,
    const NetInterfaceFrame* previous)
{
    if (NetInterfaceFileGuard::overlapsFrameLength(frame, output, capacity, required) ||
        NetInterfaceFileGuard::overlap(&required, sizeof(required), previous, sizeof(frame)))
    {
        return NI_INVALID_ARGUMENT;
    }
    const NetInterfaceFrame input = frame;
    required = 0;
    if (input.m_kind < NI_HELLO || input.m_kind > NI_DATA_ACK ||
        (input.m_payload == nullptr && input.m_payloadSize != 0) || (output == nullptr && capacity != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (input.m_payloadSize > NET_INTERFACE_MAX_PAYLOAD_LIMIT)
    {
        return NI_LIMIT_EXCEEDED;
    }
    const uintptr_t source = reinterpret_cast<uintptr_t>(input.m_payload);
    if (input.m_payloadSize != 0 && source > UINTPTR_MAX - input.m_payloadSize)
    {
        return NI_INVALID_ARGUMENT;
    }
    uint8_t header[25];
    uint32_t length = 0;
    const bool delta = previous != nullptr && previous->m_kind == NI_DATA && input.m_kind == NI_DATA &&
        previous->m_productId == input.m_productId && previous->m_messageType == input.m_messageType &&
        input.m_requestId > previous->m_requestId && input.m_requestId - previous->m_requestId <= UINT32_MAX;
    if (delta && input.m_payloadSize < 128 && input.m_requestId - previous->m_requestId < 128)
    {
        // 常用增量头直接落在最终输出，避免中转头及两次变长整数调用；校验先于任何写入。
        const bool next = input.m_payloadSize < 64 && input.m_requestId - previous->m_requestId == 1;
        const uint32_t prefix = next ? 1 : 3;
        required = prefix + input.m_payloadSize;
        if (capacity < required)
        {
            return NI_BUFFER_TOO_SMALL;
        }
        const uintptr_t destination = reinterpret_cast<uintptr_t>(output);
        if (destination > UINTPTR_MAX - required || (input.m_payloadSize != 0 &&
            source < destination + required && destination < source + input.m_payloadSize))
        {
            required = 0;
            return NI_INVALID_ARGUMENT;
        }
        if (next)
        {
            output[0] = static_cast<uint8_t>(0x80 | input.m_payloadSize);
        }
        else
        {
            output[0] = 0xe4;
            output[1] = static_cast<uint8_t>(input.m_payloadSize);
            output[2] = static_cast<uint8_t>(input.m_requestId - previous->m_requestId);
        }
        if (input.m_payloadSize != 0)
        {
            std::memcpy(output + prefix, input.m_payload, input.m_payloadSize);
        }
        return NI_OK;
    }
    if (delta)
    {
        header[length++] = 0xe4;
        length += write(header + length, input.m_payloadSize);
        length += write(header + length, static_cast<uint32_t>(input.m_requestId - previous->m_requestId));
    }
    else
    {
        header[length++] = 0xe3;
        header[length++] = static_cast<uint8_t>(input.m_kind);
        length += write(header + length, input.m_productId);
        length += write(header + length, input.m_messageType);
        length += write(header + length, input.m_payloadSize);
        for (uint32_t index = 0; index < 8; ++index)
        {
            header[length++] = static_cast<uint8_t>(input.m_requestId >> (56 - index * 8));
        }
    }
    required = length + input.m_payloadSize;
    if (capacity < required)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    const uintptr_t destination = reinterpret_cast<uintptr_t>(output);
    if (destination > UINTPTR_MAX - required || (input.m_payloadSize != 0 &&
        source < destination + required && destination < source + input.m_payloadSize))
    {
        required = 0;
        return NI_INVALID_ARGUMENT;
    }
    std::memcpy(output, header, length);
    if (input.m_payloadSize != 0)
    {
        std::memcpy(output + length, input.m_payload, input.m_payloadSize);
    }
    return NI_OK;
}