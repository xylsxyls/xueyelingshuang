#pragma once
#include "LockFreeHashMapMacro.h"
#include "HashMapNode.h"
#include "HashMapValueProxy.h"
#include <atomic>
#include <functional>
#include <new>
#include <stdint.h>
#include <stddef.h>
#include <utility>
#include <vector>

/** 无锁哈希Map
@note 本类只依赖C++11标准库，不使用第三方库，不使用mutex/critical section等锁。
@note set/insert/erase/get/contains/count/clear/forEach/keys/values/toVector/size/empty均可和其它读写接口并发调用。
@note operator[]赋值可并发调用，内部等价set；读取时返回当前值副本，不存在返回默认值。
@note clear是逻辑清空，会切换generation并保留旧节点，旧节点会在析构或clearAndFreeMemory中释放。
@note clearAndFreeMemory只能在外部保证没有其它线程访问本对象时调用，否则读线程可能访问到被释放节点。
@note count/遍历类接口在并发写入时返回弱一致结果：不会崩溃，不保证包含调用期间发生的所有新写入。
*/
template <typename KeyType, typename ValueType, typename HashType = std::hash<KeyType>, typename EqualType = std::equal_to<KeyType>>
class LockFreeHashMapAPI LockFreeHashMap
{
public:
    /** 构造函数
    @param [in] bucketCount 桶数量，会向上调整为2的幂，0按默认值处理
    */
    explicit LockFreeHashMap(size_t bucketCount = 1024);

    /** 析构函数，释放所有保留节点
    @note 析构时外部必须保证没有其它线程继续访问本对象。
    */
    ~LockFreeHashMap();

    /** 返回下标访问代理
    @param [in] key 键
    @return 返回代理对象，支持map[key] = value写法
    @note 赋值内部调用set。运算符本身不返回写入状态；需要判断失败时请直接调用set/get。
    */
    HashMapValueProxy<KeyType, ValueType, HashType, EqualType> operator[](const KeyType& key);

    /** 插入或覆盖键值
    @param [in] key 键
    @param [in] value 值
    @return 返回是否写入成功，内存分配失败时返回false
    @note 可并发调用。覆盖不会原地修改旧值，而是追加新版本节点。
    */
    bool set(const KeyType& key, const ValueType& value);

    /** 插入键值，键已存在时不覆盖
    @param [in] key 键
    @param [in] value 值
    @return 插入成功返回true，键已存在或内存分配失败返回false
    @note 可并发调用。
    */
    bool insert(const KeyType& key, const ValueType& value);

    /** 删除键
    @param [in] key 键
    @return 删除成功返回true，键不存在返回false
    @note 可并发调用。删除通过追加tombstone完成，不会立刻释放旧节点。
    */
    bool erase(const KeyType& key);

    /** 删除键，erase别名
    @param [in] key 键
    @return 删除成功返回true，键不存在返回false
    */
    bool remove(const KeyType& key);

    /** 查询键对应的值
    @param [in] key 键
    @param [out] value 查询成功时写入值副本
    @return 找到返回true，否则返回false
    @note 可并发调用。
    */
    bool get(const KeyType& key, ValueType& value) const;

    /** 查询键对应的值，不存在时返回默认值
    @param [in] key 键
    @param [in] defaultValue 默认值
    @return 返回查到的值或默认值
    @note 可并发调用。
    */
    ValueType value(const KeyType& key, const ValueType& defaultValue = ValueType()) const;

    /** 判断键是否存在
    @param [in] key 键
    @return 存在返回true
    @note 可并发调用。
    */
    bool contains(const KeyType& key) const;

    /** 返回键是否存在，语义对齐std::map::count(key)
    @param [in] key 键
    @return 存在返回1，不存在返回0
    @note 可并发调用。
    */
    size_t count(const KeyType& key) const;

    /** 返回当前键数量
    @return 返回当前可见键数量
    @note 可并发调用；通过扫描当前generation计算，无并发写入时精确，有并发写入时返回弱一致结果。
    */
    size_t count() const;

    /** 返回当前键数量，count()别名
    @return 返回当前可见键数量
    */
    size_t size() const;

    /** 判断是否为空
    @return 为空返回true
    @note 可并发调用；有并发写入时结果只代表调用瞬间附近状态。
    */
    bool empty() const;

    /** 逻辑清空Map
    @note 可并发调用。该函数不会释放旧节点，只让旧generation节点不可见。
    */
    void clear();

    /** 清空并释放已保留节点
    @note 该函数不是并发安全接口，调用方必须保证没有其它线程访问本对象。
    */
    void clearAndFreeMemory();

    /** 遍历当前可见键值
    @param [in] function 回调函数，形如function(const KeyType&, const ValueType&)
    @return 返回回调次数
    @note 可并发调用；遍历期间新增的节点不保证被本轮遍历看到。
    */
    template <typename FunctionType>
    size_t forEach(FunctionType function) const;

    /** 读取所有当前可见键
    @return 返回键列表
    @note 可并发调用；并发写入时返回弱一致快照。
    */
    std::vector<KeyType> keys() const;

    /** 读取所有当前可见值
    @return 返回值列表
    @note 可并发调用；并发写入时返回弱一致快照。
    */
    std::vector<ValueType> values() const;

    /** 读取所有当前可见键值对
    @return 返回键值对列表
    @note 可并发调用；并发写入时返回弱一致快照。
    */
    std::vector<std::pair<KeyType, ValueType>> toVector() const;

    /** 返回桶数量
    @return 返回内部桶数量
    */
    size_t bucketCount() const;

    /** 返回已分配但尚未释放的节点数量
    @return 返回保留节点数量
    @note 该值用于观察内存保留情况，erase/clear后的旧节点仍计入，析构或clearAndFreeMemory后释放。
    */
    size_t retainedNodeCount() const;

private:
    /** 禁止拷贝构造，避免多个Map对象同时管理同一个桶数组和节点链表
    @param [in] other 另一个Map对象
    */
    LockFreeHashMap(const LockFreeHashMap&);

    /** 禁止赋值，避免桶数组和节点链表生命周期被复制
    @param [in] other 另一个Map对象
    @return 返回当前对象引用
    */
    LockFreeHashMap& operator=(const LockFreeHashMap&);

    /** 规格化桶数量
    @param [in] bucketCount 原始桶数量
    @return 返回2的幂桶数量，0会返回默认桶数量
    */
    static size_t normalizeBucketCount(size_t bucketCount);

    /** 计算键所在桶下标
    @param [in] key 键
    @param [out] index 成功时写入桶下标
    @return 成功返回true，hash函数抛异常或桶未初始化时返回false
    */
    bool bucketIndex(const KeyType& key, size_t& index) const;

    /** 比较两个键是否相等
    @param [in] left 左键
    @param [in] right 右键
    @param [out] result 成功时写入比较结果
    @return 成功返回true，比较函数抛异常时返回false
    */
    bool keysEqual(const KeyType& left, const KeyType& right, bool& result) const;

    /** 创建值节点
    @param [in] key 节点键
    @param [in] value 节点值
    @param [in] generation 节点所属版本
    @return 成功返回节点指针，失败返回nullptr
    */
    HashMapNode<KeyType, ValueType>* createValueNode(const KeyType& key, const ValueType& value, uint64_t generation);

    /** 创建删除标记节点
    @param [in] key 节点键
    @param [in] generation 节点所属版本
    @return 成功返回节点指针，失败返回nullptr
    */
    HashMapNode<KeyType, ValueType>* createEraseNode(const KeyType& key, uint64_t generation);

    /** 查找当前版本中键对应的最新可见节点
    @param [in] key 键
    @param [in] generation 查询版本
    @return 找到返回节点指针，否则返回nullptr
    */
    HashMapNode<KeyType, ValueType>* visibleNode(const KeyType& key, uint64_t generation) const;

    /** 判断节点是否是当前键在遍历链表中遇到的第一个可见版本
    @param [in] head 桶链表头节点
    @param [in] node 待判断节点
    @param [in] generation 查询版本
    @return 是第一个可见版本返回true，否则返回false
    */
    bool isFirstVisibleVersion(HashMapNode<KeyType, ValueType>* head, HashMapNode<KeyType, ValueType>* node, uint64_t generation) const;

    /** 释放节点链表
    @param [in] node 待释放链表头节点
    */
    void releaseNodeList(HashMapNode<KeyType, ValueType>* node);

    /** 释放所有桶中保留的节点
    */
    void releaseAllNodes();

private:
    // 桶数组，每个桶保存无锁单向链表头
    std::atomic<HashMapNode<KeyType, ValueType>*>* m_buckets;
    // 桶数量
    size_t m_bucketCount;
    // 桶下标掩码，等于m_bucketCount - 1
    size_t m_bucketMask;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 键hash函数对象
    HashType m_hash;
    // 键相等比较函数对象
    EqualType m_equal;
    // 当前逻辑版本，clear通过递增版本隐藏旧节点
    std::atomic<uint64_t> m_generation;
    // 已分配但尚未释放的节点数量
    std::atomic<size_t> m_retainedNodeCount;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

#include "LockFreeHashMap.inl"