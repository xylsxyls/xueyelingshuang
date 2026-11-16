#pragma once

// 仅独立测试子进程在构造时拒绝nothrow分配，生产库不含故障开关。
class NetTlsAllocationTest
{
public:
    /** 在新进程运行构造失败及全部安全查询用例
    */
    static void run();

    /** 在任何后台线程启动前运行用例，退出前恢复分配
    @return 0通过，1失败
    */
    static int check();

    /** 供本测试程序的nothrow分配替身读取
    @return 当前独立进程是否处于受控故障区间
    */
    static bool refusing();

private:
    // 只在无后台线程的专用子进程中修改，普通测试进程始终为false。
    static bool m_refusing;
};