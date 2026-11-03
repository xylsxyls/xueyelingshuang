#pragma once
#include "NetEndpointRouter.h"

// 仅供本库注释main及NetWorkTest复用的宿主故障夹具，不参与生产路径。
class NetEndpointProbeSink : public NetEndpointServiceSink
{
public:
    /** 初始化固定观测区及故障开关
    */
    NetEndpointProbeSink();

    /** 保存最后一帧，按故障位模拟拒绝、抛异常或请求停止
    @param [in] endpoint 目标代次
    @param [in] frame 同步借用
    @param [in] origin 可信根
    @return 接纳或故障结果
    */
    virtual int32_t send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin);

    /** 记录关闭次数，可模拟宿主异常
    @param [in] endpoint 被关闭代次
    */
    virtual void close(uint64_t endpoint);

public:
    // 故障位1发送异常、2关闭异常、4发送背压、8发送时请求停止。
    uint32_t m_fault;
    // 已向实际接入层提交的发送观察状态，不能视作远端收到
    uint32_t m_sent;
    // 本用例观察到的终态记录，用于检查关闭不丢失且不重复
    uint32_t m_closed;
    // 同步观察的固定副本；只复制前256字节，测试大包只观察原借用位置。
    uint64_t m_endpoint;
    // 本次实际投递的帧元数据，正文由本用例对应字节存储保留
    NetInterfaceFrame m_frame;
    // 本次投递的可信根来源副本，用于核对来源隔离
    NetEndpointOrigin m_origin;
    // 本用例拥有的消息字节副本，避免在回调外保留失效借用
    uint8_t m_bytes[256];
    // 受控测试暂存的借用正文，仅在所属投递回调期间有效
    const uint8_t* m_borrowed;
    // 借用正文的字节数，用于验证内存范围不被改写
    uint32_t m_borrowedSize;
    // 重入停止测试时借用路由器。
    NetEndpointRouter* m_router;
};

// 只模拟运输生命周期的受控测试宿主，不替代TLS或网络互通验证
class NetEndpointProbeTransport : public NetServerChannel
{
public:
    /** 初始化宿主计数和故障位
    */
    NetEndpointProbeTransport();

    /** 本夹具只监听，不实现客户端连接
    @param [in] address 未使用
    @param [in] port 未使用
    @param [in] options 未使用
    @param [in] listener 未使用
    @param [out] peer 置零
    @return 不支持
    */
    virtual int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
        NetTransportListener* listener, uint64_t& peer);

    /** 模拟接纳监听或失败
    @param [in] address 未使用
    @param [in] port 未使用
    @param [in] options 未使用
    @param [in] listener 借用至shutdown
    @param [in] peers 未使用
    @return 成功或故障
    */
    virtual int32_t listen(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
        NetTransportListener* listener, uint32_t peers);

    /** 可抛异常的宿主推进
    @return 仍有资源
    */
    virtual bool poll();

    /** 本夹具不收发真实帧
    @param [in] peer 未使用
    @param [in] frame 未使用
    @return 成功
    */
    virtual int32_t send(uint64_t peer, const NetInterfaceFrame& frame);

    /** 可抛异常的关闭
    @param [in] peer 未使用
    @param [in] reason 未使用
    */
    virtual void close(uint64_t peer, int32_t reason);

    /** 统计停止并按故障位抛异常
    */
    virtual void shutdown();

    /** 可抛异常的状态查询
    @return 测试端口
    */
    virtual uint32_t localPort() const;

    /** 固定为零
    @return 无真实连接
    */
    virtual uint32_t peerCount() const;

public:
    // 故障1监听异常、2停止异常、4查询异常、8推进异常、16关闭异常、32监听拒绝、64监听宿主自行静止。
    uint32_t m_fault;
    // 模拟运输收到shutdown的次数
    uint32_t m_stopped;
    // 模拟运输收到listen的次数
    uint32_t m_listened;
    // 借用路由器的运输监听者，模拟宿主结束后不再回调
    NetTransportListener* m_listener;
};