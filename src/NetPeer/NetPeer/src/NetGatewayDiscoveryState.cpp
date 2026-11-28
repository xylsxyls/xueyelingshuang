#include "NetGatewayDiscoveryState.h"
#include <cstring>
#include <unordered_map>
#include <unordered_set>

NetGatewayDiscoveryState::NetGatewayDiscoveryState(const NetGatewayDiscoveryConfig& config) :
m_config(config), m_cache(config.m_cacheWriteMs),
m_tls(),
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
m_cached(false)
{
    m_current.m_structSize = sizeof(m_current);
    m_current.m_version = 1;
}

int32_t NetGatewayDiscoveryState::rebuild()
{
    try
    {
        std::vector<NetGatewayDiscoveryCandidate> merged;
        // 地址键包含端口；使用单线程哈希索引避免候选增长后的平方级合并。
        std::unordered_map<std::string, uint64_t> previous;
        std::unordered_set<std::string> selected;
        for (const auto& old : m_candidates)
        {
            std::string key(reinterpret_cast<const char*>(old.m_key), 16);
            key.append(reinterpret_cast<const char*>(&old.m_point.m_port), sizeof(old.m_point.m_port));
            previous.insert(std::make_pair(key, old.m_failedUntil));
        }
        if (m_seeds.size() > UINT32_MAX || m_nodes.size() > UINT32_MAX - m_seeds.size()) { return NI_INVALID_ARGUMENT; }
        const size_t total = m_seeds.size() + m_nodes.size();
        merged.reserve(total);
        for (size_t index = 0; index < total; ++index)
        {
            NetGatewayDiscoveryCandidate candidate = {};
            if (index < m_seeds.size()) { candidate.m_point = m_seeds[index]; }
            else
            {
                const auto& node = m_nodes[index - m_seeds.size()];
                candidate.m_point.m_structSize = sizeof(candidate.m_point);
                candidate.m_point.m_version = 1;
                candidate.m_point.m_port = node.m_port;
                std::memcpy(candidate.m_point.m_address, node.m_address, sizeof(node.m_address));
            }
            if (!NetClientDiscovery::addressKey(candidate.m_point, candidate.m_key)) { continue; }
            std::string key(reinterpret_cast<const char*>(candidate.m_key), 16);
            key.append(reinterpret_cast<const char*>(&candidate.m_point.m_port), sizeof(candidate.m_point.m_port));
            if (!selected.insert(key).second) { continue; }
            const auto found = previous.find(key);
            if (found != previous.end()) { candidate.m_failedUntil = found->second; }
            merged.push_back(candidate);
        }
        m_candidates.swap(merged);
        m_next = m_candidates.empty() ? 0 : m_next % m_candidates.size();
        return NI_OK;
    }
    catch (...) { return NI_NO_MEMORY; }
}