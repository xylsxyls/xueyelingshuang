#pragma once
#include "CTaskStorageModels.h"

/** 单个优先级最近选择的记录；相邻可写指针间隔一个缓存行
*/
struct CTaskRunningSlot
{
public:
    // 不拥有记录；读取者必须持有线程读域保护
    std::atomic<CTaskStorageSlot*> m_record;
    // 不同级别的生产者观察不使所有级别的指针互相失效
    unsigned char m_padding[64 - sizeof(void*)];

public:
    /** 建立空观察位置
    */
    CTaskRunningSlot();

};

/** 常用优先级的执行观察目录；只查看可能被本次投递抢占的低级别
*/
class CTaskRunningIndex
{
public:
    /** 建立空目录
    */
    CTaskRunningIndex();

    /** 唯一消费者发布已选择的记录；后续仍需完整的抢占握手屏障
    @param [in,out] record 由线程池持有的稳定记录
    @param [in] level 所属桶的冻结级别，必须等于记录级别；1 至 16 级登记，其余忽略
    */
    void publish(CTaskStorageSlot& record, int32_t level);

    /** 在读域保护内选择尚未关闭通知决定的较低级记录
    @param [in] level 本次投递级别，检查范围不超过 1 至 16
    @param [in] minimum 当前级别的保守下界，不得高于实际正在选择的级别
    @return 受调用方读域保护的记录或 nullptr；调用方仍核对完成及投递状态
    */
    CTaskStorageSlot* below(int32_t level, int32_t minimum) const;

    /** 唯一消费者在安全回收或等待之前摘除全部登记，随后才能推进回收 epoch
    */
    void clear();

private:
    // 指针发布位置，生命周期覆盖线程状态
    CTaskRunningSlot m_slots[16];
    // 保持目录末尾与相邻生产者计数的隔离，不承载业务状态
    unsigned char m_endPadding[64 + sizeof(uint32_t)];
};

#include "CTaskRunningIndex.inl"