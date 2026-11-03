#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include <memory>

/** 单执行者IPC发送状态；复用ProcessWork v2提交实现，避免复制共享协议。
*/
class NetSenderIpcState
{
public:
    /** 初始化空资源。
    */
    NetSenderIpcState();

    /** 静止状态释放本实例发送映射，不撤销已提交正文。
    */
    void clear();
public:
    // 独占发送通道，open/close由NetSender唯一执行者管理
    std::unique_ptr<ProcessWorkChannel> m_channel;
    // 注册契约中的正文上限，不含12字节IPC头
    uint32_t m_payloadLimit;
};