#include "NetGatewayClientChannelState.h"
#include "NetInterfaceMacro.h"

NetGatewayClientChannelState::NetGatewayClientChannelState(uint32_t product, NetGatewayTransport* external) :
m_transport(external),
m_listener(nullptr),
m_options(),
m_inputSize(0),
m_inputOffset(0),
m_controlCredit(0),
m_linkPeer(0),
m_startedAt(0),
m_blockedDeadline(0),
m_product(product),
m_reason(0),
m_detail(0),
m_phase(NET_GATEWAY_CLIENT_IDLE),
m_polling(false)
{
    m_credit.configure(NET_CHANNEL_MAX_WINDOW);
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