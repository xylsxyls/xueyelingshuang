#pragma once

#if defined(CTTM_BENCH_SAMPLER) && defined(_WIN32)
#include <stdint.h>
#include <map>
#include <string>
#include <Windows.h>

class CTaskPreparedState;

/** 测试工程专用的消费线程地址采样；会扰动调度，结果仅用于定位，不能当吞吐验收数据
*/
class CTaskPreparedProfiler
{
public:
    /** 打开已经预热的消费线程；调用方在业务开始后逐次采样
    @param [in,out] state 基准状态，必须长于本对象
    @param [in] threadId 预热任务记录的系统线程编号
    */
    CTaskPreparedProfiler(CTaskPreparedState& state, DWORD threadId);

    /** 关闭采样目标句柄，不拥有或关闭工作线程
    */
    ~CTaskPreparedProfiler();

    /** 暂停后无分配，取得指令地址后立即恢复目标；由基准主线程串行调用
    */
    void sample();

    /** 停止采样并在业务完成后解析符号
    @param [in] path 独立诊断 CSV 路径
    */
    void write(const std::string& path);

private:
    // 借用的基准状态
    CTaskPreparedState* m_state;
    // 仅拥有查询和短暂暂停目标所需权限的句柄
    HANDLE m_target;
    // 生产仍在进行时的地址频数
    std::map<uint64_t, size_t> m_overlap;
    // 生产结束后的地址频数
    std::map<uint64_t, size_t> m_drain;
};
#endif