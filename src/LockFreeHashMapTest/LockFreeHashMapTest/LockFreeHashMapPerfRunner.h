#pragma once
#include "LockFreeHashMapPerfModels.h"
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

/** C++ 性能控制器，校准单个被测程序，隔离执行并保留所有轮次
*/
class LockFreeHashMapPerfRunner
{
public:
    /** 逐项执行当前表或标准库对照的吞吐测试
    @param [in] program 被测程序
    @param [in] prefix 新报告路径前缀
    @param [in] rounds 轮数，1 至 9
    @param [in] affinityMask CPU 掩码，0 使用系统调度
    @param [in] first 起始编号
    @param [in] last 结束编号，0 表示末项
    @param [in] standard 选择标准库时跳过没有等价接口的场景
    @param [in] fixedRepeats 每轮批数，0 自动校准
    @param [in] iterationMultiplier 持续读写项的循环倍率
    @return 全部观测及报告写入成功时返回 true
    */
    static bool matrix(const std::string& program, const std::string& prefix, int32_t rounds,
        unsigned affinityMask, int32_t first, int32_t last, bool standard, int32_t fixedRepeats, size_t iterationMultiplier);

    /** 逐项执行独立的内存计量程序
    @param [in] program 内存计量程序
    @param [in] prefix 新报告路径前缀
    @param [in] affinityMask CPU 掩码，0 使用系统调度
    @return 所有内存结果通过并落盘时返回 true
    */
    static bool memoryMatrix(const std::string& program, const std::string& prefix, unsigned affinityMask);

    /** 校验数字字段及负载身份，不接受缺失字段、重复字段或非有限数值
    @param [in] line 完整结果行
    @param [in] spec 期望配置
    @param [in] repeats 期望批次数
    @param [in] memory 是否为内存结果
    @param [out] fields 成功输出字段
    @return 数据合同完整为 true
    */
    static bool parse(const std::string& line, const LockFreeHashMapPerfSpec& spec,
        int32_t repeats, bool memory, std::map<std::string, double>& fields);

    /** 验证协议拒绝伪成功，检查目录唯一性及分位数定义
    @return 自检通过为 true
    */
    static bool selftest();
private:
    /** 单线程固定同一个可用逻辑核心，避免两版落在不同类型的核心上
    @param [in] threads 本项实际工作线程数，目录已校验为正数
    @param [in] allowed 调用方允许的CPU集合，零表示默认调度
    @return 单线程的最低有效位，或多线程的原始集合；不会增加未允许的核心
    */
    static unsigned caseAffinity(int32_t threads, unsigned allowed);

    /** 读取本次性能测试显式优先级选项，缺省不提升；非法环境值拒绝执行
    @return LOCKFREEHASHMAP_PERF_ABOVE_NORMAL为1时返回true，为0或未设置时返回false
    */
    static bool aboveNormal();

    /** 执行和读取一次隔离负载，失败仍保留日志
    @param [in] program 可执行文件
    @param [in] spec 配置
    @param [in] repeats 批次数
    @param [in] memory 内存模式
    @param [in] standard 吞吐测试期望使用加锁的标准表，内存模式由程序自行声明后端
    @param [in] log 不存在的日志路径
    @param [in] mask CPU 掩码
    @return 完整合法字段，失败抛异常由入口报告
    */
    static std::map<std::string, double> measure(const std::string& program,
        const LockFreeHashMapPerfSpec& spec, int32_t repeats, bool memory, bool standard, const std::string& log, unsigned mask);

    /** 校验整个子进程日志的唯一后端与其后唯一结果，失败时忽略输出
    @param [in] input 本次子进程的日志流
    @param [in] spec 当前负载的期望配置
    @param [in] repeats 当前负载的期望批次数
    @param [in] memory true 检查内存记录，false 检查吞吐记录
    @param [in] standard 吞吐模式选择标准表时为 true
    @param [out] result 成功解析得到的数字字段
    @return 后端与负载均符合约定时为 true
    */
    static bool verifyObservation(std::istream& input, const LockFreeHashMapPerfSpec& spec,
        int32_t repeats, bool memory, bool standard, std::map<std::string, double>& result);

    /** 返回各轮数值的中位数
    @param [in] values 非空数值副本，会排序
    @return 奇数取中间，偶数取中间两数均值
    */
    static double median(std::vector<double> values);
};