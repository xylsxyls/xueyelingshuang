#pragma once

#include "CTaskThreadTestConfig.h"
#ifndef CTTM_TEST_DLL
#include "CTaskThreadManager/CTask.h"
#include <atomic>

/** 统计任务引用是否在读侧保护结束前被提前释放
*/
class CTaskStorageTestLifetimeTask : public CTask
{
public:
    /** 建立一个计入存活统计的空任务
    @param [in,out] live 生命周期长于本任务的统计计数
    */
    explicit CTaskStorageTestLifetimeTask(std::atomic<size_t>& live);

    /** 从存活统计扣除本任务
    */
    ~CTaskStorageTestLifetimeTask();

    /** 空业务执行
    */
    void doTask();

private:
    // 测试调用方持有的存活统计
    std::atomic<size_t>* m_live;
};

/** 验证读侧保护域的进入、回收边界及并发关闭，不向生产库注入测试入口
*/
void taskReadDomainTest();

/** 验证任务记录池的有限预算、延迟回收、动态策略和并发独占分配
*/
void taskStorageTest();

/** 验证增长拒绝、真实分配失败回滚和多个关闭批次的缩容下限
*/
void taskStorageFailureTest();

/** 验证不附带空观察回收的消费接口跨段保持 FIFO 并可恢复正常维护
*/
void taskQueueReadyTest();

/** 有界队列的分布计数、持有生产者上下文及并行消费精确性
*/
void taskQueueCounterTest();

/** 构造可控的复制赋值失败，验证有界消费的部分提交边界
*/
class CTaskStorageTestAssignmentValue
{
public:
    // 期望交付值
    int m_value;
    // 复制此值时在写入输出之前抛异常
    bool m_fail;

public:
    /** 建立普通值或故障值
    @param [in] value 期望值
    @param [in] fail 是否使复制赋值失败
    */
    CTaskStorageTestAssignmentValue(int value = -1, bool fail = false);

    /** 复制构造保持故障属性，但不抛异常
    @param [in] source 原始值
    */
    CTaskStorageTestAssignmentValue(const CTaskStorageTestAssignmentValue& source);

    /** 按故障属性决定复制赋值是否成功
    @param [in] source 期望源值
    @return 本输出
    */
    CTaskStorageTestAssignmentValue& operator=(const CTaskStorageTestAssignmentValue& source);

};

/** 验证前缀预算、跨段顺序和输出异常后的精确部分提交
*/
void taskQueueBatchTest();

/** 可复制构造但只能移动赋值，验证新增 Entry 分支不破坏普通队列模板实例化
*/
class CTaskMoveAssignmentValue
{
public:
    // 单进程单用例的复制构造故障开关；用例离开故障段前恢复
    static bool m_failCopy;

public:
    /** 创建可校验的载荷
    @param [in] value 期望值
    */
    explicit CTaskMoveAssignmentValue(int value = 0);

    /** 普通 push 使用复制构造
    @param [in] source 原始载荷
    */
    CTaskMoveAssignmentValue(const CTaskMoveAssignmentValue& source);

    /** 禁止复制赋值
    @param [in] source 不可复制赋值的源对象
    @return 当前对象
    */
    CTaskMoveAssignmentValue& operator=(const CTaskMoveAssignmentValue& source) = delete;

    /** 不抛异常地移动赋值，供普通 pop 使用
    @param [in,out] source 移动后改为负数的源对象
    @return 当前对象
    */
    CTaskMoveAssignmentValue& operator=(CTaskMoveAssignmentValue&& source) throw();

public:
    // 独立预期值
    int m_value;
};

/** 验证外部载荷的借用期、混合 FIFO、异常前缀、物理额度及并发交付
*/
void taskQueueEntryTest();

/** 验证只借用模式的有限额度、扩缩容、失败回滚、载荷借用和并发完整交付
*/
void taskEntryQuotaTest();

/** 验证 Map 缓冲区维护不丢键值，能回收空闲缓冲并在释放后重新使用
*/
void taskMapMaintenanceTest();

/** 验证吞吐基准的在线顺序检查能够识别总数相同的重复/丢失和乱序
*/
void taskBenchmarkValidatorTest();

/** 验证最高待检查位置的逐级下调、重新发布和多生产者交错
*/
void taskReadyIndexTest();

#endif