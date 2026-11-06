#pragma once
#include "CTaskPriorityBucket.h"
#include "CTaskBucketCache.h"
#include "CTaskStorage.h"
#include "CTaskReadyIndex.h"
#include "Semaphore/SemaphoreAPI.h"
#include <thread>
#include <map>

/** 工作线程状态；队列消费归工作线程，生命周期操作由一次性接纳权排重
*/
class CTaskThreadState
{
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
    @param [in,out] cache 本调用独占的位置及预留序号段
    @return 非零序号，耗尽返回零
    */
    uint64_t nextSequence(CTaskStorageCache& cache);

    /** 预留业务任务额度
    @param [in,out] cache 本调用独占的计数位置
    @return 是否取得额度
    */
    bool reserveTask(CTaskStorageCache& cache);

    /** 观察尚未归还的任务额度，先读取消费者归还量再读取生产者预留量
    @return 弱一致数量；静止时精确，累计值按 uint64_t 模运算相减
    */
    uint64_t outstandingCount() const;

    /** 观察尚未取出的任务数量，包含已发布前的投递预留
    @return 弱一致数量；静止时精确，重做沿用原队列优先级
    */
    uint64_t queuedCount() const;

    /** 在冷路径汇总某优先级的排队数，有限优先级池保持可退役桶协议
    @param [in] bucket 在保护范围内或由消费者持有的桶
    @return 包含发布前预留的弱一致数量
    */
    uint64_t bucketQueuedCount(const CTaskPriorityBucket& bucket) const;

    /** 登记或回滚本次发布预留；常用无上限桶采用分位置计数
    @param [in,out] cache 本调用独占的位置
    @param [in,out] bucket 目标优先级桶
    @param [in] add true 登记，false 回滚
    */
    void countPublication(CTaskStorageCache& cache, CTaskPriorityBucket& bucket, bool add);

    /** 取得允许发布的优先级桶，同时登记一个生产者
    @param [in] level 优先级
    @param [out] bucket 成功后调用者负责归还 writers
    @return 查询或创建结果
    */
    CTaskThreadResult acquireBucket(int32_t level, std::shared_ptr<CTaskPriorityBucket>& bucket);

    /** 向稳定桶发布记录，复用本读侧位置独占的生产者上下文
    @param [in,out] cache 本调用持有读侧租约的缓存
    @param [in,out] bucket 本次有效的优先级桶
    @param [in] record 完全构造但尚未发布的记录
    @return 只有成功时发布，容量压力下仅重试尚未发布的记录
    */
    LockFreeQueueResult publish(CTaskStorageCache& cache, CTaskPriorityBucket& bucket, CTaskStorageSlot* record);

    /** 安全通知当前高优先级抢占
    @param [in] level 已经成功接纳的新任务优先级
    @param [in] posted 可空，生产者传入自己的记录，防止延迟通知抢占后来的无关任务
    */
    CTaskThreadResult preempt(int32_t level, CTaskStorageSlot* posted = nullptr);

    /** 唯一消费者取得本级队首，FIFO 节点每次至多合并 16 项，不等待发布者
    @param [in,out] bucket 优先级桶
    */
    void prepareFront(CTaskPriorityBucket& bucket);

    /** 调用业务停止回调并隔离异常
    @param [in] task 安全任务强引用
    */
    CTaskThreadStatus notifyStop(const std::shared_ptr<CTask>& task);

    /** 唤醒工作线程，失败记诊断；工作线程另有有限补查
    @param [in] fenced 本次队列发布后是否已经执行 SC 屏障
    */
    void wake(bool fenced = false);

    /** 唯一消费者的主循环；异常不会越过线程入口
    */
    void run();

    /** 取得观察到的最高优先级任务
    @param [out] bucket 任务所属桶
    @param [out] record 取出的记录
    @return 是否取到任务
    */
    bool take(CTaskPriorityBucket*& bucket, CTaskStorageSlot*& record);

    /** 执行任务并提交一次重做决定
    @param [in] bucket 原始优先级桶
    @param [in] record 当前记录
    */
    void execute(CTaskPriorityBucket& bucket, CTaskStorageSlot& record);

    /** 仅消费者维护基础容器及退役空桶
    */
    void maintain();

    /** 仅消费者丢弃全部排队引用并清理目录
    */
    void discard();

private:
    // 冻结配置
    const CTaskThreadConfig m_config;
    // 只单向关闭接纳；在途投递由读侧位置保护，不竞争统一接纳计数
    std::atomic<bool> m_accepting;
    // 停止通知及容器快照的借用；工作线程退出前关闭并等待现有借用归还
    CTaskAdmission m_observers;
    // 原生句柄借用计数，join 前关闭并等待借用归还
    CTaskAdmission m_nativeUsers;
    // 优先级到稳定桶强引用
    LockFreeMap<int32_t, std::shared_ptr<CTaskPriorityBucket>> m_buckets;
    // 常用级别只发布一次的缓存，桶退役或遇到未缓存级别时回退通用路径
    CTaskBucketCache m_cachedBuckets[16];
    // 仅无优先级池上限时长期保留常用桶，避免逐任务强引用和写者共享计数
    const bool m_pinCommon;
    // 常用最高待检查级别；发布者提高，消费者逐级降低并复查
    CTaskReadyIndex m_readyIndex;
    // 只会由 false 变为 true，必须在任何未缓存桶发布任务前设置
    std::atomic<bool> m_scanBuckets;
    // 投递和冷路径观察共享的读侧保护；暂停读者仅阻止旧记录复用
    CTaskReadDomain m_readDomain;
    // 一条任务只占一个执行记录，不重复插入 HashMap 控制块
    CTaskStorage m_storage;
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
    // 只读启动/关闭状态不与生产者序号批量申请共用缓存行
    char m_lifecyclePadding[64];
    // 不复用的记录序号
    std::atomic<uint64_t> m_sequence;
    // 多生产者编号与单消费者当前状态隔离
    char m_sequencePadding[64];
    // 当前任务稳定槽地址；观察方持有读侧保护，空闲为 nullptr
    std::atomic<CTaskStorageSlot*> m_current;
    // 高频当前地址写入不使生产者读取的稳定优先级缓存行失效
    char m_levelPadding[64];
    // 最近执行优先级；观察空闲时必须结合当前指针
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
    // 生产者每次只读通知位，不与消费者逐任务统计写入共用缓存行
    char m_wakePadding[64];
    // 合并通知，避免每个任务留下一个无效的空队列唤醒许可
    std::atomic<bool> m_wakePending;
    // 新优先级遇到池上限时请求消费者退役空桶，之后可重试未接纳投递
    std::atomic<bool> m_trimBuckets;
};