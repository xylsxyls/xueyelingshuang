#pragma once
#include "NetRelayPoolConfig.h"
#include "NetPeerMacro.h"
#include "NetInterface/NetInterfaceAPI.h"

class NetLinkTransportState;

// 管理运输通过字节宿主承载 TLS 和 NTW3。网关处理管理消息，业务 TLS 状态由逻辑通道端点持有。
class NetPeerAPI NetLinkTransport
{
public:
    /** 创建未启动运输层，不建线程
    */
    NetLinkTransport();

    /** 使用独立的传输资源配置创建实例
    @param [in] config 当前实例的物理池、推进预算与期限
    */
    explicit NetLinkTransport(const NetLinkTransportConfig& config);

    /** 同一执行者收敛回调，监听者须仍存活
    */
    ~NetLinkTransport();

    /** 启动前配置本层实际连接记录池；底层运输的存储由其自身配置
    @param [in] config 真实槽数及增长策略；由唯一生命周期owner串行调用
    @return NI_OK或配置/内存/状态错误，失败保留旧池
    */
    int32_t configurePeerPool(const LockFreeHashMapPoolConfig& config);

    /** 监听管理和不透明业务通道，只启动一次
    @param [in] address 数值地址
    @param [in] port 0隔离测试或1至65535
    @param [in] options server必须1；公网角色掩码禁止SERVICE
    @param [in] listener 借用至shutdown
    @param [in] maxPeers 非零预留提示，实际连接存储由宿主池配置控制
    @return NI_OK或错误
    */
    int32_t listen(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
        NetLinkTransportListener* listener, uint32_t maxPeers = 64);

    /** 异步连接数值地址；空地址指针或非法端口可更正后重试，管理验证成功才回报ready
    @param [in] address 数值地址
    @param [in] port 1至65535
    @param [in] options server必须0
    @param [in] listener 借用至shutdown
    @param [out] peer 成功非零，否则0
    @return NI_OK或错误
    */
    int32_t connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
        NetLinkTransportListener* listener, uint64_t& peer);

    /** 驱动网络、管理认证、心跳、背压和关闭，禁止重入
    @return 是否还有资源
    */
    bool poll();

    /** 向已认证管理连接发控制消息，调用者检查角色及操作权限
    @param [in] peer 代次
    @param [in] frame 管理帧
    @return NI_OK、NI_LIMIT_EXCEEDED未接纳或错误
    */
    int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 向已经授权的逻辑路由运输密文字节
    @param [in] peer 物理管理代次
    @param [in] record 完整外层记录
    @return NI_OK、NI_LIMIT_EXCEEDED未接纳或错误
    */
    int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 关闭代次，不保存密文等待重连，不重放业务
    @param [in] peer 代次
    @param [in] reason 原因
    */
    void close(uint64_t peer, int32_t reason = 0);

    /** 收敛全部资源，在回调中仅发停止请求
    */
    void shutdown();

    /** 实际监听端口
    @return 未监听时0
    */
    uint32_t localPort() const;

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @param [in] peer 当前连接代次
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    int32_t outputState(uint64_t peer) const;

private:
    /** 禁止复制持有活动协议状态的对象
    @param [in] other 不可复制的实例
    */
    NetLinkTransport(const NetLinkTransport& other);

    /** 禁止赋值持有活动协议状态的对象
    @param [in] other 不可赋值的实例
    @return 不可调用
    */
    NetLinkTransport& operator=(const NetLinkTransport& other);
    // 本模块分配的独占管理运输状态，不向调用方暴露容器布局
    NetLinkTransportState* m_state;
};