#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>

/** 两版共有的负载及单独报告的新接口，编号是测试协议而非产品接口
*/
enum LockFreeHashMapPerfOperation
{
    // 按比例执行 get/set
    LockFreeHashMapPerfMixed,
    // 查询不存在的键
    LockFreeHashMapPerfMiss,
    // contains 命中
    LockFreeHashMapPerfContains,
    // contains 未命中
    LockFreeHashMapPerfContainsMiss,
    // 创建此前不存在的键
    LockFreeHashMapPerfInsert,
    // 拒绝重复插入
    LockFreeHashMapPerfInsertExists,
    // 删除已有键
    LockFreeHashMapPerfErase,
    // erase/set/get 三次调用组成一个周期
    LockFreeHashMapPerfChurn,
    // 静止表计数
    LockFreeHashMapPerfSize,
    // 静止表遍历
    LockFreeHashMapPerfVisit,
    // 返回键数组
    LockFreeHashMapPerfKeys,
    // 返回值数组
    LockFreeHashMapPerfValues,
    // 返回键值数组
    LockFreeHashMapPerfPairs,
    // 逐张清空事先填充的独立表，建表及析构不计入吞吐计时
    LockFreeHashMapPerfClear,
    // 清空并重新填充
    LockFreeHashMapPerfRefill,
    // 新接口：拥有快照
    LockFreeHashMapPerfFind,
    // 新接口：已有条目的条件插入
    LockFreeHashMapPerfConditionalExists,
    // 新接口：find 后按版本替换
    LockFreeHashMapPerfCompare,
    // 新接口：find/eraseIf/insertIfAbsent
    LockFreeHashMapPerfConditionalErase
};

/** 不含资源的确定性负载配置，两版使用同一目录和参数
*/
struct LockFreeHashMapPerfSpec
{
    // 从 1 开始的稳定编号
    int32_t m_id;
    // CSV 中的负载名称
    std::string m_name;
    // 0 整数，1 整数共享值，2 字符串共享值，3 宽整数，4 字符串值，5 大值
    int32_t m_family;
    // 逻辑键数量，不随重复批次数增长
    size_t m_keys;
    // 桶数量
    size_t m_buckets;
    // 并发工作线程数量
    int32_t m_threads;
    // get 比例，范围 0 至 100
    int32_t m_readPercent;
    // 每批总循环数，均分至工作线程
    size_t m_iterations;
    // 连续负载的循环倍率，默认一；与重建表和线程的批次数分开记录
    size_t m_iterationMultiplier;
    // 操作类别
    LockFreeHashMapPerfOperation m_operation;
    // 强制所有完整哈希相等
    bool m_collision;
    // 多线程共用同一键区间
    bool m_sharedKeys;
    // 计时前对键 0 的覆盖次数
    size_t m_history;
};

/** 512 字节无动态分配载荷，用来区分标量优化与通用复制成本
*/
struct LockFreeHashMapPerfLargeValue
{
public:
    // 完整初始化的固定载荷
    uint64_t m_words[64];
public:
    /** 创建有确定内容的载荷
    @param [in] marker 标识数值
    */
    explicit LockFreeHashMapPerfLargeValue(uint64_t marker = 0);

    /** 比较完整载荷，防止仅首字正确掩盖复制错误
    @param [in] other 被比较值，调用期借用
    @return 所有字节一致为 true
    */
    bool operator==(const LockFreeHashMapPerfLargeValue& other) const;
};

/** 本次性能测量的协议、目录和单调时钟，不拥有被测 map
*/
class LockFreeHashMapPerfModels
{
public:
    // Windows QPC 频率，线程启动前初始化后只读
    static double s_frequency;
public:
    /** 初始化平台单调时钟
    @return 时钟可用为 true
    */
    static bool initializeClock();

    /** Windows 按物理核心优先建立固定的工作线程 CPU 序列，其他平台保留自动调度
    @return 拓扑和进程允许范围有效为 true，系统查询或分配失败为 false
    */
    static bool initializeWorkers();

    /** 在计时前将工作线程绑定到两版相同的逻辑 CPU；超额线程轮转复用
    @param [in] index 工作线程编号，从零开始
    */
    static void bindWorker(size_t index);

    /** 读取同进程内的单调秒数
    @return 当前秒数，仅用于差值
    */
    static double timestamp();

    /** 生成全部公共负载及新接口负载
    @return 确定顺序的独立配置数组，分配异常交由入口处理
    */
    static std::vector<LockFreeHashMapPerfSpec> catalogue();

    /** 生成单线程分配计量目录，与吞吐程序分开编译运行
    @return 六种键值类型、两种规模和两种更新方式的配置
    */
    static std::vector<LockFreeHashMapPerfSpec> memoryCatalogue();

    /** 仅延长可持续重复的 get/set 负载，键集合、线程数和读写比例保持不变
    @param [in,out] spec 尚未缩放的配置，失败保持原值
    @param [in] multiplier 1 至 256；一表示原有负载
    @return 配置及乘法边界合法为 true
    */
    static bool scaleIterations(LockFreeHashMapPerfSpec& spec, size_t multiplier);

    /** 计算一轮逻辑循环包含的公开 map 调用数
    @param [in] spec 负载配置
    @return 公开调用数，不将业务值校验算成额外调用
    */
    static uint64_t callsPerIteration(const LockFreeHashMapPerfSpec& spec);

    /** 判断原始版是否没有此接口
    @param [in] spec 负载配置
    @return 只能单独测当前版为 true
    */
    static bool currentOnly(const LockFreeHashMapPerfSpec& spec);

    /** 分位数采用排序样本的最近秩定义
    @param [in,out] samples 秒数样本，会排序；空数组返回 0
    @param [in] percentile 百分位，范围 1 至 100
    @return 对应秒数
    */
    static double percentile(std::vector<double>& samples, size_t percentile);
private:
    /** 添加一个合法配置，自动分配编号并检查整除关系
    @param [in,out] result 配置目录
    @param [in] name 唯一名称
    @param [in] family 键值类型编号
    @param [in] keys 总键数
    @param [in] threads 工作线程数
    @param [in] readPercent 读取比例
    @param [in] operation 操作类别
    @param [in] buckets 桶数
    @param [in] collision 是否全哈希碰撞
    @param [in] sharedKeys 是否共用键区间
    @param [in] history 预置旧历史长度
    */
    static void add(std::vector<LockFreeHashMapPerfSpec>& result, const std::string& name,
        int32_t family, size_t keys, int32_t threads, int32_t readPercent,
        LockFreeHashMapPerfOperation operation = LockFreeHashMapPerfMixed,
        size_t buckets = 4096, bool collision = false, bool sharedKeys = false, size_t history = 0);
private:
    // main 在创建线程前填写，此后仅只读；不属于生产库，也不改变其他进程的亲和性
    static std::vector<uintptr_t> s_workerMasks;
};