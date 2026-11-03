#pragma once
#include "NetPeerTypes.h"
#include "NetInterface/NetInterfaceAPI.h"

// 门面状态借用监听者并持有主类创建的客户端；不复制会话、队列或协议路径
class NetPeerSenderState
{
public:
    /** 创建未准备的门面状态，不启动网络或线程
    */
    NetPeerSenderState();

    /** 构造将核心事件送到借用C++监听者的完整回调表
    @return context为本状态的v1表
    */
    NetPeerClientCallbacks callbacks();

    /** 在静止状态更换监听者并调用统一连接入口
    @param [in] product 0直连，否则固定目标逻辑服务
    @param [in] options 调用期间借用，随机上下文仍借用至关闭
    @param [in] listener 连接及回调收敛前保持存活
    @return 接纳结果，不代表已经就绪
    */
    int32_t connect(uint32_t product, const NetSenderGatewayOptions& options, NetSessionListener* listener);

    /** 转发实际就绪，异常由C++核心隔离
    @param [in] context 本状态
    @param [in] generation 本代次
    @param [in] requests 协商额度
    @param [in] chunk 协商块大小
    */
    static void ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk);

    /** 转发请求唯一终态，正文仅借用至返回
    @param [in] context 本状态
    @param [in] generation 本代次
    @param [in] id 业务请求号
    @param [in] product 产品
    @param [in] type 命令
    @param [in] status 请求终态
    @param [in] bytes 借用正文
    @param [in] size 正文字节
    */
    static void result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type,
        uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 转发流帧背压，异常由C++核心关闭本代
    @param [in] context 本状态
    @param [in] generation 本代次
    @param [in] frame 借用帧及正文
    @return 1接纳，0无副作用背压
    */
    static int32_t frame(void* context, uint64_t generation, const NetInterfaceFrame* frame);

    /** 转发本代唯一关闭，异常由C++核心隔离
    @param [in] context 本状态
    @param [in] generation 本代次
    @param [in] reason 关闭原因
    @param [in] detail 底层诊断
    */
    static void closed(void* context, uint64_t generation, int32_t reason, int32_t detail);

public:
    // 本库创建并释放的唯一核心，静止后才能销毁。
    NetPeerClientHandle m_client;
    // 借用至本代所有回调完成；失败重连不能覆盖运行中的监听者。
    NetSessionListener* m_listener;
    // 最近有返回码操作的诊断，查询和正常poll不清除。
    int32_t m_lastError;
};