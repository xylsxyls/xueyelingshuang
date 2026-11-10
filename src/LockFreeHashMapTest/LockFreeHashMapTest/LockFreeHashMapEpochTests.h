#pragma once
#include "LockFreeHashMapEpoch.h"
#include <atomic>

/** 只在静止测试夹具中注入代际上限
*/
struct LockFreeHashMapEpochTestAccess
{
    /** 设置极限时钟，不在活动读者期间调用
    @param [in,out] domain 测试保护域
    @param [in] value 非零测试时钟
    */
    static void clock(LockFreeHashMapEpoch& domain, uint64_t value);
};

/** 独立检查新保护域的接纳、重入、暂停和退休截断规则
*/
class LockFreeHashMapEpochTests
{
public:
    /** 运行所有检查，失败记录并正常返回
    @return 零为通过，非零为失败
    */
    static int run();
private:
    /** 记录独立预期，不使用主动崩溃指令
    @param [in] condition 预期是否成立
    @param [in] name 用例上下文，只在调用期间借用
    */
    static void check(bool condition, const char* name);

    /** 验证基础接纳和保护容量边界
    */
    static void admission();

    /** 暂停读者，验证其他操作可进入且老资源不能提前回收
    */
    static void paused();

    /** 验证扫描开始后的退休对象不能使用更早的空登记扫描
    */
    static void retirementOrder();

    /** 覆盖并发登记、提示复用、容量耗尽和释放
    */
    static void contention();
private:
    // 主测试线程的检查数量
    static size_t s_checks;
    // 主测试线程的失败数量
    static size_t s_failed;
};