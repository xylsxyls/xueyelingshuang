#pragma once
#include "NetSenderTestClient.h"
#if defined(_WIN32) && defined(NET_WORK_TEST_ACCOUNT)
#include "Account/AccountAPI.h"
#include "NetSender/NetSenderAPI.h"

// 通过实际网关与Account服务核对账号操作的独立测试客户端
class NetAccountGatewayProbe : public NetSessionListener
{
  public:
    /** 初始化基础往返测试，SDK不放在新的AccountTest中
    */
    NetAccountGatewayProbe();

    /** 实际桌面网关/业务A/账号B的基础运行入口
    @param [in] pid 测试拥有的NetClientManager进程
    @return 0成功或非零失败
    */
    static int run(int32_t pid);

    /** 开始同源注册请求
    @param [in] generation 连接代次
    @param [in] maxRequests 限额
    @param [in] maxChunk 块限额
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 五次基本账号调用，每个结果只处理一次
    @param [in] generation 原代次
    @param [in] requestId 请求号
    @param [in] product 产品
    @param [in] type 命令
    @param [in] status RPC状态
    @param [in] data 借用业务体
    @param [in] size 字节数
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status,
                                 const uint8_t* data, uint32_t size);

    /** 本轮基本测试不消费文件；账号替换通知交SDK
    @param [in] generation 代次
    @param [in] frame 帧
    @return 接纳
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 记录一次正常关闭
    @param [in] generation 代次
    @param [in] reason 原因
    @param [in] detail 诊断
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

  private:
    /** 构造当前阶段请求
    @param [in] parameters 同源参数消息，可空
    @return 是否接纳
    */
    bool sendNext(NetInterfaceMessageHandle parameters);

  public:
    // 本探针独占的实际Account客户端，按正式接口发起账号业务
    Account m_account;
    // 借用本用例Sender，关闭及回调收敛之前保持有效
    NetSenderTestClient<NetSender, NetSenderClient>* m_sender;
    // 本轮实际分配的请求号，回包必须精确匹配
    uint64_t m_id;
    // 当前账号测试阶段，推进只能来自匹配业务结果
    uint32_t m_stage;
    // 当前账号操作种类，用于核对结果对应的调用
    uint32_t m_operation;
    // 实际收到的请求终态次数，用于检查一请求一终态
    uint32_t m_results;
    // 实际关闭通知次数，用于检查唯一终态
    uint32_t m_closes;
    // 输入、顺序、线程或回调次数不符合用例独立预期时置位
    bool m_bad;
};
#endif