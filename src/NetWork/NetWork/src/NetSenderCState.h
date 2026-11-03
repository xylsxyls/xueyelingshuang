#pragma once
#include "NetSenderC.h"
#include "NetSender.h"
#include <atomic>

// C边界借用用户回调，并持有由统一C++门面创建的独立客户端；在创建模块释放
struct NetWorkSenderClientOpaque : public NetSessionListener
{
public:
    // 本提供方创建的独立客户端；destroy成功后置空
    NetSenderClient* m_client;
    // 复制函数指针，context借用至销毁成功
    NetInterfaceClientCallbacks m_callbacks;
    // 回调深度仅用于拒绝生命周期重入，不赋予并发销毁权限
    std::atomic<uint32_t> m_callbackDepth;

public:
    /** 创建静止C适配器
    @param [in] callbacks 已验证的完整回调表
    */
    explicit NetWorkSenderClientOpaque(const NetInterfaceClientCallbacks& callbacks);

    /** 释放初始化失败时仍持有的静止客户端；正常销毁先显式收敛
    */
    virtual ~NetWorkSenderClientOpaque();

    /** 转交本代就绪；异常关闭当前客户端
    @param [in] generation 会话代次
    @param [in] requests 请求上限
    @param [in] chunk 数据块上限
    */
    virtual void onSessionReady(uint64_t generation, uint32_t requests, uint32_t chunk);

    /** 转交唯一请求终态，正文只借用至回调返回
    @param [in] generation 会话代次
    @param [in] requestId 请求号
    @param [in] product 产品号
    @param [in] type 消息类型
    @param [in] status 结果码
    @param [in] bytes 正文，size非零时有效
    @param [in] size 正文字节数
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product,
        uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 转交帧并保持背压语义
    @param [in] generation 会话代次
    @param [in] frame 回调期间借用的完整帧
    @return true已消费，false无副作用背压
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 转交会话唯一关闭事件
    @param [in] generation 会话代次
    @param [in] reason 关闭原因
    @param [in] detail 下层诊断
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);
};