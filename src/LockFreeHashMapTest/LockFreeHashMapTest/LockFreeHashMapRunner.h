#pragma once
#include <string>
#include <vector>

/** 子进程的一次执行结果，句柄和日志由执行器在返回前释放
*/
struct LockFreeHashMapRunResult
{
    // 正常退出码，-1 为启动或读取失败，-2 为超时
    int m_code;
    // 从启动到退出的单调秒数
    double m_seconds;
};

/** C++ 原生测试控制器，隔离用例、限制截止时间并持久化原始证据
只终止自己创建的子进程，不修改 SDK，也不编译或安装工具链
*/
class LockFreeHashMapRunner
{
public:
    /** 运行单个程序并保存合并后的标准输出和错误
    @param [in] executable 已构建的可执行文件绝对路径
    @param [in] argument 仅一个不含空白的数字参数，空字符串表示无参数
    @param [in] log 日志路径，必须不存在，父目录由调用方准备
    @param [in] timeoutSeconds 正数截止时间，单位秒
    @param [in] affinityMask Windows 子进程 CPU 位掩码，0 使用默认调度；其他平台仅支持 0
    @param [in] secondArgument 0 表示不传第二参数，1 至 256 为单独的数字参数；要求第一参数非空
    @param [in] aboveNormal Windows性能测量可显式使用AboveNormal，启动前核实；其他平台仅支持false
    @return 子进程退出码及耗时；日志失败不伪报通过
    */
    static LockFreeHashMapRunResult run(const std::string& executable, const std::string& argument,
        const std::string& log, unsigned timeoutSeconds, unsigned affinityMask = 0, unsigned secondArgument = 0,
        bool aboveNormal = false);

    /** 逐项运行固定编号，失败仍保留整批证据
    @param [in] executable 用例程序路径
    @param [in] prefix 已存在目录中的新文件前缀
    @param [in] ids 要执行的非空编号列表，范围 1 至 10
    @param [in] repeats 每项执行次数，范围 1 至 100
    @return 全部通过为 true，启动、超时、报告和用例失败均为 false
    */
    static bool cases(const std::string& executable, const std::string& prefix,
        const std::vector<int>& ids, int repeats);

    /** 三版本按轮换顺序运行完全相同负载并输出中位数表
    @param [in] executables 原版、整改前版、当前版可执行文件，恰好三个
    @param [in] prefix 已存在目录中的新文件前缀
    @param [in] scale 负载倍率，范围 1 至 20
    @param [in] rounds 轮数，范围 1 至 9
    @param [in] affinityMask 三版本共用的 Windows CPU 掩码，0 使用默认调度
    @return 每轮成功、负载齐全且报告落盘成功为 true
    */
    static bool compare(const std::vector<std::string>& executables, const std::string& prefix,
        int scale, int rounds, unsigned affinityMask = 0);

    /** 严格解析一个十进制正数，不接受截断后缀、符号或溢出
    @param [in] text 非空的数字字符串
    @param [in] maximum 可接纳上界
    @param [out] value 成功写入数值，失败不变
    @return 完整输入处于 1 至 maximum 为 true
    */
    static bool number(const char* text, int maximum, int& value);
};