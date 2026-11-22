#pragma once

// 仅NetGatewayTest使用；不生成运行时测试配置或另一个转发算法。
class NetSenderServiceProxyTest
{
public:
    /** 验证实际存储池、来源边界、预算、无分配推进及失败收敛
    @param [in] memoryAudit 只在独立进程启用CRT分配失败注入
    @return 全部独立预期通过
    */
    static bool run(bool memoryAudit = false);

    /** 独立进程执行真实代理的分配失败及泄漏审计
    @return 0通过，其余失败
    */
    static int audit();
};