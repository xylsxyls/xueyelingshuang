#pragma once
#include "NetInterfaceTypes.h"

// 单条业务连接的本地心跳与接收期限，单位均为单调毫秒
struct NetInterfaceAPI NetHeartbeatConfig
{
public:
    // 空闲到发起PING的间隔，默认30000毫秒
    uint32_t m_pingIntervalMs;
    // PING交付后允许等待匹配PONG的时间，默认60000毫秒
    uint32_t m_pongTimeoutMs;
    // 最后一次接收活动至关闭的总无输入期限，默认90000毫秒
    uint32_t m_receiveTimeoutMs;

public:
    /** 创建采用默认期限的独立配置
    */
    NetHeartbeatConfig();

    /** 核对三个非零期限，接收期限须晚于首次心跳
    @return 配置可用时返回true
    */
    bool valid() const;

    /** 判断现有接收或应答期限是否已到，配置须已经过valid校验
    @param [in] idleMs 最后一次成功接收以来经过的单调毫秒
    @param [in] pongWaitMs 已交付PING以来经过的单调毫秒，无待答PING时传0
    @return 任一期限到达或超过时为true
    */
    bool expired(uint64_t idleMs, uint64_t pongWaitMs) const;
};