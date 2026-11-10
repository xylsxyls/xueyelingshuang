#pragma once
#include "LockFreeMap/LockFreeMapAPI.h"
#include <stdint.h>

/** 一个线程独占的分窗口观察，结束并回收线程后才读取
*/
struct LockFreeMapArrivalWorker
{
public:
    // 四个连续二百毫秒窗口的实际调用数
    uint64_t m_attempts[4];
    // 按调用完成时刻统计四个窗口内的成功次数
    uint64_t m_successes[4];
    // 开始观察后发起的调用最终成功总数，包含跨过末个窗口结束的调用
    uint64_t m_completed;
    // 本线程在观察阶段消耗的 CPU 时间；平台无法查询时为 UINT64_MAX
    uint64_t m_cpuNanoseconds;
    // 成功调用的最长墙钟耗时，用于定位跨窗口停顿
    uint64_t m_maxCallNanoseconds;
    // 相对共同起跑的首次调用和末次成功返回时刻
    uint64_t m_firstCallNanoseconds;
    uint64_t m_lastReturnNanoseconds;
    // 开始和结束时的线程处理器组、允许集合，用于辨别进程集合之外的线程限制
    unsigned m_processorGroup;
    uint64_t m_allowedProcessors;
    unsigned m_finalProcessorGroup;
    uint64_t m_finalAllowedProcessors;
    // 每个窗口首次发起调用时所在组内的逻辑 CPU；未调用或无法查询时为 UINT_MAX
    unsigned m_firstProcessor[4];
    // 非预期状态或载荷错误
    unsigned m_errors;
    // 分离相邻工作线程频繁修改的计数
    unsigned char m_padding[64];

public:
    /** 将全部观察计数初始化为零
    */
    LockFreeMapArrivalWorker();
};

/** 检查已有热线程持续运行时，后来加入的同类生产或消费线程仍逐窗口推进
*/
class LockFreeMapArrivalTest
{
private:
    // 固定二百毫秒窗口；类常量兼容 VS2013 的 lambda 常量访问
    enum { WINDOW_NANOSECONDS = 200000000 };

public:
    /** 运行两种先到线程数和四种竞争负载，核对提交结果与最终资源释放
    @return 所有线程在各观察窗口都有成功且内容正确时返回 true
    */
    static bool run();

private:
    /** 查询当前线程累计 CPU 时间，仅提供诊断，不参与公平性通过条件
    @return 纳秒数；平台不支持或查询失败时为 UINT64_MAX
    */
    static uint64_t cpuNanoseconds();

    /** 读取当前线程实际允许的处理器集合，不修改线程或进程设置
    @param [out] group 处理器组号；无法查询时为 UINT_MAX
    @return 组内允许集合；无法查询时为 UINT64_MAX
    */
    static uint64_t affinity(unsigned& group);

    /** 让部分线程先运行，再加入其余线程共同观察四个二百毫秒窗口
    @param [in] mode 0 状态覆盖，1 旧接口覆盖，2 独立键删除重插，3 同键条件累加
    @param [in] incumbents 先到线程数，为一或四；总线程数为八
    @return 同类线程全部推进、内容及资源正确时返回 true
    */
    static bool scenario(unsigned mode, unsigned incumbents);

    /** 执行一次调用，条件版本失效允许未提交，其他失败计为错误
    @param [in,out] map 被测容器
    @param [in,out] result 当前线程的私有观察
    @param [in] mode 负载类型，范围零至三
    @param [in] id 当前线程编号，范围零至七
    @return 本轮成功提交时返回 true
    */
    static bool attempt(LockFreeMap<int, uint64_t>& map, LockFreeMapArrivalWorker& result, unsigned mode, unsigned id);
};