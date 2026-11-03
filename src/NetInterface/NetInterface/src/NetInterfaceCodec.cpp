#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include <cstring>
#include <new>

int32_t NetInterface::create(uint32_t payloadLimit, NetInterfaceHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    if (payloadLimit == 0 || payloadLimit > NET_INTERFACE_MAX_PAYLOAD_LIMIT)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterface* decoder = new (std::nothrow) NetInterface(payloadLimit);
    if (decoder == nullptr)
    {
        return NI_NO_MEMORY;
    }
    *handle = reinterpret_cast<NetInterfaceHandle>(decoder);
    return NI_OK;
}

void NetInterface::destroy(NetInterfaceHandle handle)
{
    delete reinterpret_cast<NetInterface*>(handle);
}

int32_t NetInterface::feed(NetInterfaceHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed)
{
    if (consumed == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *consumed = 0;
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterface*>(handle)->feed(data, size, *consumed);
}

int32_t NetInterface::peek(NetInterfaceHandle handle, NetInterfaceFrame* frame)
{
    if (frame == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    std::memset(frame, 0, sizeof(*frame));
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterface*>(handle)->peek(*frame);
}

int32_t NetInterface::consume(NetInterfaceHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterface*>(handle)->consume();
}

int32_t NetInterface::finish(NetInterfaceHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterface*>(handle)->finish();
}

int32_t NetInterface::reset(NetInterfaceHandle handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    reinterpret_cast<NetInterface*>(handle)->reset();
    return NI_OK;
}

int32_t NetInterface::encode(const NetInterfaceFrame* frame, uint8_t* output, uint32_t capacity, uint32_t* required)
{
    if (required == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *required = 0;
    if (frame == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return NetInterface::encode(*frame, output, capacity, *required);
}

int32_t NetInterface::peekFields(NetInterfaceHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr)
    {
        std::memset(fields, 0, 6 * sizeof(uint32_t));
    }
    if (payload != nullptr)
    {
        *payload = nullptr;
    }
    if (fields == nullptr || payload == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    int32_t result = NetInterface::peek(handle, &frame);
    if (result == NI_READY)
    {
        fields[0] = frame.m_kind;
        fields[1] = frame.m_productId;
        fields[2] = static_cast<uint32_t>(frame.m_requestId);
        fields[3] = static_cast<uint32_t>(frame.m_requestId >> 32);
        fields[4] = frame.m_messageType;
        fields[5] = frame.m_payloadSize;
        *payload = frame.m_payload;
    }
    return result;
}

int32_t NetInterface::encodeFields(const uint32_t* fields, const uint8_t* payload, uint8_t* output, uint32_t capacity, uint32_t* required)
{
    if (required != nullptr)
    {
        *required = 0;
    }
    if (fields == nullptr || fields[0] > UINT16_MAX)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(fields[0]);
    frame.m_productId = fields[1];
    frame.m_requestId = (static_cast<uint64_t>(fields[3]) << 32) | fields[2];
    frame.m_messageType = fields[4];
    frame.m_payloadSize = fields[5];
    frame.m_payload = payload;
    return NetInterface::encode(&frame, output, capacity, required);
}