#pragma once
#include "CTaskThreadModels.h"
#include <memory>
#include <vector>

class CTaskThread;
class Semaphore;

/** 基础库状态统一为管理器的逐调用结果，不丢弃提交位
*/
class CTaskResultAdapter
{
public:
    /** 检查已构造的本地通知后端，启动线程前拒绝原生等待资源初始化失败
    @param [in] semaphore 尚未参与并发调用的通知对象，不在此函数重新初始化
    @return 成功为Ok且未提交；失败保留内存、平台或系统状态及原生错误码
    */
    static CTaskThreadResult semaphoreInitialization(const Semaphore& semaphore);

    /** 内部竞争重试时让出执行机会，不持有容器保护或未提交资源
    @param [in,out] attempts 当前退避阶段，周期性归零
    @return Ok 表示可以继续；系统退避异常返回具体错误
    */
    static CTaskThreadResult pause(uint32_t& attempts);

    /** 容量等待先快速重试，达到阈值后每次休眠
    @param [in,out] attempts 快速重试次数，饱和于 retries
    @param [in] retries 进入休眠前的重试数
    @param [in] milliseconds 每次休眠毫秒数
    @return Ok可以重试；宿主不支持或系统失败明确返回错误
    */
    static CTaskThreadResult pauseCapacity(uint32_t& attempts, uint32_t retries = 4096, uint32_t milliseconds = 1);

    /** 复制线程强引用后释放 Map 保护，后续等待或回调不占用容器保护记录
    @param [in] threads 线程注册表
    @param [out] snapshot 弱一致的线程强引用集合，分配失败时清空
    @return 复制结果；暂时没有保护记录在内部退避重试
    */
    static CTaskThreadResult snapshot(const LockFreeMap<uint32_t, std::shared_ptr<CTaskThread>>& threads,
        std::vector<std::shared_ptr<CTaskThread>>& snapshot);

    /** 转换 FIFO 操作结果
    @param [in] status 基础库状态
    @return 管理器状态
    */
    static CTaskThreadResult queue(LockFreeQueueResult status);

    /** 转换有序表状态
    @param [in] status 基础库状态
    @return 管理器状态
    */
    static CTaskThreadResult map(LockFreeMapStatus status);

};