#pragma once
#include "CTaskThreadRegressionTests.h"
#include "LockFreeQueue/LockFreeQueueChannel.h"

/** 拥有本轮线程及三层门控；析构先放行全部回调，再排空线程
*/
class CTaskV5RedoFixture
{
public:
    /** 建立独立工作线程和未放行的门控
    */
    CTaskV5RedoFixture();

    /** 即使用例断言失败，也放行并等待全部借用本夹具的任务
    */
    ~CTaskV5RedoFixture();

    /** 创建记录顺序的原任务；工厂仅在真实重做时创建下一代
    @param [in] slot 0 至 2 的独立门控及工厂计数位置
    @param [in] marker 原任务的账本标记，重做标记为 marker+1
    @param [in] blocked 原任务是否等本位置放行
    @param [in] redo 是否配置重做意图和延迟工厂
    @return 尚未投递的任务
    */
    std::shared_ptr<TestFactoryTask> create(size_t slot, int32_t marker, bool blocked, bool redo);

    /** 投递并检查接纳，只重试尚未提交的 Retry
    @param [in] task 冻结配置的非空任务
    @param [in] level 任务优先级
    */
    void post(const std::shared_ptr<CTask>& task, int32_t level);

    /** 等待原任务已经进入本轮执行体
    @param [in] slot 门控位置
    */
    void waitEntered(size_t slot);

    /** 显式排空，核对后续断言对应最终账本
    */
    void drain();

public:
    // 完整执行序列，仅测试账本使用互斥保护
    TaskLedger m_ledger;
    // 各层原任务已经进入
    std::atomic<bool> m_entered[3];
    // 各层原任务允许返回
    std::atomic<bool> m_release[3];
    // 各层工厂调用次数
    std::atomic<uint32_t> m_factories[3];
    // 是否让最低层工厂停在已调用但尚未返回的窗口
    std::atomic<bool> m_holdFactory;
    // 最低层工厂已经到达门控
    std::atomic<bool> m_factoryEntered;
    // 允许最低层工厂返回
    std::atomic<bool> m_factoryRelease;
    // 被测线程接口，持有至全部借用回调退出
    std::shared_ptr<CTaskThread> m_thread;

private:
    // 线程登记所有者，先于其借用的门控成员销毁
    TestWorkers m_workers;
};

/** 验证低级重做已生成时，高级插队仍先于低级重做及普通任务
*/
void taskV5PendingRedoPriorityTest();

/** 验证低级和中级依次被打断，两级工厂各执行一次且按原级重做
*/
void taskV5NestedPreemptionTest();

/** 验证更高级已在等待时，优先级选择和同级 FIFO 同时保持
*/
void taskV5QueuedHigherPriorityTest();

/** 验证工厂在途期间发布更高级任务及关闭时的接纳与收尾
*/
void taskV5FactoryPublicationRaceTest();

/** 验证路由回收遵守旧读者保护、单次预算、并发帮助及精确释放
*/
void taskV5ReclaimRoutingTest();

/** 验证连续 64 代继承同一工厂，每代仍在原优先级队首且仅按需构造
*/
void taskV5RepeatedRedoTest();

/** 对照测量百万普通任务中的一百次真实重做；工厂和完成账本必须精确一致
*/
void taskV5RedoPerformanceTest();

/** 验证首次发布更高优先级与下一条任务开始竞争时，抢占通知不会丢失
*/
void taskV5FirstPriorityRaceTest();

/** 验证丢弃关闭与最高级任务交接竞争时，已开始任务仍能收到合作停止
*/
void taskV5ShutdownHandoffTest();

/** 验证回收批次全满、暂停帮助者、部分归还和后续复用均保持精确释放
*/
void taskV5ReclaimBatchesTest();

/** 验证按属性停止时当前任务不受等待级别筛选限制，等待任务仍按级别筛选
*/
void taskV5CurrentPropertyStopTest();

/** 并发关闭不同线程后，成功结果必须同时表示系统线程已结束且管理器登记已移除
*/
void taskV5ConcurrentRemovalTest();

/** 验证发布顺序、跨段消费、部分封口和全部生产者位置的容量回收
*/
void taskV5ChannelPublicationTest();

/** 单个任务的外部调用区间；写入线程各自独占，join 后统一检查
*/
struct CTaskV5HistoryItem
{
public:
    // 投递调用之前的全局事件序号
    uint64_t m_begin;
    // 成功返回之后的全局事件序号
    uint64_t m_end;
    // 消费成功后写入的实际位置，初始为无效值
    size_t m_position;

public:
    /** 建立尚未投递和消费的记录
    */
    CTaskV5HistoryItem();

};

/** 消费者观察空队列时的外部调用区间
*/
struct CTaskV5EmptyObservation
{
public:
    // 空观察调用之前的全局事件序号
    uint64_t m_begin;
    // 空观察返回之后的全局事件序号
    uint64_t m_end;
    // 调用之前已经完成的消费数量
    size_t m_delivered;

public:
    /** 保存一次已经返回 false 的完整观察区间
    @param [in] begin 调用前事件
    @param [in] end 返回后事件
    @param [in] delivered 调用前消费数量
    */
    CTaskV5EmptyObservation(uint64_t begin, uint64_t end, size_t delivered);

};

/** 仅用于串行白盒模型的发布记录，覆盖正式通道所需的保护字段
*/
struct CTaskV5ChannelRecord
{
public:
    // 当前生产者保护的发布段
    LockFreeQueueAtomic<LockFreeQueueChannelSegment<int>*> m_hazard;
    // 已验证发布段的代次
    uint64_t m_generation;
    // 已证明发布的前缀下标
    size_t m_hint;
    // CAS 冲突统计，不参与顺序判定
    LockFreeQueueAtomic<uint64_t> m_retries;
    // 与正式发布记录一致的调度让步状态。
    LockFreeQueueProgress m_progress;

public:
    /** 建立空保护和零代次的独立测试记录
    */
    CTaskV5ChannelRecord();

};

/** 测试段池保留全部退役段，便于恢复暂停生产者检查旧位置；只用于串行模型
*/
class CTaskV5ChannelPool
{
public:
    /** 释放本用例保留的全部测试段，要求通道已停止使用
    */
    ~CTaskV5ChannelPool();

    /** 创建并登记一个私有发布段
    @return 成功返回段地址；分配异常由用例框架报告失败
    */
    LockFreeQueueChannelSegment<int>* acquire();

    /** 创建首个发布段
    @return 成功返回段地址；分配异常由用例框架报告失败
    */
    LockFreeQueueChannelSegment<int>* acquireInitial();

    /** 保留未发布候选供测试结束时统一释放
    @param [in] segment 本夹具已经登记的段
    */
    void release(LockFreeQueueChannelSegment<int>* segment);

    /** 保留退役段供用例继续检查旧观察，测试结束后统一释放
    @param [in] segment 本夹具已经登记的段
    */
    void retire(LockFreeQueueChannelSegment<int>* segment);

private:
    // 独占拥有的全部段，串行用例内访问
    std::vector<LockFreeQueueChannelSegment<int>*> m_segments;
};