#include "NetInterfaceMacro.h"
#include "NetInterfaceRpc.h"
#include "NetInterface.h"
#include <cstring>

int32_t NetInterface::rpcEncode(uint32_t kind, uint32_t value, uint32_t parentLow, uint32_t parentHigh, const uint8_t* data, uint32_t size,
                              uint8_t* output, uint32_t capacity, uint32_t* required)
{
    if (required == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *required = 0;
    if (kind > 65535)
    {
        return NI_INVALID_ARGUMENT;
    }
    return NetInterfaceRpc::encode(static_cast<uint16_t>(kind), value, (static_cast<uint64_t>(parentHigh) << 32) | parentLow, data, size,
                                   output, capacity, *required);
}

int32_t NetInterface::rpcParse(const uint32_t* frameFields, uint32_t count, const uint8_t* data, uint32_t* fields)
{
    uint32_t snapshot[6] = {};
    if (frameFields != nullptr && count == 6)
    {
        std::memcpy(snapshot, frameFields, sizeof(snapshot));
    }
    if (fields != nullptr)
    {
        std::memset(fields, 0, 5 * sizeof(uint32_t));
    }
    if (frameFields == nullptr || count != 6 || fields == nullptr || snapshot[0] > 65535)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(snapshot[0]);
    frame.m_productId = snapshot[1];
    frame.m_requestId = (static_cast<uint64_t>(snapshot[3]) << 32) | snapshot[2];
    frame.m_messageType = snapshot[4];
    frame.m_payloadSize = snapshot[5];
    frame.m_payload = data;
    NetInterfaceRpcView view = {};
    const int32_t result = NetInterfaceRpc::parse(frame, view);
    if (result == NI_OK)
    {
        fields[0] = view.m_value;
        fields[1] = static_cast<uint32_t>(view.m_parent);
        fields[2] = static_cast<uint32_t>(view.m_parent >> 32);
        fields[3] = 16;
        fields[4] = view.m_size;
    }
    return result;
}

int32_t NetInterface::rpcControl(const uint32_t* fields, uint32_t count, uint32_t* reply)
{
    uint32_t values[6] = {};
    if (fields != nullptr && count == 6)
    {
        std::memcpy(values, fields, sizeof(values));
    }
    if (reply != nullptr)
    {
        std::memset(reply, 0, sizeof(values));
    }
    if (fields == nullptr || count != 6 || reply == nullptr || values[0] > 65535)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(values[0]);
    frame.m_productId = values[1];
    frame.m_requestId = (static_cast<uint64_t>(values[3]) << 32) | values[2];
    frame.m_messageType = values[4];
    frame.m_payloadSize = values[5];
    const int32_t result = NetInterfaceRpc::control(frame, frame);
    if (result == NI_READY)
    {
        reply[0] = frame.m_kind;
        reply[1] = frame.m_productId;
        reply[2] = static_cast<uint32_t>(frame.m_requestId);
        reply[3] = static_cast<uint32_t>(frame.m_requestId >> 32);
        reply[4] = frame.m_messageType;
        reply[5] = frame.m_payloadSize;
    }
    return result;
}