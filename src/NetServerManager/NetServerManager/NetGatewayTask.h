#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "NetGatewayLocalServer.h"
#include <atomic>
#include <string>

// 服务端网关启动配置，先完整校验再交给唯一网络任务
struct NetGatewayConfiguration
{
public:
    // 数值地址、端口及连接容量
    std::string m_address;
    // 监听TCP端口；配置为0时请求系统分配，任务状态为0表示尚未监听
    uint32_t m_port;
    // 同时接纳的物理连接上限
    uint32_t m_peers;
    // 已读取的PEM，不写入日志
    std::string m_certificate;
    // 节点或显式旧链路模式的私钥PEM副本，禁止写入日志
    std::string m_privateKey;
    // 本机服务身份，各产品独立秘密；空表示不启用IPC接入
    std::vector<NetGatewayServiceCredential> m_services;
    // 0常驻，非零为隔离运行自动停止毫秒
    uint32_t m_runForMs;
    // 3为透明管理网关，2为显式旧链路验证模式
    uint32_t m_protocol;
    // 与节点证书匹配的网关身份名称
    std::string m_nodeName;
    // 对其他节点公布的可达数值IP，不使用本地通配监听地址
    std::string m_advertise;
    // 本节点身份与目录缓存的持久化目录
    std::string m_identityDirectory;
    // 启动发现种子，元素分别为数值IP和TCP端口
    std::vector<std::pair<std::string, uint32_t>> m_seeds;

public:
    /** 初始化启动默认值，显式旧链路兼容模式为2
    */
    NetGatewayConfiguration();
};

// 外部只读取原子状态或发停止请求，网络归一个CTask执行者。
class NetGatewayTask : public CTask
{
  public:
    /** 复制配置，不在构造启动网络
    @param [in] configuration 服务器配置
    */
    explicit NetGatewayTask(const NetGatewayConfiguration& configuration);

    /** 在线程中构造、驱动、销毁网关
    */
    virtual void DoTask();

    /** 只置停止标志，由所属执行者回收
    */
    virtual void StopTask();

    /** 实际监听端口
    @return 启动中或失败为0
    */
    uint32_t port() const;

    /** 查询资源回收完成
    @return 是否完成
    */
    bool done() const;

    /** 失败码
    @return 0正常，负数错误
    */
    int32_t error() const;

  private:
    // 任务独占的完整启动配置副本，运行中只读
    NetGatewayConfiguration m_configuration;
    // 跨线程协作停止请求，由网络执行者实际回收资源
    std::atomic<bool> m_stop;
    // 任务已完成最终资源回收，允许生命周期拥有者等待并释放
    std::atomic<bool> m_done;
    // 监听TCP端口；配置为0时请求系统分配，任务状态为0表示尚未监听
    std::atomic<uint32_t> m_port;
    // 最近启动或运行错误，0为正常，负数为NI错误
    std::atomic<int32_t> m_error;
};