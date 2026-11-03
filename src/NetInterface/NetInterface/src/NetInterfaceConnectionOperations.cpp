#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceConnection.h"
#include "NetInterface.h"
#include <cstring>
#include <sstream>

int32_t NetInterface::connectionCreate(const NetInterfaceTlsOptions* options, NetInterfaceConnectionHandle* handle, int32_t* detail)
{
    if (detail != nullptr)
    {
        *detail = 0;
    }
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    if (options == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        NetInterfaceConnection* connection = new NetInterfaceConnection(*options);
        const int32_t result = connection->status();
        if (detail != nullptr)
        {
            *detail = connection->detail();
        }
        if (result < 0)
        {
            delete connection;
        }
        else
        {
            *handle = reinterpret_cast<NetInterfaceConnectionHandle>(connection);
        }
        return result;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::connectionCreateServiceClient(uint32_t product, NetInterfaceEntropyFunction entropy, void* context,
    uint32_t timeoutMs, NetInterfaceConnectionHandle* handle, int32_t* detail)
{
    if (handle == nullptr) { return NI_INVALID_ARGUMENT; }
    *handle = nullptr;
    if (detail != nullptr) { *detail = 0; }
    try
    {
        std::ostringstream name;
        name << "service." << product;
        const std::string hostname = name.str();
        NetInterfaceTlsOptions options = {};
        options.authority = NetInterface::trustAuthority(&options.authoritySize);
        options.hostname = hostname.c_str();
        options.entropy = entropy;
        options.entropyContext = context;
        options.handshakeTimeoutMs = timeoutMs;
        return NetInterface::connectionCreate(&options, handle, detail);
    }
    catch (...) { return NI_NO_MEMORY; }
}

void NetInterface::connectionDestroy(NetInterfaceConnectionHandle handle)
{
    delete reinterpret_cast<NetInterfaceConnection*>(handle);
}

int32_t NetInterface::connectionFeed(NetInterfaceConnectionHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed)
{
    if (consumed != nullptr)
    {
        *consumed = 0;
    }
    if (handle == nullptr || consumed == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterfaceConnection*>(handle)->feed(data, size, *consumed);
}

int32_t NetInterface::connectionProgress(NetInterfaceConnectionHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterfaceConnection*>(handle)->progress((static_cast<uint64_t>(timeHigh) << 32) | timeLow);
}

int32_t NetInterface::connectionConfigureCompression(NetInterfaceConnectionHandle handle, const uint32_t* types, uint32_t count, int32_t negotiated)
{
    if (handle == nullptr || (negotiated != 0 && negotiated != 1)) { return NI_INVALID_ARGUMENT; }
    return reinterpret_cast<NetInterfaceConnection*>(handle)->configureCompression(types, count, negotiated != 0);
}

int32_t NetInterface::connectionSend(NetInterfaceConnectionHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    if (handle == nullptr || fields == nullptr || fields[0] > 65535)
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
    return reinterpret_cast<NetInterfaceConnection*>(handle)->send(frame);
}

int32_t NetInterface::connectionPeek(NetInterfaceConnectionHandle handle, uint32_t* fields, const uint8_t** payload)
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
    const int32_t result = reinterpret_cast<NetInterfaceConnection*>(handle)->peek(frame);
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

int32_t NetInterface::connectionConsume(NetInterfaceConnectionHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceConnection*>(handle)->consume();
}

int32_t NetInterface::connectionPeekOutput(NetInterfaceConnectionHandle handle, const uint8_t** data, uint32_t* size)
{
    if (data != nullptr)
    {
        *data = nullptr;
    }
    if (size != nullptr)
    {
        *size = 0;
    }
    if (handle == nullptr || data == nullptr || size == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    return reinterpret_cast<NetInterfaceConnection*>(handle)->peekOutput(*data, *size);
}

int32_t NetInterface::connectionConsumeOutput(NetInterfaceConnectionHandle handle, uint32_t size)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceConnection*>(handle)->consumeOutput(size);
}

int32_t NetInterface::connectionFinishInput(NetInterfaceConnectionHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceConnection*>(handle)->finishInput();
}

int32_t NetInterface::connectionGetStatus(NetInterfaceConnectionHandle handle, int32_t* detail)
{
    if (detail != nullptr)
    {
        *detail = handle == nullptr ? 0 : reinterpret_cast<NetInterfaceConnection*>(handle)->detail();
    }
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceConnection*>(handle)->status();
}