#pragma once
#include "CTaskRunningIndex.h"
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

    /** 投递任务，普通调用内部处理未提交竞争
    @param [in] task 任务强引用
    @param [in] level 大于零的任务优先级
    @param [in] retryTransient false 仅用于 tryPostTask，允许未提交竞争立即返回
    @return committed 为 true 时禁止重投
    */
    CTaskThreadResult post(const std::shared_ptr<CTask>& task, int32_t level, bool retryTransient = true);

    /** 关闭接纳并等待退出
    @param [in] drain true 排空，false 丢弃排队任务并停止当前任务
    @return 自等待明确拒绝；并发关闭在内部等待同一次 join 完成
    */
    CTaskThreadResult finish(bool drain);

    /** 发布停止意图，不等待线程退出；停止观察的竞争内部重试
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
    /** 执行一次停止通知尝试；取得保护前不会调用业务回调
    @param [in] propertyId 属性 ID
    @param [in] level 优先级过滤
    @param [in] all 是否通知全部属性
    @param [in] currentOnly 是否只通知当前任务
    @return Retry 表示尚未执行任何停止回调
    */
    CTaskThreadResult stopOnce(int32_t propertyId, int32_t level, bool all, bool currentOnly);

    /** 执行一次分级计数查询，失败不保留部分输出
    @param [out] counts 排队计数
    @return 查询结果，普通查询在外层处理竞争
    */
    CTaskThreadResult waitInfoOnce(std::map<int32_t, int32_t>& counts);

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
    @param [in] retryTransient 普通投递可保留本次记录做少量竞争重试，try 投递不使用此优化
    @return 只有成功时发布，容量压力下仅重试尚未发布的记录
    */
    LockFreeQueueResult publish(CTaskStorageCache& cache, CTaskPriorityBucket& bucket, CTaskStorageSlot* record, bool retryTransient);

    /** 安全通知当前高优先级抢占
    @param [in] level 已经成功接纳的新任务优先级
    @param [in] posted 可空，生产者传入自己的记录，防止延迟通知抢占后来的无关任务
    */
    CTaskThreadResult preempt(int32_t level, CTaskStorageSlot* posted = nullptr);

    /** 唯一消费者取得本级队首，一次建立至多 64 项预取前缀，不等待发布者
    @param [in,out] bucket 优先级桶
    */
    void prepareFront(CTaskPriorityBucket& bucket);

    /** 重新取得本级的就绪前缀；与逐任务选择分开，避免主循环展开完整队列消费路径
    @param [in,out] bucket 本地前缀已经耗尽且队列可能就绪的优先级桶
    */
    void refillFront(CTaskPriorityBucket& bucket);

    /** 取得非空前缀后合并有限探测并预取载荷，空队列检查不进入此路径
    @param [in,out] bucket 已经取得非空本地前缀的优先级桶
    */
    void prefetchFront(CTaskPriorityBucket& bucket);

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

    /** 唤醒复查的独立冷路径，避免主循环重复内联整段调度代码
    @param [out] bucket 任务所属桶
    @param [out] record 取出的记录
    @return 与 take 相同；只有空闲握手之后使用
    */
    bool takeAfterWake(CTaskPriorityBucket*& bucket, CTaskStorageSlot*& record);

    /** 唯一消费者准备通用 Map 中的队首并选择最高就绪级别，不消费记录
    @return 观察到的最高就绪桶，空观察返回 nullptr
    */
    CTaskPriorityBucket* chooseGeneral();

    /** 已发布当前身份后，补查更高优先级并发送合作通知
    @param [in,out] record 本次即将执行的记录
    */
    void preemptPending(CTaskStorageSlot& record);

    /** 对已经选定的当前记录发起合作抢占，保留并发关闭重做决定的核对
    @param [in,out] record 由读域或唯一消费者保护的当前记录
    @return 回调结果；重做只记录意图，不在通知线程构造任务
    */
    CTaskThreadResult requestPreemption(CTaskStorageSlot& record);

    /** 仅配置了工厂的任务进入此路径，关闭重做请求并按需调用工厂
    @param [in,out] bucket 原优先级的重做槽
    @param [in,out] record 已经执行返回的记录
    */
    void completeRedo(CTaskPriorityBucket& bucket, CTaskStorageSlot& record);

    /** 唯一消费者生成独立重做记录，异常统一计入错误统计，不越过工作线程
    @param [in] record 原任务，调用期间仍持有原任务强引用
    @param [in] rawClone 工厂返回的非空独立对象，所有权转交本函数
    @return 已构造但未发布的备份记录；空表示取消重做或已记录失败
    */
    CTaskStorageSlot* prepareRedo(const CTaskStorageSlot& record, CTask* rawClone);

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
    // 构造时查询写预取能力，不在运行期修改，也不引入全局初始化锁
    const bool m_writePrefetch;
    // 常用级别的扫描上界只增不减，也用于识别单级队列
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
    // 系统 join 操作权，并发关闭者退避等待完成，投递不使用此状态
    std::atomic<bool> m_joining;
    // 线程主函数已完成业务访问
    std::atomic<bool> m_exited;
    // join 已经完成
    std::atomic<bool> m_joined;
    // 只读启动/关闭状态不与生产者序号批量申请共用缓存行
    char m_lifecyclePadding[64];
    // 常用级别分开发布执行观察位置，生产者不读取无关级别的当前记录
    CTaskRunningIndex m_runningIndex;
    // 已领取的512序号段数量；段号留有足够余量容纳边界并发，业务序号不回绕
    std::atomic<uint64_t> m_sequence;
    // 多生产者编号与单消费者当前状态隔离
    char m_sequencePadding[64];
    // 最近选择的稳定槽地址；完成后以 flags 关闭通知，回收及等待前清空
    std::atomic<CTaskStorageSlot*> m_current;
    // 高频当前地址写入不使生产者读取的稳定优先级缓存行失效
    char m_levelPadding[64];
    // 当前执行优先级，0 表示空闲；同时承担发布当前记录后的握手屏障
    std::atomic<int32_t> m_currentLevel;
    // 唯一消费者提高抢占下界的周期
    uint32_t m_floorUses;
    // 当前观察窗口的最低已选级别，避免周期结束时偶然选中高级任务就反复抬高下界
    int32_t m_floorMinimum;
    // 执行标记与生产者观察的下界分离
    char m_currentPadding[64];
    // 不大于真实当前级别的保守下界；跳过不可能抢占的投递观察
    std::atomic<int32_t> m_preemptionFloor;
    // 下界观察不与额度修改共用缓存行
    char m_floorPadding[64];
    // 有限容量的当前占用及短暂申请数；完成或失败归还，重做沿用原额度
    std::atomic<uint64_t> m_taskClaims;
    // 限额原子与累计统计分开，避免消费者归还污染生产者累计缓存行
    char m_taskClaimsPadding[64];
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
    // 有限资源压力的调度提示；只影响退避，不参与任务接纳或发布判定
    std::atomic<uint32_t> m_pressureYields;
    // 可观察异常/系统失败次数
    std::atomic<uint64_t> m_errors;
    // 生产者每次只读通知位，不与消费者逐任务统计写入共用缓存行
    char m_wakePadding[64];
    // 合并通知，避免每个任务留下一个无效的空队列唤醒许可
    std::atomic<bool> m_wakePending;
    // 新优先级遇到池上限时请求消费者退役空桶，之后可重试未接纳投递
    std::atomic<bool> m_trimBuckets;
};