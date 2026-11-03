#include "NetSenderIpcState.h"
#include "NetSenderMacro.h"
#include "NetSenderIpcChannel.h"
#include "CSystem/CSystemAPI.h"

NetSenderIpcState::NetSenderIpcState(): m_payloadLimit(0)
{

}

void NetSenderIpcState::clear()
{
    m_channel.reset();
    m_payloadLimit = 0;
}