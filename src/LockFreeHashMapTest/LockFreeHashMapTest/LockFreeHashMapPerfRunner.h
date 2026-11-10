#pragma once
#include "LockFreeHashMapPerfModels.h"
#include <map>
#include <string>
#include <vector>

/** 两版或三版共用的 C++ 性能控制器，校准、隔离、轮换执行并保留所有轮次
*/
class LockFreeHashMapPerfRunner
{
public:
    /** 执行完整比较和独立内存测试
    @param [in] programs 先放两版或三版公共性能，再按同一顺序放内存程序，末尾放当前新增接口专项程序
    @param [in] prefix 新输出前缀，父目录须存在
    @param [in] rounds 1 至 9 轮
    @param [in] affinityMask 0 默认调度，非零为 Windows 共用 CPU 掩码
    @param [in] first 第一个配置编号，1 开始
    @param [in] last 最后一个配置编号，0 表示全部；局部运行不重复内存目录
    @param [in] revisions 两个近期修订均支持新接口时，把新增接口也纳入两版比较
    @param [in] fixedRepeats 0 使用校准，1 至 64 固定两版的批次数以复核短样本波动；clear 仍最多 4 批
    @param [in] iterationMultiplier 1 至 256，非一只接受持续 get/set 负载，必须使用支持倍率参数的两版性能程序
    @return 所有数据和日志校验通过为 true
    */
    static bool compare(const std::vector<std::string>& programs, const std::string& prefix,
        int32_t rounds, unsigned affinityMask, int32_t first, int32_t last, bool revisions = false,
        int32_t fixedRepeats = 0, size_t iterationMultiplier = 1);

    /** 单独运行两版或三版内存矩阵，不重跑已完成的吞吐测试
    @param [in] programs 原版、当前版及可选 std 版的内存计量程序
    @param [in] prefix 新输出前缀，父目录须存在
    @param [in] affinityMask 共用 CPU 掩码
    @return 全部独立进程通过数据和资源核对为 true
    */
    static bool compareMemory(const std::vector<std::string>& programs, const std::string& prefix, unsigned affinityMask);

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
    /** 读取本次性能测试显式优先级选项，缺省不提升；非法环境值拒绝执行
    @return LOCKFREEHASHMAP_PERF_ABOVE_NORMAL为1时返回true，为0或未设置时返回false
    */
    static bool aboveNormal();

    /** 执行和读取一次隔离负载，失败仍保留日志
    @param [in] program 可执行文件
    @param [in] spec 配置
    @param [in] repeats 批次数
    @param [in] memory 内存模式
    @param [in] log 不存在的日志路径
    @param [in] mask CPU 掩码
    @return 完整合法字段，失败抛异常由入口报告
    */
    static std::map<std::string, double> measure(const std::string& program,
        const LockFreeHashMapPerfSpec& spec, int32_t repeats, bool memory, const std::string& log, unsigned mask);

    /** 返回各轮数值的中位数
    @param [in] values 非空数值副本，会排序
    @return 奇数取中间，偶数取中间两数均值
    */
    static double median(std::vector<double> values);
};