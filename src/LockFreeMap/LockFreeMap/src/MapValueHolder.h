#pragma once
#include <atomic>
#include <stdint.h>

/** LockFreeMap内部值版本节点
@note 每次set或insert成功都会创建新的值版本，旧版本会保留到clearAndFreeMemory或析构时释放，避免并发读线程访问已释放内存。
*/
template <typename ValueType>
struct MapValueHolder
{
public:
    // 当前值版本保存的值副本
    ValueType m_value;
    // 当前值版本所属逻辑版本
    uint64_t m_generation;
    // 同一个MapNode保留值链表中的下一个值版本
    std::atomic<MapValueHolder<ValueType>*> m_nextRetained;

public:
    /** 构造值版本节点
    @param [in] value 值副本
    @param [in] generation 逻辑版本
    */
    MapValueHolder(const ValueType& value, uint64_t generation);

    /** 析构函数
    */
    ~MapValueHolder();

private:
    /** 禁止拷贝构造，避免多个值版本节点管理同一段链表关系
    @param [in] other 另一个值版本节点
    */
    MapValueHolder(const MapValueHolder& other);

    /** 禁止赋值，避免多个值版本节点管理同一段链表关系
    @param [in] other 另一个值版本节点
    @return 返回当前对象引用
    */
    MapValueHolder& operator=(const MapValueHolder& other);
};

#include "MapValueHolder.inl"