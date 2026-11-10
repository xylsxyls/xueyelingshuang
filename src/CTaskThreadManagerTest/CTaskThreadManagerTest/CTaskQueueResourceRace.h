#pragma once
#include "LockFreeQueue/LockFreeQueuePolicy.h"
#include "LockFreeQueue/LockFreeQueueEntry.h"

template <class T> class LockFreeQueue;

/** 在测试副本的扫描边界执行唯一消费者动作，不向生产源码添加测试入口
*/
class CTaskQueueResourceRace
{
public:
    /** 验证初次扫描无额度后消费者归还整批额度的交错
    @param [in] entryOnly 是否使用外部 Entry 额度模式
    @return 是否接纳后继并保留精确 FIFO
    */
    static bool run(bool entryOnly);

    /** 仅供测试副本插入的调用使用，消费此前已经填满的 63 项
    */
    static void afterEmptyScan();

    /** 验证全局归还统计已发布、块内额度尚未归还时仍能独立扩容
    @return 是否在归还者尚未继续前成功入队，且最终 FIFO 正确
    */
    static bool runPendingReturn();

    /** 在测试副本的归还两阶段之间发起一次独立生产操作
    */
    static void afterReturnPublished();

private:
    // 当前测试借用的队列，只在同步测试调用期间有效
    static LockFreeQueue<int32_t>* m_queue;
    // 一次性触发扫描与归还的交错
    static bool m_armed;
    // 消费的 FIFO 和数量是否正确
    static bool m_valid;
    // 非空时在全局归还发布后提交此 Entry，测试栈保持其寿命
    static LockFreeQueueEntry<int32_t>* m_pendingEntry;
    // 在块内归还仍未发生时得到的入队结果
    static LockFreeQueueResult m_pendingResult;
};