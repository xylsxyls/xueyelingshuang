#pragma once
#include "LockFreeMap/LockFreeMapAPI.h"
#ifdef LOCK_FREE_MAP_BENCHMARK_STD_MUTEX
#include "LockFreeMapBenchmarkMutex.h"
#ifndef LOCK_FREE_MAP_BASELINE
#define LOCK_FREE_MAP_BASELINE
#endif
template <typename Key, typename Value>
using LockFreeMapBenchmarkMap = LockFreeMapBenchmarkMutex<Key, Value>;
#else
template <typename Key, typename Value>
using LockFreeMapBenchmarkMap = LockFreeMap<Key, Value>;
#endif
#include <ostream>
#include <map>
#include <string>
#include <vector>
#include <stdint.h>

/** 以相同旧接口负载对照整改前后性能
*/
class LockFreeMapBenchmark
{
public:
    // 基准分配器观察到的当前申请字节
    static std::atomic<size_t> m_bytes;
    // 当前阶段申请字节峰值
    static std::atomic<size_t> m_peak;
    // 累计系统申请次数
    static std::atomic<uint64_t> m_newCalls;
    // 整体计时的每线程操作数；较长负载用于降低短时抢占对结果的影响
    static int m_iterations;
    // 稳态模式先完整预热，再延长逐次采样阶段；旧模式保留原有计时口径
    static bool m_steady;
    // 仅额外的 Windows 稳态诊断固定工作线程位置，普通模式保持运行库调度
    static bool m_pinWorkers;
    // 每个可用物理核心选一个逻辑处理器，避免两版随机使用不同的超线程组合
    static std::vector<uintptr_t> m_workerMasks;

    /** 从进程许可的处理器中建立固定放置方案，不修改系统或其他进程
    @return 不需要固定放置或成功取得方案时返回 true
    */
    static bool initializePlacement();

    /** 在计时开始前固定当前工作线程，失败记入测试错误
    @param [in] worker 从零开始的工作线程编号
    @return 当前模式不要求固定放置或系统调用成功时返回 true
    */
    static bool placeWorker(unsigned worker);

public:
    /** 执行全部基准阶段并写出原始数据
    @param [out] report 已打开的报告
    @return 独立验证全部通过时返回零
    */
    static int run(std::ostream& report);

    /** 独立测量128键插删，便于隔离Wasm异常桥接和编译后端开销
    @param [out] report 含短测、整批吞吐、内容校验及可选桥接计数的报告
    @return 完整执行并通过最终内容检查时返回零
    */
    static int diagnoseErase(std::ostream& report);

    /** 对多个交错运行的前后版本报告计算中位数和范围
    @param [in] argc 参数数，至少包含输出文件和一对输入文件
    @param [in] argv 输出文件及交替排列的旧、新报告路径
    @return 完整读取且写入成功时返回零，不把性能回退伪装成通过
    */
    static int compare(int argc, char** argv);

    /** 严格读取一次基准报告中的全部负载和错误计数
    @param [in] path 报告路径
    @param [out] values 各负载每秒操作数
    @param [out] iterations 报告实际使用的整体计时负载长度
    @param [out] steady 是否完整预热并延长逐次采样，禁止与原短测混合比较
    @param [out] placement 工作线程放置方案，禁止比较不同方案
    @return 数据完整且所有操作错误为零时返回 true
    */
    static bool read(const char* path, std::map<std::string, double>& values, unsigned& iterations,
        bool& steady, std::string& placement);

    /** 核对报告行的实际操作数及固定输入，避免同一总量标记掩盖负载差异
    @param [in] fields 已拆分的原始报告行
    @param [in] iterations 整体计时阶段每线程次数
    @param [in] steady 是否延长逐次采样阶段
    @param [in] single 是否为单线程运行时
    @return 名称、键数、线程数、种子及操作数量均符合本负载时返回 true
    */
    static bool validWorkload(const std::vector<std::string>& fields, unsigned iterations, bool steady, bool single);

    /** 测量需要实际复制内存的长键插入和删除
    @param [out] report 本批次性能报告
    @return 独立操作及最终空容器校验的失败数
    */
    static int longKeyChurn(std::ostream& report);

    /** 整批计时循环删除并重插128键，保留短采样负载用于历史对照
    @param [out] report 报告速率以完整删除加重插的轮数每秒计
    @return 每步结果及独立最终内容检查的失败数
    */
    static int eraseInsertBulk(std::ostream& report);

    /** 在整个负载外计时，测量每次真正改变值的更新
    @param [out] report 本批次性能报告
    @param [in] keys 有序活跃键的数量
    @param [in] workers 并发调用数，为一时直接在调用线程执行
    @return 独立操作及最终值校验的失败数
    */
    static int changing(std::ostream& report, int keys, int workers);

    /** 测量随机分布的真实更新，并与独立最终值对照
    @param [out] report 本批次性能报告
    @param [in] keys 实际存入的键数
    @return 独立验证失败数
    */
    static int randomUpdates(std::ostream& report, int keys);

    /** 只测量原有两次计时采样循环，不含容器操作
    @param [out] report 本批次性能报告
    @param [in] count 采样次数，五千次时与原始短负载一致
    @return 计时验证失败数，此处速率不是容器吞吐
    */
    static int timerOverhead(std::ostream& report, int count);

    /** 测量固定负载并计入所有操作失败
    @param [out] report 本批次性能报告
    @param [in,out] map 被测容器
    @param [in] name CSV 负载名称
    @param [in] kind 操作类型
    @param [in] count 迭代次数
    @return 独立预期的失败数
    */
    static int phase(std::ostream& report, LockFreeMapBenchmarkMap<int, int>& map, const char* name, int kind, int count);
};