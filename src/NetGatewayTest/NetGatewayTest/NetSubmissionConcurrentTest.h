#pragma once
#include "NetInterfaceTestAccess.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>

// 多生产者提交的测试拥有状态，最后一个CTask退出后才析构。
struct NetSubmissionConcurrentContext
{
    /** 初始化独立队列和全部观察字段
    */
    NetSubmissionConcurrentContext();

    // 被测队列，主测试线程是唯一协议owner。
    NetSenderSubmission m_submission;
    // 所有生产者已创建后同时放行。
    std::atomic<bool> m_start;
    // 超时、错误或RAII清理请求停止，所有任务主动检查。
    std::atomic<bool> m_stop;
    // 第一个实际失败结果，不覆盖之前错误。
    std::atomic<int32_t> m_failure;
    // 已退出业务循环的生产者数量。
    std::atomic<uint32_t> m_done;
    // owner发布各线程已完成的连续序号，供生产者限制在途窗口。
    std::atomic<uint32_t> m_received[16];
};

// 每条CTask专属一个生产者身份，覆盖同时提交与回执读取。
class NetSubmissionConcurrentTask : public CTask
{
public:
    /** 借用覆盖全部任务执行期的状态
    @param [in,out] context 测试拥有状态
    @param [in] producer 0至15
    */
    NetSubmissionConcurrentTask(NetSubmissionConcurrentContext& context, uint32_t producer);

    /** 交替提交普通及带回执消息，四条一组保持多条在途
    */
    virtual void doTask();

    /** 合作停止，不等待自己或其他线程
    */
    virtual void stopTask();

private:
    // 借用至管理器确认全部任务退出。
    NetSubmissionConcurrentContext& m_context;
    // 构造后固定的生产者身份。
    uint32_t m_producer;
};

// 失败或正常退出均只回收本用例登记的线程，不影响其他测试线程。
class NetSubmissionConcurrentOwner
{
public:
    /** 创建空线程登记
    @param [in,out] context 析构时发布停止标记
    */
    explicit NetSubmissionConcurrentOwner(NetSubmissionConcurrentContext& context);

    /** 合作停止并等待已创建线程，再允许context析构
    */
    ~NetSubmissionConcurrentOwner();

    // 0表示尚未创建，创建后由本对象唯一回收。
    uint32_t m_threads[16];

private:
    /** 禁止复制线程回收责任
    @param [in] other 不可复制
    */
    NetSubmissionConcurrentOwner(const NetSubmissionConcurrentOwner& other);

    /** 禁止复制线程回收责任
    @param [in] other 不可复制
    @return 不提供实现
    */
    NetSubmissionConcurrentOwner& operator=(const NetSubmissionConcurrentOwner& other);

    // 生命周期比本对象长的测试状态。
    NetSubmissionConcurrentContext& m_context;
};

// 网关无锁提交的独立多生产者回归。
class NetSubmissionConcurrentTest
{
public:
    /** 验证竞争重试、每生产者FIFO、回执和最终预算收敛
    */
    static void run();
};