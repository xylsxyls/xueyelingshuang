#pragma once
#include "NetSenderLocal.h"
#include "NetInterface/NetInterfaceAPI.h"

// 桌面业务进程的IPC适配器；会话协议继续由NetSession承担，不另写请求/超时算法。
class NetSenderAPI NetSenderClientChannel : public NetClientChannel, private NetSenderLocalListener
{
  public:
    /** 借用已经注册的收件箱；宿主负责ProcessWork启动/停止
    @param [in] inbox 生命周期覆盖本对象
    @param [in] gatewayPid 要连接的本机NetClientManager进程号，正数
    */
    NetSenderClientChannel(NetSenderLocalInbox& inbox, int32_t gatewayPid);

    /** 收敛本通道，不销毁或停止宿主ProcessWork
    */
    ~NetSenderClientChannel();

    /** 连接固定的本机PID，仍验证TLS域名/证书；address/port为统一入口保留参数
    @param [in] address 非空，可传local
    @param [in] port 可传1，不作为PID
    @param [in] options 本机网关客户端TLS配置
    @param [in] listener 借用至shutdown
    @param [out] peer 非零代次或0
    @return OK启动或负数
    */
    virtual int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options, NetTransportListener* listener,
                            uint64_t& peer);

    /** 非阻塞推进IPC/TLS
    @return 本代仍运行
    */
    virtual bool poll();

    /** 复制完整帧至有界队列
    @param [in] peer 本代通道
    @param [in] frame 借用帧
    @return OK或负数
    */
    virtual int32_t send(uint64_t peer, const NetInterfaceFrame& frame);

    /** 异步关闭
    @param [in] peer 本代通道
    @param [in] reason 原因
    */
    virtual void close(uint64_t peer, int32_t reason = 0);

    /** 收敛本通道，收件箱仍由宿主拥有
    */
    virtual void shutdown();

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @param [in] peer 当前连接代次
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    virtual int32_t outputState(uint64_t peer) const;

  private:
    /** 将TLS就绪转交同源会话
    @param [in] peer 通道
    @param [in] pid 对端声明
    */
    virtual void onLocalReady(uint64_t peer, int32_t pid);

    /** 转交完整帧及背压
    @param [in] peer 通道
    @param [in] frame 借用帧
    @return 接纳状态
    */
    virtual bool onLocalFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** 唯一关闭回报
    @param [in] peer 通道
    @param [in] reason 原因
    */
    virtual void onLocalClosed(uint64_t peer, int32_t reason);

    /** 强随机源
    @param [in] context 忽略
    @param [out] output 字节
    @param [in] size 长度
    @return 0成功
    */
    static int32_t Entropy(void* context, uint8_t* output, uint32_t size);

  private:
    /** 禁止复制独占本地传输。
    @param [in] other 原对象
    */
    NetSenderClientChannel(const NetSenderClientChannel& other);

    /** 禁止赋值。
    @param [in] other 原对象
    @return 不可调用
    */
    NetSenderClientChannel& operator=(const NetSenderClientChannel& other);

    // 借用到shutdown完成，实际注册/注销归宿主。
    NetSenderLocalInbox* m_inbox;
    // 本模块独占，替换和析构都在本模块释放。
    NetSenderLocal* m_local;
    // 借用的会话监听者，必须保持到shutdown返回
    NetTransportListener* m_listener;
    // 固定的本机网关PID，仅用于寻址，不能替代TLS身份校验
    int32_t m_pid;
    // 当前本机运输代次，0表示尚未连接或已经关闭
    uint64_t m_peer;
    // 本代已关闭，阻止重复向会话发出终态
    bool m_closed;
};