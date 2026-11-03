#pragma once
#include "NetSenderMacro.h"
#include "NetSenderLocal.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <string>

class NetSenderLinkState;

// ProcessWork只运输NTW3。管理TLS在本适配持有，业务TLS在各逻辑端点持有。
// 宿主拥有ProcessWork收件箱的注册/启动/停止；本类不创建线程或关闭全局IPC。
class NetSenderAPI NetSenderLink
{
public:
    /** 借用已经注册的本机收件箱
    @param [in] inbox 生命周期须覆盖本对象
    */
    explicit NetSenderLink(NetSenderLocalInbox& inbox);

    /** 同一执行者收敛所属代次，监听者仍须有效
    */
    ~NetSenderLink();

    /** 网关本机服务管理监听，服务仍需独立证明，不能仅凭PID注册
    @param [in] options NODE服务端配置及服务授权回调，复制身份材料
    @param [in] listener 借用监听者
    @param [in] maximum 1至64个代次
    @return NI_OK或错误
    */
    int32_t listen(const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener, uint32_t maximum = 32);

    /** 端点通过本机透明网关或服务注册入口建立管理TLS
    @param [in] pid 仅为本机运输地址，不作为身份授权
    @param [in] options CLIENT或SERVICE客户端配置
    @param [in] listener 借用监听者
    @param [out] peer 成功为非零代次；此时还未认证就绪
    @return NI_OK或错误
    */
    int32_t connect(int32_t pid, const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener, uint64_t& peer);

    /** 驱动IPC、管理TLS、期限、心跳及有界回调，禁止重入
    @return 是否启用且未停止
    */
    bool poll();

    /** 排队管理帧，LIMIT_EXCEEDED无副作用，可在下轮重试
    @param [in] peer 已认证代次
    @param [in] frame 管理帧，借用至返回
    @return NI_OK或错误
    */
    int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 排队业务密文，调用者负责通道归属授权
    @param [in] peer 已认证代次
    @param [in] record NTW3业务密文/关闭记录
    @return NI_OK或错误
    */
    int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 复制本次服务注册的公开叶证书，私钥从不由网关下发
    @param [in] peer 服务代次
    @param [out] certificate 成功为PEM，失败清空
    @return NI_READY、NI_NEED_MORE或错误
    */
    int32_t serviceGrant(uint64_t peer, std::string& certificate) const;

    /** 请求关闭，回调在poll或shutdown中唯一交付
    @param [in] peer 代次
    @param [in] reason 原因
    */
    void close(uint64_t peer, int32_t reason = 0);

    /** 停止本实例；宿主随后才可停止ProcessWork回调
    */
    void shutdown();

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
    NetSenderLink(const NetSenderLink& other);

    /** 禁止赋值持有活动协议状态的对象
    @param [in] other 不可赋值的实例
    @return 不可调用
    */
    NetSenderLink& operator=(const NetSenderLink& other);
    // 本模块分配的独占本机管理运输状态，析构前收敛下层IPC
    NetSenderLinkState* m_state;
};