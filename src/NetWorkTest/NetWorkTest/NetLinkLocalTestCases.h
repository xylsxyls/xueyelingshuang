#pragma once
#ifndef __EMSCRIPTEN__
#include "NetSender/NetSenderAPI.h"

// 真实ProcessWork管理链路的握手、权限和控制消息观察者
class NetLinkLocalTestProbe : public NetLinkTransportListener
{
public:
    /** 创建未就绪的真实IPC/网关联调探针
    */
    NetLinkLocalTestProbe();

    /** 核对真实管理链路的连接号和已验证节点身份
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] identity 回调期间借用的已验证管理身份
    */
    virtual void onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity);

    /** 独立核对发现响应、请求号和未注册服务拒绝结果
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] frame 本次调用期间借用的完整帧，背压时由调用方保留
    @return true已消费，断言失败由m_failed单独记录
    */
    virtual bool onLinkControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 标记管理控制用例中不应出现的透明业务记录
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] record 回调期间借用的完整透明记录，背压时不消费
    @return true已消费，同时将意外数据记录为测试失败
    */
    virtual bool onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 核对管理用例完成后的连接号及正常关闭原因
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] reason 本连接或服务的NI关闭原因
    */
    virtual void onLinkClosed(uint64_t peer, int32_t reason);

    /** 向真实NetClientManager发起发现及未注册服务拒绝测试
    @param [in] pid 由拥有子进程的宿主提供
    @return 0通过，否则失败
    */
    static int childMain(int32_t pid);

    /** 运行真实服务注册证书复制故障测试
    @param [in] pid 隔离测试节点PID
    @return 0通过，否则失败
    */
    static int serviceGrantChildMain(int32_t pid);

public:
    // 借用本用例实际本机管理链路，监听者先于它构造
    NetSenderLink* m_link;
    // 当前用例使用的真实运输代次，不以PID替代
    uint64_t m_peer;
    // 当前受控握手或控制往返步骤
    uint32_t m_step;
    // 本用例首次失败后保留的错误标志，后续成功不能清除
    bool m_failed;
    // 本用例观察到的实际就绪状态，不代替业务响应成功
    bool m_ready;
};
#endif