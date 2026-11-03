#pragma once
#include "NetPeerTypes.h"
#include "NetSession.h"
#include "NetGatewayClientChannel.h"
#include "NetPeerClientSession.h"
#include <memory>
#include "NetInterface/NetInterfaceAPI.h"

// 仅本库分配/释放，不向调用方暴露C++布局。
struct NetPeerClientOpaque : public NetSessionListener
{
public:
    // 复制的核心回调表，context仍借用至销毁成功。
    NetPeerClientCallbacks m_callbacks;
    // 唯一协议会话，只能由Peer执行者访问
    NetPeerClientSession m_session;
    // 统一接纳与owner分派，先于会话析构
    NetSenderDispatch m_dispatch;
    // 核心入口自身的推进标记，保证嵌套调用不能释放正在使用的核心。
    bool m_polling;
    // 用户回调嵌套层数，不能在回调内连接、配置、同步等待或销毁。
    uint32_t m_callbackDepth;

public:
    /** 初始化借用的回调表
    @param [in] callbacks 已验证v1表
    */
    explicit NetPeerClientOpaque(const NetPeerClientCallbacks& callbacks);

    /** 收敛会话后销毁适配器
    */
    ~NetPeerClientOpaque();

    /** 转发真实就绪
    @param [in] generation 代次
    @param [in] maxRequests 协商额度
    @param [in] maxChunk 块大小
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 转发唯一终态
    @param [in] generation 代次
    @param [in] requestId 请求号
    @param [in] product 产品
    @param [in] type 命令
    @param [in] status RPC状态
    @param [in] data 借用体
    @param [in] size 长度
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status,
                                 const uint8_t* data, uint32_t size);

    /** 转发数据及背压
    @param [in] generation 代次
    @param [in] frame 借用帧
    @return 是否接纳
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 转发关闭
    @param [in] generation 代次
    @param [in] reason 关闭原因
    @param [in] detail 诊断
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

    /** 检查所有执行、回调、会话、请求及交付票据均已收敛
    @return 是否允许更换连接或销毁核心
    */
    bool inactive() const;

    /** 两种连接共用的接纳事务，修改配置前先验证静止状态
    @param [in] options 已核对v1结构，地址及证书调用时借用
    @param [in] product 0直连，否则固定逻辑服务
    @return 接纳结果，真实就绪异步报告
    */
    int32_t connect(const NetPeerClientOptions& options, uint32_t product);
};