#pragma once
#include <stdint.h>

// 为只能配置一次的后端运行独立子进程，超时只结束本用例创建的子进程。
class NetIsolatedTest
{
public:
    /** 在全新地址空间执行指定测试入口
    @param [in] argument 单个UTF8测试参数，不经过shell
    @param [in] timeoutMs 等待上限，单位毫秒
    @return 0通过，其他为子进程失败或隔离设施错误
    */
    static int32_t run(const char* argument, uint32_t timeoutMs = 10000);
};