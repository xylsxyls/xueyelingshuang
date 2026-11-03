#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceHandshake.h"
#include <cstring>

int32_t NetInterface::handshakeCreate(const uint32_t* options, uint32_t optionCount, const uint8_t* identities, uint32_t identitySize,
                                    uint32_t timeLow, uint32_t timeHigh, NetInterfaceHandshakeHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    if (options == nullptr || identities == nullptr || optionCount != 6 || identitySize != 32)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceHandshakeOptions configuration = {};
    configuration.m_server = options[0];
    configuration.m_platform = options[1];
    configuration.m_capabilities = options[2];
    configuration.m_maxRequests = options[3];
    configuration.m_maxChunk = options[4];
    configuration.m_timeoutMs = options[5];
    std::memcpy(configuration.m_nonce, identities, 16);
    std::memcpy(configuration.m_device, identities + 16, 16);
    const uint64_t now = (static_cast<uint64_t>(timeHigh) << 32) | timeLow;
    try
    {
        NetInterfaceHandshake* handshake = new NetInterfaceHandshake(configuration, now);
        const int32_t result = handshake->progress(now);
        if (result < 0)
        {
            delete handshake;
        }
        else
        {
            *handle = reinterpret_cast<NetInterfaceHandshakeHandle>(handshake);
        }
        return result;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetInterface::handshakeDestroy(NetInterfaceHandshakeHandle handle)
{
    delete reinterpret_cast<NetInterfaceHandshake*>(handle);
}

int32_t NetInterface::handshakeProgress(NetInterfaceHandshakeHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    return handle == nullptr
               ? NI_INVALID_ARGUMENT
               : reinterpret_cast<NetInterfaceHandshake*>(handle)->progress((static_cast<uint64_t>(timeHigh) << 32) | timeLow);
}

int32_t NetInterface::handshakeAccept(NetInterfaceHandshakeHandle handle, const uint32_t* fields, const uint8_t* payload, uint32_t timeLow,
                                    uint32_t timeHigh)
{
    if (handle == nullptr || fields == nullptr || fields[0] > 65535 || (fields[5] != 0 && payload == nullptr))
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
    return reinterpret_cast<NetInterfaceHandshake*>(handle)->accept(frame, (static_cast<uint64_t>(timeHigh) << 32) | timeLow);
}

int32_t NetInterface::handshakePeekOutput(NetInterfaceHandshakeHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr)
    {
        std::memset(fields, 0, 6 * sizeof(uint32_t));
    }
    if (payload != nullptr)
    {
        *payload = nullptr;
    }
    if (handle == nullptr || fields == nullptr || payload == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceFrame frame = {};
    const int32_t result = reinterpret_cast<NetInterfaceHandshake*>(handle)->peekOutput(frame);
    if (result == NI_READY)
    {
        fields[0] = frame.m_kind;
        fields[4] = frame.m_messageType;
        fields[5] = frame.m_payloadSize;
        *payload = frame.m_payload;
    }
    return result;
}

int32_t NetInterface::handshakeConsumeOutput(NetInterfaceHandshakeHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceHandshake*>(handle)->consumeOutput();
}

int32_t NetInterface::handshakePeerInfo(NetInterfaceHandshakeHandle handle, uint32_t* fields, uint8_t* identities)
{
    if (fields != nullptr)
    {
        std::memset(fields, 0, 4 * sizeof(uint32_t));
    }
    if (identities != nullptr)
    {
        std::memset(identities, 0, 32);
    }
    if (handle == nullptr || fields == nullptr || identities == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterfaceHandshake*>(handle)->peerInfo(fields, identities, identities + 16);
}