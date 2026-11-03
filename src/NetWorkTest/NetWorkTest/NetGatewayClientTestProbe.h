#pragma once
#include "NetSenderTestClient.h"
#ifndef __EMSCRIPTEN__
#include "NetSender/NetSenderAPI.h"
#include "NetWork/NetWorkAPI.h"
#include <vector>

// 服务不存在场景的独占监听器，只记录事件，不保留回调数据或触发业务发送
class NetUnavailableServiceTestListener : public NetSessionListener
{
public:
    /** 清空事件观察标志
    */
    NetUnavailableServiceTestListener();

    /** 不存在的服务不能进入就绪态，收到即记录失败
    @param [in] generation 会话代次，本用例不应产生
    @param [in] maxRequests 协商请求额度，本用例不应产生
    @param [in] maxChunk 协商分块字节数，本用例不应产生
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 未发送业务请求时不能收到结果，收到即记录失败
    @param [in] generation 回调代次，本用例不使用
    @param [in] requestId 请求号，本用例未预留请求
    @param [in] product 产品号，本用例不使用
    @param [in] type 业务类型，本用例不使用
    @param [in] status 业务终态，本用例不使用
    @param [in] bytes 仅借用至回调返回，本用例不读取
    @param [in] size 正文字节数，本用例不读取
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type,
        uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 服务未就绪时不应收到流帧，记录失败并消费输入
    @param [in] generation 会话代次，本用例不使用
    @param [in] frame 仅借用至返回的帧，本用例不读取
    @return true已消费，异常状态由观察标志记录
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 记录恢复预算耗尽事件，其他关闭原因不算测试通过
    @param [in] generation 会话代次，本用例不使用
    @param [in] reason 必须观察到NI_TIMEOUT
    @param [in] detail 底层诊断码，本用例不使用
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

public:
    // 是否收到服务不可用期间禁止出现的就绪、结果或流帧
    bool m_unexpected;
    // 是否已经观察到恢复预算耗尽的关闭事件
    bool m_timeout;
};

// 同时验证桌面与Peer接入的客户端探针，使用真实请求和恢复回调
class NetGatewayClientTestProbe : public NetSessionListener
{
public:
    /** 初始化实际NetSender公开入口探针
    */
    NetGatewayClientTestProbe();

    /** 记录实际会话就绪及本用例的握手约束
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] maxRequests 已协商的最大在途请求数
    @param [in] maxChunk 已协商的最大文件分块字节数
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 核对本轮请求号、业务状态和实际返回正文
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] requestId 本轮实际请求号，必须与已提交请求匹配
    @param [in] product 协议产品号，按本用例独立预期核对
    @param [in] type 业务消息类型，按本用例独立预期核对
    @param [in] status RPC终态状态，不等同于底层发送返回值
    @param [in] bytes 回调或同步调用期间借用的正文；不保存裸指针
    @param [in] size 正文字节数，不能用C字符串长度代替
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status,
        const uint8_t* bytes, uint32_t size);

    /** 核对本用例的非请求帧，异常输入记录为失败
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] frame 本次调用期间借用的完整帧，背压时由调用方保留
    @return true已消费；测试失败另行记录
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 记录本轮会话终态，供外层推进及结果断言使用
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] reason 本连接或服务的NI关闭原因
    @param [in] detail 底层诊断码，不替代NI关闭原因
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

    /** 单独进程真实调用NetSender→本机网关→业务服务，含目标错误预留拒绝
    @param [in] pid 本次拥有的NetClientManager进程号
    @return 0通过，否则失败
    */
    static int childMain(int32_t pid);

    /** 真实C++门面生命周期/回调重入拒绝及双向大包，0测试按标准名自动发现
    @param [in] pid 本批网关PID，0为自动发现
    @return 0通过，否则失败
    */
    static int facadeMain(int32_t pid);

    /** 真实公开SDK恢复探针：首轮回复后等待宿主故障，再在新代次发送新业务
    @param [in] target 桌面为本机网关PID，否则为唯一原始IP:port种子
    @param [in] desktop 是否走NetSender及ProcessWork，false走NetWork C++入口
    @return 0仅在两代回复及实际断线均观察到时通过
    */
    static int recoveryMain(const std::string& target, bool desktop);

    /** 验证服务不存在时停止恢复，不提前发出业务请求
    @param [in] target 本用例拥有的网关IP:port
    @return 0表示公开客户端在30秒恢复预算内超时结束
    */
    static int unavailableMain(const std::string& target);

private:
    // 借用本用例Sender，关闭及回调收敛之前保持有效
    NetSenderTestClient<NetSender, NetSenderClient>* m_sender;
    // 本用例拥有的原生Peer内部客户端，销毁前先推进至静止
    NetPeerClientHandle m_handle;
    // 本用例拥有的桌面Sender C++门面客户端
    NetSenderClient* m_senderHandle;
    // 本用例拥有的独立预期正文，用于逐字节核对传输结果
    std::vector<uint8_t> m_payload;
    // 本用例记录的实际就绪代次，用于拒绝旧代回调
    uint64_t m_generation;
    // 本轮实际分配的请求号，回包必须精确匹配
    uint64_t m_request;
    // 本用例观察到的实际就绪状态，不代替业务响应成功
    bool m_ready;
    // 已向实际接入层提交的发送观察状态，不能视作远端收到
    bool m_sent;
    // 本用例工作已完成，拥有者仍须收敛运输或回收任务
    bool m_done;
    // 本用例首次失败后保留的错误标志，后续成功不能清除
    bool m_failed;
    // 是否启用入口或服务故障后恢复验证
    bool m_recovery;
    // 实际成功的恢复及往返轮数
    uint32_t m_rounds;
    // 实际关闭通知次数，恢复时核对每个旧代次都已终止
    uint32_t m_closedCount;
};
#endif