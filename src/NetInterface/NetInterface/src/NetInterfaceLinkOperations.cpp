#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceLink.h"
#include <cstring>
#include <new>

/** 不通过平台结构布局传递64位通道号
@param [in] fields 五项路由字段
@param [in] payload 字节
@return 借用记录
*/
static NetInterfaceTunnelRecord LinkApiRecord(const uint32_t* fields, const uint8_t* payload)
{
    NetInterfaceTunnelRecord record = {};
    record.m_kind = fields[0];
    record.m_product = fields[1];
    record.m_channel = (static_cast<uint64_t>(fields[3]) << 32) | fields[2];
    record.m_size = fields[4];
    record.m_data = payload;
    return record;
}

/** 复制借用记录的布局无关字段
@param [in] record 原生记录
@param [out] fields 五项
@param [out] payload 借用密文
*/
static void LinkApiRecordFields(const NetInterfaceTunnelRecord& record, uint32_t* fields, const uint8_t** payload)
{
    fields[0] = record.m_kind;
    fields[1] = record.m_product;
    fields[2] = static_cast<uint32_t>(record.m_channel);
    fields[3] = static_cast<uint32_t>(record.m_channel >> 32);
    fields[4] = record.m_size;
    *payload = record.m_data;
}

int32_t NetInterface::linkCreate(const NetInterfaceLinkOptions* options, NetInterfaceLinkHandle* handle)
{
    if (handle == nullptr) { return NI_INVALID_ARGUMENT; }
    *handle = nullptr;
    if (options == nullptr) { return NI_INVALID_ARGUMENT; }
    try
    {
        NetInterfaceLink* link = new (std::nothrow) NetInterfaceLink(*options);
        if (link == nullptr) { return NI_NO_MEMORY; }
        const int32_t status = link->status();
        if (status < 0) { delete link; return status; }
        *handle = reinterpret_cast<NetInterfaceLinkHandle>(link);
        return status;
    }
    catch (...) { return NI_NO_MEMORY; }
}

void NetInterface::linkDestroy(NetInterfaceLinkHandle handle)
{
    delete reinterpret_cast<NetInterfaceLink*>(handle);
}

int32_t NetInterface::linkAccept(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    return handle == nullptr || fields == nullptr ? NI_INVALID_ARGUMENT :
        reinterpret_cast<NetInterfaceLink*>(handle)->accept(LinkApiRecord(fields, payload));
}

int32_t NetInterface::linkProgress(NetInterfaceLinkHandle handle, uint32_t low, uint32_t high)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT :
        reinterpret_cast<NetInterfaceLink*>(handle)->progress((static_cast<uint64_t>(high) << 32) | low);
}

int32_t NetInterface::linkPeekOutput(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr) { std::memset(fields, 0, 5 * sizeof(uint32_t)); }
    if (payload != nullptr) { *payload = nullptr; }
    if (handle == nullptr || fields == nullptr || payload == nullptr) { return NI_INVALID_ARGUMENT; }
    NetInterfaceTunnelRecord record = {};
    const int32_t result = reinterpret_cast<NetInterfaceLink*>(handle)->peekOutput(record);
    if (result == NI_READY) { LinkApiRecordFields(record, fields, payload); }
    return result;
}

int32_t NetInterface::linkConsumeOutput(NetInterfaceLinkHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceLink*>(handle)->consumeOutput();
}

int32_t NetInterface::linkSendRecord(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    return handle == nullptr || fields == nullptr ? NI_INVALID_ARGUMENT :
        reinterpret_cast<NetInterfaceLink*>(handle)->sendRecord(LinkApiRecord(fields, payload));
}

int32_t NetInterface::linkPeekRecord(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr) { std::memset(fields, 0, 5 * sizeof(uint32_t)); }
    if (payload != nullptr) { *payload = nullptr; }
    if (handle == nullptr || fields == nullptr || payload == nullptr) { return NI_INVALID_ARGUMENT; }
    NetInterfaceTunnelRecord record = {};
    const int32_t result = reinterpret_cast<NetInterfaceLink*>(handle)->peekRecord(record);
    if (result == NI_READY) { LinkApiRecordFields(record, fields, payload); }
    return result;
}

int32_t NetInterface::linkConsumeRecord(NetInterfaceLinkHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceLink*>(handle)->consumeRecord();
}

int32_t NetInterface::linkSendControl(NetInterfaceLinkHandle handle, const uint32_t* fields, const uint8_t* payload)
{
    if (handle == nullptr || fields == nullptr || fields[0] > UINT16_MAX) { return NI_INVALID_ARGUMENT; }
    NetInterfaceFrame frame = {};
    frame.m_kind = static_cast<uint16_t>(fields[0]);
    frame.m_productId = fields[1];
    frame.m_requestId = (static_cast<uint64_t>(fields[3]) << 32) | fields[2];
    frame.m_messageType = fields[4];
    frame.m_payloadSize = fields[5];
    frame.m_payload = payload;
    return reinterpret_cast<NetInterfaceLink*>(handle)->sendControl(frame);
}

int32_t NetInterface::linkPeekControl(NetInterfaceLinkHandle handle, uint32_t* fields, const uint8_t** payload)
{
    if (fields != nullptr) { std::memset(fields, 0, 6 * sizeof(uint32_t)); }
    if (payload != nullptr) { *payload = nullptr; }
    if (handle == nullptr || fields == nullptr || payload == nullptr) { return NI_INVALID_ARGUMENT; }
    NetInterfaceFrame frame = {};
    const int32_t result = reinterpret_cast<NetInterfaceLink*>(handle)->peekControl(frame);
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

int32_t NetInterface::linkConsumeControl(NetInterfaceLinkHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceLink*>(handle)->consumeControl();
}

int32_t NetInterface::linkPeerInfo(NetInterfaceLinkHandle handle, uint32_t* fields,
    const uint8_t** name, const uint8_t** certificate, const uint8_t** claims)
{
    if (fields != nullptr) { std::memset(fields, 0, 4 * sizeof(uint32_t)); }
    if (name != nullptr) { *name = nullptr; }
    if (certificate != nullptr) { *certificate = nullptr; }
    if (claims != nullptr) { *claims = nullptr; }
    if (handle == nullptr || fields == nullptr || name == nullptr || certificate == nullptr || claims == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceLinkPeer peer = {};
    const int32_t result = reinterpret_cast<NetInterfaceLink*>(handle)->peerInfo(peer);
    if (result == NI_READY)
    {
        fields[0] = peer.m_role;
        fields[1] = static_cast<uint32_t>(std::strlen(peer.m_name));
        fields[2] = peer.m_certificateSize;
        fields[3] = peer.m_claimsSize;
        *name = reinterpret_cast<const uint8_t*>(peer.m_name);
        *certificate = peer.m_certificate;
        *claims = peer.m_claims;
    }
    return result;
}

int32_t NetInterface::linkFinishInput(NetInterfaceLinkHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceLink*>(handle)->finishInput();
}

int32_t NetInterface::linkStatus(NetInterfaceLinkHandle handle)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceLink*>(handle)->status();
}