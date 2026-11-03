#pragma once
#include "NetSenderTestClient.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <atomic>

class NetWork;
class NetWorkSenderState;

// 仅观察实际进程内门面的公开回调，不实现协议
class NetWorkInProcessProbe : public NetSessionListener
{
public:
    /** 初始化一代测试观察值
    @param [in] sender 本用例准备的进程内提供方
    */
    explicit NetWorkInProcessProbe(NetSenderTestClient<NetWork, NetWorkSenderState>& sender);

    /** 观察真实TLS/应用握手并检查回调内不能释放
    @param [in] generation 逻辑代次
    @param [in] maxRequests 协商容量
    @param [in] maxChunk 协商块
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 校验已接纳请求的独立预期字节
    @param [in] generation 代次
    @param [in] requestId 请求号
    @param [in] product 产品
    @param [in] type 命令
    @param [in] status 传输结果
    @param [in] bytes 正文
    @param [in] size 长度
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type,
        uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 本用例不应收到推送
    @param [in] generation 未使用
    @param [in] frame 未使用
    @return true消费并记录失败
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 观察唯一正常关闭
    @param [in] generation 本代
    @param [in] reason 关闭原因
    @param [in] detail 未使用
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

    // 借用到拥有方析构结束
    NetSenderTestClient<NetWork, NetWorkSenderState>* m_sender;
    // 最近就绪代次
    std::atomic<uint64_t> m_generation;
    // 本次预留请求
    uint64_t m_request;
    // 结果次数
    std::atomic<uint32_t> m_results;
    // 关闭次数
    std::atomic<uint32_t> m_closed;
    // 是否违反预期
    std::atomic<bool> m_bad;
};

// 后声明于观察器，保证异常清理中的关闭回调仍有有效接收者
class NetWorkInProcessSenderOwner
{
public:
    /** 准备进程内提供方，不操作ProcessWork
    @param [in] sender 测试独占使用的提供方
    */
    explicit NetWorkInProcessSenderOwner(NetSenderTestClient<NetWork, NetWorkSenderState>& sender);

    /** 先停止回调再释放门面状态
    */
    ~NetWorkInProcessSenderOwner();

private:
    /** 禁止复制测试生命周期
    @param [in] other 未使用
    */
    NetWorkInProcessSenderOwner(const NetWorkInProcessSenderOwner& other);

    /** 禁止复制测试生命周期
    @param [in] other 未使用
    @return 不可调用
    */
    NetWorkInProcessSenderOwner& operator=(const NetWorkInProcessSenderOwner& other);

    // 借用至析构
    NetSenderTestClient<NetWork, NetWorkSenderState>* m_sender;
};

// 进程内统一Sender测试集合，逐用例独立建立预期并检查实际结果
class NetWorkInProcessTestCases
{
public:
    /** 实际TCP/TLS下验证进程内C++门面支持平台1/2/3，无桌面发送库或Manager参与
    */
    static void clientPlatforms();

    /** 检查主类工厂的空值、重复创建、独立状态及配对释放，不连接网络
    */
    static void mainFactories();
};