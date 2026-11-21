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
    /** 静止注入三种Semaphore初始化错误，检查工作线程及控制线程在启动前拒绝
    三处通知分别读回九项等待策略和双池容量，验证非法设置保留原值；注入失败后核对原生错误、未提交及新实例恢复
    */
    static void semaphoreInitializationTest();

    /** 由真实优先级桶初始化队列，读取原生队列字段核对传入参数。
    @param [in] config 已指定逐级覆盖项的线程配置
    @param [in] level 静止构造的桶级别
    */
    static void verifyQueueSettings(const CTaskThreadConfig& config, int32_t level);

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
    @return 真实观察到的记录数；退出后检查静止存储，保护不足或清理尚在进行时抛测试异常
    */
    static size_t recordCount(CTaskThread& thread);

    /** 读取优先级桶数
    @param [in,out] thread 线程对象
    @return 真实观察数量；清理尚在进行且无法安全观察时抛测试异常
    */
    static size_t bucketCount(CTaskThread& thread);

    /** 观察实际投递是否已经到达容量不足路径
    @param [in,out] thread 门控消费者、尚未释放容量的线程
    @return 是否出现资源压力；只供独立新建的容量用例判断触发前提
    */
    static bool capacityPressure(CTaskThread& thread);

    /** 在门控用例开始新一轮投递前清除旧压力提示，不修改接纳额度
    @param [in,out] thread 消费者已门控且没有其他投递调用的线程
    */
    static void resetCapacityPressure(CTaskThread& thread);

    /** 关闭允许退役的空桶但不摘除，验证生产者能帮助完成删除
    @param [in,out] thread 工作线程
    @param [in] level 非固定桶级别；调用方须控制消费者停在其他桶且本桶已排空
    @param [in] interveningTask 可空。非空时在观察为空与尝试关闭之间经公开接口完成一次投递
    @return 是否关闭成功
    */
    static bool closeIdleBucket(CTaskThread& thread, int32_t level, const std::shared_ptr<CTask>& interveningTask);

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
    @return 是否取得保护且回调正常完成，未调用或回调抛异常返回 false
    */
    static bool holdManagerRead(const std::function<void()>& held);

};
#endif