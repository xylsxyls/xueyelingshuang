#pragma once
#include "NetPeerMacro.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <stdint.h>

class NetTransportState;

// 通过已配置字节宿主提供TLS帧运输，协议由NetInterface实现
class NetPeerAPI NetTransport : public NetServerChannel
{
public:
    /** 创建独占传输资源，初始化失败抛异常
    */
    NetTransport();

    /** 收敛本实例回调及句柄；监听对象必须仍存活，不得在回调中销毁
    */
    ~NetTransport();

    /** 监听TCP并使用同源TLS/帧协议；实例只能启动一次
    @param [in] address 数值IPv4/IPv6，0.0.0.0/::须显式指定
    @param [in] port 0至65535，0用于隔离测试
    @param [in] options server必须为1，证书/私钥调用时复制；entropy为空时使用系统强随机
    @param [in] listener 借用至shutdown完成，不可空
    @param [in] maxPeers 最大连接数，1至256，默认64
    @return 0接纳或负数配置/监听错误
    */
    int32_t listen(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
        NetTransportListener* listener, uint32_t maxPeers = 64);

    /** 异步连接；TCP和TLS各自受handshakeTimeoutMs约束
    @param [in] address 数值地址，TLS验证域名仍使用options.hostname
    @param [in] port 1至65535
    @param [in] options server必须为0，调用时复制；随机上下文若提供须活到shutdown
    @param [in] listener 借用至shutdown完成，不可空
    @param [out] peer 成功接纳后的连接号，失败为0；接纳不表示TLS就绪
    @return 0接纳或负数错误
    */
    int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
        NetTransportListener* listener, uint64_t& peer);

    /** 非阻塞驱动；逐连接有界推进，回调只能快速接纳或拒绝
    @return 仍有监听/连接资源为true，停止并完全回收后false
    */
    bool poll();

    /** 复制完整帧到C++有界队列，尚不表示对端收到
    @param [in] peer TLS就绪的连接号
    @param [in] frame 调用期间借用；保持请求的完整64位编号
    @return NI_OK接纳，NI_LIMIT_EXCEEDED背压，其余负数为错误
    */
    int32_t send(uint64_t peer, const NetInterfaceFrame& frame);

    /** 请求关闭，之后不再交付该连接帧；poll中完成唯一终态
    @param [in] peer 连接号，不存在时无操作
    @param [in] reason 终态原因，默认0
    */
    void close(uint64_t peer, int32_t reason = 0);

    /** 请求停止全部连接；回调内只发停止请求，由外层poll收敛
    非回调执行者调用将立即回收TCP句柄并回报终态，不等待对端配合。
    */
    void shutdown();

    /** 实际监听端口
    @return 成功监听的端口，其他状态0
    */
    uint32_t localPort() const;

    /** 包含正在关闭连接的数量
    @return 本实例拥有的协议连接数
    */
    uint32_t peerCount() const;

    /** 为应用握手、设备和本地服务代次生成强随机字节，不建立网络连接
    @param [out] output size非零时不可空
    @param [in] size 单次最多8192字节
    @return OK或INVALID_STATE，失败清空输出，不回退到伪随机
    */
    static int32_t randomBytes(uint8_t* output, uint32_t size);

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @param [in] peer 当前连接代次
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    virtual int32_t outputState(uint64_t peer) const;

private:
    /** 禁止复制所有权
    @param [in] other 不可复制实例
    */
    NetTransport(const NetTransport& other);

    /** 禁止复制所有权
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetTransport& operator=(const NetTransport& other);

public:
    /** 为指定TLS通道配置显式RPC压缩白名单
    @param [in] peer 当前运输代次
    @param [in] types 调用期间借用的业务类型数组
    @param [in] count 类型数量，最多64项
    @param [in] negotiated 是否已通过对端能力协商
    @return NI_OK或参数、状态、内存错误
    */
    virtual int32_t configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated);

private:
    // 本模块独占状态，隔离宿主适配及容器布局
    NetTransportState* m_state;
};