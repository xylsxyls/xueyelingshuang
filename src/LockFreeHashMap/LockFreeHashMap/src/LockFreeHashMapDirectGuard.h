#pragma once
#include "LockFreeHashMapStorage.h"

/** 只保护一个不可变值地址的作用域登记，使用与普通操作相同的固定记录预算
不允许追加游标保护、升级为写操作或将借用带出作用域
*/
class LockFreeHashMapDirectGuard
{
public:
    // 当前独占记录，失败为空；除释放 m_used 外不改写其他保护格
    LockFreeHashMapHazards* m_record;
public:
    /** 同时完成记录接纳和直接值保护，解引用前仍须调用方复核来源
    @param [in] records 记录数组，必须存活到作用域结束
    @param [in] count 固定记录数量
    @param [in,out] span 永久登记范围上界
    @param [in] value 非空且最低位为 0 的槽地址，不接管槽所有权
    */
    LockFreeHashMapDirectGuard(LockFreeHashMapHazards* records, size_t count,
        std::atomic<size_t>& span, void* value);

    /** 在值复制和异常展开结束后释放唯一直接保护，不检查或清理未启用的游标
    */
    ~LockFreeHashMapDirectGuard();
private:
    /** 禁止复制独占记录
    @param [in] other 不可复制的源对象
    */
    LockFreeHashMapDirectGuard(const LockFreeHashMapDirectGuard& other);

    /** 禁止复制赋值独占记录
    @param [in] other 不可复制的源对象
    @return 无实现，不可调用
    */
    LockFreeHashMapDirectGuard& operator=(const LockFreeHashMapDirectGuard& other);
};

#include "LockFreeHashMapDirectGuard.inl"