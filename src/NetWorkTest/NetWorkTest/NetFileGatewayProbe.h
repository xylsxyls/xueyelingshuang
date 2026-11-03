#pragma once
#include "NetSenderTestClient.h"
#ifdef _WIN32
#include "NetSender/NetSenderAPI.h"
#include "NetWork/NetWorkAPI.h"
#include <memory>

// 两个小文件方向的实际进程组合验证，不承载生产资源权限或磁盘存储策略。
class NetFileGatewayProbe : public NetSessionListener, public NetSenderServiceListener
{
  public:
    /** 构造一名测试端
    @param [in] server 是否独立服务进程
    */
    explicit NetFileGatewayProbe(bool server);

    /** 兜底协作收敛本测试拥有的I/O
    */
    ~NetFileGatewayProbe();

    /** 请求并有界观察本用例资源收尾；重复调用不重复清理
    @return 文件与网关已收敛且未发现失败
    */
    bool finish();

    /** 运行客户端或服务端，实际网关程序由脚本拥有
    @param [in] server 是否服务
    @param [in] pid 对应本机网关PID
    @return 0通过或非零失败
    */
    static int run(bool server, int32_t pid);

    /** 同一执行者推进共享文件队列、响应和网络输出
    */
    void pollFiles();

    /** 桌面接入就绪
    @param [in] generation 代次
    @param [in] maxRequests 上限
    @param [in] maxChunk 上限
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 接纳文件OPEN结果
    @param [in] generation 代次
    @param [in] requestId 请求
    @param [in] product 产品
    @param [in] type 命令
    @param [in] status RPC结果
    @param [in] data 状态体
    @param [in] size 长度
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status,
                                 const uint8_t* data, uint32_t size);

    /** 路由当前流数据，真实持久写没完成时返回背压
    @param [in] generation 代次
    @param [in] frame 帧
    @return 是否接纳
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 记录唯一正常关闭
    @param [in] generation 代次
    @param [in] reason 原因
    @param [in] detail 诊断
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

    /** 服务认证完成
    @param [in] endpoint 服务代次
    */
    virtual void onServiceReady(uint64_t endpoint);

    /** 基础测试资源只允许fixture键，授权条件固定在测试，不写生产默认放行
    @param [in] frame 已认证网关帧
    @param [in] origin 根来源
    @return 消费或无副作用背压
    */
    virtual bool onServiceFrame(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin);

    /** 记录服务关闭
    @param [in] reason 原因
    */
    virtual void onServiceClosed(int32_t reason);

  private:
    /** 在文件工作执行者异步打开当前阶段文件
    @return 是否接纳
    */
    bool openFile();

    /** 客户端构造下一OPEN，不自写二进制协议
    @return 是否接纳
    */
    bool sendOpen();

    /** 当前单调时间
    @return 毫秒
    */
    static uint64_t now();

  public:
    // finish已执行，析构不重复清理
    bool m_finished;
    // 本探针是否扮演接收服务，客户端使用同一协议的相反方向
    bool m_server;
    // 本用例观察到的实际就绪状态，不代替业务响应成功
    bool m_ready;
    // 输入、顺序、线程或回调次数不符合用例独立预期时置位
    bool m_bad;
    // 本用例观察到的终态记录，用于检查关闭不丢失且不重复
    bool m_closed;
    // 本用例已经取得ProcessWork接收器，退出时负责收敛
    bool m_receiving;
    // 授权响应已准备但尚未被服务出口接纳
    bool m_replyPending;
    // 实际完成文件操作的数量，不以发送完成推算
    uint32_t m_completed;
    // 当前文件后端拥有的非零文件号
    uint64_t m_file;
    // 尚未消费的打开文件请求号
    uint64_t m_openJob;
    // 本轮实际分配的请求号，回包必须精确匹配
    uint64_t m_request;
    // 完成后允许最终清理的单调毫秒时刻
    uint64_t m_finishAt;
    // 本用例独占临时文件目录
    std::string m_directory;
    // 本用例拥有的源文件路径
    std::string m_source;
    // 本用例拥有的接收临时文件路径
    std::string m_part;
    // 本用例授权的最终发布路径
    std::string m_final;
    // 独立生成的期望文件字节，接收完成后逐字节验证
    std::vector<uint8_t> m_expected;
    // 当前文件控制请求的真实协议字段
    NetInterfaceFileControl m_control;
    // 已获授权的文件信息，不能由DATA帧自行建立
    NetInterfaceFileInfo m_info;
    // 等待响应的原始RPC标量，用于保留产品、命令和请求号
    NetInterfaceFrame m_outer;
    // 本次投递的可信根来源副本，用于核对来源隔离
    NetInterfaceOrigin m_origin;
    // 待发送授权响应的独占字节
    std::vector<uint8_t> m_reply;
    // 本探针拥有的实际异步文件后端
    NetFileIo m_io;
    // 当前文件传输协调器，done之后才允许销毁
    std::unique_ptr<NetFileTransfer> m_flow;
    // 借用本用例Sender，关闭及回调收敛之前保持有效
    NetSenderTestClient<NetSender, NetSenderClient>* m_sender;
    // 本用例拥有的IPC收件箱，接收回调收敛后才可销毁
    NetSenderLocalInbox m_inbox;
    // 本用例服务接入对象，生命周期覆盖服务回调与协议推进
    std::unique_ptr<NetSenderService> m_service;
};
#endif