#pragma once
#include <functional>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

/** 仅供公平性能对照的标准容器适配器，一张表共用一个 std::mutex
所有公开操作完整持锁；读取复制值后才解锁，遍历不把迭代器借出锁外
保留 unordered_map 默认负载因子及自动扩桶，不添加自定义池、缓存或版本协议
*/
template <typename KeyType, typename ValueType, typename HashType = std::hash<KeyType>>
class LockFreeHashMapStdMutex
{
public:
    /** 创建标准表，构造异常由测试入口捕获
    @param [in] buckets 请求的初始桶数量，实际桶数由标准库决定
    */
    explicit LockFreeHashMapStdMutex(size_t buckets = 4096);

    /** 覆盖已有值或插入新键
    @param [in] key 键
    @param [in] value 候选值
    @return 成功为 true，锁或标准容器异常为 false
    */
    bool set(const KeyType& key, const ValueType& value);

    /** 仅插入不存在的键，已有键不构造无用候选
    @param [in] key 键
    @param [in] value 候选值
    @return 插入成功为 true，已存在或异常为 false
    */
    bool insert(const KeyType& key, const ValueType& value);

    /** 在锁内复制命中值，不借出标准容器内的引用
    @param [in] key 查询键
    @param [out] value 命中时赋值，用户赋值异常可能部分改变输出
    @return 命中且复制成功为 true
    */
    bool get(const KeyType& key, ValueType& value);

    /** 在锁内判断键是否存在
    @param [in] key 查询键
    @return 存在为 true，缺失或异常为 false
    */
    bool contains(const KeyType& key);

    /** 删除存在的条目，析构不越过锁边界
    @param [in] key 待删除键
    @return 实际删除为 true
    */
    bool erase(const KeyType& key);

    /** 返回锁内观察到的元素数量，锁异常由测试入口处理
    @return 元素数量
    */
    size_t size();

    /** 取得标准库的实际桶数，锁异常由测试入口处理
    @return 包括自动扩桶后的桶数
    */
    size_t bucketCount();

    /** 清空条目，保留标准容器的桶；异常由测试入口处理
    */
    void clear();

    /** 用最小空表替换存储，释放原节点和桶；异常由测试入口处理
    */
    void clearAndFreeMemory();

    /** 整次遍历持锁，测试回调禁止重入同一适配器
    @param [in] function 借用当前键值的回调
    @return 已完成回调数量，回调异常时返回已完成数量
    */
    template <typename FunctionType>
    size_t forEach(const FunctionType& function);

    /** 在锁内复制全部键，分配异常由测试入口处理
    @return 独立键数组
    */
    std::vector<KeyType> keys();

    /** 在锁内复制全部值，分配异常由测试入口处理
    @return 独立值数组
    */
    std::vector<ValueType> values();

    /** 在锁内复制全部键值对，分配异常由测试入口处理
    @return 独立键值数组
    */
    std::vector<std::pair<KeyType, ValueType>> toVector();
private:
    /** 禁止复制带同步责任的适配器
    @param [in] other 不允许复制的实例
    */
    LockFreeHashMapStdMutex(const LockFreeHashMapStdMutex& other) = delete;

    /** 禁止复制赋值
    @param [in] other 不允许复制的实例
    @return 不可调用
    */
    LockFreeHashMapStdMutex& operator=(const LockFreeHashMapStdMutex& other) = delete;
private:
    // 读、写、遍历、清空共用此锁，不使用读写锁或分段锁
    std::mutex m_mutex;
    // 使用默认分配器、默认负载因子及正常自动扩桶
    std::unordered_map<KeyType, ValueType, HashType> m_values;
};

#include "LockFreeHashMapStdMutex.inl"