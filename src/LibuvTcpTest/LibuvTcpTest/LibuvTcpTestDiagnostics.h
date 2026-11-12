#pragma once
#include "LockFreeQueue/LockFreeQueueAPI.h"

struct LibuvBase;
struct LibuvPendingSend;

/** 仅供隔离诊断构建调用的真实预算观察器，不伪造接纳
@param [in,out] state 被测实例
@param [in] charge 本次预算
@return 原接口的真实结果
*/
bool LibuvTestReserve(LibuvBase* state, uint32_t charge);

/** 仅供隔离诊断构建观察真实入队状态；普通性能构建不使用
@param [in,out] state 被测实例
@param [in] pending 本次待发送项
@return 原接口的真实状态
*/
LockFreeQueueResult LibuvTestPush(LibuvBase* state, const LibuvPendingSend& pending);

// 隔离诊断构建的线程私有统计；采样不进入正式 LibuvTcp 源码或性能验收构建
struct LibuvSubmitCounters
{
    // 提交调用总数
    uint64_t m_calls;
    // 各阶段采样耗时，单位纳秒
    uint64_t m_nanoseconds[6];
    // 各阶段采样次数
    uint64_t m_samples[6];
    // 队列结果计数，最后一项保留给未知状态
    uint64_t m_results[16];
};

// 只对每 1024 次提交采样一次，计数由当前线程独占，结束后由测试主线程汇总
class LibuvSubmitProfile
{
public:
    /** 开始一次提交观察，仅隔离诊断构建调用
    */
    LibuvSubmitProfile();

    /** 记录从上个检查点到当前阶段的耗时
    @param [in] stage 阶段编号，范围 0 至 5
    */
    void mark(uint32_t stage);

    /** 记录一次队列结果
    @param [in] result 原始队列状态，不改变其结果
    */
    void queued(int32_t result);

private:
    // 当前线程的独占统计槽，超出诊断容量时为空
    LibuvSubmitCounters* m_counters;
    // 上次采样时间，零表示本次不采样
    uint64_t m_previous;
};

/** 在全部提交线程退出后输出诊断计数；不可与提交并发调用
*/
void LibuvDumpSubmitProfiles();