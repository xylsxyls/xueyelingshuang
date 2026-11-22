#include "NetGatewayPeerHistory.h"

NetGatewayPeerHistory::NetGatewayPeerHistory() :
m_highWater()
{

}

void NetGatewayPeerHistory::remember(uint64_t channel)
{
    const uint32_t domain = static_cast<uint32_t>(channel >> 62);
    if (channel > m_highWater[domain])
    {
        m_highWater[domain] = channel;
    }
}

bool NetGatewayPeerHistory::obsolete(uint64_t channel) const
{
    return channel != 0 && channel <= m_highWater[static_cast<uint32_t>(channel >> 62)];
}