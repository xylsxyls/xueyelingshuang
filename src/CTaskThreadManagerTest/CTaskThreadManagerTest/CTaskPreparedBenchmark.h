#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <vector>
#include <string>

typedef CTaskThread CTaskPreparedThread;
typedef CTaskThreadManager CTaskPreparedManager;

/** 采样时间由生产者和单消费者分别写入；全部线程收敛后读取
*/
struct CTaskPreparedSample
{
public:
    // 本次逻辑提交开始，不包含 Task 构造
    uint64_t m_begin;
    // 成功提交返回，包含未接纳重试
    uint64_t m_returned;
    // doTask 入口，不包含测试校验
    uint64_t m_entered;
    // 本次逻辑提交中的未接纳重试数
    uint64_t m_retries;
    // 业务任务优先级
    int32_t m_level;

public:
    /** 初始化未采样状态
    */
    CTaskPreparedSample();

};

/** 每生产者独占写入；进度通过 release/acquire 发布
*/
struct CTaskPreparedProducerData
{
public:
    // 已成功提交的保守进度，最多落后 1023 个
    std::atomic<uint64_t> m_published;
    // 所有本生产者回调字段已经写完
    std::atomic<bool> m_done;
    // 提交完成时刻
    uint64_t m_end;
    // 本生产者结束时观察到的消费进度
    uint64_t m_consumedAtEnd;
    // 未接纳重试总数
    uint64_t m_retries;
    // 分隔不同生产者的高频写入
    unsigned char m_padding[128];

public:
    /** 清零本轮生产状态
    */
    CTaskPreparedProducerData();

};

/** 有界在途延迟测试的独立完成位置；吞吐测试不读取或更新此位置
*/
struct CTaskPreparedCompletion
{
public:
    // 单消费者发布本生产者的连续完成序号
    std::atomic<uint64_t> m_count;
    // 不与其他生产者的完成位置共用缓存行
    unsigned char m_padding[64];

public:
    /** 初始没有任务完成
    */
    CTaskPreparedCompletion();

};

/** 单次基准全部借用数据；生命周期覆盖生产者和消费者
*/
class CTaskPreparedState
{
public:
    /** 预分配数据和观测结构
    @param [in] producers 生产者数，1 至 32
    @param [in] total 总任务数，必须整除生产者数
    @param [in] levels 优先级数量
    @param [in] latency 是否开启每 1009 项的采样
    @param [in] separateAllocation 是否分别分配任务和shared_ptr控制块；对照场景不得混用此配置
    @param [in] reuseTasks 是否固定预建 2000 个任务循环投递；仅吞吐模式使用
    @param [in] countOnly 是否只计数消费，逐项顺序校验保留在其他模式及压力测试中
    @param [in] window 0 不限制；1 为每生产者至多一项在途，仅循环延迟探针使用
    */
    CTaskPreparedState(size_t producers, size_t total, int32_t levels, bool latency, bool separateAllocation = false,
        bool reuseTasks = false, bool countOnly = false, size_t window = 0);

    /** 返回单调时钟原始 tick
    @return tick，频率见 frequency
    */
    static uint64_t now();

    /** 查询单调时钟频率；计时前调用
    @return 每秒 tick 数
    */
    static uint64_t frequency();

public:
    // 生产者数量
    size_t m_producers;
    // 总任务数量
    size_t m_total;
    // 优先级数量
    int32_t m_levels;
    // 是否采样延迟
    bool m_latency;
    // 只用于单独标注的有界在途延迟测试，吞吐模式固定为 0
    size_t m_window;
    // 有界在途模式的完成确认位置，全部线程收敛后释放
    std::unique_ptr<CTaskPreparedCompletion[]> m_windowCompletions;
    // 调用方保留所有 Task 强引用，析构不进入计时区间
    std::vector<std::shared_ptr<CTask>> m_tasks;
    // 每生产者独占的任务对象区间，最后一项为任务对象总数
    std::vector<size_t> m_taskOffsets;
    // 预分配采样位置，与任务下标对应；非采样项不访问
    std::vector<CTaskPreparedSample> m_samples;
    // 消费者本地的每生产者/优先级下一个序号；运行期间只有消费者读写
    std::vector<uint64_t> m_expected;
    // 循环对象模式的下一项身份；单消费者写入，与累计次数分开
    std::vector<uint64_t> m_repeatNext;
    // 每个生产者的发布字段
    std::unique_ptr<CTaskPreparedProducerData[]> m_progress;
    // 所有生产者到达开始屏障
    std::atomic<size_t> m_ready;
    // 统一放行生产者
    std::atomic<bool> m_start;
    // 异常或超时终止意图
    std::atomic<bool> m_stop;
    // 生产者逐次只读标记与消费者逐次记账隔离，避免基准自身制造缓存行争用
    unsigned char m_consumerPadding[128];
    // 单消费者写，线程收敛后才读取
    uint64_t m_completed;
    // 消费进度，每 1024 次发布一次，避免每项原子 RMW
    std::atomic<uint64_t> m_observed;
#ifdef CTTM_BENCH_SAMPLER
    // 仅诊断构建由第一个任务记录，用于采样既有消费者
    std::atomic<uint32_t> m_consumerId;
#endif
    // 最后一次任务执行的结束时刻
    std::atomic<uint64_t> m_finish;
    // 异步提交异常数量
    std::atomic<uint64_t> m_errors;
#ifdef CTTM_BENCH_CPU_TRACE
    // 仅诊断构建按 1024 项采样实际消费 CPU；不进入正式性能构建
    uint64_t m_cpuSamples[64];
    // 覆盖最大4000万项，每1024项记录CPU；最终按生产结束下界分开同期消费与排空
    uint8_t m_cpuTrace[40000000 / 1024 + 1];
#endif
};

/** 无业务计算的任务；以消费者本地序号查漏、查重及核对 FIFO，不写任务对象
*/
class CTaskPreparedItem : public CTask
{
public:
    /** 借用本轮状态和可选采样位置
    @param [in,out] state 线程收敛前保持有效
    @param [in,out] sample 可空，仅延迟模式采样项使用
    @param [in,out] expected 本生产者/优先级的下一个期望序号，长于任务生命周期
    @param [in] sequence 本任务在所属生产者/优先级中的固定序号
    @param [in,out] repeatNext 循环投递的期望身份，nullptr 表示每个任务只投递一次
    @param [in] cycle 同生产者同级的循环对象数，repeatNext 非空时必须为正
    */
    CTaskPreparedItem(CTaskPreparedState& state, CTaskPreparedSample* sample, uint64_t& expected, uint64_t sequence,
        uint64_t* repeatNext = nullptr, uint64_t cycle = 0);

    /** 记录入口采样及普通执行序号，不做业务计算
    */
    virtual void doTask();

private:
    // 构造后不再修改的本生产者/优先级序号，从 0 开始
    const uint64_t m_sequence;
    // 消费者本地计数的借用指针，不与生产者进度共享可写缓存行
    uint64_t* m_expected;
    // 可空的循环身份位置，仅消费者读写所指计数器
    uint64_t* m_repeatNext;
    // 循环长度；构造后不变，不在消费时做整数除法
    const uint64_t m_cycle;
    // 借用本轮状态
    CTaskPreparedState* m_state;
    // 借用独立采样位置
    CTaskPreparedSample* m_sample;
};

/** 循环复用任务的延迟探针；采样按逻辑投递序号选择，不改写正在排队的任务对象
*/
class CTaskPreparedSampledItem : public CTaskPreparedItem
{
public:
    /** 绑定固定对象身份和消费者独占的逻辑序号
    @param [in,out] state 本轮预分配状态，保持至线程全部收敛
    @param [in] producer 生产者下标
    @param [in] sequence 对象在所属生产者中的序号
    @param [in] cycle 同生产者同优先级的对象数
    */
    CTaskPreparedSampledItem(CTaskPreparedState& state, size_t producer, size_t sequence, size_t cycle);

    /** 按每 1009 次逻辑投递采样入口，再执行原有完整顺序与计数校验
    */
    virtual void doTask();

private:
    // 只由消费者读写的对应生产者/优先级序号
    uint64_t* m_expected;
    // 预分配的本生产者采样区域，直到全部线程退出后才释放
    CTaskPreparedSample* m_samples;
    // 构造后不变的优先级总数
    size_t m_levels;
    // 本对象对应的零起点优先级
    size_t m_level;
    // 本生产者实际投递次数，防止错误的额外执行越过采样区
    size_t m_total;
    // 有界在途测试才使用；其他模式为空，不写完成确认
    std::atomic<uint64_t>* m_completion;
};

/** 只保留消费计数的空任务；顺序正确性由独立校验模式覆盖
*/
class CTaskPreparedCountedItem : public CTask
{
public:
    /** 借用测试状态，状态须保持到消费者收敛
    @param [in,out] state 本次基准的计数和终止标记
    */
    explicit CTaskPreparedCountedItem(CTaskPreparedState& state);

    /** 只增加单消费者计数，每 1024 项发布一次观察值
    */
    virtual void doTask();

private:
    // 本次基准的借用状态，任务销毁前保持有效
    CTaskPreparedState* m_state;
};

/** 使用独立工作线程全速提交已经创建的任务，不限制生产速率
*/
class CTaskPreparedProducer : public CTask
{
public:
    /** 保存借用状态与目标强引用
    @param [in,out] state 本轮状态
    @param [in] target 单消费者线程
    @param [in] producer 生产者索引
    */
    CTaskPreparedProducer(CTaskPreparedState& state, const std::shared_ptr<CTaskPreparedThread>& target, size_t producer);

    /** 同步开始后全速提交；异常记录为失败
    */
    virtual void doTask();

    /** 通知有限循环退出，不操作任务数组
    */
    virtual void stopTask();

private:
    /** 已越过开始屏障后运行有界在途探针；等待确认时间不计入下一次投递响应
    */
    void produceWindow();

private:
    // 借用本轮状态
    CTaskPreparedState* m_state;
    // 生命周期覆盖本生产者的目标
    std::shared_ptr<CTaskPreparedThread> m_target;
    // 生产者索引
    size_t m_producer;
};

/** 隔离基准拥有的线程；异常时先停止生产，再回收消费者
*/
class CTaskPreparedSession
{
public:
    /** 借用基准状态
    @param [in,out] state 生命周期覆盖会话
    */
    explicit CTaskPreparedSession(CTaskPreparedState& state);

    /** 回收本会话注册的线程，不影响其他线程
    */
    ~CTaskPreparedSession();

public:
    // 本会话的消费者 ID
    uint32_t m_targetId;
    // 本会话的生产者 ID
    std::vector<uint32_t> m_producerIds;

private:
    // 析构发出停止意图时仍然有效
    CTaskPreparedState* m_state;
};

/** 预创建任务的吞吐及延迟基准；同一源码分别链接 V1/V3/V4/V5
@param [in] argc 六个参数，包含程序名
@param [in] argv 程序名、throughput/throughput-reused/latency 等模式、生产者数、总数、优先级数、CSV 路径
@return 0 表示完整性检查及报告写入成功，非 0 失败
*/
int runPreparedTaskBenchmark(int argc, char** argv);