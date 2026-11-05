#pragma once

#include "LockFreeQueueModels.h"
#include "LockFreeQueueProducer.h"

// MPSC FIFO; one consumer owns pop/clear/maintain. Lifecycle/policy changes
// require external quiescence. No batch reversal and no producer link debt.
template <class T>
#if defined(_MSC_VER)
class __declspec(align(8)) LockFreeQueue
#else
class alignas(8) LockFreeQueue
#endif
{
public:
    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    */
    LockFreeQueue();

    /** 结束本对象拥有或借用的状态；调用前必须满足所属类型的静止条件
    */
    ~LockFreeQueue();

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueue(const LockFreeQueue& other) = delete;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueue& operator=(const LockFreeQueue& other) = delete;

    /** 清空旧代并重新初始化；外部静止，失败抛出 LockFreeQueueError
    */
    void init();

    /** 清空旧代并重新初始化；外部静止，失败保持不可接纳状态
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult tryInit();

    /** 唯一消费者逐项丢弃直到观察为空；可与 push 并发，持续生产时不保证有限结束
    */
    void clear();

    /** 外部静止下析构有效载荷并释放本实例全部存储；可重复调用
    */
    void destroy();

    /** 设置节点池参数；仅允许未初始化且外部静止时调用
    @param [in] config 节点数配置，最大容量仅允许 -1 或不小于初始容量；借用至返回
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult configurePool(const LockFreeQueuePoolConfig& config);

    /** 外部静止下替换扩容算法；空策略恢复默认，复制失败保留原策略
    @param [in] policy 线程安全、短小、不递归调用本池的策略；保存其副本
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult setGrowPolicy(const LockFreeQueueGrowPolicy& policy);

    /** 外部静止下替换缩容算法；空策略禁用自动缩容，已退役块继续回收
    @param [in] policy 线程安全、短小、不递归调用本池的策略；保存其副本
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult setShrinkPolicy(const LockFreeQueueShrinkPolicy& policy);

    /** 复制并发布一个元素；失败抛出原复制异常或 LockFreeQueueError
    @param [in] value 待复制的载荷或待更新的原子计数；借用至本次调用返回
    */
    void push(const T& value);

    /** 复制并发布一个元素；成功为唯一所有权转交点，失败未接纳
    @param [in] value 待复制的载荷或待更新的原子计数；借用至本次调用返回
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult tryPush(const T& value);

    /** 取得可选生产者上下文；成功后必须在队列换代或析构前释放
    @param [in,out] producer 未绑定的上下文；成功后借用本队列，须在换代前 release
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult tryAcquireProducer(LockFreeQueueProducer<T>& producer);

    /** 唯一消费者取出一项；nullptr 表示丢弃，输出赋值异常不移除元素
    @param [out] output 接收值，可为 nullptr；赋值抛异常时输出可能部分改变
    @return true 消费一项，false 观察为空或队列未初始化
    */
    bool pop(T* output);

    /** 近似观察队列是否为空；不能用作容量判定或销毁许可
    @return 近似数量是否为零
    */
    bool empty() const;

    /** 近似观察未消费数量；可能计入正在发布项，静止时精确
    @return 未消费项近似数，正在发布或并发观察可能影响结果
    */
    uint64_t size() const;

    /** 唯一消费者执行有预算回收；不启动后台线程，纯空闲需要 owner 主动调用
    @param [in] workBudget 本轮维护访问次数上限，零不做工作
    @return 策略或时钟结果、实际归还缓存节点数及物理释放容量
    */
    LockFreeQueueMaintenanceResult maintain(size_t workBudget = 64);

    /** 取得弱一致资源快照；不得作为严格接纳额度
    @return 实际容量、预留、空闲、已使用及退役容量、字节与诊断计数
    */
    LockFreeQueuePoolStatistics poolStatistics() const;

    /** 核实当前目标使用的原子宽度是否由无锁后端支持
    @return 是否满足上述条件或完成上述操作
    */
    bool atomicsLockFree() const;
private:
    typedef LockFreeQueueArena<T> Arena;
    typedef typename Arena::Node Node;
    typedef typename Arena::Record Record;
    typedef LockFreeQueueChannelPool<T> SegmentPool;

    /** 申请并构造载荷后在槽位 CAS 发布；失败回滚节点及私有缓存
    @param [in] value 待复制的载荷或待更新的原子计数；借用至本次调用返回
    @param [in] propagate 是否传播原始载荷复制异常
    @param [in,out] retained 可空的已绑定登记槽；非空时调用者保留租约
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult pushImpl(const T& value, bool propagate, Record* retained);

    /** 完成已取得首项的输出、消费和有界维护；空返回路径不进入此函数
    @param [in,out] node 唯一消费者借用的非空首节点
    @param [out] output 可为空的输出；赋值失败时保留首项
    @return 成功消费一项
    */
    bool consumeNode(Node* node, T* output);

    /** 归还上下文缓存与登记槽；队列在调用结束前必须存活
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    */
    void releaseProducer(Record& record);

    /** 按赋值异常保证选择 noexcept 移动或复制；不提交消费
    @param [out] output 接收值；pop 可为 nullptr，赋值抛异常时可能已被部分修改
    @param [in,out] source 队列仍拥有的载荷；仅在 noexcept 移动路径改变其内容
    @param [in] moveTag 编译期赋值策略标签
    */
    static void assign(T& output, T& source, std::true_type moveTag);

    /** 按赋值异常保证选择 noexcept 移动或复制；不提交消费
    @param [out] output 接收值；pop 可为 nullptr，赋值抛异常时可能已被部分修改
    @param [in,out] source 队列仍拥有的载荷；仅在 noexcept 移动路径改变其内容
    @param [in] copyTag 编译期赋值策略标签
    */
    static void assign(T& output, T& source, std::false_type copyTag);
    friend class LockFreeQueueProducer<T>;

private:
    // 唯一发布通道，固定池和动态池共用同一 FIFO 算法
    LockFreeQueueChannel<Node, SegmentPool, Record> m_channel;
    // 生产者已准备发布项的模计数；失败回滚，与消费计数求差
    std::atomic<size_t> m_prepared;
    // 缓存行隔离填充，不承载业务状态
    char m_preparedPadding[64];
    // 唯一消费者自动维护的摊销计数
    size_t m_maintenance;
    // 缓存行隔离填充，不承载业务状态
    char m_consumedPadding[64];
    // 本队列拥有的统一节点池，destroy 释放其全部存储
    Arena m_arena;
};

#include "LockFreeQueue.inl"