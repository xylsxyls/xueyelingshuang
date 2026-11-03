#include "NetInterfaceMacro.h"
#include "NetInterfaceEvent.h"
#include "NetInterfaceFileTypes.h"
#include <cstring>

int32_t NetInterfaceEvent::encode(uint64_t target, uint32_t flags, const uint8_t* data, uint32_t size, uint8_t* output, uint32_t capacity,
                                  uint32_t& required)
{
    required = 0;
    if (flags > 1 || (flags == 0 && target == 0) || (data == nullptr && size != 0) || size > 1048552 || (flags == 1 && size != 16))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (flags == 1)
    {
        uint8_t combined = 0;
        for (uint32_t index = 0; index < size; ++index)
        {
            combined |= data[index];
        }
        if (combined == 0)
        {
            return NI_INVALID_ARGUMENT;
        }
    }
    required = size + 24;
    if (output == nullptr || capacity < required)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    if (size != 0)
    {
        const uintptr_t from = reinterpret_cast<uintptr_t>(data);
        const uintptr_t to = reinterpret_cast<uintptr_t>(output);
        if ((from <= to && to - from < size) || (to <= from && from - to < required))
        {
            return NI_INVALID_ARGUMENT;
        }
    }
    std::memset(output, 0, 24);
    std::memcpy(output, "NVE2", 4);
    output[7] = 2;
    for (uint32_t index = 0; index < 8; ++index)
    {
        output[8 + index] = static_cast<uint8_t>(target >> ((7 - index) * 8));
    }
    output[19] = static_cast<uint8_t>(flags);
    if (size != 0)
    {
        std::memcpy(output + 24, data, size);
    }
    return NI_OK;
}

int32_t NetInterfaceEvent::parse(const NetInterfaceFrame& frame, NetInterfaceEventView& view)
{
    std::memset(&view, 0, sizeof(view));
    if (frame.m_kind != NI_EVENT || frame.m_requestId == 0 || frame.m_payload == nullptr || frame.m_payloadSize < 24 ||
        frame.m_payloadSize > 1048576 || std::memcmp(frame.m_payload, "NVE2", 4) != 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    const uint8_t* bytes = frame.m_payload;
    if (bytes[4] != 0 || bytes[5] != 0 || bytes[6] != 0 || bytes[7] != 2 || bytes[16] != 0 || bytes[17] != 0 || bytes[18] != 0 ||
        bytes[19] > 1 || bytes[20] != 0 || bytes[21] != 0 || bytes[22] != 0 || bytes[23] != 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    uint64_t target = 0;
    for (uint32_t index = 0; index < 8; ++index)
    {
        target = (target << 8) | bytes[8 + index];
    }
    const uint32_t size = frame.m_payloadSize - 24;
    if ((bytes[19] == 0 && target == 0) || (bytes[19] == 1 && (size != 16 || frame.m_messageType != NI_FILE_REVOKE_SCOPE)))
    {
        return NI_PROTOCOL_ERROR;
    }
    if (bytes[19] == 1)
    {
        uint8_t combined = 0;
        for (uint32_t index = 0; index < size; ++index)
        {
            combined |= bytes[24 + index];
        }
        if (combined == 0)
        {
            return NI_PROTOCOL_ERROR;
        }
    }
    view.m_target = target;
    view.m_flags = bytes[19];
    view.m_data = size != 0 ? bytes + 24 : nullptr;
    view.m_size = size;
    return NI_OK;
}