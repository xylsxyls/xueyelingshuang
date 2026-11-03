#pragma once
#include "NetWork.h"
#include "NetWorkTypes.h"
#include <atomic>

// 门面状态借用监听者并持有主类创建的客户端；不复制会话、队列或协议路径
class NetWorkSenderState
{
public:
    /** 创建静止适配，不启动线程
    */
    NetWorkSenderState();

    /** 构造借用本适配的完整回调表
    @return v1表，直到句柄收敛均有效
    */
    NetInterfaceClientCallbacks callbacks();

    /** 在协议执行者转交本代就绪回调
    @param [in] context 本C++门面状态
    @param [in] generation 本次非零会话代次
    @param [in] requests 协商的最大并行请求数
    @param [in] chunk 协商的最大数据块字节数
    */
    static void ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk);

    /** 在协议执行者转交已关联的业务结果
    @param [in] context 本C++门面状态
    @param [in] generation 结果所属代次
    @param [in] id 本代请求号
    @param [in] product 产品号
    @param [in] type 业务消息类型
    @param [in] status 业务结果码
    @param [in] bytes 回调期间借用的结果字节，可为空仅当size为0
    @param [in] size 结果字节数
    */
    static void result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 在协议执行者转交通道帧，保留上层背压语义
    @param [in] context 本C++门面状态
    @param [in] generation 帧所属代次
    @param [in] value 回调期间借用的帧及正文
    @return C++监听者帧回调结果；非0已消费，0未消费
    */
    static int32_t frame(void* context, uint64_t generation, const NetInterfaceFrame* value);

    /** 在协议执行者转交本代终态通知
    @param [in] context 本C++门面状态
    @param [in] generation 已关闭的代次
    @param [in] reason NI关闭原因
    @param [in] detail 下层诊断码，不作为业务结果码
    */
    static void closed(void* context, uint64_t generation, int32_t reason, int32_t detail);
public:
    // 只由本提供方分配、销毁
    NetWorkClientHandle m_client;
    // 连接之前设置，协议及回调收敛之后才可更换
    NetSessionListener* m_listener;
    // 多生产者各次结果仍以直接返回为准
    std::atomic<int32_t> m_lastError;
};