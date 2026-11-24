#include "NetInterfaceCredit.h"
#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceGatewayTypes.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"

NetInterfaceCredit::NetInterfaceCredit() :
m_credit()
{

}

int32_t NetInterfaceCredit::configure(uint32_t bytes)
{
    return m_credit.configure(bytes);
}

uint32_t NetInterfaceCredit::capacity() const
{
    return m_credit.capacity();
}

int32_t NetInterfaceCredit::window(const ProtoMessage& message, uint32_t& bytes, int32_t group)
{
    const Variant* value = message.findValue(NI_GATEWAY_WINDOW, group);
    bytes = NET_CHANNEL_DEFAULT_WINDOW;
    if (value == nullptr)
    {
        return NI_OK;
    }
    if (value->type() != Variant::INT32)
    {
        return NI_PROTOCOL_ERROR;
    }
    const int32_t number = static_cast<int32_t>(*value);
    if (number < static_cast<int32_t>(NET_CHANNEL_DEFAULT_WINDOW) || number > static_cast<int32_t>(NET_CHANNEL_MAX_WINDOW) ||
        number % static_cast<int32_t>(NET_CHANNEL_PAYLOAD_LIMIT) != 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    bytes = static_cast<uint32_t>(number);
    return NI_OK;
}

uint32_t NetInterfaceCredit::available() const
{
    return m_credit.available();
}

int32_t NetInterfaceCredit::sent(uint32_t size)
{
    return m_credit.sent(size);
}

int32_t NetInterfaceCredit::grant(uint32_t size)
{
    return m_credit.grant(size);
}

int32_t NetInterfaceCredit::received(uint32_t size)
{
    return m_credit.received(size);
}

uint32_t NetInterfaceCredit::pending() const
{
    return m_credit.pending();
}

int32_t NetInterfaceCredit::consume(uint32_t size)
{
    return m_credit.consume(size);
}

int32_t NetInterfaceCredit::status() const
{
    return m_credit.status();
}