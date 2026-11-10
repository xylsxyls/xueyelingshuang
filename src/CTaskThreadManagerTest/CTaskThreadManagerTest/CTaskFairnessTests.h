#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <vector>
#include <memory>

/** 单个生产线程的独立进展计数，填充用于避免监测计数彼此共享缓存行 */
struct CTaskFairnessCounter
{
public:
    // 当前执行流累计成功数，每 64 项或退出时发布
    std::atomic<uint64_t> m_published;
    // 该执行流已开始参与测量
    std::atomic<bool> m_started;
    // 保持相邻计数的写入位置相隔至少一条缓存行
    unsigned char m_padding[128];
    // 线程结束后的精确计数
    uint64_t m_count;
    // 最长采样无进展间隔，毫秒，由监测线程写入
    uint64_t m_maxGap;
    // 线程 CPU 时间，100 纳秒，非 Windows 为零
    uint64_t m_cpu;
    // 资源上限失败次数
    uint64_t m_capacity;
    // 预算未满但当前不能安全复用的资源失败次数
    uint64_t m_resources;
    // 暴露给调用者的竞争 Retry 次数
    uint64_t m_retries;

public:
    /** 构造清零的监测记录 */
    CTaskFairnessCounter();

};

/** 公平性用例状态；任务预建完成后冻结布局，生产计数各自独占、消费计数单消费者独占 */
class CTaskFairnessState
{
public:
    /** 配置参与线程及任务优先级
    @param [in] producers 生产线程数
    @param [in] levels 任务优先级数量
    */
    CTaskFairnessState(size_t producers, size_t levels);

    /** 消费一个预建任务并核对本生产者同级 FIFO
    @param [in] producer 生产者下标
    @param [in] ordinal 本生产者任务对象序号
    */
    void consume(size_t producer, size_t ordinal);

public:
    // 固定生产线程数
    const size_t m_producers;
    // 固定任务优先级数
    const size_t m_levels;
    // 生产者计数数组
    std::unique_ptr<CTaskFairnessCounter[]> m_sent;
    // 消费者按来源分类的计数数组
    std::unique_ptr<CTaskFairnessCounter[]> m_received;
    // 每来源每优先级消费次数，仅由消费者写入
    std::vector<uint64_t> m_orders;
    // 每个生产者预建对象数量
    std::vector<size_t> m_objects;
    // 同时启动生产者
    std::atomic<bool> m_start;
    // 停止发起新的投递
    std::atomic<bool> m_stop;
    // 同级乱序、接口错误及线程优先级设置错误计数
    std::atomic<uint64_t> m_errors;
    // 测量开始的 steady_clock 毫秒时间
    uint64_t m_begin;
};

/** 只做公平性计数和顺序核对的预建任务，不用于无仪表吞吐成绩 */
class CTaskFairnessTask : public CTask
{
public:
    /** 保存冻结参数，状态必须活到所有任务退出
    @param [in,out] state 测量状态
    @param [in] producer 来源线程下标
    @param [in] ordinal 对象在来源线程中的序号
    */
    CTaskFairnessTask(CTaskFairnessState& state, size_t producer, size_t ordinal);

    /** 唯一消费者记录进展 */
    virtual void doTask();

private:
    // 借用生命周期覆盖整个用例的状态
    CTaskFairnessState* m_state;
    // 不变来源线程
    size_t m_producer;
    // 不变任务对象序号
    size_t m_ordinal;
};

/** 同优先级公平性压力入口；参数为生产者数、优先级数、秒数、可用 CPU 数、报告路径，可选最后一项指定队列登记数
@param [in] argc 参数数量
@param [in] argv 参数数组
@return 完整计数、顺序及进展验证成功返回零
*/
int runTaskFairnessTests(int argc, char** argv);