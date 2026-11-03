#include "NetSenderMacro.h"
#include "NetSenderLocalPacket.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <cstring>

/** 不依赖对齐读取大端字
@param [in] value 四字节
@return 无符号值
*/
static uint32_t LocalWord(const uint8_t* value)
{
    return (static_cast<uint32_t>(value[0]) << 24) | (static_cast<uint32_t>(value[1]) << 16) |
        (static_cast<uint32_t>(value[2]) << 8) | value[3];
}

/** 写大端字
@param [out] bytes 四字节
@param [in] value 数值
*/
static void WriteLocalWord(uint8_t* bytes, uint32_t value)
{
    bytes[0] = static_cast<uint8_t>(value >> 24);
    bytes[1] = static_cast<uint8_t>(value >> 16);
    bytes[2] = static_cast<uint8_t>(value >> 8);
    bytes[3] = static_cast<uint8_t>(value);
}

/** 公开通道代次不能全零
@param [in] bytes 16字节
@return 是否含非零
*/
static bool HasLocalGeneration(const uint8_t* bytes)
{
    uint8_t value = 0;
    for (uint32_t index = 0; index < 16; ++index)
    {
        value |= bytes[index];
    }
    return value != 0;
}

/** 校验本地信封，密文本身仍由TLS验证
@param [in] packet 已取得字段
@return 是否合法
*/
static bool ValidLocalPacket(const NetSenderLocalPacketView& packet)
{
    if (packet.m_kind < NET_LOCAL_OPEN || packet.m_kind > NET_LOCAL_END ||
        !HasLocalGeneration(packet.m_source) || packet.m_size > 16384 ||
        (packet.m_size != 0 && packet.m_data == nullptr) ||
        (packet.m_kind == NET_LOCAL_OPEN ? HasLocalGeneration(packet.m_target) : !HasLocalGeneration(packet.m_target)))
    {
        return false;
    }
    if (packet.m_kind == NET_LOCAL_END)
    {
        return packet.m_packet == 0 && packet.m_size == 0;
    }
    return packet.m_packet != 0 && (packet.m_kind == NET_LOCAL_ACK ? packet.m_size == 0 : packet.m_size != 0);
}

int32_t NetSenderLocalPacket::encode(const NetSenderLocalPacketView& packet, uint8_t* output,
    uint32_t capacity, uint32_t& required)
{
    required = 0;
    if (!ValidLocalPacket(packet) || (capacity != 0 && output == nullptr))
    {
        return NI_INVALID_ARGUMENT;
    }
    required = 56 + packet.m_size;
    if (capacity < required)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    std::memcpy(output, "NIP2", 4);
    WriteLocalWord(output + 4, packet.m_kind);
    WriteLocalWord(output + 8, static_cast<uint32_t>(packet.m_packet >> 32));
    WriteLocalWord(output + 12, static_cast<uint32_t>(packet.m_packet));
    std::memcpy(output + 16, packet.m_source, 16);
    std::memcpy(output + 32, packet.m_target, 16);
    WriteLocalWord(output + 48, packet.m_size);
    WriteLocalWord(output + 52, 0);
    if (packet.m_size != 0)
    {
        std::memcpy(output + 56, packet.m_data, packet.m_size);
    }
    return NI_OK;
}

int32_t NetSenderLocalPacket::parse(const uint8_t* bytes, uint32_t size, NetSenderLocalPacketView& packet)
{
    std::memset(&packet, 0, sizeof(packet));
    if (bytes == nullptr || size < 56 || size > 16440 || std::memcmp(bytes, "NIP2", 4) != 0 ||
        LocalWord(bytes + 52) != 0 || LocalWord(bytes + 48) != size - 56)
    {
        return NI_PROTOCOL_ERROR;
    }
    NetSenderLocalPacketView value = {};
    value.m_kind = LocalWord(bytes + 4);
    value.m_packet = (static_cast<uint64_t>(LocalWord(bytes + 8)) << 32) | LocalWord(bytes + 12);
    std::memcpy(value.m_source, bytes + 16, 16);
    std::memcpy(value.m_target, bytes + 32, 16);
    value.m_size = size - 56;
    value.m_data = value.m_size == 0 ? nullptr : bytes + 56;
    if (!ValidLocalPacket(value))
    {
        return NI_PROTOCOL_ERROR;
    }
    packet = value;
    return NI_OK;
}

bool NetSenderLocalPacket::sameGeneration(const uint8_t* left, const uint8_t* right)
{
    return left != nullptr && right != nullptr && std::memcmp(left, right, 16) == 0;
}

NetSenderLocalWindow::NetSenderLocalWindow() : m_sent(0), m_acknowledged(0), m_received(0)
{

}

uint64_t NetSenderLocalWindow::next() const
{
    return m_sent == UINT64_MAX || m_sent - m_acknowledged >= 4 ? 0 : m_sent + 1;
}

int32_t NetSenderLocalWindow::published(uint64_t packet)
{
    if (packet == 0 || packet != next())
    {
        return NI_INVALID_STATE;
    }
    m_sent = packet;
    return NI_OK;
}

int32_t NetSenderLocalWindow::acknowledge(uint64_t packet)
{
    if (packet == 0 || packet > m_sent)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (packet > m_acknowledged)
    {
        m_acknowledged = packet;
    }
    return NI_OK;
}

int32_t NetSenderLocalWindow::received(uint64_t packet)
{
    if (packet == 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (packet <= m_received)
    {
        return NI_NOT_FOUND;
    }
    if (m_received == UINT64_MAX || packet != m_received + 1)
    {
        return NI_PROTOCOL_ERROR;
    }
    m_received = packet;
    return NI_OK;
}

uint64_t NetSenderLocalWindow::receipt() const
{
    return m_received;
}

uint32_t NetSenderLocalWindow::outstanding() const
{
    return static_cast<uint32_t>(m_sent - m_acknowledged);
}