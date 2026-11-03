#pragma once

// 仅注释main及NetWorkTest使用；不生成运行时测试配置或另一个转发算法。
class NetSenderServiceProxyTest
{
public:
    /** 验证实际固定槽位、来源边界、预算、无分配推进及失败收敛
    @return 全部独立预期通过
    */
    static bool run();
};