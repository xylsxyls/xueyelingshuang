#pragma once

#include "LockFreeQueueArena.h"

// 本次操作的段池适配器；借用 Arena，不延长其生命，失败发生在载荷发布前
template <class T>
class LockFreeQueueChannelPool
{
public:
    typedef typename LockFreeQueueArena<T>::Segment Segment;
    typedef typename LockFreeQueueArena<T>::Record Record;

    /** 借用节点池和可空生产者记录，适配单次发布或消费所需的段操作
    @param [in,out] arena 生命周期覆盖适配器的节点池；不转移所有权
    @param [in,out] record 可空的独占登记槽；nullptr表示消费者适配或无须清理，非空借用至适配器析构
    */
    LockFreeQueueChannelPool(LockFreeQueueArena<T>& arena, Record* record);

    /** 取得已清空的独占发布段；失败保留原载荷所有权
    @return 本次独占且已重置的段；无生产者登记或申请失败返回nullptr，状态通过result读取
    */
    Segment* acquire();

    /** 唯一消费者从不可摘除的初始块取得发布段
    @return 初始块中本次独占的段；无可用安全段时返回nullptr，不创建持久登记
    */
    Segment* acquireInitial();

    /** 归还本调用尚未发布的独占候选段，不进入消费者退役链
    @param [in,out] segment 受保护或独占的发布段；借用至返回
    */
    void release(Segment* segment);

    /** 唯一消费者摘除发布段后进行有界安全回收
    @param [in,out] segment 受保护或独占的发布段；借用至返回
    */
    void retire(Segment* segment);

    /** 取得最近一次发布段申请结果
    @return 最近acquire的结果；尚未acquire时为ResourceUnavailable，acquireInitial不更新该字段
    */
    LockFreeQueueResult result() const;

    /** Inspect spare capacity before parking a contending publisher
    @return Whether the arena currently permits the longer contention pause
    */
    bool amplePublisherCapacity() const;

    /** Observe in SC order whether only the first producer record has been registered
    @return True until another record is registered; the bound never decreases before reset
    */
    bool singlePublisherRegistered() const;

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
    @return 不可调用；此赋值操作已禁用
    */
    LockFreeQueueChannelPool& operator=(const LockFreeQueueChannelPool& other) = delete;
};

// 单次操作的登记租约守卫；异常回退同样释放保护，绑定上下文保留登记租约
template <class T>
class LockFreeQueueArenaLease
{
public:
    typedef typename LockFreeQueueArena<T>::Record Record;

    /** 借用登记记录并建立作用域守卫；不重新申请租约
    @param [in,out] arena 生命周期覆盖本守卫的节点池；不转移所有权
    @param [in,out] record 独占的生产者登记槽，借用至本守卫析构；申请失败时传 nullptr，无须归还
    @param [in] releaseLease 是否在离开时归还登记租约
    */
    LockFreeQueueArenaLease(LockFreeQueueArena<T>& arena, Record* record, bool releaseLease);

    /** 清除分配保护并按配置归还租约，保留可由回收者安全清理的发布提示保护
    */
    ~LockFreeQueueArenaLease();

private:
    // 借用节点池，必须覆盖本守卫生命周期
    LockFreeQueueArena<T>* m_arena;
    // 借用的独占生产者记录；nullptr 表示本次没有取得登记槽
    Record* m_record;
    // 析构时是否归还本操作的登记槽
    bool m_releaseLease;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    */
    LockFreeQueueArenaLease(const LockFreeQueueArenaLease& other) = delete;

    /** 禁止复制或移动；所有权、登记与原子状态不能按位转移
    @param [in] other 禁止复制或移动的源对象
    @return 不可调用；此赋值操作已禁用
    */
    LockFreeQueueArenaLease& operator=(const LockFreeQueueArenaLease& other) = delete;
};

#include "LockFreeQueueModels.inl"