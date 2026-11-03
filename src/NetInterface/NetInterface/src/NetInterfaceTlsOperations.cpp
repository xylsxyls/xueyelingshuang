#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceTls.h"
#include <new>

int32_t NetInterface::tlsCreateClient(const uint8_t* authority, uint32_t authoritySize, const char* hostname,
                                    NetInterfaceEntropyFunction entropy, void* context, uint32_t timeoutMs, NetInterfaceTlsHandle* handle,
                                    int32_t* detail)
{
    NetInterfaceTlsOptions options = {};
    options.authority = authority;
    options.authoritySize = authoritySize;
    options.hostname = hostname;
    options.entropy = entropy;
    options.entropyContext = context;
    options.handshakeTimeoutMs = timeoutMs;
    return NetInterface::tlsCreate(&options, handle, detail);
}

int32_t NetInterface::tlsCreate(const NetInterfaceTlsOptions* options, NetInterfaceTlsHandle* handle, int32_t* detail)
{
    if (detail != nullptr)
    {
        *detail = 0;
    }
    if (handle == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    *handle = nullptr;
    if (options == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    NetInterfaceTls* instance = new (std::nothrow) NetInterfaceTls(*options);
    if (instance == nullptr)
    {
        return NET_INTERFACE_TLS_MEMORY;
    }
    int32_t result = instance->status();
    if (detail != nullptr)
    {
        *detail = instance->detail();
    }
    if (result < 0)
    {
        delete instance;
    }
    else
    {
        *handle = reinterpret_cast<NetInterfaceTlsHandle>(instance);
    }
    return result;
}

void NetInterface::tlsDestroy(NetInterfaceTlsHandle handle)
{
    delete reinterpret_cast<NetInterfaceTls*>(handle);
}

int32_t NetInterface::tlsFeed(NetInterfaceTlsHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed)
{
    if (consumed == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    *consumed = 0;
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->feed(data, size, *consumed);
}

int32_t NetInterface::tlsProgress(NetInterfaceTlsHandle handle, uint32_t timeLow, uint32_t timeHigh)
{
    uint64_t time = (static_cast<uint64_t>(timeHigh) << 32) | timeLow;
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->progress(time);
}

int32_t NetInterface::tlsPeekOutput(NetInterfaceTlsHandle handle, const uint8_t** data, uint32_t* size)
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
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    return reinterpret_cast<NetInterfaceTls*>(handle)->peekOutput(*data, *size);
}

int32_t NetInterface::tlsConsumeOutput(NetInterfaceTlsHandle handle, uint32_t size)
{
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->consumeOutput(size);
}

int32_t NetInterface::tlsWrite(NetInterfaceTlsHandle handle, const uint8_t* data, uint32_t size, uint32_t* accepted)
{
    if (accepted == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    *accepted = 0;
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->write(data, size, *accepted);
}

int32_t NetInterface::tlsRead(NetInterfaceTlsHandle handle, uint8_t* output, uint32_t capacity, uint32_t* received)
{
    if (received == nullptr)
    {
        return NET_INTERFACE_TLS_ARGUMENT;
    }
    *received = 0;
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->read(output, capacity, *received);
}

int32_t NetInterface::tlsFinishInput(NetInterfaceTlsHandle handle)
{
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->finishInput();
}

int32_t NetInterface::tlsClose(NetInterfaceTlsHandle handle)
{
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->close();
}

int32_t NetInterface::tlsGetStatus(NetInterfaceTlsHandle handle, int32_t* detail)
{
    if (detail != nullptr)
    {
        *detail = handle == nullptr ? 0 : reinterpret_cast<NetInterfaceTls*>(handle)->detail();
    }
    return handle == nullptr ? NET_INTERFACE_TLS_ARGUMENT : reinterpret_cast<NetInterfaceTls*>(handle)->status();
}