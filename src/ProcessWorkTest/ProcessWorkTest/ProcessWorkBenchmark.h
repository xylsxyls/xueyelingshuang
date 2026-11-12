#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "BlackBoxTestSupport.h"
#include <atomic>
#include <functional>
#include <vector>

#ifdef PROCESS_WORK_BENCH_STORAGE_GROWTH
/** 测试内的实际池增长观察者，默认算法与未安装观察者时相同；不进入生产工程
*/
class ProcessWorkBenchmarkGrowth : public ProcessWorkMemoryPolicy
{
public:
    /** 借用进程内增长计数，计数的生命周期覆盖所有后台任务
    @param [in,out] count 仅记录真实成功增长
    */
    explicit ProcessWorkBenchmarkGrowth(std::atomic<uint64_t>& count);

    /** 保持实际存储的默认增长算法
    @param [in] context 资源种类、容量及所需增量
    @return 与默认路径一致的增长量
    */
    virtual uint64_t grow(const ProcessWorkMemoryGrowContext& context);

    /** 累计真实成功事件
    @param [in] growth 本次事件
    */
    virtual void grown(const ProcessWorkMemoryGrowth& growth);

private:
    // 借用基准进程的计数，实例销毁前保持有效
    std::atomic<uint64_t>* m_count;
};
#endif

/** 测试用任务，只负责执行预先准备的生产循环。
*/
class ProcessWorkBenchmarkTask : public CTask
{
public:
    /** 保存生产函数；构造在计时前完成。
    @param [in] work 生产函数
    @param [in,out] failure 测试失败标记
    */
    ProcessWorkBenchmarkTask(const std::function<void()>& work, std::atomic<bool>& failure);

    /** 运行生产函数，异常转为测试失败。
    */
    void doTask();

    /** 测试被意外取消时标记失败。
    */
    void stopTask();

private:
    // 在计时前构造的生产循环
    std::function<void()> m_work;
    // 借用全批次异常标记
    std::atomic<bool>* m_failure;
};

/** 单消费者核对独立序号账本；不在回调中打印、加锁或动态分配。
*/
class ProcessWorkBenchmarkReceiver : public ProcessReceiveCallback
{
public:
    /** 预分配各发送流的预期序号。
    @param [in] streams 总生产线程数
    @param [in] count 每个生产线程的消息数
    @param [in] bytes 正文长度
    */
    ProcessWorkBenchmarkReceiver(int32_t streams, int32_t count, int32_t bytes);

    /** 核对消息序号、长度及正文首尾，功能测试另行逐字节验证。
    @param [in] pid 实际发送进程
    @param [in] data 正文
    @param [in] length 正文长度
    @param [in] type 测试序号
    */
    void receive(int32_t pid, const char* data, int32_t length, MessageType type);

    /** 统计满载事件，不改变生产时序。
    @param [in] count 当前块数
    */
    void bufferFull(int32_t count);

public:
    // 已经接收的预热消息计数
    std::atomic<int32_t> m_warmup;
    // 最后业务回调完成的QPC纳秒，0尚未完成
    std::atomic<int64_t> m_finished;
    // 账本错误计数，不随意忽略异常报文
    std::atomic<int32_t> m_bad;
    // 首个错误报文的类型，先写明细再通过m_bad发布
    int32_t m_firstBadValue;
    // 首个错误所属流的独立预期序号，非法流为-1
    int32_t m_firstBadExpected;
    // 首个错误报文的发送进程
    int32_t m_firstBadPid;
    // 首个错误报文声明的正文长度
    int32_t m_firstBadLength;
    // 发现首个错误前已核对通过的消息数
    int32_t m_firstBadReceived;
    // 发布不可变PID与账本准备状态
    std::atomic<bool> m_armed;
    // 每个生产进程的实际PID，计时前固定
    std::vector<int32_t> m_pids;
    // 每个进程的生产线程数
    int32_t m_threadsPerProcess;

private:
    // 每个生产流的消息总数
    int32_t m_count;
    // 正文声明长度
    int32_t m_bytes;
    // 唯一消费者私有的累计消息数
    int32_t m_received;
    // 每个流下一条预期序号，预分配后不扩容
    std::vector<int32_t> m_next;
};

/** 独立或集成的性能入口，子进程共用相同参数协议
@param [in] argc 包含程序位置的参数数量
@param [in] argv 参数数组，借用至返回
@return 0完整验证通过，否则失败
*/
int RunProcessWorkBenchmark(int argc, char** argv);