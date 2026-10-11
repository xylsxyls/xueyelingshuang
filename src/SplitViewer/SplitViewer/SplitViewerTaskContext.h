#pragma once
#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <stdint.h>

/** 一次请求的执行和呈现协议；业务函数只接触独立输入，不借用窗口
*/
struct SplitViewerTaskRequest
{
public:
    // 日志与结果关联编号，在GUI线程分配
    uint64_t m_id;
    // 耗时任务送执行队列，否则在逻辑队列执行
    bool m_longRunning;
    // 返回本次实际业务结果，包含安全提交段
    std::function<bool(const std::atomic<bool>&)> m_work;
    // GUI呈现函数，仅轮询端取出后调用
    std::function<void(bool)> m_complete;
    // 已完成的实际结果，不跨线程并发修改
    bool m_success;

public:
    /** 初始化尚未提交的请求
    */
    SplitViewerTaskRequest();
};

/** 每个窗口独立的任务上下文，关闭后由协调队列持有到线程回收完成
*/
struct SplitViewerTaskContext
{
public:
    // 对应窗口的逻辑线程ID
    uint32_t m_logicId;
    // 对应窗口的耗时执行线程ID
    uint32_t m_executionId;
    // 只提出停止，不在GUI等待
    std::atomic<bool> m_exit;
    // 协调任务回收两个ID后的通知
    std::atomic<bool> m_retired;
    // 只保护短提交段，停止标志在此锁内发布
    std::mutex m_dispatchMutex;
    // 单请求结果槽，统一通过atomic_load/store/exchange访问
    std::shared_ptr<SplitViewerTaskRequest> m_completed;

public:
    /** 初始化尚未分配线程的上下文
    */
    SplitViewerTaskContext();
};