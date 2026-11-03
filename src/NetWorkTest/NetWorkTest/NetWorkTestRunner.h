#pragma once

#include <stdint.h>

// 稳定登记；名称为UTF-8，压力与瞬时按显式模式选择
struct NetWorkTestEntry
{
    // 测试文档中的唯一编号
    int m_id;
    // 中文用例名，登记表生命周期
    const char* m_name;
    // 是否属于独立压力批次
    bool m_stress;
    // 执行函数；失败抛异常，由runner统一记录
    void (*m_run)();
    // 0沿用瞬时10秒/压力30秒；真实计时协议测试可声明更长但有界的预算。
    uint32_t m_timeoutSeconds;
};