#pragma once

// 通过真实节点路由实现检查容量、快照、预留、重连代次和资源失败。
class NetGatewayNodeLoadTest
{
public:
    /** 执行独立预期并向统一runner报告失败
    */
    static void run();

private:
    /** 检查受控状态下的生产路由选择和OPEN
    @return 0通过，1失败
    */
    static int check();
};