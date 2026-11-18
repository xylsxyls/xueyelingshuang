#pragma once
#include <stdint.h>

/** 测试进程的真实 new 故障注入，仅作用于调用线程，生产库不切换实现
*/
class CTaskFaultAllocation
{
public:
    /** 开始计数，指定第几次申请失败一次
    @param [in] nth 1 开始的失败位置，UINT64_MAX 只计数
    */
    static void begin(uint64_t nth);

    /** 从指定位置开始让本线程后续全部申请失败，验证持续低内存收尾
    @param [in] nth 1开始的首个失败位置，结束时必须调用end
    */
    static void beginPersistent(uint64_t nth = 1);

    /** 结束本线程注入
    @return 本轮观察的申请次数
    */
    static uint64_t end();

    /** 查询本轮是否实际触发故障
    @return 是否抛出过注入的 bad_alloc
    */
    static bool triggered();

};