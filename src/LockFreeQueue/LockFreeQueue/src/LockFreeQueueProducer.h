#pragma once

#include "LockFreeQueueArena.h"
#include "LockFreeQueueEntry.h"

template <class T>
class LockFreeQueue;

// 可选生产者上下文，避免逐次登记；单执行流使用，可在外部同步后转交线程
// 必须先 release 再对队列 init/destroy/析构；不会改变 MPSC 和复制入队语义
template <class T>
class LockFreeQueueProducer
{
public:
    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    */
    LockFreeQueueProducer();

    /** 结束本对象拥有或借用的状态；调用前必须满足所属类型的静止条件
    */
    ~LockFreeQueueProducer();

    /** 观察上下文是否绑定；不得与同一上下文的其他操作并发
    @return 是否满足上述条件或完成上述操作
    */
    bool attached() const;

    /** 归还借用资源；生产者上下文须由单一执行流操作
    */
    void release();

    /** 由本线程复制并发布一个元素；竞争持续重试并定期让步，不返回 Busy
    @param [in] value 待复制的载荷或待更新的原子计数；借用至本次调用返回
    @return Success 已提交，资源、初始化或复制等失败返回明确状态；不传播复制或分配异常
    */
    LockFreeQueueResult push(const T& value);

    /** 尝试复制并发布；竞争失败为 Busy，额度已满为 CapacityLimit；失败未接纳
    @param [in] value 待复制的载荷或待更新的原子计数；借用至本次调用返回
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult tryPush(const T& value);

    /** 复用上下文发布一项，并完成发布到调用者后续原子读取之间的完整屏障
    @param [in] value 借用至返回，复制和失败语义与 tryPush 一致
    @return 成功时可与消费者 SC 屏障和队列复查配对；不排序返回后的新写入
    */
    LockFreeQueueResult tryPushAndFence(const T& value);

    /** 发布调用方拥有的外部载荷，仍消耗一个节点池额度；不得重复提交尚未消费的同一对象
    @param [in,out] entry 成功后借用到消费返回或静止 destroy；T 必须可复制赋值
    @return 失败未接纳，既有复制入队接口及其所有权语义不变
    */
    LockFreeQueueResult tryPushEntry(LockFreeQueueEntry<T>& entry);

    /** 发布外部载荷，并保证发布先于调用者后续原子读取
    @param [in,out] entry 借用与失败语义同 tryPushEntry
    @return 成功路径具有与 tryPushAndFence 相同的发布屏障保证
    */
    LockFreeQueueResult tryPushEntryAndFence(LockFreeQueueEntry<T>& entry);

private:
    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueProducer(const LockFreeQueueProducer& other) = delete;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueProducer& operator=(const LockFreeQueueProducer& other) = delete;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueProducer(LockFreeQueueProducer&& other) = delete;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueProducer& operator=(LockFreeQueueProducer&& other) = delete;

    friend class LockFreeQueue<T>;
    // 借用的所属对象或带有效标志的完整所有者地址，见所属类型契约
    LockFreeQueue<T>* m_owner;
    // 借用的独占生产者记录；nullptr 表示消费者适配或未绑定
    typename LockFreeQueueArena<T>::Record* m_record;
};

#include "LockFreeQueueProducer.inl"