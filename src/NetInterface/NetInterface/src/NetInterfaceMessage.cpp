#include "NetInterfaceMacro.h"
#include "NetInterfaceMessage.h"

NetInterfaceMessage::NetInterfaceMessage() :
m_budget(0)
{

}

void NetInterfaceMessage::clear()
{
    m_message.clear();
    m_budget = 0;
}