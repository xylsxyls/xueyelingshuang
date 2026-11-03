#include "NetSenderServiceState.h"
#include "NetSenderMacro.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include "NetSenderServiceManaged.h"
#include <cstring>

NetSenderServiceState::NetSenderServiceState(NetSenderLocalInbox& inbox) :
m_inbox(&inbox),
m_local(inbox),
m_listener(nullptr),
m_peer(0),
m_endpoint(0),
m_deadline(0),
m_lastAllocated(0),
m_lastSent(0),
m_lastEvent(0),
m_product(0),
m_started(false),
m_closed(false)
{
    std::memset(m_hello, 0, sizeof(m_hello));
}

NetSenderServiceState::~NetSenderServiceState()
{
    wipe();
}

void NetSenderServiceState::wipe()
{
    volatile uint8_t* bytes = m_hello;
    for (uint32_t index = 0; index < sizeof(m_hello); ++index)
    {
        bytes[index] = 0;
    }
}