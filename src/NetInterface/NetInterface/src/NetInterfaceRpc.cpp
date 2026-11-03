#include "NetInterfaceMacro.h"
#include "NetInterfaceRpc.h"
#include <cstring>

/** 不依赖内存对齐读取网络字节序
@param [in] data 四字节
@return 无符号值
*/
static uint32_t ReadRpcWord(const uint8_t* data)
{
    return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) | (static_cast<uint32_t>(data[2]) << 8) |
           data[3];
}

/** 写出固定网络字节序
@param [out] data 四字节
@param [in] value 数值
*/
static void WriteRpcWord(uint8_t* data, uint32_t value)
{
    data[0] = static_cast<uint8_t>(value >> 24);
    data[1] = static_cast<uint8_t>(value >> 16);
    data[2] = static_cast<uint8_t>(value >> 8);
    data[3] = static_cast<uint8_t>(value);
}

/** 检查两种消息的共同约束
@param [in] kind 消息种类
@param [in] value 时限或状态
@param [in] parent 父路由
@param [in] size 业务体大小
@return 是否合法
*/
static bool ValidRpc(uint16_t kind, uint32_t value, uint64_t parent, uint32_t size)
{
    if (size > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT - 16)
    {
        return false;
    }
    return kind == NI_REQUEST
               ? value != 0 && value <= 60000
               : kind == NI_RESPONSE && value <= NI_RPC_UNAUTHORIZED && parent == 0 && (value == NI_RPC_SUCCESS || size == 0);
}

int32_t NetInterfaceRpc::encode(uint16_t kind, uint32_t value, uint64_t parent, const uint8_t* data, uint32_t size, uint8_t* output,
                                uint32_t capacity, uint32_t& required)
{
    required = 0;
    if (!ValidRpc(kind, value, parent, size) || (size != 0 && data == nullptr) || (capacity != 0 && output == nullptr))
    {
        return NI_INVALID_ARGUMENT;
    }
    required = size + 16;
    if (capacity < required)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    std::memcpy(output, "NRP2", 4);
    WriteRpcWord(output + 4, value);
    WriteRpcWord(output + 8, static_cast<uint32_t>(parent >> 32));
    WriteRpcWord(output + 12, static_cast<uint32_t>(parent));
    if (size != 0)
    {
        std::memcpy(output + 16, data, size);
    }
    return NI_OK;
}

int32_t NetInterfaceRpc::parse(const NetInterfaceFrame& frame, NetInterfaceRpcView& view)
{
    std::memset(&view, 0, sizeof(view));
    if (frame.m_productId == 0 || frame.m_requestId == 0 || frame.m_payload == nullptr || frame.m_payloadSize < 16 ||
        frame.m_payloadSize > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT || std::memcmp(frame.m_payload, "NRP2", 4) != 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    const uint32_t value = ReadRpcWord(frame.m_payload + 4);
    const uint64_t parent = (static_cast<uint64_t>(ReadRpcWord(frame.m_payload + 8)) << 32) | ReadRpcWord(frame.m_payload + 12);
    const uint32_t size = frame.m_payloadSize - 16;
    if (!ValidRpc(frame.m_kind, value, parent, size))
    {
        return NI_PROTOCOL_ERROR;
    }
    view.m_value = value;
    view.m_parent = parent;
    view.m_size = size;
    view.m_data = size == 0 ? nullptr : frame.m_payload + 16;
    return NI_OK;
}

int32_t NetInterfaceRpc::control(const NetInterfaceFrame& frame, NetInterfaceFrame& reply)
{
    const NetInterfaceFrame snapshot = frame;
    std::memset(&reply, 0, sizeof(reply));
    if (snapshot.m_requestId == 0 || snapshot.m_payloadSize != 0 ||
        (snapshot.m_kind == NI_CANCEL
             ? snapshot.m_productId == 0
             : (snapshot.m_kind != NI_PING && snapshot.m_kind != NI_PONG) || snapshot.m_productId != 0 || snapshot.m_messageType != 0))
    {
        return NI_PROTOCOL_ERROR;
    }
    if (snapshot.m_kind != NI_PING)
    {
        return NI_OK;
    }
    reply = snapshot;
    reply.m_kind = NI_PONG;
    reply.m_payload = nullptr;
    return NI_READY;
}