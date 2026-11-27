#include "NetInterfaceMacro.h"
#include "NetInterfaceMessage.h"
#include "NetInterfaceTypes.h"
#include <cstring>

/** 转换本机整数与通用 ProtoMessage 中的小端整数表示；同一转换用于两个方向
@param [in] value 32 位有符号或 64 位无符号整数
@return 小端机器上保持原值，大端机器上反转字节序
*/
template <typename Integer>
static Integer MessageLittleEndian(Integer value)
{
    uint8_t bytes[sizeof(Integer)];
    const uint64_t bits = static_cast<uint64_t>(value);
    for (size_t index = 0; index < sizeof(bytes); ++index)
    {
        bytes[index] = static_cast<uint8_t>(bits >> (index * 8));
    }
    Integer result;
    std::memcpy(&result, bytes, sizeof(result));
    return result;
}

Variant NetInterfaceMessage::encodeInteger(int32_t value)
{
    return Variant(MessageLittleEndian(value));
}

Variant NetInterfaceMessage::encodeInteger(uint64_t value)
{
    return Variant(MessageLittleEndian(value));
}

int32_t NetInterfaceMessage::decodeInt32(const Variant& value)
{
    return MessageLittleEndian(static_cast<int32_t>(value));
}

uint64_t NetInterfaceMessage::decodeUint64(const Variant& value)
{
    return MessageLittleEndian(static_cast<uint64_t>(value));
}

NetInterfaceMessage::NetInterfaceMessage() :
m_budget(0),
m_budgetLimit(NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT),
m_errorLimit(4096)
{

}

bool NetInterfaceMessage::canCharge(uint64_t cost) const
{
    const uint64_t maximum = m_budgetLimit < 0 ? UINT64_MAX : static_cast<uint64_t>(m_budgetLimit);
    return cost <= maximum - m_budget;
}

void NetInterfaceMessage::clear()
{
    m_message.clear();
    m_budget = 0;
}