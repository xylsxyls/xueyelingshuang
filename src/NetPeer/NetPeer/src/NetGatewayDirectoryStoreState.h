#pragma once
#include "NetGatewayDirectoryStore.h"
#include "NetDirectorySnapshot.h"

// 每阶段最多一个在途工作；停止路径不等待后端
enum NetGatewayDirectoryStorePhase
{
    // 未打开或停止已完成
    NET_CACHE_CLOSED,
    // 异步打开既有缓存
    NET_CACHE_LOAD_OPEN,
    // 有界读取已有缓存
    NET_CACHE_LOAD_READ,
    // 释放读取句柄
    NET_CACHE_LOAD_CLOSE,
    // 等待最新快照或合并期限
    NET_CACHE_IDLE,
    // 独占创建由后端负责清理的临时文件
    NET_CACHE_SAVE_OPEN,
    // 写出有界快照并完成持久化刷新
    NET_CACHE_SAVE_WRITE,
    // 失败写的临时句柄释放及删除
    NET_CACHE_SAVE_CLOSE,
    // 关闭并原子提交实际拥有的临时文件
    NET_CACHE_SAVE_PUBLISH,
    // 已请求停止，等待宿主完成资源回收
    NET_CACHE_STOPPING
};

// 同一poll执行者独占，无原生文件或线程类型
class NetGatewayDirectoryStoreState
{
public:
    /** 初始化静止状态，不请求宿主能力
    */
    NetGatewayDirectoryStoreState();

public:
    // 显式配置的文件后端门面
    NetFileIo m_io;
    // 最终公开缓存路径，只来自宿主配置
    std::string m_path;
    // 本次独占临时路径，实际删除权只在文件后端
    std::string m_temporary;
    // 唯一在写快照，最多目录上限加48字节
    std::string m_active;
    // 合并的最新待写快照，同样有界
    std::string m_pending;
    // 只交付一次的初始缓存候选
    NetDirectorySnapshot m_loaded;
    // 临时名强随机，空表示字节宿主默认熵
    NetInterfaceEntropyFunction m_entropy;
    // 借用至shutdown的熵上下文
    void* m_context;
    // 当前阶段
    NetGatewayDirectoryStorePhase m_phase;
    // 当前唯一在途工作号，0尚未接纳
    uint64_t m_job;
    // 当前后端文件号，0无句柄
    uint64_t m_file;
    // 校验过的初始缓存长度
    uint64_t m_length;
    // 最近启动一次写尝试的单调时刻
    uint64_t m_lastWrite;
    // 当前在途操作，用于核对宿主结果
    uint32_t m_operation;
    // 最近加载/写出结果，不构成授权证明
    int32_t m_result;
    // 初始缓存是否有一份完整有效结果未取走
    bool m_loadedReady;
};