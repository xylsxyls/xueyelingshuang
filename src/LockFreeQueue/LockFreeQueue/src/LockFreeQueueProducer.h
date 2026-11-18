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
    /** 创建未绑定的生产者上下文，不领取队列登记槽
    */
    LockFreeQueueProducer();

    /** 归还尚持有的上下文缓存与登记租约；所属队列必须仍然存活
    */
    ~LockFreeQueueProducer();

    /** 观察上下文是否绑定；不得与同一上下文的其他操作并发
    @return true表示持有队列登记租约；false表示尚未绑定或已经release
    */
    bool attached() const;

    /** 归还借用资源；生产者上下文须由单一执行流操作
    */
    void release();

    /** 由本线程复制并发布一个元素；竞争持续重试并定期让步，不返回 Busy
    容量等待与Queue::push一致；单线程Wasm无法等待消费者时返回false
    @param [in] value 待复制的载荷；借用至本次调用返回
    @return true已提交；false为初始化、分配或复制等错误，不传播异常
    */
    bool push(const T& value);

    /** 尝试复制并发布；竞争失败为 Busy，额度已满为 CapacityLimit；失败未接纳
    @param [in] value 待复制的载荷；借用至本次调用返回
    @param [in] retryContention true持续重试CAS竞争，false允许返回Busy
    @param [in] retryCapacity true等待容量；先重试4096次，之后每次等待1毫秒；单线程Wasm不能等待时返回ResourceUnavailable
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult tryPush(const T& value, bool retryContention = false, bool retryCapacity = false);

    /** 复用上下文发布一项，并完成发布到调用者后续原子读取之间的完整屏障
    @param [in] value 借用至返回，复制和失败语义与 tryPush 一致
    @param [in] retryContention true持续重试CAS竞争，false允许返回Busy
    @param [in] retryCapacity true等待容量；先重试4096次，之后每次等待1毫秒；单线程Wasm不能等待时返回ResourceUnavailable
    @return 成功时可与消费者 SC 屏障和队列复查配对；不排序返回后的新写入
    */
    LockFreeQueueResult tryPushAndFence(const T& value, bool retryContention = false, bool retryCapacity = false);

    /** 发布调用方拥有的外部载荷，仍消耗一个节点池额度；不得重复提交尚未消费的同一对象
    @param [in,out] entry 成功后借用到消费返回或静止destroy；只提取Entry地址时不要求T可赋值
    @param [in] retryContention true持续重试CAS竞争，false允许返回Busy
    @param [in] retryCapacity true等待容量；先重试4096次，之后每次等待1毫秒；单线程Wasm不能等待时返回ResourceUnavailable
    @return 失败未接纳，既有复制入队接口及其所有权语义不变
    */
    LockFreeQueueResult tryPushEntry(LockFreeQueueEntry<T>& entry, bool retryContention = false, bool retryCapacity = false);

    /** 发布外部载荷，并保证发布先于调用者后续原子读取
    @param [in,out] entry 借用与失败语义同 tryPushEntry
    @param [in] retryContention true持续重试CAS竞争，false允许返回Busy
    @param [in] retryCapacity true等待容量；先重试4096次，之后每次等待1毫秒；单线程Wasm不能等待时返回ResourceUnavailable
    @return 成功路径具有与 tryPushAndFence 相同的发布屏障保证
    */
    LockFreeQueueResult tryPushEntryAndFence(LockFreeQueueEntry<T>& entry, bool retryContention = false, bool retryCapacity = false);

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
    // 借用的所属队列；nullptr表示未绑定，队列必须存活至release完成
    LockFreeQueue<T>* m_owner;
    // 借用的独占生产者记录；nullptr 表示未绑定，绑定期间与m_owner属于同一队列
    typename LockFreeQueueArena<T>::Record* m_record;
};

#include "LockFreeQueueProducer.inl"