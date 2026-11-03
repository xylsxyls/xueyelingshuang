#pragma once
#ifndef __EMSCRIPTEN__
#include "NetWork/NetWorkAPI.h"
#include <vector>

// 端点路由实际投递的独占测试快照
struct NetEndpointRouterTestFrame
{
public:
    // 实际路由或服务端点编号，用于核对投递目标
    uint64_t m_endpoint;
    // 本次实际投递的帧元数据，正文由本用例对应字节存储保留
    NetInterfaceFrame m_frame;
    // 本次投递的可信根来源副本，用于核对来源隔离
    NetInterfaceOrigin m_origin;
    // 本用例拥有的独立预期正文，用于逐字节核对传输结果
    std::vector<uint8_t> m_payload;

public:
    /** 保存实际投递，指针不跨越输入借用期
    @param [in] endpoint 投递目标
    @param [in] frame 借用帧
    @param [in] origin 验证后的来源
    */
    NetEndpointRouterTestFrame(uint64_t endpoint, const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin);

    /** 返回当前自有内容视图
    @return 借用至本对象修改/析构
    */
    NetInterfaceFrame view() const;
};

// 仅模拟业务接收边界，保留真实路由交付的帧和来源
class NetEndpointRouterTestSink : public NetEndpointServiceSink
{
public:
    /** 记录路由器真实发送的端点、帧及可信来源
    @param [in] endpoint 网关已分配的逻辑端点号
    @param [in] frame 本次调用期间借用的完整帧，背压时由调用方保留
    @param [in] origin 本次调用期间借用的网关可信来源
    @return NI_OK表示本次已记录
    */
    virtual int32_t send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin);

    /** 记录路由器要求关闭的逻辑端点
    @param [in] endpoint 网关已分配的逻辑端点号
    */
    virtual void close(uint64_t endpoint);

    // 本测试服务边界实际收到的完整帧副本
    std::vector<NetEndpointRouterTestFrame> m_frames;
    // 本用例观察到的终态记录，用于检查关闭不丢失且不重复
    std::vector<uint64_t> m_closed;
};

// 进程内端点路由与服务依赖测试集合，逐用例独立建立预期并检查实际结果
class NetEndpointRouterTestCases
{
public:
    /** 已认证外部端点同号隔离、父子回程、公开来源定向事件及非法父号拒绝
    */
    static void attachedEndpoints();

    /** 路由关闭公平性、宿主异常、文件限额/终态背压、父子收敛与授权分组隔离
    */
    static void routerLifecycle();
};
#endif