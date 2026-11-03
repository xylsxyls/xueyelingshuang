#include "NetGatewayClientChannelState.h"
#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceCredit.h"
#include "NetInterfaceDirectory.h"
#include "NetInterfaceConnection.h"
#include "NetInterfaceTransport.h"
#include "NetGatewayClientChannel.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include <algorithm>
#include <chrono>
#include <cstring>

NetGatewayClientChannelState::NetGatewayClientChannelState(uint32_t product, NetGatewayTransport* external) :
m_transport(external),
m_listener(nullptr),
m_options(),
m_inputSize(0),
m_inputOffset(0),
m_controlCredit(0),
m_linkPeer(0),
m_startedAt(0),
m_blockedSince(0),
m_product(product),
m_reason(0),
m_detail(0),
m_phase(NET_GATEWAY_CLIENT_IDLE),
m_polling(false)
{

}

void NetGatewayClientChannelState::fail(int32_t reason)
{
    if (m_phase == NET_GATEWAY_CLIENT_IDLE || m_phase >= NET_GATEWAY_CLIENT_CLOSING)
    {
        return;
    }
    m_phase = NET_GATEWAY_CLIENT_CLOSING;
    m_reason = reason;
    m_detail = m_connection ? m_connection->detail() : 0;
}