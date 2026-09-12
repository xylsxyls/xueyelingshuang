#pragma once
#include <atomic>
#include <stdint.h>

/** 工作线程只在全部报告落盘后发布终态，GUI不读取未完成数据 */
struct TestRunState
{
public:
    // 任务终态标记
    std::atomic<bool> m_done;
    // 已完成退出码
    std::atomic<int32_t> m_result;

public:
    /** 初始为未完成 */
    TestRunState();
};