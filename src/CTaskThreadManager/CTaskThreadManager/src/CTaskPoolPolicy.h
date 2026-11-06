#pragma once
#include "CTaskPoolModels.h"

/** 独立扩缩容策略；外部继承并重写 evaluate，算法无需自行处理线程数上限
*/
class CTaskThreadManagerAPI CTaskPoolPolicy
{
public:
    /** 释放派生策略
    */
    virtual ~CTaskPoolPolicy();

    /** 根据负载决定目标规模；每个池只由控制线程调用
    @param [in] load 初始容量、成功增长次数、当前容量、任务负载及时间
    @return 默认全忙且积压时增长约 50%，全池空闲 1 秒后每轮缩一个
    */
    virtual CTaskPoolDecision evaluate(const CTaskPoolLoad& load);
};