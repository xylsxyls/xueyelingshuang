#pragma once
#include "LockFreeHashMapPool.h"
#include "LockFreeHashMapTestThread.h"

/** 池模块的独立测试载荷，记录构造、析构和复制异常
*/
struct LockFreeHashMapPoolTestValue
{
public:
    // 每个有效载荷的独立标记
    uint64_t m_value;
    // 当前仍存活的载荷数
    LOCKFREEHASHMAP_TEST_ALIGN_64 static std::atomic<int64_t> s_live;
    // 构造为 true 时主动抛出可捕获异常，不触发进程崩溃
    static std::atomic<bool> s_throw;
    // 指定标记的析构暂停，负一关闭，只由当前夹具设置
    LOCKFREEHASHMAP_TEST_ALIGN_64 static std::atomic<int64_t> s_pause;
    // 析构进入暂停点的通知
    static std::atomic<bool> s_entered;
    // 允许已暂停析构继续完成
    static std::atomic<bool> s_resume;
public:
    /** 构造一个可独立核对的载荷
    @param [in] value 数据标记
    */
    explicit LockFreeHashMapPoolTestValue(uint64_t value);

    /** 归还存活计数
    */
    ~LockFreeHashMapPoolTestValue();
};

/** 模块测试进程独占的分配计账和故障注入，不进入性能程序
*/
struct LockFreeHashMapPoolTestAllocation
{
    // main 完成运行库初始化后才开启，避免全局构造顺序影响故障注入
    static bool s_tracking;
    // 实际成功申请且尚未释放的数量
    LOCKFREEHASHMAP_TEST_ALIGN_64 static std::atomic<int64_t> s_live;
    // -1 不注入，零使下次申请失败
    LOCKFREEHASHMAP_TEST_ALIGN_64 static std::atomic<int64_t> s_fail;
};

/** 按独立预期验证位图分配、代际回收和有界预算
*/
class LockFreeHashMapPoolTests
{
public:
    /** 执行所有模块用例并正常返回结果
    @return 零为全部通过，非零为失败
    */
    static int run();
private:
    /** 只运行标准库线程及异常，不调用池，建立稳定的运行库分配基线
    */
    static void warmRuntime();

    /** 记录预期，不使用断言或主动终止
    @param [in] condition 核验结果
    @param [in] name 独立预期名称
    */
    static void check(bool condition, const char* name);

    /** 验证不同位图尾部、满池、重用、退休和外部静止释放
    */
    static void boundaries();

    /** 对初始化与增长的每个申请点注入失败并核对额度及泄漏
    */
    static void exceptions();

    /** 验证正阈值、零阈值、策略拒绝和策略抛异常
    */
    static void policies();

    /** 暂停保护者，验证退休槽不能提前析构且容量失败可返回
    */
    static void paused();

    /** 暂停批量回收中的析构，其他线程仍能增长及完成申请，旧槽不会提前复用
    */
    static void pausedDestructor();

    /** 同一位图字中并发发布新退役位，验证暂停回收者不覆盖新责任，未过宽限期的位完整归还
    */
    static void overlappingReclaim();

    /** 多线程竞争有限额度，独立核对载荷、占用与回收总量
    */
    static void contention();

    /** 单一维护者与申请、统计并发，核对多块缩容下限和持续空闲
    */
    static void shrinking();
private:
    // 主线程检查数
    static size_t s_checks;
    // 主线程失败数
    static size_t s_failed;
};