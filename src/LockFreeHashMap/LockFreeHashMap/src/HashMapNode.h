#pragma once
#include <atomic>
#include <new>
#include <stdint.h>

/** LockFreeHashMap内部链表节点
@note 模板节点类型单独放文件，避免在LockFreeHashMap类内部嵌套struct。
*/
template <typename KeyType, typename ValueType>
struct HashMapNode
{
public:
    // 节点键
    KeyType m_key;
    // 节点值副本，nullptr表示删除标记
    ValueType* m_value;
    // 桶内链表的下一个节点
    std::atomic<HashMapNode<KeyType, ValueType>*> m_next;
    // 节点所属逻辑版本
    uint64_t m_generation;

public:
    /** 构造值节点
    @param [in] key 节点键
    @param [in] value 节点值
    @param [in] generation 节点所属版本
    */
    HashMapNode(const KeyType& key, const ValueType& value, uint64_t generation);

    /** 构造删除标记节点
    @param [in] key 节点键
    @param [in] generation 节点所属版本
    */
    HashMapNode(const KeyType& key, uint64_t generation);

    /** 析构函数，释放节点持有的值副本
    */
    ~HashMapNode();

    /** 判断节点是否持有有效值
    @return 持有值返回true，删除标记节点返回false
    */
    bool hasValue() const;
};

#include "HashMapNode.inl"