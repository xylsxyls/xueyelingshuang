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

#if 0
#include <cstdio>
#include <stdexcept>

/** 检查实际四域水位；默认关闭，验证时仅在隔离副本启用main。
@param [in] value 独立预期
@return 通过断言数1，失败抛出
*/
static uint32_t CheckHistory(bool value)
{
    if (!value)
    {
        throw std::runtime_error("Gateway peer history contract failed");
    }
    return 1;
}

/** 超过旧8192上限、跳号、四域隔离、未来号及物理代次重新开始。
@return 0通过，1失败
*/
int main()
{
    try
    {
        uint32_t checks = 0;
        NetGatewayPeerHistory history;
        checks += CheckHistory(sizeof(history) == 32 && !history.obsolete(0) && !history.obsolete(1));
        for (uint32_t domain = 0; domain < 4; ++domain)
        {
            const uint64_t prefix = static_cast<uint64_t>(domain) << 62;
            for (uint32_t number = 2; number <= 20000; number += 2)
            {
                const uint64_t channel = prefix | number;
                checks += CheckHistory(!history.obsolete(channel));
                history.remember(channel);
                checks += CheckHistory(history.obsolete(channel) && history.obsolete(prefix | (number - 1)));
                history.remember(prefix | (number - 1));
                checks += CheckHistory(!history.obsolete(channel + 1));
            }
            checks += CheckHistory(history.obsolete(prefix | 1) && !history.obsolete(prefix | 20001));
        }
        history.remember(UINT64_MAX);
        checks += CheckHistory(history.obsolete(UINT64_MAX) && !history.obsolete(0));
        NetGatewayPeerHistory next;
        checks += CheckHistory(!next.obsolete(1) && !next.obsolete(UINT64_MAX));
        std::printf("Gateway peer history checks=%u PASS\n", checks);
        return 0;
    }
    catch (...)
    {
        return 1;
    }
}

#endif