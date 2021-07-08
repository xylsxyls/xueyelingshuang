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
	*/
	void setParam(std::vector<ProcessReceiveCallback*>* callback,
		LockFreeQueue<char*>* receiveQueue,
		Semaphore* receiveSemaphore);

private:
	std::vector<ProcessReceiveCallback*>* m_callback;
	LockFreeQueue<char*>* m_receiveQueue;
	Semaphore* m_receiveSemaphore;
	std::atomic<bool> m_exit;
};