#pragma once
#include <atomic>
#include <stdint.h>
#include <stddef.h>

/** 单槽拥有不可变键值，链接低位一表示删除，低位二表示同键替换
候选构造完成后才发布，逻辑终态不可撤销，物理摘除后由池负责安全回收
*/
template <typename KeyType, typename ValueType>
struct LockFreeHashMapNode
{
public:
    // 后继槽地址，最低两位只用于不可逆终态
    std::atomic<uintptr_t> m_next;
    // 构造时计算的哈希
    size_t m_hash;
    // 不可变逻辑代次
    uint64_t m_generation;
    // 实例内永不复用的完整值版本
    uint64_t m_version;
    // 发布后不可修改的键副本
    KeyType m_key;
    // 发布后不可修改的值副本，与键合并在同一个受控池槽中
    ValueType m_value;
public:
    /** 完整构造候选，复制异常交由尚未提交的修改边界捕获
    @param [in] key 键来源
    @param [in] value 值来源
    @param [in] hash 与等价关系一致的哈希
    @param [in] generation 本次可见代次
    @param [in] version 已预留的非零值版本
    */
    LockFreeHashMapNode(const KeyType& key, const ValueType& value, size_t hash, uint64_t generation, uint64_t version);
};

#include "HashMapNode.inl"