#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <vector>
#include <string>

/** 同源基准的独立预期和计数；业务校验不使用互斥量。
*/
class CTaskBenchmarkState
{
public:
    /** 准备固定数据集，计时开始前完成分配。
    @param [in] producers 生产者数量
    @param [in] count 每生产者操作数
    @param [in] levels 优先级数量
    @param [in] work 每任务计算轮数
    */
    CTaskBenchmarkState(size_t producers, size_t count, int32_t levels, uint32_t work);

    /** 先收敛任务，再释放预期数据。
    */
    ~CTaskBenchmarkState();

    /** 生成确定性业务校验值。
    @param [in] seed 输入值
    @param [in] work 计算轮数
    @return 预期摘要
    */
    static uint64_t digest(uint64_t seed, uint32_t work);

    /** 单调高分辨率纳秒时间。
    @return 纳秒计数
    */
    static uint64_t now();

    /** 记录最后一个生产者的提交时间。
    @param [in] timestamp 单调时间
    */
    void submitted(uint64_t timestamp);

public:
    // 固定负载参数
    size_t m_producers;
    // 每生产者操作数
    size_t m_count;
    // 优先级种类
    int32_t m_levels;
    // 计算轮数
    uint32_t m_work;
    // 预先计算的完整摘要预期
    std::vector<uint64_t> m_expected;
    // 单消费者检查的出现次数
    std::vector<unsigned char> m_seen;
    // 单消费者维护的每生产者下一序号
    std::vector<size_t> m_next;
    // 预先分配的采样延迟位置，每个任务最多写一次
    std::vector<uint64_t> m_latency;
    // 查询模式的稳定目标 ID
    std::vector<uint32_t> m_targets;
    // 开始屏障
    std::atomic<bool> m_start;
    // 异常或终止意图
    std::atomic<bool> m_stop;
    // 已到达屏障的生产者数
    std::atomic<size_t> m_ready;
    // 成功提交数
    std::atomic<uint64_t> m_accepted;
    // 实际完成数
    std::atomic<uint64_t> m_completed;
    // 未接纳重试数
    std::atomic<uint64_t> m_retries;
    // 校验或提交错误数
    std::atomic<uint64_t> m_errors;
    // 最后提交时刻
    std::atomic<uint64_t> m_submitTime;
    // 最后执行完成时刻
    std::atomic<uint64_t> m_finishTime;
};

/** 有独立序号、摘要和延迟采样的业务任务。
*/
class CTaskBenchmarkItem : public CTask
{
public:
    /** 保存不可变输入。
    @param [in,out] state 生命周期覆盖任务
    @param [in] producer 生产者索引
    @param [in] sequence 生产者内序号
    @param [in] timestamp 采样起点，0 不采样
    */
    CTaskBenchmarkItem(CTaskBenchmarkState& state, size_t producer, size_t sequence, uint64_t timestamp);

    /** 校验每项只执行一次以及同级 FIFO。
    */
    virtual void doTask();

private:
    // 借用测试状态
    CTaskBenchmarkState& m_state;
    // 不变生产者号
    size_t m_producer;
    // 不变序号
    size_t m_sequence;
    // 不变采样起点
    uint64_t m_timestamp;
};

/** 独立生产任务，同一代码分别链接原版和新版。
*/
class CTaskBenchmarkProducer : public CTask
{
public:
    /** 保存目标和测试模式。
    @param [in,out] state 测试状态
    @param [in] target 投递目标，查询模式允许为空
    @param [in] producer 生产者号
    @param [in] lookup 是否测注册表查询
    */
    CTaskBenchmarkProducer(CTaskBenchmarkState& state, const std::shared_ptr<CTaskThread>& target, size_t producer, bool lookup);

    /** 等待统一屏障后执行固定数量操作。
    */
    virtual void doTask();

    /** 异常收尾时让生产循环退出。
    */
    virtual void stopTask();

private:
    // 借用测试状态
    CTaskBenchmarkState& m_state;
    // 目标强引用
    std::shared_ptr<CTaskThread> m_target;
    // 生产者号
    size_t m_producer;
    // 是否查询模式
    bool m_lookup;
};

/** 同源基准入口，性能构建不链接故障分配器。
@param [in] argc 参数数量
@param [in] argv mode producers count levels work report.csv
@return 验证或运行失败返回非零
*/
int runTaskThreadBenchmark(int argc, char** argv);