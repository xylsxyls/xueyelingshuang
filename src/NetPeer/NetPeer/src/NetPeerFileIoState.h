#pragma once
#include "NetFileIo.h"

// 一次初始化拥有的宿主后端和回调状态，最后一次释放完成前始终有效。
struct NetPeerFileIoState
{
public:
    // 本实例独占的后端，允许初始化期间暂为空。
    NetFileIoBackend* m_backend;
    // 当前宿主异常或生命周期错误。
    int32_t m_error;
    // 后端处于初始化、活动、停止或释放阶段。
    NetFileIoPhase m_phase;
    // 正在调用宿主，回入时推迟停止并拒绝释放。
    bool m_calling;

public:
    /** 创建尚未调用工厂的启动状态 */
    NetPeerFileIoState();
};