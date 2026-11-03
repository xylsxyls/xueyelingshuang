#pragma once
#include "NetInterface/NetInterfaceAPI.h"
#include "NetSenderClientChannel.h"
#include "NetSenderGatewayTransport.h"

// 桌面Sender的后台协议状态；提交队列拥有者独占访问，不包含网络实现
class NetSenderGatewayState
{
  public:
    /** 不启动线程或网络
    */
    NetSenderGatewayState();

    /** 桌面缺省熵来自系统；临时进程内分支由NetWork宿主组合提供
    @param [in] options 调用者TLS配置，只复制POD，不保存临时引用
    @return 已补齐桌面强随机回调的配置
    */
    static NetInterfaceTlsOptions clientTls(const NetInterfaceTlsOptions& options);

  public:
    // 本Sender独占的IPC收件箱，租约释放并收敛回调后才能销毁
    NetSenderLocalInbox m_inbox;
    // 显式旧链路模式的独占IPC会话通道，生命周期覆盖m_session
    std::unique_ptr<NetSenderClientChannel> m_channel;
    // 当前服务接入的独占IPC重连工厂，借用m_inbox
    std::unique_ptr<NetSenderGatewayTransportFactory> m_serviceFactory;
    // 本收件箱已经接入ProcessWork分派Hub，不表示网关或业务就绪
    bool m_registered;
    // 本Sender的一份接收器租用，默认自管或显式外部共享
    bool m_receiverLease;
    // 声明在通道之后，析构先停止会话，再释放借用通道及收件箱。
    NetClientSession m_session;
    // owner统一分派，生产调用只接纳本地命令
    NetSenderDispatch m_dispatch;
    // 真实桌面地址，仅后台配置，0按租约发现
    int32_t m_backendPid;
    // 后台选定的服务产品号，0为公共Account服务
    uint32_t m_serviceProduct;
    // 协议执行者保存的最近错误；单次提交仍以直接返回值为准
    int32_t m_lastError;
};