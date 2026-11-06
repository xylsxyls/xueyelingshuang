#pragma once
#include "CTaskPriorityBucket.h"
#include "CTaskBucketCache.h"
#include "CTaskSequenceHash.h"
#include "Semaphore/SemaphoreAPI.h"
#include <thread>
#include <map>

/** 工作线程状态；队列消费归工作线程，生命周期操作由一次性接纳权排重
*/
class CTaskThreadState
{
    friend class CTaskThreadTestAccess;
public:
    /** 构造尚未启动的状态
    @param [in] config 冻结的配置副本
    */
    explicit CTaskThreadState(const CTaskThreadConfig& config);

    /** 所有外部引用退出后销毁；正常情况下已经 join
    */
    ~CTaskThreadState();

    /** 初始化容器和系统线程
    @return 完整启动结果，失败回滚
    */
    CTaskThreadResult start();

    /** 投递一次任务
    @param [in] task 任务强引用
    @param [in] level 大于零的任务优先级
    @return committed 为 true 时禁止重投
    */
    CTaskThreadResult post(const std::shared_ptr<CTask>& task, int32_t level);

    /** 关闭接纳并等待退出
    @param [in] drain true 排空，false 丢弃排队任务并停止当前任务
    @return 自等待和并发生命周期操作明确拒绝
    */
    CTaskThreadResult finish(bool drain);

    /** 仅发布停止意图，不等待
    @param [in] drain 是否排空
    */
    CTaskThreadResult requestFinish(bool drain);

    /** 查询当前调用是否处于本线程或本线程的用户回调
    @return 是否不能同步等待本线程
    */
    bool selfWait() const;

    /** 通知匹配任务，回调在目录保护之外执行
    @param [in] propertyId 属性 ID，all 为 true 时忽略
    @param [in] level 排队任务的优先级过滤，0 为全部
    @param [in] all 是否忽略属性 ID 并通知全部任务
    @param [in] currentOnly 是否只通知当前任务
    @return 通知结果，失败不会移除任务
    */
    CTaskThreadResult stop(int32_t propertyId, int32_t level, bool all, bool currentOnly);

    /** 设置系统线程优先级
    @param [in] priority 系统优先级
    @return 实际系统结果
    */
    CTaskThreadResult setPriority(CTaskSystemPriority priority);

    /** 查询弱一致负载
    @return 负载副本
    */
    CTaskThreadStatistics statistics() const;

    /** 查询分优先级的排队数
    @param [out] counts 先清空再写入
    @return 分配或访问结果
    */
    CTaskThreadResult waitInfo(std::map<int32_t, int32_t>& counts);
private:
    /** 分配不回绕序号
    @return 非零序号，耗尽返回零
    */
    uint64_t nextSequence();

    /** 预留业务任务额度
    @return 是否取得额度
    */
    bool reserveTask();

    /** 观察尚未归还的任务额度，先读取消费者归还量再读取生产者预留量
    @return 弱一致数量；静止时精确，累计值按 uint64_t 模运算相减
    */
    uint64_t outstandingCount() const;

    /** 观察尚未取出的任务数量，包含已发布前的投递预留
    @return 弱一致数量；静止时精确，重做沿用原队列优先级
    */
    uint64_t queuedCount() const;

    /** 取得允许发布的优先级桶，同时登记一个生产者
    @param [in] level 优先级
    @param [out] bucket 成功后调用者负责归还 writers
    @return 查询或创建结果
    */
    CTaskThreadResult acquireBucket(int32_t level, std::shared_ptr<CTaskPriorityBucket>& bucket);

    /** 安全通知当前高优先级抢占
    @param [in] level 已经成功接纳的新任务优先级
    @param [in] posted 可空，生产者传入自己的记录，防止延迟通知抢占后来的无关任务
    */
    CTaskThreadResult preempt(int32_t level, const std::shared_ptr<CTaskRecord>& posted = std::shared_ptr<CTaskRecord>());

    /** 唯一消费者预读至多一个已发布队首，不等待正在准备的生产者
    @param [in,out] bucket 优先级桶
    */
    void prepareFront(CTaskPriorityBucket& bucket);

    /** 调用业务停止回调并隔离异常
    @param [in] task 安全任务强引用
    */
    CTaskThreadStatus notifyStop(const std::shared_ptr<CTask>& task);

    /** 唤醒工作线程，失败记诊断；工作线程另有有限补查
    */
    void wake();

    /** 唯一消费者的主循环；异常不会越过线程入口
    */
    void run();

    /** 取得观察到的最高优先级任务
    @param [out] bucket 任务所属桶
    @param [out] record 取出的记录
    @return 是否取到任务
    */
    bool take(std::shared_ptr<CTaskPriorityBucket>& bucket, std::shared_ptr<CTaskRecord>& record);

    /** 执行任务并提交一次重做决定
    @param [in] bucket 原始优先级桶
    @param [in] record 当前记录
    */
    void execute(const std::shared_ptr<CTaskPriorityBucket>& bucket, const std::shared_ptr<CTaskRecord>& record);

    /** 仅消费者维护基础容器及退役空桶
    */
    void maintain();

    /** 仅消费者丢弃全部排队引用并清理目录
    */
    void discard();

    /** 完成记录删除，保护记录暂不可用时交给消费者后续维护
    @param [in] sequence 要移除的唯一编号
    */
    void eraseRecord(uint64_t sequence);
private:
    // 冻结配置
    const CTaskThreadConfig m_config;
    // 只关闭接纳，不互斥并发投递
    CTaskAdmission m_posts;
    // 避免高频投递借用与外部观察共用缓存行
    char m_postPadding[64];
    // 停止通知及容器快照的借用；工作线程退出前关闭并等待现有借用归还
    CTaskAdmission m_observers;
    // 原生句柄借用计数，join 前关闭并等待借用归还
    CTaskAdmission m_nativeUsers;
    // 优先级到稳定桶强引用
    LockFreeMap<int32_t, std::shared_ptr<CTaskPriorityBucket>> m_buckets;
    // 常用级别只发布一次的缓存，桶退役或遇到未缓存级别时回退通用路径
    CTaskBucketCache m_cachedBuckets[16];
    // 只会由 false 变为 true，必须在任何未缓存桶发布任务前设置
    std::atomic<bool> m_scanBuckets;
    // 每条记录安全引用目录，仅消费者调用 maintain
    LockFreeHashMap<uint64_t, std::shared_ptr<CTaskRecord>, CTaskSequenceHash> m_records;
    // 系统线程，由取得生命周期操作权的一方 join
    std::thread m_thread;
    // 任务和状态改变通知
    Semaphore m_wakeup;
    // 完成以及在途调用收敛通知，有限补查避免依赖单次通知
    Semaphore m_completion;
    // 启动握手：0 初始化中，1 可执行，2 启动失败
    std::atomic<int32_t> m_started;
    // 0 运行，1 排空，2 丢弃
    std::atomic<int32_t> m_stopMode;
    // 生命周期操作权，竞争者直接返回 Busy，不循环等待占有者
    std::atomic<bool> m_joining;
    // 线程主函数已完成业务访问
    std::atomic<bool> m_exited;
    // join 已经完成
    std::atomic<bool> m_joined;
    // 不复用的记录序号
    std::atomic<uint64_t> m_sequence;
    // 多生产者编号与单消费者当前状态隔离
    char m_sequencePadding[64];
    // 当前任务目录编号，0 表示空闲
    std::atomic<uint64_t> m_current;
    // 当前执行优先级
    std::atomic<int32_t> m_currentLevel;
    // 当前任务发布与业务额度计数隔离
    char m_currentPadding[64];
    // 生产者累计预留额度，失败回滚；消费者不修改此计数
    std::atomic<uint64_t> m_outstanding;
    // 接纳额度与队列计数分离，避免无关原子写互相失效
    char m_outstandingPadding[64];
    // 累计入队预留，失败回滚；重做发布也计入此项
    std::atomic<uint64_t> m_queued;
    // 并发投递计数与消费者统计隔离
    char m_queuedPadding[64];
    // 唯一消费者累计归还业务额度，与生产者的原子修改分离
    std::atomic<uint64_t> m_releasedTasks;
    // 唯一消费者累计取出数量；分别与入队预留做无符号模差
    std::atomic<uint64_t> m_dequeuedTasks;
    // 已完成执行次数
    std::atomic<uint64_t> m_executed;
    // 接纳失败次数
    std::atomic<uint64_t> m_rejected;
    // 可观察异常/系统失败次数
    std::atomic<uint64_t> m_errors;
    // 合并通知，避免每个任务留下一个无效的空队列唤醒许可
    std::atomic<bool> m_wakePending;
    // 只有删除实际失败后才扫描完成记录
    std::atomic<bool> m_cleanupNeeded;
    // 新优先级遇到池上限时请求消费者退役空桶，之后可重试未接纳投递
    std::atomic<bool> m_trimBuckets;
};