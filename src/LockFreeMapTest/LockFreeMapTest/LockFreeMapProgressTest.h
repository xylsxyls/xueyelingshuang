#pragma once
#include "LockFreeMap/LockFreeMapAPI.h"
#include <ostream>
#include <vector>

/** 仅查询参数携带交错动作的测试键，已发布键不保存动作或测试状态
*/
struct LockFreeMapProgressKey
{
public:
    // 唯一参与严格弱序的整数键值
    int m_value;
    // 查询期间借用的动作；普通存储键为 nullptr
    std::function<void()>* m_compare;

public:
    /** 构造普通键或带交错动作的查询键
    @param [in] value 排序键值
    @param [in] compare 查询期间存活的可选动作
    */
    explicit LockFreeMapProgressKey(int value, std::function<void()>* compare = nullptr);

    /** 执行查询的受控动作，再按整数键比较；动作不得改变排序键值
    @param [in] other 另一操作数
    @return 当前键是否严格小于另一键
    */
    bool operator<(const LockFreeMapProgressKey& other) const;
};

/** 单个工作线程独占写入的进展数据，在线程回收后才汇总
*/
struct LockFreeMapProgressWorker
{
public:
    // 实际调用轮数，包含竞争和容量拒绝
    uint64_t m_attempts;
    // 已提交变更或已获得值的次数
    uint64_t m_successes;
    // 合同允许的竞争、已存在、空容器或暂时资源不足
    uint64_t m_transient;
    // 非预期状态或载荷错误数
    uint64_t m_errors;
    // 两次成功之间的最大间隔，包含起止边界，单位纳秒
    uint64_t m_maxGap;
    // 最后一次成功的单调时间，单位纳秒
    uint64_t m_lastSuccess;
    // 当前线程退出测量循环的单调时间，单位纳秒
    uint64_t m_finish;
    // 混合生产提取场景逐状态计数，用于区分无条目、提交冲突及资源不足
    uint64_t m_statuses[LOCK_FREE_MAP_VERSION_EXHAUSTED + 1];
    // 已接纳或已消费的唯一编号，用于测量结束后的独立逐条核对
    std::vector<uint64_t> m_items;
    // 隔开不同工作线程的频繁计数字段
    unsigned char m_padding[64];

public:
    /** 初始化不含成功记录的统计
    */
    LockFreeMapProgressWorker();

    /** 记录一次已返回的状态，异常枚举值计为错误，避免越界写入
    @param [in] status 本次接口返回状态
    */
    void recordStatus(LockFreeMapStatus status);
};

/** 对当前代码和冻结基线使用同一组持续竞争负载，不预先给各线程固定成功配额
*/
class LockFreeMapProgressTest
{
public:
    /** 运行生产、消费、借用和记录不足的进展检查，并输出逐线程原始数据
    @param [out] report 已打开的结果流
    @param [in] milliseconds 每个场景共同起跑后的测量毫秒数，范围为一至六万
    @return 持续可执行场景逐线程成功、混合场景逐线程返回且整体提交、载荷和资源核对通过时返回零
    */
    static int run(std::ostream& report, unsigned milliseconds);

    /** 使每次比较都改变无关键，观察精确查询是否被迫反复重来；最多注入三十二次
    @param [out] report get 和 visit 各自实际发生的结构变化数
    @return 两种查询中的最大变化次数；载荷或接口检查失败时返回零
    */
    static unsigned queryChurn(std::ostream& report);

private:
    /** 执行单个八线程竞争场景并校验所有成功接纳和消费的编号
    @param [out] report 逐线程结果流
    @param [in] mode 固定场景编号，零至十
    @param [in] milliseconds 持续竞争的测量毫秒数
    @return 独立判定的错误总数
    */
    static int scenario(std::ostream& report, unsigned mode, unsigned milliseconds);

    /** 执行一次实际操作并只记录已提交或已观察到的结果
    @param [in,out] map 被测容器
    @param [in,out] worker 当前线程的私有统计
    @param [in] mode 固定场景编号
    @param [in] id 当前线程编号，零至七
    @return 本轮是否成功提交或取得值
    */
    static bool attempt(LockFreeMap<int, uint64_t>& map, LockFreeMapProgressWorker& worker, unsigned mode, unsigned id);
};