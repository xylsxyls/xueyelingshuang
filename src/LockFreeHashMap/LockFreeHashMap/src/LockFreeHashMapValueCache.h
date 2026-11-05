#pragma once
#include "LockFreeHashMapTypes.h"
#include "LockFreeHashMapMacro.h"
#include <atomic>
#include <cstring>
#include <limits>

/** 固定副本格，只保存原子整数，不借用池节点或用户资源
身份按顺序一致执行；载荷逐字段发布和获取，使读到新载荷必然在末次身份检查中看见此前失效
发布占用只影响缓存，不阻塞容器操作
*/
struct LockFreeHashMapValueCacheEntry
{
public:
    // 零表示无效；非零偶数为写入前失效身份，奇数为该身份的已发布副本
    std::atomic<uint64_t> m_stamp;
    // 完整清空代次，不与键值打包截断
    std::atomic<uint64_t> m_generation;
    // 完整整数键的无符号数值转换，不截断宽整数或有符号键
    std::atomic<uint64_t> m_key;
    // 完整整数值的对象表示，未使用高字节为零
    std::atomic<uint64_t> m_value;
    // 单次尝试的发布权，持有者暂停时其他操作跳过副本发布
    std::atomic<bool> m_publishing;
public:
    /** 初始化空格，不申请资源
    */
    LockFreeHashMapValueCacheEntry();
};

/** 不支持原子整数副本的类型使用空实现，不增加任何系统分配
*/
template <typename KeyType, typename ValueType, typename EqualType,
    bool Enabled = std::is_integral<KeyType>::value && std::is_integral<ValueType>::value &&
    sizeof(KeyType) <= sizeof(uint64_t) && sizeof(ValueType) <= sizeof(uint64_t) &&
    std::is_same<EqualType, std::equal_to<KeyType>>::value>
class LockFreeHashMapValueCache
{
public:
    /** 非整数类型保持关闭，配置检查由池统一执行
    @param [in] count 请求的固定格数
    @return Ok，不申请内存
    */
    LockFreeHashMapStatus initialize(size_t count);

    /** 非整数类型从不命中
    @param [in] key 查询键
    @param [in] generation 观察代次
    @param [out] value 可空的输出，不修改
    @return false
    */
    bool lookup(const KeyType& key, uint64_t generation, ValueType* value) const;

    /** 无缓存时不执行失效操作
    @param [in] key 查询键，不调用用户哈希或比较器
    @param [in] version 非零候选身份，删除使用零
    */
    void invalidate(const KeyType& key, uint64_t version);

    /** 无缓存时不执行发布操作
    @param [in] key 已提交键
    @param [in] value 已提交值
    @param [in] generation 提交代次
    @param [in] version 本次提交身份
    */
    void publish(const KeyType& key, const ValueType& value, uint64_t generation, uint64_t version);

    /** 交换两个空配置
    @param [in,out] other 同类型对象
    */
    void swap(LockFreeHashMapValueCache& other);

    /** 查询实际缓存格数
    @return 零
    */
    size_t count() const;
};

/** 内置整数副本缓存，失效先于结构提交，提交后单次尝试发布，不提供排序
*/
template <typename KeyType, typename ValueType, typename EqualType>
class LockFreeHashMapValueCache<KeyType, ValueType, EqualType, true>
{
public:
    /** 构造关闭的缓存
    */
    LockFreeHashMapValueCache();

    /** 外部静止后释放固定副本数组
    */
    ~LockFreeHashMapValueCache();

    /** 一次初始化固定格，失败保留空配置
    @param [in] count 二次幂格数，零表示关闭
    @return 初始化状态，不向外抛异常
    */
    LockFreeHashMapStatus initialize(size_t count);

    /** 纯原子读取独立副本，不访问池和用户对象，无重试循环
    @param [in] key 完整查询键
    @param [in] generation 观察代次
    @param [out] value 非空时写入整数副本，未命中不修改
    @return 同身份、同代次且完整键相同时为 true
    */
    bool lookup(const KeyType& key, uint64_t generation, ValueType* value) const;

    /** 每次结构提交尝试前先使对应副本格失效
    @param [in] key 完整键，缓存位置独立于结构桶位置
    @param [in] version 本次唯一候选身份；零及超出副本编码范围时只失效
    */
    void invalidate(const KeyType& key, uint64_t version);

    /** 结构提交成功后尝试发布；失去发布权或失效身份已变化时跳过，不等待其他线程
    @param [in] key 已提交完整整数键
    @param [in] value 已提交完整整数值
    @param [in] generation 提交代次
    @param [in] version 本次唯一候选身份
    */
    void publish(const KeyType& key, const ValueType& value, uint64_t generation, uint64_t version);

    /** 外部静止时交换数组所有权，不分配
    @param [in,out] other 同类型独占缓存
    */
    void swap(LockFreeHashMapValueCache& other);

    /** 查询实际固定格数
    @return 已分配格数
    */
    size_t count() const;
private:
    /** 禁止复制独占副本数组
    @param [in] other 不可复制对象
    */
    LockFreeHashMapValueCache(const LockFreeHashMapValueCache& other);

    /** 禁止复制覆盖数组所有权
    @param [in] other 不可复制对象
    @return 无实现
    */
    LockFreeHashMapValueCache& operator=(const LockFreeHashMapValueCache& other);
private:
    // 独占固定数组，运行期不扩展或释放
    LockFreeHashMapValueCacheEntry* m_entries;
    // 固定格数，关闭时为零
    size_t m_count;
};

#include "LockFreeHashMapValueCache.inl"