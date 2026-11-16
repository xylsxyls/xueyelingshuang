#pragma once

// 原生异步文件后端的真实文件、容量和失败清理用例，全部属于测试工程。
class NetNativeFileTestCases
{
public:
    /** 验证临时文件所有权、发布、错配源路径和停止清理
    */
    static void temporary();

    /** 验证低内存、反复停止、关闭唤醒、结果容量与字节预算
    */
    static void cleanup();
};