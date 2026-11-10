#pragma once
#include "LockFreeHashMapMacro.h"
#include <atomic>
#include <stdint.h>

/** 无填充的平凡链接位值，仅用于标准原子对象的载荷
*/
struct LockFreeHashMapLinkWord
{
    // 完整地址及低位终态标记
    uintptr_t m_bits;
};

/** 指针宽度的原子链接；私有构造不需要同步，发布后的操作沿用标准原子语义
@note 单字段平凡值避免 v120 整数 atomic 构造中不必要的原子交换；不访问标准库内部布局
*/
class LockFreeHashMapLink
{
public:
    /** 构造尚未发布的链接
    @param [in] value 初始位值，低位标记按哈希表协议解释
    */
    explicit LockFreeHashMapLink(uintptr_t value = 0);

    /** 检查本平台实际采用的原子实现
    @return 标准原子实现保证无锁时为 true
    */
    bool isLockFree() const;

    /** 原子读取链接
    @param [in] order 标准原子加载内存序
    @return 观察到的完整位值
    */
    uintptr_t load(std::memory_order order = std::memory_order_seq_cst) const;

    /** 原子写入链接
    @param [in] value 新位值
    @param [in] order 标准原子存储内存序
    */
    void store(uintptr_t value, std::memory_order order = std::memory_order_seq_cst);

    /** 顺序一致地比较交换，不弱化结构发布及回收的全序约束
    @note 显式固定成功和失败内存序，帮助旧编译器消除通用原子实现的运行期分派
    @param [in,out] expected 预期位值，失败时更新为实际观察值
    @param [in] desired 新位值
    @return 是否完成交换
    */
    LOCKFREEHASHMAP_FORCE_INLINE bool compareExchangeStrong(uintptr_t& expected, uintptr_t desired);

private:
    // 唯一数据成员；所有运行期读写均通过 std::atomic 执行
    std::atomic<LockFreeHashMapLinkWord> m_value;
};

#include "LockFreeHashMapLink.inl"