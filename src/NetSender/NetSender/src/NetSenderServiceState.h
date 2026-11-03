#pragma once
#include "NetSenderService.h"

class NetSenderServiceManaged;

// 业务服务接入状态，当前管理协议与显式旧链路实现共用外层入口
class NetSenderServiceState
{
  public:
    /** 初始化全部标量，不依赖隐式值初始化
    @param [in] inbox 借用收件箱
    */
    explicit NetSenderServiceState(NetSenderLocalInbox& inbox);

    /** 擦除凭据
    */
    ~NetSenderServiceState();

    /** 固定清除48字节注册体
    */
    void wipe();

  public:
    // 借用至服务停止及ProcessWork接收收敛
    NetSenderLocalInbox* m_inbox;
    // 当前管理协议服务实现；非空时由该实例处理服务路由与内部依赖
    std::unique_ptr<NetSenderServiceManaged> m_managed;
    // 显式旧链路模式的本机TLS运输，借用m_inbox
    NetSenderLocal m_local;
    // 借用业务服务监听者，必须存活到shutdown返回
    NetSenderServiceListener* m_listener;
    // 显式旧链路的本机运输代次，未连接为0
    uint64_t m_peer;
    // 显式旧链路经网关授权的服务端点，未就绪或已关闭为0
    uint64_t m_endpoint;
    // 显式旧链路登记的认证截止单调毫秒，未发HELLO为0
    uint64_t m_deadline;
    // 本服务已分配或已成功发送的请求号高水位，不复用旧编号
    uint64_t m_lastAllocated;
    // 本服务最后成功发送的请求号，防止倒序或重复请求
    uint64_t m_lastSent;
    // 成功接纳的事件编号高水位，与请求号独立递增
    uint64_t m_lastEvent;
    // 本业务服务产品号，0表示公共Account服务
    uint32_t m_product;
    // 显式旧链路的48字节登记体，发送或失败后立即擦除
    uint8_t m_hello[48];
    // 已消费本实例连接机会，失败后不可重新复用
    bool m_started;
    // 已进入终态，不再接纳新的业务发送
    bool m_closed;
};