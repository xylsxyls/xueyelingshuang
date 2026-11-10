#pragma once
#include "CTaskThreadTestConfig.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <functional>

class CTaskThread;

/** 测试工程私有的白盒访问入口，仅链接正式静态库，不进入生产 API 或导出表
*/
#ifndef CTTM_TEST_DLL
class CTaskThreadTestAccess
{
public:
    /** 静止设置线程记录序号，部分尾段仅供当前测试线程随后投递
    @param [in,out] thread 空闲线程，外部保证没有投递
    @param [in] sequence 已分配的最后序号
    */
    static void setTaskSequence(CTaskThread& thread, uint64_t sequence);

    /** 把累计计数移至回绕边界；测试必须阻塞唯一消费者并禁止并发投递
    @param [in,out] thread 仅有一个正在执行的受控任务、没有排队项的线程
    */
    static void seedPendingCounters(CTaskThread& thread);

    /** 静止设置管理器编号，测试进程结束后不再正常创建线程
    @param [in] sequence 已分配的最后编号
    */
    static void setManagerSequence(uint32_t sequence);

    /** 读取任务目录数量
    @param [in,out] thread 线程对象
    @return 观察到的记录数；已经退出返回 0
    */
    static size_t recordCount(CTaskThread& thread);

    /** 读取优先级桶数
    @param [in,out] thread 线程对象
    @return 观察数量
    */
    static size_t bucketCount(CTaskThread& thread);

    /** 关闭一个空桶但不摘除，验证生产者能帮助完成删除
    @param [in,out] thread 工作线程
    @param [in] level 空桶级别
    @return 是否关闭成功
    */
    static bool closeIdleBucket(CTaskThread& thread, int32_t level);

    /** 模拟已完成投递的迟到抢占通知
    @param [in,out] thread 正在执行低优先级任务的线程
    @return 通知处理结果
    */
    static CTaskThreadResult latePreempt(CTaskThread& thread);

    /** 在真实目录保护范围内执行测试回调，模拟暂停读者
    @param [in,out] thread 目标线程
    @param [in] held 有界测试回调；需要目标至少包含一个记录
    @return 遍历结果
    */
    static CTaskThreadResult holdDirectoryRead(CTaskThread& thread, const std::function<void()>& held);

    /** 占用管理器的真实 Map 保护记录
    @param [in] held 有界回调；管理器必须包含至少一个线程
    @return 是否进入回调
    */
    static bool holdManagerRead(const std::function<void()>& held);

};
#endif