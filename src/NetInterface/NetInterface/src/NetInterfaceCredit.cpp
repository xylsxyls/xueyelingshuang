#include "NetInterfaceMacro.h"
#include "NetInterfaceCredit.h"
#include "NetInterface.h"
#include "NetInterfaceGatewayTypes.h"
#include "ProtoMessage/ProtoMessageAPI.h"

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
    bytes = 65536;
    Variant value;
    if (!message.getKeyValue(NI_GATEWAY_WINDOW, value, group)) { return NI_OK; }
    if (value.type() != Variant::INT32) { return NI_PROTOCOL_ERROR; }
    const int32_t number = static_cast<int32_t>(value);
    if (number < 65536 || number > 1048576 || number % 16384 != 0) { return NI_PROTOCOL_ERROR; }
    bytes = static_cast<uint32_t>(number);
    return NI_OK;
}

int32_t NetInterface::creditConfigure(NetInterfaceCreditHandle handle, uint32_t bytes)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceCredit*>(handle)->configure(bytes);
}

int32_t NetInterface::creditConfigureMessage(NetInterfaceCreditHandle handle, const uint8_t* input, uint32_t size)
{
    if (handle == nullptr || input == nullptr || size == 0 || size > 16384) { return NI_INVALID_ARGUMENT; }
    try
    {
        ProtoMessage message;
        uint32_t bytes = 0;
        if (!message.from(std::string(reinterpret_cast<const char*>(input), size)) ||
            NetInterfaceCredit::window(message, bytes) != NI_OK)
        {
            return NI_PROTOCOL_ERROR;
        }
        return reinterpret_cast<NetInterfaceCredit*>(handle)->configure(bytes);
    }
    catch (...) { return NI_NO_MEMORY; }
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

int32_t NetInterface::creditCreate(NetInterfaceCreditHandle* handle)
{
    if (handle == nullptr) { return NI_INVALID_ARGUMENT; }
    *handle = nullptr;
    try { *handle = reinterpret_cast<NetInterfaceCreditHandle>(new NetInterfaceCredit()); return NI_OK; }
    catch (...) { return NI_NO_MEMORY; }
}

void NetInterface::creditDestroy(NetInterfaceCreditHandle handle)
{
    delete reinterpret_cast<NetInterfaceCredit*>(handle);
}

int32_t NetInterface::creditGet(NetInterfaceCreditHandle handle, uint32_t* fields)
{
    if (handle == nullptr || fields == nullptr) { return NI_INVALID_ARGUMENT; }
    NetInterfaceCredit* credit = reinterpret_cast<NetInterfaceCredit*>(handle);
    fields[0] = credit->available();
    fields[1] = credit->pending();
    return credit->status();
}

int32_t NetInterface::creditSent(NetInterfaceCreditHandle handle, uint32_t size)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceCredit*>(handle)->sent(size);
}

int32_t NetInterface::creditGrant(NetInterfaceCreditHandle handle, uint32_t size)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceCredit*>(handle)->grant(size);
}

int32_t NetInterface::creditReceived(NetInterfaceCreditHandle handle, uint32_t size)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceCredit*>(handle)->received(size);
}

int32_t NetInterface::creditConsume(NetInterfaceCreditHandle handle, uint32_t size)
{
    return handle == nullptr ? NI_INVALID_ARGUMENT : reinterpret_cast<NetInterfaceCredit*>(handle)->consume(size);
}