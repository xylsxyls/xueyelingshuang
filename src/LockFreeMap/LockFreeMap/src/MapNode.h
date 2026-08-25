#pragma once
#include "MapValueHolder.h"
#include <atomic>
#include <stddef.h>
#include <stdint.h>

/** LockFreeMap内部跳表节点
@note 节点一旦插入跳表，不会在并发接口中物理删除，避免无锁遍历过程中访问到已释放节点。
*/
template <typename KeyType, typename ValueType, size_t MaxLevel>
struct MapNode
{
public:
    // 节点键
    KeyType m_key;
    // 当前可见值版本，nullptr表示当前逻辑版本下不可见或已删除
    std::atomic<MapValueHolder<ValueType>*> m_value;
    // 当前节点保留的所有值版本链表头
    std::atomic<MapValueHolder<ValueType>*> m_retainedValues;
    // 跳表每一层的后继节点
    std::atomic<MapNode<KeyType, ValueType, MaxLevel>*> m_next[MaxLevel];
    // 当前节点实际参与的跳表层数
    size_t m_level;

public:
    /** 构造跳表节点
    @param [in] key 节点键
    @param [in] level 节点层数，外部保证范围为1到MaxLevel
    */
    MapNode(const KeyType& key, size_t level);

    /** 析构函数，释放当前节点保留的所有值版本
    */
    ~MapNode();

    /** 追加一个保留值版本
    @param [in] valueHolder 值版本节点，不能为空
    @return 追加成功返回true，valueHolder为空时返回false
    @note 可和同一个MapNode上的其它appendRetainedValue并发调用。
    */
    bool appendRetainedValue(MapValueHolder<ValueType>* valueHolder);

    /** 读取当前逻辑版本下的值副本
    @param [in] generation 查询逻辑版本
    @param [out] value 查询成功时写入值副本
    @return 找到有效值返回true，否则返回false
    */
    bool visibleValue(uint64_t generation, ValueType& value) const;

    /** 判断当前逻辑版本下是否有可见值
    @param [in] generation 查询逻辑版本
    @return 存在可见值返回true
    */
    bool hasVisibleValue(uint64_t generation) const;

    /** 获取当前逻辑版本下的可见值版本节点
    @param [in] generation 查询逻辑版本
    @return 找到返回值版本节点，否则返回nullptr
    */
    MapValueHolder<ValueType>* visibleHolder(uint64_t generation) const;

    /** 判断值版本节点是否属于指定逻辑版本且可见
    @param [in] valueHolder 值版本节点
    @param [in] generation 查询逻辑版本
    @return valueHolder非空且逻辑版本一致时返回true
    */
    bool isVisibleHolder(MapValueHolder<ValueType>* valueHolder, uint64_t generation) const;

    /** 释放当前节点保留的所有值版本
    */
    void releaseValueHolders();

private:
    /** 禁止拷贝构造，避免多个节点管理同一批后继指针和值版本
    @param [in] other 另一个跳表节点
    */
    MapNode(const MapNode& other);

    /** 禁止赋值，避免多个节点管理同一批后继指针和值版本
    @param [in] other 另一个跳表节点
    @return 返回当前对象引用
    */
    MapNode& operator=(const MapNode& other);
};

#include "MapNode.inl"