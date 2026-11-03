#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceTunnel.h"
#include <cstring>
#include <new>

int32_t NetInterface::tunnelCreate(NetInterfaceTunnelHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    NetInterfaceTunnel* decoder = new (std::nothrow) NetInterfaceTunnel();
    if (decoder == nullptr)
    {
        return NI_NO_MEMORY;
    }
    *handle = reinterpret_cast<NetInterfaceTunnelHandle>(decoder);
    return NI_OK;
}

void NetInterface::tunnelDestroy(NetInterfaceTunnelHandle handle)
{
    delete reinterpret_cast<NetInterfaceTunnel*>(handle);
}

int32_t NetInterface::tunnelFeed(NetInterfaceTunnelHandle handle, const uint8_t* data, uint32_t size, uint32_t* consumed)
{
    if (consumed == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *consumed = 0;
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceTunnel*>(handle)->feed(data, size, *consumed);
}

int32_t NetInterface::tunnelPeek(NetInterfaceTunnelHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr)
    {
        std::memset(fields, 0, 5 * sizeof(uint32_t));
    }
    if (payload != nullptr)
    {
        *payload = nullptr;
    }
    if (handle == nullptr || fields == nullptr || payload == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceTunnelRecord record = {};
    const int32_t result = reinterpret_cast<NetInterfaceTunnel*>(handle)->peek(record);
    if (result == NI_READY)
    {
        fields[0] = record.m_kind;
        fields[1] = record.m_product;
        fields[2] = static_cast<uint32_t>(record.m_channel);
        fields[3] = static_cast<uint32_t>(record.m_channel >> 32);
        fields[4] = record.m_size;
        *payload = record.m_data;
    }
    return result;
}

int32_t NetInterface::tunnelConsume(NetInterfaceTunnelHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceTunnel*>(handle)->consume();
}

int32_t NetInterface::tunnelFinish(NetInterfaceTunnelHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceTunnel*>(handle)->finish();
}

int32_t NetInterface::tunnelEncode(const uint32_t* fields, const uint8_t* payload, uint8_t* output,
    uint32_t capacity, uint32_t* required)
{
    if (required == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *required = 0;
    if (fields == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceTunnelRecord record = {};
    record.m_kind = fields[0];
    record.m_product = fields[1];
    record.m_channel = static_cast<uint64_t>(fields[3]) << 32 | fields[2];
    record.m_data = payload;
    record.m_size = fields[4];
    return NetInterfaceTunnel::encode(record, output, capacity, *required);
}