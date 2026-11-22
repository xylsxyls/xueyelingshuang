#pragma once
#include <string>

// 在独立子进程核对管理单例、任务停止、信号恢复及分配失败回收。
class NetClientManagerTest
{
public:
    /** 创建隔离子进程并等待完整验证
    */
    static void run();

    /** 在未配置宿主的新进程中运行真实管理类
    @param [in] root 当前测试进程独占的输出目录
    @return 0通过，非0失败
    */
    static int check(const std::string& root);

    /** 作为NetGatewayTestClient测试宿主运行实际客户端管理类
    @param [in] argc 原始参数数量
    @param [in] argv 原始参数，Windows由系统读取UTF8参数
    @return 管理进程退出码
    */
    static int host(int argc, char** argv);
};