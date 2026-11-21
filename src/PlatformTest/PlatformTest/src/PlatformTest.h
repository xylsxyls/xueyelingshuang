#pragma once
#include "PlatformTestMacro.h"
#include <stddef.h>
#include <stdint.h>
#include <cstring>
#include <string>

/** 为测试程序按编号选择用例，并按批量统计公共接口的耗时
*/
class PlatformTestAPI PlatformTest
{
public:
    /** 从当前位置读取不超过上界的正整数
    @param [in,out] cursor 首个待解析字符，成功时指向数字后的字符
    @param [in] maximum 允许的最大编号
    @param [out] value 已解析的值，仅成功时可用
    @return 成功返回 true
    */
    static bool caseNumber(const char*& cursor, unsigned maximum, unsigned& value);

    /** 读取指定下标用例的编号与压力测试标记
    @param [in] context 调用方的用例表和执行上下文
    @param [in] index 从零开始的下标
    @param [out] stress 该用例是否属于压力测试
    @return 非零的稳定用例编号
    */
    typedef unsigned (*CaseInfo)(void* context, size_t index, bool& stress);

    /** 执行指定下标的用例
    @param [in,out] context 调用方的用例表和执行上下文
    @param [in] index 从零开始的下标
    @return 零表示通过
    */
    typedef int (*CaseRun)(void* context, size_t index);

    /** 解析控制台选择并按登记顺序同步执行用例，回调上下文在返回前保持有效
    @param [in] argc 命令行参数数量，包含程序名
    @param [in] argv 命令行参数
    @param [in] count 按编号递增登记的用例数量
    @param [in,out] context 传给 info 和 run 的调用方上下文
    @param [in] info 读取单个用例的编号和压力测试标记
    @param [in] run 执行单个选中的用例
    @return 0 表示所有选中用例通过，1 表示有用例失败，2 表示选择无效
    */
    static int runCases(int argc, char** argv, size_t count, void* context, CaseInfo info, CaseRun run);

    /** 执行一批被测操作并生成校验值
    @param [in] context 由调用方持有的工作负载上下文
    @param [in] count 批内迭代次数
    @return 由各次操作结果产生的校验值
    */
    typedef uint64_t (*BenchmarkBatch)(const void* context, uint32_t count);

    /** 预热并校准批量，测量九轮后输出中位数、四分位数、原值和校验值
    @param [in] name 计时报告中的场景名
    @param [in] context 传入 batch 的工作负载上下文，返回前保持有效
    @param [in] batch 执行给定次数的回调，每个计时批仅调用一次
    @param [in] operations 每次迭代的真实操作数，必须大于零
    */
    static void runBenchmark(const char* name, const void* context, BenchmarkBatch batch, uint32_t operations = 1);

private:
    /** 读取本进程用于计时间隔的单调时间
    @return 单位为秒
    */
    static double seconds();
};

/** 控制台功能用例，每个目标只登记可以实际执行的用例
*/
struct LibraryTestCase
{
    // 在本测试工程内稳定的正整数编号
    unsigned id;
    // 运行成本较高的并发、容量或长时间场景
    bool stress;
    // 执行独立场景，返回零表示通过
    int (*run)();
};

/** 使用稳定数字 ID 和既有命令行名称的功能用例
*/
struct LibraryNamedTestCase
{
    // 在测试工程内唯一的正整数
    unsigned id;
    // 是否属于压力测试
    bool stress;
    // 原有 --case 参数接受的英文名称
    const char* name;
};

/** 同一宿主上对一批真实接口调用计时；组装输入和功能断言由调用方放在计时区外
*/
class LibraryBenchmark
{
public:
    /** 校准批量后用同一工作负载采九轮，输出每次操作的中位数、四分位数和各轮原值
    @param [in] name 稳定的用例名，测试文档另记输入和操作含义
    @param [in] batch 接收批量次数并返回由各次真实结果形成的校验值，不得在批内打印
    @param [in] operations 每个批内迭代对应的操作次数，默认一次
    */
    template<class Batch>
    static void run(const char* name, const Batch& batch, uint32_t operations = 1);
};

#include "PlatformTest.inl"