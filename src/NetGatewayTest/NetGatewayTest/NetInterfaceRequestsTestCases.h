#pragma once

// 请求期限、唯一终态及编号测试集合，逐用例独立建立预期并检查实际结果
class NetInterfaceRequestsTestCases
{
public:
    /** 客户端A及Cook到Account的B分离，可信来源/产品/父关系校验
    */
    static void nestedRouting();

    /** 全局及端点并发容量、重复请求和重复回复
    */
    static void budgets();

    /** 父子截止收敛，到期回复和已结束父请求均不能成功完成
    */
    static void deadlines();

    /** CANCEL可先于REQUEST，未来取消有界且不误取消较早正常请求
    */
    static void cancellationOrder();

    /** 根客户端或服务重启时清理相关层级，不影响其他客户端
    */
    static void endpointGeneration();

    /** 端点容量、参数、单调时钟及错误后资源清理
    */
    static void argumentBounds();

    /** 平面C++主类入口保留64位，自动编号不回绕，容量拒绝后仍正确关联原号
    */
    static void cBoundary();

    /** 满4096项后任意取消、按截止收敛及断开局部端点，验证索引与剩余请求一致
    */
    static void indexedLifetime();
};