#include "NetGatewayDiscoveryState.h"
#include "NetPeerMacro.h"
#include "NetGatewayDiscovery.h"
#include "NetDirectorySnapshot.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetTransportEntropy.h"
#include <chrono>
#include <cstring>

NetGatewayDiscoveryState::NetGatewayDiscoveryState() :
m_tls(),
m_seedCount(0),
m_candidateCount(0),
m_nodeCount(0),
m_current(),
m_peer(0),
m_request(0),
m_waiting(0),
m_sentAt(0),
m_retryAt(0),
m_next(0),
m_running(false),
m_ready(false),
m_closed(false),
m_cached(false),
m_cacheEnabled(false)
{
    m_current.m_structSize = sizeof(m_current);
    m_current.m_version = 1;
}

void NetGatewayDiscoveryState::rebuild()
{
    NetGatewayDiscoveryCandidate previous[128];
    const uint32_t oldCount = m_candidateCount;
    std::memcpy(previous, m_candidates, oldCount * sizeof(previous[0]));
    m_candidateCount = 0;
    for (uint32_t index = 0; index < m_seedCount + m_nodeCount; ++index)
    {
        NetGatewayDiscoveryCandidate candidate = {};
        if (index < m_seedCount)
        {
            candidate.m_point = m_seeds[index];
        }
        else
        {
            const NetInterfaceDirectoryNodeData& node = m_nodes[index - m_seedCount];
            candidate.m_point.m_structSize = sizeof(candidate.m_point);
            candidate.m_point.m_version = 1;
            candidate.m_point.m_port = node.m_port;
            std::memcpy(candidate.m_point.m_address, node.m_address, sizeof(node.m_address));
        }
        if (!NetClientDiscovery::addressKey(candidate.m_point, candidate.m_key))
        {
            continue;
        }
        bool duplicate = false;
        for (uint32_t item = 0; item < m_candidateCount; ++item)
        {
            const NetGatewayDiscoveryCandidate& added = m_candidates[item];
            if (candidate.m_point.m_port == added.m_point.m_port && std::memcmp(candidate.m_key, added.m_key, 16) == 0)
            {
                duplicate = true;
                break;
            }
        }
        if (duplicate)
        {
            continue;
        }
        for (uint32_t item = 0; item < oldCount; ++item)
        {
            if (candidate.m_point.m_port == previous[item].m_point.m_port && std::memcmp(candidate.m_key, previous[item].m_key, 16) == 0)
            {
                candidate.m_failedUntil = previous[item].m_failedUntil;
                break;
            }
        }
        m_candidates[m_candidateCount++] = candidate;
    }
    m_next = m_candidateCount == 0 ? 0 : m_next % m_candidateCount;
}