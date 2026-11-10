#pragma once
#include "LockFreeMapBenchmarkClock.h"
#include <map>
#include <vector>
#include <string>
#include <stdint.h>
#ifdef LOCK_FREE_MAP_SINGLE_STD
typedef std::map<uint32_t, uint64_t> LockFreeMapSingleContainer;
#else
#include "LockFreeMap.h"
typedef LockFreeMap<uint32_t, uint64_t> LockFreeMapSingleContainer;
#endif

/** 单线程公共操作基准；标准库分支直接使用 std::map，不添加锁或版本模拟
*/
class LockFreeMapSingleBenchmark
{
public:
    /** 校准或执行一组固定次数的基准，异常由 main 统一转换为退出码
    @param [in] argc 参数数量
    @param [in] argv --calibrate 或逗号分隔的各场景次数
    @return 全部正确返回零，参数、计时或内容校验失败返回非零
    */
    static int run(int argc, char** argv);

private:
    /** 编译器屏障，防止单线程只读结果被移到循环外；不生成硬件锁
    @param [in] container 本次必须可观察的容器地址
    */
    static void barrier(const void* container);

    /** 插入一个确定值，供准备阶段与组合负载共用
    @param [in,out] map 被测容器
    @param [in] key 不存在的偶数键
    @return 插入成功返回 true
    */
    static bool insert(LockFreeMapSingleContainer& map, uint32_t key);

    /** 校验整个容器的数量、顺序和实际值；不计入性能时间
    @param [in] map 被测容器
    @param [in] expected 按偶数键排列的期望值；空数组表示空表
    @return 内容一致返回 true
    */
    static bool validate(const LockFreeMapSingleContainer& map, const std::vector<uint64_t>& expected);

    /** 执行编译期选定的操作，不在热循环内进行运行期场景分派
    @param [in,out] map 被测容器
    @param [in] queries 预生成的确定性随机排列，元素是偶数键
    @param [in] iterations 操作次数；扫描按整表次数，批次按完整插入及清空轮数
    @param [out] checksum 对所有结果求和，避免编译器消除访问
    @return 操作返回值或数量异常时返回 false
    */
    template <size_t Operation>
    static bool exercise(LockFreeMapSingleContainer& map, const std::vector<uint32_t>& queries,
        uint64_t iterations, uint64_t& checksum);

    /** 使用独立公式和最终内容验证负载，再预热并计时
    @param [in] name 场景标识
    @param [in] keys 偶数键数量
    @param [in] iterations 零表示校准；否则按给定固定次数运行
    @return 成功返回 true，并向标准输出写一行 CSV
    */
    template <size_t Operation>
    static bool sample(const char* name, uint32_t keys, uint64_t iterations);
};

#include "LockFreeMapSingleBenchmark.inl"