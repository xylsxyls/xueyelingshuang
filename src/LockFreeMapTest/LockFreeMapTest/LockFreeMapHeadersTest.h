#pragma once
#include "LockFreeMap/LockFreeMapAPI.h"
#include "LockFreeHashMap/LockFreeHashMapAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <string>
#include <memory>

/** 隔离准入和通知适配器，信号量实际等待由其独立测试负责
*/
struct LockFreeMapTestBucket
{
public:
    // 桶拥有的真实单消费者队列
    LockFreeQueue<int> m_tasks;
    // 准入状态，仅用于受控时序验证
    std::atomic<bool> m_accepting;
    // 通知适配器仅在成功发布后增加许可数
    std::atomic<int> m_permits;

public:
    /** 初始化允许准入的空桶
    */
    LockFreeMapTestBucket();

    /** 先发布再通知，并拒绝借用已关闭桶的生产者
    @param [in] value 任务编号
    @return 适配器接受该任务时返回 true
    */
    bool post(int value);
};