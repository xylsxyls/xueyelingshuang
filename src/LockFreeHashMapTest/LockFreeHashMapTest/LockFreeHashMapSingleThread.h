#pragma once
#include "LockFreeHashMapAPI.h"
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/** 单线程对照项目，集合操作的一个操作单位是一整张表 */
enum LockFreeHashMapSingleOperation
{
    // 命中并复制值
    SingleGetHit,
    // 未命中查询
    SingleGetMiss,
    // 存在性命中
    SingleContainsHit,
    // 存在性未命中
    SingleContainsMiss,
    // 单键计数
    SingleCountKey,
    // 返回值副本
    SingleValue,
    // 已存在键的下标读取
    SingleSubscriptRead,
    // 更新已存在键
    SingleSet,
    // 下标更新已存在键
    SingleSubscriptWrite,
    // 向空表插入全部新键
    SingleInsertNew,
    // 对已存在键尝试插入
    SingleInsertExists,
    // 删除存在的键
    SingleEraseHit,
    // 删除不存在的键
    SingleEraseMiss,
    // 一次删除再插入为一个循环
    SingleChurn,
    // 每十次中九次查询、一次更新
    SingleMixed,
    // 非空表数量
    SingleSize,
    // 非空表判空
    SingleEmpty,
    // 遍历整表并累计键值
    SingleTraverse,
    // 导出所有键
    SingleKeys,
    // 导出所有值
    SingleValues,
    // 导出所有键值对
    SinglePairs,
    // 清空接口返回，不强迫延迟回收
    SingleClear,
    // 清空并完成节点析构，允许保留空闲池和桶
    SingleClearReclaim,
    // 构造、填表和析构的完整生命周期
    SingleLifecycle,
    // 项目数量，不参与计时
    SingleOperationCount
};

/** 无同步的标准容器适配，只由主线程使用；没有池、缓存、版本或快照包装 */
template <typename V>
class LockFreeHashMapSingleStd
{
public:
    /** 创建指定桶数的标准表，保留默认负载因子
    @param [in] buckets 请求桶数
    */
    explicit LockFreeHashMapSingleStd(size_t buckets);

    /** 查找后复制值
    @param [in] key 键
    @param [out] value 找到时复制，不存在保持原值
    @return 是否找到
    */
    bool get(int32_t key, V& value) const;

    /** 判断存在性
    @param [in] key 键
    @return 是否存在
    */
    bool contains(int32_t key) const;

    /** 读取单键计数
    @param [in] key 键
    @return 零或一
    */
    size_t count(int32_t key) const;

    /** 读取副本或默认值
    @param [in] key 键
    @return 值副本
    */
    V value(int32_t key) const;

    /** 标准下标，不给读取缺失键伪造与本库相同的语义
    @param [in] key 键，读取项目保证存在
    @return 标准值引用
    */
    V& operator[](int32_t key);

    /** 使用标准下标赋值
    @param [in] key 键
    @param [in] value 候选值
    @return 成功为 true，异常交给测量入口
    */
    bool set(int32_t key, const V& value);

    /** 使用 emplace 尝试插入
    @param [in] key 键
    @param [in] value 候选值
    @return 是否插入
    */
    bool insert(int32_t key, const V& value);

    /** 删除单键
    @param [in] key 键
    @return 是否删除
    */
    bool erase(int32_t key);

    /** 读取标准表数量
    @return 条目数
    */
    size_t size() const;

    /** 判断空表
    @return 是否为空
    */
    bool empty() const;

    /** 读取实际桶数
    @return 桶数
    */
    size_t bucketCount() const;

    /** 清空并析构节点，保留桶 */
    void clear();

    /** 整表只读遍历，不复制值
    @param [in] function 当次调用内借用的回调
    @return 回调数
    */
    template <typename F>
    size_t forEach(const F& function) const;

    /** 预留已知数量并复制所有键
    @return 独立键数组
    */
    std::vector<int32_t> keys() const;

    /** 预留已知数量并复制所有值
    @return 独立值数组
    */
    std::vector<V> values() const;

    /** 预留已知数量并复制键值对
    @return 独立键值数组
    */
    std::vector<std::pair<int32_t, V>> toVector() const;
private:
    // 只有标准容器，没有任何互斥量或共享锁
    std::unordered_map<int32_t, V> m_values;
};

/** 只读数量接口的独立翻译单元边界，禁止跨单元优化将调用移出循环 */
class LockFreeHashMapSingleObservation
{
public:
    /** 实际调用数量接口，两角色都承担一次函数边界
    @param [in] map 当前测量表
    @return 条目数
    */
    template <typename Map>
    static size_t size(const Map& map);

    /** 实际调用判空接口，两角色都承担一次函数边界
    @param [in] map 当前测量表
    @return 是否为空
    */
    template <typename Map>
    static bool empty(const Map& map);
};

/** 测量结果，计时外检查错误，保留实际桶数 */
struct LockFreeHashMapSingleSample
{
public:
    // 有效计时秒数
    double m_seconds;
    // 计时内逻辑操作数量
    uint64_t m_calls;
    // 验证错误数量
    uint64_t m_errors;
    // 结果校验和，防止整个负载被优化删除
    uint64_t m_checksum;
    // 计时前实际桶数
    size_t m_bucketsBefore;
    // 计时后实际桶数
    size_t m_bucketsAfter;
public:
    /** 初始化空结果 */
    LockFreeHashMapSingleSample();
};

/** 类型的候选构造及结果校验，动态候选始终在计时外准备 */
template <typename V>
class LockFreeHashMapSingleData
{
public:
    /** 创建可校验的值
    @param [in] marker 正整数标记
    @return 值对象
    */
    static V make(int32_t marker);

    /** 提取校验标记
    @param [in] value 值对象
    @return 对应标记
    */
    static uint64_t marker(const V& value);

    /** 检查夹具以外的拥有引用已释放
    @param [in] value 候选值
    @return 普通整数始终为 true
    */
    static bool released(const V& value);
};

/** shared_ptr 路径保持业务拥有副本，标准表不返回借用指针冒充复制 */
template <>
class LockFreeHashMapSingleData<std::shared_ptr<const int32_t>>
{
public:
    /** 构造共享不可变整数
    @param [in] marker 正整数标记
    @return 拥有指针
    */
    static std::shared_ptr<const int32_t> make(int32_t marker);

    /** 读取业务整数
    @param [in] value 拥有指针
    @return 标记，空指针返回零
    */
    static uint64_t marker(const std::shared_ptr<const int32_t>& value);

    /** 检查节点、输出及导出结果均已释放自己的引用
    @param [in] value 只应由候选数组持有的共享值
    @return 拥有计数等于一为 true
    */
    static bool released(const std::shared_ptr<const int32_t>& value);
};

/** 单线程负载、验证及统计；两个角色使用同一模板循环 */
class LockFreeHashMapSingleThread
{
public:
    /** 执行全部或指定规模的对照，异常转成失败退出
    @param [in] argc 参数数量
    @param [in] argv 前缀、轮数、目标毫秒、可选键数、可选首末项目号
    @return 全部数据与输出有效时为零
    */
    static int run(int argc, char** argv);
private:
    /** 读取单调时间
    @return 秒数，时钟失败抛出由入口捕获的异常
    */
    static double timestamp();

    /** 限制跨操作内存调度的编译器屏障，不发 CPU 锁指令；数量接口另有不可消除边界 */
    static void compilerBarrier();

    /** 取得稳定的项目名
    @param [in] operation 合法项目编号
    @return 静态英文标识
    */
    static const char* operationName(int32_t operation);

    /** 建立标准表
    @param [in] count 键数及请求桶数
    @param [out] map 新建表
    */
    template <typename V>
    static void create(size_t count, std::unique_ptr<LockFreeHashMapSingleStd<V>>& map);

    /** 建立本库表，预留常用容量，保留默认缓存和保护记录配置
    @param [in] count 键数及请求桶数
    @param [out] map 新建表
    */
    template <typename V>
    static void create(size_t count, std::unique_ptr<LockFreeHashMap<int32_t, V>>& map);

    /** 标准表清空已完成节点析构
    @param [in,out] map 待检查的空表
    */
    template <typename V>
    static void reclaim(LockFreeHashMapSingleStd<V>& map);

    /** 推进本库回收，有限次校验收尾，失败由入口报告
    @param [in,out] map 已逻辑清空的表
    */
    template <typename V>
    static void reclaim(LockFreeHashMap<int32_t, V>& map);

    /** 执行一个批次，准备、结果核对及普通销毁不计时
    @param [in] order 确定性打乱的全部键
    @param [in] values 按键下标存放的候选
    @param [in] loops 每轮扫描键表的次数；整表操作为调用次数
    @return 实际耗时、数量及验证结果
    */
    template <typename Map, typename V, int32_t Operation>
    static LockFreeHashMapSingleSample batch(const std::vector<int32_t>& order,
        const std::vector<V>& values, size_t loops);

    /** 编译期选定操作，先预热再独立校准，两角色交替运行
    @param [in] name 值类型名称
    @param [in] order 查询顺序
    @param [in] values 候选值
    @param [in] rounds 正式轮数
    @param [in] milliseconds 校准目标毫秒
    @param [in,out] raw 原始逐轮 CSV
    @param [in,out] summary 汇总 CSV
    @return 数据验证全部通过为 true
    */
    template <typename V, int32_t Operation>
    static bool compare(const char* name, const std::vector<int32_t>& order, const std::vector<V>& values,
        size_t rounds, double milliseconds, std::ofstream& raw, std::ofstream& summary);

    /** 对指定类型和规模运行选择的项目
    @param [in] name 类型名称
    @param [in] count 键数，必须为二的幂
    @param [in] rounds 正式轮数
    @param [in] milliseconds 校准目标毫秒
    @param [in] first 首项目号
    @param [in] last 末项目号
    @param [in,out] raw 原始数据
    @param [in,out] summary 汇总数据
    @return 所有项目通过为 true
    */
    template <typename V>
    static bool suite(const char* name, size_t count, size_t rounds, double milliseconds,
        int32_t first, int32_t last, std::ofstream& raw, std::ofstream& summary);
};

#include "LockFreeHashMapSingleThread.inl"