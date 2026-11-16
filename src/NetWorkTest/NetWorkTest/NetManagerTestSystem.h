#pragma once
#include "CSystem/CSystemAPI.h"
#include <string>
#include <vector>

// 管理类测试隔离租约并剥离测试宿主参数，其他系统调用仍使用实际实现。
class NetManagerTestSystem : public CSystem
{
public:
    /** 读取实际平台命令行并剥离测试程序专用的宿主选择参数
    @param [in] argc 原始参数数量
    @param [in] argv 原始参数，Windows仍由CSystem读取Unicode命令行
    @param [out] arguments 交给生产管理类的参数，不包含--node-host
    @return 是否成功读取参数
    */
    static bool commandArguments(int argc, char** argv, std::vector<std::string>& arguments);

    /** 为当前测试子进程取得独立租约，不接触生产网关租约
    @param [in] name 生产角色名
    @param [in] replace 保留生产替换语义
    @param [in] timeoutMs 保留生产等待上限
    @param [out] state 实际系统租约，仍由CSystem释放
    @param [in] isolated 测试调用不接受外部作用域
    @return CSystem实际租约结果
    */
    static int32_t acquireProcessLease(const std::string& name, bool replace, uint32_t timeoutMs,
        std::vector<uint64_t>& state, const std::string& isolated = std::string());
};

// 仅管理类生命周期用例使用；真实守护启动、复用和恢复另由多进程用例验证。
class NetManagerTestWatchdog
{
public:
    /** 测试进程不等待生产守护，也不读取其配置
    @return false，没有需要复用的测试守护
    */
    static bool waitForRecovery();

    /** 模拟监视句柄交接成功，不创建后台常驻程序
    @param [in] arguments 生产解析后的参数，本替身不保留
    @return true，允许进入待测试的任务生命周期
    */
    static bool start(const std::vector<std::string>& arguments);
};