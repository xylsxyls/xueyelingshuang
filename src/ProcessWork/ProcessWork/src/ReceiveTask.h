#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>
#include "LockFreeQueue/LockFreeQueueAPI.h"

class ProcessReceiveCallback;
class Semaphore;

/** 接收任务
*/
class ReceiveTask : public CTask
{
public:
	/** 构造函数
	*/
	ReceiveTask();

public:
	/** 执行任务
	*/
	void DoTask();

	/** 中断任务
	*/
	void StopTask();

	/** 设置参数
	@param [in] callback 接收回调类
	@param [in] receiveQueue 接收队列
	@param [in] receiveSemaphore 接收信号量
    @param [in] fullCount 合并的满载事件，借用至任务加入完成
	*/
	void setParam(std::vector<ProcessReceiveCallback*>* callback,
		LockFreeQueue<char*>* receiveQueue,
		Semaphore* receiveSemaphore, std::atomic<int32_t>* fullCount = nullptr);

private:
    // 合并后的满载快照，0表示没有待分派事件
    std::atomic<int32_t>* m_fullCount;
	std::vector<ProcessReceiveCallback*>* m_callback;
	LockFreeQueue<char*>* m_receiveQueue;
	Semaphore* m_receiveSemaphore;
	std::atomic<bool> m_exit;
};