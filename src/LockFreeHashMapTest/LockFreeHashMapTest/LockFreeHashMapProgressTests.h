#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>

/** 每个工作线程独占的观测和取值记录，不在计时路径争用共享统计
*/
struct LockFreeHashMapProgressSample
{
public:
    // 已返回的 API 调用数
    uint64_t m_calls;
    // 已进入的 API 调用数，用于区分未开始与调用尚未返回
    uint64_t m_entered;
    // 状态为 Ok 的实际完成数
    uint64_t m_completed;
    // 公共 1.2 秒窗口截止前返回 Ok 的次数，截止后的成功不补算通过
    uint64_t m_windowCompleted;
    // Exists 或 NotFound 的正常逻辑结果数
    uint64_t m_logicalMisses;
    // 资源、用户异常及设施失败数
    uint64_t m_failures;
    // 包含调度暂停的单次调用最大毫秒数
    double m_maxCallMs;
    // 包含末尾未成功时间的最大成功间隔
    double m_maxProgressGapMs;
    // Windows 实测 CPU 毫秒数，其他平台不采样
    double m_cpuMs;
    // 从公共起点到离开启动门的毫秒数
    double m_startDelayMs;
    // 从公共起点到首次成功返回的毫秒数，未成功为 -1
    double m_firstSuccessMs;
    // Windows 线程周期计数差，避免把粗粒度 CPU 时间为零解释为未运行
    uint64_t m_cpuCycles;
    // 实测 Windows 线程优先级
    int32_t m_priority;
    // take 取得的独立值，用于收拢后检查重复
    std::vector<uint64_t> m_taken;
public:
    /** 初始化单线程观测，不申请取值数组
    */
    LockFreeHashMapProgressSample();
};

/** 全哈希碰撞夹具，和正常哈希使用相同的负载及计量
*/
struct LockFreeHashMapProgressCollision
{
public:
    /** 将所有键映射到同一个哈希
    @param [in] key 受测键，不参与哈希计算
    @return 固定为零
    */
    size_t operator()(uint64_t key) const;
};