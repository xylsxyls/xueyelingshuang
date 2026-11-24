#include "NetInterfaceMacro.h"
#include "NetInterfaceTunnel.h"
#include "NetInterfaceFileGuard.h"

static_assert(static_cast<int32_t>(NI_OK) == NC_OK && static_cast<int32_t>(NI_NEED_MORE) == NC_NEED_MORE && static_cast<int32_t>(NI_READY) == NC_READY &&
    static_cast<int32_t>(NI_INVALID_ARGUMENT) == NC_INVALID_ARGUMENT && static_cast<int32_t>(NI_PROTOCOL_ERROR) == NC_PROTOCOL_ERROR &&
    static_cast<int32_t>(NI_LIMIT_EXCEEDED) == NC_LIMIT_EXCEEDED && static_cast<int32_t>(NI_BUFFER_TOO_SMALL) == NC_BUFFER_TOO_SMALL &&
    static_cast<int32_t>(NI_NO_MEMORY) == NC_NO_MEMORY && static_cast<int32_t>(NI_TRUNCATED) == NC_TRUNCATED && static_cast<int32_t>(NI_INVALID_STATE) == NC_INVALID_STATE,
    "NetInterface compatibility result values must match NetChannel");
static_assert(NET_INTERFACE_TUNNEL_HEADER_SIZE == NET_CHANNEL_HEADER_SIZE &&
    NET_INTERFACE_TUNNEL_PAYLOAD_LIMIT == NET_CHANNEL_PAYLOAD_LIMIT && static_cast<int32_t>(NI_TUNNEL_HELLO) == NC_HELLO &&
    static_cast<int32_t>(NI_TUNNEL_CONTROL) == NC_CONTROL && static_cast<int32_t>(NI_TUNNEL_BYTES) == NC_BYTES && static_cast<int32_t>(NI_TUNNEL_CLOSE) == NC_CLOSE,
    "NetInterface compatibility records must preserve the NTW3 format");

NetInterfaceTunnel::NetInterfaceTunnel() :
m_decoder()
{

}

NetInterfaceTunnel::~NetInterfaceTunnel()
{

}

int32_t NetInterfaceTunnel::feed(const uint8_t* data, uint32_t size, uint32_t& consumed)
{
    if (NetInterfaceFileGuard::overlap(data, size, &consumed, sizeof(consumed))) { return NI_INVALID_ARGUMENT; }
    return m_decoder.feed(data, size, consumed);
}

int32_t NetInterfaceTunnel::peek(NetInterfaceTunnelRecord& record) const
{
    NetChannelRecord view = {};
    const int32_t result = m_decoder.peek(view);
    record.m_kind = view.m_kind;
    record.m_product = view.m_product;
    record.m_channel = view.m_channel;
    record.m_data = view.m_data;
    record.m_size = view.m_size;
    return result;
}

int32_t NetInterfaceTunnel::consume()
{
    return m_decoder.consume();
}

int32_t NetInterfaceTunnel::finish()
{
    return m_decoder.finish();
}

int32_t NetInterfaceTunnel::encode(const NetInterfaceTunnelRecord& record, uint8_t* output, uint32_t capacity, uint32_t& required)
{
    if (NetInterfaceFileGuard::overlap(&required, sizeof(required), &record, sizeof(record)) ||
        NetInterfaceFileGuard::overlap(&required, sizeof(required), record.m_data, record.m_size) ||
        NetInterfaceFileGuard::overlap(&required, sizeof(required), output, capacity))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetChannelRecord view = {};
    view.m_kind = record.m_kind;
    view.m_product = record.m_product;
    view.m_channel = record.m_channel;
    view.m_data = record.m_data;
    view.m_size = record.m_size;
    return NetChannel::encode(view, output, capacity, required);
}