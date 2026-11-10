#pragma once
#include <string>
#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#endif

/** 仅测试工程使用的子进程调试器，捕获未处理异常，不修改被测 EXE 或生产头文件
*/
class LockFreeHashMapCrashRunner
{
public:
    /** 独立执行一个原始测试 EXE；未处理异常保存栈和小型转储
    @note Windows 调试器与被测 EXE 位数必须一致
    @param [in] executable 被测 EXE 的绝对路径
    @param [in] argument 一个已校验的数字参数
    @param [in] prefix 不存在的输出前缀，父目录须存在
    @return 子进程退出码；设施失败为 -1，超过 120 秒为 -2
    */
    static int run(const std::string& executable, const std::string& argument, const std::string& prefix);

    /** 读取小型转储的模块、寄存器和栈原始字节，不运行被测程序
    @param [in] path 转储绝对路径，最多 16 MiB
    @param [in] executable 可选的匹配主程序路径；栈字的符号仅作线索，不冒充完整回溯
    @return 内容完整并成功输出为 true
    */
    static bool inspect(const std::string& path, const std::string& executable = std::string());

private:
#if defined(_WIN32)
    /** 子进程暂停在首次访问违例或未处理异常时采集现场，不在子进程堆上执行诊断
    @param [in] process 本次创建的子进程句柄
    @param [in] processId 对应进程编号
    @param [in] threadId 异常线程编号
    @param [in] exception 原始异常通知；超时诊断传空，不伪造异常
    @param [in] executable 被测 EXE 路径，同目录应保留匹配的 PDB
    @param [in] prefix 栈文本和转储的全新前缀
    @return 栈及转储均保存成功为 true
    */
    static bool capture(HANDLE process, DWORD processId, DWORD threadId, const EXCEPTION_DEBUG_INFO* exception,
        const std::string& executable, const std::string& prefix);

    /** 超时后逐个暂停本次子进程线程并采集栈；这是诊断，不参与性能测量
    @param [in] process 本次创建且尚未退出的子进程句柄
    @param [in] processId 对应进程编号，句柄保持存活以核对所有权
    @param [in] executable 被测 EXE 路径
    @param [in] prefix 全新的超时诊断前缀，每条线程单独输出
    @return 至少一个线程的现场保存成功为 true
    */
    static bool captureTimeout(HANDLE process, DWORD processId, const std::string& executable, const std::string& prefix);
#endif
};