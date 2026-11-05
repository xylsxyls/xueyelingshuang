#pragma once

#include "LockFreeQueueArena.h"

// 本次操作的段池适配器；借用 Arena，不延长其生命，失败发生在载荷发布前
template <class T>
class LockFreeQueueChannelPool
{
public:
    typedef typename LockFreeQueueArena<T>::Segment Segment;
    typedef typename LockFreeQueueArena<T>::Record Record;

    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    @param [in,out] arena 生命周期覆盖适配器的节点池；不转移所有权
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    */
    LockFreeQueueChannelPool(LockFreeQueueArena<T>& arena, Record* record);

    /** 取得已清空的独占发布段；失败保留原载荷所有权
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Segment* acquire();

    /** 唯一消费者从不可摘除的初始块取得发布段
    @return 满足上述保护范围的借用指针；申请失败或空观察返回 nullptr
    */
    Segment* acquireInitial();

    /** 归还借用资源；生产者上下文须由单一执行流操作
    @param [in,out] segment 受保护或独占的发布段；借用至返回
    */
    void release(Segment* segment);

    /** 唯一消费者摘除发布段后进行有界安全回收
    @param [in,out] segment 受保护或独占的发布段；借用至返回
    */
    void retire(Segment* segment);

    /** 取得最近一次发布段申请结果
    @return 成功或明确失败状态；失败不接纳当前载荷
    */
    LockFreeQueueResult result() const;

private:
    // 借用节点池，必须覆盖本适配器生命周期
    LockFreeQueueArena<T>* m_arena;
    // 借用的独占生产者记录；nullptr 表示消费者适配或未绑定
    Record* m_record;
    // 最近一次申请发布段的状态；失败发生在载荷提交前
    LockFreeQueueResult m_result;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueChannelPool(const LockFreeQueueChannelPool& other) = delete;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueChannelPool& operator=(const LockFreeQueueChannelPool& other) = delete;
};

// 单次操作的登记租约守卫；异常回退同样释放保护，绑定上下文保留登记租约
template <class T>
class LockFreeQueueArenaLease
{
public:
    typedef typename LockFreeQueueArena<T>::Record Record;

    /** 建立本对象确定的初始状态；所属类型约定其所有权与生命周期
    @param [in,out] arena 生命周期覆盖适配器的节点池；不转移所有权
    @param [in,out] record 本次操作独占的登记槽；借用至返回
    @param [in] releaseLease 是否在离开时归还登记租约
    */
    LockFreeQueueArenaLease(LockFreeQueueArena<T>& arena, Record* record, bool releaseLease);

    /** 结束本对象拥有或借用的状态；调用前必须满足所属类型的静止条件
    */
    ~LockFreeQueueArenaLease();

private:
    // 借用节点池，必须覆盖本适配器生命周期
    LockFreeQueueArena<T>* m_arena;
    // 借用的独占生产者记录；nullptr 表示消费者适配或未绑定
    Record* m_record;
    // 析构时是否归还本操作的登记槽
    bool m_releaseLease;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueArenaLease(const LockFreeQueueArenaLease& other) = delete;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueArenaLease& operator=(const LockFreeQueueArenaLease& other) = delete;
};

#include "LockFreeQueueModels.inl"