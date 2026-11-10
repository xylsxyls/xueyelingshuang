#pragma once
#include <map>
#include <mutex>
#include <cstddef>

/** 性能对照用的有序容器，每个公开操作使用同一把标准互斥锁
@note 只实现基准共有接口；析构要求调用方已经回收所有工作线程
*/
template <typename Key, typename Value>
class LockFreeMapBenchmarkMutex
{
public:
    /** 写入或覆盖一个键，异常映射为失败
    @param [in] key 待写键
    @param [in] value 待复制的值
    @return 操作成功时返回true
    */
    bool set(const Key& key, const Value& value);

    /** 在锁内查找并复制值
    @param [in] key 查询键
    @param [out] value 成功时取得值
    @return 找到且复制成功时返回true
    */
    bool get(const Key& key, Value& value);

    /** 删除一个键，并立即释放标准容器节点
    @param [in] key 删除键
    @return 确实删除一个条目时返回true
    */
    bool erase(const Key& key);

    /** 在同一次加锁期间取得最大键和值
    @param [out] key 最大键
    @param [out] value 对应值
    @return 非空且复制成功时返回true
    */
    bool last(Key& key, Value& value);

    /** 查询条目数量，锁异常由基准外围捕获
    @return 当前条目数量
    */
    size_t size();

    /** 查询容器是否为空
    @return 没有条目时返回true
    */
    bool empty();

    /** 清空全部条目并释放节点；异常交给基准外围
    */
    void clear();

    /** 静止期清空，标准容器不保留额外节点池
    */
    void clearAndFreeMemory();

    /** 查询尚保留的键数量
    @return 当前条目数量
    */
    size_t retainedNodeCount();

    /** 查询尚保留的值数量
    @return 当前条目数量
    */
    size_t retainedValueCount();

private:
    // 拥有全部键和值，比较及复制均在锁内执行
    std::map<Key, Value> m_values;
    // 所有操作共用一把锁，不拆分读写或按键分锁
    std::mutex m_mutex;
};

#include "LockFreeMapBenchmarkMutex.inl"