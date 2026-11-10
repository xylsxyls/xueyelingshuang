#pragma once
#include "LockFreeHashMapPerfModels.h"
#include "LockFreeHashMapPerfMemory.h"
#include "LockFreeHashMapTestThread.h"
#if defined(LOCKFREEHASHMAP_PERF_STD_MUTEX)
#if defined(LOCKFREEHASHMAP_PERF_CURRENT)
#error Standard mutex baseline has no version snapshot interface
#endif
#include "LockFreeHashMapStdMutex.h"
#else
#include <LockFreeHashMap.h>
#endif
#include <algorithm>
#include <atomic>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

/** 生成两版相同的键或值，测试载荷在计时和分配计量前准备
*/
template <typename ValueType>
struct LockFreeHashMapPerfData
{
public:
    /** 从标识创建测试载荷
    @param [in] marker 非负标识
    @return 独立载荷；分配异常由测试入口处理
    */
    static ValueType make(size_t marker);
};

/** 字符串保证超过短字符串容量，覆盖真实键和值的动态存储
*/
template <>
struct LockFreeHashMapPerfData<std::string>
{
public:
    /** 创建 64 字节前缀和十进制标识组成的字符串
    @param [in] marker 标识
    @return 独立字符串
    */
    static std::string make(size_t marker);
};

/** 宽整数键高位非零，避免误把 64 位类型当成小整数缓存负载
*/
template <>
struct LockFreeHashMapPerfData<uint64_t>
{
public:
    /** 创建超出 32 位范围的整数
    @param [in] marker 标识
    @return 高位非零的值
    */
    static uint64_t make(size_t marker);
};

/** 预先准备的共享值，吞吐计时不混入业务对象的创建
*/
template <>
struct LockFreeHashMapPerfData<std::shared_ptr<const uint64_t>>
{
public:
    /** 创建拥有引用
    @param [in] marker 业务标识
    @return 独立业务对象的共享指针
    */
    static std::shared_ptr<const uint64_t> make(size_t marker);
};

/** 专用全碰撞哈希，正常负载仍直接实例化 std::hash
*/
template <typename KeyType>
struct LockFreeHashMapPerfCollisionHash
{
public:
    /** 返回固定哈希
    @param [in] key 不使用的键
    @return 0
    */
    size_t operator()(const KeyType& key) const;
};

/** 两版共同编译的独立负载程序，吞吐、抽样延迟及内存计量相互分离
*/
class LockFreeHashMapPerformance
{
public:
    /** 执行一个配置，内存模式由编译宏选择
    @param [in] spec 有效配置
    @param [in] repeats 相同工作量的独立批次数，1 至 64
    @return 数据及资源校验通过为 true
    */
    static bool dispatch(const LockFreeHashMapPerfSpec& spec, int32_t repeats);
private:
    /** 选择正常哈希或全碰撞实例
    @param [in] spec 配置
    @param [in] repeats 批次数
    @return 测量通过为 true
    */
    template <typename KeyType, typename ValueType>
    static bool select(const LockFreeHashMapPerfSpec& spec, int32_t repeats);

    /** 重复吞吐批次后另行执行一批延迟抽样
    @param [in] spec 配置
    @param [in] repeats 批次数
    @return 全部批次正确为 true
    */
    template <typename KeyType, typename ValueType, typename HashType>
    static bool measure(const LockFreeHashMapPerfSpec& spec, int32_t repeats);

    /** 执行一个全新 map 的批次，构造、预热和析构不计入操作吞吐
    @param [in] spec 配置
    @param [in] keys 命中键及缺失键，调用期借用
    @param [in] candidates 每个键的两个候选值，调用期借用
    @param [in] sampled 是否在独立延迟阶段抽样
    @param [in,out] samples 接收采样秒数
    @param [out] seconds 单线程直接调用耗时；多线程发令至最后工作结束，不含 join
    @param [out] extraCalls 有界重试导致的额外公开调用数，计入时间但不算作完成的逻辑工作
    @return 数据正确且工作线程无隐藏异常为 true
    */
    template <typename KeyType, typename ValueType, typename HashType, bool ReadOnly = false>
    static bool batch(const LockFreeHashMapPerfSpec& spec, const std::vector<KeyType>& keys,
        const std::vector<ValueType>& candidates, bool sampled, std::vector<double>& samples, double& seconds, uint64_t& extraCalls);

    /** 预先填充一组独立表，连续清空以消除单次 clear 与线程启动的比例失真
    @param [in] spec 单线程 clear 配置，iterations 表示独立表数
    @param [in] keys 输入键，调用期借用
    @param [in] candidates 输入值，调用期借用
    @param [in] sampled 是否逐调用采样延迟
    @param [in,out] samples 接收采样秒数
    @param [out] seconds 仅连续 clear 调用阶段耗时
    @return 每张表原内容正确且清空后不可见为 true
    */
    template <typename KeyType, typename ValueType, typename HashType>
    static bool clearBatch(const LockFreeHashMapPerfSpec& spec, const std::vector<KeyType>& keys,
        const std::vector<ValueType>& candidates, bool sampled, std::vector<double>& samples, double& seconds);

    /** 两版使用相同的最多 1024 次写入尝试，处理明确允许的暂时拒绝
    @param [in,out] map 被测表，调用期借用
    @param [in] key 键
    @param [in] value 完整候选值
    @param [in,out] extraCalls 累计失败后重试次数
    @return 在界内提交成功为 true，始终失败为 false
    */
    template <typename MapType, typename KeyType, typename ValueType>
    static bool write(MapType& map, const KeyType& key, const ValueType& value, uint64_t& extraCalls);

    /** 不创建工作线程的独立分配计量，完整析构后必须归零
    @param [in] spec 内存计量配置
    @return 值和净申请校验正确为 true
    */
    template <typename KeyType, typename ValueType>
    static bool memory(const LockFreeHashMapPerfSpec& spec);
};

#include "LockFreeHashMapPerformance.inl"