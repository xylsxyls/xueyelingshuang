#pragma once
#include "LockFreeHashMapStdMutex.h"
#include <atomic>
#include <stdint.h>

/** 标准容器适配器的抛异常复制夹具，不用于计时
*/
struct LockFreeHashMapStdMutexValue
{
public:
    // 业务值
    int32_t m_value;
    // 只在无工作线程时切换故障状态
    static bool s_throw;
public:
    /** 创建确定值
    @param [in] value 业务值
    */
    explicit LockFreeHashMapStdMutexValue(int32_t value = 0);

    /** 可注入故障的复制
    @param [in] other 复制来源
    */
    LockFreeHashMapStdMutexValue(const LockFreeHashMapStdMutexValue& other);

    /** 可注入故障的赋值
    @param [in] other 赋值来源
    @return 当前对象
    */
    LockFreeHashMapStdMutexValue& operator=(const LockFreeHashMapStdMutexValue& other);
};

/** 固定哈希迫使标准表的模型和竞争测试覆盖全碰撞
*/
struct LockFreeHashMapStdMutexHash
{
public:
    /** 返回固定哈希
    @param [in] key 不使用的键
    @return 0
    */
    size_t operator()(int32_t key) const;
};

/** 独立核对 std 适配器的语义、锁范围、异常后解锁及拥有引用
*/
class LockFreeHashMapStdMutexTests
{
public:
    /** 执行测试并输出可由 C++ 控制器核对的结果
    @return 零为通过，非零为失败
    */
    static int run();
private:
    /** 累计检查并在失败时结束依赖步骤
    @param [in] condition 是否满足独立预期
    @param [in] name 失败上下文
    */
    static void check(bool condition, const char* name);

    /** 与 std::map 独立模型核对混合操作及所有导出形式
    */
    static void model();

    /** 核对异常后锁归还以及删除后拥有副本存活
    */
    static void exceptionsAndOwnership();

    /** 核对同键唯一插入、并发更新和持锁遍历
    */
    static void concurrent();
private:
    // 仅主线程累计检查数
    static uint64_t s_checks;
    // 仅主线程累计失败数，工作线程另用原子结果传回
    static uint64_t s_failed;
};