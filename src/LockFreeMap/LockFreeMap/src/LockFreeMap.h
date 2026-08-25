#pragma once
#include "LockFreeMapMacro.h"
#include "MapNode.h"
#include "MapValueHolder.h"
#include "MapValueProxy.h"
#include <atomic>
#include <functional>
#include <new>
#include <stdint.h>
#include <stddef.h>
#include <utility>
#include <vector>

/** 无锁有序Map
@note 本类使用无锁skip list保存键，默认按std::less<KeyType>升序排列。
@note 本类只依赖C++11标准库，不使用第三方库，不使用mutex/critical section等锁。
@note set/insert/erase/get/contains/count/clear/forEach/keys/values/toVector/first/last/lowerBound/upperBound均可和其它读写接口并发调用。
@note erase是逻辑删除，不会在并发接口中物理移除节点；clear通过切换generation隐藏旧值，复杂度为O(1)。
@note clearAndFreeMemory只能在外部保证没有其它线程访问本对象时调用，否则读线程可能访问到被释放节点。
@note count和遍历类接口在并发写入时返回弱一致结果：不会崩溃，不保证包含调用期间发生的所有新写入。
*/
template <typename KeyType, typename ValueType, typename LessType = std::less<KeyType>, size_t MaxLevel = 16>
class LockFreeMapAPI LockFreeMap
{
public:
    /** 构造函数
    */
    LockFreeMap();

    /** 析构函数，释放所有保留节点和值版本
    @note 析构时外部必须保证没有其它线程继续访问本对象。
    */
    ~LockFreeMap();

    /** 返回下标访问代理
    @param [in] key 键
    @return 返回代理对象，支持map[key] = value写法
    @note 赋值内部调用set。运算符本身不返回写入状态；需要判断失败时请直接调用set/get。
    */
    MapValueProxy<KeyType, ValueType, LessType, MaxLevel> operator[](const KeyType& key);

    /** 插入或覆盖键值
    @param [in] key 键
    @param [in] value 值
    @return 返回是否写入成功，内存分配失败或比较函数抛异常时返回false
    @note 可并发调用。覆盖不会释放旧值，旧值会保留到析构或clearAndFreeMemory。
    */
    bool set(const KeyType& key, const ValueType& value);

    /** 插入键值，键已存在时不覆盖
    @param [in] key 键
    @param [in] value 值
    @return 插入成功返回true，键已存在、内存分配失败或比较函数抛异常时返回false
    @note 可并发调用。键存在但属于旧generation或已删除时，允许重新插入。
    */
    bool insert(const KeyType& key, const ValueType& value);

    /** 删除键
    @param [in] key 键
    @return 删除成功返回true，键不存在返回false
    @note 可并发调用。删除只清空当前可见值，不物理移除跳表节点。
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
    @note 可并发调用；无并发写入时精确，有并发写入时返回弱一致结果。
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
    @note 可并发调用。该函数不会释放旧节点和值版本，只让旧generation值不可见。
    */
    void clear();

    /** 清空并释放已保留节点和值版本
    @note 该函数不是并发安全接口，调用方必须保证没有其它线程访问本对象。
    */
    void clearAndFreeMemory();

    /** 按key升序遍历当前可见键值
    @param [in] function 回调函数，形如function(const KeyType&, const ValueType&)
    @return 返回回调次数
    @note 可并发调用；遍历期间新增、覆盖、删除的节点不保证被本轮遍历看到。
    */
    template <typename FunctionType>
    size_t forEach(FunctionType function) const;

    /** 读取所有当前可见键
    @return 返回按key升序排列的键列表
    @note 可并发调用；并发写入时返回弱一致快照。
    */
    std::vector<KeyType> keys() const;

    /** 读取所有当前可见值
    @return 返回按key升序排列的值列表
    @note 可并发调用；并发写入时返回弱一致快照。
    */
    std::vector<ValueType> values() const;

    /** 读取所有当前可见键值对
    @return 返回按key升序排列的键值对列表
    @note 可并发调用；并发写入时返回弱一致快照。
    */
    std::vector<std::pair<KeyType, ValueType>> toVector() const;

    /** 获取第一个可见键值
    @param [out] key 查询成功时写入最小键
    @param [out] value 查询成功时写入最小键对应的值
    @return 找到返回true，否则返回false
    @note 可并发调用；并发写入时返回弱一致结果。
    */
    bool first(KeyType& key, ValueType& value) const;

    /** 获取最后一个可见键值
    @param [out] key 查询成功时写入最大键
    @param [out] value 查询成功时写入最大键对应的值
    @return 找到返回true，否则返回false
    @note 可并发调用；并发写入时返回弱一致结果。
    */
    bool last(KeyType& key, ValueType& value) const;

    /** 查询第一个不小于指定键的可见键值
    @param [in] key 查询键
    @param [out] foundKey 查询成功时写入找到的键
    @param [out] value 查询成功时写入找到的值
    @return 找到返回true，否则返回false
    @note 可并发调用；并发写入时返回弱一致结果。
    */
    bool lowerBound(const KeyType& key, KeyType& foundKey, ValueType& value) const;

    /** 查询第一个大于指定键的可见键值
    @param [in] key 查询键
    @param [out] foundKey 查询成功时写入找到的键
    @param [out] value 查询成功时写入找到的值
    @return 找到返回true，否则返回false
    @note 可并发调用；并发写入时返回弱一致结果。
    */
    bool upperBound(const KeyType& key, KeyType& foundKey, ValueType& value) const;

    /** 返回跳表最大层数
    @return 返回模板参数MaxLevel
    */
    size_t maxLevel() const;

    /** 返回已分配但尚未释放的跳表节点数量
    @return 返回保留跳表节点数量
    @note erase/clear后的节点仍计入，析构或clearAndFreeMemory后释放。
    */
    size_t retainedNodeCount() const;

    /** 返回已分配但尚未释放的值版本数量
    @return 返回保留值版本数量
    @note set覆盖、erase和clear后的旧值版本仍计入，析构或clearAndFreeMemory后释放。
    */
    size_t retainedValueCount() const;

private:
    /** 禁止拷贝构造，避免多个Map对象同时管理同一个跳表节点集合
    @param [in] other 另一个Map对象
    */
    LockFreeMap(const LockFreeMap& other);

    /** 禁止赋值，避免多个Map对象同时管理同一个跳表节点集合
    @param [in] other 另一个Map对象
    @return 返回当前对象引用
    */
    LockFreeMap& operator=(const LockFreeMap& other);

    /** 比较左键是否小于右键
    @param [in] left 左键
    @param [in] right 右键
    @param [out] result 成功时写入比较结果
    @return 成功返回true，比较函数抛异常时返回false
    */
    bool keyLess(const KeyType& left, const KeyType& right, bool& result) const;

    /** 比较两个键是否等价
    @param [in] left 左键
    @param [in] right 右键
    @param [out] result 成功时写入比较结果
    @return 成功返回true，比较函数抛异常时返回false
    */
    bool keyEqual(const KeyType& left, const KeyType& right, bool& result) const;

    /** 生成新节点层数
    @return 返回1到MaxLevel之间的层数
    */
    size_t randomLevel();

    /** 查找键所在位置
    @param [in] key 键
    @param [out] predecessors 每一层的前驱节点数组，可为nullptr
    @param [out] successors 每一层的后继节点数组，可为nullptr
    @param [out] foundNode 找到等价键时写入节点指针，可为nullptr
    @return 成功返回true，比较函数抛异常时返回false
    */
    bool findPosition(const KeyType& key,
                      MapNode<KeyType, ValueType, MaxLevel>** predecessors,
                      MapNode<KeyType, ValueType, MaxLevel>** successors,
                      MapNode<KeyType, ValueType, MaxLevel>** foundNode) const;

    /** 查找键对应的跳表节点
    @param [in] key 键
    @param [out] node 找到时写入节点指针，未找到写入nullptr
    @return 成功返回true，比较函数抛异常时返回false
    */
    bool findNode(const KeyType& key, MapNode<KeyType, ValueType, MaxLevel>** node) const;

    /** 查找或创建键对应的跳表节点
    @param [in] key 键
    @param [out] node 成功时写入节点指针
    @return 成功返回true，内存分配失败或比较函数抛异常时返回false
    */
    bool findOrCreateNode(const KeyType& key, MapNode<KeyType, ValueType, MaxLevel>** node);

    /** 创建跳表节点
    @param [in] key 节点键
    @param [in] level 节点层数
    @return 成功返回节点指针，失败返回nullptr
    */
    MapNode<KeyType, ValueType, MaxLevel>* createNode(const KeyType& key, size_t level);

    /** 创建值版本节点
    @param [in] value 值
    @param [in] generation 逻辑版本
    @return 成功返回值版本节点指针，失败返回nullptr
    */
    MapValueHolder<ValueType>* createValueHolder(const ValueType& value, uint64_t generation);

    /** 释放尚未发布到MapNode的值版本节点
    @param [in] valueHolder 值版本节点
    */
    void releaseUnusedValueHolder(MapValueHolder<ValueType>* valueHolder);

    /** 比较并交换指定层的后继节点
    @param [in] predecessor 前驱节点，nullptr表示当前层头指针
    @param [in] level 层下标
    @param [in,out] expected 期望后继节点，失败时写入实际后继节点
    @param [in] desired 新后继节点
    @return 交换成功返回true
    */
    bool compareAndSwapNext(MapNode<KeyType, ValueType, MaxLevel>* predecessor,
                            size_t level,
                            MapNode<KeyType, ValueType, MaxLevel>*& expected,
                            MapNode<KeyType, ValueType, MaxLevel>* desired);

    /** 补齐新节点在高层上的链接
    @param [in] node 已经插入第0层的节点
    */
    void linkUpperLevels(MapNode<KeyType, ValueType, MaxLevel>* node);

    /** 释放第0层链表中的所有节点
    @param [in] node 待释放链表头节点
    */
    void releaseNodeList(MapNode<KeyType, ValueType, MaxLevel>* node);

    /** 释放所有保留节点和值版本
    */
    void releaseAllNodes();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 跳表每一层的头指针
    std::atomic<MapNode<KeyType, ValueType, MaxLevel>*> m_heads[MaxLevel];
    // 键排序比较函数对象
    LessType m_less;
    // 当前逻辑版本，clear通过递增版本隐藏旧值
    std::atomic<uint64_t> m_generation;
    // 已分配但尚未释放的跳表节点数量
    std::atomic<size_t> m_retainedNodeCount;
    // 已分配但尚未释放的值版本数量
    std::atomic<size_t> m_retainedValueCount;
    // 随机层数种子
    std::atomic<uint32_t> m_randomSeed;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

#include "LockFreeMap.inl"