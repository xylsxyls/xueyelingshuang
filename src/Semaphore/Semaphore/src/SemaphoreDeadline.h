#pragma once
#include <stdint.h>

/** 一次调用的单调时间预算，不在重复唤醒或组合操作的第二阶段重新计时
*/
class SemaphoreDeadline
{
public:
    /** 从当前时刻开始计时，无限等待不读取时钟
    @param [in] infinite 是否无限等待
    @param [in] milliseconds 有限等待总毫秒数
    */
    SemaphoreDeadline(bool infinite, uint32_t milliseconds);

    /** 读取剩余片段，超时返回0
    @param [in] maximum 本次允许等待的最大毫秒数
    @return 不超过maximum的剩余毫秒数
    */
    uint32_t remaining(uint32_t maximum = UINT32_MAX) const;

    /** 取得单调毫秒数，供等待和连续空闲时间使用
    @return 单调毫秒时间
    */
    static uint64_t now();

private:
    // 无限预算不建立截止时间
    bool m_infinite;
    // 调用开始时间；使用时间差避免截止时间加法溢出
    uint64_t m_started;
    // 初始预算，单位毫秒
    uint32_t m_milliseconds;
};