#pragma once
#include <stdint.h>
#include <string>
#include <vector>

/** 测试专用独立进程执行器，超时只终止本次创建且仍持有身份的子进程
*/
class SemaphoreTestProcess
{
public:
    /** 取得本测试程序的绝对路径
    @return 当前程序路径，无法取得时抛出异常交给主入口记录
    */
    static std::string executable();

    /** 执行子进程并将标准输出及错误写入独立文件
    @param [in] executable 程序绝对路径
    @param [in] arguments 各个参数，不能含NUL
    @param [in] log 本次日志文件，不覆盖已有报告
    @param [in] timeout 总预算毫秒
    @return 程序退出码，124超时，125执行器或清理失败
    */
    static int32_t run(const std::string& executable, const std::vector<std::string>& arguments, const std::string& log,
        uint32_t timeout);

    /** 计算Windows文件的SHA256，其他环境明确标记未提供哈希能力
    @param [in] path 输入文件完整路径
    @return SHA256十六进制或非Windows的NOT_AVAILABLE，Windows失败抛出异常
    */
    static std::string hash(const std::string& path);

    /** 创建一层输出目录，父目录必须已存在
    @param [in] path 本批新目录，已有目录允许但已有文件不得覆盖
    @return 创建成功或目录已存在
    */
    static bool makeDirectory(const std::string& path);

    /** 取得单调时钟
    @return 毫秒
    */
    static double now();
};