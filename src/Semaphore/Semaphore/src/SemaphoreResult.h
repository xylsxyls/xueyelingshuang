#pragma once
#include "SemaphoreMacro.h"
#include <stdint.h>

/** 一次操作的结果；系统错误码使用当前平台的原生编码
*/
enum SemaphoreStatus
{
    // 操作完成；通知是否提交同时由committed指示
    SEMAPHORE_OK = 0,
    // 总期限到达，未取得许可或事件
    SEMAPHORE_TIMEOUT = 1,
    // 许可达到上限，本次未提交
    SEMAPHORE_OVERFLOW = 2,
    // 当前宿主不具备所请求的等待或IPC能力
    SEMAPHORE_UNSUPPORTED = 3,
    // 原生调用失败，nativeError保留原码
    SEMAPHORE_SYSTEM_ERROR = 4,
    // 未打开命名对象
    SEMAPHORE_NOT_OPEN = 5,
    // 参数或生命周期状态非法
    SEMAPHORE_INVALID = 6,
    // C++资源申请失败
    SEMAPHORE_NO_MEMORY = 7,
    // 等待记录容量（包括退役记录及扩容预留）已满
    SEMAPHORE_CAPACITY = 8,
    // 扩容策略拒绝增长或抛异常
    SEMAPHORE_POLICY = 9,
    // 扩容额度尚被其他申请预留，或新批次被其他申请先取得；本次未登记
    SEMAPHORE_RETRY = 10
};

/** 无分配的逐调用结果，不使用跨线程共享lastError推断本次提交
*/
struct SemaphoreAPI SemaphoreResult
{
public:
    // 本次状态
    SemaphoreStatus status;
    // 通知已发布或等待已消费；true时禁止重试发布
    bool committed;
    // 系统失败原码，非系统结果为0
    int32_t nativeError;

public:
    /** 构造结果
    @param [in] value 操作状态
    @param [in] published 本次是否已提交或消费
    @param [in] error 原生错误码
    */
    SemaphoreResult(SemaphoreStatus value = SEMAPHORE_OK, bool published = false, int32_t error = 0);
};

/** 组合通知/等待中每个阶段的提交状态；UNKNOWN不得当成可安全重发
*/
enum SemaphoreCommitState
{
    // 本阶段确定未发布许可或未消费许可
    SEMAPHORE_COMMIT_NONE = 0,
    // 本阶段确定已经发布或消费一个许可
    SEMAPHORE_COMMIT_DONE = 1,
    // 原生组合调用失败且无法确定阶段，须按业务代次恢复，不能重试整个组合
    SEMAPHORE_COMMIT_UNKNOWN = 2
};

/** 一次发送后等待的结果；分别报告发送和消费，避免等待超时导致重复发布
*/
struct SemaphoreAPI SemaphoreSignalWaitResult
{
public:
    // 整次调用的结果，OK同时要求发送和消费均完成
    SemaphoreStatus status;
    // 当前对象的许可发布状态
    SemaphoreCommitState signalState;
    // 目标对象的许可消费状态
    SemaphoreCommitState waitState;
    // 系统失败原码，非系统结果为0
    int32_t nativeError;

public:
    /** 构造组合操作结果
    @param [in] value 操作状态
    @param [in] signal 当前对象发布状态
    @param [in] wait 目标对象消费状态
    @param [in] error 原生错误码
    */
    SemaphoreSignalWaitResult(SemaphoreStatus value = SEMAPHORE_OK, SemaphoreCommitState signal = SEMAPHORE_COMMIT_NONE,
        SemaphoreCommitState wait = SEMAPHORE_COMMIT_NONE, int32_t error = 0);
};

#include "SemaphoreResult.inl"