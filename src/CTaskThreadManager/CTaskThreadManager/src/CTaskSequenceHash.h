#pragma once
#include <stddef.h>
#include <stdint.h>

/** 内部连续序号的散列；保留低位分布及相邻任务的桶局部性，不用于业务属性 ID
*/
class CTaskSequenceHash
{
public:
    /** 将不复用的记录序号分配到二次幂桶；32 位平台使用低 32 位
    @param [in] sequence 内部记录序号，冲突仍由 HashMap 完整键比较处理
    @return 桶散列值
    */
    size_t operator()(uint64_t sequence) const;
};

#include "CTaskSequenceHash.inl"