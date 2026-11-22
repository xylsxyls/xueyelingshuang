#pragma once
#include "NetBufferPoolConfig.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

// 提交层全部物理存储配置；初始化前按实际基础池契约配置，不另设消息条数阈值
struct NetInterfaceAPI NetSenderSubmissionPools
{
public:
    // 请求身份和生命周期记录
    LockFreeHashMapPoolConfig m_requests;
    // 普通长DATA/同步流记录；异步短DATA直接由m_queue节点存储，不占本池
    LockFreeHashMapPoolConfig m_frames;
    // ACK和取消记录，与普通数据隔离
    LockFreeHashMapPoolConfig m_controls;
    // 并发请求身份
    LockFreeHashMapPoolConfig m_fronts;
    // 同步交付回执，与业务应答隔离
    LockFreeHashMapPoolConfig m_receipts;
    // RPC/同步及控制的提交记录；异步DATA由队列内联或m_frames拥有，不重复占输入槽
    LockFreeHashMapPoolConfig m_inputs;
    // RPC/同步正文所有权记录；异步DATA由帧记录直接拥有
    LockFreeHashMapPoolConfig m_payloads;
    // 控制正文所有权记录
    LockFreeHashMapPoolConfig m_controlPayloads;
    // 请求索引含退役节点
    LockFreeHashMapPoolConfig m_requestIndex;
    // 回执索引含退役节点
    LockFreeHashMapPoolConfig m_receiptIndex;
    // MPSC入口实际节点和生产者记录；异步DATA的至多64字节正文内联在节点中
    // owner额外持有固定64项消费工作区，不随接纳量增长；上一批完整释放后复用
    LockFreeQueuePoolConfig m_queue;
    // 普通正文的五级物理池；异步DATA的64字节内联正文不占此池
    NetBufferPoolConfig m_bodies;
    // 控制正文独立保留，避免普通正文耗尽后无法取消
    NetBufferPoolConfig m_controlBodies;

public:
    /** 默认元数据池各约8MiB物理槽预算，正文按大小类独立配置；允许调用者更换初值、上限和增长策略
    */
    NetSenderSubmissionPools();
};