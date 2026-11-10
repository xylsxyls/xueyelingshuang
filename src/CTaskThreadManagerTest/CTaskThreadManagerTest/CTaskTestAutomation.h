#pragma once
#include "BlackBoxTestSupport.h"
#include <map>

/** 测试工程的原生批量执行与汇总，不进入被测库或计时循环
*/
class CTaskTestAutomation
{
public:
    /** 执行矩阵、重复回归、报告汇总或测试私有 Queue 夹具准备
    @param [in] argc 从子命令开始的参数个数
    @param [in] argv 子命令及参数，调用期间借用
    @return 0 为成功，其他值为参数、构建或验证失败
    */
    static int32_t run(int32_t argc, char** argv);

    /** 自检数值汇总、严格 CSV 解析、超时收尾和进程作用域
    */
    static void selfTest();

    /** 执行已登记的一项公平性矩阵，保持完整独立判定
    @param [in] index 0 到 16，最后三项使用一个 Queue 登记位置
    */
    static void fairness(size_t index);

private:
    /** 严格读取 CSV，拒绝列数错误、重复列及无数据文件
    @param [in] path 原始结果路径
    @return 表头及所有数据行
    */
    static std::vector<std::vector<std::string>> readCsv(const std::string& path);

    /** 求有限数值的中位数，不使用最大单轮代替
    @param [in] values 非空数值集合
    @return 中位数，偶数项取中间两项均值
    */
    static double median(std::vector<double> values);

    /** 汇总各可执行程序的同场景原始数据
    @param [in] output 已创建的独立批次目录
    @param [in] reports 输入原始 CSV 路径
    @param [in] labels 对应程序标签，第一种为比较基线
    */
    static void summarize(const std::string& output, const std::vector<std::string>& reports,
        const std::vector<std::string>& labels);

    /** 建立一个不存在的批次目录，拒绝覆盖已有结果
    @param [in] path 由调用方明确指定的目录
    */
    static void directory(const std::string& path);

    /** 在自有子进程中执行一种测试入口
    @param [in] program 可执行程序路径
    @param [in] arguments 参数数组，不经过命令解释器
    @param [in] log 输出日志
    @param [in] seconds 最长等待秒数
    */
    static void execute(const std::string& program, const std::vector<std::string>& arguments,
        const std::string& log, int32_t seconds);

    /** 只在新建输出目录复制并插入 Queue 测试边界，然后编译和执行
    @param [in] root 仓库根目录
    @param [in] output 不存在的自有输出目录
    @param [in] compiler 已配置环境的 C++ 编译器路径
    */
    static void queueRace(const std::string& root, const std::string& output, const std::string& compiler);
};