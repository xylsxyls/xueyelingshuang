#include "NetInterfaceMacro.h"
#include "NetInterfaceMessage.h"
#include "NetInterfaceTypes.h"

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