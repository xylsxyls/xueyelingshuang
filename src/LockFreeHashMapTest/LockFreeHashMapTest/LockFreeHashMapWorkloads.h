#pragma once
#include "LockFreeHashMapTestThread.h"
#include <LockFreeHashMap.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <stddef.h>
#include <thread>
#include <vector>

/** 所有完整哈希值均相同的无状态哈希器，独立键缓存仍须校验完整键身份
*/
struct LockFreeHashMapWorkloadCollisionHash
{
public:
    /** 强制完整哈希碰撞
    @param [in] key 待查整数键，不影响哈希结果
    @return 始终为 0
    */
    size_t operator()(int key) const;
};

/** 原版、整改前版及当前版共用的负载，大部分操作不插入逐次计时
*/
class LockFreeHashMapWorkloads
{
public:
    // 线程启动前设置，之后只读的 QPC 每秒计数
    static double s_frequency;
public:
    /** 读取单调高精度秒数
    @return 当前单调秒数，用于同进程内时间差
    */
    static double timestamp();

    /** 每个工作线程使用独立键区间，在真实并发发布下逐次核对精确值
    @param [in] name 报告负载名称，同步借用
    @param [in] buckets 桶数量
    @param [in] threads 工作线程数量，每线程 64 个键
    @param [in] iterations 每线程操作次数
    @param [in] writeEvery 写入周期，0 表示只读
    @param [in] hotHistory 计时前对热键覆盖的次数
    @return 所有值核对和提交均成功为 true，设施异常由 main 捕获
    */
    template <typename HashType = std::hash<int>>
    static bool run(const char* name, size_t buckets, int threads, int iterations, int writeEvery, int hotHistory);

    /** 预构造共享所有权值模拟缓存和注册表，计时内不分配业务对象
    @param [in] name 报告负载名称，同步借用
    @param [in] buckets 桶数量
    @param [in] threads 工作线程数量，每线程 64 个键
    @param [in] iterations 每线程操作次数
    @param [in] writeEvery 写入周期，0 表示只读
    @param [in] hotHistory 计时前对热键覆盖的次数
    @return 所有值核对和提交均成功为 true，设施异常由 main 捕获
    */
    template <typename HashType = std::hash<int>>
    static bool runOwned(const char* name, size_t buckets, int threads, int iterations, int writeEvery, int hotHistory);
};

#include "LockFreeHashMapWorkloads.inl"